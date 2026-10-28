# Hard Mode with the Packaged KataGo Engine

`tools/run-katago-hard.sh` uses `src/AI/katago-v1.18.1-cuda12.1-cudnn8.9.7-linux-x64+bs50.zip`. This is the local package selected for this project, not a claim that v1.18.1 is the newest upstream release. The launcher does not download a different engine or compile native CUDA sources.

## Launch

```bash
./tools/run-katago-hard.sh /path/to/model.bin.gz --ai-time-ms 1800 --ai-threads 4
```

Select **Play against AI → Hard**. Easy, Medium, Sandbox, online rules, and server behavior are unchanged. Without engine/model arguments, the normal desktop build retains heuristic Hard mode.

A playing model is required separately; the ZIP contains no neural weights. The default model path is `src/AI/kata1-tf3-b11c768-s11003M-d5973M-7gres.bin.gz`. Override it with a positional path or `GOGAME_KATAGO_MODEL`. The model must be supported by this engine; HumanSL models need additional profile configuration and are not the default.

The first launch extracts the ZIP into `build/katago-engine/engine`, and detects/extracts its AppImage wrapper without FUSE or desktop integration. It caches the extracted files using the ZIP's SHA-256 and rebuilds them when the archive changes. Only extraction/build artifacts go into the ignored build directory. The original ZIP is preserved.

Useful checks:

```bash
./tools/run-katago-hard.sh --extract-only
./tools/run-katago-hard.sh --check
./tools/run-katago-hard.sh --help
```

`--extract-only` requires no model or CUDA initialization. `--check` runs the engine's version command and reports missing runtime libraries before building the client.

## CUDA Runtime

This ZIP targets Linux x86-64, CUDA 12.x and cuDNN 8.x. CUDA 13 and cuDNN 9 do not provide the corresponding library names. For example, `libcublas.so.12` cannot be replaced by a symlink to `libcublas.so.13`.

Use existing compatible runtime libraries, or install compatible runtimes separately. To supply library directories without changing the system loader configuration:

```bash
GOGAME_KATAGO_LIBRARY_DIR=/path/to/cuda12/lib:/path/to/cudnn8/lib \
  ./tools/run-katago-hard.sh /path/to/model.bin.gz
```

The AppImage provides some auxiliary libraries; NVIDIA runtime libraries and a compatible NVIDIA driver are still needed. The script does not install packages automatically.

## Client Integration

`KataGoEngine` implements the `HardAI::Engine` interface and runs a persistent `katago analysis` process on a dedicated worker thread. The engine loads and warms its model once before the client window opens. Each move sends a compact JSON query, and KataGo performs its own neural evaluation and search. This path does not run GoGame MCTS on top of KataGo's search.

Each request includes the real initial stones, initial player, complete move history, board dimensions, komi, and explicit territory/simple-ko rules. Initial prisoners in legacy setup positions are compensated in komi. The current local legal moves constrain the root, and the returned move is checked again with `GoMatch` before being played. Ko/pass setups without sufficient replay history report an error instead of inventing previous moves.

Undo, redo, and loading a save work by submitting the restored history in the next query. The process and neural cache remain loaded. No online server or cloud service is involved.

Queries have unique IDs. Cancellation sends `terminate`; late replies cannot be applied to a new move. Timeouts, process exits, malformed JSON, and illegal moves become an AI error. Hard does not silently pass when the engine fails. Shutdown kills the child process and joins its worker.

`--ai-time-ms` controls KataGo's search time; IPC, scheduling and an in-flight neural batch can add overhead. `--ai-threads` sets the search threads in the generated config. The default config uses one analysis job at a time and a neural cache of 2^16 entries. `--katago-duty` belongs to the older native inference path and is rejected for the external engine; this mode does not promise a GPU utilization cap.

The game's own final scorer is unchanged and still has dead-stone/seki limitations. KataGo's prediction may differ from the displayed final score.

## Manual Launch and Overrides

Build the client normally; `GOGAME_ENABLE_KATAGO=ON` is not needed:

```bash
cmake -S . -B build/client -G Ninja -DCMAKE_BUILD_TYPE=Release -DGOGAME_ENABLE_KATAGO=OFF
cmake --build build/client --target appGoGame --parallel 3
./build/client/appGoGame \
  --katago-engine "$PWD/build/katago-engine/engine/squashfs-root/AppRun" \
  --katago-model /path/to/model.bin.gz --ai-time-ms 1800 --ai-threads 4
```

Optional `--katago-config /path/to/analysis.cfg` replaces the generated config. Reporting perspective is forced to `SIDETOMOVE` and each request supplies its time limit. Other limits in a custom config may stop search earlier. Without a custom config, use `--katago-fp32` to disable FP16.

Launcher environment variables: `GOGAME_KATAGO_ARCHIVE`, `GOGAME_KATAGO_ENGINE` (already runnable executable/AppRun), `GOGAME_KATAGO_MODEL`, `GOGAME_AI_ROOT` (default model directory), `GOGAME_KATAGO_BUILD`, `GOGAME_KATAGO_LIBRARY_DIR`, and `GOGAME_BUILD_JOBS`.

The previous native inference integration remains available through `tools/run-katago-native.sh`; its instructions are in [KATAGO_NATIVE.md](KATAGO_NATIVE.md).

Protocol references: [KataGo Analysis Engine](https://github.com/lightvector/KataGo/blob/v1.18.1/docs/Analysis_Engine.md), [KataGo Rules](https://github.com/lightvector/KataGo/blob/v1.18.1/docs/GTP_Extensions.md#kata-go-gtp-extensions).
