// src/Core/Game.h
#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <vector>

#include "../Core/Enums.h"
#include "../Network/NetworkClient.h"

class GameManager : public QObject {
  Q_OBJECT
  Q_PROPERTY(int difficultyLevel READ difficultyLevel WRITE setDifficultyLevel
                 NOTIFY difficultyLevelChanged)
  Q_PROPERTY(QVariantList boardState READ boardState NOTIFY boardStateChanged)
  Q_PROPERTY(int currentPlayer READ currentPlayer NOTIFY currentPlayerChanged)
  Q_PROPERTY(int playerIndex READ playerIndex NOTIFY matchInfoChanged)
  Q_PROPERTY(QVariantMap snapshot READ snapshot NOTIFY snapshotChanged)
  Q_PROPERTY(bool online READ online NOTIFY matchInfoChanged)
  Q_PROPERTY(bool canAct READ canAct NOTIFY interactionChanged)
  Q_PROPERTY(bool finished READ finished NOTIFY interactionChanged)
  Q_PROPERTY(bool pending READ pending NOTIFY interactionChanged)
  Q_PROPERTY(QString resultMessage READ resultMessage NOTIFY interactionChanged)

public:
  explicit GameManager(QObject *parent = nullptr);
  int difficultyLevel() const;
  void setDifficultyLevel(int level);
  void setNetworkClient(NetworkClient *networkClient);
  Q_INVOKABLE void setMatchInfo(const QString &matchId, int playerIndex,
                                const QString &opponentName);

  QVariantMap snapshot() const { return m_snapshot; }
  bool online() const { return !m_matchId.isEmpty(); }
  bool canAct() const {
    return !online() ||
           (m_ready && !m_finished && !m_pending && m_networkClient &&
            m_networkClient->isConnected() && m_currentPlayer == m_playerIndex);
  }
  bool finished() const { return m_finished; }
  bool pending() const { return m_pending; }
  QString resultMessage() const { return m_resultMessage; }
  Q_INVOKABLE void concede();
  QString matchId() const;
  int playerIndex() const;
  int currentPlayer() const { return m_currentPlayer; }
  QVariantList boardState() const {
    QVariantList list;
    for (int i = 0; i < 19; ++i) {
      for (int j = 0; j < 19; ++j) {
        list.append(m_board_state[i][j]);
      }
    }
    return list;
  }
  Q_INVOKABLE bool is_valid_move(int x, int y);
  Q_INVOKABLE void update_pieces(int x, int y);
  Q_INVOKABLE void pass_turn();

  void sendCommand(const QJsonObject &command);
signals:
  void difficultyLevelChanged();
  void boardStateChanged();
  void currentPlayerChanged();
  void matchInfoChanged();
  void snapshotChanged();
  void interactionChanged();

  void gameError(const QString &message);
public slots:
  void handleServerMessage(const QJsonObject &message);

private:
  NetworkClient *m_networkClient = nullptr;
  QString m_matchId;
  int m_difficultyLevel = 0;
  int m_playerIndex = -1;
  int m_currentPlayer = 0;
  QString m_opponentName;
  int m_ko_square = -1;
  std::vector<std::vector<int>> m_board_state;
  QVariantMap m_snapshot;
  bool m_ready = false;
  bool m_finished = false;
  bool m_pending = false;
  QString m_resultMessage;
  std::vector<std::vector<int>> m_previousOnlineBoard;
  void submit(GameCommandType type, const QJsonObject &command = {});
};
