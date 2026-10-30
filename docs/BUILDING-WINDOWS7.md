# Building AERO7 GRAM for Windows 7

AERO7 GRAM targets **Windows 7 SP1 only**. The project is intended to provide separate x86 (32-bit) and x64 (64-bit) builds.

## Current application state

The repository contains a native Qt Widgets desktop interface with:

- Custom Windows-style title bar, minimize/maximize/close controls and window resizing
- DWM glass extension when Windows 7 Aero composition is available
- Chats, Contacts, Calls and Settings navigation
- A first-run connection screen and honest disconnected/empty states
- Local persistence of the window geometry

**Telegram is not connected yet.** This preview does not authenticate, load real conversations, send messages, transfer media or receive notifications. The connect action explains this instead of pretending to sign in.

## Supported build environment

Use a Windows 7-compatible Qt 5.15 kit and matching MSVC compiler/runtime for the target architecture.

- Windows 7 SP1, fully updated
- Visual Studio 2019 C++ build tools (v142) with a Windows 7-compatible toolset/runtime
- Qt 5.15.x built for the same compiler and architecture
- CMake 3.16 or newer
- Git

Qt 6 is not supported for this target. Do not mix compiler versions, Qt kits or x86/x64 dependencies.

## Build x64

Open an x64 Native Tools Command Prompt for Visual Studio 2019:

    cmake -S . -B build-x64 -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/5.15.x/msvc2019_64"
    cmake --build build-x64 --config Release

## Build x86

Open an x86 Native Tools Command Prompt for Visual Studio 2019:

    cmake -S . -B build-x86 -A Win32 -DCMAKE_PREFIX_PATH="C:/Qt/5.15.x/msvc2019"
    cmake --build build-x86 --config Release

Replace the Qt paths with the actual locations of the matching Qt kits. Run each build from the matching compiler environment.

## Aero glass and fallback

When Windows 7 Desktop Window Manager composition is enabled, the application extends the native glass frame into its title area. If DWM composition is unavailable, the regular blue interface remains usable.

## Telegram integration plan

The intended messaging backend is TDLib through its JSON C interface. Before integration, a Windows 7-compatible TDLib build must be prepared and tested separately for x86 and x64.

The developer must register an application at https://my.telegram.org to obtain an API ID and API hash. Keep credentials, account data and session databases out of source control. A secure local session-storage design is required before account login is released.

## Compatibility and release policy

- Target OS: Windows 7 SP1 only
- Architectures: x86 and x64
- No Windows 10/11-only APIs in the application core
- Third-party runtime dependencies must be compatible with Windows 7 SP1
- Test both architectures on clean Windows 7 SP1 virtual machines before publishing binaries

The current source has **not yet been compiled or runtime-tested on Windows 7**. A successful source update is not a release binary.
