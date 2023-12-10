.. Copyright 2023 The Elastic AI Search Authors.


simple example
=================================================

Suppose we have a T add(T a, Tb) function that we want to test:

..  _add_test:


..  code-block:: c++

    template <typename T>
    T add(T a, T b) {
        return a + b;
    }

We can write a test for it like this:

..  code-block:: c++

    TEST_CASE("add") {
        auto test = [](auto a, auto b) {
            return add(a, b);
        };
        REQUIRE(test(1, 2) == 3);
        REQUIRE(test(1.0, 2.0) == 3.0);
    }

the complete add_test.cc file is here:

..  code-block:: c++

    #define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
    #include "doctest.h"

    template <typename T>
    T add(T a, T b) {
        return a + b;
    }

    TEST_CASE("add") {
        auto test = [](auto a, auto b) {
            return add(a, b);
        };
        REQUIRE(test(1, 2) == 3);
        REQUIRE(test(1.0, 2.0) == 3.0);
    }

We can compile and run it like this:

..  code-block:: console

    $ g++ -std=c++17 -I. -o add_test add_test.cc
    $ ./add_test
    [doctest] doctest version is "2.4.6"
    [doctest] run with "--help" for options
    ===============================================================================
    [doctest] test cases:      1 |      1 passed |      0 failed |      0 skipped
    [doctest] assertions:      2 |      2 passed |      0 failed |
    [doctest] Status: SUCCESS!


In the turbo project, turbo uses cmake as the build system, and encapsulates the ci
system based on cmake. Call carbin-ccmake. You can see the specific modules in the
carbin_cmake directory of the project. Next, use turbo to perform integrated compilation testing.

..  code-block:: cmake

    carbin_cc_test(
        NAME
        add_test
        SOURCES
        "add_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}


the above is the cmake code for compiling the add_test.cc file. The carbin_cc_test is a ``cmake``
function provided by ``carbin``.

..  note::

    to install carbin-cmake is easy, first ``pip install catbin``, then ``carbin ccreate
    --name project_name --requirements --test  --example``.

add the above cmake code to the CMakeLists.txt file in the project test directory, and then
execute the following command to compile and run the test:

..  code-block:: console

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ make test

if no error curred, that means the test is passed.