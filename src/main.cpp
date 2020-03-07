#include "CLI/CLI.hpp"
#include "ootoc.h"

using namespace ootoc;

int main(int argc, char **argv)
{
    CLI::App app;

    string addr = "127.0.0.1";
    app.add_option("--addr", addr);

    int port = 21730;
    app.add_option("--port", port)->check(CLI::Range(1 << 15));

    string url;
    app.add_option("--url", url)->required();

    string aux;
    app.add_option("--aux", aux)->required();

    CLI11_PARSE(app, argc, argv);

    fstream auxstm(aux, ios::in);
    string aux_content((std::istreambuf_iterator<char>(auxstm)),
                       (std::istreambuf_iterator<char>()));
    auxstm.close();

    OpkgServer server;
    server.setRemoteTar(url, aux_content);
    server.setServer(addr, port);
    server.Start();
    return 0;
}
