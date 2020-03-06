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
    p.Open("./test/data/Packages.tar");
    p.Parse();
    p.Close();
    auto f = fstream("./test/data/Tar.yml", ios::out);
    f << p.GetOutput();
    f.close();
}
