#include "ootoc.h"
#include <gtest/gtest.h>

#include <curl/curl.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <string>

using namespace ootoc;
TEST(OpkgServer, test)
{
    Logger::start();
    OpkgServer s;
    s.tar_url = "https://cdn.jsdelivr.net/gh/ElonH/ootoc@master/test/data/Packages.tar";
    s.aux_url = "https://cdn.jsdelivr.net/gh/ElonH/ootoc@master/test/data/Tar.yml";
    s.local_addr = "127.0.0.1";
    s.local_port = 21730;
    if (s.Check())
        thread([&s]() { s.Start(); }).detach();
    std::this_thread::sleep_for(1s);
    QuickCurl c(fmt::format("http://{}:{}/Packages/packages/x86_64/routing/Packages", s.local_addr, s.local_port));
    auto contents = make_shared<string>();
    c.FetchRange([contents](const string &part_contents) {
        *contents += part_contents;
    });
    spdlog::info(*contents);
    ASSERT_EQ(contents->size(), 708);
    QuickCurl d(fmt::format("http://{}:{}/stop", s.local_addr, s.local_port));
    d.FetchRange([](const string &) {});
}
