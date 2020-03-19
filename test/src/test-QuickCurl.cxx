#include "ootoc.h"
#include <gtest/gtest.h>

#include <curl/curl.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <string>

using namespace ootoc;
TEST(QuickCurl, test)
{
    QuickCurl c("https://cdn.jsdelivr.net/gh/ElonH/ootoc@master/test/data/simple.txt");
    ASSERT_EQ(c.ConnectionTest(), true);
    ASSERT_EQ(c.FetchRange([](const string &) {}, "0", "1"), true);
    auto all_content = make_shared<string>();
    ASSERT_EQ(true, c.FetchRange([all_content](const string &part_content) {
        all_content->append(part_content);
    },
                                 "0"));
    ASSERT_STRCASEEQ(all_content->c_str(), "123456789\n");
    ASSERT_EQ(true, c.FetchRange([](const string &) {}, "0", "999"));
    ASSERT_EQ(false, c.FetchRange([](const string &) {}, "998", "999"));
}
