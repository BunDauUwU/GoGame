#!/usr/bin/env bash
set -euo pipefail

game_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
model_path="${GOGAME_KATAGO_MODEL:-$game_root/models/model.bin.gz}"

if [[ ! -f "$model_path" ]]; then
    printf 'Model not found: %s\n' "$model_path" >&2
    exit 2
fi

exec "$game_root/appGoGame" --katago-model "$model_path" "$@"
