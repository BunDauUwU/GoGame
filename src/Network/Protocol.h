#pragma once
#include <QJsonObject>
#include <QString>
#include "../Core/Enums.h"

namespace Protocol {
QJsonObject makeEnvelope(NetworkMessageType type, const QJsonObject &payload);
QJsonObject makeAuthRequest(const QString &nickname);
QJsonObject makeStartMatchmaking(const QString &mode);
QJsonObject makeCancelMatchmaking();
QJsonObject makeGameCommand(const QString &matchId, GameCommandType commandType,
                            const QJsonObject &commandPayload);
QJsonObject makePlayCommand(const QString &matchId, int row, int col);
QString readMessageType(const QJsonObject &message);
QJsonObject readPayload(const QJsonObject &message);
} // namespace Protocol
