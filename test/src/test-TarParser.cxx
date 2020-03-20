#include "ootoc.h"
#include <gtest/gtest.h>

#include <curl/curl.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>

using namespace ootoc;
TEST(TarParser, test)
{
    TarParser p;
    bool ret;
    ret = p.Open("./test/data/Packages.tar");
    ASSERT_EQ(ret, true);
    ret = p.Parse();
    ASSERT_EQ(ret, true);
    ret = p.Close();
    ASSERT_EQ(ret, true);
    auto f = fstream("./test/data/Tar.yml", ios::out);
    f << p.GetOutput();
    f.close();
}

TEST(TarParser, bigfile)
{
    TarParser p;
    bool ret;
    ret = p.Open("./test/data/random.tar");
    ASSERT_EQ(ret, true);
    ret = p.Parse();
    ASSERT_EQ(ret, true);
    ret = p.Close();
    ASSERT_EQ(ret, true);
    auto f = fstream("./test/data/random.yml", ios::out);
    f << p.GetOutput();
    f.close();
}
