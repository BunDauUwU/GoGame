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

- **Linux:** Use a distribution compatible with the release archive. Choose the CPU edition for KataGo inference on the CPU, or the GPU (CUDA) edition if you have a compatible NVIDIA GPU and driver.
- **Windows:** Use 64-bit Windows. The installer bundles Qt and the KataGo model, and installs the Microsoft Visual C++ Redistributable x64 if needed. The Windows release uses the CPU backend (I do not like Windows as how annoying they are so there is no GPU version).
- **Building from source:** Install CMake 3.17 or later, a C++17 compiler, and Qt 6 (CMake requires ≥ 6.2) with Core, Gui, Quick/QML, Network, WebSockets, Sql, Concurrent, and Multimedia. Use a Qt version that supports `QWebSocket::errorOccurred` and includes the SQLite driver for local accounts.

# Installation

## Linux

1. Download the CPU or CUDA archive from the [Releases page](https://github.com/BunDauUwU/GoGame/releases).
2. Extract the archive and open a terminal in the extracted folder.
3. Run the included `.sh` launcher to start GoGame.

## Windows

1. Download `gogame_installer.exe` from the [Releases page](https://github.com/BunDauUwU/GoGame/releases).
2. Open the installer, review the license, and follow the prompts. You can choose to create a desktop shortcut.
3. Launch **GoGame** from the Start menu or the desktop shortcut.

# Usage

- **Find Match** searches for an online opponent. If the hosted server has gone idle, wait for it to start and use **Reconnect**. If the connection still fails, restart the game.
- **Practice Mode** lets you play offline against Easy, Medium, or Hard AI. In the packaged releases, Hard uses the KataGo model.
- **Sandbox** lets you control both sides. Use it to explore positions or play locally with a friend.
- **Load** lets you resume a local game. Each mode ("Practice", "Sandbox") has 20 separate save slots.
- **Settings** lets you change the background, stone style, board texture, music, and volume.
- **Fast mode** places a stone with one click. With Fast mode off, click once to preview the move and again to confirm it.

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
