#include "CLI/CLI.hpp"
#include "ootoc.h"

using namespace ootoc;

int main(int argc, char **argv)
{
    CLI::App app;

    auto appSvr = app.add_subcommand("server", "start ootoc server");
    auto appPsr = app.add_subcommand("parse", "parse tar archive file");

    string log_path = "";
    appSvr->add_option("--log,-l", log_path, "log file, otherwise output log to console", true);
    appPsr->add_option("--log,-l", log_path, "log file, otherwise output log to console", true);
    string addr = "127.0.0.1";
    appSvr->add_option("--addr", addr, "local address", true);
    int port = 21730;
    appSvr->add_option("--port", port, "local port", true);
    string tar_url;
    appSvr->add_option("--tar-url", tar_url)->required();
    string aux_url;
    appSvr->add_option("--aux-url", aux_url)->required();
    string tar_path;
    appPsr->add_option("--in,-i", tar_path, "tar file")->required();

    string sub_path = "";
    appSvr->add_option("--subscription", sub_path, "subscription file path. Default, disable output subscription content.", true);
    string out;
    appPsr->add_option("--out,-o", out, "output result[yaml file]")->required();

    app.callback([&]() { cout << app.help() << endl; });
    appSvr->callback([&]() {
        Logger::start(log_path);
        OpkgServer server;
        server.tar_url = tar_url;
        server.aux_url = aux_url;
        server.local_addr = addr;
        server.local_port = port;
        server.subscription_path = sub_path;
        server.Check();
        server.Start();
    });

    appPsr->callback([&]() {
        Logger::start(log_path);
        TarParser psr;
        psr.Open(tar_path);
        psr.Parse();
        psr.Close();
        fstream ofs(out, ios::out);
        ofs << psr.GetOutput();
        cout << "finished." << endl;
        ofs.close();
    });

    CLI11_PARSE(app, argc, argv);
    return 0;
}
