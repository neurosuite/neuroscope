#!/bin/sh
export RELEASE_PKG_FILE=""
if [ "$TRAVIS_OS_NAME" = "linux" ] &&
   [ "$QT_VERSION"     = "qt4" ]; then
   export RELEASE_PKG_FILE=$(ls "${TRAVIS_BUILD_DIR}/build/neuroscope_*.deb");
fi
if [ "$TRAVIS_OS_NAME" = "osx" ] &&
   [ "$QT_VERSION"     = "qt5" ]; then
   export RELEASE_PKG_FILE=$(ls "${TRAVIS_BUILD_DIR}/build/neuroscope-*.dmg");
fi
