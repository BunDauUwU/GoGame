#include "Game.h"
#include "../Network/Protocol.h"
#include <cmath>

GameManager::GameManager(QObject *parent) : QObject(parent) {
  m_aiPool.setMaxThreadCount(1);
  m_history.push_back(m_position);
  connect(this, &GameManager::gameError, this, [this](const QString &message) {
    if (message == "Occupied")
      notify(tr("Invalid move: intersection is occupied"));
    else if (message == "Suicide")
      notify(tr("Invalid move: suicide is not allowed"));
    else if (message == "OutsideBoard")
      notify(tr("Invalid move: outside the board"));
    else if (message == "Ko")
      notify(
          tr("Ko: immediate recapture is forbidden; play elsewhere or pass"));
    else
      notify(message);
  });
  updateScoring();
}

GameManager::~GameManager() {
  if (m_aiEnabled)
    saveGame();
  cancelSearch();
  m_aiPool.waitForDone();
}
void GameManager::setLastMove(int row, int col) {
  QVariantList next{row, col};
  if (next == m_lastMoveCoords)
    return;
  m_lastMoveCoords = next;
  emit lastMoveChanged();
}

void GameManager::clearLastMove() {
  if (m_lastMoveCoords.isEmpty())
    return;
  m_lastMoveCoords.clear();
  emit lastMoveChanged();
}

// for undo/redo/load: last applied move is m_moves[m_cursor - 1]
void GameManager::syncLastMoveFromHistory() {
  if (m_cursor <= 0 || m_cursor > (int)m_moves.size()) {
    clearLastMove();
    return;
  }
  const QJsonObject &mv = m_moves[m_cursor - 1];
  // adjust keys to whatever recordMove() stores
  if (mv.contains("row") && mv.contains("col"))
    setLastMove(mv["row"].toInt(), mv["col"].toInt());
  else
    clearLastMove(); // pass
}
void GameManager::cancelSearch() {
  if (m_aiCancel)
    m_aiCancel->store(true);
  ++m_aiGeneration;
  m_pending = false;
}
bool GameManager::canAct() const {
  if (m_finished || m_pending)
    return false;
  if (online())
    return m_ready && m_networkClient && m_networkClient->isConnected() &&
           currentPlayer() == m_playerIndex;
  return !m_aiEnabled || currentPlayer() == m_playerIndex;
}
void GameManager::setNetworkClient(NetworkClient *client) {
  if (m_networkClient)
    disconnect(m_networkClient, nullptr, this, nullptr);
  m_networkClient = client;
  if (client)
    connect(client, &NetworkClient::disconnected, this, [this] {
      if (!online() || m_finished)
        return;
      m_pending = false;
      m_finished = true;
      m_resultMessage =
          QStringLiteral("Disconnected from server; match cannot continue");
      emit interactionChanged();
      emit gameError(m_resultMessage);
    });
}
void GameManager::notify(const QString &text) {
  m_notification = text;
  emit notificationChanged();
}
void GameManager::notifyMove() {
  QStringList parts;
  if (m_position.capturedLastMove > 0)
    parts << QStringLiteral("Captured %1 stone(s)")
                 .arg(m_position.capturedLastMove);
  if (m_position.koPoint >= 0)
    parts << QStringLiteral("Ko at (%1, %2): immediate recapture is forbidden")
                 .arg(m_position.koPoint / 19 + 1)
                 .arg(m_position.koPoint % 19 + 1);
  if (m_position.koThreatLastMove)
    parts << "Ko threat: opposing group is in atari";
  if (!parts.isEmpty())
    notify(parts.join(" · "));
}
bool GameManager::is_valid_move(int row, int col) {
  if (!canAct()) {
    emit gameError(tr("Wait for your turn"));
    return false;
  }
  auto candidate = m_position;
  QString error;
  bool valid = candidate.play(row, col, error);
  if (!valid)
    emit gameError(error);
  return valid;
}
void GameManager::update_pieces(int row, int col) {
  if (!canAct()) {
    emit gameError(QStringLiteral("Wait for your turn"));
    return;
  }
  auto candidate = m_position;
  QString error;
  if (!candidate.play(row, col, error)) {
    emit gameError(error);
    return;
  }
  if (online()) {
    submit(GameCommandType::Play, {{"row", row}, {"col", col}});
    return;
  }
  m_position = std::move(candidate);
  recordMove({{"row", row}, {"col", col}});
  notifyMove();
  requestMove();
}
void GameManager::pass_turn() {
  if (!canAct())
    return;
  if (online()) {
    submit(GameCommandType::EndRound);
    return;
  }
  m_position.pass();
  recordMove({{"pass", true}});
  if (m_position.consecutivePasses >= 2)
    finishLocalGame(m_position.score() > 0 ? 0 : 1,
                    QStringLiteral("Two consecutive passes"));
  requestMove();
}
void GameManager::setMatchInfo(const QString &id, int player,
                               const QString &opponent) {
  if (id.isEmpty() || (player != 0 && player != 1))
    return;
  if (id == m_matchId && player == m_playerIndex && opponent == m_opponentName)
    return;
  stopAI();
  m_sandbox = false;
  m_position = GoMatch();
  m_snapshot.clear();
  m_matchId = id;
  m_playerIndex = player;
  m_opponentName = opponent;
  m_ready = false;
  m_finished = false;
  m_pending = false;
  m_resultMessage.clear();
  notify("");
  updateScoring();
  emit matchInfoChanged();
  emit aiStateChanged();
  emit boardStateChanged();
  emit currentPlayerChanged();
  emit snapshotChanged();
  emit interactionChanged();
  emit historyChanged();
}
void GameManager::handleServerMessage(const QJsonObject &message) {
  const auto type = Protocol::readMessageType(message);
  const auto payload = Protocol::readPayload(message);
  if (type == "MatchFound") {
    setMatchInfo(payload["matchId"].toString(),
                 payload["playerIndex"].toInt(-1),
                 payload["opponentName"].toString());
    return;
  }
  if (type == "Error") {
    if (online()) {
      m_pending = false;
      emit interactionChanged();
      emit gameError(payload["message"].toString());
    }
    return;
  }
  if (!online() || payload["matchId"].toString() != m_matchId)
    return;
  if (type == "GameSnapshot") {
    int newMoves = payload["moveCount"].toInt(-1),
        oldMoves = m_snapshot.value("moveCount", -1).toInt();
    if (newMoves < oldMoves)
      return;
    GoMatch position;
    QString error;
    auto json = payload["position"].toObject();
    if (json.isEmpty()) {
      json = m_position.toJson();
      json["board"] = payload["board"];
      json["previousBoard"] = m_position.state();
      json["hasPrevious"] = m_ready && newMoves == oldMoves + 1;
      json["currentPlayerIndex"] = payload["currentPlayerIndex"];
      json["moveCount"] = newMoves;
      json["consecutivePasses"] = payload["consecutivePasses"].toInt();
    }
    if (!GoMatch::fromJson(json, position, error)) {
      emit gameError(error);
      return;
    }
    bool boardChanged = position.cells() != m_position.cells(),
         turnChanged = position.currentPlayer != currentPlayer();
    bool oldAct = canAct(), oldFinished = m_finished;
    m_position = std::move(position);
    m_ready = true;
    m_finished = !payload["started"].toBool();
    if (newMoves != oldMoves || m_finished)
      m_pending = false;
    if (m_finished)
      m_resultMessage = payload["endReason"].toString();
    m_snapshot = payload.toVariantMap();
    updateScoring(payload["scoring"].toObject().toVariantMap());
    if (boardChanged)
      emit boardStateChanged();
    if (turnChanged)
      emit currentPlayerChanged();
    emit snapshotChanged();
    if (m_finished && m_position.consecutivePasses >= 2 &&
        (newMoves != oldMoves || !oldFinished))
      notifyScore();
    if (oldAct != canAct() || oldFinished != m_finished)
      emit interactionChanged();
  } else if (type == "GameEvent") {
    for (const auto &value : payload["events"].toArray()) {
      const auto event = value.toObject();
      auto kind = event["eventType"].toString();
      if (kind == "StonePlayed") {
        QStringList parts;
        int captured = event["captured"].toInt();
        int ko = event["koPoint"].toInt(-1);
        if (captured > 0)
          parts << QString("Captured %1 stone(s)").arg(captured);
        if (ko >= 0)
          parts << QString("Ko at (%1, %2)").arg(ko / 19 + 1).arg(ko % 19 + 1);
        if (event["koThreat"].toBool())
          parts << "Ko threat: opposing group is in atari";
        if (!parts.isEmpty())
          notify(parts.join(" · "));
      } else if (kind == "GameEnded") {
        m_finished = true;
        m_pending = false;
        m_resultMessage = event["reason"].toString();
        emit interactionChanged();
      }
    }
  }
}
void GameManager::sendCommand(const QJsonObject &command) {
  if (!m_networkClient || !m_networkClient->isConnected()) {
    m_pending = false;
    emit interactionChanged();
    emit gameError("Not connected to server");
    return;
  }
  m_networkClient->sendMessage(command);
}
void GameManager::submit(GameCommandType type, const QJsonObject &command) {
  if (!online() || !m_ready || m_finished || m_pending || !m_networkClient ||
      !m_networkClient->isConnected() ||
      (type != GameCommandType::Concede && !canAct())) {
    emit gameError("Cannot send command now");
    return;
  }
  m_pending = true;
  emit interactionChanged();
  sendCommand(Protocol::makeGameCommand(m_matchId, type, command));
}
void GameManager::concede() {
  if (m_aiEnabled && !m_finished)
    finishLocalGame(1 - m_playerIndex, "Conceded");
  else
    submit(GameCommandType::Concede);
}

bool GameManager::startSandbox() {
  if (online() && !m_finished)
    return false;
  stopAI();
  m_matchId.clear();
  m_sandbox = true;
  m_playerIndex = 0;
  m_currentSlot = 0;
  emit matchInfoChanged();
  emit aiStateChanged();
  resetGame();
  return true;
}

bool GameManager::exitLocalGame() {
  if (m_aiEnabled && !saveGame())
    return false;
  cancelSearch();
  m_aiEnabled = false;
  m_sandbox = false;
  emit aiStateChanged();
  emit interactionChanged();
  return true;
}
void GameManager::resetGame() {
  if (!m_sandbox)
    return;
  cancelSearch();
  m_position = GoMatch();
  m_history = {m_position};
  m_moves.clear();
  m_cursor = 0;
  m_finished = false;
  m_resultMessage.clear();
  syncLocalPosition();
  notify("Sandbox reset");
}
void GameManager::updateScoring(const QVariantMap &serverScoring) {
  const auto phase = m_finished ? GoMatch::ScoringPhase::Final
                                : GoMatch::ScoringPhase::Current;
  const auto client = m_position.scoreDetails(phase).toVariantMap();
  const auto server = online() && m_ready ? serverScoring : QVariantMap{};
  QVariantMap differences;
  const auto numeric = [](const QVariant &value) {
    const int type = value.metaType().id();
    return type == QMetaType::Int || type == QMetaType::UInt ||
           type == QMetaType::LongLong || type == QMetaType::ULongLong ||
           type == QMetaType::Float || type == QMetaType::Double;
  };
  if (!server.isEmpty()) {
    for (auto it = client.constBegin(); it != client.constEnd(); ++it) {
      const auto actual = server.value(it.key());
      const bool isNumeric = numeric(it.value());
      const bool matches =
          server.contains(it.key()) &&
          (isNumeric
               ? numeric(actual) && std::isfinite(actual.toDouble()) &&
                     std::abs(actual.toDouble() - it.value().toDouble()) <= 1e-9
               : actual.metaType() == it.value().metaType() &&
                     actual == it.value());
      if (matches)
        continue;
      QVariantMap detail{{"client", it.value()}, {"server", actual}};
      if (!server.contains(it.key()))
        detail["missing"] = true;
      else if (isNumeric && numeric(actual))
        detail["delta"] = actual.toDouble() - it.value().toDouble();
      differences[it.key()] = detail;
    }
    if (m_snapshot.value("moveCount", -1).toInt() != m_position.moveCount)
      differences["moveCount"] =
          QVariantMap{{"client", m_position.moveCount},
                      {"server", m_snapshot.value("moveCount")}};
  }
  const QString status = !online()               ? "local"
                         : server.isEmpty()      ? "unavailable"
                         : differences.isEmpty() ? "matched"
                                                 : "mismatch";
  const QVariantMap comparison{{"status", status},
                               {"matches", status == "matched"},
                               {"moveCount", m_position.moveCount},
                               {"differences", differences}};
  const bool changed = client != m_scoring || server != m_serverScoring ||
                       comparison != m_scoringComparison;
  const bool newMismatch =
      status == "mismatch" && comparison != m_scoringComparison;
  m_scoring = client;
  m_serverScoring = server;
  m_scoringComparison = comparison;
  if (changed)
    emit scoringChanged();
  if (newMismatch)
    emit scoringMismatchDetected(m_scoringComparison);
}
void GameManager::notifyScore() {
  notify(
      tr("Japanese score: Black %1\nWhite %2 (includes %3 komi)\nDifference %4")
          .arg(m_scoring.value("blackPoints").toDouble())
          .arg(m_scoring.value("whitePoints").toDouble())
          .arg(m_scoring.value("komi").toDouble())
          .arg(m_scoring.value("difference").toDouble()));
}
void GameManager::syncLocalPosition() {
  m_snapshot = {{"mode", m_aiEnabled ? m_aiDifficulty : QStringLiteral("Sandbox")},
                {"started", !m_finished},
                {"moveCount", m_position.moveCount},
                {"currentPlayerIndex", currentPlayer()},
                {"board", m_position.state().toVariantList()},
                {"komi", 6.5},
                {"consecutivePasses", m_position.consecutivePasses}};
  syncLastMoveFromHistory();
  updateScoring();
  m_snapshot["scoring"] = m_scoring;
  emit boardStateChanged();
  emit currentPlayerChanged();
  emit snapshotChanged();
  emit interactionChanged();
  emit historyChanged();
}
void GameManager::recordMove(const QJsonObject &move) {
  m_history.resize(m_cursor + 1);
  m_moves.resize(m_cursor);
  m_moves.push_back(move);
  m_history.push_back(m_position);
  ++m_cursor;
  syncLocalPosition();
  if (m_aiEnabled && m_position.consecutivePasses < 2)
    saveGame();
}
void GameManager::finishLocalGame(int winner, const QString &reason) {
  cancelSearch();
  m_finished = true;
  m_resultMessage = reason;
  m_snapshot["started"] = false;
  m_snapshot["winnerIndex"] = winner;
  m_snapshot["endReason"] = reason;
  updateScoring();
  m_snapshot["scoring"] = m_scoring;
  if (m_position.consecutivePasses >= 2)
    notifyScore();
  emit snapshotChanged();
  emit interactionChanged();
  emit historyChanged();
  if (m_aiEnabled)
    saveGame();
}

int GameManager::undoTarget() const {
  if (m_cursor <= 0)
    return -1;
  if (!m_aiEnabled)
    return m_cursor - 1;
  for (int i = m_cursor - 1; i >= 0; --i)
    if (m_history[i].currentPlayer == m_playerIndex)
      return i;
  return -1;
}
int GameManager::redoTarget() const {
  if (m_cursor + 1 >= int(m_history.size()))
    return -1;
  if (!m_aiEnabled)
    return m_cursor + 1;
  for (int i = m_cursor + 1; i < int(m_history.size()); ++i)
    if (m_history[i].currentPlayer == m_playerIndex ||
        i == int(m_history.size()) - 1)
      return i;
  return -1;
}
bool GameManager::canUndo() const {
  return !online() && (m_sandbox || m_aiEnabled) && undoTarget() >= 0 &&
         (!m_aiEnabled || m_undoRemaining > 0);
}
bool GameManager::canRedo() const {
  return !online() && (m_sandbox || m_aiEnabled) && redoTarget() >= 0 &&
         (!m_aiEnabled || m_redoRemaining > 0);
}
void GameManager::undo() {
  if (!canUndo())
    return;
  int target = undoTarget();
  cancelSearch();
  m_cursor = target;
  m_position = m_history[target];
  if (m_aiEnabled)
    --m_undoRemaining;
  m_finished = false;
  m_resultMessage.clear();
  syncLocalPosition();
  notify("Undo");
  if (m_aiEnabled)
    saveGame();
}
void GameManager::redo() {
  if (!canRedo())
    return;
  int target = redoTarget();
  cancelSearch();
  m_cursor = target;
  m_position = m_history[target];
  if (m_aiEnabled)
    --m_redoRemaining;
  m_finished = false;
  m_resultMessage.clear();
  syncLocalPosition();
  notify("Redo");
  if (m_position.consecutivePasses >= 2)
    finishLocalGame(m_position.score() > 0 ? 0 : 1, "Two consecutive passes");
  else {
    if (m_aiEnabled)
      saveGame();
    requestMove();
  }
}
QVariantList GameManager::savedSlots() const {
  return DataManager::savedSlots();
}
bool GameManager::saveGame(int slot) {
  if (online() || (!m_aiEnabled && !m_sandbox))
    return false;
  if (m_aiEnabled && slot == 0)
    slot = m_currentSlot;
  LocalGameData data;
  data.ai = m_aiEnabled;
  data.difficulty = m_aiDifficulty;
  data.humanColor = m_playerIndex;
  data.moves = m_moves;
  data.cursor = m_cursor;
  data.position = m_position;
  data.undoRemaining = m_undoRemaining;
  data.redoRemaining = m_redoRemaining;
  data.finished = m_finished;
  data.winnerIndex = m_snapshot.value("winnerIndex", -1).toInt();
  data.endReason = m_resultMessage;
  QString error;
  if (!DataManager::save(data, slot, error)) {
    emit gameError(error);
    return false;
  }
  if (m_aiEnabled)
    m_currentSlot = slot;
  emit savesChanged();
  return true;
}
bool GameManager::loadSandbox() { return loadGame(false, 0); }
bool GameManager::loadAIGame(int slot) { return loadGame(true, slot); }
bool GameManager::loadGame(bool ai, int slot) {
  if (online() && !m_finished)
    return false;
  LocalGameData data;
  QString error;
  if (!DataManager::load(ai, slot, AIHistoryLimit, data, error)) {
    emit gameError(error);
    return false;
  }
  cancelSearch();
  m_matchId.clear();
  m_aiEnabled = ai;
  m_aiDifficulty = data.difficulty;
  m_opponentName = ai ? m_aiDifficulty + QStringLiteral(" AI") : QStringLiteral("White");
  m_hardAI.reset();
  m_sandbox = !ai;
  m_playerIndex = data.humanColor;
  m_currentSlot = ai ? slot : 0;
  m_history = std::move(data.history);
  m_moves = std::move(data.moves);
  m_cursor = data.cursor;
  m_position = std::move(data.position);
  m_undoRemaining = data.undoRemaining;
  m_redoRemaining = data.redoRemaining;
  m_finished = data.finished;
  m_ready = true;
  m_resultMessage = data.endReason;
  emit matchInfoChanged();
  emit aiStateChanged();
  syncLocalPosition();
  m_snapshot["winnerIndex"] = m_finished && m_position.consecutivePasses >= 2
                                  ? (m_position.score() > 0 ? 0 : 1)
                                  : data.winnerIndex;
  m_snapshot["endReason"] = m_resultMessage;
  emit snapshotChanged();
  emit savesChanged();
  notify("Saved game loaded");
  requestMove();
  return true;
}
