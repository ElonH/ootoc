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
    fstream auxstm("./test/data/Tar.yml", ios::in);
    string content((std::istreambuf_iterator<char>(auxstm)),
                   (std::istreambuf_iterator<char>()));
    auxstm.close();
    OpkgServer s;
    s.setRemoteTar("https://github.com/ElonH/ootoc/releases/download/test/Packages.tar", content);
    s.setServer("localhost", 21730);
    // s.Start();
}
