# Turbo CMake Build Instructions

Turbo comes with a CMake build script ([CMakeLists.txt](../CMakeLists.txt))
that can be used on a wide range of platforms ("C" stands for cross-platform.).
If you don't have CMake installed already, you can download it for free from
<https://www.cmake.org/>.

CMake works by generating native makefiles or build projects that can
be used in the compiler environment of your choice.

For API/ABI compatibility reasons, we strongly recommend building Turbo in a
subdirectory of your project or as an embedded dependency.

## Incorporating Turbo Into a CMake Project

The recommendations below are similar to those for using CMake within the
googletest framework
(<https://github.com/google/googletest/blob/master/googletest/README.md#incorporating-into-an-existing-cmake-project>)

### Step-by-Step Instructions

1. If you want to build the Turbo tests, integrate the Turbo dependency
[Google Test](https://github.com/google/googletest) into your CMake
project. To disable Turbo tests, you have to pass either
`-DBUILD_TESTING=OFF` or `-DTURBO_BUILD_TESTING=OFF` when configuring your
project with CMake.

2. Download Turbo and copy it into a subdirectory in your CMake project or add
Turbo as a [git submodule](https://git-scm.com/docs/git-submodule) in your
CMake project.

3. You can then use the CMake command
[`add_subdirectory()`](https://cmake.org/cmake/help/latest/command/add_subdirectory.html)
to include Turbo directly in your CMake project.

4. Add the **turbo::** target you wish to use to the
[`target_link_libraries()`](https://cmake.org/cmake/help/latest/command/target_link_libraries.html)
section of your executable or of your library.<br>
Here is a short CMakeLists.txt example of an application project using Turbo.

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_app_project)

# Pick the C++ standard to compile with.
# Turbo currently supports C++14, C++17, and C++20.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(abseil-cpp)

add_executable(my_exe source.cpp)
target_link_libraries(my_exe turbo::base turbo::synchronization turbo::strings)
```

Note that if you are developing a library designed for use by other clients, you
should instead leave `CMAKE_CXX_STANDARD` unset (or only set if being built as
the current top-level CMake project) and configure the minimum required C++
standard at the target level. If you require a later minimum C++ standard than
Turbo does, it's a good idea to also enforce that `CMAKE_CXX_STANDARD` (which
will control Turbo library targets) is set to at least that minimum. For
example:

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_lib_project)

# Leave C++ standard up to the root application, so set it only if this is the
# current top-level CMake project.
if(CMAKE_SOURCE_DIR STREQUAL my_lib_project_SOURCE_DIR)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

add_subdirectory(abseil-cpp)

add_library(my_lib source.cpp)
target_link_libraries(my_lib turbo::base turbo::synchronization turbo::strings)

# Enforce that my_lib requires C++17. Important to document for clients that they
# must set CMAKE_CXX_STANDARD to 17 or higher for proper Turbo ABI compatibility
# (since otherwise, Turbo library targets could be compiled with a lower C++
# standard than my_lib).
target_compile_features(my_lib PUBLIC cxx_std_17)
if(CMAKE_CXX_STANDARD LESS 17)
  message(FATAL_ERROR
      "my_lib_project requires CMAKE_CXX_STANDARD >= 17 (got: ${CMAKE_CXX_STANDARD})")
endif()
```

Then the top-level application project that uses your library is responsible for
setting a consistent `CMAKE_CXX_STANDARD` that is sufficiently high.

### Running Turbo Tests with CMake

Use the `-DTURBO_BUILD_TESTING=ON` flag to run Turbo tests.  Note that
BUILD_TESTING must also be on (the default).

You will need to provide Turbo with a Googletest dependency.  There are two
options for how to do this:

* Use `-DTURBO_USE_GOOGLETEST_HEAD`.  This will automatically download the latest
Googletest source into the build directory at configure time.  Googletest will
then be compiled directly alongside Turbo's tests.
* Manually integrate Googletest with your build.  See
https://github.com/google/googletest/blob/master/googletest/README.md#using-cmake
for more information on using Googletest in a CMake project.

For example, to run just the Turbo tests, you could use this script:

```
cd path/to/abseil-cpp
mkdir build
cd build
cmake -DTURBO_BUILD_TESTING=ON -DTURBO_USE_GOOGLETEST_HEAD=ON ..
make -j
ctest
```

Currently, we only run our tests with CMake in a Linux environment, but we are
working on the rest of our supported platforms. See
https://github.com/abseil/abseil-cpp/projects/1 and
https://github.com/abseil/abseil-cpp/issues/109 for more information.

### Available Turbo CMake Public Targets

Here's a non-exhaustive list of Turbo CMake public targets:

```cmake
turbo::algorithm
turbo::base
turbo::debugging
turbo::flat_hash_map
turbo::flags
turbo::memory
turbo::meta
turbo::numeric
turbo::random_random
turbo::strings
turbo::synchronization
turbo::time
turbo::utility
```

## Traditional CMake Set-Up

For larger projects, it may make sense to use the traditional CMake set-up where you build and install projects separately.

First, you'd need to build and install Google Test:
```
cmake -S /source/googletest -B /build/googletest -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/installation/dir -DBUILD_GMOCK=ON
cmake --build /build/googletest --target install
```

Then you need to configure and build Turbo. Make sure you enable `TURBO_USE_EXTERNAL_GOOGLETEST` and `TURBO_FIND_GOOGLETEST`. You also need to enable `TURBO_ENABLE_INSTALL` so that you can install Turbo itself.
```
cmake -S /source/abseil-cpp -B /build/abseil-cpp -DCMAKE_PREFIX_PATH=/installation/dir -DCMAKE_INSTALL_PREFIX=/installation/dir -DTURBO_ENABLE_INSTALL=ON -DTURBO_USE_EXTERNAL_GOOGLETEST=ON -DTURBO_FIND_GOOGLETEST=ON
cmake --build /temporary/build/abseil-cpp
```

(`CMAKE_PREFIX_PATH` is where you already have Google Test installed; `CMAKE_INSTALL_PREFIX` is where you want to have Turbo installed; they can be different.)

Run the tests:
```
ctest --test-dir /temporary/build/abseil-cpp
```

And finally install:
```
cmake --build /temporary/build/abseil-cpp --target install
```

# CMake Option Synposis

## Enable Standard CMake Installation

`-DTURBO_ENABLE_INSTALL=ON`

## Google Test Options

`-DTURBO_BUILD_TESTING=ON` must be set to enable testing

- Have Turbo download and build Google Test for you: `-DTURBO_USE_EXTERNAL_GOOGLETEST=OFF` (default)
  - Download and build latest Google Test: `-DTURBO_USE_GOOGLETEST_HEAD=ON`
  - Download specific Google Test version (ZIP archive): `-DTURBO_GOOGLETEST_DOWNLOAD_URL=https://.../version.zip`
  - Use Google Test from specific local directory: `-DTURBO_LOCAL_GOOGLETEST_DIR=/path/to/googletest`
- Use Google Test included elsewhere in your project: `-DTURBO_USE_EXTERNAL_GOOGLETEST=ON`
- Use standard CMake `find_package(CTest)` to find installed Google Test: `-DTURBO_USE_EXTERNAL_GOOGLETEST=ON -DTURBO_FIND_GOOGLETEST=ON`
