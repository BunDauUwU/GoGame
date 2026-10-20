#!/usr/bin/env bash
set -euo pipefail
# Build the native neural backend and launch Hard mode with the user's model.
game_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
ai_root="${GOGAME_AI_ROOT:-$game_root/../DatasetGogame/AI}"
model_path="${GOGAME_KATAGO_MODEL:-$ai_root/kata1-tf3-b11c768-s11003M-d5973M-7gres.bin.gz}"
if [[ $# -gt 0 && "$1" != --* ]]; then model_path="$1"; shift; fi
if [[ ! -f "$model_path" ]]; then printf 'Model not found: %s\n' "$model_path" >&2; exit 2; fi
build_path="${GOGAME_KATAGO_BUILD:-$game_root/build/katago-native}"
backend="${GOGAME_KATAGO_BACKEND:-CUDA}"
cmake_options=(-S "$game_root" -B "$build_path" -G Ninja
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DGOGAME_ENABLE_KATAGO=ON
    "-DGOGAME_KATAGO_BACKEND=$backend")
source_path="${GOGAME_KATAGO_SOURCE_DIR:-$ai_root/training/match-tools/KataGo-1.18.1}"
if [[ -d "$source_path/cpp" ]]; then cmake_options+=("-DGOGAME_KATAGO_SOURCE_DIR=$source_path"); fi
if [[ "$backend" == CUDA ]]; then
    cmake_options+=("-DCMAKE_CUDA_ARCHITECTURES=${GOGAME_CUDA_ARCHITECTURES:-89}")
    if [[ -x /opt/cuda/bin/nvcc ]]; then cmake_options+=(-DCMAKE_CUDA_COMPILER=/opt/cuda/bin/nvcc); fi
    for cudnn_path in "$ai_root"/.venv-ai/lib/python*/site-packages/nvidia/cudnn; do
        if [[ -f "$cudnn_path/include/cudnn.h" && -f "$cudnn_path/lib/libcudnn.so.9" ]]; then
            cmake_options+=("-DGOGAME_CUDNN_INCLUDE=$cudnn_path/include" "-DGOGAME_CUDNN_LIBRARY=$cudnn_path/lib/libcudnn.so.9")
            export LD_LIBRARY_PATH="$cudnn_path/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
            break
        fi
    done
    for cuda_lib_path in "$ai_root"/.venv-ai/lib/python*/site-packages/nvidia/cu13/lib; do
        if [[ -f "$cuda_lib_path/libcudart.so.13" ]]; then
            export LD_LIBRARY_PATH="$cuda_lib_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
            break
        fi
    done
elif [[ -d "$ai_root/training/match-tools/eigen-3.4.0/Eigen" ]]; then
    cmake_options+=("-DGOGAME_EIGEN_INCLUDE_DIR=$ai_root/training/match-tools/eigen-3.4.0")
fi
cmake "${cmake_options[@]}"
cmake --build "$build_path" --target appGoGame -j "${GOGAME_BUILD_JOBS:-2}"
exec "$build_path/appGoGame" --katago-model "$model_path" "$@"
