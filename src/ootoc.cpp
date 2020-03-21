#include "ootoc.h"
#include "spdlog/details/log_msg_buffer.h"
#include "spdlog/details/null_mutex.h"
#include "spdlog/sinks/base_sink.h"
#include <iostream>
#include <queue>
#include <regex>

extern "C"
{
#include <fcntl.h>
}

namespace ootoc
{
using namespace spdlog;

namespace
{
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

#define return_false(ret) \
    if (!(ret))           \
    return false

#define return_false_log(ret, level, msg) \
    if (!(ret))                           \
    {                                     \
        spdlog::log(level, msg);          \
        return false;                     \
    }

QuickCurl::QuickCurl(const string &url) : url(url)
{
    curl = curl_easy_init();
    if (false == ReInitCurl())
        spdlog::critical(fmt::format("QuickCurl create error. url: {}", url));
}

bool QuickCurl::ReInitCurl()
{
    return_false_log(curl != nullptr, level::critical, "curl handler not exist");
    curl_easy_reset(curl);
    return_false_log(curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) == CURLE_OK, level::err, "setting url error");
    return_false_log(curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L) == CURLE_OK, level::info, "setting curl error(keepaliave)");
    return_false_log(curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L) == CURLE_OK, level::info, "setting curl error(keepidle)");
    return_false_log(curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L) == CURLE_OK, level::info, "setting curl error(keepintvl)");
    return_false_log(curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L) == CURLE_OK, level::info, "setting curl error(follow)");
    return true;
}

bool QuickCurl::ConnectionTest()
{
    spdlog::info(fmt::format("testing connection: {}", url));
    return_false(ReInitCurl());
    return_false_log(curl_easy_setopt(curl, CURLOPT_RANGE, "0-1") == CURLE_OK, level::err, "setting curl error(range)");
    return_false_log(curl_easy_perform(curl) == CURLE_OK, level::err, "perform curl error");
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    spdlog::log(level::trace, fmt::format("response_code: {}", response_code));
    return_false_log(response_code == 206, level::err, fmt::format("test connection: {} can't connected.\nresponse_code: {}", url, response_code));
    spdlog::log(level::info, fmt::format("success: url[{}] connected.", url));
    return true;
}

bool QuickCurl::FetchRange(FallbackFn &&fallback, const string &sta, const string &end)
{
    auto range = fmt::format("{}-{}", sta, end);
    spdlog::info(fmt::format("fetching range: {} [{}]", url, range));
    return_false(ReInitCurl());
    return_false_log(curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str()) == CURLE_OK, level::err, "setting curl error(range)");
    return_false_log(curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fallback) == CURLE_OK, level::err, "setting curl error(fallback function)");
    return_false_log(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                                      +[](char *contents, size_t size, size_t nmemb,
                                          const std::function<void(const string &)> *hptr) -> size_t {
                                          auto realsize = size * nmemb;
                                          // cout << string(contents, contents + realsize) << endl;
                                          (*hptr)(string(contents, contents + size * nmemb));
                                          // cout << realsize << endl;
                                          return realsize;
                                      }) == CURLE_OK,
                     level::err, "setting curl error(fallback middle function)");
    return_false_log(curl_easy_perform(curl) == CURLE_OK, level::err, "perform curl error");
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    spdlog::log(level::trace, fmt::format("response_code: {}", response_code));
    return_false_log(response_code == 206, level::err, fmt::format("fetching range error in:\n{}\nresponse_code: {}", url, response_code));
    spdlog::log(level::info, fmt::format("fetching range success: {}", url));
    return true;
}

TarOverCurl::TarOverCurl(const string &url, const string &fastAux) : url(url), curl(url), aux(YAML::Load(fastAux)) {}

bool TarOverCurl::ExtractFile(const string &inner_path, FallbackFn &&handler)
{
    return_false_log(aux.IsMap(), level::critical, "auxilary data format is error.");
    return_false_log(aux[inner_path], level::warn, fmt::format("file '{}' not exist in tar or auxilary is out of date.", inner_path));
    return_false_log(aux[inner_path]["start"], level::critical, fmt::format("node '{}' hasn't sub-node '{}'.", inner_path, "start"));
    return_false_log(aux[inner_path]["end"], level::critical, fmt::format("node '{}' hasn't sub-node '{}'.", inner_path, "end"));
    auto beg = aux[inner_path]["start"].as<string>();
    auto end = aux[inner_path]["end"].as<string>();
    return_false_log(curl.FetchRange(std::forward<FallbackFn>(handler), beg, end), level::warn, fmt::format("failure to extract file: {}", inner_path));
    info("extract file success:" + inner_path);
    return true;
}

shared_ptr<vector<string>> TarOverCurl::SearchFileLocation(const regex &reg)
{
    auto path_list = make_shared<vector<string>>();
    for (auto &&item : aux)
    {
        const auto inner_path = item.first.as<string>();
        if (regex_match(inner_path, reg))
            path_list->push_back(inner_path);
    }
    return path_list;
}

bool TarOverCurl::HasFile(const string &inner_path)
{
    return_false_log(aux[inner_path], level::trace, fmt::format("file {} not exist in TarOverCurl.", inner_path));
    return true;
}

TarOverCurl::ull TarOverCurl::GetStarPosByPath(const string &inner_path)
{
    if (HasFile(inner_path))
        return aux[inner_path]["start"].as<ull>();
    return 0;
}

TarOverCurl::ull TarOverCurl::GetEndPosByPath(const string &inner_path)
{
    if (HasFile(inner_path))
        return aux[inner_path]["end"].as<ull>();
    return 0;
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

bool OpkgServer::DownloadAux()
{
    QuickCurl curl(this->aux_url);
    auto contents = make_shared<string>();
    auto length = 0ULL;
    return_false_log(curl.FetchRange([contents, &length](const string &part_conts) {
        if (part_conts.size() == 0)
            return;
        contents->append(part_conts);
        debug("auxilary file downloaded: {}+{}", length, part_conts.size());
        length += part_conts.size();
    }),
                     level::err, "download auxilary file error");
    info("auxilary file download completed");
    info(fmt::format("auxilary file size: {}", contents->length()));
    m_tar = make_shared<TarOverCurl>(tar_url, *contents);
    return true;
}

bool OpkgServer::OutputSubscription()
{
    static int num = 1;
    if (subscription_path == "")
    {
        warn("'subscription path' is empty, disable subscription.");
        return true;
    }
    string subscription_contents;
    regex reg(".*?/Packages\\.gz$");
    auto list = m_tar->SearchFileLocation(reg);
    for (auto &&path : *list)
        subscription_contents += fmt::format("src/gz {} http://{}:{}/{}\n", num++, local_addr, local_port, path.substr(0, path.size() - 12));

    info(fmt::format("subscription: save content to '{}'\n{}", subscription_path, subscription_contents));
    ofstream ofs(subscription_path, ios::out);
    return_false_log(!ofs.fail(), level::err, "subscription file write failure.");
    ofs << subscription_contents;
    ofs.close();
    return true;
}

bool OpkgServer::Check()
{
    info(fmt::format("auxilary url: {}", aux_url));
    info(fmt::format("tar url: {}", tar_url));
    info(fmt::format("subscription output path: {}", subscription_path));
    info(fmt::format("local address: {}", local_addr));
    info(fmt::format("local port: {}", local_port));

    return_false_log(aux_url != "", level::critical, "not provide auxilary url");
    return_false_log(tar_url != "", level::critical, "not provide tar url");
    return_false(DownloadAux());

    return_false(OutputSubscription());
    return_false(DeployServer());
    return true;
}

class SlidingWindow
{
    mutex mtx;
    list<string> buffer;
    using ull = unsigned long long;
    ull beg = 0, end = 0;
    atomic<bool> done, interrupt;

public:
    SlidingWindow()
    {
        done.store(false);
        interrupt.store(false);
    }
    void Interrupt()
    {
        interrupt.store(true);
        buffer.clear();
    }
    void Done() { done.store(true); }
    bool IsInterrupt() { return interrupt.load(); }
    bool IsDone() { return done.load(); }
    bool push(const string &in)
    {
        if (in.size() == 0)
            return true;
        lock_guard<mutex> lock(mtx);
        if (buffer.size() == buffer.max_size())
            return false;
        buffer.push_back(in);
        end += in.size();
        return true;
    }

    string *get()
    {
        lock_guard<mutex> lock(mtx);
        if (buffer.size() == 0)
            return nullptr;
        while (buffer.front().size() == 0)
        {
            buffer.pop_front();
            if (buffer.size() == 0)
                return nullptr;
        }
        return &buffer.front();
    }

    bool MoveTo(ull new_beg)
    {
        lock_guard<mutex> lock(mtx);
        if (new_beg < beg)
        {
            critical("Slidling Windows can't move beg back(from {} to {})", beg, new_beg);
            return false;
        }
        auto remain = new_beg - beg;
        while (remain > 0)
        {
            if (buffer.size() == 0)
            {
                critical("beg isn't sync with buffer size, Please report this bug");
                return false;
            }
            auto part_size = buffer.front().size();
            if (part_size <= remain)
            {
                beg += part_size;
                buffer.pop_front();
                remain -= part_size;
            }
            else
            {
                buffer.front().substr(remain);
                beg += remain;
                remain = 0;
            }
        }
        return true;
    }
};

bool OpkgServer::DeployServer()
{
    debug("Deploying server.");
    using namespace httplib;
    auto svr = m_svr = make_shared<Server>();
    auto tar = m_tar;
    m_svr->Get("/stop", [svr](const Request &req, Response &res) {
        svr->stop();
        info("shutdown opkg server");
    });
    m_svr->Get("/.*?/(.*?\\.ipk|Packages(\\.gz|\\.sig|\\.manifest)?)$", [tar](const Request &req, Response &res) {
        auto inner_path = make_shared<string>(req.path.substr(1));
        trace("request " + req.path);
        if (!tar->HasFile(*inner_path))
        {
            debug(fmt::format("file not exist, return 404: {}", *inner_path));
            res.status = 404;
            return;
        }
        auto data = make_shared<SlidingWindow>();
        auto beg = tar->GetStarPosByPath(*inner_path);
        auto end = tar->GetEndPosByPath(*inner_path);
        auto size = end - beg + 1;
        thread in_coming([tar, size, inner_path, data]() {
            TarOverCurl::ull progress = 0;
            struct InterruptException : public std::exception
            {
            };
            try
            {
                tar->ExtractFile(*inner_path, [&progress, size, data](const string &part_conts) {
                    if (part_conts.size() == 0)
                        return;
                    while (false == data->push(part_conts))
                        std::this_thread::sleep_for(2s);
                    if (data->IsInterrupt())
                        throw InterruptException();
                    progress += part_conts.length();
                    debug(fmt::format(FMT_STRING("Download Progress: {}/{} {:*^15.2f}"), progress, size, progress * 100.0 / size));
                });
            }
            catch (InterruptException &)
            {
                return;
            }
            data->Done();
            if (progress != size)
                data->Interrupt();
        });
        in_coming.detach();
        res.set_content_provider(
            size, // Content length
            [data, size](uint64_t offset, uint64_t length, DataSink &sink) {
                // sync offset
                if (data->MoveTo(offset) == false)
                {
                    data->Interrupt();
                    sink.done();
                    return;
                }

                auto data_part = data->get();
                if (data_part == nullptr)
                {
                    std::this_thread::sleep_for(0.01s);
                    return;
                }
                sink.write(data_part->c_str(), data_part->size());
                debug("Upload Progress: {}/{} {:*^15.2f}", offset, size, offset * 100.0 / size);
            },
            [size, data]() {
                if (data->IsInterrupt())
                    error("Transfer file status: failure");
                else if (data->IsDone())
                    debug("Upload Progress: {0}/{0} {1:*^15.2f}", size, 100.0);
                else
                    critical("Transfer file status: Unknow. Please report this bug.");
            });
    });
    spdlog::log(level::info, "Deploy done.");
    return true;
}

bool OpkgServer::Start()
{
    return_false_log(m_svr.get() != nullptr, level::critical, "server not deploy");
    return_false_log(m_tar.get() != nullptr, level::critical, "TarOvercurl isn't setup");
    return_false_log(m_svr->listen(local_addr.c_str(), local_port), level::critical, "listen error");
    return false;
}

} // namespace ootoc
