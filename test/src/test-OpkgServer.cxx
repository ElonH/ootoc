#include "ootoc.h"
#include <gtest/gtest.h>

#include <curl/curl.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace ootoc;
TEST(OpkgServer, test)
{
    Logger::start();
    OpkgServer s;
    s.tar_url = "https://cdn.jsdelivr.net/gh/ElonH/ootoc@master/test/data/Packages.tar";
    s.aux_url = "https://cdn.jsdelivr.net/gh/ElonH/ootoc@master/test/data/Tar.yml";
    s.local_addr = "127.0.0.1";
    s.local_port = 21730;
    ASSERT_EQ(s.Check(), true);
    auto t = thread([&s]() { s.Start(); });
    std::this_thread::sleep_for(1s);
    QuickCurl c(fmt::format("http://{}:{}/Packages/packages/x86_64/routing/Packages", s.local_addr, s.local_port));
    auto contents = make_shared<string>();
    hash<string> hasher;
    c.FetchRange([contents](const string &part_contents) {
        *contents += part_contents;
    });
    spdlog::info(*contents);
    ASSERT_EQ(contents->size(), 708);
    auto hash_rst = hasher(*contents);
    spdlog::info("hash: {}", hash_rst);
    ASSERT_EQ(hash_rst, 7460544817086421715);
    QuickCurl d(fmt::format("http://{}:{}/stop", s.local_addr, s.local_port));
    d.FetchRange([](const string &) {});
    t.join();
}

TEST(OpkgServer, bigfile)
{
    Logger::start();
    OpkgServer s;
    s.tar_url = "https://cdn.jsdelivr.net/gh/ElonH/ootoc@f868d32851834a0b13ecc51fdc59385c1779a313/test/data/random.tar";
    s.aux_url = "https://cdn.jsdelivr.net/gh/ElonH/ootoc@f868d32851834a0b13ecc51fdc59385c1779a313/test/data/random.yml";
    s.local_addr = "127.0.0.1";
    s.local_port = 21731;
    ASSERT_EQ(s.Check(), true);
    auto t = thread([&s]() { s.Start(); });
    std::this_thread::sleep_for(1s);
    QuickCurl c(fmt::format("http://{}:{}/123/random.ipk", s.local_addr, s.local_port));
    auto length = 0ULL;
    auto contents = make_shared<string>();
    hash<string> hasher;
    c.FetchRange([&length](const string &part_contents) {
        length += part_contents.size();
    });
    spdlog::info("{}", length);
    ASSERT_EQ(length, 10485760);
    auto hash_rst = hasher(*contents);
    spdlog::info("hash: {}", hash_rst);
    ASSERT_EQ(hash_rst, 6142509188972423790);
    QuickCurl d(fmt::format("http://{}:{}/stop", s.local_addr, s.local_port));
    d.FetchRange([](const string &) {});
    t.join();
}
