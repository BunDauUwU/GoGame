# Tự triển khai AI client: Minimax, Alpha-Beta, MCTS và mạng 6B

## Các phần đã có

```text
src/
├── Rules/GoMatch.h/.cpp          # Luật DSU dùng chung cho client và server
├── Managers/DataManager.h/.cpp   # JSON, slot, validation/replay lịch sử
└── AI/
    ├── MediumAI.h/.cpp           # Baseline Minimax + Alpha-Beta
    ├── AIGame.cpp                # Điều phối AI, worker, cancellation/generation
    ├── trainAI.py                # SGF → policy/value → Transformer ~6B → ONNX
    └── training/requirements.txt
```

AI chạy hoàn toàn ở client. Server chỉ xử lý luật, đồng hồ và multiplayer, không tìm nước AI hoặc huấn luyện. AI, DataManager và GoMatch được khai báo trực tiếp trong `CMakeLists.txt` gốc, biên dịch vào target client `appGoGame`; không có CMake con hoặc library riêng cho ba phần này. Client link Qt Concurrent để chạy AI. Server có `server/CMakeLists.txt` riêng, biên dịch source GoMatch dùng chung và chỉ dùng Qt Core/Network/WebSockets.

Baseline Medium được giữ để ứng dụng tiếp tục chạy. MCTS và suy luận ONNX **chưa được nối vào runtime**: tài liệu này dành cho bạn tự triển khai. File trainAI tạo được kiến trúc/training pipeline khi cài dependencies; chưa có trọng số được huấn luyện đi kèm.

## Điểm nối trong client

Mở `AIGame.cpp`, tìm `GameManager::requestAIMove()`. Worker hiện gọi:

```cpp
return engine.chooseMove(position, cancel);
```

`GameManager` sở hữu đối tượng `AI::MediumAI m_ai`; worker nhận bản sao `engine = m_ai`. Class nhận `SearchOptions` qua constructor, giữ cấu hình và hàm đánh giá; class lồng `MediumAI::Search` giữ timer, node count, cancellation và Minimax cho từng lần tìm kiếm. `chooseMove()` là phương thức const, không dùng trạng thái tìm kiếm chung giữa các lần gọi.

Thay lời gọi này bằng engine của bạn, giữ hợp đồng `AI::SearchResult`:

- `row`, `col`: tọa độ dữ liệu, 0–18; tuyệt đối không xoay theo UI.
- `pass`: true khi bỏ lượt, row/col = -1.
- `cancelled`: tác vụ đã bị hủy.
- `nodes`, `elapsedMs`, `completedDepth`: thông tin đo kiểm.

Worker chỉ đọc bản sao `GoMatch`. Không chạm QObject/QML, lịch sử, file save hoặc trạng thái GameManager từ worker. Callback GUI đã kiểm tra generation, áp nước qua GoMatch, cập nhật history, auto-save và xử lý hai pass. Giữ cancellation check trong mọi vòng lặp dài để Undo/Load/Exit không nhận kết quả của ván cũ.

## Minimax

1. Sinh nước hợp lệ bằng bản sao position và `play(row, col, error)`. Thêm pass.
2. Tìm đến depth giới hạn hoặc trạng thái hai pass.
3. Lá: đánh giá vật chất, khí/atari, ảnh hưởng hoặc mạng.
4. Nếu điểm theo **đen**, node đen lấy max, node trắng lấy min.
5. Dùng iterative deepening; chỉ công bố nước của depth đã hoàn thành. Hết giờ dùng kết quả depth trước hoặc nước hợp lệ dự phòng.

Baseline chỉ sinh vùng gần quân + hoshi rồi chọn top-k, nên đây là tìm kiếm trên tập ứng viên, không quét toàn bộ cây cờ vây. Khi đổi heuristic, kiểm tra bắt quân/cứu atari và những nước xa vùng ứng viên; giới hạn ứng viên có thể bỏ sót chiến thuật.

## Alpha-Beta Pruning

Alpha-Beta giữ kết quả Minimax trên **cùng cây và cùng hàm đánh giá**, giảm nhánh cần duyệt. Tại node max cập nhật `alpha = max(alpha, best)`; tại node min cập nhật `beta = min(beta, best)`; dừng nhánh khi `beta <= alpha`. Xếp nước bắt quân/cứu nhóm hoặc policy cao trước để có nhiều cutoff.

Bắt đầu từ implementation `MediumAI::Search::minimax` trong MediumAI.cpp. Nếu dùng transposition table, key phải gồm board, người đi, trạng thái ko/previousBoard, số pass và komi; lưu depth cùng loại bound exact/lower/upper. Không coi kết quả của search bị timeout là exact. Giới hạn bảng và xóa khi đổi model/luật.

## MCTS với PUCT

Mỗi node có position, người đến lượt và các edge `{move, prior, N, W, Q, child}`. `Q = W/N` được định nghĩa theo góc nhìn của người đi tại **parent**.

Lặp đến 1000 simulations hoặc hết ngân sách 1–2 giây:

1. **Selection:** chọn edge tối đa `Q + c_puct * P * sqrt(N_parent) / (1 + N_edge)`.
2. **Expansion:** sinh nước hợp lệ + pass. Policy mạng cho prior P, mask nước sai rồi chuẩn hóa. Có thể giữ top-k, nhưng phải giữ các nước bắt/cứu nhóm quan trọng.
3. **Evaluation:** nếu hai pass thì tính kết quả bằng luật. Nếu chưa kết thúc, lấy value từ mạng hoặc dùng rollout/heuristic baseline.
4. **Backup:** value lá theo người đi tại lá; đổi dấu khi đi ngược mỗi ply. W/Q của edge phải theo người đi ở parent, không giữ cùng dấu ở hai bên.
5. **Chọn nước:** trong trận thật chọn edge có visit count cao nhất. Noise/temperature phục vụ self-play huấn luyện, không tự bật ở trận người chơi.

Tài liệu gốc về policy/value kết hợp tree search: [AlphaGo paper](https://storage.googleapis.com/deepmind-media/alphago/AlphaGoNaturePaper.pdf), [AlphaZero implementation trong OpenSpiel](https://github.com/google-deepmind/open_spiel/blob/master/docs/alpha_zero.md).

Đầu tiên làm MCTS một worker để kiểm tra backup và tính điểm. Khi song song, mỗi worker giữ bản sao position; đồng bộ N/W/expansion của node, xử lý virtual loss và cancellation. Không cho worker trên pool chỉ có một thread chờ công việc khác cũng nằm trên chính pool đó. Tổng số worker tính toán không quá 3; benchmark số simulations/giây trên máy thật.

## Huấn luyện mạng 6B từ SGF

`--architecture 6b` là cấu hình mặc định: **5.974.470.659 tham số**, xấp xỉ 6 tỷ. Transformer encoder toàn bàn cờ, width 4096, 32 layers, 32 attention heads, SwiGLU hidden 9728, pre-RMSNorm. Mỗi giao điểm là một token; attention hai chiều, không dùng causal mask. Đầu ra giữ cùng hợp đồng policy/value. Đây là kiến trúc tự định nghĩa cho cờ vây, chưa có trọng số pretrained hoặc bằng chứng sức chơi.

```text
[5 × 19 × 19] → 361 tokens × 4096 + learned positional embedding
               → 32 × [RMSNorm / attention + skip / RMSNorm / SwiGLU + skip]
               → RMSNorm
               ├→ từng token → 361 logits; mean pool → pass logit
               └→ mean pool → tanh value
```

Xem cấu hình và đếm tham số bằng `--describe-model`, không cần PyTorch hoặc cấp phát 6B trọng số. FP32 weights riêng khoảng 23,9 GB (22,3 GiB); weights + gradients + hai buffers AdamW khoảng 95,6 GB, chưa gồm activations và checkpoint staging. Script hiện train một CPU/GPU, chưa có FSDP, sharding hoặc quantization. Chọn phần cứng đủ bộ nhớ; batch 1 là mặc định. Client local cần bạn tự benchmark/quantize/distill trước khi dùng 6B trong mỗi lượt MCTS.

Có thêm `--architecture resnet6` để thử nhanh pipeline: 6 residual blocks, mặc định 64 channels, cùng input/output:

```text
[5 × 19 × 19] → Conv 3×3 / BN / ReLU
              → 6 × [Conv / BN / ReLU / Conv / BN + skip / ReLU]
              ├→ policy head → 362 logits (361 ô + pass)
              └→ value head  → tanh, 1 giá trị [-1,1]
```

Python 3.10+; prepare/inspect chỉ dùng standard library. Training/export cần dependencies:

```bash
python3 -m venv .venv-ai
. .venv-ai/bin/activate
python -m pip install -r src/AI/training/requirements.txt
```

Chọn bản PyTorch CPU/CUDA tương ứng máy theo [hướng dẫn PyTorch](https://pytorch.org/get-started/locally/); không chạy pip vào môi trường hệ thống nếu muốn giữ dự án cô lập.

```bash
# Kiểm tra corpus, chưa cần PyTorch
python src/AI/trainAI.py --inspect /path/to/sgf
python src/AI/trainAI.py --describe-model --architecture 6b

# Replay SGF, tạo train.jsonl / validation.jsonl / manifest.json
python src/AI/trainAI.py --prepare /path/to/sgf --output training-data --seed 42

# Huấn luyện và lưu checkpoint sau mỗi epoch
python src/AI/trainAI.py --train training-data --architecture 6b --output models/gonet6b.pt --epochs 10 --batch-size 1 --threads 3

# Thử pipeline nhỏ trước khi cấp phát model 6B
python src/AI/trainAI.py --train training-data --architecture resnet6 --output models/smoke.pt --epochs 1 --batch-size 8

# Tiếp tục optimizer/epoch từ checkpoint
python src/AI/trainAI.py --train training-data --output models/gonet6b.pt --resume models/gonet6b.pt --epochs 5

# Export ONNX và file mô tả tensor .onnx.json
python src/AI/trainAI.py --export models/gonet6b.pt --output models/gonet6b.onnx
```

Corpus phải có ván 19×19 và kết quả RE biết được (B+/W+/draw). Parser hỗ trợ collection, main variation đầu tiên, escape, setup AB/AW/AE, rectangle setup, HA/PL và pass rỗng/tt. File sai, nước trái luật hoặc kết quả chưa biết bị bỏ và báo rõ. Chỉ lấy nhánh chính; không gán kết quả của ván thật cho các nhánh phân tích. Chia train/validation theo game ID; ván trùng khác annotation được khử trùng. Corpus quá nhỏ có thể không có validation: thêm dữ liệu hoặc đổi seed/split, không lấy các nước của cùng ván sang hai tập. Tham chiếu: [SGF FF4](https://www.red-bean.com/sgf/), [property index](https://www.red-bean.com/sgf/proplist_t.html).

Transformer dùng [scaled dot-product attention của PyTorch](https://docs.pytorch.org/docs/stable/generated/torch.nn.functional.scaled_dot_product_attention.html). Export 6B dùng ONNX external data vì weights vượt giới hạn protobuf 2 GB; giữ file `.onnx` và toàn bộ file weights đi kèm trong cùng thư mục, không đổi tên riêng file weights. C++ Runtime đọc external data trực tiếp theo [tài liệu ONNX Runtime](https://onnxruntime.ai/docs/tutorials/web/large-models.html#external-data).

Loss = cross-entropy policy + MSE value. Dữ liệu được stream theo JSONL, augmentation xoay/lật áp dụng đồng thời lên input, legal mask và nhãn policy; pass giữ index 361. Theo dõi validation loss, policy accuracy, rồi đấu nhiều ván đối chứng; supervised SGF không tự tạo ra engine mạnh hoặc self-play AlphaZero. Script chưa dùng mixed precision, scheduler, replay self-play hoặc RNG-resume bit-for-bit.

## Hợp đồng input/output neuron

Input `planes`, float32 NCHW `[batch,5,19,19]`:

| Plane | Ý nghĩa |
|---|---|
| 0 | Quân của người **đến lượt**, 0/1 |
| 1 | Quân đối phương, 0/1 |
| 2 | Nước đặt quân hợp lệ, 0/1; tính cả tự sát và simple ko |
| 3 | Toàn 1 nếu đen đến lượt, toàn 0 nếu trắng |
| 4 | Komi/20 nếu trắng đến lượt; -komi/20 nếu đen |

Output `policy_logits` `[batch,362]`, ô `(row,col)` có index `row*19+col`; pass = 361. Output `value` `[batch,1]` theo người **đến lượt**: +1 thắng, -1 thua, 0 hòa. Mạng trả logits thô; client phải mask luật hiện tại rồi softmax, không dùng trực tiếp logits làm xác suất. Không xoay input theo màu người chơi trên màn hình.

Minimax dùng điểm theo đen thì chuyển `v_black = turn == Black ? v : -v`. MCTS đổi dấu qua mỗi ply khi backup. Nếu trộn value [-1,1] với heuristic vật chất có thang điểm lớn, chuẩn hóa hoặc đặt trọng số trước khi so sánh.

## Đọc ONNX từ C++ trên client

Cài/link ONNX Runtime theo [C++ getting started](https://onnxruntime.ai/docs/get-started/with-cpp). Đặt code suy luận riêng, ví dụ `src/AI/NeuralEvaluator.cpp`, rồi gọi trong worker. Giữ một session, không nạp model cho từng nước.

```cpp
#include <onnxruntime_cxx_api.h>
Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "GoGameAI");
Ort::SessionOptions options;
options.SetIntraOpNumThreads(1);
options.SetInterOpNumThreads(1);
Ort::Session session(env, "models/gonet6b.onnx", options); // Linux path

std::array<int64_t, 4> shape{1, 5, 19, 19};
std::vector<float> planes(5 * 19 * 19); // Fill using the table above.
auto memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
auto input = Ort::Value::CreateTensor<float>(memory, planes.data(), planes.size(), shape.data(), shape.size());
const char *inputs[] = {"planes"};
const char *outputs[] = {"policy_logits", "value"};
auto result = session.Run(Ort::RunOptions{nullptr}, inputs, &input, 1, outputs, 2);
```

Kiểm tra tên, dtype và shape output theo `.onnx.json` trước khi đọc 362 logits/1 value. Output pointers chỉ sống cùng Ort::Value; sao chép nếu giữ làm cache. Handle lỗi nạp/suy luận để fallback về baseline hợp lệ. Export dùng exporter dynamo và batch động theo [PyTorch ONNX](https://docs.pytorch.org/docs/stable/onnx.html).

Bắt đầu ORT với 1 intra-op thread nếu MCTS đang dùng nhiều search workers, tránh nhân số luồng ngoài dự kiến. Nếu chỉ một search worker, có thể benchmark ORT 1–3 threads. Xem [ONNX Runtime threading](https://onnxruntime.ai/docs/performance/tune-performance/threading.html).

## Thứ tự bạn có thể tự làm tiếp

1. Chạy baseline, kiểm tra luật/cancellation/save-load khi đổi engine.
2. Train SGF corpus thật; kiểm tra policy/value và validation.
3. Nối NeuralEvaluator, đối chiếu tensor PyTorch và ONNX với cùng input.
4. Dùng policy xếp ứng viên Minimax/Alpha-Beta, đo lại budget và chiến thuật.
5. Triển khai MCTS một worker, kiểm tra dấu value/pass/ko/terminal, rồi benchmark song song.
6. Đấu baseline, đo sức chơi và tốc độ trước khi thay engine mặc định.

Kiểm chứng đã thực hiện trong venv tạm với PyTorch CPU/ONNX: 9 unittest đạt, gồm replay/split SGF, shape/backward ResNet và Transformer nhỏ, đếm đúng 5.974.470.659 tham số của 6B trên meta device. Fixture SGF đã chạy prepare → train model nhỏ → resume → export; ONNX ResNet và Transformer nhỏ khớp PyTorch ở batch 1 và 3. Kiểm tra này xác nhận pipeline, không đo sức chơi. Chưa có corpus SGF thật, checkpoint đã huấn luyện hoặc export đầy đủ model 6B.

Các file test và cấu hình CTest đã được xóa khỏi dự án theo yêu cầu. Kết quả kiểm chứng trên ghi nhận lần kiểm tra trước khi xóa.
