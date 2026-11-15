#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QDebug>
#include <stdexcept>
#include "src/AI/KataGoEvaluator.h"

#include "src/Core/Game.h"
#include "src/Managers/AuthManager.h"
#include "src/Managers/MatchmakingManager.h"
#include "src/Network/NetworkClient.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOptions({
        {"katago-model", "KataGo .bin.gz model for Hard mode", "path"},
        {"katago-fp32", "Disable FP16 for older convolution models"},
        {"ai-threads", "MCTS workers (1-16)", "number", "4"},
        {"ai-time-ms", "Move search budget in milliseconds", "ms", "1800"},
        {"katago-duty", "Approximate GPU inference duty (0,1]", "fraction", "0.30"}
    });
    parser.process(app);
    std::shared_ptr<KataGoEvaluator> kataGo;
    HardSearchOptions hardOptions;
    if (parser.isSet("katago-model")) {
        try {
            bool threadsOK = false, timeOK = false, dutyOK = false;
            hardOptions.searchThreads = parser.value("ai-threads").toInt(&threadsOK);
            hardOptions.timeBudgetMs = parser.value("ai-time-ms").toInt(&timeOK);
            KataGoEvaluator::Options options;
            options.modelPath = parser.value("katago-model");
            options.fp16 = !parser.isSet("katago-fp32");
            options.batchSize = hardOptions.searchThreads;
            options.duty = parser.value("katago-duty").toDouble(&dutyOK);
            if (!threadsOK || !timeOK || !dutyOK || hardOptions.timeBudgetMs < 1
                || hardOptions.timeBudgetMs > 60000 || hardOptions.searchThreads < 1
                || hardOptions.searchThreads > 16) throw std::runtime_error("Invalid AI command line options");
            kataGo = std::make_shared<KataGoEvaluator>(options);
            qInfo() << "Loading and warming KataGo model...";
            kataGo->prepare();
        } catch (const std::exception& e) {
            qCritical() << "Cannot start KataGo HardAI:" << e.what(); return 2;
        }
    }

    // I dont know why above code work

    NetworkClient networkClient;

    MatchmakingManager matchmakingManager;
    matchmakingManager.setNetworkClient(&networkClient);

    AuthManager authManager;
    authManager.setNetworkClient(&networkClient);

    GameManager gameManager;
    gameManager.setNetworkClient(&networkClient);
    if (kataGo) gameManager.configureHard(hardOptions, kataGo);

    QObject::connect(&networkClient,
                     &NetworkClient::messageReceived,
                     &gameManager,
                     &GameManager::handleServerMessage);

    QObject::connect(&networkClient,
                     &NetworkClient::messageReceived,
                     &matchmakingManager,
                     &MatchmakingManager::handleServerMessage);

    QObject::connect(&networkClient,
                     &NetworkClient::messageReceived,
                     &authManager,
                     &AuthManager::handleServerMessage);

    QObject::connect(&matchmakingManager,
                     &MatchmakingManager::matchFound,
                     &gameManager,
                     &GameManager::setMatchInfo);

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/"));
    engine.rootContext()->setContextProperty("networkClient", &networkClient);
    engine.rootContext()->setContextProperty("matchmakingManager", &matchmakingManager);
    engine.rootContext()->setContextProperty("gameManager", &gameManager);
    engine.rootContext()->setContextProperty("authManager", &authManager);

    networkClient.connectToServer(networkClient.serverUrl());

    engine.load(QUrl(QStringLiteral("qrc:/GoGame/main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
