#include "ootoc.h"
#include "spdlog/details/log_msg_buffer.h"
#include "spdlog/details/null_mutex.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/spdlog.h"
#include <iostream>
#include <regex>

extern "C"
{
#include <fcntl.h>
}

namespace ootoc
{

namespace
{
using namespace spdlog;
using namespace spdlog::sinks;
class ootoc_sink final : public base_sink<std::mutex>
{
public:
    explicit ootoc_sink(const string &log_path)
    {
        if (log_path == "")
        {
            ofs = &cout;
            isCout = true;
            return;
        }
        isCout = false;
        ofs = new ofstream(log_path, ios::out);
    }
    ~ootoc_sink()
    {
        if (!isCout)
            free(ofs);
    }

protected:
    bool isCout = true;
    ostream *ofs;
    void sink_it_(const details::log_msg &msg) override
    {
        memory_buf_t formatted;
        base_sink<std::mutex>::formatter_->format(msg, formatted);
        // strip Line feed, due to DebugLog will auto add Line feed.
        auto &stream = *ofs;
        stream << fmt::to_string(formatted);
        stream.flush();
    }
    void flush_() override {}
};
} // namespace

void Logger::start(const string &file_path)
{
    try
    {
        auto default_logger = make_shared<ootoc_sink>(file_path);
        default_logger->set_level(spdlog::level::debug);
        default_logger->set_pattern("[%n] [%T.%e] [%t] [%^%l%$] %v");
        spdlog::set_default_logger(make_shared<spdlog::logger>("ootoc", default_logger));
        spdlog::set_level(spdlog::level::trace);
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        cerr << "ootoc log initialization failed: " << ex.what() << endl;
    }
}

TarOverCurl::~TarOverCurl()
{
    Close();
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
    spdlog::log(level::trace, fmt::format("response_code: {}", response_code));
    if (response_code == 206)
        return true;
    spdlog::log(level::err, "can't not connected.");
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
    spdlog::log(level::debug, "aux file loaded.");
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
    spdlog::log(level::info, "url connected.");
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
        // cout << "Tar url: " << url << endl;
        spdlog::log(level::info, fmt::format("fetching data range: {}-{}", beg, end));
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
        spdlog::log(level::info, "extract file success: " + inner_path);
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
    return true;
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
        // cout << inner_path << endl;
        // cout << tar->offset << endl;
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
    // cout << output << endl;
    return true;
}

const string &TarParser::GetOutput()
{
    return this->output;
}

void OpkgServer::setAuxUrl(const string &url, const string &path)
{
    aux_url = url;
    aux_path = path;
    fstream auxstm(aux_path, ios::in);
    string aux_content((std::istreambuf_iterator<char>(auxstm)),
                       (std::istreambuf_iterator<char>()));
    auxstm.close();
    aux = aux_content;
}

bool OpkgServer::fetchAux()
{
    string remote_aux_content;
    spdlog::log(level::info, fmt::format("fetching aux from {}", aux_url));
    auto curl = curl_easy_init();
    quick_return_false(curl_easy_setopt(curl, CURLOPT_URL, aux_url.c_str()));
    quick_return_false(curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L));
    quick_return_false(curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L));
    quick_return_false(curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L));
    quick_return_false(curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L));
    quick_return_false(curl_easy_setopt(curl, CURLOPT_WRITEDATA, &remote_aux_content));
    quick_return_false(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                                        +[](char *contents, size_t size, size_t nmemb,
                                            string *dataptr) -> size_t {
                                            auto realsize = size * nmemb;
                                            auto &remote_aux_content = *dataptr;
                                            remote_aux_content += string(contents, contents + realsize);
                                            if (realsize)
                                                spdlog::log(level::debug, fmt::format("aux progress: {}", remote_aux_content.size()));
                                            return realsize;
                                        }));
    quick_return_false(curl_easy_perform(curl));
    spdlog::log(level::info, "fetch aux completed.");
    aux = remote_aux_content;
    ofstream ofs(aux_path, ios::out);
    ofs << aux;
    ofs.close();
    spdlog::log(level::info, "saved aux.");
    return true;
}

void OpkgServer::setRemoteTar(const string &url)
{
    remote.Open(url, aux);
}

void OpkgServer::setServer(const string &addr, long port)
{
    if (aux_url != "" && aux == "" && !fetchAux())
        spdlog::log(level::err, "fetching remote aux error.");
    this->addr = addr;
    this->port = port;
    using namespace httplib;
    auto node = YAML::Load(aux);
    if (!node.IsMap())
        return;
    spdlog::log(level::info, "aux file loaded.");
    auto remote = &this->remote;
    regex reg(".*?/Packages.gz$");
    for (auto &&it : node)
    {
        const auto inner_path = it.first.as<string>();
        auto beg = node[inner_path]["start"].as<unsigned long long>();
        auto end = node[inner_path]["end"].as<unsigned long long>();
        auto size = end - beg + 1;
        this->svr.Get(("/" + inner_path).c_str(), [remote, inner_path, size](const Request &req, Response &res) {
            auto mtx = make_shared<std::mutex>();
            auto data = make_shared<string>(); // TODO: redule useless memory space
            thread in_coming([&inner_path, &remote, size, mtx, data]() {
                unsigned long long progress = 0;
                remote->ExtractFile(inner_path, [mtx, data, &progress, size](const string &part_conts) {
                    if (part_conts.size() == 0)
                        return;
                    lock_guard<std::mutex> lock(*mtx);
                    *data += part_conts;
                    progress += part_conts.length();
                    spdlog::log(level::debug, "Download Progress: {}/{} \t{}%", progress, size, ((double)progress) / size * 100);
                });
                spdlog::log(level::info, "Download completed.");
            });
            res.set_content_provider(
                size, // Content length
                [mtx, data, size](uint64_t offset, uint64_t length, DataSink &sink) {
                    mtx->lock();
                    if (data->length() - offset == 0)
                    {
                        mtx->unlock();
                        std::this_thread::sleep_for(2s);
                        return;
                    }
                    spdlog::log(level::debug, "Upload Progress: {}/{} \t{}%", offset, size, ((double)offset) / size * 100);
                    auto d = data->c_str();
                    sink.write(&d[offset], min(length, data->length() - offset));
                    mtx->unlock();
                },
                []() {
                    spdlog::log(level::info, "Upload completed.");
                });
            in_coming.join();
        });
        static int num = 1;
        if (regex_match(inner_path, reg))
            spdlog::log(level::info, fmt::format("src/gz {} http://{}:{}/{}", num++, addr, port, inner_path.substr(0, inner_path.size() - 12)));
    }
    spdlog::log(level::info, "prepared.");
}

string OpkgServer::getSubscription(const string &addr, long port)
{
    if (aux_url != "" && aux == "" && !fetchAux())
        spdlog::log(level::err, "fetching remote aux error.");
    auto node = YAML::Load(aux);
    regex reg(".*?/Packages.gz$");
    stringstream ss;
    for (auto &&it : node)
    {
        const auto inner_path = it.first.as<string>();
        static int num = 1;
        if (regex_match(inner_path, reg))
            ss << "src/gz " << num++ << " http://" << addr << ':' << port << '/' << inner_path.substr(0, inner_path.size() - 12) << endl;
    }
    return ss.str();
}

void OpkgServer::Start()
{
    svr.listen(addr.c_str(), port);
    spdlog::log(level::critical, "listen error");
}

} // namespace ootoc
