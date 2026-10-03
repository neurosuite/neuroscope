[![CI](https://github.com/neurosuite/neuroscope/actions/workflows/ci.yml/badge.svg)](https://github.com/neurosuite/neuroscope/actions/workflows/ci.yml)

# NeuroScope

NeuroScope is an advanced viewer for electrophysiological and behavioral data (with limited
editing capabilities): it can display local field potentials, neuronal spikes, behavioral
events, and the position of the animal in the environment.

Developed by Lynn Hazan (main developer), Laurent Montel (Qt3 to Qt4/5 porting), David Faure
(Qt3 to Qt4/5 porting), Michaël Zugaro (plugins, maintenance), Florian Franzen (Blackrock
support, maintenance) and Joscha Schmiedt (Qt5 to Qt6 porting), distributed under the GNU
General Public License v3 or later.

If you use NeuroScope, please cite: L. Hazan, M. Zugaro, G. Buzsáki (2006). Klusters,
NeuroScope, NDManager: a free software suite for neurophysiological data processing and
visualization. *J Neurosci Methods* 155:207-216.

## Installing

Download a package for Linux (.deb, AppImage), macOS (.dmg) or Windows (installer or .zip)
from the [releases page](https://github.com/neurosuite/neuroscope/releases).

## Building

Requires CMake 3.16+, a C++17 compiler, Qt 6.4+ (Widgets, PrintSupport, Xml) and
[libneurosuite](https://github.com/neurosuite/libneurosuite) 3.x.

```bash
# with libneurosuite installed
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build

# or let CMake fetch and build libneurosuite as part of NeuroScope
cmake -B build -S . -DNEUROSCOPE_BUNDLE_NEUROSUITE=ON
```

On Ubuntu 24.04 the build dependencies are `cmake ninja-build qt6-base-dev`, plus
`qt6-webengine-dev` if libneurosuite is built with its WebEngine handbook viewer.
With Nix: `nix build` or `nix develop`.

See [CHANGELOG.md](CHANGELOG.md) for changes.
