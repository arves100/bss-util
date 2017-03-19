// Copyright �2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"

#include "cBitStream.h"
#include <sstream>
#include "test.h"

using namespace bss_util;

TESTDEF::RETPAIR test_BITSTREAM()
{
  BEGINTEST;

  std::stringstream s;
  {
    int a = -1;
    short b = 8199;
    char c = 31;
    char d = -63;
    cBitStream<std::ostream> so(s);
    so << a;
    so << b;
    so.Write(true);
    so.Write(&d, 7);
    so.Write(&d, 7);
    so.Write(true);
    so << c;
    so.Write(&c, 3);
    so.Write(&b, 14);
    so.Write(&c, 3);
    so << c;
  }
  {
    cBitStream<std::istream> si(s);
    int a;
    short b;
    char c;
    bool k;
    si >> a;
    TEST(a == -1);
    si >> b;
    TEST(b == 8199);
    si >> k;
    TEST(k);
    c = 0;
    si.Read(&c, 7);
    TEST(c == 65);
    c = 0;
    si.Read(&c, 7);
    TEST(c == 65);
    si >> k;
    TEST(k);
    si >> c;
    TEST(c == 31);
    c = 0;
    si.Read(&c, 3);
    TEST(c == 7);
    b = 0;
    si.Read(&b, 14);
    TEST(b == 8199);
    c = 0;
    si.Read(&c, 3);
    TEST(c == 7);
    c = 0;
    si.Read<char>(c);
    TEST(c == 31);
  }

  ENDTEST;
}
