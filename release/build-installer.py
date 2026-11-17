import argparse
import datetime
import hashlib
import json
import os
import platform
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


def run(arguments, **kwargs):
    print("Running:", subprocess.list2cmdline([str(value) for value in arguments]), flush=True)
    return subprocess.run([str(value) for value in arguments], check=True, **kwargs)


def tool(value, name):
    result = value or shutil.which(name)
    if not result or not Path(result).is_file():
        raise RuntimeError(f"Missing {name}. Install it or supply its path with the matching option.")
    return str(Path(result).resolve())


def render(source, target, values):
    content = source.read_text(encoding="utf-8")
    for name, value in values.items():
        content = content.replace("@" + name + "@", value)
    ET.fromstring(content)
    target.write_text(content, encoding="utf-8")


def check_binary(path, host):
    with path.open("rb") as stream:
        header = stream.read(64)
        if host == "Linux":
            valid = header[:6] == b"\x7fELF\x02\x01" and header[18:20] == b"\x3e\x00"
        elif header[:2] == b"MZ" and len(header) == 64:
            stream.seek(int.from_bytes(header[60:64], "little"))
            valid = stream.read(6) == b"PE\x00\x00\x64\x86"
        else:
            valid = False
    if not valid:
        raise RuntimeError(f"Expected a native x86_64 {host} executable: {path}")


def linux(payload, args, game_root):
    deployqt = tool(args.linuxdeployqt, "linuxdeployqt")
    qmake = tool(args.qmake or shutil.which("qmake6") or shutil.which("qmake"), "qmake6")
    if not subprocess.check_output([qmake, "-query", "QT_VERSION"], text=True).strip().startswith("6."):
        raise RuntimeError("Select the Qt 6 qmake matching the application build with --qmake.")
    plugin_root = Path(subprocess.check_output([qmake, "-query", "QT_INSTALL_PLUGINS"], text=True).strip())
    excluded = [path.stem for path in (plugin_root / "sqldrivers").glob("*.so") if path.name != "libqsqlite.so"]
    env = os.environ.copy()
    env.update(APPIMAGELAUNCHER_DISABLE="1", APPIMAGE_EXTRACT_AND_RUN="1")
    command = [deployqt, payload / "appGoGame", "-bundle-non-qt-libs", "-no-strip",
               f"-qmake={qmake}", f"-qmldir={game_root / 'qml'}",
               "-extra-plugins=sqldrivers/libqsqlite.so,multimedia,tls,platforms/libqoffscreen.so,platforms/libqwayland.so"]
    if args.allow_new_glibc:
        command.append("-unsupported-allow-new-glibc")
    if excluded:
        command.append("-exclude-libs=" + ",".join(excluded))
    run(command, env=env, timeout=600)
    shutil.copy2(game_root / "release/installer/run-gogame.sh", payload / "run-gogame.sh")
    (payload / "run-gogame.sh").chmod(0o755)
    (payload / "qt.conf").write_text("[Paths]\nPrefix=.\nPlugins=plugins\nQmlImports=qml\n", encoding="utf-8")
    env["LD_LIBRARY_PATH"] = str(payload / "lib")
    for binary in [payload / "appGoGame", *payload.rglob("*.so*")]:
        if not binary.is_file():
            continue
        with binary.open("rb") as stream:
            if stream.read(4) != b"\x7fELF":
                continue
        result = subprocess.run(["ldd", str(binary)], env=env, capture_output=True, text=True)
        if "not found" in result.stdout + result.stderr:
            raise RuntimeError(f"Unresolved dependencies in {binary}:\n{result.stdout}{result.stderr}")
        for line in result.stdout.splitlines():
            if "libQt6" in line and "=>" in line:
                dependency = line.split("=>", 1)[1].strip().split(" (", 1)[0]
                if not Path(dependency).resolve().is_relative_to(payload):
                    raise RuntimeError(f"Qt dependency was not bundled: {dependency}")
    if not (payload / "plugins/platforms/libqxcb.so").is_file():
        raise RuntimeError("Deployment is missing the Qt xcb platform plugin")
    if not (payload / "plugins/sqldrivers/libqsqlite.so").is_file():
        raise RuntimeError("Deployment is missing the SQLite plugin")
    if not (payload / "plugins/tls/libqopensslbackend.so").is_file():
        raise RuntimeError("Deployment is missing the OpenSSL TLS plugin")
    for name in ("qt6-base", "qt6-declarative", "qt6-multimedia", "qt6-websockets", "qt6-wayland", "qt6-svg"):
        license_dir = Path("/usr/share/licenses") / name
        if license_dir.is_dir():
            shutil.copytree(license_dir, payload / "licenses" / name, dirs_exist_ok=True)


def windows(payload, args, game_root):
    deployqt = tool(args.windeployqt, "windeployqt")
    run([deployqt, "--release", "--compiler-runtime", "--qmldir", game_root / "qml",
         "--dir", payload, "--plugindir", payload / "plugins", payload / "appGoGame.exe"])
    redist = os.environ.get("VCToolsRedistDir")
    if redist:
        for dll in Path(redist).glob("x64/Microsoft.VC*.CRT/*.dll"):
            shutil.copy2(dll, payload / dll.name)
    directories = [payload, Path(deployqt).parent, *map(Path, args.runtime_dir)]
    directories.extend(Path(value) for value in os.environ.get("PATH", "").split(os.pathsep) if value)
    run(["cmake", f"-Dpayload={payload.as_posix()}",
         "-Druntime_dirs=" + ";".join(directory.resolve().as_posix() for directory in directories),
         "-P", game_root / "release/installer/collect-runtime.cmake"])
    if not list(payload.glob("msvcp140*.dll")) or not list(payload.glob("vcruntime140*.dll")):
        raise RuntimeError("MSVC runtime DLLs are missing. Use an x64 MSVC developer terminal with VCToolsRedistDir set.")
    if not (payload / "plugins/sqldrivers/qsqlite.dll").is_file():
        raise RuntimeError("Deployment is missing the SQLite plugin")
    (payload / "qt.conf").write_text("[Paths]\nPrefix=.\nPlugins=plugins\nQmlImports=qml\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description="Build an offline GoGame CPU installer on the target OS.")
    parser.add_argument("--model", type=Path)
    parser.add_argument("--binarycreator", default=os.environ.get("IFW_BINARYCREATOR"))
    parser.add_argument("--linuxdeployqt")
    parser.add_argument("--qmake")
    parser.add_argument("--windeployqt")
    parser.add_argument("--runtime-dir", action="append", default=[])
    parser.add_argument("--allow-new-glibc", action="store_true",
                        help="Allow a local Linux build; this does not provide compatibility with older glibc systems.")
    parser.add_argument("--skip-build", action="store_true", help="Package the existing matching Release CPU build.")
    parser.add_argument("--work-dir", type=Path, help="Reuse an installer workspace under release/build after a failed deployment.")
    parser.add_argument("--prepare-only", action="store_true", help="Deploy the payload and generate IFW inputs without binarycreator.")
    args = parser.parse_args()
    host = platform.system()
    if host not in ("Linux", "Windows") or platform.machine().lower() not in ("x86_64", "amd64"):
        raise RuntimeError("Run this script on x86_64 Linux or Windows with an x64 Qt/compiler kit.")
    selected = host.lower()
    profile = selected + "-cpu"
    binarycreator = None if args.prepare_only else tool(args.binarycreator, "binarycreator")
    game_root = Path(__file__).resolve().parent.parent
    model_path = args.model
    if model_path is None:
        model_path = next((path for path in [game_root / "tools/b10c384h6nbttflrs.bin.gz",
                          game_root / f"release/staging/{profile}/models/model.bin.gz",
                          game_root / "release/staging/linux-cuda/models/model.bin.gz"] if path.is_file()), None)
    if model_path is None or not model_path.is_file():
        raise RuntimeError("Supply the KataGo playing model with --model /path/to/model.bin.gz.")
    model_path = model_path.resolve()
    with model_path.open("rb") as stream:
        if stream.read(2) != b"\x1f\x8b":
            raise RuntimeError("The model must be a gzip file, not a Git LFS pointer. Run git lfs pull if needed.")
    version = re.search(r"project\(GoGame VERSION ([\d.]+)", (game_root / "CMakeLists.txt").read_text()).group(1)
    if not args.skip_build:
        run(["cmake", "--preset", profile], cwd=game_root)
        run(["cmake", "--build", "--preset", profile], cwd=game_root)
    build_path = game_root / "release/build" / profile
    cache = (build_path / "CMakeCache.txt").read_text()
    for pattern in [r"^CMAKE_BUILD_TYPE:[^=]+=Release$", r"^GOGAME_KATAGO_BACKEND:[^=]+=EIGEN$",
                    r"^GOGAME_ENABLE_KATAGO:[^=]+=(?:ON|TRUE|1)$"]:
        if not re.search(pattern, cache, re.MULTILINE | re.IGNORECASE):
            raise RuntimeError(f"{profile} must be a Release build with KataGo enabled and the EIGEN backend.")
    template = game_root / "release/installer"
    if args.work_dir:
        work = args.work_dir.resolve()
        if not work.is_relative_to(game_root / "release/build"):
            raise RuntimeError("The installer workspace must be inside release/build.")
        work.mkdir(parents=True, exist_ok=True)
    else:
        work = Path(tempfile.mkdtemp(prefix=profile + "-", dir=game_root / "release/build"))
    component = work / "packages/com.gogame.client"
    payload, meta, config = component / "data", component / "meta", work / "config"
    for directory in (payload / "models", meta, config):
        directory.mkdir(parents=True, exist_ok=True)
    binary = "appGoGame.exe" if host == "Windows" else "appGoGame"
    check_binary(build_path / "bin" / binary, host)
    shutil.copy2(build_path / "bin" / binary, payload / binary)
    shutil.copy2(model_path, payload / "models/model.bin.gz")
    for directory in (payload, config):
        shutil.copy2(game_root / "assets/stones/black1.png", directory / "gogame.png")
    print("Installer workspace:", work, flush=True)
    if host == "Linux":
        linux(payload, args, game_root)
    else:
        windows(payload, args, game_root)
    licenses = list((build_path / "_deps/gogame_katago_source-src").glob("LICENSE*"))
    if licenses:
        (payload / "licenses/KataGo").mkdir(parents=True, exist_ok=True)
        for license_file in licenses:
            shutil.copy2(license_file, payload / "licenses/KataGo" / license_file.name)
    values = {"VERSION": version, "DATE": datetime.date.today().isoformat(),
              "RUN_PROGRAM": "@TargetDir@/" + (binary if host == "Windows" else "run-gogame.sh"),
              "RUN_ARGUMENTS": '<RunProgramArguments><Argument>--katago-model</Argument><Argument>@TargetDir@/models/model.bin.gz</Argument></RunProgramArguments>' if host == "Windows" else ""}
    render(template / "config.xml.in", config / "config.xml", values)
    render(template / "package.xml.in", meta / "package.xml", values)
    shutil.copy2(template / "installscript.qs", meta / "installscript.qs")
    manifest = {"profile": profile, "version": version, "architecture": "x86_64", "backend": "EIGEN",
                "model": "models/model.bin.gz", "packaged": True,
                "preparedAtUtc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                "buildLibc": platform.libc_ver() if host == "Linux" else None}
    (payload / "release.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    if args.prepare_only:
        print("Prepared IFW inputs:", work)
        return
    output = game_root / "release/packages" / profile
    output.mkdir(parents=True, exist_ok=True)
    artifact = output / f"GoGame-{version}-{selected}-x86_64-Setup{'.exe' if host == 'Windows' else '.run'}"
    if artifact.exists():
        raise RuntimeError(f"Output already exists: {artifact}. Move it before building another installer.")
    run([binarycreator, "--offline-only", "--compression", "3", "-c", config / "config.xml", "-p", work / "packages", artifact])
    if host == "Linux":
        artifact.chmod(0o755)
    checksum = hashlib.sha256()
    with artifact.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            checksum.update(chunk)
    digest = checksum.hexdigest()
    artifact.with_suffix(artifact.suffix + ".sha256").write_text(f"{digest}  {artifact.name}\n", encoding="utf-8")
    print("Offline installer:", artifact)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
        sys.exit(str(error))
