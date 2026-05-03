# Building The Portable Windows EXE (Unified Repo)

This document is the canonical process for rebuilding and updating the portable
`projectMSDL.exe` package from this unified repository.

## What Changed (Repo Unification)

- Core projectM code is in this repo root.
- Standalone UI frontend code is now in `frontend-sdl-cpp/` inside this same repo.
- The overlay/logo upload UI work lives in frontend sources (for example,
  `frontend-sdl-cpp/src/gui/SettingsWindow.*` and related files), while the core
  logo API lives in libprojectM.

If overlay settings are missing in the app, verify frontend code/branch content,
not just libprojectM API symbols.

## Prerequisites

- Windows with Visual Studio 2022 Build Tools installed.
- vcpkg checkout at:
  `C:/Users/diegobaccino/Downloads/vcpkg`
- CMake from Visual Studio Build Tools:
  `C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe`

## Important Lessons Learned

- Do not rely on `cmake` being in PATH.
- Do not use `-G Ninja` unless Ninja is definitely installed and discoverable.
  Using Visual Studio generator avoids this issue.
- Build/install core first, then point frontend to that local install using
  `CMAKE_PREFIX_PATH` and `projectM4_DIR`.
- For portability, include all DLLs from frontend vcpkg runtime bin and from the
  local core install bin.
- Presets/textures are required for a complete user experience; package them with
  the EXE.

## Paths Used By This Workflow

- Repo root:
  `C:/Source/projectm`
- Core build dir:
  `C:/Source/projectm/build-copilot-vs-merged`
- Core install dir:
  `C:/Users/diegobaccino/Downloads/projectm-local-install-merged`
- Frontend build dir:
  `C:/Source/projectm/frontend-sdl-cpp/build-merged`
- Frontend install dir (optional):
  `C:/Users/diegobaccino/Downloads/projectm-standalone-install-merged`
- Portable output folder:
  `C:/Users/diegobaccino/Downloads/projectm-standalone-portable-merged-latest`
- Portable zip:
  `C:/Users/diegobaccino/Downloads/projectm-standalone-portable-merged-latest.zip`

## 1) Configure + Build + Install Core (libprojectM)

```powershell
$cm = 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'

& $cm -S C:/Source/projectm -B C:/Source/projectm/build-copilot-vs-merged `
  -G 'Visual Studio 17 2022' -A x64 `
  -DCMAKE_INSTALL_PREFIX=C:/Users/diegobaccino/Downloads/projectm-local-install-merged `
  -DCMAKE_TOOLCHAIN_FILE=C:/Users/diegobaccino/Downloads/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DBUILD_TESTING=OFF

& $cm --build C:/Source/projectm/build-copilot-vs-merged --config Release --target INSTALL --parallel
```

Expected key outputs:

- `C:/Users/diegobaccino/Downloads/projectm-local-install-merged/bin/projectM-4.dll`
- `C:/Users/diegobaccino/Downloads/projectm-local-install-merged/bin/projectM-4-playlist.dll`
- `C:/Users/diegobaccino/Downloads/projectm-local-install-merged/include/projectM-4/logo_overlay.h`

## 2) Configure + Build Frontend (projectMSDL)

```powershell
$cm = 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'

& $cm -S C:/Source/projectm/frontend-sdl-cpp -B C:/Source/projectm/frontend-sdl-cpp/build-merged `
  -G 'Visual Studio 17 2022' -A x64 `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_INSTALL_PREFIX=C:/Users/diegobaccino/Downloads/projectm-standalone-install-merged `
  -DCMAKE_TOOLCHAIN_FILE=C:/Users/diegobaccino/Downloads/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DCMAKE_PREFIX_PATH=C:/Users/diegobaccino/Downloads/projectm-local-install-merged `
  -DprojectM4_DIR=C:/Users/diegobaccino/Downloads/projectm-local-install-merged/lib/cmake/projectM4

& $cm --build C:/Source/projectm/frontend-sdl-cpp/build-merged --config Release --parallel
```

Expected output:

- `C:/Source/projectm/frontend-sdl-cpp/build-merged/src/Release/projectMSDL.exe`

## 3) Assemble Portable Folder

```powershell
$portable = 'C:/Users/diegobaccino/Downloads/projectm-standalone-portable-merged-latest'
if (Test-Path $portable) { Remove-Item -Recurse -Force $portable }
New-Item -ItemType Directory -Path $portable | Out-Null

# EXE
Copy-Item C:/Source/projectm/frontend-sdl-cpp/build-merged/src/Release/projectMSDL.exe $portable

# Runtime DLLs from frontend vcpkg and local core install
Copy-Item C:/Source/projectm/frontend-sdl-cpp/build-merged/vcpkg_installed/x64-windows/bin/*.dll $portable -Force
Copy-Item C:/Users/diegobaccino/Downloads/projectm-local-install-merged/bin/*.dll $portable -Force

# Assets (presets/textures)
# Preferred: copy from your latest known-good portable bundle that already has the full collection.
$assetSource = (Get-ChildItem C:/Users/diegobaccino/Downloads -Directory |
  Where-Object {
    $_.Name -like 'projectm-standalone-portable*' -and
    (Test-Path (Join-Path $_.FullName 'presets')) -and
    $_.Name -ne 'projectm-standalone-portable-merged-latest'
  } |
  Sort-Object LastWriteTime -Descending |
  Select-Object -First 1).FullName

if ($assetSource) {
  Copy-Item (Join-Path $assetSource 'presets') (Join-Path $portable 'presets') -Recurse -Force
  if (Test-Path (Join-Path $assetSource 'textures')) {
    Copy-Item (Join-Path $assetSource 'textures') (Join-Path $portable 'textures') -Recurse -Force
  }
}

# Zip output
$zip = 'C:/Users/diegobaccino/Downloads/projectm-standalone-portable-merged-latest.zip'
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $portable '*') -DestinationPath $zip
```

## 4) Validate Portable Contents

```powershell
$portable = 'C:/Users/diegobaccino/Downloads/projectm-standalone-portable-merged-latest'

"exeExists=$((Test-Path (Join-Path $portable 'projectMSDL.exe')))"
"dllCount=$((Get-ChildItem $portable -Filter *.dll).Count)"
"presetCount=$((Get-ChildItem (Join-Path $portable 'presets') -Recurse -File -ErrorAction SilentlyContinue).Count)"
"textureCount=$((Get-ChildItem (Join-Path $portable 'textures') -Recurse -File -ErrorAction SilentlyContinue).Count)"
```

Current known-good reference numbers from the last successful run:

- DLL count: 21
- Preset files: 621
- Texture files: 96

## 5) Quick Troubleshooting

- `CMake was unable to find a build program corresponding to Ninja`
  - Use Visual Studio generator:
    `-G 'Visual Studio 17 2022' -A x64`
- Overlay tab/logo upload missing in app
  - Check frontend files in `frontend-sdl-cpp/src/gui/` for your UI changes.
  - Core API presence alone does not create frontend settings controls.
- App starts but no presets/blank visuals
  - Ensure `presets/` and `textures/` are next to `projectMSDL.exe` in portable folder.
- Missing DLL errors on launch
  - Re-copy all DLLs from both runtime sources listed in Step 3.

## 6) Recommended Workflow After Future Changes

1. Commit your core and frontend updates in this unified repo.
2. Re-run Steps 1 to 4.
3. Replace `projectm-standalone-portable-merged-latest.zip` for distribution.
