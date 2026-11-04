# GoGame Releases

This directory keeps the five desktop release profiles separate while using the same client source code. CPU profiles enable KataGo neural inference with Eigen; they do not disable the model or switch to heuristic-only Hard AI. CUDA profiles enable NVIDIA inference. AI, Sandbox and local saves stay in the client. These profiles build only `appGoGame`; the online server is deployed separately.

| Profile | Build host | Hard AI backend | Packaging tool |
| --- | --- | --- | --- |
| `windows-cpu` | Windows | Eigen CPU | `windeployqt`, then ZIP/installer |
| `linux-cpu` | Linux | Eigen CPU | `linuxdeployqt`, then AppImage |
| `macos-cpu` | macOS | Eigen CPU | `macdeployqt`, then DMG |
| `windows-cuda` | Windows | CUDA + cuDNN | `windeployqt`, NVIDIA runtime deployment, then ZIP/installer |
| `linux-cuda` | Linux | CUDA + cuDNN | `linuxdeployqt`, NVIDIA runtime deployment, then AppImage |

## Directory layout

```text
release/
├── README.md
├── CMakePresets.json
├── stage.cmake
├── cmake/common.json
├── linux/
│   ├── CMakePresets.json
│   └── run-gogame.sh
├── windows/
│   ├── CMakePresets.json
│   └── run-gogame.cmd
├── macos/
│   ├── CMakePresets.json
│   └── run-gogame.command
├── build/<profile>/
├── staging/<profile>/
└── packages/<profile>/
```

The root `CMakePresets.json` only includes this directory's presets. Configuration and launcher templates are tracked in Git; `build`, `staging` and `packages` contain generated files and are ignored. The generated directories are created when used.

## Requirements

- CMake 3.23 or newer, Ninja, and a C++17 compiler.
- Qt 6 and the modules required by the project, including QML, Quick, Sql, WebSockets, Concurrent and Multimedia.
- zlib; CPU profiles also require Eigen 3.4 or an explicit Eigen header directory.
- CUDA profiles require a compatible NVIDIA GPU/driver, CUDA toolkit and cuDNN. Select target GPU architectures explicitly when preparing a CUDA release.
- On Windows, use a Qt MSVC kit and an MSVC developer terminal with Ninja available. CUDA releases require an MSVC toolset supported by the selected CUDA toolkit.
- A KataGo playing model. The staging default is `tools/b10c384h6nbttflrs.bin.gz`.

Build on the target operating system. Each profile uses the selected compiler's architecture; it is not a universal or cross-compilation preset. Use separate build directories/checkouts for different architectures. Windows and macOS profiles still require native build and runtime verification.

## Configure and build

Run from the project root. CMake lists only the profiles enabled for the current host:

```sh
cmake --list-presets=all
```

For example, build the standard Linux release:

```sh
cmake --preset linux-cpu
cmake --build --preset linux-cpu
```

Use `windows-cpu`, `macos-cpu`, `linux-cuda` or `windows-cuda` in both commands for another profile. All presets explicitly select `Release` and enable the KataGo backend. Build output is kept under `release/build/<profile>` and the client executable/bundle is written to its `bin` directory. The existing `build/Desktop_Release` directory is not moved or changed by these presets.

Machine-specific paths belong in command-line overrides or a gitignored root `CMakeUserPresets.json`, not in the shared profiles. Available configure overrides include:

| CMake variable | Purpose |
| --- | --- |
| `CMAKE_PREFIX_PATH` | Qt and dependency installation prefixes |
| `CMAKE_TOOLCHAIN_FILE` | Dependency/toolchain integration, such as vcpkg |
| `GOGAME_EIGEN_INCLUDE_DIR` | Header directory containing `Eigen/` |
| `GOGAME_KATAGO_SOURCE_DIR` | Offline KataGo v1.18.1 source containing `cpp/` |
| `CMAKE_CUDA_COMPILER` | Path to `nvcc` |
| `CMAKE_CUDA_ARCHITECTURES` | GPU architectures to include in a CUDA release |
| `GOGAME_CUDNN_INCLUDE` | Directory containing `cudnn.h` |
| `GOGAME_CUDNN_LIBRARY` | cuDNN shared-library import path (`.so` on Linux or `.lib` on Windows) |

For example, append `-DGOGAME_EIGEN_INCLUDE_DIR=/path/to/eigen` to the CPU configure command. If no offline KataGo source is supplied, the existing backend integration downloads its pinned upstream source. Changing dependency locations after configuration may require a fresh build directory.

## Prepare files for packaging

After a successful build, stage that profile:

```sh
cmake -DGOGAME_RELEASE=linux-cpu -P release/stage.cmake
```

The same command accepts any of the five profile names. It checks that the build is Release, enables KataGo and matches the requested CPU/CUDA backend before copying the client, model and platform launcher. `release.json` records the profile, backend and UTC preparation time, with `packaged` set to `false`.

To stage an existing compatible build instead of a preset build:

```sh
cmake -DGOGAME_RELEASE=linux-cuda -DGOGAME_BUILD_DIR=build/Desktop_Release -P release/stage.cmake
```

Override the model with `-DGOGAME_KATAGO_MODEL=/path/to/model.bin.gz`. Existing builds must use the same single-configuration Release layout, with the client at the build root or in `bin`. Staging copies files; it does not rebuild or certify that an existing binary matches the latest source.

Linux staging contains `appGoGame`, `models/model.bin.gz`, `run-gogame.sh` and `release.json`. Windows uses `appGoGame.exe` and `run-gogame.cmd`. macOS keeps the bundle as `appGoGame.app`, places the model in `Contents/Resources/models` and supplies `run-gogame.command` alongside it. Launchers accept the existing client options and a `GOGAME_KATAGO_MODEL` environment override.

## Packaging later

Staging is an input to packaging, not a self-contained application. Before distribution, deploy the matching Qt libraries, QML modules, plugins and compiler/runtime dependencies into a separate working copy of the staged directory. Preserve the SQLite driver and multimedia support. For CUDA editions, also deploy the required CUDA/cuDNN runtime libraries, including dynamically loaded components; keep the NVIDIA driver on the destination machine.

- Linux: use `linuxdeployqt` on the staged ELF executable, scan the project QML, and make AppImage's `AppRun` launch `run-gogame.sh`. Add a portable desktop entry and icon rather than reusing absolute development paths. An AppImage built on a recent Arch system is not guaranteed to run on older distributions.
- Windows: use `windeployqt` on the staged `.exe`; make the shortcut or installer launch `run-gogame.cmd` so the model argument is supplied.
- macOS: use `macdeployqt` on the staged `.app`. The current executable loads the model only when `--katago-model` is supplied: use the staged command launcher, or implement bundle-relative model discovery before advertising direct Finder launch with neural Hard AI. Final signing/notarization follows all bundle modifications.

Put finished outputs in `release/packages/<profile>`, with version and architecture in the filename, for example `GoGame-0.1-linux-x86_64-cpu.AppImage`. Verify each package on its target OS and hardware before distribution. Do not include development `.a` files, source trees, build caches, personal saves or authentication databases.

## References

- [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
- [Qt deployment](https://doc.qt.io/qt-6/deployment.html)
- [KataGo v1.18.1 compilation](https://github.com/lightvector/KataGo/blob/v1.18.1/Compiling.md)
