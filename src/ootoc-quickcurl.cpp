
#include "ootoc.h"
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
} // namespace ootoc
