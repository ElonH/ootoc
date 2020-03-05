extern "C"
{
#include <libtar.h>
}

#include <string>

namespace ootoc
{
using namespace std;
class TarOverCurlSet
{
    TAR *tar = nullptr;

public:
    TarOverCurlSet(const string &tar_file_path);
    ~TarOverCurlSet();
};

class TarParser
{
    TAR *tar = nullptr;
    string output = "";

public:
    TarParser() {}
    ~TarParser();
    bool Open(const string& path);
    bool Close();
    bool Parse();
    const string& GetOutput();

};
} // namespace ootoc
