# Changelog

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [3.0.0] - unreleased

### Changed
- Ported to Qt 6 (6.4 or newer). Qt4 and Qt5 are no longer supported. Port by Joscha Schmiedt.
- Requires CMake 3.16, C++17 and libneurosuite 3. Sources reformatted with clang-format.
- libneurosuite can be built as part of NeuroScope (`-DNEUROSCOPE_BUNDLE_NEUROSUITE=ON`).
- Installs a desktop entry and AppStream metadata under the ID `io.github.neurosuite.NeuroScope`.
- Windows and macOS packages bundle the Qt runtime (Windows also the MSVC runtime) and show the handbook with the
  built-in viewer (QTextBrowser), as does the AppImage; the .deb uses QtWebEngine.
- Licence file corrected to GPL-3.0-or-later, matching the source headers.

### Fixed
- Files shorter than the initial time window (1 s by default) open instead of failing
  with "the file size is incorrect" (#2).
- The `-t`/`--timeWindow` command line option sets the time window; it was passed on as
  the raster height.
- Looking up companion files next to a session file (fix by Joscha Schmiedt).
- Toolbar and dialog icons are shown again (resources were missing from libneurosuite).

### Removed
- Qt4/Qt5 build paths, KDE4-based handbook regeneration, the Qt5 deployment modules,
  and the generated 2004 API documentation (`neuroscope-api/`).
- Travis CI and AppVeyor configuration.

### Known limitations
- The Blackrock Cerebus streaming support (`-DWITH_CEREBUS=ON`) has not been built or
  tested against the current CereLink SDK.

Thanks to Joscha Schmiedt for the Qt6 port and to Théotime de Charrin (MOBS team) whose Qt6
work served as a checklist.
