#!/bin/sh
CMAKE_CONFIG_VARS="-DWITH_CEREBUS=ON"
if [ $QT_VERSION = "qt4" ]; then
    CMAKE_CONFIG_VARS="${CMAKE_CONFIG_VARS} -DWITH_QT4=ON"
fi

mkdir build
cd build
cmake $CMAKE_CONFIG_VARS .. && make package
