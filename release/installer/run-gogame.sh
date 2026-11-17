#!/usr/bin/env bash
set -euo pipefail
game_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$game_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$game_root/plugins"
export QML_IMPORT_PATH="$game_root/qml"
export QML2_IMPORT_PATH="$game_root/qml"
exec "$game_root/appGoGame" --katago-model "$game_root/models/model.bin.gz" "$@"
