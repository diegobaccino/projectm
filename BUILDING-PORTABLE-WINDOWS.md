# Building Portable projectMSDL on Windows (Unified Repo)

This is the current, working process to build and package a portable app from this repo.

## Scope

- Core projectM source: this repo root
- Frontend source: frontend-sdl-cpp inside this repo
- Portable output target: C:/Users/diegobaccino/Downloads/ProjectMPortable

## Profile Feature Status

The profile system is implemented in this branch:

- Profile picker on startup (interactive terminal launches)
- Create/delete profile UI
- File menu profile switcher
- Numeric profile shortcuts `Ctrl+1` to `Ctrl+9`
- Per-profile last-used preset persistence

## Dependency Policy (Repo-local)

Use repo-local dependency paths so builds are reproducible and do not rely on ad-hoc folders under Downloads.

- Keep vcpkg inside the repo at: C:/Source/projectm/.deps/vcpkg
- Keep build trees inside repo (build-copilot-vs-merged, frontend-sdl-cpp/build-merged)
- Keep runtime dependency discovery from frontend-sdl-cpp/build-merged/vcpkg_installed/x64-windows/bin

Note: this repo already contains source-side vendor dependencies under vendor and frontend-sdl-cpp/vendor.

## One-time Setup

```powershell
# 1) Repo-local vcpkg checkout
if (!(Test-Path C:/Source/projectm/.deps/vcpkg)) {
  git clone https://github.com/microsoft/vcpkg C:/Source/projectm/.deps/vcpkg
}

# 2) Bootstrap vcpkg
& 'C:/Source/projectm/.deps/vcpkg/bootstrap-vcpkg.bat'
```

## Preflight (Avoid Stale vcpkg Lock)

Run this before configure if a previous build was interrupted:

```powershell
Get-Process vcpkg,cmake,cl -ErrorAction SilentlyContinue | Stop-Process -Force

$locks = @(
  'C:/Source/projectm/build-copilot-vs-merged/vcpkg_installed/vcpkg/vcpkg-running.lock',
  'C:/Source/projectm/frontend-sdl-cpp/build-merged/vcpkg_installed/vcpkg/vcpkg-running.lock'
)
foreach ($lock in $locks) {
  if (Test-Path $lock) { Remove-Item -Force $lock }
}
```

## Tool Paths

```powershell
$cm = 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$toolchain = 'C:/Source/projectm/.deps/vcpkg/scripts/buildsystems/vcpkg.cmake'
```

## 1) Configure + Build + Install Core

```powershell
& $cm -S C:/Source/projectm -B C:/Source/projectm/build-copilot-vs-merged `
  -G 'Visual Studio 17 2022' -A x64 `
  -DCMAKE_INSTALL_PREFIX=C:/Source/projectm/.local/install-core `
  -DCMAKE_TOOLCHAIN_FILE=$toolchain `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DBUILD_TESTING=OFF

& $cm --build C:/Source/projectm/build-copilot-vs-merged --config Release --target INSTALL --parallel
```

Expected outputs:

- C:/Source/projectm/.local/install-core/bin/projectM-4.dll
- C:/Source/projectm/.local/install-core/bin/projectM-4-playlist.dll

## 2) Configure + Build Frontend

```powershell
& $cm -S C:/Source/projectm/frontend-sdl-cpp -B C:/Source/projectm/frontend-sdl-cpp/build-merged `
  -G 'Visual Studio 17 2022' -A x64 `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_INSTALL_PREFIX=C:/Source/projectm/.local/install-frontend `
  -DCMAKE_TOOLCHAIN_FILE=$toolchain `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DCMAKE_PREFIX_PATH=C:/Source/projectm/.local/install-core `
  -DprojectM4_DIR=C:/Source/projectm/.local/install-core/lib/cmake/projectM4

& $cm --build C:/Source/projectm/frontend-sdl-cpp/build-merged --config Release --parallel
```

Expected output:

- C:/Source/projectm/frontend-sdl-cpp/build-merged/src/Release/projectMSDL.exe

## 3) Assemble Portable Folder (Requested Location)

```powershell
$portable = 'C:/Users/diegobaccino/Downloads/ProjectMPortable'
if (Test-Path $portable) { Remove-Item -Recurse -Force $portable }
New-Item -ItemType Directory -Path $portable | Out-Null

# EXE
Copy-Item C:/Source/projectm/frontend-sdl-cpp/build-merged/src/Release/projectMSDL.exe $portable

# Runtime DLLs from frontend build + core install
Copy-Item C:/Source/projectm/frontend-sdl-cpp/build-merged/vcpkg_installed/x64-windows/bin/*.dll $portable -Force
Copy-Item C:/Source/projectm/.local/install-core/bin/*.dll $portable -Force

# Presets from repo
Copy-Item C:/Source/projectm/presets (Join-Path $portable 'presets') -Recurse -Force
```

## 4) Validate Portable Contents

```powershell
$portable = 'C:/Users/diegobaccino/Downloads/ProjectMPortable'
"exeExists=$((Test-Path (Join-Path $portable 'projectMSDL.exe')))"
"dllCount=$((Get-ChildItem $portable -Filter *.dll -ErrorAction SilentlyContinue).Count)"
"presetCount=$((Get-ChildItem (Join-Path $portable 'presets') -Recurse -File -ErrorAction SilentlyContinue).Count)"
```

## Build Notes from Recent Run

- Long first-time frontend configure is expected while vcpkg builds Poco/SDL2 and related dependencies.
- If configure appears stalled, check for vcpkg lock files and leftover vcpkg/cmake processes.
- Prefer Visual Studio generator over Ninja unless Ninja is explicitly installed and available.

## Logo Overlay Defaults in Current Code

Current defaults have been aligned to:

- Anchor: Center
- Offset X: 0
- Offset Y: 0

Settings are also now auto-persisted when the settings window is closed with unsaved changes.
