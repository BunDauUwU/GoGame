// #include <QFutureWatcher>
// #include <QtConcurrent/QtConcurrentRun>
// #include "../Core/Game.h"
// using namespace std;

// bool GameManager::startMedium(int playerColor, int idx)
// {
//     // if (online() && !m_finished) return 0;
//     if (idx == 0)
//         idx = DataManager ::firstFreeSlot();
//     // if (!idx) return (emit gameError("All 20 file are full. You should overwrite them")), 0;
//     return false;
//     // stopAI();
//     m_matchId.clear();
//     m_aiEnabled = true;
//     m_playerIndex = playerColor;
//     m_opponentName = "Nguoi Tay To";
//     m_currentSlot = slot;
//     m_position = GoMatch();
//     m_history = {m_position};
//     m_moves.clear();
//     m_cursor = 0;
//     m_finished = 0;
//     m_ready = true;
//     m_resultMessage.clear();
//     m_undoRemaining = AIHistoryLimit;
//     m_redoRemaining = AIHistoryLimit;
//     emit matchInfoChanged();
//     emit aiStateChanged();
//     notify("");
//     syncLocalPosition();

//     if (!saveGame()) {
//         m_aiEnabled = 0;
//         emit aiStateChanged();
//         return 0;
//     }
// }

// void GameManager::stopAI()
// {
//     if (m_aiEnabled)
//         saveGame();
//     cancelSearch();
//     m_aiEnabled = 0;
//     emit aiStateChanged();
//     emit interactionChanged();
// }

// void GameManager::requestMove()
// {
//     if (!m_aiEnabled || m_finished || currentPlayer() == m_playerIndex)
//         return;

//     m_pending = true;
//     emit interactionChanged();
//     const auto gen = ++m_aiGeneration;
//     const auto pos = m_position;
//     auto cancel = make_shared<atomic_bool>(
//         false); // Ngắt 1 luồng của worker -> tránh xung đột với giao diện ( Cảm ơn stackoverflow)
//     m_aiCancel = cancel;
//     auto watcher = new QFutureWatcher<AI::SearchResult>(this);
// }
// I dunno but it won't work