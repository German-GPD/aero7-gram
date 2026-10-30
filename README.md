# AERO7 GRAM

<p align="center">
  <strong>Telegram, with an Aero soul.</strong><br>
  An independent native desktop client project for Windows 7 SP1.
</p>

<p align="center">
  <a href="https://german-gpd.github.io/aero7-gram/">Project website</a> ·
  <a href="https://github.com/German-GPD/aero7-gram">GitHub repository</a>
</p>

---

## Platform

- **Operating system:** Windows 7 SP1 only
- **Architectures:** x86 (32-bit) and x64 (64-bit)
- **UI framework:** Qt 5.15 Widgets / C++14
- **Window effects:** Windows 7 DWM Aero glass
- **Planned Telegram backend:** TDLib

Windows 10 and Windows 11 are not target platforms. They can still be used for preliminary development smoke tests; compatibility mode does not emulate Windows 7.

## Current status

**Version 0.2.0 — native interface preview.**

The application has a native Qt desktop shell with a custom title bar, window controls, resizing, DWM glass integration, a Telegram-inspired Aero icon, navigation for Chats / Contacts / Calls / Settings and saved window geometry.

**Telegram authorization flow is implemented behind an optional TDLib build.** It handles phone number, login code, two-step verification password, registration and email verification. A matching Windows 7-compatible TDLib library is required; no TDLib binaries are bundled. Real chat loading, message sending, file transfers and notifications are still pending.

## Build

See [docs/BUILDING-WINDOWS7.md](docs/BUILDING-WINDOWS7.md) for x86 and x64 build instructions.

Use a Windows 7-compatible Qt 5.15 kit and the matching Visual Studio 2019 toolchain. Do not mix architectures or compiler runtimes.

The current source update has not yet been compiled or tested on a Windows 7 machine. No ready-to-install .exe is claimed.

## Roadmap

1. Compile and stabilize the native interface on Windows 7 SP1, x86 and x64.
2. Build and verify TDLib for Windows 7 SP1 in both architectures.
3. Test the implemented Telegram authorization and DPAPI-protected local database key.
4. Load chats, contacts and message history.
5. Implement text messaging, media/files, notifications and settings.
6. Package and test portable x86 and x64 builds on clean Windows 7 installations.

## Website

https://german-gpd.github.io/aero7-gram/

## Disclaimer

AERO7 GRAM is an independent community project and is not affiliated with, endorsed by, or officially connected to Telegram or Microsoft. Telegram, Windows and Aero are trademarks of their respective owners.

## License

Original project materials are distributed under the MIT License. Third-party libraries retain their own licenses and notices. See [LICENSE](LICENSE).
