#include <iostream>
extern "C"
{
#include <curl/curl.h>
#include <fcntl.h>
#include <libtar.h>
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << "tarfile output" << std::endl;
        return 1;
    }
    TAR *t = nullptr;

    if (tar_open(&t, argv[1], nullptr, O_RDONLY, 0, 0) != 0)
    {
        std::cerr << "Fail to open file " << argv[1] << std::endl;
        return 1;
    }

    while (!th_read(t))
    {
        std::string filename = th_get_pathname(t);
        std::cout << filename << std::endl;
        th_print(t);
        // if( TH_ISREG(t)
    }
    tar_close(t);
    t = nullptr;
    std::cout << "Hello, world!\n";
}
