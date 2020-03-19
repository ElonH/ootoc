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
    static bool LogWhenFalse(bool rst, const string &msg);
    static bool LogWhenFalse(bool rst, spdlog::level::level_enum level, const string &msg);
};

class QuickCurl
{
    CURL *curl = nullptr;
    string url;
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
    // TAR *tar = nullptr;
    CURL *curl = nullptr;
    string url = "";
    string aux = "";

public:
    TarOverCurl() {}
    ~TarOverCurl();
    /**
     * @brief enstablish a tar node
     * @param url a url path that point to tar file on network.
     * Note: The remote server recommend support range requests.
     * @param fastAux yaml content that contain file location info in tar.
     * @return true open success
     * @return false failure
     */
    bool Open(const string &url, const string &fastAux);
    /**
     * @brief extract a file content in tar.
     * @param inner_path a file path in tar
     * @param handler handle the file content pices by pices.
     * @return true success
     * @return false failure
     */
    bool ExtractFile(const string &inner_path, std::function<void(const string &)> &&handler);
    /**
     * @brief closing network connection
     */
    bool Close();

protected:
    bool ReInitCurl();
    bool ExecuteCurl();
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
    httplib::Server svr;
    string aux = "";
    string aux_url = "";
    string aux_path = "";
    TarOverCurl remote;
    string addr;
    string subscription;
    long port;

public:
    void setSubscription(const string &sub_path);
    void setAuxUrl(const string &url, const string &path);
    bool fetchAux();
    void setRemoteTar(const string &url);
    void setServer(const string &addr, long port);
    void Start();
};
} // namespace ootoc
