# GoGame

GoGame is an application that allows users to play the classic board game Go. It provides a simple and interactive interface to enjoy the game. It is written in **C++17 and Qt 6**, featuring a Qt Quick/QML client and a JSON WebSocket server. It supports local two-player matches, offline play against **Easy / Medium / Hard** AI opponents, and a **Sandbox** mode where players can control both sides.

# Table of contents

- [Requirement](#requirements)
- [Installation](#installation)
- [Setup and Run](#setup-and-run)
- [Usage](#usage)
- [Rule](#rules)
- [Contributing](#contributing)
- [License](#license)

# Requirements

CMake ≥ 3.16, a C++17 compiler, and Qt 6 (CMake requires ≥ 6.2) with Core, Gui, Quick/QML, Network, WebSockets, Sql, Concurrent, and Multimedia. Qt must support `QWebSocket::errorOccurred`, as required by the current source code. The SQLite plugin is also required for local account authentication.

# Installation

- I don't know but just don't load **Cmake** and continue reading.

# Usage

## Build and Launch the Client

```bash
./tools/run-katago-hard.sh --ai-time-ms 1800 --ai-threads 4 --katago-duty 0.30
```

You can also open `CMakeLists.txt` in Qt Creator. The default build does not require a KataGo model; Hard uses a heuristic evaluator instead.

## Run the Server and Play Local

Open a separate terminal and run the following commands to build and start the server. Keep the server running while playing:

```bash
cmake -S server -B build/server
cmake --build build/server --parallel 3
./build/server/appserver --data-file server/data/state.json
```

The server listens at `ws://127.0.0.1:14509`.

If the file is corrupted, the server will stop without modifying the existing data. `state.json` is used by the JSON read/write API; it is not a file that automatically saves ongoing matches.

By default, each player has 10 minutes of main time plus 3 periods of 20-second Byo-yomi.

The server currently listens on localhost only. Client-side SQLite authentication and server-side guest nickname identities are separate systems.

The client uses the default server URL. If you change the port, set the corresponding URL before launching the client but you should not change it due to potential bugs:

```bash
GOGAME_SERVER_URL=ws://127.0.0.1:14509 ./build/local/appGoGame
```

In another terminal, launch two client instances, register or log in with two local accounts, and select **Find Match** in both clients to get matched.

## Play Against AI, use Sandbox Mode and Fast Mode

- **Play against AI → Easy / Medium / Hard**: Play offline, with the human player controlling Black. You can save up to 20 games angainst AI. Use the game-loading button in the menu to resume a saved game, select a slot, and confirm overwriting if all slots are occupied.
- **Sandbox**: Control both sides, with support for Reset, Undo, Redo, Save, and Load. The `sandbox.json` slot is separate. Reset and Exit do not automatically overwrite manually saved games.
- By default, game data is stored in `QStandardPaths::AppDataLocation/games`. Set the `GOGAME_SAVE_DIR` environment variable to use a different directory.
- The first click on a valid intersection displays a move preview; the second click confirms the move. **Fast mode** allows a move to be placed with a single click.

The server is not required for AI matches or Sandbox mode.

The current scorer still has limitations when adjudicating dead stones and seki.

# Rule

This game currently uses Japanese rules, however there may be some inaccuracies, so please report them for debugging.

# Project Documentation

All of these files were written by Vietnamese people to support debugging for developers and assist AI. The translation will not be implemented.

- [Project Structure and References](docs/STRUCTUREANDREF.md)
- [Prompt / Feedback / Change Log](docs/PROMPT.md)
- [Architecture and Protocol](docs/ARCHITECTURE.md)
- [Server Documentation](docs/SERVER.md)

# Contributing

This application developed by two members:

- Hoang Minh Vu
- Nguyen Cao Long Khanh

We work together to develop this Go Game application as part of a university project.

# License

This project is for educational purposes only and is not intended for commercial use.
