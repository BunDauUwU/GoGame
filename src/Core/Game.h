#pragma once
#include <QObject>
#include <QThreadPool>
#include <QVariantList>
#include <QVariantMap>
#include "../AI/EasyAI.h"
#include "../AI/HardAI.h"
#include "../AI/MediumAI.h"
#include "../Managers/DataManager.h"
#include "../Network/NetworkClient.h"
#include "Enums.h"

class GameManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList boardState READ boardState NOTIFY boardStateChanged)
    Q_PROPERTY(int currentPlayer READ currentPlayer NOTIFY currentPlayerChanged)
    Q_PROPERTY(QVariantList lastMoveCoords READ lastMoveCoords NOTIFY lastMoveChanged)
    Q_PROPERTY(int playerIndex READ playerIndex NOTIFY matchInfoChanged)
    Q_PROPERTY(QVariantMap snapshot READ snapshot NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantMap scoring READ scoring NOTIFY scoringChanged)
    Q_PROPERTY(QVariantMap serverScoring READ serverScoring NOTIFY scoringChanged)
    Q_PROPERTY(QVariantMap scoringComparison READ scoringComparison NOTIFY scoringChanged)
    Q_PROPERTY(bool online READ online NOTIFY matchInfoChanged)
    Q_PROPERTY(bool aiEnabled READ aiEnabled NOTIFY aiStateChanged)
    Q_PROPERTY(bool sandbox READ sandbox NOTIFY aiStateChanged)
    Q_PROPERTY(QString aiDifficulty READ aiDifficulty NOTIFY aiStateChanged)
    Q_PROPERTY(bool hasSandboxSave READ hasSandboxSave NOTIFY savesChanged)
    Q_PROPERTY(bool canAct READ canAct NOTIFY interactionChanged)
    Q_PROPERTY(bool finished READ finished NOTIFY interactionChanged)
    Q_PROPERTY(bool pending READ pending NOTIFY interactionChanged)
    Q_PROPERTY(QString resultMessage READ resultMessage NOTIFY interactionChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(int undoRemaining READ undoRemaining NOTIFY historyChanged)
    Q_PROPERTY(int redoRemaining READ redoRemaining NOTIFY historyChanged)
    Q_PROPERTY(QVariantList savedSlots READ savedSlots NOTIFY savesChanged)
    Q_PROPERTY(QVariantList savedGames READ savedGames NOTIFY savesChanged)
    Q_PROPERTY(QVariantList sandboxSavedGames READ sandboxSavedGames NOTIFY savesChanged)
    Q_PROPERTY(int currentSaveSlot READ currentSaveSlot NOTIFY savesChanged)
    Q_PROPERTY(QString notification READ notification NOTIFY notificationChanged)
public:
    static constexpr int AIHistoryLimit = 1000; // Undo Redo Limit
    static constexpr int AISaveSlots = DataManager::AISaveSlots;
    explicit GameManager(QObject *parent = nullptr);
    ~GameManager() override;
    QVariantList lastMoveCoords() const { return m_lastMoveCoords; }
    Q_INVOKABLE bool startEasyGame(int humanColor = 0, int slot = 0);
    Q_INVOKABLE bool startMediumGame(int humanColor = 0, int slot = 0);
    Q_INVOKABLE bool startHardGame(int humanColor = 0, int slot = 0);
    Q_INVOKABLE bool startSandbox();
    Q_INVOKABLE void stopAI();
    Q_INVOKABLE bool exitLocalGame();
    Q_INVOKABLE void resetGame();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE bool saveGame(int slot = 0);
    Q_INVOKABLE bool loadSandbox(int slot = 1);
    Q_INVOKABLE bool loadAIGame(int slot);
    Q_INVOKABLE void refreshSaves() { emit savesChanged(); }
    bool aiEnabled() const { return m_aiEnabled; }
    bool sandbox() const { return m_sandbox; }
    QString aiDifficulty() const { return m_aiDifficulty; }
    bool hasSandboxSave() const { return DataManager::hasSandboxSave(); }
    // Model backends are configured before dispatch; the worker gets a safe copy.
    void configureHard(HardSearchOptions options, std::shared_ptr<HardAI::Evaluator> evaluator)
    {
        cancelSearch();
        m_hardAI = HardAI(options);
        m_hardAI.setEvaluator(std::move(evaluator));
        requestMove();
    }
    void setHardEvaluator(std::shared_ptr<HardAI::Evaluator> evaluator)
    {
        cancelSearch();
        m_hardAI.setEvaluator(std::move(evaluator));
        requestMove();
    }
    bool canUndo() const;
    bool canRedo() const;
    int undoRemaining() const { return m_undoRemaining; }
    int redoRemaining() const { return m_redoRemaining; }
    QVariantList savedSlots() const;
    QVariantList savedGames() const { return DataManager::savedGames(true); }
    QVariantList sandboxSavedGames() const { return DataManager::savedGames(false); }
    int currentSaveSlot() const { return m_currentSlot; }
    QString notification() const { return m_notification; }
    void setNetworkClient(NetworkClient *client);
    Q_INVOKABLE void setMatchInfo(const QString &id, int player, const QString &opponent);
    QVariantMap snapshot() const { return m_snapshot; }
    QVariantMap scoring() const { return m_scoring; }
    QVariantMap serverScoring() const { return m_serverScoring; }
    QVariantMap scoringComparison() const { return m_scoringComparison; }
    bool online() const { return !m_matchId.isEmpty(); }
    bool canAct() const;
    bool finished() const { return m_finished; }
    bool pending() const { return m_pending; }
    QString resultMessage() const { return m_resultMessage; }
    Q_INVOKABLE void concede();
    QString matchId() const { return m_matchId; }
    int playerIndex() const { return m_playerIndex; }
    int currentPlayer() const { return m_position.currentPlayer; }
    QString opponentName() const { return m_opponentName; }
    QVariantList boardState() const { return m_position.state().toVariantList(); }
    Q_INVOKABLE bool is_valid_move(int row, int col);
    Q_INVOKABLE void update_pieces(int row, int col);
    Q_INVOKABLE void pass_turn();
    void sendCommand(const QJsonObject &command);
public slots:
    void handleServerMessage(const QJsonObject &message);
signals:
    void lastMoveChanged();
    void boardStateChanged();
    void currentPlayerChanged();
    void matchInfoChanged();
    void snapshotChanged();
    void scoringChanged();
    void scoringMismatchDetected(const QVariantMap &comparison);
    void interactionChanged();
    void aiStateChanged();
    void historyChanged();
    void savesChanged();
    void notificationChanged();
    void gameError(const QString &message);

private:
    QVariantList m_lastMoveCoords;
    NetworkClient *m_networkClient = nullptr;
    QString m_matchId, m_opponentName, m_resultMessage, m_notification;
    int m_playerIndex = -1;
    bool m_ready = false, m_finished = false, m_pending = false, m_aiEnabled = false,
         m_sandbox = false;
    GoMatch m_position;
    QVariantMap m_snapshot;
    QVariantMap m_scoring, m_serverScoring, m_scoringComparison;
    QThreadPool m_aiPool;
    EasyAI m_easyAI;
    MediumAI m_mediumAI;
    HardAI m_hardAI;
    QString m_aiDifficulty = QStringLiteral("Medium");
    std::shared_ptr<std::atomic_bool> m_aiCancel;
    quint64 m_aiGeneration = 0;
    std::vector<GoMatch> m_history;
    std::vector<QJsonObject> m_moves;
    int m_cursor = 0, m_undoRemaining = AIHistoryLimit, m_redoRemaining = AIHistoryLimit,
        m_currentSlot = 0;
    void setLastMove(int row, int col);
    void clearLastMove();
    void syncLastMoveFromHistory();
    void cancelSearch();
    void syncLocalPosition();
    void updateScoring(const QVariantMap &serverScoring = {});
    void notifyScore();
    void recordMove(const QJsonObject &move);
    void requestMove();
    bool startAIGame(const QString &difficulty, int playerColor, int slot);
    void finishLocalGame(int winner, const QString &reason);
    void submit(GameCommandType type, const QJsonObject &command = {});
    void notify(const QString &text);
    void notifyMove();
    int undoTarget() const;
    int redoTarget() const;
    bool loadGame(bool ai, int slot);
};
