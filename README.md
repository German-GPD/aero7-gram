# AERO7 GRAM

<p align="center">
  <strong>Telegram, with an Aero soul.</strong><br>
  A native desktop Telegram client concept built around the Windows 7 Aero experience.
</p>

<p align="center">
  <a href="https://german-gpd.github.io/aero7-gram/">Project website</a> ·
  <a href="https://github.com/German-GPD/aero7-gram">GitHub repository</a>
</p>

---

## Platform

- **Operating system:** Windows 7 SP1 only
- **Architectures:** x86 (32-bit) and x64 (64-bit)
- **UI framework:** Qt 5.15 Widgets
- **Window effects:** Windows 7 Desktop Window Manager (DWM) Aero glass
- **Planned Telegram backend:** TDLib

Windows 10 and Windows 11 are not target platforms.

## Project status

<em>Early development — application shell only.</em>

The repository now includes the first native Qt application scaffold, a Windows-specific DWM glass integration attempt, CMake build configuration and separate x86/x64 build instructions.

**This is not a usable Telegram client yet.** Telegram authorization, chat loading, message sending, media transfer, notifications and session management still need to be implemented. The current preview window uses placeholder content.

## Build

See [docs/BUILDING-WINDOWS7.md](docs/BUILDING-WINDOWS7.md) for the Windows 7 SP1 toolchain and x86/x64 build commands.

Use a Qt 5.15 kit that matches the selected compiler and architecture. Do not mix 32-bit and 64-bit dependencies. The application has not yet been compiled or runtime-tested on Windows 7.

## Roadmap

1. Stabilize the native Aero window and verify it on Windows 7 SP1.
2. Build and integrate a Windows 7-compatible TDLib for x86 and x64.
3. Implement secure Telegram authorization and session persistence.
4. Load chats, contacts and message history.
5. Add text messages, files, photos, voice messages and notifications.
6. Package and test portable x86 and x64 releases on clean Windows 7 installations.

## Website

The project website is available at:

**https://german-gpd.github.io/aero7-gram/**

## Disclaimer

AERO7 GRAM is an independent community project and is not affiliated with, endorsed by, or officially connected to Telegram or Microsoft. Telegram, Windows and Aero are trademarks of their respective owners.

## License

Original project materials are distributed under the MIT License. Third-party libraries retain their own licenses and notices. See [LICENSE](LICENSE).
