#include "ootoc.h"
#include <iostream>
#include <regex>
#include <yaml-cpp/yaml.h>

extern "C"
{
#include <fcntl.h>
}

namespace ootoc
{
TarOverCurl::~TarOverCurl()
{
}

#define quick_return_false(ret) \
    if (ret != CURLE_OK)        \
    return false

bool TarOverCurl::ReInitCurl()
{
    curl_easy_reset(curl);
    quick_return_false(curl_easy_setopt(curl, CURLOPT_URL, url.c_str()));
    quick_return_false(curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L));
    quick_return_false(curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L));
    quick_return_false(curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L));
    quick_return_false(curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L));
    return true;
}

bool TarOverCurl::ExecuteCurl()
{
    quick_return_false(curl_easy_perform(curl));
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    cout << response_code << endl;
    if (response_code == 206)
        return true;
    cerr << "can't not connected." << endl;
    return false;
}

bool TarOverCurl::Open(const string &url, const string &fastAux)
{
    this->url = url;
    this->aux = fastAux;
    // valiate aux
    auto node = YAML::Load(aux);
    if (!node.IsMap())
        return false;
    cout << "aux file loaded." << endl;
    // valiate url
    if (curl)
        curl_easy_cleanup(curl);
    curl = curl_easy_init();
    if (curl == nullptr)
        return false;
    if (!ReInitCurl())
        return false;
    /* 
    * NOTE: Leader '+' trigger conversion from non-captured Lambda Object to plain C pointer
    * refs: https://stackoverflow.com/a/58154943/12118675
    */
    quick_return_false(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                                        +[](void *contents, size_t size, size_t nmemb, void *userp) -> size_t {
                                            auto realsize = size * nmemb;
                                            // cout << realsize << endl;
                                            return realsize;
                                        }));
    quick_return_false(curl_easy_setopt(curl, CURLOPT_RANGE, "0-1"));
    if (!ExecuteCurl())
        return false;
    cout << "url connected." << endl;
    return true;
}

bool TarOverCurl::ExtractFile(const string &inner_path, std::function<void(const string &)> &&handler)
{
    auto node = YAML::Load(aux);
    if (node[inner_path])
    {
        auto beg = node[inner_path]["start"].as<string>();
        auto end = node[inner_path]["end"].as<string>();
        if (!ReInitCurl())
            return false;
        quick_return_false(curl_easy_setopt(curl, CURLOPT_RANGE, (beg + "-" + end).c_str()));
        cout << "fetching data: " + beg + "-" + end << endl;
        quick_return_false(curl_easy_setopt(curl, CURLOPT_WRITEDATA, &handler));
        quick_return_false(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                                            +[](char *contents, size_t size, size_t nmemb,
                                                const std::function<void(const string &)> *hptr) -> size_t {
                                                auto realsize = size * nmemb;
                                                auto &handler = *hptr;
                                                // cout << string(contents, contents + realsize) << endl;
                                                handler(string(contents, contents + realsize));
                                                // cout << realsize << endl;
                                                return realsize;
                                            }));
        if (!ExecuteCurl())
            return false;
        cout << "extract file success: " << inner_path << endl;
        return true;
    }
    return false;
}

bool TarOverCurl::Close()
{
    if (curl)
    {
        curl_easy_cleanup(curl);
        curl = nullptr;
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
        auto end = sta - 1 + th_get_size(tar);
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
