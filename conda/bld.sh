#!/bin/bash
set -e

mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$PREFIX \
        -DCMAKE_BUILD_TYPE=Release \
        -DTURBO_BUILD_TESTING=OFF \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DTURBO_BUILD_EXAMPLE=OFF \
        -DBUILD_SHARED_LIBS=ON

cmake --build .
cmake --build . --target install