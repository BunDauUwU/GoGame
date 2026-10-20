# GoGame

Game cờ vây viết bằng **C++17 và Qt 6**, với client Qt Quick/QML và server WebSocket JSON. Có đấu hai người qua server, chơi offline với **Easy / Medium / Hard**, và **Sandbox** để tự điều khiển hai bên. Server quản lý luật, lượt và đồng hồ trận online; AI chạy ở client. Hard dùng MCTS/PUCT/FPU, có thể bật backend KataGo native để nạp model `.bin.gz`.

## Yêu cầu

CMake ≥ 3.16, compiler C++17 và Qt 6 (CMake yêu cầu ≥ 6.2) với Core, Gui, Quick/QML, Network, WebSockets, Sql, Concurrent và Multimedia. Cần Qt hỗ trợ `QWebSocket::errorOccurred` như mã nguồn hiện tại và plugin SQLite cho đăng nhập local. Nếu CMake chưa tìm thấy Qt, thêm `-DCMAKE_PREFIX_PATH=/duong/dan/toi/Qt` khi configure.

Mọi lệnh bên dưới chạy từ thư mục gốc dự án. Các ví dụ executable dùng đường dẫn Linux.

## Build và mở client

```bash
cmake -S . -B build/local -DBUILD_TESTING=OFF
cmake --build build/local --parallel 3
./build/local/appGoGame
```

Dùng `BUILD_TESTING=OFF` vì cây nguồn hiện chưa có thư mục `tests/` mà CMake tham chiếu. Có thể mở `CMakeLists.txt` bằng Qt Creator. Bản build mặc định không cần model KataGo; Hard dùng evaluator heuristic.

## Chạy server và đấu hai người

Mở một terminal riêng, chạy và giữ server hoạt động:

```bash
cmake -S server -B build/server
cmake --build build/server --parallel 3
./build/server/appserver --data-file server/data/state.json
```

Server nghe tại `ws://127.0.0.1:14509`; dừng bằng `Ctrl+C`. File JSON chưa có sẽ được tạo; file hỏng khiến server dừng và giữ nguyên dữ liệu. `state.json` phục vụ API đọc/ghi JSON, chưa phải file tự lưu các trận đang chơi. Có thể thêm `--port 14509` hoặc `--main-minutes 15`; mặc định mỗi bên 10 phút chính + 3 lần Byo-yomi × 20 giây.

Ở terminal khác, mở hai instance client, đăng ký/đăng nhập hai tài khoản local, rồi chọn **Find Match** ở cả hai để ghép trận. Server hiện chỉ nghe localhost. Đăng nhập SQLite ở client và danh tính guest nickname ở server là hai phần riêng.

Client mặc định dùng URL trên. Khi đổi cổng, đặt URL tương ứng trước khi mở client:

```bash
GOGAME_SERVER_URL=ws://127.0.0.1:14509 ./build/local/appGoGame
```

## Chơi AI và Sandbox

- **Play against AI → Easy / Medium / Hard**: chơi offline, người chơi cầm đen. Có 20 slot lưu AI; dùng nút tải ván ở menu để tiếp tục, chọn slot và xác nhận ghi đè khi đầy.
- **Sandbox**: điều khiển cả hai bên, hỗ trợ Reset, Undo, Redo, Save và Load. Slot `sandbox.json` riêng; Reset/Exit không tự ghi đè bản lưu thủ công.
- Mặc định dữ liệu ván ở `QStandardPaths::AppDataLocation/games`; biến môi trường `GOGAME_SAVE_DIR` cho phép đổi thư mục.
- Lần nhấn đầu tại ô hợp lệ hiện preview, lần thứ hai xác nhận. Nút **Fast mode** cho phép đặt ngay bằng một lần nhấn.

Không cần chạy server khi chơi AI hoặc Sandbox. Scorer hiện còn giới hạn trong phân xử quân chết/seki; nhãn độ khó chưa phải chứng nhận rank của AI.

## Chạy Hard với model KataGo

Cần model `.bin.gz` và dependencies backend tương ứng. Launcher build rồi mở **client**:

```bash
./tools/run-katago-hard.sh /duong/dan/model.bin.gz --ai-time-ms 1800 --ai-threads 4 --katago-duty 0.30
```

Sau khi mở, chọn **Play against AI → Hard**. Launcher mặc định dùng CUDA; các đường dẫn tự dò phụ thuộc máy phát triển. Không có NVIDIA, dùng backend CPU với model nhỏ và thư mục build riêng:

```bash
GOGAME_KATAGO_BACKEND=EIGEN GOGAME_KATAGO_BUILD="$PWD/build/katago-cpu" ./tools/run-katago-hard.sh /duong/dan/model-nho.bin.gz --ai-threads 2
```

Cách cài dependency, build thủ công và tùy chỉnh model ở [hướng dẫn KataGo](docs/ai/KATAGO_BACKEND.md). Bản build không bật backend sẽ báo lỗi nếu truyền `--katago-model`.

## Tài liệu dự án

Xem [cấu trúc và tài liệu tham khảo](docs/STRUCTUREANDREF.md), [nhật ký prompt / phản hồi / thay đổi](docs/PROMPT.md), [kiến trúc và giao thức](docs/ARCHITECTURE.md), [server](docs/SERVER.md), [kế hoạch phát triển](docs/ROADMAP.md) và [ghi chú thay đổi lịch sử](docs/CHANGELOG.md).
