# GoGame WebSocket server

Server C++/Qt 6 chạy console, độc lập với ứng dụng QML. Cần Qt Core, Network và WebSockets.

```bash
cmake -S server -B build/server
cmake --build build/server --parallel 3
./build/server/appserver --port 14509 --data-file server/data/state.json
```

Chạy các lệnh trên từ thư mục gốc dự án. Server nghe tại `ws://127.0.0.1:14509`. Nếu bỏ `--data-file`, mặc định là `data/state.json` tính từ thư mục đang chạy. File chưa có được tạo với nội dung `{}`. File đã có phải chứa JSON object hợp lệ; lỗi đọc hoặc JSON hỏng khiến server dừng và giữ nguyên file.

## Giao thức

Gửi text message chứa JSON object theo dạng `{ "type": "...", "payload": { ... } }`.

Đọc file:

```json
{"type":"ReadData","payload":{}}
```

Phản hồi:

```json
{"type":"Data","payload":{"data":{}}}
```

Ghi file:

```json
{"type":"WriteData","payload":{"data":{"board":[0,-1,1],"moveCount":3,"currentPlayer":1,"time":{"black":600,"white":590}}}}
```

`payload.data` phải là JSON object. Mỗi yêu cầu ghi **thay toàn bộ nội dung file**; dữ liệu ví dụ trên chỉ minh họa giao thức, chưa phải trạng thái bàn cờ 19×19 đầy đủ. Ghi thành công trả về:

```json
{"type":"DataWritten","payload":{"ok":true}}
```

Lỗi trả về dạng:

```json
{"type":"Error","payload":{"code":"InvalidPayload","message":"payload.data must be a JSON object"}}
```

Các mã lỗi: `InvalidJson`, `InvalidPayload`, `UnknownType`, `ReadFailed`, `WriteFailed`, `UnsupportedMessage`. Binary message không được hỗ trợ. Giới hạn message nhận là 1 MiB.

Server xử lý yêu cầu tuần tự và ghi qua `QSaveFile` để thay file nguyên tử. Các client cùng dùng một file; yêu cầu ghi sau cùng thành công quyết định nội dung. Chỉ chạy một server cho mỗi file dữ liệu. Server phục vụ local, có đăng nhập guest bằng nickname, ghép trận và luật cờ vây cơ bản. Chưa có TLS hoặc xác thực tài khoản bằng mật khẩu.

## Kết nối client hiện có

Client có biến môi trường `GOGAME_SERVER_URL`. Khi chạy client có thể đặt:

```bash
GOGAME_SERVER_URL=ws://127.0.0.1:14509 ./build/Desktop_Debug/appGoGame
```

Client tự kết nối lúc khởi động, mặc định dùng `ws://127.0.0.1:14509`. Màn hình đăng nhập hiển thị trạng thái/lỗi và có nút kết nối lại. Cần gửi `ReadData`/`WriteData` khi tích hợp đồng bộ dữ liệu. Server hỗ trợ AuthRequest dạng guest và MatchmakingStart/MatchmakingCancel. Đăng nhập bằng mật khẩu ở UI chưa được tích hợp với giao thức guest này.

## Kiểm tra

```bash
ctest --test-dir build/server --output-on-failure
```

Bài kiểm tra dùng port localhost 24509 và thư mục dữ liệu tạm, kiểm tra đọc/ghi qua WebSocket, hai client, dữ liệu tồn tại sau restart, từ chối message sai và bảo toàn file JSON hỏng. Có thể tắt build bài kiểm tra bằng `-DBUILD_TESTING=OFF`.

`main.qml` là file khung có sẵn; executable console không nạp file này.

## Class Server

`Server.h` khai báo API, `Server.cpp` triển khai; `main.cpp` đọc CLI và gọi `start`. Class nhận đường dẫn file khi khởi tạo, sở hữu WebSocket listener và các kết nối client.

- `start(port, error)`: chuẩn bị file JSON, mở listener localhost.
- `read(client, text)`: nhận text, parse JSON rồi gọi `handler`.
- `handler(client, message)`: phân loại `type`, xử lý yêu cầu.
- `send(client, message)`: gửi JSON object; trả số byte được chấp nhận gửi hoặc `-1` nếu client không kết nối.
- `jsonToString(json)`: chuyển object thành JSON string compact.
- `stringToJson(text, json, error)`: trả `true` khi text chứa JSON object hợp lệ; trả `false` và thông báo lỗi nếu không hợp lệ.
- `envelope(type, payload = {})`: trả object `{type, payload}`, payload mặc định là object rỗng.
- `readData(data, error)` / `writeData(data, error)`: đọc/ghi file JSON của server.

Ví dụ phản hồi trong handler:

```cpp
send(client, envelope("Data", {{"data", data}}));
send(client, envelope("Ready")); // {"type":"Ready","payload":{}}
```

Khi thêm loại message mới, bổ sung nhánh trong `Server::handler`. Auth guest, matchmaking và GameCommand được xử lý trong class Server.

## GameCommand cho cờ vây

Hai client gửi `AuthRequest` với `payload.nickname`, rồi `MatchmakingStart` cùng `payload.mode` để tạo trận. Server gửi `MatchFound` (đen = 0, trắng = 1) và `GameSnapshot`. Mỗi trận có bàn cờ và đồng hồ riêng, bắt đầu với 10 phút mỗi bên; timer kiểm tra tối đa mỗi 30 giây và đúng lúc hết thời gian còn lại.

Đặt quân:

```json
{"type":"GameCommand","payload":{"matchId":"ID_FROM_MATCH_FOUND","commandType":"Play","command":{"row":3,"col":3}}}
```

Tọa độ nguyên 0–18 theo dữ liệu bàn cờ, độc lập với hướng xoay UI. Server kiểm tra thành viên trận, lượt đi, ô trống, bắt nhóm hết khí, cấm tự sát và simple ko. Lệnh bị từ chối không đổi bàn/lượt; thời gian đã suy nghĩ vẫn bị trừ.

Bỏ lượt (tên `EndRound` được giữ để tương thích enum hiện có):

```json
{"type":"GameCommand","payload":{"matchId":"ID_FROM_MATCH_FOUND","commandType":"EndRound","command":{}}}
```

Đầu hàng (được phép ngoài lượt):

```json
{"type":"GameCommand","payload":{"matchId":"ID_FROM_MATCH_FOUND","commandType":"Concede","command":{}}}
```

Server phát `GameEvent` với `StonePlayed`, `Passed` hoặc `GameEnded`, và `GameSnapshot` gồm `board` (361 ô, -1/0/1), `moveCount`, `currentPlayerIndex`, `remainingMs`, `started`, `komi`. Hai lần bỏ lượt liên tiếp kết thúc trận, tính diện tích quân sống trên bàn cộng vùng trống chỉ tiếp giáp một màu; trắng có komi 6.5. Bản này chưa có thỏa thuận loại quân chết trước tính điểm hoặc positional/situational superko. Ngắt kết nối trong trận tính là thua. Trận đang chạy chỉ lưu trong bộ nhớ; file JSON ReadData/WriteData vẫn độc lập.

`GoMatch.h/.cpp` chứa luật, `Server::gameCommand` chứa kiểm tra/giao thức/đồng hồ. Các lệnh thẻ bài trong JS được thay bằng ba lệnh cờ vây trên. Board phía client hiện vẫn xử lý local; đồng bộ thao tác đặt quân từ UI qua GameCommand là bước tích hợp riêng.

CTest `goCommands` kiểm tra bắt quân, tự sát, ko, tách trạng thái giữa các trận, komi, guest auth, ghép trận, chặn client ngoài trận và đi sai lượt, tọa độ sai, broadcast/snapshot, hai lần bỏ lượt và đầu hàng ngoài lượt qua WebSocket thực.

## Snapshot theo từng người nhận

`sendSnapshot(socket, match)` gửi riêng cho mỗi socket thành viên; `sendSnapshot(match)` gọi cho cả hai. Giữ `board`, `moveCount`, `currentPlayerIndex`, `remainingMs`, `started`, `komi` và bổ sung:

- `stage`: `Action` hoặc `GameEnd`; `boardSize: 19`, `consecutivePasses`.
- `players`: `playerId`, `nickname`, `playerIndex`, `color`, `remainingTimeMs`.
- `self.playerIndex`: góc nhìn người nhận, 0 đen/1 trắng.
- `serverTimeEpochMs`, `actionDeadlineEpochMs`: mốc đồng bộ hiển thị; deadline là lúc tổng clock của người đang đi hết, không phải hạn 30 giây mỗi nước.
- `winnerIndex`, `endReason`: -1/chuỗi rỗng khi chơi, kết quả khi kết thúc. Clock cuối đóng băng, deadline cuối bằng 0.

Server dùng monotonic time để thực thi clock, epoch chỉ để UI hiển thị. Không gửi snapshot mỗi giây; client tự cập nhật countdown giữa các snapshot. Không có trường round/weather/character/card vì game là cờ vây. Xem [README gốc](../README.md) để biết sơ đồ tương tác, schema và các bước còn thiếu ở client.
