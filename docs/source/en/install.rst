.. Copyright 2023 The Elastic AI Search Authors.

Build
=====


build with cmake::

    >carbin install
    >mkdir build
    >cd build
    >cmake .. -DCARBIN_BUILD_TESTS=ON
    >make


Install
=======
install with cmake::

    >mkdir build
    >cd build
    >cmake .. -DCARBIN_BUILD_TESTS=OFF
    >make
    >make install

