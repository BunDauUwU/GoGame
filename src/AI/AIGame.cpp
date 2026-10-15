#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include "../Core/Game.h"

bool GameManager::startMediumGame(int playerColor, int slot)
{
    if (online() && !m_finished) {
        emit gameError(tr("Finish the online game before starting an AI game"));
        return false;
    }
    if ((playerColor != 0 && playerColor != 1) || slot < 0 || slot > AISaveSlots) {
        emit gameError(tr("Invalid player color or save slot"));
        return false;
    }
    if (slot == 0)
        slot = DataManager::firstFreeSlot();
    if (!slot) {
        emit gameError(tr("All 20 save slots are full; select a slot to overwrite"));
        return false;
    }
    if (m_aiEnabled && !saveGame())
        return false;
    cancelSearch();
    m_matchId.clear();
    m_aiEnabled = true;
    m_sandbox = false;
    m_playerIndex = playerColor;
    m_opponentName = QStringLiteral("Medium AI");
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

void GameManager::stopAI()
{
    if (m_aiEnabled)
        saveGame();
    cancelSearch();
    m_aiEnabled = false;
    emit aiStateChanged();
    emit interactionChanged();
}

void GameManager::requestMove()
{
    if (!m_aiEnabled || m_finished || m_pending || currentPlayer() == m_playerIndex)
        return;
    m_pending = true;
    emit interactionChanged();
    const auto generation = ++m_aiGeneration;
    const auto position = m_position;
    const auto engine = m_ai;
    auto cancel = std::make_shared<std::atomic_bool>(false);
    m_aiCancel = cancel;
    auto watcher = new QFutureWatcher<SearchResult>(this);
    connect(watcher, &QFutureWatcher<SearchResult>::finished, this,
            [this, watcher, generation] {
        const auto result = watcher->result();
        watcher->deleteLater();
        // Undo, load, exit or a new match may have replaced the searched position.
        if (generation != m_aiGeneration || !m_aiEnabled || m_finished)
            return;
        m_pending = false;
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
        if (m_position.consecutivePasses >= 2)
            finishLocalGame(m_position.score() > 0 ? 0 : 1, tr("Two consecutive passes"));
    });
    // Capture independent data, never GameManager, in the background worker.
    watcher->setFuture(QtConcurrent::run(&m_aiPool, [engine, position, cancel] {
        return engine.chooseMove(position, cancel);
    }));
}
