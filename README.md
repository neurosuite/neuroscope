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

On Ubuntu 24.04 or newer, install the .deb together with the `libneurosuite3` package from
the same release:

```bash
sudo apt install ./libneurosuite3_*.deb ./neuroscope_*.deb
```

## Building

Requires CMake 3.16+, a C++17 compiler, Qt 6.4+ (Widgets, PrintSupport, Xml) and
[libneurosuite](https://github.com/neurosuite/libneurosuite) 3.x, which can also be built as part
of NeuroScope. On Ubuntu 24.04:

```bash
make ubuntu-deps
make check BUNDLE_NEUROSUITE=ON   # build, run the unit tests, install to ./install
```

See [BUILD.md](BUILD.md) for macOS, Windows, Docker and Nix, the unit tests and the release
packages.

See [CHANGELOG.md](CHANGELOG.md) for changes.
