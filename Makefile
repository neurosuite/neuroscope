# Job runner for building, testing and packaging NeuroScope on Linux and macOS, locally and in CI;
# make.ps1 is the counterpart for Windows. CMake does the actual work; the GitHub workflows only set
# up the runners and call these targets.
#
# Development
#   make                build (configures on first use)
#   make test           build and run the unit tests
#   make install        install to $(PREFIX)
#   make smoke          start the installed neuroscope with --version on the offscreen platform
#   make check          test, install and smoke
#   make package        the platform's default packages (.deb, .dmg, NSIS installer) in $(PACKAGE_DIR)
#   make docker         check and package in the Ubuntu container (see Dockerfile); the installed files
#                       and packages are copied to $(DIST_DIR)
#   make sanitize       build in $(BUILD_DIR)-sanitize with AddressSanitizer and UBSan and run the tests
#   make docker-sanitize  sanitize in the Ubuntu container
#   make reconfigure    rerun CMake, e.g. after changing the variables below
#   make clean          empty the build directory and remove the release build directories
#
# Dependencies
#   make ubuntu-deps    install the Ubuntu build dependencies with apt (EXTRA_PACKAGES for more)
#   make macos-deps     install the macOS build dependencies with Homebrew
#
# Release packages, in $(PACKAGE_DIR), versioned after the checked-out v* tag
#   make deb            libneurosuite3 and NeuroScope .debs; installs the libneurosuite .debs with apt
#   make appimage       AppImage with a bundled libneurosuite, without Qt WebEngine
#   make dmg            macOS disk image with a bundled libneurosuite, without Qt WebEngine
#   make dmg-check      start the application from the disk image and check it uses no Homebrew libraries
#
# Variables (on the command line or in the environment), e.g. make test BUILD_TYPE=Debug:
#   BUILD_DIR, BUILD_TYPE, PREFIX, PACKAGE_DIR, DIST_DIR, GENERATOR, CMAKE_ARGS
#   BUNDLE_NEUROSUITE=ON   build libneurosuite at LIBNEUROSUITE_REF as part of NeuroScope instead of
#                          using an installed copy
#   WITH_WEBENGINE=OFF     build the bundled libneurosuite without Qt WebEngine
#   LIBNEUROSUITE_DIR      libneurosuite source tree for BUNDLE_NEUROSUITE and make deb; cloned at
#                          LIBNEUROSUITE_REF if missing

BUILD_DIR ?= build
BUILD_TYPE ?= Release
PREFIX ?= $(CURDIR)/install
PACKAGE_DIR ?= $(CURDIR)/packages
DIST_DIR ?= $(CURDIR)/dist
GENERATOR ?= $(if $(shell command -v ninja),Ninja,Unix Makefiles)
BUNDLE_NEUROSUITE ?= OFF
LIBNEUROSUITE_REF ?= main
LIBNEUROSUITE_DIR ?= $(CURDIR)/libneurosuite
LIBNEUROSUITE_URL ?= https://github.com/neurosuite/libneurosuite.git
WITH_WEBENGINE ?= ON
CMAKE_ARGS ?=
DOCKER ?= docker
SUDO ?= sudo
EXTRA_PACKAGES ?=

ifeq ($(shell uname -s),Darwin)
# Homebrew's Qt is keg-only.
QT_PREFIX ?= $(shell brew --prefix qt 2>/dev/null)
endif

# Release versions: v3.0.0-rc1 gives 3.0.0-rc1, and 3.0.0~rc1 for Debian so that 3.0.0 sorts after it.
# Untagged builds are dev-<commit>, and their .debs keep the project version.
RELEASE_TAG ?= $(if $(filter refs/tags/v%,$(GITHUB_REF)),$(GITHUB_REF_NAME),$(shell git describe --tags --exact-match --match "v*" 2>/dev/null))
PKG_VERSION ?= $(if $(RELEASE_TAG),$(RELEASE_TAG:v%=%),dev-$(shell git rev-parse --short=7 HEAD 2>/dev/null))
DEB_VERSION ?= $(if $(RELEASE_TAG),$(subst -,~,$(PKG_VERSION)))

# Bundled libneurosuite: from LIBNEUROSUITE_DIR if it exists, otherwise fetched by CMake.
BUNDLE_ARGS = -DNEUROSCOPE_BUNDLE_NEUROSUITE=ON -DNEUROSUITE_GIT_TAG=$(LIBNEUROSUITE_REF) \
	$(if $(wildcard $(LIBNEUROSUITE_DIR)/CMakeLists.txt),-DFETCHCONTENT_SOURCE_DIR_NEUROSUITE=$(LIBNEUROSUITE_DIR))

# Configures a build directory: $(call configure-cmake,build directory,arguments)
configure-cmake = cmake -S . -B $(1) -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	$(if $(QT_PREFIX),-DCMAKE_PREFIX_PATH=$(QT_PREFIX)) $(2) $(CMAKE_ARGS)

UBUNTU_PACKAGES = ca-certificates cmake dpkg-dev file g++ git make ninja-build qt6-base-dev libgl-dev \
	$(if $(filter ON,$(WITH_WEBENGINE)),qt6-webengine-dev) $(EXTRA_PACKAGES)

LINUXDEPLOY_URL = https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
LINUXDEPLOY_QT_URL = https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage

.PHONY: all configure reconfigure build test install smoke check package docker sanitize docker-sanitize clean \
	ubuntu-deps macos-deps libneurosuite deb appimage dmg dmg-check

all: build

################################################################################
# Development
################################################################################
$(BUILD_DIR)/CMakeCache.txt:
	$(call configure-cmake,$(BUILD_DIR), \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX) -DWITH_WEBENGINE=$(WITH_WEBENGINE) \
		$(if $(filter ON,$(BUNDLE_NEUROSUITE)),$(BUNDLE_ARGS)))

configure: $(BUILD_DIR)/CMakeCache.txt

reconfigure:
	rm -f $(BUILD_DIR)/CMakeCache.txt
	$(MAKE) configure

build: configure
	cmake --build $(BUILD_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

install: build
	cmake --install $(BUILD_DIR)

smoke:
	QT_QPA_PLATFORM=offscreen LD_LIBRARY_PATH=$(PREFIX)/lib:$(PREFIX)/lib64 $(PREFIX)/bin/neuroscope --version

check: test install
	$(MAKE) smoke

package: build
	cpack --config $(BUILD_DIR)/CPackConfig.cmake -B $(PACKAGE_DIR)

docker:
	$(DOCKER) build \
		--build-arg WITH_WEBENGINE=$(WITH_WEBENGINE) \
		--build-arg LIBNEUROSUITE_REF=$(LIBNEUROSUITE_REF) \
		--target artifact --output type=local,dest=$(DIST_DIR) \
		.

# The build directory itself is kept, with its .gitkeep.
# Memory errors and undefined behaviour stop the test. Leak detection is off: some readers leak on
# error paths, which the tests exercise.
SANITIZE_FLAGS = -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer

sanitize:
	$(call configure-cmake,$(BUILD_DIR)-sanitize, \
		-DCMAKE_BUILD_TYPE=Debug -DWITH_WEBENGINE=$(WITH_WEBENGINE) \
		$(if $(filter ON,$(BUNDLE_NEUROSUITE)),$(BUNDLE_ARGS)) \
		"-DCMAKE_CXX_FLAGS=$(SANITIZE_FLAGS)" \
		"-DCMAKE_EXE_LINKER_FLAGS=$(SANITIZE_FLAGS)" "-DCMAKE_SHARED_LINKER_FLAGS=$(SANITIZE_FLAGS)")
	cmake --build $(BUILD_DIR)-sanitize
	ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1 \
		ctest --test-dir $(BUILD_DIR)-sanitize --output-on-failure

docker-sanitize:
	$(DOCKER) build \
		--build-arg WITH_WEBENGINE=OFF \
		--build-arg LIBNEUROSUITE_REF=$(LIBNEUROSUITE_REF) \
		--target sanitize \
		.

clean:
	[ ! -d $(BUILD_DIR) ] || find $(BUILD_DIR) -mindepth 1 -maxdepth 1 ! -name .gitkeep -exec rm -rf {} +
	rm -rf $(BUILD_DIR)-deb $(BUILD_DIR)-libneurosuite $(BUILD_DIR)-appimage \
		$(BUILD_DIR)-dmg $(BUILD_DIR)-sanitize

################################################################################
# Dependencies
################################################################################
ubuntu-deps:
	$(SUDO) apt-get update
	DEBIAN_FRONTEND=noninteractive $(SUDO) apt-get install -y --no-install-recommends $(UBUNTU_PACKAGES)

macos-deps:
	brew install qt ninja

$(LIBNEUROSUITE_DIR)/CMakeLists.txt:
	git clone --depth 1 --branch $(LIBNEUROSUITE_REF) $(LIBNEUROSUITE_URL) $(LIBNEUROSUITE_DIR)

libneurosuite: $(LIBNEUROSUITE_DIR)/CMakeLists.txt

################################################################################
# Release packages
################################################################################
# NeuroScope .deb depending on the libneurosuite3 package, which is built first and installed with apt.
deb: libneurosuite
	cmake -S $(LIBNEUROSUITE_DIR) -B $(BUILD_DIR)-libneurosuite -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=/usr $(if $(DEB_VERSION),-DCPACK_PACKAGE_VERSION=$(DEB_VERSION))
	cmake --build $(BUILD_DIR)-libneurosuite
	cpack --config $(BUILD_DIR)-libneurosuite/CPackConfig.cmake -G DEB -B $(BUILD_DIR)-libneurosuite/packages
	$(SUDO) apt-get install -y $(abspath $(BUILD_DIR)-libneurosuite)/packages/*.deb
	mkdir -p $(PACKAGE_DIR)
	cp $(BUILD_DIR)-libneurosuite/packages/libneurosuite3_*.deb $(PACKAGE_DIR)/
	$(call configure-cmake,$(BUILD_DIR)-deb, \
		-DCMAKE_INSTALL_PREFIX=/usr $(if $(DEB_VERSION),-DCPACK_PACKAGE_VERSION=$(DEB_VERSION)))
	cmake --build $(BUILD_DIR)-deb
	cpack --config $(BUILD_DIR)-deb/CPackConfig.cmake -G DEB -B $(BUILD_DIR)-deb/packages
	dpkg-deb -f $(BUILD_DIR)-deb/packages/neuroscope_*.deb Depends | tr ',' '\n' | grep -q libneurosuite3
	cp $(BUILD_DIR)-deb/packages/neuroscope_*.deb $(PACKAGE_DIR)/

# AppImage with the QTextBrowser handbook, which is much smaller than Qt WebEngine.
appimage:
	$(call configure-cmake,$(BUILD_DIR)-appimage,-DCMAKE_INSTALL_PREFIX=/usr -DWITH_WEBENGINE=OFF $(BUNDLE_ARGS))
	cmake --build $(BUILD_DIR)-appimage
	rm -rf $(BUILD_DIR)-appimage/AppDir
	DESTDIR=$(abspath $(BUILD_DIR)-appimage/AppDir) cmake --install $(BUILD_DIR)-appimage
	mkdir -p $(BUILD_DIR)-appimage/tools $(PACKAGE_DIR)
	cd $(BUILD_DIR)-appimage/tools && for url in $(LINUXDEPLOY_URL) $(LINUXDEPLOY_QT_URL); do \
		[ -f $$(basename $$url) ] || wget -q $$url; done && chmod +x *.AppImage
	cd $(PACKAGE_DIR) && PATH=$(abspath $(BUILD_DIR)-appimage/tools):$$PATH QMAKE=/usr/lib/qt6/bin/qmake \
		LDAI_OUTPUT=NeuroScope-$(PKG_VERSION)-x86_64.AppImage \
		$(abspath $(BUILD_DIR)-appimage/tools)/linuxdeploy-x86_64.AppImage \
		--appdir $(abspath $(BUILD_DIR)-appimage/AppDir) --plugin qt --output appimage \
		--desktop-file $(abspath $(BUILD_DIR)-appimage/AppDir)/usr/share/applications/io.github.neurosuite.NeuroScope.desktop

dmg:
	$(call configure-cmake,$(BUILD_DIR)-dmg, \
		-DCPACK_PACKAGE_VERSION=$(PKG_VERSION) -DWITH_WEBENGINE=OFF -DAPPBUNDLE=ON $(BUNDLE_ARGS))
	cmake --build $(BUILD_DIR)-dmg
	cpack --config $(BUILD_DIR)-dmg/CPackConfig.cmake -G DragNDrop -B $(PACKAGE_DIR)

# Every dependency must come from the bundle, none from Homebrew. A library's own install name
# (otool -D) may still point to Homebrew; dyld does not use it.
dmg-check:
	rm -rf $(BUILD_DIR)-dmg/mount && mkdir -p $(BUILD_DIR)-dmg/mount
	hdiutil attach $(PACKAGE_DIR)/*.dmg -mountpoint $(BUILD_DIR)-dmg/mount -nobrowse
	app=$(BUILD_DIR)-dmg/mount/neuroscope.app; \
	status=0; \
	for f in $$app/Contents/MacOS/neuroscope $$app/Contents/Frameworks/*.dylib \
	         $$app/Contents/Frameworks/*.framework/Versions/A/* $$app/Contents/PlugIns/*/*.dylib; do \
		[ -f "$$f" ] || continue; \
		id=$$(otool -D "$$f" | tail -n +2); \
		if otool -L "$$f" | tail -n +2 | grep /opt/homebrew | grep -vF "$${id:-none}"; then echo "in $$f"; status=1; fi; \
	done; \
	$$app/Contents/MacOS/neuroscope --version || status=1; \
	hdiutil detach $(BUILD_DIR)-dmg/mount; \
	exit $$status
