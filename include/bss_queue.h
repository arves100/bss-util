// Copyright �2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"

#ifndef __C_BSS_QUEUE_H__
#define __C_BSS_QUEUE_H__

#include "cArrayCircular.h"

namespace bss_util {
  // Fast, tiny circular array-based queue. Pop and Peek are only valid if there is an item in the stack; this check must be done by the user.
  template<class T, typename SizeType=int, ARRAY_TYPE ArrayType = CARRAY_SIMPLE, typename Alloc=StaticAllocPolicy<T>>
  class BSS_COMPILER_DLLEXPORT cQueue : protected cArrayCircular<T, SizeType, ArrayType, Alloc>
  {
  protected:
    typedef cArrayCircular<T, SizeType, ArrayType, Alloc> BASE;
    using BASE::_length;

  public:
    inline cQueue(const cQueue& copy) : BASE(copy) {}
    inline cQueue(cQueue&& mov) : BASE(std::move(mov)) {}
    inline explicit cQueue(SizeType init=0) : BASE(init) {}
    inline ~cQueue() {}
    BSS_FORCEINLINE void BSS_FASTCALL Push(const T& value) { _push<const T&>(value); }
    BSS_FORCEINLINE void BSS_FASTCALL Push(T&& value) { _push<T&&>(std::move(value)); }
    BSS_FORCEINLINE T BSS_FASTCALL Pop() { assert(_length!=0); return BASE::PopBack(); }
    BSS_FORCEINLINE T& BSS_FASTCALL Peek() { assert(_length!=0); return BASE::Back(); }
    BSS_FORCEINLINE const T& BSS_FASTCALL Peek() const { assert(_length!=0); return BASE::Back(); }
    BSS_FORCEINLINE void Discard() { --_length; }
    BSS_FORCEINLINE bool Empty() { return !_length; }
    BSS_FORCEINLINE void Clear() { BASE::Clear(); }
    BSS_FORCEINLINE SizeType Capacity() const { return BASE::Capacity(); }
    BSS_FORCEINLINE SizeType Length() const { return _length; }

    inline cQueue& operator=(const cQueue& copy) { BASE::operator=(copy); return *this; }
    inline cQueue& operator=(cQueue&& mov) { BASE::operator=(std::move(mov)); return *this; }

  protected:
    template<typename U>
    void BSS_FASTCALL _push(U && value) { if(_length>=BASE::_size) BASE::SetSize(T_FBNEXT(_length)); BASE::_push(std::forward<U>(value)); }
  };
}

#endif
