#!/usr/bin/env sh

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:"$PATH"
fetch --no-history v8
cd v8
gclient sync
export CXXFLAGS="-Wno-unknown-warning-option"
export CXX=`which clang++`
export CC=`which clang`
export CPP="`which clang` -E -std=c++11 -stdlib=libc++"
export LINK="`which clang++` -std=c++11 -stdlib=libc++"
export CXX_host=`which clang++`
export CC_host=`which clang`
export CPP_host="`which clang` -E"
export LINK_host=`which clang++`
export GYP_DEFINES="clang=1 mac_deployment_target=10.7"
make native -j8
# make native -j8 snapshot=off

