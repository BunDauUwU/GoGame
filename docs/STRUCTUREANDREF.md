# Project Structure and References

## Documents

| File                                                       | Contents                                                                            |
| ---------------------------------------------------------- | ----------------------------------------------------------------------------------- |
| [README.md](../README.md)                                  | Overview.                                                                           |
| [AI_Declaration.md](AI_Declaration.md)                     | Team AI usage declaration.                                                          |
| [PROMPT.md](PROMPT.md)                                     | Full prompts, recoverable responses, changes, and checks for each recorded request. |
| [STRUCTUREANDREF.md](STRUCTUREANDREF.md)                   | Project tree and reference catalog.                                                 |
| [ARCHITECTURE.md](ARCHITECTURE.md)                         | Client/server diagrams, WebSocket flow, and snapshot schema.                        |
| [SERVER.md](SERVER.md)                                     | Server protocol, commands, snapshots, clocks, and JSON API.                         |
| [references/HARD_AI_INPUT.md](references/HARD_AI_INPUT.md) | Complete user-supplied MCTS code for the HardAI request.                            |

## File Tree

```text
GoGame/
├── .qtcreator/  [collapsed: generated content or local configuration]
├── assets/
│   ├── background/
│   │   ├── ...
│   ├── fonts/
│   │   └── ...
│   ├── playerAvatar/
│   │   ├── ...
│   ├── sounds/
│   │   ├── ...
│   └── stones/
│       ├── ...
├── build/  [collapsed: generated content or local configuration]
├── cmake/
│   └── KataGoBackend.cmake
├── docs/
│   ├── references/
│   │   └── HARD_AI_INPUT.md
│   ├── AI_Declaration.md
│   ├── ARCHITECTURE.md
│   ├── PROMPT.md
│   ├── SERVER.md
│   ├── STRUCTUREANDREF.md
│   └── WORK_DIVISION.md
├── qml/
│   ├── Components/
│   │   ├── battle/
│   │   │   ├── GameClock.qml
│   │   │   └── PlayerPanel.qml
│   │   └── controls/
│   │       ├── GameButton.qml
│   │       ├── GamePanel.qml
│   │       ├── GameText.qml
│   │       ├── SaveGameDialog.qml
│   │       └── SaveSlotTable.qml
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
│   │   ├── KataGoEvaluator.cpp
│   │   ├── KataGoEvaluator.h
│   │   ├── KataGoNative.cpp
│   │   ├── KataGoNative.h
│   │   ├── MediumAI.cpp
│   │   ├── MediumAI.h
│   │   ├── SearchTypes.h
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
    ├── b10c384h6nbttflrs.bin
│   └── run-katago-hard.sh
├── .gitignore
├── CMakeLists.txt
├── main.cpp
├── main.qml
└── README.md
```

## References

| Reference                                                                                                                                                                                                                         | Recorded source / context                    |
| --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------- |
| [Introduction to Artificial Intelligence Report — Studocu](https://www.studocu.vn/vn/document/dai-hoc-dien-luc/dien-luc-university/bao-cao-nhap-mon-tri-tue-nhan-tao/88382780?sid=a353be05-df77-4401-bae7-4597fba4fd601791775595) |                                              |
| [Caro Report — equation reference](https://www.scribd.com/document/434712509/BAO-CAO)                                                                                                                                             |                                              |
| [Implementing Go — Modern Descartes](https://www.moderndescartes.com/essays/implementing_go/)                                                                                                                                     |                                              |
| [Atari Go — CodinGame](https://www.codingame.com/ide/puzzle/atari-go)                                                                                                                                                             |                                              |
| [Go Minimax — BaseGo/core](https://github.com/HosseinBakand/GO-Game-Min-Max-/tree/main/BaseGo/core)                                                                                                                               |                                              |
| [Go Monte Carlo — Java](https://github.com/GTnull/Go/blob/master/version_2.0/src/gogame/monteCarloMethod.java)                                                                                                                    |                                              |
| [5×5 Go AI Bot — Q-learning](https://github.com/27rg5/AI-bot-for-GO-game)                                                                                                                                                         |                                              |
| [GoByGo — Monte-Carlo Based Computer Player for the Game of Go](https://durhamgo.club/GoByGo.pdf)                                                                                                                                 |                                              |
| [AlphaGo — Nature paper (historically recorded URL)](https://deepmind-media.storage.googleapis.com/alphago/AlphaGoNaturePaper.pdf)                                                                                                |                                              |
| [Nihon Ki-in — Japanese rules](https://www.nihonkiin.or.jp/match/kiyaku/zenbun.htm)                                                                                                                                               | [SERVER.md](SERVER.md)                       |
| [KataGo — pass-alive territory definitions](https://lightvector.github.io/KataGo/rules.html#additional-definitions)                                                                                                               | [SERVER.md](SERVER.md)                       |
| [KataGo — Analysis Engine protocol](https://github.com/lightvector/KataGo/blob/master/docs/Analysis_Engine.md)                                                                                                                    | [ai/HARD_AI.md](ai/HARD_AI.md)               |
| [AlphaGo — Nature paper](https://storage.googleapis.com/deepmind-media/alphago/AlphaGoNaturePaper.pdf)                                                                                                                            |                                              |
| [OpenSpiel — AlphaZero](https://github.com/google-deepmind/open_spiel/blob/master/docs/alpha_zero.md)                                                                                                                             |                                              |
| [PyTorch — installation](https://pytorch.org/get-started/locally/)                                                                                                                                                                |                                              |
| [SGF FF4 — specification](https://www.red-bean.com/sgf/)                                                                                                                                                                          |                                              |
| [SGF FF4 — property index](https://www.red-bean.com/sgf/proplist_t.html)                                                                                                                                                          |                                              |
| [PyTorch — scaled dot-product attention](https://docs.pytorch.org/docs/stable/generated/torch.nn.functional.scaled_dot_product_attention.html)                                                                                    |                                              |
| [ONNX Runtime — external data for large models](https://onnxruntime.ai/docs/tutorials/web/large-models.html#external-data)                                                                                                        |                                              |
| [ONNX Runtime — C++](https://onnxruntime.ai/docs/get-started/with-cpp)                                                                                                                                                            |                                              |
| [PyTorch — ONNX export](https://docs.pytorch.org/docs/stable/onnx.html)                                                                                                                                                           |                                              |
| [ONNX Runtime — threading configuration](https://onnxruntime.ai/docs/performance/tune-performance/threading.html)                                                                                                                 |                                              |
| [KataGo v1.18.1 — NNEvaluator](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/neuralnet/nneval.h)                                                                                                                         | [ai/KATAGO_BACKEND.md](ai/KATAGO_BACKEND.md) |
| [KataGo v1.18.1 — features and NNOutput](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/neuralnet/nninputs.h)                                                                                                             | [ai/KATAGO_BACKEND.md](ai/KATAGO_BACKEND.md) |
| [KataGo v1.18.1 — BoardHistory and rules](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/game/boardhistory.h)                                                                                                             | [ai/KATAGO_BACKEND.md](ai/KATAGO_BACKEND.md) |

## Reference Files and Local Dependencies

- [Original user-supplied MCTS code](references/HARD_AI_INPUT.md): reference for naming and PUCT/FPU (`cpuct = 1.5`, `fpuReduction = 0.2`, `19652`).
- [CMake backend](../cmake/KataGoBackend.cmake): integration source and KataGo v1.18.1 version/SHA-256. Builds can download upstream or use explicitly supplied local source.
