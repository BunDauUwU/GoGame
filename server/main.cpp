#include "Server.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <cstdio>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("GoGameServer"));
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Local WebSocket JSON file server"));
    parser.addHelpOption();
    parser.addOption({{"p", "port"}, "Listen port (localhost only).", "port", "14509"});
    parser.addOption({{"d", "data-file"}, "JSON storage file.", "path", "data/state.json"});
    parser.process(app);

    bool portOk = false;
    const uint port = parser.value("port").toUInt(&portOk);
    if (!portOk || port == 0 || port > 65535) {
        std::fprintf(stderr, "Port must be between 1 and 65535\n");
        return 1;
    }
    Server server(parser.value("data-file"));
    QString error;
    if (!server.start(static_cast<quint16>(port), error)) {
        std::fprintf(stderr, "%s\n", qPrintable(error));
        return 1;
    }
    std::fprintf(stdout, "Listening on ws://127.0.0.1:%u\nJSON storage: %s\n",
                 port, qPrintable(server.dataFile()));
    std::fflush(stdout);
    return app.exec();
}
