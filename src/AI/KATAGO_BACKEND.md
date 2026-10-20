# KataGo model + MCTS của GoGame

`KataGoEvaluator` dùng bộ tạo features và `NNEvaluator` C++ chính thức của KataGo để đọc model `.bin.gz`. Chỉ liên kết phần inference/rules của KataGo; không liên kết `Search`, GTP hoặc Analysis engine. Selection, PUCT/FPU, backup, chọn nước và tái sử dụng cây vẫn thuộc `HardAI`.

## Chạy trên máy hiện tại

Từ thư mục GoGame:

```bash
./tools/run-katago-hard.sh
```

Script mặc định tìm model transformer tại `../DatasetGogame/AI/kata1-tf3-b11c768-s11003M-d5973M-7gres.bin.gz`, build Release vào `build/katago-native`, rồi mở game. Chọn **Play against AI → Hard**. Khởi tạo/warm-up model diễn ra trước khi mở cửa sổ; không tính vào ngân sách một nước. Các lần suy luận sau dùng chung model đã nạp.

Có thể chỉ định model khác và ngân sách:

```bash
./tools/run-katago-hard.sh /duong/dan/model.bin.gz --ai-time-ms 1800 --ai-threads 4 --katago-duty 0.30
```

`--katago-duty` điều tiết **từng batch inference trên server chung**, bao gồm warm-up forward. Nó không nhân tải lên theo số worker. Mặc định 0.30 để chừa khoảng trống cho mục tiêu GPU dưới 50%. Đây là pacing theo thời gian, không phải một giới hạn cứng của driver: quá trình nạp model, khoảng đo của GPU, xung nhịp và ứng dụng khác vẫn có thể tạo đỉnh tải. RAM toàn hệ thống cũng không được khóa ở 60%; game không thể kiểm soát bộ nhớ của các ứng dụng khác.

Không truyền `--katago-model` thì bản game vẫn dùng heuristic cho Hard. Truyền model vào bản build chưa bật backend sẽ báo lỗi và thoát, không âm thầm chuyển sang heuristic. `--katago-fp32` cho phép tắt FP16 nếu gặp vấn đề với một model/driver cụ thể. HumanSL profile model không được dùng làm playing model ở backend này.

## Build thủ công

```bash
cmake -S . -B build/katago-native -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DGOGAME_ENABLE_KATAGO=ON -DGOGAME_KATAGO_BACKEND=CUDA \
  -DCMAKE_CUDA_ARCHITECTURES=89 \
  -DGOGAME_CUDNN_INCLUDE=/duong/dan/cudnn/include \
  -DGOGAME_CUDNN_LIBRARY=/duong/dan/cudnn/lib/libcudnn.so.9
cmake --build build/katago-native -j 2
```

Kiến trúc 89 dành cho RTX 4050 hiện tại; thay bằng kiến trúc GPU đích khi chuyển máy. Cần CUDA Toolkit, cuDNN, zlib và Qt. Launcher tự tìm cuDNN trong venv hiện có, không cài thư viện hệ thống. Cần để thư mục chứa các thư viện cuDNN vào `LD_LIBRARY_PATH` khi chạy; launcher đã làm việc này.

CMake mặc định tải nguồn KataGo v1.18.1 và kiểm tra SHA-256 `011a205c60c22f83008ba92d31f91196640f620edbfc2f09af79ef3f3311750c`. Để build offline, truyền `-DGOGAME_KATAGO_SOURCE_DIR=/thu/muc/KataGo-1.18.1`. Nếu chủ động chỉ định nguồn khác, người build chịu trách nhiệm kiểm tra tương thích; các patch có kiểm tra mẫu nguồn.

Đường CPU cho máy không có NVIDIA:

```bash
GOGAME_KATAGO_BACKEND=EIGEN ./tools/run-katago-hard.sh /duong/dan/model-nho.bin.gz --ai-threads 2
```

Cần Eigen >= 3.4 hoặc `-DGOGAME_EIGEN_INCLUDE_DIR=/thu/muc/eigen-3.4.0`. Dùng build directory riêng khi đổi CPU/CUDA, ví dụ đặt `GOGAME_KATAGO_BUILD="$PWD/build/katago-cpu"`. Script nhận `GOGAME_AI_ROOT`, `GOGAME_KATAGO_MODEL`, `GOGAME_KATAGO_SOURCE_DIR`, `GOGAME_CUDA_ARCHITECTURES`, `GOGAME_BUILD_JOBS` để thay vị trí/tùy chọn. Model transformer 70 triệu tham số không phải lựa chọn nhẹ cho CPU yếu; chọn model KataGo nhỏ nếu cần nhiều visits trong 2 giây.

## Đầu vào và quy ước

- `GoMatch` giữ lịch sử thật bằng bản ghi có copy-on-write. Save JSON có thêm `history`; load kiểm tra bằng cách replay và so với thế cờ đã lưu. Undo/redo dùng snapshot mang cùng lịch sử.
- Save cũ không có lịch sử trở thành setup position rõ ràng: giữ hình quân, lượt, ko, quân bắt và số nước. Không bịa thứ tự đặt quân để thay lịch sử đã mất.
- Trong MCTS, journal bất biến nối thêm các nước giả lập. Cache xác nhận cả lịch sử, tránh dùng lẫn policy/value giữa hai đường đi có cùng hình quân.
- Backend dựng `BoardHistory`, replay cả nước thật và nước tìm kiếm, đối chiếu hình quân/lượt/ko trước khi chạy mạng. Đen/trắng và số prisoners được đổi theo đúng quy ước hai thư viện.
- Rules: simple ko, territory, không suicide, tax none, không handicap bonus, komi 6.5. Không vào encore vì GoGame chỉ có một pha chơi.
- Policy được chuyển từ xác suất sang log, cùng tọa độ hàng từ trên xuống; pass ở index 361. MCTS lọc và chuẩn hóa theo legal mask của game.
- Value = `whiteWinProb - whiteLossProb`, đổi dấu khi lượt đen; backup MCTS tiếp tục đổi dấu mỗi tầng. Chưa kết hợp score head/ownership vào utility.

Luật kết thúc vẫn có giới hạn: GoGame chấm territory + prisoners trên quân còn lại, chưa phân xử quân chết như KataGo hoàn chỉnh. Vì vậy tương thích encoding/inference không đồng nghĩa luật chấm điểm và sức mạnh ngang engine KataGo. Hai pass vẫn dùng terminal scorer của game; đây là giới hạn cần xử lý trước khi so sức mạnh nghiêm túc ở tàn cuộc.

## Batch, bộ nhớ và thời gian

`searchThreads` (1–16) cho phép các worker chọn lá dưới mutex của cây rồi thả mutex trong lúc suy luận. `pending` ngăn đánh giá trùng một lá; virtual visits/loss phân tán lượt tìm. Các worker đẩy request vào cùng `NNEvaluator`, nơi tự gom batch. Lỗi hoặc cancellation thu hồi virtual visits và join toàn bộ worker trước khi trả kết quả.

Arena 64 MiB là bộ nhớ cây/cache được reserve, không bao gồm trọng số model, bộ nhớ CUDA/cuDNN, journal lịch sử và stack của các luồng. Neural cache mặc định 2^14 entries, batch tối đa 4 khi chạy bằng launcher mặc định. Các con số này không phải giới hạn tổng RAM/VRAM của tiến trình.

Deadline mềm 1800 ms chừa margin dựa trên latency suy luận; một forward đang chạy không thể bị ngắt giữa kernel. Nạp model được thực hiện bằng `prepare()` trước ván. CPU yếu hoặc model lớn vẫn có thể vượt thời gian khi một forward đơn lẻ quá chậm.

## Bản sao nguồn và các sửa đổi cục bộ

`cmake/KataGoBackend.cmake` tạo bản sao trong build directory, không sửa nguồn tải về:

1. `nneval.cpp`: bọc lời gọi forward để đo thời gian và nghỉ theo duty **mỗi batch**, qua registry riêng của từng evaluator. Tất cả workers vẫn dùng chung một NN server.
2. CUDA convolution: dùng heuristic chọn thuật toán của cuDNN thay giả định FP16 PRECOMP_GEMM; đặt đúng `mathType` tương ứng trước khi hỏi workspace và chạy convolution. Cần thiết với bộ thư viện trên máy này; trước khi sửa cả model nhỏ và transformer đều báo `CUDNN_STATUS_NOT_SUPPORTED`.

Giữ bật các tối ưu có sẵn trong backend như FP16 khi hỗ trợ, QKV gộp và tensor-core attention; không bật kernel CUTLASS tùy chọn trong bản tích hợp này. Giấy phép upstream được kèm ở `KATAGO_LICENSE.txt`; các dependency có giấy phép trong nguồn tải về.

## Kiểm tra

```bash
ctest --test-dir build/katago-native --output-on-failure
build/katago-native/tests/kataGoTest /duong/dan/model.bin.gz 0.30 128
```

Nếu không cấu hình `GOGAME_KATAGO_TEST_MODEL`, CTest bỏ qua ca model thật; chạy lệnh thứ hai để kiểm tra trực tiếp. Test gồm lịch sử/save/load, concurrent leaves, budget, exception/cancel, nước hợp lệ, capture, ko và pass. `KataGoEvaluator::statistics()` trả số rows/batches/cache hits để kiểm tra batch thực sự hoạt động.

Benchmark và log trong `DatasetGogame/AI/training/katago-integration/` là smoke test trên máy hiện tại, không phải đo rank. Chưa đấu đối chứng với engine KataGo cùng model/thời gian và chưa tune cpuct/FPU cho model này.

Nguồn tham khảo: [NNEvaluator](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/neuralnet/nneval.h), [đặc trưng/NNOutput](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/neuralnet/nninputs.h), [giao diện lịch sử và luật](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/game/boardhistory.h).
