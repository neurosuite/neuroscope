#!/bin/sh

if [ "$TRAVIS_OS_NAME" = "osx" ]; then
    brew update && brew install qt@5.5 || exit 1
    export PATH="/usr/local/opt/qt@5.5/bin:$PATH"
fi

CMAKE_CONFIG_VARS=""
if [ $QT_VERSION = "qt4" ]; then
    CMAKE_CONFIG_VARS="${CMAKE_CONFIG_VARS} -DWITH_QT4=ON"
fi

git clone https://github.com/neurosuite/libneurosuite.git || exit 1
cd libneurosuite
cmake $CMAKE_CONFIG_VARS . && sudo make install || exit 1
cd ..

git clone https://github.com/neurosuite/libcbsdk.git || exit 1
cd libcbsdk
cmake $CMAKE_CONFIG_VARS . && sudo make install || exit 1
cd ..
