#!/usr/bin/env bash
set -euo pipefail
game_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
ai_root="${GOGAME_AI_ROOT:-$game_root/src/AI}"
archive_path="${GOGAME_KATAGO_ARCHIVE:-$game_root/src/AI/katago-v1.18.1-cuda12.1-cudnn8.9.7-linux-x64+bs50.zip}"
build_path="${GOGAME_KATAGO_BUILD:-$game_root/build/katago-engine}"
engine_path="${GOGAME_KATAGO_ENGINE:-}"
model_path="${GOGAME_KATAGO_MODEL:-$ai_root/kata1-tf3-b11c768-s11003M-d5973M-7gres.bin.gz}"
extract_only=false
check_only=false
if [[ "${1:-}" == --help || "${1:-}" == -h ]]; then
    printf '%s\n' \
        'Usage: tools/run-katago-hard.sh [model.bin.gz] [client options]' \
        '       tools/run-katago-hard.sh --extract-only' \
        '       tools/run-katago-hard.sh --check' \
        'Uses the KataGo ZIP in src/AI for Hard mode; no native CUDA compilation.' \
        'Options: --ai-time-ms 1800 --ai-threads 4 --katago-fp32' \
        'Environment: GOGAME_KATAGO_ARCHIVE, GOGAME_KATAGO_ENGINE, GOGAME_KATAGO_MODEL,' \
        'GOGAME_KATAGO_BUILD, GOGAME_KATAGO_LIBRARY_DIR, GOGAME_BUILD_JOBS.'
    exit 0
fi
if [[ "${1:-}" == --extract-only ]]; then extract_only=true; shift; fi
if [[ "${1:-}" == --check ]]; then check_only=true; shift; fi
if [[ $# -gt 0 && "$1" != --* ]]; then model_path="$1"; shift; fi
mkdir -p -- "$build_path"
build_path="$(cd -- "$build_path" && pwd)"
if [[ -z "$engine_path" ]]; then
    command -v python3 >/dev/null || { printf 'python3 is required to unpack KataGo.\n' >&2; exit 2; }
    engine_dir="$build_path/engine"
    python3 - "$archive_path" "$engine_dir" <<'PY'
import hashlib
import os
from pathlib import Path
import sys
from zipfile import ZipFile

archive, target = map(Path, sys.argv[1:])
if not archive.is_file():
    raise SystemExit(f'KataGo archive not found: {archive}')
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
stamp = target / '.archive-sha256'
if not stamp.is_file() or stamp.read_text() != digest or not (target / 'katago').is_file():
    target.mkdir(parents=True, exist_ok=True)
    with ZipFile(archive) as package:
        for name in ('katago', 'analysis_example.cfg', 'default_gtp.cfg', 'README.txt'):
            path = target / name
            temporary = path.with_suffix('.tmp')
            temporary.write_bytes(package.read(name))
            if name == 'katago':
                temporary.chmod(0o755)
            os.replace(temporary, path)
    stamp.write_text(digest)
    (target / '.appimage-sha256').unlink(missing_ok=True)
PY
    engine_path="$engine_dir/katago"
    marker="$(od -An -tx1 -j8 -N3 "$engine_path" | tr -d ' \n')"
    if [[ "$marker" == 414902 ]]; then
        if [[ ! -f "$engine_dir/.appimage-sha256" || ! -x "$engine_dir/squashfs-root/AppRun" ]]; then
            (cd -- "$engine_dir" && APPIMAGELAUNCHER_DISABLE=1 ./katago --appimage-extract >appimage-extract.log 2>&1)
            cp -- "$engine_dir/.archive-sha256" "$engine_dir/.appimage-sha256"
        fi
        engine_path="$engine_dir/squashfs-root/AppRun"
    fi
fi
if [[ ! -x "$engine_path" ]]; then printf 'KataGo executable not found: %s\n' "$engine_path" >&2; exit 2; fi
engine_path="$(realpath -- "$engine_path")"
if $extract_only; then printf 'KataGo executable: %s\n' "$engine_path"; exit 0; fi
export APPIMAGELAUNCHER_DISABLE=1
if [[ -n "${GOGAME_KATAGO_LIBRARY_DIR:-}" ]]; then
    export LD_LIBRARY_PATH="$GOGAME_KATAGO_LIBRARY_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
if ! "$engine_path" version; then
    printf '%s\n' \
        'Cannot run the packaged KataGo engine.' \
        'This ZIP requires CUDA 12.x and cuDNN 8.x runtime libraries (CUDA 13/cuDNN 9 do not replace them).' \
        'Set GOGAME_KATAGO_LIBRARY_DIR to existing compatible library directories, separated by colons.' >&2
    exit 2
fi
if $check_only; then exit 0; fi
if [[ ! -f "$model_path" ]]; then
    printf 'Model not found: %s\nPass a model path or set GOGAME_KATAGO_MODEL.\n' "$model_path" >&2
    exit 2
fi
cmake -S "$game_root" -B "$build_path" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DGOGAME_ENABLE_KATAGO=OFF
cmake --build "$build_path" --target appGoGame -j "${GOGAME_BUILD_JOBS:-2}"
exec "$build_path/appGoGame" --katago-engine "$engine_path" --katago-model "$model_path" "$@"
