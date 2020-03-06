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
    TarOverCurl toc;
    bool ret;
    ret = toc.Open("https://github.com/ElonH/ootoc/releases/download/test/Packages.tar", content);
    ASSERT_EQ(ret, true);
    bool connected = false;
    ret = toc.ExtractFile("Packages/packages/x86_64/routing/Packages", [&](const string &conts) {
        cout << conts << endl;
        connected = true;
    });
    ASSERT_EQ(ret, true);
    ASSERT_EQ(connected, true);
}
