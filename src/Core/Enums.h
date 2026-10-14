// src/Core/Enums.h
#pragma once

enum class NetworkMessageType {
    Unknown,
    AuthRequest,
    AuthResult,
    MatchmakingStart,
    MatchmakingCancel,
    MatchFound,
    MatchmakingStatus,
    GameSnapshot,
    GameCommand,
    GameEvent,
    Error
};

enum class GameCommandType { Unknown, Play, EndRound, Concede };
