# Team Work Division

**Project:** GoGame — C++17 / Qt 6  
**Updated:** 21 October 2026

This document responsibilities describe development and integration work; it does not imply sole authorship of every line in a listed file. AI assistance and upstream code are identified separately in [AI_Declaration.md](AI_Declaration.md).

## Team Members

| Member | Student ID | Record status |
| --- | --- | --- |
| Hoang Minh Vu | 26125090 | Name and ID supplied by the member. |
| Nguyen Cao Long Khanh | 261250 | ID retained from the original work-division notes; the member should confirm the complete ID before submission. |

## Hoang Minh Vu

Recorded responsibilities include the login screen, server logic, reusable QML components, initial game UI, Medium/Hard AI, KataGo integration, game rules, game management, matchmaking, authentication, save/load, and parts of the documentation. **"-----"** Means all wrote by Vu.

### Server and Shared Rules

| Files | Recorded contribution |
| --- | --- |
| `server/main.cpp`, `server/Server.h`, `server/Server.cpp` | **-----** |
| `server/ByoYomiClock.h` | **-----** |
| `server/GoMatch.h`, `server/GoMatch.cpp` | **-----** |
| `src/Rules/GoMatch.h`, `src/Rules/GoMatch.cpp` | **-----** |
| `server/CMakeLists.txt` | **-----** |

### QML Interface and Components

| Files | Recorded contribution |
| --- | --- |
| `qml/Screens/LoginScreen.qml` | **-----** |
| `qml/Screens/MainMenu.qml` | Initial/default menu interface; approximately 20% contribution. This percentage is self-reported, not calculated from Git or line counts. |
| `qml/Components/battle/GameClock.qml`, `qml/Components/battle/PlayerPanel.qml` | **-----** |
| `qml/Components/controls/GameButton.qml`, `GamePanel.qml`, `GameText.qml` | **-----** |
| `qml/Components/controls/SaveGameDialog.qml`, `SaveSlotTable.qml` | **-----** |
| `qml/Core/Motion.qml`, `Radius.qml`, `SceneManager.qml`, `Spacing.qml`, `Theme.qml`, `Typography.qml` | **-----** |

### Game Management, Networking, and Persistence

| Files | Recorded contribution |
| --- | --- |
| `src/Core/Enums.h`, `src/Core/Enumutils.h`, `src/Core/Enumutils.cpp` | **-----** |
| `src/Core/Game.h`, `src/Core/Game.cpp` | Do not contain `startMediumGame()`, `startHardGame()`, `stopAI()`, `loadAIGame()`, `aiEnabled()`, `aiDifficulty()`, `pass_turn()`, `aiStateChanged()`, and `startAIGame()`.  |
| `src/Managers/AuthManager.h`, `src/Managers/AuthManager.cpp` | **-----** |
| `src/Managers/MatchmakingManager.h`, `src/Managers/MatchmakingManager.cpp` | **-----** |
| `src/Managers/DataManager.h`, `src/Managers/DataManager.cpp` | Including AI-assisted Sandbox save support. |
| `src/Network/NetworkClient.h`, `src/Network/NetworkClient.cpp` | **-----** |
| `src/Network/Protocol.h`, `src/Network/Protocol.cpp` | **-----** |

### AI and KataGo Integration

| Files | Recorded contribution and attribution |
| --- | --- |
| `src/AI/MediumAI.h`, `src/AI/MediumAI.cpp` | Medium AI logic and integration, with assistance recorded in the AI declaration. |
| `src/AI/HardAI.h`, `src/AI/HardAI.cpp` | Basic search logic, design direction, and optimization ideas; AI assisted with completion under the member's guidance. See the HardAI entries in the AI declaration. |
| `src/AI/AIGame.h`, `src/AI/AIGame.cpp` | _ |
| `src/AI/KataGoEvaluator.h`, `src/AI/KataGoEvaluator.cpp` | Almost copy from open source. |
| `src/AI/KataGoNative.h`, `src/AI/KataGoNative.cpp` | Almost copy from open source. |
| `src/AI/SearchTypes.h` | _-_ |
| `main.cpp`,`main.qml` | _|

## Nguyen Cao Long Khanh

The supplied notes list Khanh as a team member but do not specify his individual tasks or files. This section remains to be completed with his confirmed contributions. Unlisted work and functions excluded from Vu's claim are not automatically attributed to Khanh.

## Collaboration Tools Used:
- Github.
