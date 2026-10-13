#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "src/Core/Game.h"
#include "src/Network/NetworkClient.h"
#include "src/Managers/AssetManager.h"
#include "src/Managers/AuthManager.h"
#include "src/Managers/MatchmakingManager.h"


#ifdef QT_DEBUG
#include <QDebug>
#define Debug(x) do { qDebug() << (x); } while (0)
#else
#define Debug(x) do { } while (0)
#endif


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    NetworkClient networkClient;

    AssetManager assetManager;

    MatchmakingManager matchmakingManager;
    matchmakingManager.setNetworkClient(&networkClient);

    AuthManager authManager;
    authManager.setNetworkClient(&networkClient);

    GameManager gameManager;
    gameManager.setNetworkClient(&networkClient);

    QObject::connect(
        &networkClient,
        &NetworkClient::messageReceived,
        &gameManager,
        &GameManager::handleServerMessage
        );

    QObject::connect(
        &networkClient,
        &NetworkClient::messageReceived,
        &matchmakingManager,
        &MatchmakingManager::handleServerMessage
        );

    QObject::connect(
        &networkClient,
        &NetworkClient::messageReceived,
        &authManager,
        &AuthManager::handleServerMessage
        );

    QObject::connect(
        &matchmakingManager,
        &MatchmakingManager::matchFound,
        &gameManager,
        &GameManager::setMatchInfo
        );

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
