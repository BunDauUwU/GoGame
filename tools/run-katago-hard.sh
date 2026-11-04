#!/usr/bin/env bash
set -euo pipefail

game_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_path="${GOGAME_KATAGO_BUILD:-$game_root}"
if [[ -z "${GOGAME_KATAGO_BUILD:-}" && ! -x "$build_path/appGoGame" ]]; then
    build_path="$game_root/build/Desktop_Release"
fi
model_path="${GOGAME_KATAGO_MODEL:-$game_root/tools/b10c384h6nbttflrs.bin.gz}"

if [[ $# -gt 0 && "$1" != --* ]]; then
    model_path="$1"
    shift
fi
if [[ ! -x "$build_path/appGoGame" ]]; then
    printf 'Client executable not found: %s/appGoGame\n' "$build_path" >&2
    exit 2
fi
if [[ ! -f "$model_path" ]]; then
    printf 'Model not found: %s\n' "$model_path" >&2
    exit 2
fi

exec "$build_path/appGoGame" --katago-model "$model_path" "$@"
