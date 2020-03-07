#include "CLI/CLI.hpp"
#include "ootoc.h"

using namespace ootoc;

int main(int argc, char **argv)
{
    CLI::App app;

    auto appSvr = app.add_subcommand("server", "start ootoc server");
    auto appPsr = app.add_subcommand("parse", "parse tar archive file");

    string addr = "127.0.0.1";
    appSvr->add_option("--addr", addr);
    int port = 21730;
    appSvr->add_option("--port", port)->check(CLI::Range(1 << 15));
    string url;
    appSvr->add_option("--url", url)->required();
    string aux;
    appSvr->add_option("--aux", aux)->required();
    appPsr->add_option("--in,-i", aux, "tar file")->required();

    string out;
    appPsr->add_option("--out,-o", out, "output result[yaml file]")->required();

    // app.callback([&]() { cout << app.help() << endl; });
    appSvr->callback([&]() {
        fstream auxstm(aux, ios::in);
        string aux_content((std::istreambuf_iterator<char>(auxstm)),
                           (std::istreambuf_iterator<char>()));
        auxstm.close();

        OpkgServer server;
        server.setRemoteTar(url, aux_content);
        server.setServer(addr, port);
        server.Start();
    });

    appPsr->callback([&]() {
        TarParser psr;
        psr.Open(aux);
        psr.Parse();
        psr.Close();
        fstream ofs(out, ios::out);
        ofs << psr.GetOutput();
        // cout << psr.GetOutput() << endl;
        cout << "finished." << endl;
        ofs.close();
    });

    CLI11_PARSE(app, argc, argv);
    return 0;
}
