#include "ootoc.h"
#include <gtest/gtest.h>

#include <curl/curl.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <string>

using namespace ootoc;
TEST(TarOverCurl, test)
{
    fstream auxstm("./test/data/Tar.yml", ios::in);
    string content((std::istreambuf_iterator<char>(auxstm)),
                   (std::istreambuf_iterator<char>()));
    auxstm.close();
    TarOverCurl toc("https://cdn.jsdelivr.net/gh/ElonH/ootoc@master/test/data/Packages.tar", content);
    bool connected = false;
    bool ret = toc.ExtractFile("Packages/packages/x86_64/routing/Packages", [&](const string &conts) {
        cout << conts;
        connected = true;
    });
    ASSERT_EQ(ret, true);
    ASSERT_EQ(connected, true);
}
