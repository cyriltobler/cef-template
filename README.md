# examshell

A minimal Chromium Embedded Framework (CEF) project. Builds a standalone browser app for Linux, macOS, and Windows using pure CMake — no Python scripts or extra build tools required.

## Features

- **Pure CMake** build system
- **Auto-downloads CEF** binary distribution from Spotify CDN at configure time
- **Cross-platform**: Linux x64, macOS x64/arm64, Windows x64
- **CEF Views framework** for platform-independent windowing
- **Configurable URL** via command-line flag
- **CI/CD** with GitHub Actions for all platforms

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The first configure will download the CEF binary distribution (~300 MB) and cache it in `third_party/cef/`.

### macOS with Xcode

```bash
cmake -S . -B build -G "Xcode"
cmake --build build --config Release
```

### Windows with Visual Studio

```bash
cmake -S . -B build -A x64
cmake --build build --config Release
```

> **Note:** Each generator (Make, Xcode, Visual Studio) requires its own build directory. If you switch generators, use a fresh directory: `cmake -S . -B build-xcode -G "Xcode"` or delete the existing `build/` first.

## Run

```bash
# Linux / Windows
./build/Release/examshell

# macOS
open build/Release/examshell.app

# Custom URL
./build/Release/examshell --url=https://example.com
```

## CEF Version

The CEF version is set in `CMakeLists.txt`:

```cmake
set(CEF_VERSION "137.0.10+g7e14fe1+chromium-137.0.7151.69")
```

Browse available versions at https://cef-builds.spotifycdn.com/index.html.

## Project Structure

```
examshell/
├── cmake/DownloadCEF.cmake   # Auto-downloads CEF from Spotify CDN
├── src/
│   ├── app.h/cc              # CefApp — creates browser on context init
│   ├── handler.h/cc          # CefClient — browser lifecycle handling
│   ├── main_linux.cc         # Linux entry point
│   ├── main_mac.mm           # macOS entry point
│   ├── main_win.cc           # Windows entry point
│   └── mac_helper.cc         # macOS helper subprocess
├── CMakeLists.txt            # Root build config
└── .github/workflows/        # CI for Linux, macOS, Windows
```

## License

MIT
