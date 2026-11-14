# GoGame

GoGame is an application that allows users to play the classic board game Go. It provides a simple and interactive interface to enjoy the game. It is written in **C++17 and Qt 6**, featuring a Qt Quick/QML client and a JSON WebSocket server. It supports local two-player matches, offline play against **Easy / Medium / Hard** AI opponents, and a **Sandbox** mode where players can control both sides.

# Table of contents

- [Requirement](#requirements)
- [Installation](#installation)
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

## Game Feature

- **Play against AI → Easy / Medium / Hard**: Play offline, with the human player controlling Black. You can save up to 20 games angainst AI. Use the game-loading button in the menu to resume a saved game, select a slot, and confirm overwriting if all slots are occupied.
- **Sandbox**: Control both sides, with support for Reset, Undo, Redo, Save, and Load. The `sandbox.json` slot is separate. Reset and Exit do not automatically overwrite manually saved games.
- The first click on a valid intersection displays a move preview; the second click confirms the move. **Fast mode** allows a move to be placed with a single click.

The server is not required for AI matches or Sandbox mode.

# Rules

See [GoGame Rules — Simplified Japanese-style Scoring](docs/RULES.md) for the current rule.

# Project Documentation

- [AI Usage Declaration](docs/AI_Declaration.md)
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
