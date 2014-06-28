// Copyright �2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"

#ifndef __PROFILER_H__BSS__
#define __PROFILER_H__BSS__

#include "cHighPrecisionTimer.h"
#include "cArray.h"
#include "bss_alloc_fixed.h"

#ifndef BSS_ENABLE_PROFILER
#define PROFILE_BEGIN(name)
#define PROFILE_END(name)
#define PROFILE_BLOCK(name)
#define PROFILE_FUNC()
#define PROFILE_OUTPUT(file,output)
#else
#define __PROFILE_STATBLOCK(name,str) static bss_util::Profiler::ProfilerData PROFDATA_##name(str,__FILE__,__LINE__)
#define __PROFILE_ZONE(name) bss_util::ProfilerBlock BLOCK_##name(PROFDATA_##name .id, bss_util::Profiler::profiler.GetCur())
#define PROFILE_BEGIN(name) __PROFILE_STATBLOCK(name, MAKESTRING(name)); PROF_TRIENODE* PROFCACHE_##name = bss_util::Profiler::profiler.GetCur(); unsigned __int64 PROFTIME_##name = bss_util::Profiler::profiler.StartProfile(PROFDATA_##name .id)
#define PROFILE_END(name) bss_util::Profiler::profiler.EndProfile(PROFTIME_##name, PROFCACHE_##name)
#define PROFILE_BLOCK(name) __PROFILE_STATBLOCK(name, MAKESTRING(name)); __PROFILE_ZONE(name);
#define PROFILE_FUNC() __PROFILE_STATBLOCK(func, __FUNCTION__); __PROFILE_ZONE(func)
#define PROFILE_OUTPUT(file,output) bss_util::Profiler::profiler.WriteToFile(file,output)
#endif

typedef unsigned short PROFILER_INT;

namespace bss_util {
  struct PROF_HEATNODE;
  struct PROF_FLATOUT;

  struct PROF_TRIENODE
  {
    PROF_TRIENODE* _children[16];
    double avg;
    double codeavg;
    //double variance; 
    unsigned __int64 total; // If total is -1 this isn't a terminating node
    unsigned __int64 inner;
  };
  struct BSS_DLLEXPORT Profiler
  {
    struct ProfilerData
    {
      const char* name;
      const char* file;
      unsigned int line;
      PROFILER_INT id;

      inline ProfilerData(const char* Name, const char* File, unsigned int Line) : name(Name), file(File), line(Line), id(++total) { Profiler::profiler.AddData(id, this); }
    };

    BSS_FORCEINLINE unsigned __int64 BSS_FASTCALL StartProfile(PROFILER_INT id)
    { 
      PROF_TRIENODE** r=&_cur;
      while(id>0)
      {
        r = &(*r)->_children[id%16];
        id = (id>>4);
        if(!*r)
          *r = _allocnode(id==0);
      }
      _cur=*r;
      _cur->inner=0;
      return cHighPrecisionTimer::OpenProfiler();
    }
    BSS_FORCEINLINE void BSS_FASTCALL EndProfile(unsigned __int64 time, PROF_TRIENODE* old)
    {
      time = cHighPrecisionTimer::CloseProfiler(time);
      //bssvariance<double, unsigned __int64>(_cur->variance, _cur->avg, (double)time, ++_cur->total);
      _cur->avg = bssavg<double, unsigned __int64>(_cur->avg, (double)time, ++_cur->total);
      _cur->codeavg = bssavg<double, unsigned __int64>(_cur->codeavg, (double)(time - _cur->inner), _cur->total);
      _cur=old;
      _cur->inner += time;
    }
    BSS_FORCEINLINE PROF_TRIENODE* GetCur() { return _cur; }
    void AddData(PROFILER_INT id, ProfilerData* p);
    enum OUTPUT_DATA : unsigned char { OUTPUT_FLAT=1, OUTPUT_TREE=2, OUTPUT_HEATMAP=4, OUTPUT_ALL=1|2|4 };
    void WriteToFile(const char* s, unsigned char output);
    void WriteToStream(std::ostream& stream, unsigned char output);

    static Profiler profiler;
    static const PROFILER_INT BUFSIZE=4096;
    
  private:
    Profiler();
    PROF_TRIENODE* _allocnode(bool leaf);
    void BSS_FASTCALL _treeout(std::ostream& stream, PROF_TRIENODE* node, PROFILER_INT id, unsigned int level, PROFILER_INT idlevel);
    void BSS_FASTCALL _heatout(PROF_HEATNODE& heat, PROF_TRIENODE* node, PROFILER_INT id, PROFILER_INT idlevel);
    void BSS_FASTCALL _heatwrite(std::ostream& stream, PROF_HEATNODE& node, unsigned int level, double max);
    double BSS_FASTCALL _heatfindmax(PROF_HEATNODE& heat);
    static void BSS_FASTCALL _flatout(PROF_FLATOUT* avg, PROF_TRIENODE* node, PROFILER_INT id, PROFILER_INT idlevel);
    static const char* BSS_FASTCALL _trimpath(const char* path);
    static void BSS_FASTCALL _timeformat(std::ostream& stream, double avg, double variance, unsigned __int64 num);
    static PROFILER_INT total;

    WArray<ProfilerData*, PROFILER_INT>::t _data;
    PROF_TRIENODE* _trie;
    PROF_TRIENODE* _cur;
    cFixedAlloc<PROF_TRIENODE> _alloc;
    unsigned int _totalnodes;
  };

  struct ProfilerBlock
  {
    BSS_FORCEINLINE ProfilerBlock(PROFILER_INT id, PROF_TRIENODE* cache) : _cache(cache), _time(Profiler::profiler.StartProfile(id)) { }
    BSS_FORCEINLINE ~ProfilerBlock() { Profiler::profiler.EndProfile(_time, _cache); }
    PROF_TRIENODE* _cache;
    unsigned __int64 _time;
  };
}

#endif