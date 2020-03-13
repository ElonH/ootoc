#include "CLI/CLI.hpp"
#include "ootoc.h"

using namespace ootoc;

int main(int argc, char **argv)
{
    CLI::App app;

    auto appSvr = app.add_subcommand("server", "start ootoc server");
    auto appPsr = app.add_subcommand("parse", "parse tar archive file");
    auto appSub = app.add_subcommand("subscription", "output subscription of server");

    string log_path = "";
    appSvr->add_option("--log,-l", log_path, "log file, otherwise output log to console");
    appPsr->add_option("--log,-l", log_path, "log file, otherwise output log to console");
    appSub->add_option("--log,-l", log_path, "log file, otherwise output log to console");
    string addr = "127.0.0.1";
    appSvr->add_option("--addr", addr);
    appSub->add_option("--addr", addr);
    int port = 21730;
    appSvr->add_option("--port", port);
    appSub->add_option("--port", port);
    string url;
    appSvr->add_option("--url", url)->required();
    string aux;
    appSvr->add_option("--aux", aux)->required();
    appSub->add_option("--in,-i", aux)->required();
    appPsr->add_option("--in,-i", aux, "tar file")->required();
    string online_aux = "";
    appSvr->add_option("--aux-url", online_aux);
    appSub->add_option("--aux-url", online_aux);

    string out;
    appPsr->add_option("--out,-o", out, "output result[yaml file]")->required();

    // app.callback([&]() { cout << app.help() << endl; });
    appSvr->callback([&]() {
        Logger::start(log_path);
        OpkgServer server;
        
        server.setAuxUrl(online_aux, aux);
        server.setRemoteTar(url);
        server.setServer(addr, port);
        server.Start();
    });

    appSub->callback([&]() {
        Logger::start(log_path);

        OpkgServer server;
        
        server.setAuxUrl(online_aux, aux);
        server.setRemoteTar(url);
        cout << server.getSubscription(addr, port) << endl;
    });

    appPsr->callback([&]() {
        Logger::start(log_path);
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
