# GoGame WebSocket Server

This online game now.

## Protocol

Send text messages containing a JSON object in the form `{ "type": "...", "payload": { ... } }`.

Read the file:

```json
{ "type": "ReadData", "payload": {} }
```

Response:

```json
{ "type": "Data", "payload": { "data": {} } }
```

Write the file:

```json
{
    "type": "WriteData",
    "payload": {
        "data": { "board": [0, -1, 1], "moveCount": 3, "currentPlayer": 1, "time": { "black": 600, "white": 590 } }
    }
}
```

`payload.data` must be a JSON object. Each write request **replaces the entire file**; the example above only illustrates the protocol and is not a complete 19×19 board state. A successful write returns:

```json
{ "type": "DataWritten", "payload": { "ok": true } }
```

Errors use this format:

```json
{ "type": "Error", "payload": { "code": "InvalidPayload", "message": "payload.data must be a JSON object" } }
```

Error codes: `InvalidJson`, `InvalidPayload`, `UnknownType`, `ReadFailed`, `WriteFailed`, `UnsupportedMessage`. Binary messages are unsupported. Incoming messages are limited to 1 MiB.

The server processes requests sequentially and uses `QSaveFile` for atomic file replacement. Clients share one file; the last successful write determines its contents. Run only one server per data file. The server serves local clients and supports guest nickname authentication, matchmaking, and basic Go rules. TLS and password-based account authentication are not implemented.

## Existing Client Connection

The client supports the `GOGAME_SERVER_URL` environment variable. For example:

```bash
GOGAME_SERVER_URL=ws://127.0.0.1:14509 ./build/Desktop_Debug/appGoGame
```

The client connects automatically at startup, defaulting to `ws://127.0.0.1:14509`. The login screen displays connection status/errors and provides a reconnect button. `ReadData`/`WriteData` is a separate JSON-file API; online matches use commands/snapshots. The server supports guest AuthRequest and MatchmakingStart/MatchmakingCancel. Password login in the UI is not integrated with this guest protocol.

## Server Class

`Server.h` declares the API, `Server.cpp` implements it, and `main.cpp` parses CLI arguments and calls `start`. The class receives the file path at construction and owns the WebSocket listener and client connections.

- `start(port, error)`: prepares the JSON file and opens the localhost listener.
- `read(client, text)`: receives text, parses JSON, and calls `handler`.
- `handler(client, message)`: dispatches requests by `type`.
- `send(client, message)`: sends a JSON object; returns the number of bytes accepted for sending, or `-1` if the client is disconnected.
- `jsonToString(json)`: converts an object to a compact JSON string.
- `stringToJson(text, json, error)`: returns `true` for a valid JSON object, otherwise `false` with an error message.
- `envelope(type, payload = {})`: returns `{type, payload}`, with an empty object as the default payload.
- `readData(data, error)` / `writeData(data, error)`: reads/writes the server's JSON file.

Example responses in a handler:

```cpp
send(client, envelope("Data", {{"data", data}}));
send(client, envelope("Ready")); // {"type":"Ready","payload":{}}
```

To add a message type, add a branch in `Server::handler`. Guest authentication, matchmaking, and GameCommand are handled by the Server class.

## GameCommand for Go

Both clients send `AuthRequest` with `payload.nickname`, then `MatchmakingStart` with the same `payload.mode` to create a match. The server sends `MatchFound` (Black = 0, White = 1) and `GameSnapshot`. Each match has its own board and clocks: by default, 10 minutes of main time per player followed by 3 × 20-second Byo-yomi periods. The timer checks main-time/Byo-yomi boundaries and at most every 30 seconds.

Place a stone:

```json
{
    "type": "GameCommand",
    "payload": { "matchId": "ID_FROM_MATCH_FOUND", "commandType": "Play", "command": { "row": 3, "col": 3 } }
}
```

Coordinates are integers from 0 to 18 in board-data coordinates, independent of UI rotation. The server validates match membership, turn, empty intersections, capture of groups without liberties, the suicide prohibition, and simple ko. Rejected commands do not change the board/turn; thinking time is still deducted.

Pass (`EndRound` is retained for compatibility with the existing enum):

```json
{ "type": "GameCommand", "payload": { "matchId": "ID_FROM_MATCH_FOUND", "commandType": "EndRound", "command": {} } }
```

Resign, including outside the player's turn:

```json
{ "type": "GameCommand", "payload": { "matchId": "ID_FROM_MATCH_FOUND", "commandType": "Concede", "command": {} } }
```

The server emits `GameEvent` with `StonePlayed`, `Passed`, or `GameEnded`, and `GameSnapshot` containing `board` (361 cells, -1/0/1), `moveCount`, `currentPlayerIndex`, `remainingMs`, `started`, and `komi`. Two consecutive passes end the game. Scoring counts empty territory bordered by only one color plus captured stones; White receives 6.5 komi under the Japanese scoring formula. Stones remaining on the board do not add points. Scores are provisional because dead stones/seki have not been confirmed. This version has no dead-stone agreement before scoring or positional/situational superko. Disconnecting during a match counts as a loss. Active matches exist only in memory; the ReadData/WriteData JSON file remains independent.

`GoMatch.h/.cpp` contains the rules; `Server::gameCommand` handles validation, protocol, and clocks. The JavaScript card-game commands were replaced with the three Go commands above. Client MatchScreen sends GameCommand during online matches and updates board/turn from the server's GameSnapshot. Preview can only be selected on the player's turn; confirmation sends a command and locks input while waiting for a response.

## Recipient-Specific Snapshots

`sendSnapshot(socket, match)` sends to an individual member socket; `sendSnapshot(match)` calls it for both players. It retains `board`, `moveCount`, `currentPlayerIndex`, `remainingMs`, `started`, and `komi`, and adds:

- `stage`: `Action` or `GameEnd`; `boardSize: 19`, `consecutivePasses`.
- `players`: `playerId`, `nickname`, `playerIndex`, `color`, `remainingTimeMs`.
- `self.playerIndex`: recipient perspective, 0 for Black / 1 for White.
- `serverTimeEpochMs`, `actionDeadlineEpochMs`: display synchronization timestamps; the deadline is the active player's current main-time/period boundary, not a 30-second per-move limit.
- `winnerIndex`, `endReason`: -1/empty string during play, results after completion. Final clocks freeze and the final deadline is 0.

The server enforces clocks using monotonic time; epoch timestamps are only for UI display. Snapshots are not sent every second; the client updates countdowns between snapshots. There are no round/weather/character/card fields because this is Go. See [architecture and schemas](ARCHITECTURE.md) for interaction diagrams and [roadmap](ROADMAP.md) for remaining work.

## Byo-yomi Clocks

Each clock appears next to its player's name panel, above the background. The active player has a gold border. The UI shows `mm:ss` during main time and seconds plus remaining periods during Byo-yomi. Unconfirmed preview time still counts against the clock.

Default: **10 minutes of main time + 3 × 20 seconds**. To use 15 minutes of main time:

```bash
./build/server/appserver --main-minutes 15 --data-file server/data/state.json
```

Exhausting main time starts Byo-yomi rather than an immediate loss. Every exhausted 20-second period consumes one period, then the next starts immediately within the same turn. A legal move or pass stops the clock and restores the current period to 20 seconds for the next turn; consumed periods are not restored. Invalid commands do not reset time. Exhausting the final period immediately loses on the server. If a timer runs late, the server still deducts all actual elapsed time using a monotonic clock.

Snapshots add `clocks[0/1]`: `mainRemainingMs`, `periodRemainingMs`, `periodDurationMs`, `periodsRemaining`, and `phase` (`Main`/`ByoYomi`). `remainingMs` is retained for compatibility and represents time to the current boundary. The client only interpolates the display; the server decides the result. Clock-only snapshot updates do not clear preview or change moves.

## Cleanup and Source Statistics (13 October 2026)

These are historical measurements. `scripts/server_stats.py` and the tests mentioned here are no longer in the source tree; do not treat the recount command below as current instructions.

Removed the unused `Server::stop`/`address` APIs and old selection timeout; the destructor still closes sockets/timers. Removed server QML and an old test version; retained GoMatch, ByoYomiClock, snapshots, and all four tests running at that time. The client matchmaking protocol sends only mode, without deckId. The Play helper takes matchId/row/col.

Measurements on 13 October 2026: server runtime **841 lines / 28,126 characters**; including four tests and CMake, **1,277 lines / 51,924 characters**. These are source totals at that time, including comments/whitespace, not separate human/AI authorship counts. The recount command was `python3 scripts/server_stats.py`; that script is now absent. Cleanup details, measurement methodology, and per-file counts were recorded in the [technical log](CHANGELOG.md).

## DSU and Match Notifications

GoMatch uses DSU stone groups, liberty bitsets, an immutable position cache, and empty-region DSU for territory scoring. Snapshots add `position` (board, previousBoard, simple ko, captures, turn/move count) and `scoring` (rules, blackTerritory, whiteTerritory, blackPrisoners, whitePrisoners, blackPoints, whitePoints, komi, provisional, difference). StonePlayed GameEvent adds `captured`, `koPoint`, and `koThreat`; GameEnded after two passes adds `scoring`. The client displays capture, ko, and provisional Japanese score notifications. `koThreat` identifies a response after ko that puts an opposing group into atari; it is not a complete analysis of every ko threat.

Sandbox and AI save/undo/redo run on the client; online matches do not allow undo/redo. See the [AI documentation](ai/IMPLEMENTATION_GUIDE.md).

## Server/Client Build Boundary

Shared rules live in `src/Rules/GoMatch.*`; the server compiles the rules source directly and links only Qt Core/Network/WebSockets. The standalone server does not build AI, DataManager, or Qt Concurrent. Root CMake compiles AI, DataManager, and rules directly into the client; only the server has a separate CMake file. Test files are absent, but root CMake still references `tests/` when `BUILD_TESTING` is enabled; use `-DBUILD_TESTING=OFF` for the current client build. `DataManager` belongs to the client and handles JSON Save/Load.

PlayerPanel displays Current points from scoring for each player's color. Territory, captured-stone, and komi calculations follow the [Nihon Ki-in Japanese rules](https://www.nihonkiin.or.jp/match/kiyaku/zenbun.html).

Current points count only secured territory identified by Benson elimination, using chains with two vital eye regions, plus captured stones; White receives komi. Open regions, groups not proven alive, and false eyes do not count as territory. scoreBasis is SecuredTerritory; unsettledPoints records uncounted empty intersections. This is a conservative estimate, not a replacement for dead-stone/seki confirmation. See [pass-alive territory in KataGo](https://lightvector.github.io/KataGo/rules.html#additional-definitions).

GameManager independently calculates scoring from m_position (board plus captures) using GoMatch::scoreDetails. QML uses gameManager.scoring for Current points; gameManager.serverScoring retains scoring received from GameSnapshot. gameManager.scoringComparison includes status local/unavailable/matched/mismatch, moveCount, matches, and per-field differences (client/server/delta or missing). Comparison happens only after applying the position from the same snapshot; a game-ending GameEvent must not be compared with the previous turn's board. scoringChanged updates the UI; scoringMismatchDetected fires on a new mismatch. The panel includes a Server score line alongside the client score. Comparing calculations on the same state does not prove that the server state itself is correct.

ScoringPhase::Current uses secured territory during play; ScoringPhase::Final counts all empty regions enclosed by only one color, captured stones, and White's komi. Regions bordered by both colors do not score. score() uses Final to choose the winner after two passes. Final snapshots and GameManager use Final, so comparison uses the same phase. The result panel separates territory, captures, komi, and total; dead stones/seki still require confirmation for complete Japanese adjudication.
