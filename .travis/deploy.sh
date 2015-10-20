#!/bin/sh
export ENABLE_DEPLOYMENT=false
if [ "$TRAVIS_OS_NAME" = "linux" ] &&
   [ "$QT_VERSION" = "qt4" ]; then
   export ENABLE_DEPLOYMENT=true
fi
if [ "$TRAVIS_OS_NAME" = "osx" ] &&
   [ "$QT_VERSION" = "qt5" ]; then
   export ENABLE_DEPLOYMENT=true
fi
