#include "ootoc.h"
#include <regex>

#define return_false(ret) \
    if (!(ret))           \
    return false

#define return_false_log(ret, level, msg) \
    if (!(ret))                           \
    {                                     \
        spdlog::log(level, msg);          \
        return false;                     \
    }

namespace ootoc
{
using namespace spdlog;

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
    bool push(const string &in, bool done = false)
    {
        if (IsDone() || IsInterrupt())
            return true;
        if (in.size() == 0)
            return true;
        lock_guard<mutex> lock(mtx);
        if (buffer.size() == buffer.max_size())
            return false;
        buffer.push_back(in);
        end += in.size();
        if (done)
            Done();
        return true;
    }

    shared_ptr<string> get()
    {
        lock_guard<mutex> lock(mtx);
        auto ans = make_shared<string>();
        if (buffer.size() == 0)
            return ans;
        while (buffer.front().size() == 0)
        {
            buffer.pop_front();
            if (buffer.size() == 0)
                return ans;
        }
        for (auto &&item : buffer)
            *ans += item;
        return ans;
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
        debug("request " + req.path);
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
                    progress += part_conts.length();
                    while (false == data->push(part_conts, progress == size))
                        std::this_thread::sleep_for(2s);
                    if (data->IsInterrupt())
                        throw InterruptException();
                    debug(fmt::format(FMT_STRING("Download Progress: {}/{} {:*^15.2f}"), progress, size, progress * 100.0 / size));
                });
            }
            catch (InterruptException &)
            {
                warn("Download Abort.");
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
                if (data_part->size() == 0)
                {
                    std::this_thread::sleep_for(0.01s);
                    return;
                }
                sink.write(data_part->c_str(), data_part->size());
                offset += data_part->size();
                debug("Upload Progress: {}/{} {:*^15.2f}", offset, size, offset * 100.0 / size);
            },
            [size, data]() {
                if (data->IsInterrupt())
                    error("Transfer file status: failure");
                else if (data->IsDone())
                    debug("Transfer file status: success");
                else
                {
                    critical("Transfer file status: Abort");
                    data->Interrupt();
                }
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
