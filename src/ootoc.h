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
} // namespace ootoc
