#include "../Core/Game.h"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

bool GameManager::startEasyGame(int playerColor, int slot) {
  return startAIGame(QStringLiteral("Easy"), playerColor, slot);
}
// AI-assisted: see AI12 in docs/AI_Declaration.md (offline Medium startup).
bool GameManager::startMediumGame(int playerColor, int slot) {
  return startAIGame(QStringLiteral("Medium"), playerColor, slot);
}
// AI-assisted: see AI13 in docs/AI_Declaration.md (Hard mode integration).
bool GameManager::startHardGame(int playerColor, int slot) {
  return startAIGame(QStringLiteral("Hard"), playerColor, slot);
}
// AI-assisted: see AI12 and AI13 in docs/AI_Declaration.md (AI game startup
// integration).
bool GameManager::startAIGame(const QString &difficulty, int playerColor,
                              int slot) {
  if (online() && !m_finished) {
    emit gameError(tr("Finish the online game before starting an AI game"));
    return false;
  }
  if ((playerColor != 0 && playerColor != 1) || slot < 0 ||
      slot > AISaveSlots) {
    emit gameError(tr("Invalid player color or save slot"));
    return false;
  }
  if (slot == 0)
    slot = DataManager::firstFreeSlot();
  if (!slot) {
    emit gameError(
        tr("All 20 save slots are full; select a slot to overwrite"));
    return false;
  }
  if (m_aiEnabled && !saveGame())
    return false;
  cancelSearch();
  m_matchId.clear();
  m_aiEnabled = true;
  m_sandbox = false;
  m_playerIndex = playerColor;
  m_aiDifficulty = difficulty;
  m_opponentName = difficulty + QStringLiteral(" AI");
  m_hardAI.reset();
  m_currentSlot = slot;
  m_position = GoMatch();
  m_history = {m_position};
  m_moves.clear();
  m_cursor = 0;
  m_finished = false;
  m_ready = true;
  m_resultMessage.clear();
  m_snapshot.clear();
  m_undoRemaining = AIHistoryLimit;
  m_redoRemaining = AIHistoryLimit;
  emit matchInfoChanged();
  emit aiStateChanged();
  notify("");
  syncLocalPosition();
  if (!saveGame()) {
    m_aiEnabled = false;
    emit aiStateChanged();
    emit interactionChanged();
    return false;
  }
  requestMove();
  return true;
}

void GameManager::stopAI() {
  if (m_aiEnabled)
    saveGame();
  cancelSearch();
  m_aiEnabled = false;
  emit aiStateChanged();
  emit interactionChanged();
}

// AI-assisted: see AI12 and AI13 in docs/AI_Declaration.md (worker dispatch and
// cancellation).
void GameManager::requestMove() {
  if (!m_aiEnabled || m_finished || m_pending ||
      currentPlayer() == m_playerIndex)
    return;
  m_pending = true;
  emit interactionChanged();
  const auto generation = ++m_aiGeneration;
  const auto position = m_position;
  const auto easyEngine = m_easyAI;
  const auto mediumEngine = m_mediumAI;
  const auto hardEngine = m_hardAI;
  const bool easy = m_aiDifficulty == QStringLiteral("Easy");
  const bool hard = m_aiDifficulty == QStringLiteral("Hard");
  auto cancel = std::make_shared<std::atomic_bool>(false);
  m_aiCancel = cancel;
  auto watcher = new QFutureWatcher<SearchResult>(this);
  connect(
      watcher, &QFutureWatcher<SearchResult>::finished, this,
      [this, watcher, generation] {
        const auto result = watcher->result();
        watcher->deleteLater();
        if (generation != m_aiGeneration || !m_aiEnabled || m_finished)
          return;
        m_pending = false;
        if (!result.error.isEmpty()) {
          emit interactionChanged();
          emit gameError(tr("AI evaluation failed: %1").arg(result.error));
          return;
        }
        if (result.cancelled) {
          emit interactionChanged();
          return;
        }
        if (result.pass) {
          m_position.pass();
          recordMove({{"pass", true}});
        } else {
          QString error;
          if (!m_position.play(result.row, result.col, error)) {
            emit interactionChanged();
            emit gameError(tr("AI returned an invalid move: %1").arg(error));
            return;
          }
          recordMove({{"row", result.row}, {"col", result.col}});
          notifyMove();
        }
        m_snapshot["aiSearch"] = QVariantMap{
            {"estimatedScore", result.estimatedScore},
            {"difficulty", m_aiDifficulty},
            {"winValue", result.winValue},
            {"passScore", result.passScore},
            {"moveScore", result.moveScore},
            {"gainOverPass", result.gainOverPass},
            {"completedDepth", result.completedDepth},
            {"nodes", result.nodes},
            {"elapsedMs", result.elapsedMs},
            {"stoppedEarly", result.stoppedEarly},
            {"stopReason", result.stopReason},
            {"evaluations",
             QVariant::fromValue<qulonglong>(result.evaluations)},
            {"cacheHits", QVariant::fromValue<qulonglong>(result.cacheHits)},
            {"rootVisits", result.rootVisits},
            {"reusedVisits", result.reusedVisits},
            {"allocatedBytes",
             QVariant::fromValue<qulonglong>(result.allocatedBytes)},
            {"memoryLimited", result.memoryLimited}};
        emit snapshotChanged();
        if (m_position.m_passes >= 2)
            finishLocalGame(m_position.score() > 0 ? 0 : 1, tr("Two consecutive passes"));
      });

  watcher->setFuture(
      QtConcurrent::run(&m_aiPool, [easyEngine, mediumEngine, hardEngine, easy,
                                    hard, position, cancel] {
        return hard   ? hardEngine.chooseMove(position, cancel)
               : easy ? easyEngine.chooseMove(position, cancel)
                      : mediumEngine.chooseMove(position, cancel);
      }));
}
