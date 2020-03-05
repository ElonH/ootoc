#include "ootoc.h"
// #include <gtest/gtest.h>

#include <curl/curl.h>
#include <fcntl.h>
#include <iostream>

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
    TarParser p;
    p.Open("/home/elonh/git_prj/opkg-over-tar-over-curl/test/data/Packages.tar");
    p.Parse();
    p.Close();
    return 0;
}
