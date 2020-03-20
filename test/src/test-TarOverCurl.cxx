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

    auto ans_list = toc.SearchFileLocation(regex(".*?/README.md"));
    ASSERT_EQ(ans_list->size(), 1);
    ASSERT_EQ(*ans_list->begin(), "Packages/txt/README.md");

    auto est = toc.HasFile("Packages/packages/x86_64/routing/bmx7-json_7.1.1-2_x86_64.ipk");
    ASSERT_EQ(est, true);
    est = toc.HasFile("Packages/packages/x86_64/routing/bmx7-");
    ASSERT_EQ(est, false);

    auto pos = toc.GetStarPosByPath("Packages/packages/x86_64/routing/Packages.gz");
    ASSERT_EQ(pos, 59392ull);
    pos = toc.GetEndPosByPath("Packages/packages/x86_64/routing/Packages.gz");
    ASSERT_EQ(pos, 59855ull);
}
