# Building AERO7 GRAM for Windows 7

AERO7 GRAM targets **Windows 7 SP1 only**. The project is intended to provide separate x86 (32-bit) and x64 (64-bit) builds.

## Current application state

The repository contains a native Qt Widgets desktop interface with:

- Custom Windows-style title bar, minimize/maximize/close controls and window resizing
- DWM glass extension when Windows 7 Aero composition is available
- Chats, Contacts, Calls and Settings navigation
- A first-run connection screen and honest disconnected/empty states
- Local persistence of the window geometry

The native UI now includes a TDLib authorization controller. It supports the phone-number, login-code, two-step password, account registration and email-verification states, and stores the TDLib database under the current Windows user profile. On Windows, the local database key is protected with DPAPI.

**A working login still requires a separately built Windows 7-compatible TDLib library.** The repository does not bundle TDLib binaries. Chat loading and message sending are not implemented yet.

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

## Enable Telegram authorization

Obtain your own Telegram API ID and API hash from https://my.telegram.org. The application asks for these the first time you connect and stores them in the current user's application settings. Do not commit them to the repository.

Build a TDLib JSON library that is compatible with Windows 7 SP1, the selected MSVC runtime and the target architecture. Install its header and import library under one prefix, with the matching `tdjson.dll` available beside the application executable.

Configure x64:

    cmake -S . -B build-x64 -A x64 -DAERO7_ENABLE_TDLIB=ON -DTDJSON_ROOT="C:/deps/tdlib-x64" -DCMAKE_PREFIX_PATH="C:/Qt/5.15.x/msvc2019_64"
    cmake --build build-x64 --config Release

Configure x86 using the x86 compiler prompt and x86 Qt/TDLib prefix:

    cmake -S . -B build-x86 -A Win32 -DAERO7_ENABLE_TDLIB=ON -DTDJSON_ROOT="C:/deps/tdlib-x86" -DCMAKE_PREFIX_PATH="C:/Qt/5.15.x/msvc2019"
    cmake --build build-x86 --config Release

When TDLib is disabled (the default), the interface still builds but the Connect action explains that authorization support is not present in that binary. Do not distribute a build as login-capable unless it includes the matching TDLib runtime and has been tested on Windows 7.

## Testing on Windows 10/11 during development

A Windows 11 machine is useful for an early smoke test: launching the executable, checking the Qt interface, window controls, settings persistence and (when the matching TDLib build is present) exercising Telegram authorization. The application is still targeted at Windows 7; a successful Windows 11 run does not prove Windows 7 compatibility.

The Windows compatibility-mode checkbox is not an emulator. It cannot replace missing Windows 7 APIs, DLLs, TLS behavior or compiler runtimes. You may try Windows 7 compatibility mode if an older dependency behaves differently, but test the actual target on a Windows 7 SP1 virtual machine or physical PC before release.

For a Windows 11 smoke test, build with the same Qt/MSVC target toolchain and all runtime DLLs beside the executable. Start without compatibility mode first. If the application fails, record the exact error and test with Qt's platform plugin and required DLLs deployed correctly. Do not use a Windows 11-only build as the Windows 7 release artifact.

## Aero glass and fallback

When Windows 7 Desktop Window Manager composition is enabled, the application extends the native glass frame into its title area. If DWM composition is unavailable, the regular blue interface remains usable.

## Telegram integration plan

The messaging backend is TDLib through its JSON C interface. Authorization is implemented behind the optional AERO7_ENABLE_TDLIB build flag. The current repository does not include a prebuilt TDLib binary, and Windows 7 compatibility of a TDLib build must be independently verified for x86 and x64.

Phone numbers, login codes and two-step passwords are passed directly to TDLib and are not written to application settings. TDLib's local database is stored in the user's application-data directory. Its database encryption key is protected using Windows DPAPI for the current Windows user. API ID and API hash are stored in the user's application settings; never put these values in source control.

## Compatibility and release policy

- Target OS: Windows 7 SP1 only
- Architectures: x86 and x64
- No Windows 10/11-only APIs in the application core
- Third-party runtime dependencies must be compatible with Windows 7 SP1
- Test both architectures on clean Windows 7 SP1 virtual machines before publishing binaries

The current source has **not yet been compiled or runtime-tested on Windows 7**. A successful source update is not a release binary.
