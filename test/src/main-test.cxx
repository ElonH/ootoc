#include "ootoc.h"
// #include <gtest/gtest.h>

#include <curl/curl.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <string>

using namespace ootoc;
// TEST(toc, open)
// {
// }

template <typename T>
void log(const T msg)
{
    std::cerr << "Error: " << msg << std::endl;
}

// TEST(tar, example)
int main()
{
    fstream auxstm("./test/data/Tar.yml", ios::in);
    string content((std::istreambuf_iterator<char>(auxstm)),
                   (std::istreambuf_iterator<char>()));
    auxstm.close();
    OpkgServer s;
    s.setRemoteTar("https://github.com/ElonH/ootoc/releases/download/test/Packages.tar", content);
    s.setServer("localhost", 21730);
    // s.Start();
    return 0;
}
