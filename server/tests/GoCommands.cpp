#include "Server.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QWebSocket>
#include <QThread>
#include <cmath>
#include <cstdio>
#include <cstdlib>

void check(bool ok, const char *text) {
    if (!ok) { std::fprintf(stderr, "%s\n", text); std::exit(1); }
}
template<class Predicate> void waitFor(Predicate ready) {
    QElapsedTimer clock; clock.start();
    while (!ready() && clock.elapsed() < 3000) {
        QCoreApplication::processEvents(); QThread::msleep(1);
    }
    check(ready(), "WebSocket timeout");
}
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    GoMatch rules; QString error;
    check(rules.play(0,0,error), "Initial play failed");
    check(!rules.play(0,0,error) && error == "Occupied" && rules.moveCount == 1, "Occupied changed state");
    check(rules.play(0,1,error) && rules.play(10,10,error) && rules.play(1,0,error), "Capture setup failed");
    check(rules.state()[0].toInt() == -1, "Capture failed");
    check(!rules.play(0,0,error) && error == "Suicide", "Suicide allowed");
    GoMatch ko;
    for (auto pos : {QPair<int,int>{0,1}, {0,2}, {1,0}, {1,1}, {2,1}, {2,2}, {10,10}, {1,3}, {1,2}})
        check(ko.play(pos.first,pos.second,error), "Ko setup failed");
    check(!ko.play(1,1,error) && error == "Ko", "Immediate ko recapture allowed");
    ko.pass(); ko.pass();
    check(ko.play(1,1,error), "Ko did not clear after passes");
    GoMatch independent;
    check(independent.state()[1].toInt() == -1, "Match boards shared state");
    check(std::abs(independent.score() + 6.5) < 0.001, "Komi calculation failed");

    QTemporaryDir storage;
    Server server(storage.filePath("state.json"));
    check(server.start(24510,error), "Server start failed");
    QWebSocket a,b,outsider;
    QMap<QWebSocket*, QList<QJsonObject>> messages;
    for (auto socket : {&a,&b,&outsider}) {
        QObject::connect(socket,&QWebSocket::textMessageReceived,&app,[&,socket](const QString &text) {
            QJsonObject json; QString error;
            check(Server::stringToJson(text,json,error),"Bad server JSON");
            messages[socket].append(json);
        });
        socket->open(QUrl("ws://127.0.0.1:24510"));
        waitFor([&] { return socket->state() == QAbstractSocket::ConnectedState; });
    }
    auto send = [&](QWebSocket &socket, QString type, QJsonObject payload) {
        socket.sendTextMessage(Server::jsonToString(Server::envelope(type,payload)));
    };
    auto receive = [&](QWebSocket &socket, QString type) {
        auto find = [&] { for (int i=0;i<messages[&socket].size();++i)
            if (messages[&socket][i]["type"] == type) return i; return -1; };
        waitFor([&] { return find() >= 0; });
        return messages[&socket].takeAt(find())["payload"].toObject();
    };
    send(a,"MatchmakingStart",{{"mode","Go"}});
    check(receive(a,"Error")["code"] == "NotAuthenticated", "Unauthenticated matchmaking allowed");
    send(a,"AuthRequest",{{"nickname","Black"}}); receive(a,"AuthResult");
    send(b,"AuthRequest",{{"nickname","White"}}); receive(b,"AuthResult");
    send(a,"MatchmakingStart",{{"mode",""}});
    check(receive(a,"Error")["code"] == "InvalidMode", "Empty mode accepted");
    send(b,"MatchmakingStart",{{"mode","Go"}}); receive(b,"MatchmakingStatus");
    send(b,"MatchmakingStart",{{"mode","Go"}}); receive(b,"MatchmakingStatus");
    send(b,"MatchmakingCancel",{});
    check(!receive(b,"MatchmakingStatus")["searching"].toBool(true), "Cancel did not clear searching");
    for (const auto &message : messages[&b])
        check(message["type"] != "MatchFound", "Duplicate queue entry created a self-match");
    send(a,"MatchmakingStart",{{"mode","Go"}}); receive(a,"MatchmakingStatus");
    send(b,"MatchmakingStart",{{"mode","Go"}}); receive(b,"MatchmakingStatus");
    const auto found = receive(a,"MatchFound"); receive(b,"MatchFound");
    const auto id = found["matchId"];
    check(found["playerIndex"].toInt() == 0, "Queue order incorrect");
    const auto blackSnapshot = receive(a,"GameSnapshot");
    const auto whiteSnapshot = receive(b,"GameSnapshot");
    check(blackSnapshot["self"].toObject()["playerIndex"].toInt(-1) == 0
          && whiteSnapshot["self"].toObject()["playerIndex"].toInt(-1) == 1,
          "Snapshot perspective incorrect");
    check(blackSnapshot["stage"] == "Action" && blackSnapshot["boardSize"].toInt() == 19
          && blackSnapshot["board"].toArray().size() == 361, "Snapshot board metadata incorrect");
    const auto players = blackSnapshot["players"].toArray();
    check(players.size() == 2 && players[0].toObject()["nickname"] == "Black"
          && players[1].toObject()["nickname"] == "White"
          && !players[0].toObject()["playerId"].toString().isEmpty(), "Snapshot identities incorrect");
    const auto deadline = blackSnapshot["actionDeadlineEpochMs"].toVariant().toLongLong();
    const auto timestamp = blackSnapshot["serverTimeEpochMs"].toVariant().toLongLong();
    check(deadline - timestamp == blackSnapshot["remainingMs"].toArray()[0].toVariant().toLongLong(),
          "Snapshot deadline incorrect");
    check(players[1].toObject()["remainingTimeMs"].toVariant().toLongLong() == 600000,
          "Inactive clock decreased");
    auto command = [&](QWebSocket &socket, QString type, QJsonObject action = {}) {
        send(socket,"GameCommand",{{"matchId",id},{"commandType",type},{"command",action}});
    };
    command(outsider,"Concede");
    check(receive(outsider,"Error")["code"] == "MatchNotFound", "Outsider affected match");
    command(b,"Play",{{"row",0},{"col",0}});
    check(receive(b,"Error")["code"] == "NotYourTurn", "Out-of-turn play allowed");
    command(a,"UseSkill");
    check(receive(a,"Error")["code"] == "InvalidCommand", "Card command allowed");
    command(a,"Play",{{"row",0.5},{"col",0}});
    check(receive(a,"Error")["code"] == "InvalidPosition", "Fractional position allowed");
    command(a,"Play",{{"row",0},{"col",0}});
    receive(a,"GameEvent"); receive(b,"GameEvent");
    auto snapshot=receive(a,"GameSnapshot"); receive(b,"GameSnapshot");
    check(snapshot["board"].toArray()[0].toInt()==0 && snapshot["currentPlayerIndex"].toInt()==1,
          "Play snapshot incorrect");
    command(b,"Play",{{"row",0},{"col",0}});
    check(receive(b,"Error")["code"] == "InvalidMove", "Occupied move allowed");
    command(b,"EndRound"); receive(a,"GameEvent"); receive(b,"GameEvent");
    receive(a,"GameSnapshot"); receive(b,"GameSnapshot");
    command(a,"EndRound");
    receive(a,"GameEvent"); receive(b,"GameEvent"); // Passed
    auto ended=receive(a,"GameEvent"); receive(b,"GameEvent");
    check(ended["events"].toArray()[0].toObject()["eventType"]=="GameEnded", "Two passes did not finish");
    const auto finalSnapshot = receive(a,"GameSnapshot"); receive(b,"GameSnapshot");
    check(!finalSnapshot["started"].toBool() && finalSnapshot["stage"] == "GameEnd"
          && finalSnapshot["actionDeadlineEpochMs"].toVariant().toLongLong() == 0
          && finalSnapshot["winnerIndex"].toInt(-1) == 0
          && !finalSnapshot["endReason"].toString().isEmpty(), "Final snapshot incorrect");
    command(a,"Play",{{"row",1},{"col",1}});
    check(receive(a,"Error")["code"]=="MatchNotFound", "Finished match accepted command");
    send(a,"MatchmakingStart",{{"mode","Go"}}); receive(a,"MatchmakingStatus");
    send(b,"MatchmakingStart",{{"mode","Go"}}); receive(b,"MatchmakingStatus");
    auto next = receive(a,"MatchFound"); receive(b,"MatchFound");
    receive(a,"GameSnapshot"); receive(b,"GameSnapshot");
    send(b,"GameCommand",{{"matchId",next["matchId"]},{"commandType","Concede"}});
    ended=receive(a,"GameEvent");
    check(ended["events"].toArray()[0].toObject()["winnerIndex"].toInt()==0,"Out-of-turn concede failed");
    std::puts("Go rules and commands passed");
}
