#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QDebug>
#include <stdexcept>
#include "src/AI/KataGoEvaluator.h"
#include "src/AI/KataGoEngine.h"

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
        {"katago-engine", "Use an external KataGo executable for Hard", "path"},
        {"katago-config", "Optional external KataGo analysis config", "path"},
        {"katago-fp32", "Disable FP16 for older convolution models"},
        {"ai-threads", "MCTS workers (1-16)", "number", "4"},
        {"ai-time-ms", "Move search budget in milliseconds", "ms", "1800"},
        {"katago-duty", "Approximate inference duty (0,1]", "fraction", "0.30"}
    });
    parser.process(app);
    std::shared_ptr<KataGoEvaluator> kataGo;
    std::shared_ptr<KataGoEngine> kataGoEngine;
    HardSearchOptions hardOptions;
    if ((parser.isSet("katago-engine") || parser.isSet("katago-config")) && !parser.isSet("katago-model")) {
        qCritical() << "--katago-engine/--katago-config requires --katago-model";
        return 2;
    }
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
            qInfo() << "Loading and warming KataGo model...";
            if (parser.isSet("katago-engine")) {
                if (parser.isSet("katago-duty"))
                    throw std::runtime_error("--katago-duty applies only to the native backend");
                KataGoEngine::Options engineOptions;
                engineOptions.executablePath = parser.value("katago-engine");
                engineOptions.modelPath = options.modelPath;
                engineOptions.configPath = parser.value("katago-config");
                engineOptions.timeBudgetMs = hardOptions.timeBudgetMs;
                engineOptions.searchThreads = hardOptions.searchThreads;
                engineOptions.fp16 = options.fp16;
                kataGoEngine = std::make_shared<KataGoEngine>(engineOptions);
                kataGoEngine->prepare();
            } else {
                if (parser.isSet("katago-config"))
                    throw std::runtime_error("--katago-config requires --katago-engine");
                kataGo = std::make_shared<KataGoEvaluator>(options);
                kataGo->prepare();
            }
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
    if (kataGo || kataGoEngine) gameManager.configureHard(hardOptions, kataGo, kataGoEngine);

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
