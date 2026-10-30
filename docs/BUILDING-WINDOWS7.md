# Building AERO7 GRAM for Windows 7

AERO7 GRAM targets **Windows 7 SP1 only**. Separate x86 and x64 builds are intended.

## Current state

The repository contains the first native Qt Widgets application shell. It is not a working Telegram client yet: account authorization, chat synchronization, message sending, media transfer and notifications are not implemented.

## Toolchain

Use a Windows 7-compatible Qt 5.15 kit and matching compiler/runtime for the target architecture.

- Windows 7 SP1 with the platform updates required by the selected compiler runtime
- Visual Studio 2019 C++ build tools (v142), with the Windows 7-compatible toolset/runtime
- Qt 5.15.x built for the same compiler and architecture
- CMake 3.16 or newer
- Git

Qt 5.15 documentation lists Windows 7 for x86 and x86_64 configurations. Do not use Qt 6: current Qt 6 releases no longer support Windows 7.

## Build x64

Open an x64 Native Tools Command Prompt for the selected Visual Studio toolset:

```bat
cmake -S . -B build-x64 -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/5.15.x/msvc2019_64"
cmake --build build-x64 --config Release
```

## Build x86

Open an x86 Native Tools Command Prompt:

```bat
cmake -S . -B build-x86 -A Win32 -DCMAKE_PREFIX_PATH="C:/Qt/5.15.x/msvc2019"
cmake --build build-x86 --config Release
```

Replace the Qt paths with the actual locations of the matching Qt kits. Never mix x86 and x64 Qt libraries, compiler environments or dependencies.

## Aero glass

On Windows 7 with Desktop Window Manager composition enabled, the application requests DWM glass over the client area. If composition is disabled or unavailable, the interface falls back to its opaque blue/mint theme.

## Telegram integration

The planned messaging backend is TDLib using its JSON C interface. Before the client can connect, the project must add a Windows 7-compatible TDLib build for each architecture and the developer must register an application at https://my.telegram.org to obtain their own API ID and API hash.

Do not commit API credentials, account data, session databases or private keys.

## Compatibility policy

- Target: Windows 7 SP1
- Architectures: x86 and x64
- No Windows 10/11-only APIs in the core application
- Keep all third-party runtime dependencies compatible with Windows 7 SP1
- Test each architecture on a clean Windows 7 SP1 virtual machine before release
