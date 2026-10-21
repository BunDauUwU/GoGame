# Project Structure and References

Checked directly against the filesystem on **21 October 2026**, after documentation reorganization. Source paths and commands are relative to the project root; Markdown links are relative to the document being read.

## Documentation Locations

| File | Contents |
| --- | --- |
| [README.md](../README.md) | Overview and usage: build, client, server, AI, Sandbox, and KataGo. |
| [AI_Declaration.md](AI_Declaration.md) | Team AI usage declaration, attribution, and individual confirmations. |
| [PROMPT.md](PROMPT.md) | Full prompts, recoverable responses, changes, and checks for each recorded request. |
| [STRUCTUREANDREF.md](STRUCTUREANDREF.md) | Project tree and reference catalog. |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Client/server diagrams, WebSocket flow, and snapshot schema. |
| [SERVER.md](SERVER.md) | Server protocol, commands, snapshots, clocks, and JSON API; moved from `server/README.md`. |
| [ROADMAP.md](ROADMAP.md) | Plans and future-work notes (removed because no further implementation was planned). |
| [ai/KATAGO_BACKEND.md](ai/KATAGO_BACKEND.md) | Native KataGo backend build/run instructions, CPU/CUDA, and limitations. |
| [references/HARD_AI_INPUT.md](references/HARD_AI_INPUT.md) | Complete user-supplied MCTS code for the HardAI request. |
| [archive/README-2026-10-20.md](archive/README-2026-10-20.md) | Unmodified README before reorganization; its paths/status are historical. |

## Current File Tree

Lists source, assets, documentation, and existing empty directories. Build directories, Python environments, caches, Git, and local configuration are collapsed; generated dependency documentation/libraries are outside the project's Markdown set. Runtime data in `server/data/` may be Git-ignored and vary by machine. The KataGo `.zip` package was not extracted to list its contents.

```text
GoGame/
├── .agents/  [collapsed: generated content or local configuration]
├── .aws/  [collapsed: generated content or local configuration]
├── .cache/  [collapsed: generated content or local configuration]
├── .codex/  [collapsed: generated content or local configuration]
├── .git/  [collapsed: generated content or local configuration]
├── .qtcreator/  [collapsed: generated content or local configuration]
├── .venv-ai/  [collapsed: generated content or local configuration]
├── assets/
│   ├── background/
│   │   ├── background1.png
│   │   ├── background2.png
│   │   └── background3.png
│   ├── fonts/
│   │   └── genshin.ttf
│   ├── playerAvatar/
│   │   ├── ayame1.png
│   │   ├── ayame2.png
│   │   ├── ayame3.png
│   │   ├── ayame4.png
│   │   ├── ayame5.png
│   │   ├── ayame6.png
│   │   ├── ayame8.webp
│   │   ├── eula (5).png
│   │   ├── hutao (1).jpeg
│   │   ├── hutao (1).png
│   │   ├── hutao (11).png
│   │   ├── hutao (12).png
│   │   ├── hutao (3).png
│   │   ├── kazuha (3).png
│   │   ├── kazuha (4).png
│   │   ├── kazuha (5).png
│   │   ├── nahida.jpg
│   │   ├── paimon (1).jpg
│   │   ├── yoimiya (1).png
│   │   ├── yoimiya (4).png
│   │   └── yoimiya (8).png
│   ├── sounds/
│   │   ├── background1.mp3
│   │   ├── background2.opus
│   │   ├── background3.mp3
│   │   └── place.wav
│   └── stones/
│       ├── black1.png
│       ├── black2.png
│       ├── white1.png
│       └── white2.png
├── build/  [collapsed: generated content or local configuration]
├── cmake/
│   └── KataGoBackend.cmake
├── docs/
│   ├── ai/
│   │   ├── HARD_AI.md
│   │   ├── IMPLEMENTATION_GUIDE.md
│   │   └── KATAGO_BACKEND.md
│   ├── archive/
│   │   └── README-2026-10-20.md
│   ├── references/
│   │   └── HARD_AI_INPUT.md
│   ├── AI_Declaration.md
│   ├── ARCHITECTURE.md
│   ├── CHANGELOG.md
│   ├── PROMPT.md
│   ├── ROADMAP.md
│   ├── SERVER.md
│   └── STRUCTUREANDREF.md
├── qml/
│   ├── Components/
│   │   ├── battle/
│   │   │   ├── GameClock.qml
│   │   │   └── PlayerPanel.qml
│   │   └── controls/
│   │       ├── GameButton.qml
│   │       ├── GamePanel.qml
│   │       └── GameText.qml
│   ├── Core/
│   │   ├── Motion.qml
│   │   ├── Radius.qml
│   │   ├── SceneManager.qml
│   │   ├── Spacing.qml
│   │   ├── Theme.qml
│   │   └── Typography.qml
│   ├── Screens/
│   │   ├── LoginScreen.qml
│   │   ├── MainMenu.qml
│   │   ├── MatchScreen.qml
│   │   └── SettingsScreen.qml
│   └── Singletons/
│       └── AppState.qml
├── server/
│   ├── data/
│   │   └── state.json
│   ├── ByoYomiClock.h
│   ├── CMakeLists.txt
│   ├── GoMatch.cpp
│   ├── GoMatch.h
│   ├── main.cpp
│   ├── Server.cpp
│   └── Server.h
├── src/
│   ├── AI/
│   │   ├── __pycache__/  [collapsed: generated content or local configuration]
│   │   ├── training/
│   │   │   └── requirements.txt
│   │   ├── AIGame.cpp
│   │   ├── AIGame.h
│   │   ├── EasyAI.cpp
│   │   ├── EasyAI.h
│   │   ├── HardAI.cpp
│   │   ├── HardAI.h
│   │   ├── katago-v1.18.1-cuda12.1-cudnn8.9.7-linux-x64+bs50.zip
│   │   ├── KATAGO_LICENSE.txt
│   │   ├── KataGoEvaluator.cpp
│   │   ├── KataGoEvaluator.h
│   │   ├── KataGoNative.cpp
│   │   ├── KataGoNative.h
│   │   ├── MediumAI.cpp
│   │   ├── MediumAI.h
│   │   ├── SearchTypes.h
│   │   └── trainAI.py
│   ├── Core/
│   │   ├── Enums.h
│   │   ├── Enumutils.cpp
│   │   ├── Enumutils.h
│   │   ├── Game.cpp
│   │   └── Game.h
│   ├── Managers/
│   │   ├── AssetManager.cpp
│   │   ├── AssetManager.h
│   │   ├── AuthManager.cpp
│   │   ├── AuthManager.h
│   │   ├── DataManager.cpp
│   │   ├── DataManager.h
│   │   ├── MatchmakingManager.cpp
│   │   └── MatchmakingManager.h
│   ├── Network/
│   │   ├── NetworkClient.cpp
│   │   ├── NetworkClient.h
│   │   ├── Protocol.cpp
│   │   └── Protocol.h
│   └── Rules/
│       ├── GoMatch.cpp
│       └── GoMatch.h
├── tools/
│   └── run-katago-hard.sh
├── .gitignore
├── CMakeLists.txt
├── main.cpp
├── main.qml
├── README.md
└── users.sqlite
```

## Responsibilities of Major Components

| Location | Responsibility |
| --- | --- |
| `main.cpp`, `main.qml`, `qml/` | Client startup, QML context, menus, match screen, and components. |
| `src/Core/Game.*`, `src/AI/AIGame.cpp` | Game state, mode coordination, AI workers, and stale-result cancellation. |
| `src/Rules/GoMatch.*` | Shared rules, board, groups/liberties, history, and scoring. |
| `src/Managers/` | Local authentication, matchmaking, assets, and JSON save/load. |
| `src/Network/` | WebSocket connections and client protocol packaging. |
| `server/` | Console server, queue, matches, clocks, and JSON read/write API. CMake builds rules from `src/Rules/`; old `server/GoMatch.*` files remain on disk. |
| `src/AI/` | EasyAI, MediumAI, HardAI, KataGo adapters, and SGF training. Hard has independent search board/groups. |
| `cmake/KataGoBackend.cmake` | Native backend integration and a copy of KataGo source. |
| `tools/run-katago-hard.sh` | Builds and launches the client with a KataGo model. |
| `assets/` | Backgrounds, avatars, fonts, stones, music, and sound effects. |
| `docs/` | All project-managed Markdown except the root README. |

The source tree no longer contains `tests/` or `scripts/server_stats.py`; mentions in logs/archives are historical. No source files were changed or redundant files removed during the documentation reorganization.

## References

This catalog was compiled from existing documents rather than a new content review or availability check. The source column identifies where each reference was mentioned; URLs are preserved. The Caro report, 5×5 Q-learning project, and 9×9 Go paper are references, not evidence of GoGame's 19×19 AI performance.

| Reference | Recorded source / context |
| --- | --- |
| [Introduction to Artificial Intelligence Report — Studocu](https://www.studocu.vn/vn/document/dai-hoc-dien-luc/dien-luc-university/bao-cao-nhap-mon-tri-tue-nhan-tao/88382780?sid=a353be05-df77-4401-bae7-4597fba4fd601791775595) |                                                          |
| [Caro Report — equation reference](https://www.scribd.com/document/434712509/BAO-CAO)                                                                                                                        |                                                          |
| [Implementing Go — Modern Descartes](https://www.moderndescartes.com/essays/implementing_go/)                                                                                                                        |                                                          |
| [Atari Go — CodinGame](https://www.codingame.com/ide/puzzle/atari-go)                                                                                                                                                |                                                          |
| [Go Minimax — BaseGo/core](https://github.com/HosseinBakand/GO-Game-Min-Max-/tree/main/BaseGo/core)                                                                                                                  |                                                          |
| [Go Monte Carlo — Java](https://github.com/GTnull/Go/blob/master/version_2.0/src/gogame/monteCarloMethod.java)                                                                                                       |                                                          |
| [5×5 Go AI Bot — Q-learning](https://github.com/27rg5/AI-bot-for-GO-game)                                                                                                                                        |                                                          |
| [GoByGo — Monte-Carlo Based Computer Player for the Game of Go](https://durhamgo.club/GoByGo.pdf)                                                                                                                    |                                                          |
| [AlphaGo — Nature paper (historically recorded URL)](https://deepmind-media.storage.googleapis.com/alphago/AlphaGoNaturePaper.pdf)                                                                                            |                                                          |
| [Nihon Ki-in — Japanese rules](https://www.nihonkiin.or.jp/match/kiyaku/zenbun.htm)                                                                                                                                       | [SERVER.md](SERVER.md)                                   |
| [KataGo — pass-alive territory definitions](https://lightvector.github.io/KataGo/rules.html#additional-definitions)                                                                                                   | [SERVER.md](SERVER.md)                                   |
| [KataGo — Analysis Engine protocol](https://github.com/lightvector/KataGo/blob/master/docs/Analysis_Engine.md)                                                                                                      | [ai/HARD_AI.md](ai/HARD_AI.md)                           |
| [AlphaGo — Nature paper](https://storage.googleapis.com/deepmind-media/alphago/AlphaGoNaturePaper.pdf)                                                                                                               | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [OpenSpiel — AlphaZero](https://github.com/google-deepmind/open_spiel/blob/master/docs/alpha_zero.md)                                                                                                                | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [PyTorch — installation](https://pytorch.org/get-started/locally/)                                                                                                                                                        | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [SGF FF4 — specification](https://www.red-bean.com/sgf/)                                                                                                                                                                    | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [SGF FF4 — property index](https://www.red-bean.com/sgf/proplist_t.html)                                                                                                                                        | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [PyTorch — scaled dot-product attention](https://docs.pytorch.org/docs/stable/generated/torch.nn.functional.scaled_dot_product_attention.html)                                                                       | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [ONNX Runtime — external data for large models](https://onnxruntime.ai/docs/tutorials/web/large-models.html#external-data)                                                                                              | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [ONNX Runtime — C++](https://onnxruntime.ai/docs/get-started/with-cpp)                                                                                                                                               | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [PyTorch — ONNX export](https://docs.pytorch.org/docs/stable/onnx.html)                                                                                                                                                | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [ONNX Runtime — threading configuration](https://onnxruntime.ai/docs/performance/tune-performance/threading.html)                                                                                                             | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [KataGo v1.18.1 — NNEvaluator](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/neuralnet/nneval.h)                                                                                                            | [ai/KATAGO_BACKEND.md](ai/KATAGO_BACKEND.md)             |
| [KataGo v1.18.1 — features and NNOutput](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/neuralnet/nninputs.h)                                                                                                 | [ai/KATAGO_BACKEND.md](ai/KATAGO_BACKEND.md)             |
| [KataGo v1.18.1 — BoardHistory and rules](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/game/boardhistory.h)                                                                                                  | [ai/KATAGO_BACKEND.md](ai/KATAGO_BACKEND.md)             |

## Reference Files and Local Dependencies

- [Original user-supplied MCTS code](references/HARD_AI_INPUT.md): reference for naming and PUCT/FPU (`cpuct = 1.5`, `fpuReduction = 0.2`, `19652`).
- [KataGo license](../src/AI/KATAGO_LICENSE.txt): retained at its current source location; the license is not Markdown.
- [CMake backend](../cmake/KataGoBackend.cmake): integration source and KataGo v1.18.1 version/SHA-256. Builds can download upstream or use explicitly supplied local source.
- `../DatasetGogame/AI/`: directory outside the repository used by the launcher to locate models/source/libraries; not part of the project tree and not copied into `docs/`.
- SplitMix64 notes inserted into the old log are preserved in the [archived README](archive/README-2026-10-20.md) and [PROMPT.md](PROMPT.md). That record did not include a source for verification; see [HardAI](ai/HARD_AI.md) for actual `mix64` use.
