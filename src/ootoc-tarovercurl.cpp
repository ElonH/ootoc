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

TarOverCurl::TarOverCurl(const string &url, const string &fastAux) : url(url), aux(YAML::Load(fastAux)) {}

bool TarOverCurl::ExtractFile(const string &inner_path, FallbackFn &&handler)
{
    QuickCurl curl(url);
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
} // namespace ootoc
