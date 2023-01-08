#!/bin/bash
set -e

mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$PREFIX \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_TESTING=OFF \
        -DENABLE_BENCHMARK=OFF \
        -DENABLE_EXAMPLE=OFF

cmake --build .
cmake --build . --target install