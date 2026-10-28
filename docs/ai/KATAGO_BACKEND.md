# KataGo Model with GoGame MCTS

`KataGoEvaluator` uses KataGo's official C++ feature generator and `NNEvaluator` to load `.bin.gz` models. Only KataGo's inference/rules components are linked, not its Search, GTP, or Analysis engine. Selection, PUCT/FPU, backup, move selection, and tree reuse remain part of `HardAI`.

All commands and paths below are relative to the project root. The script launches the **client**; run the online server separately as described in [README](../../README.md).

## Running on the Current Machine

From the GoGame directory:

```bash
./tools/run-katago-hard.sh
```

The default model is `tools/b10c384h6nbttflrs.bin.gz`, downloaded from the official KataGo v1.17.1 GitHub release. See [model provenance and checksum](../../tools/KATAGO_MODEL.md). You can always supply another model path or set `GOGAME_KATAGO_MODEL`. Dependency discovery uses `../DatasetGogame/AI` when that directory exists, otherwise the project root, and also checks the project's `.venv-ai`. The launcher builds Release into `build/katago-native` and launches the game. Select **Play against AI → Hard**. Model initialization/warm-up happens before the window opens and is excluded from the per-move budget. Subsequent inference calls reuse the loaded model. The precompiled ZIP is not used; Hard retains GoGame MCTS and links only KataGo's neural inference components.

You can specify another model and budget:

```bash
./tools/run-katago-hard.sh /path/to/model.bin.gz --ai-time-ms 1800 --ai-threads 4 --katago-duty 0.30
```

`--katago-duty` paces **each inference batch on the shared server**, including warm-up forward passes. It does not multiply load by the worker count. The default is 0.30 to leave headroom for the target of keeping GPU utilization below 50%. This is time-based pacing, not a hard driver limit: model loading, GPU sampling intervals, clock speeds, and other applications can still cause spikes. Total system RAM is not capped at 60%; the game cannot control other applications' memory.

Without `--katago-model`, Hard still uses its heuristic. Supplying a model to a build without the backend enabled reports an error and exits rather than silently switching to the heuristic. `--katago-fp32` disables FP16 if a particular model/driver causes problems. HumanSL profile models cannot serve as playing models in this backend.

## Manual Build

```bash
cmake -S . -B build/katago-native -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
  -DGOGAME_ENABLE_KATAGO=ON -DGOGAME_KATAGO_BACKEND=CUDA \
  -DCMAKE_CUDA_ARCHITECTURES=89 \
  -DGOGAME_CUDNN_INCLUDE=/path/to/cudnn/include \
  -DGOGAME_CUDNN_LIBRARY=/path/to/cudnn/lib/libcudnn.so.9
cmake --build build/katago-native -j 2
```

Architecture 89 targets the current RTX 4050; change it to the target GPU architecture when moving to another machine. CUDA Toolkit, cuDNN, zlib, and Qt are required. The launcher locates cuDNN in an existing virtual environment without installing system libraries. The cuDNN library directory must be in `LD_LIBRARY_PATH` at runtime; the launcher sets this up.

By default, CMake downloads KataGo v1.18.1 source and verifies SHA-256 `011a205c60c22f83008ba92d31f91196640f620edbfc2f09af79ef3f3311750c`. For offline builds, pass `-DGOGAME_KATAGO_SOURCE_DIR=/path/to/KataGo-1.18.1`. If you deliberately supply different source, you are responsible for checking compatibility; the patches check expected source patterns.

CPU path for machines without NVIDIA:

```bash
GOGAME_KATAGO_BACKEND=EIGEN ./tools/run-katago-hard.sh /path/to/small-model.bin.gz --ai-threads 2
```

Eigen >= 3.4 or `-DGOGAME_EIGEN_INCLUDE_DIR=/path/to/eigen-3.4.0` is required. Use a separate build directory when switching CPU/CUDA, for example `GOGAME_KATAGO_BUILD="$PWD/build/katago-cpu"`. The script accepts `GOGAME_AI_ROOT`, `GOGAME_KATAGO_MODEL`, `GOGAME_KATAGO_SOURCE_DIR`, `GOGAME_CUDA_ARCHITECTURES`, and `GOGAME_BUILD_JOBS` to override locations/options. A 70-million-parameter transformer is not lightweight for a weak CPU; choose a small KataGo model if you need more visits within two seconds.

## Inputs and Conventions

- `GoMatch` stores actual history using a copy-on-write record. JSON saves include `history`; loading replays and compares it with the saved position. Undo/redo snapshots carry the same history.
- Old saves without history become explicit setup positions, preserving stones, turn, ko, captures, and move count. Missing move order is not fabricated.
- During MCTS, the immutable journal appends simulated moves. Cache validation includes history to avoid mixing policy/value between paths with identical stone layouts.
- The backend constructs `BoardHistory`, replays real and search moves, and verifies stones/turn/ko before inference. Colors and prisoner counts are converted to the conventions of each library.
- Rules: simple ko, territory scoring, no suicide, no tax, no handicap bonus, and komi 6.5. Encore is not entered because GoGame has only one playing phase.
- Policy probabilities are converted to logs, with row coordinates increasing downward; pass is index 361. MCTS filters and normalizes using the game's legal mask.
- Value = `whiteWinProb - whiteLossProb`, with sign reversed when Black is to move; MCTS backup reverses the sign at each level. Score head/ownership are not yet part of the utility.

End-game rules still have limitations: GoGame scores territory plus prisoners on the remaining board without adjudicating dead stones as full KataGo does. Compatible encoding/inference does not imply identical scoring rules or playing strength. Two passes still use the game's terminal scorer; this limitation needs addressing before serious end-game strength comparisons.

## Batching, Memory, and Time

`searchThreads` (1–16) lets workers select leaves under the tree mutex and release it during inference. `pending` prevents duplicate leaf evaluations; virtual visits/loss distribute search effort. Workers submit requests to the same `NNEvaluator`, which batches them. Errors or cancellation release virtual visits and join all workers before returning.

The 64 MiB arena is reserved tree/cache memory, excluding model weights, CUDA/cuDNN memory, history journals, and thread stacks. The neural cache defaults to 2^14 entries, with a maximum batch of 4 under the default launcher settings. These are not total process RAM/VRAM limits.

The soft 1800 ms deadline reserves a margin based on inference latency; a running forward pass cannot be interrupted mid-kernel. Model loading is performed by `prepare()` before a game. A weak CPU or large model can still exceed the budget if a single forward pass is too slow.

## Source Copy and Local Changes

`cmake/KataGoBackend.cmake` creates a copy in the build directory without modifying downloaded source:

1. `nneval.cpp`: wraps forward calls to measure duration and sleep according to duty **per batch**, through an evaluator-specific registry. All workers still share one NN server.
2. CUDA convolution: uses cuDNN's algorithm-selection heuristic instead of assuming FP16 PRECOMP_GEMM; sets the corresponding `mathType` before querying workspace and executing convolution. This was required by the libraries on this machine; before the change, both small and transformer models reported `CUDNN_STATUS_NOT_SUPPORTED`.

Existing backend optimizations such as supported FP16, fused QKV, and tensor-core attention remain enabled; optional CUTLASS kernels are not enabled in this integration. The upstream license is included at `src/AI/KATAGO_LICENSE.txt`; dependencies have licenses in the downloaded source.

## Checks Recorded Before Documentation Reorganization

The commands/results below are historical records. The `tests/` directory is no longer in the source tree; the current build commands above do not create these test targets. Build/tests were not rerun during the documentation move on 20 October 2026.

```bash
ctest --test-dir build/katago-native --output-on-failure
build/katago-native/tests/kataGoTest /path/to/model.bin.gz 0.30 128
```

Without `GOGAME_KATAGO_TEST_MODEL`, CTest skips the real-model case; run the second command for a direct check. Tests covered history/save/load, concurrent leaves, budgets, exceptions/cancellation, legal moves, captures, ko, and pass. `KataGoEvaluator::statistics()` reports rows/batches/cache hits to verify that batching actually occurs.

Benchmarks and logs under `DatasetGogame/AI/training/katago-integration/` are smoke tests on the current machine, not rank measurements. No controlled matches against the KataGo engine with the same model/time have been run, and cpuct/FPU have not been tuned for this model.

References: [NNEvaluator](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/neuralnet/nneval.h), [features/NNOutput](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/neuralnet/nninputs.h), [history and rules interface](https://github.com/lightvector/KataGo/blob/v1.18.1/cpp/game/boardhistory.h).
