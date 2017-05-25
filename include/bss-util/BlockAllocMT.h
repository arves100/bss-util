// Copyright �2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"

#ifndef __BSS_ALLOC_BLOCK_LOCKLESS_H__
#define __BSS_ALLOC_BLOCK_LOCKLESS_H__

#include "BlockAlloc.h"
#include "lockless.h"

namespace bss {
  /* Multi-producer multi-consumer lockless fixed size allocator */
  template<class T>
  class BSS_COMPILER_DLLEXPORT LocklessBlockAlloc
  {
    typedef BlockAllocVoid::FIXEDLIST_NODE NODE;
    LocklessBlockAlloc(const LocklessBlockAlloc& copy) BSS_DELETEFUNC
      LocklessBlockAlloc& operator=(const LocklessBlockAlloc& copy) BSS_DELETEFUNCOP
  public:
    inline LocklessBlockAlloc(LocklessBlockAlloc&& mov) : _root(mov._root)
    {
      _freelist.p = mov._freelist.p;
      _freelist.tag = mov._freelist.tag;
      mov._freelist.p = mov._root = 0;
      _flag.clear(std::memory_order_relaxed);
    }
    inline explicit LocklessBlockAlloc(size_t init = 8) : _root(0)
    {
      _flag.clear(std::memory_order_relaxed);
      //contention=0;
      _freelist.p = 0;
      _freelist.tag = 0;
      static_assert((sizeof(T) >= sizeof(void*)), "T cannot be less than the size of a pointer");
      static_assert((sizeof(bss_PTag<void>) == (sizeof(void*) * 2)), "ABAPointer isn't twice the size of a pointer!");
      _allocChunk(init * sizeof(T));
    }
    inline ~LocklessBlockAlloc()
    {
      NODE* hold = _root;
      while((_root = hold))
      {
        hold = _root->next;
        free(_root);
      }
    }
    inline T* alloc(size_t num) noexcept
    {
      assert(num == 1);
#ifdef BSS_DISABLE_CUSTOM_ALLOCATORS
      return bssMalloc<T>(num);
#endif
      bss_PTag<void> ret = { 0, 0 };
      bss_PTag<void> nval;
      asmcasr<bss_PTag<void>>(&_freelist, ret, ret, ret);
      for(;;)
      {
        if(!ret.p)
        {
          if(!_flag.test_and_set(std::memory_order_acquire))
          { // If we get the lock, do a new allocation
            if(!_freelist.p) // Check this due to race condition where someone finishes a new allocation and unlocks while we were testing that flag.
              _allocChunk(fbnext(_root->size / sizeof(T)) * sizeof(T));
            _flag.clear(std::memory_order_release);
          }
          asmcasr<bss_PTag<void>>(&_freelist, ret, ret, ret); // we could put this in the while loop but then you have to set nval to ret and it's just as messy
          continue;
        }
        nval.p = *((void**)ret.p);
        nval.tag = ret.tag + 1;
        if(asmcasr<bss_PTag<void>>(&_freelist, nval, ret, ret)) break;
      }

      //assert(_validPointer(ret));
      return (T*)ret.p;
    }
    inline void dealloc(void* p) noexcept
    {
#ifdef BSS_DISABLE_CUSTOM_ALLOCATORS
      free(p); return;
#endif
      assert(_validPointer(p));
#ifdef BSS_DEBUG
      memset(p, 0xfd, sizeof(T));
#endif
      _setFreeList(p, p);
      //*((void**)p)=(void*)_freelist;
      //while(!asmcas<void*>(&_freelist,p,*((void**)p))) //ABA problem
      //  *((void**)p)=(void*)_freelist;
    }

    LocklessBlockAlloc& operator=(LocklessBlockAlloc&& mov) noexcept
    {
      _root = mov._root;
      _freelist.p = mov._freelist.p;
      _freelist.tag = mov._freelist.tag;
      mov._freelist.p = mov._root = 0;
      _flag.clear(std::memory_order_relaxed);
      return *this;
    }

    //size_t contention;

  protected:
#ifdef BSS_DEBUG
    inline bool _validPointer(const void* p) const
    {
      const NODE* hold = _root;
      while(hold)
      {
        if(p >= (hold + 1) && p < (((uint8_t*)(hold + 1)) + hold->size))
          return ((((uint8_t*)p) - ((uint8_t*)(hold + 1))) % sizeof(T)) == 0; //the pointer should be an exact multiple of sizeof(T)

        hold = hold->next;
      }
      return false;
    }
#endif
    inline void _allocChunk(size_t nsize) noexcept
    {
      NODE* retval = reinterpret_cast<NODE*>(malloc(sizeof(NODE) + nsize));
      retval->next = _root;
      retval->size = nsize;
      _root = retval; // There's a potential race condition on DEBUG mode only where failing to set this first would allow a thread to allocate a new pointer and then delete it before _root got changed, which would then be mistaken for an invalid pointer
      _initChunk(retval);
    }

    inline void _initChunk(const NODE* chunk) noexcept
    {
      void* hold = 0;
      uint8_t* memend = ((uint8_t*)(chunk + 1)) + chunk->size;
      for(uint8_t* memref = (uint8_t*)(chunk + 1); memref < memend; memref += sizeof(T))
      {
        *((void**)(memref)) = hold;
        hold = memref;
      }
      _setFreeList(hold, (void*)(chunk + 1)); // The target here is different because normally, the first block (at chunk+1) would point to whatever _freelist used to be. However, since we are lockless, _freelist could not be 0 at the time we insert this, so we have to essentially go backwards and set the first one to whatever freelist is NOW, before setting freelist to the one on the end.
    }

    inline void _setFreeList(void* p, void* target) noexcept
    {
      bss_PTag<void> prev = { 0, 0 };
      bss_PTag<void> nval = { p, 0 };
      asmcasr<bss_PTag<void>>(&_freelist, prev, prev, prev);
      do
      {
        nval.tag = prev.tag + 1;
        *((void**)(target)) = (void*)prev.p;
        //atomic_xadd<size_t>(&contention); //DEBUG
      } while(!asmcasr<bss_PTag<void>>(&_freelist, nval, prev, prev));
    }

    BSS_ALIGN(16) volatile bss_PTag<void> _freelist;
    BSS_ALIGN(4) std::atomic_flag _flag;
    NODE* _root;
  };

  template<typename T>
  class BSS_COMPILER_DLLEXPORT LocklessBlockPolicy : protected LocklessBlockAlloc<T>
  {
    LocklessBlockPolicy(const LocklessBlockPolicy& copy) BSS_DELETEFUNC
      LocklessBlockPolicy& operator=(const LocklessBlockPolicy& copy) BSS_DELETEFUNCOP

  public:
    typedef T* pointer;
    typedef T value_type;
    template<typename U>
    struct rebind { typedef LocklessBlockPolicy<U> other; };

    inline LocklessBlockPolicy(LocklessBlockPolicy&& mov) : LocklessBlockAlloc<T>(std::move(mov)) {}
    inline LocklessBlockPolicy() {}
    inline ~LocklessBlockPolicy() {}

    inline pointer allocate(size_t cnt, const pointer = 0) noexcept { return LocklessBlockAlloc<T>::alloc(cnt); }
    inline void deallocate(pointer p, size_t num = 0) noexcept { return LocklessBlockAlloc<T>::dealloc(p); }
  };
}

#endif