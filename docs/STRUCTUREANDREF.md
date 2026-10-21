# Cấu trúc dự án và tài liệu tham khảo

Đối chiếu trực tiếp hệ thống file ngày **21/10/2026**, sau khi tách tài liệu. Các đường dẫn source và lệnh trong tài liệu tính từ gốc dự án; đường dẫn Markdown là tương đối với file đang đọc.

## Tài liệu nằm ở đâu?

| File                                                         | Nội dung                                                                                    |
| ------------------------------------------------------------ | ------------------------------------------------------------------------------------------- |
| [README.md](../README.md)                                    | Tổng quan và usage: build, client, server, AI, Sandbox, KataGo.                             |
| [AI_Declaration.md](AI_Declaration.md) | Bản khai sử dụng AI theo mẫu nộp nhóm, attribution và xác nhận từng thành viên. |
| [PROMPT.md](PROMPT.md)                                       | Bảng prompt đầy đủ, phản hồi còn khôi phục được, thay đổi và kiểm tra từng lượt.            |
| [STRUCTUREANDREF.md](STRUCTUREANDREF.md)                     | Cây dự án và danh mục nguồn tham khảo.                                                      |
| [ARCHITECTURE.md](ARCHITECTURE.md)                           | Sơ đồ client/server, luồng WebSocket và schema snapshot.                                    |
| [SERVER.md](SERVER.md)                                       | Giao thức, command, snapshot, đồng hồ và API JSON của server; chuyển từ `server/README.md`. |
| [ROADMAP.md](ROADMAP.md)                                     | Kế hoạch và ghi chú cần làm sau (removed because no more implement).                        |
| [ai/KATAGO_BACKEND.md](ai/KATAGO_BACKEND.md)                 | Build/chạy backend KataGo native, CPU/CUDA và giới hạn.                                     |
| [references/HARD_AI_INPUT.md](references/HARD_AI_INPUT.md)   | Toàn bộ mã MCTS người dùng đính kèm cho yêu cầu HardAI.                                     |
| [archive/README-2026-10-20.md](archive/README-2026-10-20.md) | README nguyên trạng trước khi tách; đường dẫn/trạng thái bên trong là lịch sử.              |

## Cây file hiện tại

Liệt kê mã nguồn, tài nguyên, tài liệu và các thư mục rỗng đang có. Các thư mục build, môi trường Python, cache, Git và cấu hình cục bộ được thu gọn; tài liệu/thư viện do dependency tạo trong đó không thuộc bộ Markdown của dự án. Dữ liệu runtime trong `server/data/` có thể bị Git ignore và thay đổi theo máy. Không giải nén gói KataGo `.zip` để liệt kê nội dung.

```text
GoGame/
├── .agents/  [thu gọn: nội dung sinh tự động hoặc cấu hình cục bộ]
├── .aws/  [thu gọn: nội dung sinh tự động hoặc cấu hình cục bộ]
├── .cache/  [thu gọn: nội dung sinh tự động hoặc cấu hình cục bộ]
├── .codex/  [thu gọn: nội dung sinh tự động hoặc cấu hình cục bộ]
├── .git/  [thu gọn: nội dung sinh tự động hoặc cấu hình cục bộ]
├── .qtcreator/  [thu gọn: nội dung sinh tự động hoặc cấu hình cục bộ]
├── .venv-ai/  [thu gọn: nội dung sinh tự động hoặc cấu hình cục bộ]
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
├── build/  [thu gọn: nội dung sinh tự động hoặc cấu hình cục bộ]
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
│   │   ├── __pycache__/  [thu gọn: nội dung sinh tự động hoặc cấu hình cục bộ]
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

## Vai trò các phần chính

| Vị trí                                 | Vai trò                                                                                                                                      |
| -------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------- |
| `main.cpp`, `main.qml`, `qml/`         | Khởi động client, context QML, menu, màn hình đấu và components.                                                                             |
| `src/Core/Game.*`, `src/AI/AIGame.cpp` | Trạng thái game, điều phối chế độ, worker AI và hủy kết quả cũ.                                                                              |
| `src/Rules/GoMatch.*`                  | Luật dùng chung, bàn cờ, nhóm/khí, lịch sử và chấm điểm.                                                                                     |
| `src/Managers/`                        | Auth local, matchmaking, tài nguyên và JSON save/load.                                                                                       |
| `src/Network/`                         | Kết nối WebSocket và đóng gói giao thức client.                                                                                              |
| `server/`                              | Console server, queue, match, đồng hồ và API đọc/ghi JSON. CMake build luật từ `src/Rules/`; các file `server/GoMatch.*` cũ vẫn có trên đĩa. |
| `src/AI/`                              | EasyAI, MediumAI, HardAI, adapter KataGo và training SGF. Hard có board/groups riêng cho tìm kiếm.                                           |
| `cmake/KataGoBackend.cmake`            | Tích hợp backend native và bản sao nguồn KataGo.                                                                                             |
| `tools/run-katago-hard.sh`             | Build và chạy client với model KataGo.                                                                                                       |
| `assets/`                              | Hình nền, avatar, font, quân cờ, nhạc và âm thanh.                                                                                           |
| `docs/`                                | Toàn bộ Markdown do dự án quản lý, trừ README gốc.                                                                                           |

Cây nguồn hiện không có `tests/` hoặc `scripts/server_stats.py`; các nhắc đến trong nhật ký/bản lưu là lịch sử. Không thay đổi file code hoặc dọn file dư trong lần tách tài liệu này.

## Tài liệu tham khảo

Danh mục được gom từ các tài liệu sẵn có, không phải kết quả đánh giá lại nội dung hoặc kiểm tra khả năng truy cập các trang. Cột nguồn cho biết nơi tài liệu đã được nhắc đến; URL được giữ nguyên. Báo cáo cờ Caro, Q-learning 5×5 và bài Go 9×9 phục vụ tham khảo, không chứng minh hiệu năng AI GoGame 19×19.

| Tài liệu                                                                                                                                                                                                             | Nguồn đã ghi / ngữ cảnh                                  |
| -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------- |
| [Báo cáo nhập môn trí tuệ nhân tạo — Studocu](https://www.studocu.vn/vn/document/dai-hoc-dien-luc/dien-luc-university/bao-cao-nhap-mon-tri-tue-nhan-tao/88382780?sid=a353be05-df77-4401-bae7-4597fba4fd601791775595) |                                                          |
| [Báo cáo cờ Caro — tham khảo phương trình](https://www.scribd.com/document/434712509/BAO-CAO)                                                                                                                        |                                                          |
| [Implementing Go — Modern Descartes](https://www.moderndescartes.com/essays/implementing_go/)                                                                                                                        |                                                          |
| [Atari Go — CodinGame](https://www.codingame.com/ide/puzzle/atari-go)                                                                                                                                                |                                                          |
| [Go Minimax — BaseGo/core](https://github.com/HosseinBakand/GO-Game-Min-Max-/tree/main/BaseGo/core)                                                                                                                  |                                                          |
| [Go Monte Carlo — Java](https://github.com/GTnull/Go/blob/master/version_2.0/src/gogame/monteCarloMethod.java)                                                                                                       |                                                          |
| [AI bot cho Go 5×5 — Q-learning](https://github.com/27rg5/AI-bot-for-GO-game)                                                                                                                                        |                                                          |
| [GoByGo — Monte-Carlo Based Computer Player for the Game of Go](https://durhamgo.club/GoByGo.pdf)                                                                                                                    |                                                          |
| [AlphaGo — Nature paper (đường dẫn lưu cũ)](https://deepmind-media.storage.googleapis.com/alphago/AlphaGoNaturePaper.pdf)                                                                                            |                                                          |
| [Nihon Ki-in — luật Nhật](https://www.nihonkiin.or.jp/match/kiyaku/zenbun.htm)                                                                                                                                       | [SERVER.md](SERVER.md)                                   |
| [KataGo — định nghĩa pass-alive territory](https://lightvector.github.io/KataGo/rules.html#additional-definitions)                                                                                                   | [SERVER.md](SERVER.md)                                   |
| [KataGo — giao thức Analysis Engine](https://github.com/lightvector/KataGo/blob/master/docs/Analysis_Engine.md)                                                                                                      | [ai/HARD_AI.md](ai/HARD_AI.md)                           |
| [AlphaGo — Nature paper](https://storage.googleapis.com/deepmind-media/alphago/AlphaGoNaturePaper.pdf)                                                                                                               | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [OpenSpiel — AlphaZero](https://github.com/google-deepmind/open_spiel/blob/master/docs/alpha_zero.md)                                                                                                                | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [PyTorch — cài đặt](https://pytorch.org/get-started/locally/)                                                                                                                                                        | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [SGF FF4 — đặc tả](https://www.red-bean.com/sgf/)                                                                                                                                                                    | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [SGF FF4 — danh mục thuộc tính](https://www.red-bean.com/sgf/proplist_t.html)                                                                                                                                        | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [PyTorch — scaled dot-product attention](https://docs.pytorch.org/docs/stable/generated/torch.nn.functional.scaled_dot_product_attention.html)                                                                       | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [ONNX Runtime — external data cho model lớn](https://onnxruntime.ai/docs/tutorials/web/large-models.html#external-data)                                                                                              | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [ONNX Runtime — C++](https://onnxruntime.ai/docs/get-started/with-cpp)                                                                                                                                               | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [PyTorch — xuất ONNX](https://docs.pytorch.org/docs/stable/onnx.html)                                                                                                                                                | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [ONNX Runtime — cấu hình luồng](https://onnxruntime.ai/docs/performance/tune-performance/threading.html)                                                                                                             | [ai/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md) |
| [KataGo v1.18.1 — NNEvaluator](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/neuralnet/nneval.h)                                                                                                            | [ai/KATAGO_BACKEND.md](ai/KATAGO_BACKEND.md)             |
| [KataGo v1.18.1 — features và NNOutput](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/neuralnet/nninputs.h)                                                                                                 | [ai/KATAGO_BACKEND.md](ai/KATAGO_BACKEND.md)             |
| [KataGo v1.18.1 — BoardHistory và luật](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/game/boardhistory.h)                                                                                                  | [ai/KATAGO_BACKEND.md](ai/KATAGO_BACKEND.md)             |

## Tệp tham khảo và phụ thuộc cục bộ

- [Mã MCTS gốc người dùng gửi](references/HARD_AI_INPUT.md): nguồn đối chiếu cách đặt tên và PUCT/FPU (`cpuct = 1.5`, `fpuReduction = 0.2`, `19652`).
- [Giấy phép KataGo](../src/AI/KATAGO_LICENSE.txt): giữ ở vị trí source hiện tại; giấy phép không phải Markdown.
- [CMake backend](../cmake/KataGoBackend.cmake): nguồn tích hợp và phiên bản KataGo v1.18.1 cùng SHA-256. Build có thể tải upstream hoặc dùng bản nguồn cục bộ được chỉ định.
- `../DatasetGogame/AI/`: thư mục ngoài repository được launcher dùng để tìm model/nguồn/thư viện; không thuộc cây dự án và không được sao chép vào `docs/`.
- Ghi chú SplitMix64 đã được chèn vào bản nhật ký cũ được bảo toàn trong [bản README lưu](archive/README-2026-10-20.md) và [PROMPT.md](PROMPT.md). Bản ghi đó chưa kèm nguồn gốc để xác minh; phần sử dụng `mix64` thực tế xem [HardAI](ai/HARD_AI.md).
