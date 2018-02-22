#!/bin/sh
set -e

if [ "$TRAVIS_OS_NAME" = "osx" ]; then
    brew update && brew install qt
    export PATH="/usr/local/opt/qt/bin:$PATH"
fi

CMAKE_CONFIG_VARS=""
if [ $QT_VERSION = "qt4" ]; then
    CMAKE_CONFIG_VARS="${CMAKE_CONFIG_VARS} -DWITH_QT4=ON"
fi

git clone https://github.com/neurosuite/libneurosuite.git
cd libneurosuite
cmake $CMAKE_CONFIG_VARS . && sudo make install
cd ..

git clone https://github.com/neurosuite/libcbsdk.git
cd libcbsdk
cmake $CMAKE_CONFIG_VARS . && sudo make install
cd ..
