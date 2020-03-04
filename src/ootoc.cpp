#include "ootoc.h"
#include <iostream>

extern "C"
{
#include <curl/curl.h>
#include <fcntl.h>
}

namespace ootoc
{
TarOverCurlSet::TarOverCurlSet(const string &tar_file_path)
{
    if (tar_open(&tar, tar_file_path.c_str(), nullptr, O_RDONLY, 0, TAR_GNU | TAR_VERBOSE) != 0)
    {
        std::cerr << "Fail to open file " << tar_file_path << std::endl;
        exit(1);
    }
    while (!th_read(tar))
    {
        std::string filename = th_get_pathname(tar);
        std::cout << filename << std::endl;
        th_print(tar);
        th_print_long_ls(tar);
    }
}

TarOverCurlSet::~TarOverCurlSet()
{
    if (tar)
    {
        tar_close(tar);
        tar = nullptr;
    }
}
} // namespace ootoc
