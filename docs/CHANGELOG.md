# Ghi chú kỹ thuật và thay đổi lịch sử

Các phần dưới được chuyển từ README trước khi tách tài liệu ngày **20/10/2026**, giữ nguyên số liệu và kết quả đã ghi. Đây là lịch sử, không phải trạng thái triển khai mới nhất; riêng cây file hiện tại xem [STRUCTUREANDREF.md](STRUCTUREANDREF.md), usage xem [README](../README.md), và lịch sử từng yêu cầu xem [PROMPT.md](PROMPT.md).

## Những thông tin cũ cần đọc theo thời điểm

- Bảng trạng thái cũ ghi Easy/Hard chưa có; hiện cả hai đã có trong mã/menu. KataGo native cũng đã có, xem [hướng dẫn hiện tại](ai/KATAGO_BACKEND.md).
- Tests và `scripts/server_stats.py` được nhắc dưới đây không còn trong cây nguồn hiện tại. Không dùng các lệnh/số đo cũ như kết quả kiểm tra ở lần cập nhật tài liệu này.
- Số dòng/ký tự server là số đo ngày 13/10/2026, không phải thống kê ngày 20/10/2026 hay số ký tự do riêng người dùng viết.
- Giới hạn Undo/Redo cũ là 10; giá trị hiện tại trong `GameManager::AIHistoryLimit` là 1000. AI có 20 slot và Sandbox một slot riêng.
- Kết quả timing là phép đo đã ghi trên cấu hình/vị trí thử cụ thể; không bảo đảm thời gian trên máy khác.

## Trạng thái hiện tại và việc còn lại

| Phần          | Đã có                                                                                                                   | Còn thiếu                                                                                                            |
| ------------- | ----------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------- |
| Server        | Guest nickname, queue theo mode, nhiều trận riêng, luật cơ bản, xác nhận lượt/thành viên, timer và snapshot từng người. | Tài khoản server thật, TLS/phân quyền, khôi phục trận sau reconnect.                                                 |
| Client online | MatchFound, gửi Play/EndRound/Concede, chỉ áp dụng bàn/lượt sau snapshot, pending lock, lỗi và kết quả trận.            | Kiểm thử trực tiếp toàn bộ thao tác UI; hiện tên/avatar vẫn có giá trị mặc định khi chạy local.                      |
| Luật          | Ăn quân, tự sát, simple ko, hai pass tính đất + quân bắt theo công thức Nhật với komi 6,5.                              | Superko, xử lý/thỏa thuận quân chết, xác nhận quân chết/seki để hoàn thiện tính điểm Nhật.                           |
| Đồng hồ       | Byo-yomi do server quản lý, hiển thị clock hai bên, timer gửi snapshot.                                                 | Tùy chọn time control trong UI; hiện 10/15 phút chọn qua CLI.                                                        |
| Auth          | SQLite local, gửi guest nickname khi login và kết nối lại.                                                              | Server chưa xác thực password/tài khoản local; logout local không hủy danh tính guest trên server.                   |
| Settings      | Chọn background qua AppState trong phiên.                                                                               | Âm thanh, preview toggle có UI/lưu lựa chọn, avatar/frame/title/level/rank. Preview vẫn là property của MatchScreen. |
| Dữ liệu       | ReadData/WriteData nguyên tử bằng QSaveFile.                                                                            | Match chỉ ở RAM; chưa tự lưu match, replay 5 ván hoặc phục hồi sau restart. API file vẫn chưa phân quyền.            |
| AI / âm thanh | Có kế hoạch bên dưới và tài nguyên hình/font.                                                                           | Medium đã có trong `src/AI/`; Easy/Hard và nhạc/âm thanh đặt quân/báo lỗi chưa triển khai.                           |

## Những thay đổi trong lần dọn project này

Đối chiếu với working tree ngay trước tác vụ, **xóa 22 file không được luồng hiện tại sử dụng**, tổng **403 dòng / 13.997 ký tự**. Các thay đổi online/Byo-yomi đã có trước tác vụ được giữ nguyên.

- Bỏ Card, CharacterCard, SupportCard, ElementPointBar và CountdownClock cũ; giữ GameClock và PlayerPanel đang dùng.
- Bỏ Loading/Popup, GameIcon/GameImage, GameBackground và GamePanel ở layout. Login dùng duy nhất GamePanel trong controls, loại bỏ trùng tên.
- Bỏ Constants không có consumer, GamemodeScreen chưa nối vào navigation, các file AI/Board/UI/Enums.cpp rỗng hoặc chỉ chứa khung.
- Bỏ `server/main.qml` và bản test cũ `server/test/Intergration.cpp`; giữ đủ bốn test thực trong `server/tests/`.
- Gỡ file đã xóa khỏi CMake và import layout khỏi các màn hình; bỏ dependency Qt Multimedia vì không có code audio hiện tại, bỏ tìm Quick lặp và resource SQLite root không được đọc. Dữ liệu tài khoản ở AppDataLocation giữ nguyên.
- Bỏ object AssetManager khởi tạo thừa ở main (QML singleton tự quản lý), macro Debug không dùng, include Multimedia thừa và mapping font không có consumer. Typography vẫn dùng font hiện có.
- Bỏ SubmitDeck, GameEventType và GameStage của template; EnumUtils chỉ giữ hai chuyển enum → string đang dùng. Bỏ các parser enum/helper protocol không có consumer hoặc chỉ khai báo: ErrorPayload, SubmitDeck, ChooseActiveCharacter, SwitchCharacter, UseSkill, PlayCard, EndRound wrapper, hai helper JSON nội bộ.
- Matchmaking chỉ nhận `mode`, không còn `deckId`. Play helper chỉ nhận `matchId`, `row`, `col`. Cập nhật mọi caller và test Players theo API này.
- Bỏ GameManager `play()` không làm gì, `endRound()` wrapper không được gọi, getter board_state/opponentName và signal/stage state không có consumer. Giữ snapshot, interaction/board/turn notifications, pass_turn và submit đang dùng.
- Bỏ Server `stop`/`address` không có caller và selection timeout của template; destructor vẫn dừng timer/socket. Không đổi protocol snapshot của server.
- Dọn design token Motion/Radius/Spacing/Theme không được component hiện tại dùng; đổi chữ TCG ở menu thành GoGame và `deckError` thành `matchmakingError`. Bỏ nút AI chưa có hành động; kế hoạch AI vẫn giữ bên dưới.
- Bỏ tạo ba deck JSON khi đăng ký; ghi `{}` vào cột SQLite `decks` cũ để tương thích database có sẵn. Không xóa dữ liệu hay migrate schema trong tác vụ này.
- Thêm `scripts/server_stats.py` để tái đếm dòng/ký tự. Giữ ảnh/avatar/font hiện có làm tài nguyên dự án và kế hoạch customization.

Các file đã xóa:

- `qml/Components/cards/Card.qml`
- `qml/Components/cards/CharacterCard.qml`
- `qml/Components/cards/SupportCard.qml`
- `qml/Components/battle/ElementPointBar.qml`
- `qml/Components/battle/CountdownClock.qml`
- `qml/Components/common/Loading.qml`
- `qml/Components/common/Popup.qml`
- `qml/Components/controls/GameIcon.qml`
- `qml/Components/controls/GameImage.qml`
- `qml/Components/layout/GameBackground.qml`
- `qml/Components/layout/GamePanel.qml`
- `qml/Core/Constants.qml`
- `qml/Screens/GamemodeScreen.qml`
- `src/Core/AI.cpp`
- `src/Core/AI.h`
- `src/Core/Enums.cpp`
- `src/Models/Board.cpp`
- `src/Models/Board.h`
- `src/Models/UI.cpp`
- `src/Models/UI.h`
- `server/main.qml`
- `server/test/Intergration.cpp`

## Dòng và ký tự của server

Đếm **mã server hiện tại**, không suy đoán số ký tự do riêng chủ dự án gõ: lịch sử có thay đổi của người và AI, Git author không chứng minh tác giả của từng ký tự. Mỗi dòng vật lý tính bằng `splitlines()` (kể cả dòng trống/comment); ký tự tính Unicode code points sau decode UTF-8, gồm khoảng trắng và xuống dòng, không phải byte. Không tính README, JSON dữ liệu, client code, file build hoặc script thống kê.

| File / phạm vi              |      Dòng |      Ký tự |
| --------------------------- | --------: | ---------: |
| `server/main.cpp`           |        42 |      1,665 |
| `server/Server.h`           |        77 |      2,777 |
| `server/Server.cpp`         |       580 |     18,973 |
| `server/GoMatch.h`          |        20 |        428 |
| `server/GoMatch.cpp`        |        84 |      2,905 |
| `server/ByoYomiClock.h`     |        38 |      1,378 |
| **Runtime .cpp/.h**         |   **841** | **28,126** |
| Tests (4 file .cpp)         |       395 |     21,815 |
| server/CMakeLists.txt       |        41 |      1,983 |
| **Runtime + tests + CMake** | **1,277** | **51,924** |

Runtime trước dọn: 885 dòng / 29.272 ký tự; sau dọn giảm 44 dòng / 1.146 ký tự. Đây là chênh lệch đầu/cuối tác vụ, không so với HEAD (working tree đã có thay đổi trước đó).

Chạy lại thống kê khi server thay đổi:

```bash
python3 scripts/server_stats.py
```

## Chế độ mô phỏng và lưu ván AI

Chọn **Sandbox** trong menu để điều khiển cả đen và trắng theo lượt, không xoay bàn. Trong trận có Reset, Undo, Redo, Save, Load và Exit. Sandbox chỉ lưu một bản `sandbox.json`; Reset không xóa bản Save, Exit không tự ghi đè bản sandbox đã lưu.

Trận Medium có **10 lần Undo và 10 lần Redo** (đổi `GameManager::AIHistoryLimit` trong `src/Core/Game.h`). Undo lùi cả cặp nước người chơi–AI; đi nhánh mới xóa Redo cũ. Có tối đa **20 slot JSON**. Slot đang chơi được auto-save sau mỗi nước và khi thoát; menu **Load AI battle** khôi phục đúng lượt, kể cả AI đang chờ đi. Khi đầy, chọn slot ghi đè để tạo ván mới.

Engine GoMatch dùng DSU chung cho client, server, AI và tính vùng trống; AI đọc cache nhóm/khí, dùng Minimax có Alpha-Beta và vùng ứng viên cục bộ. Điều phối AI tách tại `src/AI/AIGame.cpp`. Phép đo bàn trống sau tối ưu: depth 3 / 586 nodes / khoảng 61 ms, trong khi phép đo trước hoàn thành depth 2 ở 1000 ms; vùng ứng viên nay được giới hạn nên không phải so sánh trên cùng cây tìm kiếm. Trần 1 giây vẫn giữ cho thế khó.

Thông báo trong trận gồm ăn quân, ko/tái bắt bị cấm, phản ứng ko gây atari và điểm khi hai bên pass. JSON lưu lịch sử, cursor, bàn hiện tại/trước đó, ko, lượt, số nước, quota và kết quả. Load kiểm tra/replay dữ liệu trước khi thay trạng thái. Chi tiết vị trí file, cấu trúc và giới hạn luật: [src/AI/README.md](ai/IMPLEMENTATION_GUIDE.md).

## Tách client AI, DataManager và huấn luyện SGF

AI nằm hoàn toàn trong `src/AI/`, chỉ được build ở client. Luật DSU chung chuyển sang `src/Rules/`; client và server cùng biên dịch source luật, server không build AI hoặc phụ thuộc Qt Concurrent. AI, DataManager và luật được khai báo trực tiếp trong CMake gốc của client; chỉ server có CMake riêng. Phần JSON save/load/slots/replay được tách khỏi GameManager sang `src/Managers/DataManager.h/.cpp`, giữ nguyên API nút lưu/tải.

File `src/AI/trainAI.py` có inspect/prepare SGF, huấn luyện Transformer ~6 tỷ tham số (5.974.470.659; policy 362 + value), resume checkpoint và export ONNX external data. Cấu hình `resnet6` dùng để thử pipeline nhỏ. Chưa có trọng số 6B đã train vì chưa có corpus SGF thật. Pipeline train/resume/export đã được kiểm tra bằng model nhỏ trong venv tạm; cấu hình 6B được đếm tham số trên meta device. Hướng dẫn để tự triển khai Minimax, Alpha-Beta, MCTS và đọc neuron: [src/AI/IMPLEMENTATION_GUIDE.md](ai/IMPLEMENTATION_GUIDE.md).


## Hard AI và slot Sandbox (19/10/2026)

Đã bật **Play against AI → Hard**, tích hợp class `HardAI` theo MCTS được cung cấp, tách type chung, giữ generation/cancellation khi Undo/Load/Exit. Save AI giữ lại độ khó Hard/Medium. Sandbox có slot riêng `sandbox.json` và nút Load riêng trong menu, không chiếm 20 slot AI.

Xem [HardAI: tích hợp, model adapter và tối ưu](ai/HARD_AI.md). Hard hiện dùng heuristic mặc định; public `HardAI::Evaluator`/`GameManager::setHardEvaluator()` là chỗ gắn backend policy/value. Chưa tự nạp model KataGo`.bin.gz`; không coi nhãn Hard là chứng nhận trình độ hoặc bảo đảm mạnh hơn Medium.


## Hard với model KataGo (2026-10-19)

Chạy `./tools/run-katago-hard.sh`, sau đó chọn Hard trong menu AI. Backend C++ nạp model `.bin.gz` một lần; MCTS của GoGame tiếp tục chọn nhánh/nước đi, có concurrent leaves, batch inference và pacing mặc định 0.30 để chừa tải GPU. Hướng dẫn CPU/CUDA, lịch sử save tương thích và các giới hạn về chấm điểm/thời gian nằm ở [KATAGO_BACKEND.md](ai/KATAGO_BACKEND.md). Đây chưa phải sức mạnh đã được kiểm chứng ngang engine KataGo.
