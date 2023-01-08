#!/bin/bash
set -e

mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$PREFIX \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_UNIT_TESTS=OFF \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_BENCHMARK=OFF

cmake --build .
cmake --build . --target install