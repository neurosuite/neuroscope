#!/bin/sh

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    if [ "$QT_VERSION" = "qt5" ]; then
        sudo add-apt-repository -y ppa:beineri/opt-qt55
    fi
    sudo add-apt-repository -y ppa:george-edison55/precise-backports
    sudo apt-get update
    sudo apt-get install -y cmake cmake-data

    if [ "$QT_VERSION" = "qt4" ]; then
        sudo apt-get install -y libqtgui4 libqtwebkit4
    fi
    if [ "$QT_VERSION" = "qt5" ]; then
        sudo apt-get install -y qt55base qt55webkit
        source /opt/qt55/bin/qt55-env.sh
    fi
fi

if [ "$TRAVIS_OS_NAME" = "osx" ]; then
    brew update
    brew install $QT_VERSION
    if [ "$QT_VERSION" = "qt5" ]; then
        brew link --force qt5
        ln -s /usr/local/opt/qt5/mkspecs /usr/local/mkspecs
        ln -s /usr/local/opt/qt5/plugins /usr/local/plugins
    fi
fi

CMAKE_CONFIG_VARS=""
if [ $QT_VERSION = "qt4" ]; then
    CMAKE_CONFIG_VARS="${CMAKE_CONFIG_VARS} -DWITH_QT4=ON"
fi

git clone https://github.com/neurosuite/libneurosuite.git
cd libneurosuite
cmake $CMAKE_CONFIG_VARS .
make
sudo make install
cd ..

git clone https://github.com/neurosuite/libcbsdk.git
cd libcbsdk
cmake $CMAKE_CONFIG_VARS .
make
sudo make install
cd ..
