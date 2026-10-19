#pragma once
#include <QVariantList>
#include "../Rules/GoMatch.h"
#include <vector>

struct LocalGameData
{
    bool ai = false;
    QString difficulty = QStringLiteral("Medium");
    int humanColor = 0;
    std::vector<QJsonObject> moves;
    std::vector<GoMatch> history;
    GoMatch position;
    int cursor = 0;
    int undoRemaining = 10;
    int redoRemaining = 10;
    bool finished = false;
    int winnerIndex = -1;
    QString endReason;
};

class DataManager
{
public:
    static constexpr int AISaveSlots = 20;
    static constexpr int MaxMoves = 5000;
    static constexpr qint64 MaxFileBytes = 4 * 1024 * 1024;
    static QString savePath(bool ai, int slot = 0);
    static QVariantList savedSlots();
    static bool hasSandboxSave();
    static int firstFreeSlot();
    static bool save(const LocalGameData &game, int slot, QString &error);
    static bool load(bool ai, int slot, int historyLimit, LocalGameData &game, QString &error);
};
