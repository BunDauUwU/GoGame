#include "Server.h"
#include "../../src/Core/Game.h"
#include "../../src/Network/Protocol.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QThread>
#include <cstdio>
#include <cstdlib>
void check(bool ok,const char *text) { if (!ok) { std::fprintf(stderr,"%s\n",text); std::exit(1); } }
template<class P> void waitFor(P predicate) {
 QElapsedTimer clock; clock.start();
 while(!predicate() && clock.elapsed()<3000) { QCoreApplication::processEvents(); QThread::msleep(1); }
 check(predicate(),"Two-player sync timeout");
}
int main(int argc,char **argv) {
 QCoreApplication app(argc,argv); QTemporaryDir dir;
 Server server(dir.filePath("state.json")); QString error;
 check(server.start(24511,error),"Cannot start test server");
 NetworkClient a,b; GameManager black,white;
 black.setNetworkClient(&a); white.setNetworkClient(&b);
 QObject::connect(&a,&NetworkClient::messageReceived,&black,&GameManager::handleServerMessage);
 QObject::connect(&b,&NetworkClient::messageReceived,&white,&GameManager::handleServerMessage);
 a.connectToServer("ws://127.0.0.1:24511"); b.connectToServer("ws://127.0.0.1:24511");
 waitFor([&]{return a.isConnected()&&b.isConnected();});
 a.sendMessage(Protocol::makeAuthRequest("Black")); b.sendMessage(Protocol::makeAuthRequest("White"));
 auto match=[&] {
  a.sendMessage(Protocol::makeStartMatchmaking("Go"));
  b.sendMessage(Protocol::makeStartMatchmaking("Go"));
  waitFor([&]{return black.canAct() && black.online() && white.snapshot().value("started").toBool();});
  check(black.matchId()==white.matchId() && black.playerIndex()==0 && white.playerIndex()==1,"Match identity mismatch");
 };
 match();
 check(!white.canAct() && !white.is_valid_move(0,0),"White can play outside turn");
 black.update_pieces(0,0);
 check(black.pending() && black.boardState()[0].toInt()==-1,"Client changed state before server snapshot");
 black.update_pieces(4,4); // Pending duplicate must not send a second move.
 waitFor([&]{return white.canAct() && !black.pending();});
 check(black.boardState()==white.boardState() && black.boardState()[0].toInt()==0
       && black.boardState()[80].toInt()==-1,"Boards differ after black move");
 white.update_pieces(0,1); waitFor([&]{return black.canAct();});
 black.update_pieces(10,10); waitFor([&]{return white.canAct();});
 white.update_pieces(1,0); waitFor([&]{return black.canAct();});
 check(black.boardState()==white.boardState() && black.boardState()[0].toInt()==-1,"Capture not synced");
 check(!black.is_valid_move(0,0),"Online suicide preview allowed");
 // Force a rejected request through transport, then confirm turn can continue.
 a.sendMessage(Protocol::makePlayCommand(black.matchId(),0,1));
 QCoreApplication::processEvents();
 black.pass_turn(); waitFor([&]{return white.canAct();});
 white.pass_turn(); waitFor([&]{return black.finished()&&white.finished();});
 check(!black.canAct()&&!white.canAct()&&black.boardState()==white.boardState(),"Ended match remains interactive");
 auto oldId=black.matchId(); match();
 check(black.matchId()!=oldId && black.boardState()[0].toInt()==-1,"Rematch retained board");
 white.concede(); waitFor([&]{return black.finished()&&white.finished();});
 check(black.snapshot().value("winnerIndex").toInt()==0,"Concession result not synced");
 match();
 b.disconnectFromServer(); waitFor([&]{return black.finished()&&white.finished();});
 check(!black.canAct()&&!white.canAct(),"Disconnected game accepts moves");
 // Short time control exercises server boundary timers without waiting ten minutes.
 ByoYomiClock shortClock; shortClock.mainRemainingMs=100;
 shortClock.periodDurationMs=80; shortClock.periodRemainingMs=80;
 Server fastServer(dir.filePath("fast.json"),nullptr,shortClock);
 check(fastServer.start(24512,error),"Fast clock server failed");
 NetworkClient c,d; GameManager timedBlack,timedWhite;
 timedBlack.setNetworkClient(&c); timedWhite.setNetworkClient(&d);
 QObject::connect(&c,&NetworkClient::messageReceived,&timedBlack,&GameManager::handleServerMessage);
 QObject::connect(&d,&NetworkClient::messageReceived,&timedWhite,&GameManager::handleServerMessage);
 c.connectToServer("ws://127.0.0.1:24512"); d.connectToServer("ws://127.0.0.1:24512");
 waitFor([&]{return c.isConnected()&&d.isConnected();});
 c.sendMessage(Protocol::makeAuthRequest("TimedBlack")); d.sendMessage(Protocol::makeAuthRequest("TimedWhite"));
 c.sendMessage(Protocol::makeStartMatchmaking("Go")); d.sendMessage(Protocol::makeStartMatchmaking("Go"));
 waitFor([&]{return timedBlack.snapshot().value("clocks").toList().size()==2;});
 waitFor([&]{return timedBlack.snapshot().value("clocks").toList()[0].toMap().value("phase")=="ByoYomi";});
 check(!timedBlack.finished(),"Main time expiry caused immediate defeat");
 waitFor([&]{return timedBlack.snapshot().value("clocks").toList()[0].toMap().value("periodsRemaining").toInt()==2;});
 check(!timedBlack.finished(),"First byo-yomi expiry caused defeat");
 waitFor([&]{return timedBlack.finished()&&timedWhite.finished();});
 check(timedBlack.snapshot().value("winnerIndex").toInt()==1
       && timedBlack.snapshot().value("clocks").toList()[0].toMap().value("periodsRemaining").toInt()==0,
       "Final byo-yomi did not forfeit to opponent");
 std::puts("Two real GameManager clients and byo-yomi timers synced successfully");
}
