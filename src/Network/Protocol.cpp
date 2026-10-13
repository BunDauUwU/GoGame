// src/Network/Protocol.cpp
#include "Protocol.h"

#include "../Core/Enumutils.h"

namespace Protocol {

QJsonObject makeEnvelope(NetworkMessageType type, const QJsonObject& payload)
{
    QJsonObject message;
    message["type"] = EnumUtils::toString(type);
    message["payload"] = payload;

    return message;
}

QJsonObject makeAuthRequest(const QString& nickname)
{
    QJsonObject payload;
    payload["mode"] = "guest";
    payload["nickname"] = nickname;

    return makeEnvelope(NetworkMessageType::AuthRequest, payload);
}

QJsonObject makeStartMatchmaking(const QString& mode)
{
    QJsonObject payload;
    payload["mode"] = mode;

    return makeEnvelope(NetworkMessageType::MatchmakingStart, payload);
}

QJsonObject makeCancelMatchmaking()
{
    return makeEnvelope(NetworkMessageType::MatchmakingCancel, QJsonObject{});
}

QJsonObject makeGameCommand(
    const QString& matchId,
    GameCommandType commandType,
    const QJsonObject& commandPayload
    )
{
    QJsonObject payload;
    payload["matchId"] = matchId;
    payload["commandType"] = EnumUtils::toString(commandType);
    payload["command"] = commandPayload;

    return makeEnvelope(NetworkMessageType::GameCommand, payload);
}

QJsonObject makePlayCommand(const QString &matchId, int row, int col) {
    return makeGameCommand(matchId, GameCommandType::Play, {{"row", row}, {"col", col}});
}


QString readMessageType(const QJsonObject& message)
{
    return message.value("type").toString("Unknown");
}

QJsonObject readPayload(const QJsonObject& message)
{
    return message.value("payload").toObject();
}

}
