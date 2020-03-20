extern "C"
{
#include <curl/curl.h>
#include <libtar.h>
}
#include <functional>
#include <httplib/httplib.h>
#include <spdlog/spdlog.h>
#include <string>
#include <yaml-cpp/yaml.h>

namespace ootoc
{
using namespace std;

class Logger
{
public:
    static void start(const string &file_path = "");
};

class QuickCurl
{
    CURL *curl = nullptr;
    string url;

public:
    using FallbackFn = std::function<void(const string &)>;

protected:
    /**
     * @brief reset curl option
     */
    bool ReInitCurl();

public:
    QuickCurl(const string &url);
    ~QuickCurl()
    {
        if (curl)
            curl_easy_cleanup(curl);
    }
    /**
     * @brief validate url existence
     */
    bool ConnectionTest();
    /**
     * @brief fetching some data of range [sta, end] Tip: not [sta,end)
     * @param fallback a fallback function that handle data pices by pices
     * @param sta string of number, default is "0"
     * @param end string of number, default is "" (mean that fetching to the end of data)
     */
    bool FetchRange(FallbackFn &&fallback, const string &sta = "0", const string &end = "");
};

class TarOverCurl
{
    QuickCurl curl;
    string url = "";
    YAML::Node aux;

public:
    using FallbackFn = QuickCurl::FallbackFn;
    using ull = unsigned long long;

public:
    /**
     * @brief enstablish a tar node
     * @param url a url path that point to tar file on network.
     * Note: The remote server needs support range requests.
     * @param fastAux yaml content that contain file location info in tar.
     */
    TarOverCurl(const string &url, const string &fastAux);
    ~TarOverCurl() {}

    /**
     * @brief extract a file content in tar.
     * @param inner_path a file path in tar
     * @param handler handle the file content pices by pices.
     * @return true success
     * @return false failure
     */
    bool ExtractFile(const string &inner_path, FallbackFn &&handler);
    shared_ptr<vector<string>> SearchFileLocation(const regex &reg); // TODO: test
    bool HasFile(const string &inner_path);                          // TODO: test
    ull GetStarPosByPath(const string &inner_path);                  // TODO: test
    ull GetEndPosByPath(const string &inner_path);                   // TODO: test
};

/**
 * @brief generate a auxilary yml file that help `TarOverCurl` quickly locate file
 */
class TarParser
{
    TAR *tar = nullptr;
    string output = "";

public:
    TarParser() {}
    ~TarParser();
    bool Open(const string &path);
    bool Close();
    bool Parse();
    const string &GetOutput();
};

class OpkgServer
{
    shared_ptr<httplib::Server> m_svr;
    shared_ptr<TarOverCurl> m_tar;

public:
    string aux_url = "";
    string tar_url = "";
    string subscription_path = "";
    string local_addr;
    long local_port;

public:
    bool Check();
    bool Start();

private:
    bool DownloadAux();
    bool OutputSubscription();
    bool DeployServer();
};
} // namespace ootoc
