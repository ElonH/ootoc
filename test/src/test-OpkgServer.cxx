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
    OpkgServer s;
    s.setAuxUrl("", "./test/data/Tar.yml");
    s.setRemoteTar("https://github.com/ElonH/ootoc/releases/download/test/Packages.tar");
    s.setServer("localhost", 21730);
    // s.Start();
}
