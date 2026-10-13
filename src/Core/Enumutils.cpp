#include "Enumutils.h"

namespace EnumUtils{
QString toString(NetworkMessageType type)
{
    switch (type) {
    case NetworkMessageType::Unknown: return QStringLiteral("Unknown");
    case NetworkMessageType::AuthRequest: return QStringLiteral("AuthRequest");
    case NetworkMessageType::AuthResult: return QStringLiteral("AuthResult");
    case NetworkMessageType::MatchmakingStart: return QStringLiteral("MatchmakingStart");
    case NetworkMessageType::MatchmakingCancel: return QStringLiteral("MatchmakingCancel");
    case NetworkMessageType::MatchFound: return QStringLiteral("MatchFound");
    case NetworkMessageType::MatchmakingStatus: return QStringLiteral("MatchmakingStatus");
    case NetworkMessageType::GameSnapshot: return QStringLiteral("GameSnapshot");
    case NetworkMessageType::GameCommand: return QStringLiteral("GameCommand");
    case NetworkMessageType::GameEvent: return QStringLiteral("GameEvent");
    case NetworkMessageType::Error: return QStringLiteral("Error");
    }

    return QStringLiteral("Unknown");
}

QString toString(GameCommandType type)
{
    switch (type) {
    case GameCommandType::Unknown: return QStringLiteral("Unknown");
    case GameCommandType::Play: return QStringLiteral("Play");
    case GameCommandType::EndRound: return QStringLiteral("EndRound");
    case GameCommandType::Concede: return QStringLiteral("Concede");
    }

    return QStringLiteral("Unknown");
}
}