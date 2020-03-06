extern "C"
{
#include <curl/curl.h>
#include <libtar.h>
}
#include <functional>
#include <string>

namespace ootoc
{
using namespace std;
class TarOverCurl
{
    // TAR *tar = nullptr;
    CURL *curl = nullptr;
    string url = "";
    string aux = "";

public:
    TarOverCurl() {}
    ~TarOverCurl();
    bool Open(const string &url, const string &fastAux);
    bool ExtractFile(const string &inner_path, std::function<void(const string &)> &&);
    bool Close();
protected:
    bool ReInitCurl();
    bool ExecuteCurl();
};

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
} // namespace ootoc
