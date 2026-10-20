# Kế hoạch phát triển

Chuyển từ README cũ ngày **20/10/2026**; các mục phía dưới giữ ý tưởng và mốc tiến độ từng ghi, có thể chứa đánh dấu cũ. Bản nguyên trạng nằm ở [kho lưu](archive/README-2026-10-20.md). Phần cập nhật này được ưu tiên khi mô tả trạng thái hiện tại.

## Đối chiếu với mã hiện tại

- Menu đã có Easy, Medium và Hard; mục 11 phía dưới là thiết kế ban đầu. Hard hiện dùng PUCT/FPU với cache, arena và root reuse; có thể nạp model KataGo native, không dùng RAVE mặc định. Xem [HardAI](ai/HARD_AI.md) và [KataGo](ai/KATAGO_BACKEND.md).
- Đồng hồ server và hai đồng hồ client đã có; tiếp tục hoàn thiện luồng multiplayer và các tùy chọn điều khiển thời gian.
- Báo lỗi vị trí đã có, **bỏ yêu cầu khoanh đỏ**; âm thanh báo lỗi vẫn là việc cần làm. Đã có tài nguyên nhạc/đặt quân và lời gọi âm thanh trong MatchScreen.
- Preview hai lần nhấn và nút Fast mode đã có; tùy chọn/lưu cấu hình preview trong Settings vẫn cần hoàn thiện.
- Giữ ý tưởng thu nhỏ layout bàn cờ để thêm chế độ chơi; animation ăn quân làm sau.
- AI có 20 slot; Sandbox dùng một slot riêng. Replay tối đa 5 ván và đánh giá nước đi vẫn là kế hoạch riêng.
- Ban đầu yêu cầu ưu tiên luật Trung; code hiện dùng simple ko, territory + prisoners và komi 6.5. Chọn/hoàn thiện luật Trung hoặc Nhật, superko, quân chết và seki vẫn cần quyết định; không xem scorer hiện tại là bộ luật hoàn chỉnh.
- “6-block” và Transformer khoảng 6 tỷ tham số là hai cấu hình khác nhau trong hướng nghiên cứu; chưa có trọng số tự huấn luyện 6B đi kèm. Backend KataGo hiện đọc model có sẵn; không đồng nghĩa đã hoàn thành việc tự xây mạng 6-block.

## Kế hoạch đã ghi trước khi tách tài liệu

### 1. Hoàn thiện MatchScreen.qml trước

- [ ] Ưu tiên `qml/Screens/MatchScreen.qml`.
- [x] Hoàn thiện UI bàn đấu: ô hiện tên người chơi và avatar.
- [x] Hoàn thành logic đồng hồ đếm ngược phía client.
- [x] Server quản lý thời gian mỗi người chơi và gửi deadline trong snapshot.
- [x] Nối snapshot và đồng hồ server vào UI client bằng GameClock.
- [x] Hiển thị đồng hồ của đối thủ.
- [ ] Cân nhắc thu nhỏ layout bàn cờ để có thêm không gian cho chế độ chơi; đây là ý tưởng có thể làm sau.

### 2. Đặt quân bằng hai lần nhấn

- [x] Lần đầu tại vị trí hợp lệ: chỉ hiện quân cờ mờ, nhấp nháy (preview).
- [x] Lần thứ hai tại cùng vị trí: xác nhận đặt quân.
- [ ] Cho phép bật/tắt chế độ này trong settings.

### 3. Phản hồi khi đặt sai vị trí

- [x] Hiện chữ báo lỗi ở trên cùng màn hình.
- [x] Không cho đặt quân, kể cả preview, ở vị trí không hợp lệ.
- [ ] Phát âm thanh báo lỗi — phần còn thiếu duy nhất của phản hồi vị trí sai.

### 4. Luật Trung / Nhật

- [x] Server có luật cơ bản: ăn quân, tự sát, simple ko, hai lần pass và tính đất + quân bắt với komi 6,5.
- [ ] Hoàn thiện luật Trung: superko, xử lý quân chết/thỏa thuận kết quả và kiểm thử tính điểm.
- [ ] Cân nhắc luật Nhật sau vì triển khai phức tạp; chưa ưu tiên.

### 5. Animation khi ăn quân

- [ ] Thêm animation ăn quân ở giai đoạn sau vì khá phức tạp.

### 6. Tài nguyên âm thanh

- [x] Tìm nhạc game.
- [x] Tìm âm thanh đặt quân.
- [ ] Tìm âm thanh báo lỗi.

### 7. Settings trong trận

- [x] Cho phép tùy chỉnh âm thanh.
- [ ] Cho phép bật/tắt chế độ preview.

### 8. Settings ngoài trận

- [ ] Đổi giao diện bàn đấu, avatar, khung avatar và danh hiệu.
- [ ] Có tùy chỉnh level và rank theo yêu cầu; cơ chế cụ thể sẽ xác định khi triển khai.
- [x] Bao gồm mọi tùy chỉnh của settings trong trận.

### 9. Đấu với người qua JSON

- [x] Server gửi JSON snapshot chứa bàn cờ, số nước, lượt và thời gian.
- [x] Client áp dụng snapshot và gửi lệnh thay cho thay đổi bàn cờ local.
- [ ] Lưu trạng thái trận/replay riêng; hiện trận chỉ nằm trong bộ nhớ, file JSON chưa lưu trận tự động.

### 10. Xem lại ván đấu

- [ ] Lưu tối đa 5 ván gần nhất bằng JSON chứa các nước đã đi.
- [ ] Cho phép xem lại diễn biến ván đấu.
- [ ] Đánh giá nước đi làm sau vì cần AI.

### 11. AI đấu với máy

| Độ khó     | Dự định                                                                               |
| ---------- | ------------------------------------------------------------------------------------- |
| Dễ         | Minimax suy nghĩ nông, tính trước 1–2 nước.                                           |
| Trung bình | Đã có Minimax trong `src/AI/`: depth tối đa 3, 10 ứng viên/nút, ngân sách 1 giây.     |
| Khó        | Alpha-Beta Pruning kết hợp MCTS để tối ưu; hướng tới dùng mạng 6-block ONNX trên CPU. |

Medium được tách thành module riêng tại [hướng dẫn AI](ai/IMPLEMENTATION_GUIDE.md). Chọn **Play against AI → Medium** để chơi offline. Luồng: nước người chơi → luật GoMatch → worker Minimax → áp nước AI trên GUI. Dùng một worker, có hủy khi rời/reset ván và giữ chế độ preview/Fast mode. Sau tối ưu DSU/vùng ứng viên: bàn trống hoàn thành depth 3, 586 nodes, khoảng 61 ms; độ sâu và thời gian thực tế tùy vị trí. Easy/Hard chưa triển khai, đồng hồ Byo-yomi hiện chỉ có trong trận online.

- [ ] Đo số phép tính và thời gian trên máy thực tế để mỗi nước AI không tốn quá 1–2 giây.
- [ ] Giới hạn song song tối đa 3 luồng CPU. Ý tưởng ban đầu dùng `ParallelOptions { MaxDegreeOfParallelism = 3 }`; đây là cú pháp C#, khi triển khai trong dự án C++/Qt cần cơ chế tương đương.
- [ ] MCTS giới hạn tối đa 1000 playouts và dừng khi hết ngân sách thời gian.
- [ ] Lớp bảo vệ Alpha-Beta nông ở depth 1 quét các tình huống nguy cấp như Atari, cứu quân; mục tiêu quét khoảng 1 ms cần được đo kiểm.
- [ ] Nếu có nước nguy cấp, trả về nước xử lý ngay; nếu thế trận hòa hoãn, chạy MCTS.
- [ ] Mạng 6-block `.onnx` chạy CPU: policy gợi ý 3–5 nước tốt nhất, value chấm điểm thế trận.

Luồng ý tưởng ban đầu:

```text
[ Người chơi đi 1 nước ]
             |
             v
[ Lớp bảo vệ: Alpha-Beta cắt tỉa nông (Depth 1) ]
[ Quét nhanh: Atari, cứu quân — mục tiêu 1 ms ]
             |
       +-----+--------------------+
       |                          |
Có nước nguy cấp          Thế trận hòa hoãn
       |                          |
       v                          v
[ AI đi luôn ]           [ Bộ não chính: MCTS ]
                         [ Tối đa 3 luồng CPU ]
                         [ Tối đa 1000 playouts ]
                                  |
                                  v
                         [ Mạng 6-block ONNX / CPU ]
                         [ Policy: gợi ý 3–5 nước ]
                         [ Value: chấm điểm thế trận ]
                                  |
                                  v
                         [ Trả về nước đi tối ưu ]
```

“Mát máy, 0% CPU” trong ý tưởng gốc là mong muốn giảm tải khi chọn được nước ngay; xử lý thực tế vẫn sử dụng CPU. Các mốc 1 ms, 1–2 giây, 3 luồng và 1000 playouts là mục tiêu/giới hạn thiết kế cần thử nghiệm, chưa phải kết quả benchmark.

### 12. Build mạng neuron 6b =)))

- [ ] Xây dựng và huấn luyện mạng neuron 6-block.
- [ ] Xuất mô hình `.onnx` để phục vụ policy/value trong AI.
- [ ] Thử nghiệm tốc độ suy luận CPU cùng ngân sách thời gian của MCTS.

