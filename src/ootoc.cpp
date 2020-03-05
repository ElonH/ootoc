#include "ootoc.h"
#include <iostream>
#include <regex>

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

TarParser::~TarParser()
{
    Close();
}

bool TarParser::Open(const string &path)
{
    auto ret = tar_open(&tar, path.c_str(), nullptr, O_RDONLY, 0, TAR_GNU);
    if (ret)
        return false;
    return true;
}

bool TarParser::Close()
{
    if (tar == nullptr)
        return false;
    auto ret = tar_close(tar);
    if (!ret)
        tar = nullptr;
    return !ret;
}
bool TarParser::Parse()
{
    if (tar == nullptr)
        return false;
    output = "";
    while (!th_read(tar))
    {
        // th_print(tar);
        // th_print_long_ls(tar);
        if (TH_ISREG(tar) && tar_skip_regfile(tar) != 0)
            return false;
        if (!TH_ISREG(tar))
            continue;
        string inner_path = th_get_pathname(tar);
        cout << inner_path << endl;
        cout << tar->offset << endl;
        regex reg(".*?/Packages.gz$");
        auto is_exist = regex_match(inner_path, reg);
        if (is_exist)
        {
            cout << "true" << endl;
        }
    }
    return true;
}

const string &TarParser::GetOutput()
{
    return this->output;
}

} // namespace ootoc
