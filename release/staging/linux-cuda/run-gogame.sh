#!/usr/bin/env bash
set -euo pipefail

game_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_root="$game_root"
while [[ ! -f "$project_root/CMakeLists.txt" && "$project_root" != / ]]; do
    project_root="$(dirname -- "$project_root")"
done

if [[ "$game_root" == "$project_root/release/linux" ]]; then
    profile="${GOGAME_RELEASE:-linux-cpu}"
    case "$profile" in
        linux-cpu|linux-cuda) ;;
        *)
            printf 'Unsupported Linux profile: %s\n' "$profile" >&2
            exit 2
            ;;
    esac
    build_path="$project_root/release/staging/$profile"
    if [[ ! -x "$build_path/appGoGame" || ! -f "$build_path/run-gogame.sh" ]]; then
        printf 'Stage %s first: cmake -DGOGAME_RELEASE=%s -P release/stage.cmake\n' \
            "$profile" "$profile" >&2
        exit 2
    fi
    printf 'Launching staged profile: %s\n' "$profile" >&2
    exec bash "$build_path/run-gogame.sh" "$@"
fi

build_path="${GOGAME_KATAGO_BUILD:-$game_root}"
model_path="${GOGAME_KATAGO_MODEL:-$game_root/models/model.bin.gz}"

if [[ ! -x "$build_path/appGoGame" ]]; then
    printf 'Client executable not found: %s/appGoGame\n' "$build_path" >&2
    exit 2
fi
if [[ ! -f "$model_path" ]]; then
    printf 'Model not found: %s\n' "$model_path" >&2
    exit 2
fi

runtime_paths=""
for runtime_path in "${GOGAME_CUDNN_LIB_DIR:-}" "$build_path/lib" "$build_path/usr/lib" \
                    "$project_root/release/runtime/cuda/lib"; do
    if [[ -n "$runtime_path" && -d "$runtime_path" ]]; then
        runtime_paths="${runtime_paths:+$runtime_paths:}$runtime_path"
    fi
done
if [[ -n "$runtime_paths" ]]; then
    export LD_LIBRARY_PATH="$runtime_paths${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

exec "$build_path/appGoGame" --katago-model "$model_path" "$@"
