.. Copyright 2023 The Elastic AI Search Authors.

Build
=====

build environment requirement
-----------------------------------------------------

* Cmake 3.17 is the minimum supported version.
* gcc >= 9.3 or Apple Clang > 12

centos docker
-----------------------------------------------------

..  code-block:: console

    docker run -it bohuli2048/centos7:v1.0.0 /bin/bash
    scl enable devtoolset-7 bash


centos prepare
-----------------------------------------------------

.. code-block::

    sudo yum install centos-release-scl
    sudo yum install devtoolset-7-gcc*
    scl enable devtoolset-7 bash

ubuntu prepare(>=20.04)
----------------------------------------------------

..  code-block::

    sudo apt install gcc g++


build with cmake
--------------------------------------------

.. code-block:: console

    >carbin install
    >mkdir build
    >cd build
    >cmake .. -DCARBIN_BUILD_TESTS=ON
    >make


Install
===================


install with cmake

.. code-block:: console

    >mkdir build
    >cd build
    >cmake .. -DCARBIN_BUILD_TESTS=OFF
    >make
    >make install

