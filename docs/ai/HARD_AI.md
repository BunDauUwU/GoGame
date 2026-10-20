# HardAI và slot Sandbox

`HardAI.h/.cpp` là class C++17 độc lập với giao diện, dùng PUCT/FPU dựa trên MCTS đã cung cấp. Bàn được cố định nhất quán 19×19; `Size=10` trong bản nháp không khớp 361 giao điểm và có thể làm truy cập sai biên. Các type chung của Medium/Hard nằm ở `SearchTypes.h`, không khai báo trùng `SearchOptions`/`SearchResult`.

Đường dẫn source và lệnh tính từ thư mục gốc dự án. Các kết quả/lệnh test ngày 19/10/2026 là lịch sử: thư mục `tests/` hiện không còn trong cây nguồn. Để build hiện tại dùng `-DBUILD_TESTING=OFF` theo [README](../../README.md).

## Luồng chạy trong game

Chọn **Play against AI → Hard**. `GameManager::startHardGame()` khởi tạo ván; worker nhận snapshot `GoMatch` và bản sao engine. Kết quả chỉ được áp trên GUI nếu generation còn đúng và ván chưa thay đổi. Undo, load, chuyển Sandbox hoặc thoát sẽ hủy request cũ. Có một tác vụ Qt quản lý mỗi lượt; bên trong Hard có thể dùng nhiều worker MCTS. Mặc định class là một worker, 1800 ms, arena/cache 64 MiB; launcher KataGo dùng bốn worker. Trọng số model và journal lịch sử nằm ngoài ngân sách arena này. Ngân sách thời gian mềm; evaluator đồng bộ phải hỗ trợ cancellation/timeout riêng để tránh treo khi nạp hoặc suy luận model.

`GameManager.aiDifficulty`/snapshot phân biệt Medium/Hard; các save AI vẫn có 20 slot chung, nhưng `mode` lưu đúng độ khó và tải lại bằng đúng engine. Save Medium phiên bản 1 cũ vẫn đọc được. Các file đã có không được đổi tên hoặc di chuyển.

Sandbox có slot riêng `sandbox.json`, tách khỏi `ai-01.json`…`ai-20.json`. Menu có **Load Sandbox · Dedicated slot**; nút chỉ bật khi có save. Save/Load trong Sandbox dùng slot 0 riêng; việc Reset/Exit vẫn không tự ghi đè bản Sandbox đã lưu thủ công. Slot này không chiếm một trong 20 slot AI.

## Chỗ gắn model

Đã có `KataGoEvaluator` chạy model `.bin.gz` bằng native backend của KataGo. Xem [hướng dẫn build/chạy và giới hạn](KATAGO_BACKEND.md). Không truyền model thì Hard vẫn dùng heuristic; chưa có bằng chứng đạt sức mạnh ngang engine KataGo.

`HardAI::Evaluator` trả policy/value theo người đang đi. `Position::history` chứa setup và journal các nước thật/giả lập; evaluator phải an toàn khi gọi đồng thời nếu `searchThreads > 1`. `HardAI::inputPlanes()` chỉ dành cho GoNet 5 planes, không phải feature encoder của KataGo.

## Vì sao nhẹ và các đánh đổi

- PUCT hướng thăm dò bằng prior/value; FPU tránh coi mọi nhánh chưa biết là ngang nhau.
- Edge 8 byte, node tạo lười, vector arena được reserve; không cấp phát đối tượng heap cho từng edge hoặc giữ một bàn cờ ở từng node.
- Mỗi simulation sao chép bàn gốc nhỏ một lần, rồi áp nước dọc đường. Nhóm quân và khí dùng mảng cố định, legal mask tính một lần khi mở node.
- Cache policy/value có giới hạn và xác nhận chính xác trạng thái ngoài hash; không gộp visits giữa các đường đi.
- Giữ nhánh sau nước mình; chỉ giữ tiếp sau nước đối thủ nếu tái hiện **chính xác** snapshot mới. Undo/load/ván mới không giả định lịch sử từ hình quân.
- Có deadline/cancellation/memory cap; tất cả nước hợp lệ được giữ, không cắt cứng top-k.

Đây là tối ưu chi phí, không phải chứng minh MCTS tối ưu tuyệt đối. Prior 16-bit có làm tròn; đầy bộ nhớ thì trả nước tốt nhất đã có rồi làm mới cây. Cache và root reuse chỉ có ích nếu gặp trạng thái tương ứng. Một worker giảm overhead trên máy yếu nhưng có thể kém throughput hơn batch GPU/nhiều worker trên máy mạnh. Heuristic vẫn có thể đánh giá sai; model mạnh hơn cần kiểm tra cả tốc độ lẫn win rate.

Terminal scorer dùng cùng cách của `GoMatch::score()`: territory + prisoners + 6,5 komi, chưa tự phân xử quân chết. Không giữ scorer area từ engine benchmark cũ vì sẽ lệch luật game. Chất lượng chấm điểm hiện tại vẫn là giới hạn chung của game và AI.

## Kiểm tra

Build client/server và CTest gồm kiểm thử HardAI và controller/QML. Các ca kiểm tra gồm reply hợp lệ, snapshot không bị sửa, token hủy, hook model, lỗi model, tải lại đúng Hard/Medium, hủy kết quả cũ khi đổi chế độ và slot Sandbox độc lập. Test core so với GoMatch trên 210 lượt ngẫu nhiên: 10.830 phép kiểm tra legal; trạng thái quân, số quân bắt, lượt, hash tăng dần và final score được đối chiếu. Có fixture ko và hai pass riêng.

## Công thức PUCT và FPU giữ từ code bạn gửi

Các biến trong `HardAI.cpp` như `nodes_`, `edges_`, `cache_`, `root_`, `board_`, `visitedPrior`, `fpu`, `exploration`, `q`, `s` được giữ để đối chiếu/debug. Không đổi tên hàng loạt hoặc đổi cách biểu diễn Edge.

Gọi `N = node.visits`, `n = visits(e)`, `P = prior(e)`, `V = node.value`. Value nằm trong [-1,1], nhìn từ người sắp đi ở node hiện tại:

```text
visitedPrior = tổng P của những edge đã có ít nhất 1 visit
fpu = max(-1, V - 0.2 * sqrt(visitedPrior))
C(N) = 1.5 + log1p(N / 19652)
exploration = C(N) * sqrt(max(1, N))
Q = -child.sum / child.visits                 nếu n > 0
Q = fpu                                     nếu n = 0
s = Q + exploration * P / (1 + n)
```

Chọn edge có `s` lớn nhất trong selection; hết tìm kiếm trả edge được thăm nhiều nhất, hòa thì ưu tiên prior cao hơn. Edge giữ `child` 32-bit + `prior` 16-bit + `move` 16-bit; `static_assert(sizeof(Edge) == 8)` chặn layout bị lớn lên.

- **Q khai thác kinh nghiệm:** dấu trừ là bắt buộc vì child lưu value theo lượt đối thủ. Backup đổi dấu mỗi khi lên một tầng. Bỏ dấu này khiến AI tìm lợi ích cho đối thủ.
- **PUCT khám phá theo policy:** prior cao được thử sớm; mẫu số `1+n` giảm bonus của nhánh đã được thăm nhiều. `max(1,N)` bảo đảm có bonus ngay cả root chưa có visit.
- **1.5 là mức khám phá ban đầu.** Hệ số log tăng chậm khi cây có nhiều visits. Khi N=19652, phần cộng thêm là ln(2), nên C xấp xỉ 2.1931. 19652 là thang điều chỉnh thực nghiệm, không phải số nước, số node tối đa hay một hằng số chứng minh tối ưu.
- **FPU (First Play Urgency)** ước lượng Q của nhánh chưa thử bằng V trừ một mức thận trọng. Khi càng nhiều prior đã được thăm, nhánh chưa biết bị giảm tới khoảng 0.2 nếu prior chuẩn hóa. Khi chưa thăm nhánh nào, FPU bằng V. `max(-1,...)` giữ cận dưới hợp lệ.
- **log1p** tính ln(1+x) ổn định hơn khi x nhỏ. `prior` lưu 16-bit là xấp xỉ có làm tròn; mọi nước hợp lệ vẫn có prior dương. Không dùng `fpu` để loại nước hợp lệ vĩnh viễn.

Các hệ số này là heuristic theo yêu cầu, chưa có benchmark chứng minh tốt nhất cho GoGame. Chúng điều phối tài nguyên tìm kiếm; chất lượng cuối cùng còn phụ thuộc evaluator, luật và ngân sách. Bản mặc định dùng heuristic leaf evaluation, không chạy rollout ngẫu nhiên đến hết ván và không có RAVE. Không đồng nhất PUCT này với toàn bộ thuật toán KataGo.

## mix64, nhóm quân và nhận diện trạng thái

`mix64` giữ phép trộn SplitMix64 trong code đính kèm. Nó dùng để sinh khóa Zobrist cho `(vị trí padded, màu)` và trộn metadata với salt riêng; không cấp phát bộ sinh số ngẫu nhiên ở mỗi nước. Khi đặt/bắt quân, `stonesHash` được XOR tăng dần. Đây là phép ánh xạ xác định để phân bố khóa, không phải RNG mật mã và không bảo đảm hết collision.

Cache kiểm tra cả hash **và** `Board::same`: toàn bộ `cells`, lượt, ko, pass, komi, quân bắt, moveCount và lịch sử khi có model. Hai thế cờ cùng hình quân nhưng khác các metadata này không được dùng lẫn. Cache dùng bảng direct-mapped có dung lượng cố định: collision đè một entry cũ sau khi đánh giá mới thành công. Nó chỉ chia sẻ policy/value, không gộp visits của các nhánh khác nhau. Kết quả evaluator bị hủy không được ghi vào cache. Thay model/reset tạo backend/cache riêng; worker đã chạy giữ backend cũ đến khi kết thúc.

Hard dùng `Board` padded 21×21, biên sentinel, `Groups`/queue/visited/legal mask là mảng cố định riêng. Mỗi group đếm một giao điểm khí chung đúng một lần. `Groups` tính một lần ở expansion và truyền lại cho heuristic, tránh BFS hai lần. Hiện dùng flood-fill mảng, **không thêm DSU cho có**: chưa có benchmark cho thấy giữ DSU tăng dần với việc copy board/capture sẽ tốt hơn. Chỉ `GoMatch` tại biên API cung cấp snapshot/kiểm chứng; selection/di chuyển trong cây/scorer của Hard không gọi luật hoặc nhóm của class đó.

Đây là simple ko với luật/scorer hiện có. Nếu thêm superko hoặc model dùng lịch sử nhiều nước, phải bổ sung lịch sử vào Board/Position/hash/so sánh, không tái sử dụng cache chỉ từ hình quân.

## Giữ nhánh, arena và giới hạn

Root sau nước mình được chuyển tới child tương ứng. Khi snapshot đối thủ đến, engine thử tái hiện đúng một nước/pass rồi so sánh toàn trạng thái; chỉ khi khớp mới giữ nhánh. Nếu không khớp thì reset root, cache policy/value vẫn có thể dùng khi trạng thái khớp chính xác. Undo và Load không được đoán là đối thủ vừa đi.

Khi arena gần đầy, engine bỏ sibling không còn liên quan và **compact in-place** các node/edge còn tới được từ root. `remap` và `order` là workspace đã reserve trong ngân sách; không tạo một arena thứ hai. Child luôn được cấp phát sau parent; edge được dồn theo thứ tự vùng nhớ cũ, sau đó node và index được cập nhật. Visits/value của subtree được giữ nguyên. Nếu cây phù hợp vẫn đầy thì dừng tìm kiếm và trả nước tốt nhất đã biết, không cố tăng vector ngoài cap.

Ngân sách `memoryMB` gồm capacity của nodes, edges, cache, path/parents và workspace compact; chừa 64 KiB cho fixed work arrays/đối tượng phụ. Không phải giới hạn RSS của cả ứng dụng: chưa tính UI, lịch sử undo, allocator metadata, stack worker của hệ điều hành hoặc trọng số/workspace của model. Khi gắn KataGo cần cấu hình thêm RAM/cache/thread của backend đó. `allocatedBytes` báo heap search đã reserve; không báo toàn bộ RSS.

Deadline tính từ đầu `chooseMove`, gồm chờ mutex, chuẩn bị board, cấp phát và tìm kiếm; không cấp lại toàn bộ 1800 ms sau mỗi bước. Một evaluator đồng bộ không hợp tác vẫn có thể vượt deadline, nên đây là ngân sách mềm. Token hủy được kiểm tra trong search và sau evaluate. Có một tác vụ Qt điều phối mỗi lượt; Hard mặc định một search worker, còn launcher KataGo cấu hình nhiều worker MCTS bên trong (mặc định bốn). Mặc định: 1800 ms, 64 MiB, depth tối đa 256, 100000 simulation tối đa; dừng theo giới hạn chạm trước.

Các số debug trong `snapshot.aiSearch`: `nodes` (simulations hoàn tất của lượt này), `completedDepth`, `elapsedMs`, `evaluations`, `cacheHits`, `rootVisits`, `reusedVisits`, `allocatedBytes`, `memoryLimited`. `reusedVisits` là visits đã có ở root đầu search, không phải một cam kết rằng mọi lượt đều tái sử dụng được.

## Hợp đồng tích hợp KataGo

Giữ session/model sống trong `HardAI::Evaluator`, tải trước lúc bắt đầu search; tuân thủ contract logits/value và cancellation. Model cần lịch sử phải mở rộng contract và cache identity tương ứng. API phân tích của KataGo cũng phân biệt moves/history với initialStones; không nên bịa lại lịch sử từ một snapshot: [KataGo Analysis Engine](https://github.com/lightvector/KataGo/blob/master/docs/Analysis_Engine.md). Hook là điểm mở rộng C++; inference native hiện có trong `KataGoEvaluator` theo [KATAGO_BACKEND.md](KATAGO_BACKEND.md). Hook không tự tải trọng số từ mạng.

### Kết quả kiểm tra ngày 19/10/2026

- Release: build client/server tại `build/hard-mcts`; CTest **3/3 đạt** (`soloAI`, `hardAI`, `hardCore`).
- Debug với AddressSanitizer + UndefinedBehaviorSanitizer: **2/2 đạt** (`hardAI`, `hardCore`), không báo truy cập sai hoặc undefined behavior trong các ca chạy.
- Core test có 10.830 phép đối chiếu legal, cố tình tạo collision hash, kiểm tra chiều dấu backup/FPU, depth cutoff, exact cache identity, compact subtree, root reuse hai lượt và arena 4 MiB khi đầy.
- Test public thêm zero-budget fallback, NaN output và hủy ngay trong evaluator (không làm bẩn cache). Test QML xác nhận Hard selectable/save/load và Sandbox không ghi đè file AI.

Lệnh chạy lại:

```bash
cmake -S . -B build/hard-mcts -DCMAKE_BUILD_TYPE=Release
cmake --build build/hard-mcts --parallel 3
ctest --test-dir build/hard-mcts --output-on-failure
```

Ví dụ giảm tài nguyên khi dùng class độc lập:

```cpp
HardSearchOptions options;
options.timeBudgetMs = 800;
options.memoryMB = 16;
HardAI hard(options);
auto result = hard.chooseMove(position, cancellationToken);
```

Các test này kiểm chứng tính đúng của cơ chế và tích hợp, chưa phải benchmark rank/win rate hoặc cam kết thời gian trên mọi CPU.
