#include "../src/Core/Game.h"
#include "../src/Managers/AssetManager.h"
#include "../src/Managers/AuthManager.h"
#include "../src/Managers/MatchmakingManager.h"
#include <QGuiApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QThread>
#include <cstdio>
#include <cstdlib>

void check(bool ok, const char *message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
template<class Predicate> void waitFor(Predicate predicate) {
    QElapsedTimer timer; timer.start();
    while (!predicate() && timer.elapsed() < 6000) {
        QCoreApplication::processEvents(); QThread::msleep(1);
    }
    check(predicate(), "Timed out waiting for AI / UI");
}
QObject *findVisual(QQuickItem *item, const QString &name) {
    if (item->objectName() == name) return item;
    for (auto child : item->childItems())
        if (auto found = findVisual(child, name)) return found;
    return nullptr;
}
int main(int argc, char **argv) {
    std::fprintf(stderr, "Creating isolated storage\n");
    QTemporaryDir storage;
    check(storage.isValid(), "Cannot create test storage");
    qputenv("XDG_DATA_HOME", storage.path().toUtf8());
    qputenv("GOGAME_SAVE_DIR", storage.path().toUtf8());
    std::fprintf(stderr, "Initializing Qt\n");
    QGuiApplication app(argc, argv);
    std::fprintf(stderr, "Testing solo controller\n");
    GameManager game;
    NetworkClient network;
    game.setNetworkClient(&network); // Offline solo must not require a connection.
    check(game.startMediumGame(0), "Cannot start solo as black");
    check(game.aiEnabled() && !game.online() && game.canAct(), "Invalid solo initial state");
    game.update_pieces(3,3);
    check(game.pending() && !game.canAct(), "AI search did not lock human input");
    waitFor([&] { return !game.pending(); });
    check(game.snapshot()["moveCount"].toInt() == 2 && game.currentPlayer() == 0,
          "AI did not reply to human move");
    check(game.boardState()[60].toInt() == 0, "Human stone disappeared");
    const int slot = game.currentSaveSlot();
    check(game.exitLocalGame() && !game.aiEnabled(), "Cannot exit solo");
    check(game.loadAIGame(slot) && game.snapshot()["moveCount"].toInt() == 2,
          "Cannot resume solo save");
    check(game.startMediumGame(1), "Cannot start solo as white");
    waitFor([&] { return !game.pending(); });
    check(game.currentPlayer() == 1 && game.canAct(), "AI did not open as black");
    game.update_pieces(10,10);
    check(game.pending(), "Missing pending search before cancellation");
    check(game.startSandbox(), "Cannot switch to sandbox");
    // A stale worker must not write its result into the new sandbox position.
    QElapsedTimer timer; timer.start();
    while (timer.elapsed() < 1200) { QCoreApplication::processEvents(); QThread::msleep(1); }
    check(game.sandbox() && game.snapshot()["moveCount"].toInt() == 0,
          "Stale AI result changed sandbox");

    check(game.startHardGame(0), "Cannot start Hard as black");
    check(game.aiDifficulty() == "Hard" && game.snapshot()["mode"].toString() == "Hard", "Wrong Hard mode");
    game.update_pieces(3,3);
    waitFor([&] { return !game.pending(); });
    check(game.snapshot()["moveCount"].toInt() == 2, "Hard did not reply");
    const int hardSlot = game.currentSaveSlot();
    check(game.exitLocalGame() && game.loadAIGame(hardSlot), "Hard save did not load");
    check(game.aiDifficulty() == "Hard", "Hard difficulty lost after load");
    QFile hardFile(DataManager::savePath(true, hardSlot));
    check(game.startSandbox(), "Cannot start independent sandbox");
    // Leaving AI intentionally auto-saves it; compare after that operation.
    check(hardFile.open(QIODevice::ReadOnly), "Cannot inspect Hard save");
    const auto hardBytes = hardFile.readAll(); hardFile.close();
    game.update_pieces(0,0);
    check(game.saveGame() && game.hasSandboxSave(), "Sandbox slot not saved");
    check(DataManager::savePath(false,0) != DataManager::savePath(true,hardSlot), "Sandbox path overlaps AI slot");
    game.resetGame();
    check(game.loadSandbox() && game.snapshot()["moveCount"].toInt() == 1 && !game.aiEnabled(), "Sandbox slot failed to restore");
    check(hardFile.open(QIODevice::ReadOnly) && hardFile.readAll() == hardBytes, "Sandbox overwrote Hard slot"); hardFile.close();
    check(game.startHardGame(1), "Cannot start Hard as white");
    check(game.pending(), "Hard opening not dispatched");
    check(game.startSandbox(), "Cannot cancel Hard into sandbox");
    timer.restart();
    while (timer.elapsed() < 400) { QCoreApplication::processEvents(); QThread::msleep(1); }
    check(game.sandbox() && game.snapshot()["moveCount"].toInt() == 0, "Stale Hard result changed sandbox");

    AuthManager auth;
    MatchmakingManager matchmaking;
    matchmaking.setNetworkClient(&network);
    AssetManager assets;
    qmlRegisterSingletonInstance("GoGame", 1, 0, "AssetManager", &assets);
    const QString root = QStringLiteral(GOGAME_SOURCE_DIR);
    for (const auto &name : {"Theme", "Motion", "Radius", "Spacing", "Typography"})
        qmlRegisterSingletonType(QUrl::fromLocalFile(root + "/qml/Core/" + name + ".qml"),
                                 "GoGame", 1, 0, name);
    qmlRegisterSingletonType(QUrl::fromLocalFile(root + "/qml/Singletons/AppState.qml"),
                             "GoGame", 1, 0, "AppState");
    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlEngine::warnings, &app, [](const QList<QQmlError> &errors) {
        for (const auto &error : errors) std::fprintf(stderr, "%s\n", qPrintable(error.toString()));
    });
    engine.rootContext()->setContextProperty("gameManager", &game);
    engine.rootContext()->setContextProperty("networkClient", &network);
    engine.rootContext()->setContextProperty("matchmakingManager", &matchmaking);
    engine.rootContext()->setContextProperty("authManager", &auth);
    const auto qml = QString("import QtQuick\nimport QtQuick.Controls\nimport \"../qml/Screens\" as Screens\n"
        "ApplicationWindow { visible: true; width: 1280; height: 720; "
        "StackView { id: stack; anchors.fill: parent; initialItem: Screens.MainMenu {} } }");
    engine.loadData(qml.toUtf8(), QUrl::fromLocalFile(root + "/tests/MenuFixture.qml"));
    check(!engine.rootObjects().isEmpty(), "Cannot load MainMenu test fixture");
    auto window = engine.rootObjects().first();
    auto find = [&](const QString &name) -> QObject * {
        if (auto found = window->findChild<QObject *>(name)) return found;
        return findVisual(qobject_cast<QQuickWindow *>(window)->contentItem(), name);
    };
    auto button = find("playAgainstAIButton");
    check(button && button->property("enabled").toBool(), "Solo button disabled offline");
    check(QMetaObject::invokeMethod(button, "clicked"), "Cannot activate solo button");
    QCoreApplication::processEvents();
    waitFor([&] { return find("aiDifficultyMedium") != nullptr; });
    auto medium = find("aiDifficultyMedium");
    auto easy = find("aiDifficultyEasy");
    auto hard = find("aiDifficultyHard");
    check(medium && medium->property("enabled").toBool(), "Medium not selectable");
    check(easy && hard && !easy->property("enabled").toBool() && hard->property("enabled").toBool(),
          "Hard must be selectable while Easy remains unavailable");
    check(QMetaObject::invokeMethod(hard, "clicked"), "Cannot choose Hard");
    waitFor([&] { return find("goBoard") != nullptr; });
    check(game.aiDifficulty() == "Hard", "Hard menu selected Medium instead");
    check(QMetaObject::invokeMethod(medium, "clicked"), "Cannot choose Medium");
    waitFor([&] { return find("goBoard") != nullptr; });
    check(game.aiEnabled() && !game.online() && game.aiDifficulty() == "Medium", "Menu did not start Medium solo");
    auto sandboxLoad = find("loadSandboxSlotButton");
    check(sandboxLoad && sandboxLoad->property("enabled").toBool(), "Dedicated sandbox load slot unavailable");
    for (int slot = 1; slot <= DataManager::AISaveSlots; ++slot)
        check(game.saveGame(slot), "Cannot fill save slot");
    check(!game.startMediumGame(0) && game.aiEnabled(), "Full slots silently overwritten");
    check(game.startMediumGame(0, 20), "Confirmed overwrite failed");
    check(!game.startMediumGame(3, 1), "Invalid player color accepted");
    std::puts("Offline solo, AI replies, save/load, cancellation, full slots and menu navigation passed");
}
