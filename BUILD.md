# Building NeuroScope

NeuroScope is built with CMake. Two job runners wrap the CMake steps with the same targets and
variables: the [Makefile](Makefile) on Linux and macOS and [make.ps1](make.ps1) on Windows. The
GitHub workflows only set up the runners and call these targets, so a local `make check` runs the
same steps as CI.

## Requirements

- CMake 3.16 or newer and a C++17 compiler (GCC, Clang, or MSVC from Visual Studio 2026 or 2022)
- Qt 6.4 or newer: Widgets, PrintSupport and Xml, and Test for the unit tests
- [libneurosuite](https://github.com/neurosuite/libneurosuite) 3.x, either installed or built as
  part of NeuroScope with `BUNDLE_NEUROSUITE=ON` (which needs git and, unless `WITH_WEBENGINE=OFF`,
  Qt WebEngine)
- Ninja is used if it is installed

## Linux

On Ubuntu 24.04:

```bash
make ubuntu-deps                  # installs the build dependencies with apt (uses sudo)
make check BUNDLE_NEUROSUITE=ON   # build, run the unit tests, install to ./install, start it
```

`make ubuntu-deps WITH_WEBENGINE=OFF` leaves out Qt WebEngine; build libneurosuite without it as
well with `make check BUNDLE_NEUROSUITE=ON WITH_WEBENGINE=OFF`. With libneurosuite installed,
leave out `BUNDLE_NEUROSUITE=ON`. On other distributions, install the packages above with the
distribution's package manager.

### In a container

```bash
make docker                       # or: docker build .
```

builds NeuroScope in an Ubuntu 24.04 container (see the [Dockerfile](Dockerfile)), runs
`make check` and `make package` there and copies the installed files and the .deb to `dist/`. This
is what the Linux CI jobs run, so it reproduces them without installing anything but Docker or
Podman (`make docker DOCKER=podman`).

## macOS

With [Homebrew](https://brew.sh) and the Xcode command line tools:

```bash
make macos-deps                   # brew install qt ninja
make check BUNDLE_NEUROSUITE=ON
```

Homebrew's Qt is found automatically.

## Windows

Install Visual Studio 2026 with the "Desktop development with C++" workload (it includes CMake and
Ninja) and Qt 6 for MSVC 2022 64-bit, for example with the Qt online installer; the MSVC 2022 build
of Qt works with Visual Studio 2026, which keeps the same binary interface. Visual Studio 2022 works
as well. Then, in a Developer PowerShell for VS 2026:

```powershell
$env:CMAKE_PREFIX_PATH = "C:\Qt\6.8.3\msvc2022_64"   # your Qt installation
$env:PATH = "$env:CMAKE_PREFIX_PATH\bin;$env:PATH"   # Qt DLLs for the unit tests
./make.ps1 check BUNDLE_NEUROSUITE=ON
```

The Qt libraries are copied next to `neuroscope.exe` when it is installed. If scripts are blocked
on your machine, run `powershell -ExecutionPolicy Bypass -File make.ps1 check ...`.

## Nix

```bash
nix build          # builds and runs the unit tests
nix develop        # shell with the build dependencies
```

## Targets and variables

| Target        | What it does                                                               |
|---------------|----------------------------------------------------------------------------|
| `build`       | Configure (on first use) and build; the default target                     |
| `test`        | Build and run the unit tests                                               |
| `install`     | Install to `PREFIX`                                                        |
| `smoke`       | Start the installed `neuroscope --version` on the offscreen platform       |
| `check`       | `test`, `install` and `smoke`                                              |
| `package`     | The platform's default packages (.deb, .dmg, NSIS installer) in `PACKAGE_DIR` |
| `docker`      | `check` and `package` in the Ubuntu container, results in `DIST_DIR` (Makefile only) |
| `reconfigure` | Rerun CMake, needed after changing the variables below                     |
| `clean`       | Remove the build directories                                               |

Variables are given on the command line (`make test BUILD_TYPE=Debug`,
`./make.ps1 test BUILD_TYPE=Debug`) or as environment variables:

| Variable            | Default      | Meaning                                                       |
|---------------------|--------------|---------------------------------------------------------------|
| `BUILD_DIR`         | `build`      | CMake build directory                                         |
| `BUILD_TYPE`        | `Release`    | `CMAKE_BUILD_TYPE`                                            |
| `PREFIX`            | `./install`  | Installation prefix                                           |
| `PACKAGE_DIR`       | `./packages` | Where packages are written                                    |
| `BUNDLE_NEUROSUITE` | `OFF`        | Build libneurosuite as part of NeuroScope                     |
| `LIBNEUROSUITE_REF` | `main`       | libneurosuite branch or tag to fetch                          |
| `LIBNEUROSUITE_DIR` | `./libneurosuite` | Local libneurosuite sources, used instead of fetching if present |
| `WITH_WEBENGINE`    | `ON`         | Build the bundled libneurosuite with the Qt WebEngine handbook viewer |
| `GENERATOR`         | Ninja        | CMake generator                                               |
| `CMAKE_ARGS`        |              | Further CMake arguments                                       |

The targets are thin wrappers. Without them, the equivalent of `make check BUNDLE_NEUROSUITE=ON` is:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install -DNEUROSCOPE_BUNDLE_NEUROSUITE=ON
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build
```

## Editor setup

CMake writes `compile_commands.json` to the build directory, and clangd finds it in `build/`, the
default `BUILD_DIR`. Build once (`make`), so that the files generated by Qt (`.moc` and `ui_*.h`)
exist. For another build directory, point clangd to it with `--compile-commands-dir` or a `.clangd`
file.

## Tests

The unit tests in [tests/](tests) are Qt Test programs for the file readers: .dat/.eeg and
Neuralynx .ncs traces, Blackrock NSX and NEV files, .clu/.res spikes, .evt events, position files
and parameter files. They write small synthetic files and check the data the providers emit, without
widgets. The readers are built as the static library `neuroscope_dataio`, which the application and
the tests link.

```bash
make test                                         # all tests (./make.ps1 test on Windows)
ctest --test-dir build -R clusters                # tests whose name matches
build/tests/test_clustersprovider browseWindows   # one test function, with Qt Test's output
build/tests/test_clustersprovider -help           # Qt Test options
```

The tests are built by default; configure with `-DBUILD_TESTING=OFF` (or
`CMAKE_ARGS=-DBUILD_TESTING=OFF`) to skip them.

Known bugs in the readers are marked with `QEXPECT_FAIL` and a description, so that the tests pass
and document the current behaviour. Fixing such a bug turns the expected failure into an unexpected
pass, which fails the test until the `QEXPECT_FAIL` is removed.

To add a test, write `tests/test_<name>.cpp` with a `QTEST_GUILESS_MAIN` test class and add
`neuroscope_add_test(test_<name>)` to [tests/CMakeLists.txt](tests/CMakeLists.txt).
[tests/testutils.h](tests/testutils.h) has helpers to write the binary formats and to copy the data
emitted by the providers.

## Release packages

The [release workflow](.github/workflows/release.yml) builds these for a `v*` tag and attaches them
to a draft release. They can also be built locally:

| Target                                     | Package                                                        |
|--------------------------------------------|----------------------------------------------------------------|
| `make deb`                                 | libneurosuite3 and NeuroScope .debs, with the WebEngine handbook; installs the libneurosuite .debs with apt |
| `make appimage`                            | AppImage with a bundled libneurosuite, without WebEngine        |
| `make dmg`, `make dmg-check`               | macOS disk image, and a check that it starts without Homebrew   |
| `./make.ps1 windows-packages`, `./make.ps1 windows-packages-check` | NSIS installer and zip, and a check that it starts without Qt on the PATH |

The package version comes from the checked-out `v*` tag (`v3.0.0-rc1` gives `3.0.0-rc1`, and
`3.0.0~rc1` for the .deb); untagged builds are `dev-<commit>`. Set `PKG_VERSION` to override it.
