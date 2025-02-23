// Copyright �2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"

#include "test.h"
#include "bss-util/os.h"
#include "bss-util/Trie.h"
#include <string.h>
#ifdef BSS_PLATFORM_WIN32
#include "bss-util/win32_includes.h"
#endif

using namespace bss;

TESTDEF::RETPAIR test_OS()
{
  BEGINTEST;
  TEST(FolderExists("../bin") || FolderExists("bin"));
#ifdef BSS_PLATFORM_WIN32
  TEST(FolderExistsW(BSS__L("C:/windows/")));
#else
  TEST(FolderExists(BSS__L("/usr/")));
#endif
  TEST(!FolderExists("abasdfwefs"));
  FILE* f;
  FOPEN(f, "blank.txt", "w+");
  fclose(f);
  TEST(FileExists("blank.txt"));
  TEST(!FileExists("testaskdjlhfs.sdkj"));
#ifdef BSS_PLATFORM_WIN32
  TEST(!FolderExistsW(BSS__L("abasdfwefs/alkjsdfs/sdfjkd/alkjsdfs/sdfjkd/alkjsdfs/sdfjkd/")));
  TEST(FileExistsW(BSS__L("blank.txt")));
  TEST(!FileExistsW(BSS__L("testaskdjlhfs.sdkj")));
#endif

  //Str cmd(GetCommandLineW());
  Str cmd("\"\"C:/fake/f\"\"ile/p\"ath.txt\"\" -r 2738 283.5 -a\"a\" 3 \"-no indice\"");
  int argc = ToArgV<char>(0, cmd.UnsafeString());
  VARARRAY(char*, argv, argc);
  ToArgV((char**)argv, cmd.UnsafeString());
  ProcessCmdArgs(argc, argv, [&__testret](const char* const* p, size_t n)
  {
    switch(hash_fnv1a(p[0]))
    {
    case hash_fnv1a("-r"):
      TEST(n == 3);
      TEST(atof(p[1]) == 2738.0);
      TEST(atof(p[2]) == 283.5);
      break;
    case hash_fnv1a("-a\"a\""):
      TEST(n == 2);
      TEST(atoi(p[1]) == 3);
      break;
    case hash_fnv1a("-no indice"):
      TEST(n == 1);
      break;
    default:
      TEST(!strcmp(p[0], "\"C:/fake/f\"\"ile/p\"ath.txt\""));
    }
  });

  TEST(CreateDir("testdir/recurse/recurseagain", false));
  TEST(!FolderExists("testdir/recurse/recurseagain"));
  TEST(!CreateDir("dontrecurse", false));
  TEST(FolderExists("dontrecurse"));
  TEST(!CreateDir("testdir/recurse/recurseagain"));
  TEST(FolderExists("testdir/recurse/recurseagain"));
  TEST(DelDir("testdir/recurse", false));
  TEST(!DelDir("testdir/recurse"));
  TEST(!DelDir("dontrecurse", false));
  TEST(!FolderExists("dontrecurse"));
  TEST(!FolderExists("testdir/recurse/recurseagain"));
  TEST(!FolderExists("testdir/recurse"));
  TEST(FolderExists("testdir"));
  TEST(!DelDir("testdir"));
  TEST(!FolderExists("testdir"));
  std::vector<Str> files;
  ListDir(".", files, 1);
  // There should be at least two files, test.exe and bss-util.dll
  TEST(files.size()>2);

  //TEST(FileExists("testlink"));
  //TEST(FileExists(BSS__L("testlink")));
  //TEST(FolderExists("IGNORE/symlink/"));
  //TEST(FolderExists(BSS__L("IGNORE/symlink/")));
  //{
  //std::unique_ptr<char[],bssDLLDelete<char[]>> p = FileDialog(true,0,BSS__L("test"));
  //}

#ifdef BSS_PLATFORM_WIN32
  int64_t sz = GetRegistryValue(HKEY_CURRENT_USER, "Control Panel\\Desktop", "CursorBlinkRate", 0, 0);
  TEST(sz > 0);
  if(sz > 0)
  {
    VARARRAY(wchar_t, buf, (sz / 2) + 1);
    buf[sz / 2] = 0;
    sz = GetRegistryValue(HKEY_CURRENT_USER, "Control Panel\\Desktop", "CursorBlinkRate", reinterpret_cast<unsigned char*>((wchar_t*)buf), (unsigned long)sz);
    TEST(sz >= 0);
    int blinkrate = atoi(Str(buf));
    TEST(blinkrate > 0);
  }

  //SetRegistryValue(HKEY_LOCAL_MACHINE,"SOFTWARE\\test","valcheck","data");
  //SetRegistryValue(HKEY_LOCAL_MACHINE,"SOFTWARE\\test\\test","valcheck","data");
  //sz = GetRegistryValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\test\\test", "valcheck", 0, 0);
  //TEST(sz > 0);
  //if(sz > 0)
  //{
  //  VARARRAY(unsigned char, buf, sz+1);
  //  buf[sz] = 0;
  //  sz = GetRegistryValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\test\\test", "valcheck", buf, sz);
  //  TEST(!sz);
  //  TEST(!strcmp((char*)buf, "data"));
  //}
  //DelRegistryNode(HKEY_LOCAL_MACHINE,"SOFTWARE\\test");
  #endif

  //AlertBox("test", "title", 0x00000010L);

  //float at[4][4];
  //at[0][0]=(float)(size_t)f;
  //CPU_Barrier();
  //tttest(0,&at);
  //CPU_Barrier();

  auto p = GetFontPath("Arial", 400, false);
  TEST(!STRICMP(Logger::_trimPath(p.get()), "arial.ttf"));
  p = GetFontPath("Arial", 500, false);
  TEST(!STRICMP(Logger::_trimPath(p.get()), "arial.ttf"));
  p = GetFontPath("Arial", 501, false);
  TEST(!STRICMP(Logger::_trimPath(p.get()), "arialbd.ttf"));
  p = GetFontPath("Arial", 399, false);
  TEST(!STRICMP(Logger::_trimPath(p.get()), "arial.ttf"));
  p = GetFontPath("Arial", 1, false);
  TEST(!STRICMP(Logger::_trimPath(p.get()), "arial.ttf"));
  p = GetFontPath("Arial", 600, false);
  TEST(!STRICMP(Logger::_trimPath(p.get()), "arialbd.ttf"));
  p = GetFontPath("Arial", 600, true);
  TEST(!STRICMP(Logger::_trimPath(p.get()), "arialbi.ttf"));
  p = GetFontPath("Arial", 500, true);
  TEST(!STRICMP(Logger::_trimPath(p.get()), "ariali.ttf"));

  ENDTEST;
}