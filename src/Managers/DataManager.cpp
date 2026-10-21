#include "DataManager.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <cmath>

QString DataManager::savePath(bool ai, int slot)
{
    QString dir = qEnvironmentVariable("GOGAME_SAVE_DIR");
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/games";
    return QDir(dir).filePath(ai ? QString("ai-%1.json").arg(slot, 2, 10, QChar('0'))
                                 : slot == 0 ? QStringLiteral("sandbox.json")
                                             : QString("sandbox-%1.json").arg(slot, 2, 10, QChar('0')));
}
namespace {
QString existingSavePath(bool ai, int slot)
{
    const auto path = DataManager::savePath(ai, slot);

    if (!ai && slot == 1 && !QFile::exists(path))
        return DataManager::savePath(false, 0);
    return path;
}
}
bool DataManager::hasSandboxSave()
{ return !savedSlots(false).isEmpty(); }
QVariantList DataManager::savedSlots(bool ai)
{
    QVariantList result;
    for (int slot = 1; slot <= (ai ? AISaveSlots : SandboxSaveSlots); ++slot)
        if (QFile::exists(existingSavePath(ai, slot)))
            result.append(slot);
    return result;
}
QVariantList DataManager::savedGames(bool ai)
{
    QVariantList result;
    for (int slot = 1; slot <= (ai ? AISaveSlots : SandboxSaveSlots); ++slot) {
        QFile file(existingSavePath(ai, slot));
        const bool saved = file.exists();
        QJsonObject data;
        if (saved && file.open(QIODevice::ReadOnly) && file.size() <= MaxFileBytes)
            data = QJsonDocument::fromJson(file.readAll()).object();
        const QString mode = data.value("mode").toString(ai ? QStringLiteral("AI") : QStringLiteral("Sandbox"));
        const QString savedAt = data.value("savedAt").toString();
        const auto date = QDateTime::fromString(savedAt, Qt::ISODate);
        const QString time = date.isValid() ? date.toLocalTime().toString("dd/MM/yyyy HH:mm:ss t")
                                           : QStringLiteral("Unknown save time");
        const QString label = saved ? QString("Slot %1 · %2 · %3").arg(slot).arg(mode, time)
                                    : QString("Slot %1 · Empty").arg(slot);
        result.append(QVariantMap{{"slot", slot}, {"saved", saved}, {"mode", mode},
                                  {"savedAt", savedAt}, {"label", label}});
    }

    return result;
}
int DataManager::firstFreeSlot(bool ai)
{
    for (int slot = 1; slot <= (ai ? AISaveSlots : SandboxSaveSlots); ++slot)
        if (!QFile::exists(existingSavePath(ai, slot)))
            return slot;
    return 0;
}
bool DataManager::save(const LocalGameData &game, int slot, QString &error)
{
    error.clear();
    if ((game.ai && (slot < 1 || slot > AISaveSlots ||
         (game.difficulty != "Easy" && game.difficulty != "Medium" && game.difficulty != "Hard"))) ||
         (!game.ai && (slot < 1 || slot > SandboxSaveSlots)) || game.moves.size() > MaxMoves) {
        error = "Invalid slot or save exceeds move limit";
        return false;
    }
    QJsonArray moves;
    for (const auto &move : game.moves)
        moves.append(move);
    QJsonObject json{{"version", 1},
                     {"slot", slot},
                     {"gameMode", game.ai ? QStringLiteral("AI") : QStringLiteral("Sandbox")},
                     {"mode", game.ai ? game.difficulty : QStringLiteral("Sandbox")},
                     {"humanColor", game.humanColor},
                     {"moves", moves},
                     {"cursor", game.cursor},
                     {"position", game.position.toJson()},
                     {"undoRemaining", game.undoRemaining},
                     {"redoRemaining", game.redoRemaining},
                     {"finished", game.finished},
                     {"winnerIndex", game.winnerIndex},
                     {"endReason", game.endReason},
                     {"savedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}};
    auto bytes = QJsonDocument(json).toJson(QJsonDocument::Indented);
    if (bytes.size() > MaxFileBytes) {
        error = "Save exceeds file size limit";
        return false;
    }
    auto path = savePath(game.ai, slot);
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        error = "Cannot create save directory";
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        error = "Save failed: " + file.errorString();
        return false;
    }
    return true;
}
bool DataManager::load(bool ai, int slot, int historyLimit, LocalGameData &game, QString &error)
{
    auto fail = [&] {
        error = "Invalid saved game; current game was preserved";
        return false;
    };
    if (!ai && slot == 0) slot = 1;
    if ((ai && (slot < 1 || slot > AISaveSlots)) || (!ai && (slot < 1 || slot > SandboxSaveSlots)))
        return fail();
    QFile file(existingSavePath(ai, slot));
    if (!file.open(QIODevice::ReadOnly) || file.size() > MaxFileBytes) {
        error = "Cannot open saved game";
        return false;
    }
    QJsonParseError parse;
    auto document = QJsonDocument::fromJson(file.readAll(), &parse);
    auto data = document.object();
    if (parse.error != QJsonParseError::NoError || !document.isObject()
        || !data["version"].isDouble() || data["version"].toDouble() != 1
        || (ai ? (data["mode"].toString() != "Easy" && data["mode"].toString() != "Medium" && data["mode"].toString() != "Hard")
               : data["mode"].toString() != "Sandbox"))
        return fail();
    auto integer = [&](QString key, int low, int high) {
        const auto v = data.value(key);
        double n = v.toDouble(-1e12);
        return v.isDouble() && n == std::floor(n) && n >= low && n <= high;
    };
    if (!data["moves"].isArray() || data["moves"].toArray().size() > MaxMoves
        || !integer("humanColor", 0, 1) || !integer("cursor", 0, data["moves"].toArray().size())
        || !integer("undoRemaining", 0, historyLimit) || !integer("redoRemaining", 0, historyLimit)
        || !data["finished"].isBool() || !integer("winnerIndex", -1, 1))
        return fail();
    std::vector<GoMatch> history{GoMatch()};
    std::vector<QJsonObject> moves;
    QString replayError;
    for (const auto &value : data["moves"].toArray()) {
        if (!value.isObject() || history.back().consecutivePasses >= 2)
            return fail();
        auto move = value.toObject();
        auto next = history.back();
        if (move["pass"].isBool() && move["pass"].toBool())
            next.pass();
        else {
            auto row = move["row"], col = move["col"];
            if (!row.isDouble() || !col.isDouble() || row.toDouble() != std::floor(row.toDouble())
                || col.toDouble() != std::floor(col.toDouble()) || row.toDouble() < 0
                || row.toDouble() > 18 || col.toDouble() < 0 || col.toDouble() > 18
                || !next.play(row.toInt(), col.toInt(), replayError))
                return fail();
        }
        history.push_back(next);
        moves.push_back(move);
    }
    const int cursor = data["cursor"].toInt();
    GoMatch position;
    if (!GoMatch::fromJson(data["position"].toObject(), position, replayError)
        || position.toJson() != history[cursor].toJson())
        return fail();
    if ((position.consecutivePasses >= 2 && !data["finished"].toBool())
        || (data["finished"].toBool() && data["winnerIndex"].toInt() < 0))
        return fail();

    LocalGameData loaded;
    loaded.ai = ai;
    loaded.difficulty = ai ? data["mode"].toString() : QStringLiteral("Medium");
    loaded.humanColor = data["humanColor"].toInt();
    loaded.moves = std::move(moves);
    loaded.history = std::move(history);
    loaded.position = std::move(position);
    loaded.cursor = cursor;
    loaded.undoRemaining = data["undoRemaining"].toInt();
    loaded.redoRemaining = data["redoRemaining"].toInt();
    loaded.finished = data["finished"].toBool();
    loaded.winnerIndex = data["winnerIndex"].toInt();
    loaded.endReason = data["endReason"].toString();
    game = std::move(loaded);
    error.clear();
    return true;
}
