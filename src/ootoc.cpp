#include "ootoc.h"
#include <iostream>
#include <regex>
#include <yaml-cpp/yaml.h>

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
    YAML::Node node;
    stringstream ss;
    while (!th_read(tar))
    {
        // th_print(tar);
        // th_print_long_ls(tar);
        if (!TH_ISREG(tar))
            continue;
        string inner_path = th_get_pathname(tar);
        auto sta = tar->offset;
        auto end = sta -1 + th_get_size(tar);
        cout << inner_path << endl;
        cout << tar->offset << endl;
        ss.str("");
        ss << sta;
        node[inner_path]["start"] = ss.str();
        ss.str("");
        ss << end;
        node[inner_path]["end"] = ss.str();
        // regex reg(".*?/Packages.gz$");
        // auto is_exist = regex_match(inner_path, reg);
        // if (is_exist)
        //     cout << "true" << endl;
        if (TH_ISREG(tar) && tar_skip_regfile(tar) != 0)
            return false;
    }
    ss.str("");
    ss << node;
    output = ss.str();
    cout << output << endl;
    return true;
}

const string &TarParser::GetOutput()
{
    return this->output;
}

} // namespace ootoc
