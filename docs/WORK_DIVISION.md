# Team Work Division

**Project:** GoGame — C++17 / Qt 6  
**Updated:** 21 October 2026

This document responsibilities describe development and integration work; it does not imply sole authorship of every line in a listed file. AI assistance and upstream code are identified separately in [AI_Declaration.md](AI_Declaration.md).

## Team Members

| Member                | Student ID |
| --------------------- | ---------- |
| Hoang Minh Vu         | 26125090   |
| Nguyen Cao Long Khanh | 26125035   |

## Hoang Minh Vu

Recorded responsibilities include the login screen, server logic, reusable QML components, initial game UI, Medium/Hard AI, KataGo integration, game rules, game management, matchmaking, authentication, save/load, and parts of the documentation. **"-----"** Means all wrote by this member.

### Server and Shared Rules

| Files                                                     | Recorded contribution |
| --------------------------------------------------------- | --------------------- |
| `server/main.cpp`, `server/Server.h`, `server/Server.cpp` | **-----**             |
| `server/ByoYomiClock.h`                                   | **-----**             |
| `server/GoMatch.h`, `server/GoMatch.cpp`                  | **-----**             |
| `src/Rules/GoMatch.h`, `src/Rules/GoMatch.cpp`            | **-----**             |
| `server/CMakeLists.txt`                                   | **-----**             |

### QML Interface and Components

| Files                                                                                                 | Recorded contribution                                                                                                                     |
| ----------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------- |
| `qml/Screens/LoginScreen.qml`                                                                         | **-----**                                                                                                                                 |
| `qml/Screens/MainMenu.qml`                                                                            | Initial/default menu interface; approximately 20% contribution. This percentage is self-reported, not calculated from Git or line counts. |
| `qml/Components/battle/GameClock.qml`, `qml/Components/battle/PlayerPanel.qml`                        | **-----**                                                                                                                                 |
| `qml/Components/controls/GameButton.qml`, `GamePanel.qml`, `GameText.qml`                             | **-----**                                                                                                                                 |
| `qml/Components/controls/SaveGameDialog.qml`, `SaveSlotTable.qml`                                     | **-----**                                                                                                                                 |
| `qml/Core/Motion.qml`, `Radius.qml`, `SceneManager.qml`, `Spacing.qml`, `Theme.qml`, `Typography.qml` | **-----**                                                                                                                                 |

### Game Management, Networking, and Persistence

| Files                                                                      | Recorded contribution                                                                                                                                                       |
| -------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/Core/Enums.h`, `src/Core/Enumutils.h`, `src/Core/Enumutils.cpp`       | **-----**                                                                                                                                                                   |
| `src/Core/Game.h`, `src/Core/Game.cpp`                                     | Do not contain `startMediumGame()`, `startHardGame()`, `stopAI()`, `loadAIGame()`, `aiEnabled()`, `aiDifficulty()`, `pass_turn()`, `aiStateChanged()`, and `startAIGame()`. |
| `src/Managers/AuthManager.h`, `src/Managers/AuthManager.cpp`               | **-----**                                                                                                                                                                   |
| `src/Managers/MatchmakingManager.h`, `src/Managers/MatchmakingManager.cpp` | **-----**                                                                                                                                                                   |
| `src/Managers/DataManager.h`, `src/Managers/DataManager.cpp`               | Including AI-assisted Sandbox save support.                                                                                                                                 |
| `src/Network/NetworkClient.h`, `src/Network/NetworkClient.cpp`             | **-----**                                                                                                                                                                   |
| `src/Network/Protocol.h`, `src/Network/Protocol.cpp`                       | **-----**                                                                                                                                                                   |

### AI and KataGo Integration

| Files                                                    | Recorded contribution and attribution                                                                                                                                |
| -------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/AI/MediumAI.h`, `src/AI/MediumAI.cpp`               | Medium AI logic and integration, with assistance recorded in the AI declaration.                                                                                     |
| `src/AI/HardAI.h`, `src/AI/HardAI.cpp`                   | Basic search logic, design direction, and optimization ideas; AI assisted with completion under the member's guidance. See the HardAI entries in the AI declaration. |
| `src/AI/AIGame.h`, `src/AI/AIGame.cpp`                   | \_                                                                                                                                                                   |
| `src/AI/KataGoEvaluator.h`, `src/AI/KataGoEvaluator.cpp` | Almost copy from open source.                                                                                                                                        |
| `src/AI/KataGoNative.h`, `src/AI/KataGoNative.cpp`       | Almost copy from open source.                                                                                                                                        |
| `src/AI/SearchTypes.h`                                   | _-_                                                                                                                                                                  |
| `main.cpp`,`main.qml`                                    | \_                                                                                                                                                                   |

## Nguyen Cao Long Khanh

Recorded responsibilities cover asset collection and integration, the settings interface and asset preference state, main-menu match-option selection for local/AI play, and Easy AI integration.

### Asset collection and integration

| Files                                                                         | Recorded contribution and attribution                                                                                                                                                                                                |
| ----------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `assets/background/*`, `assets/board/*`, `assets/sounds/*`, `assets/stones/*` | Collected royalty-free game assets from Internet sources, including in-match background ambient images, playing-board textures, stone appearances, background songs, and in-game sound effects like stone placing, wrong move error. |
| `src/Managers/AssetManager.cpp`, `src/Managers/AssetManager.h`                | Integrated the collected assets into the game and implemented state management for user preferences, so selected background, board, stone, song, and volume settings are applied consistently.                                       |

### Settings interface

| Files                                                     | Recorded contribution and attribution                                                                                                                                                                                                                                                                                                    |
| --------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `qml/Screens/SettingsScreen.qml`                          | Implemented the settings layout and connected its controls to asset preference state managed through `src/Managers/AssetManager.cpp`. The screen allows the user to customize the in-match background ambient image, playing-board texture, stone appearance, background song, background-song volume, and in-match sound-effect volume. |
| `qml/Screens/MainMenu.qml`, `qml/Screens/MatchScreen.qml` | Added two-way access to `SettingsScreen.qml` from these screens, allowing customization before and after a match without disrupting the existing screen flow.                                                                                                                                                                            |

### Main menu match options selection for local play

| Files                      | Recorded contribution and attribution                                                                        |
| -------------------------- | ------------------------------------------------------------------------------------------------------------ |
| `qml/Screens/MainMenu.qml` | Handled the selection logic for AI difficulties and the user's playing color in the local AI game interface. |

### Easy AI integration

| Files                                  | Recorded contribution and attribution                                                                                                                                                                                          |
| -------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `src/AI/EasyAI.h`, `src/AI/EasyAI.cpp` | Adapted the Medium AI implementation into an Easy AI variant, adjusted its decision behavior for easier play, and integrated it into the AI game flow. The logic is derived from `src/AI/MediumAI.h` and `src/AI/MediumAI.cpp` |

## Collaboration Tools Used:

- GitHub: For codebase management and version control.
- Messenger: For facilitating communication between team members.
