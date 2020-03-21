
#include "ootoc.h"
#include "spdlog/details/log_msg_buffer.h"
#include "spdlog/details/null_mutex.h"
#include "spdlog/sinks/base_sink.h"
#include <iostream>

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
}
