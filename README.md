# turbo - c++ base library

turbo is an Oteam collection of c++ library code. it designed to use c++ smartly.

## status

platform|compiler | status
:--- | :---| :---
centos7 | gcc-8.5 | ok
mac os  | llvm    | ok

## content index

* [about turbo](#about)
* [about cmake](#cmake)
* [build turbo](#build)
* [modules](#modules)
* [examples](#examples)
* [papers](#papers)
* [topics](#topics)

<a name="about"> </a>

## about turbo

Since the inception, it's aimed to <font size=16> **`consolidate algorithms, data structures, system operations and 
make sure it's under control.`**</font>
Particularly, the goals are:

* to have a library that has been well implemented and tested containing tools and algorithm.
* aim high modularity with reduced dependencies between modules.
* zero external dependencies.
* build on all platforms with c++, such as linux, mac, android, windows, mobiles.
* no warning and bugs on any platform and compiler.
* published interfaces are required to have full documentation， using case description，performance benchmark and evaluation.
* keep overhead down, compress overall size.


<a name="cmake"> </a>
 
## about cmake


<a name="build"> </a>

## build turbo

turbo use cmake as build system. sample to build turbo

compiler requirement

  - clang version > 3.3
  - gcc version > 8.5
  - cmake version > 3.15
  - conda

build step

turbo dependency managed by conda, first need install conda
depends by conda, on mac
```shell
  conda env create -f conda/environment_osx.yaml
  conda activate turbo-dev
  mkdir build
  cd build
  cmake ..
  make 
```
on linux
```shell
  conda env create -f conda/environment_linux.yaml
  conda activate turbo-dev
  mkdir build
  cd build
  cmake ..
  make 
```
    
<a name="modules"> </a>

## modules

* [base](/docs/en/base.md)
    
    the base module contains code that other modules depends on. no extern dependencies.
* [algorithm](/docs/en/algorithm.md)
* [atomic](/docs/en/atomic.md)
* [chrono](/docs/en/chrono.md) 
* [container](/docs/en/container.md)
* [debugging](/docs/en/debugging.md)
* [digest](/docs/en/digest.md) 

    the digest module contains md5, sha1, sha256 tools.
    
* [filesystem](/docs/en/filesystem.md)

    the filesystem module contain a C++17-like filesystem implementation for C++11/C++147/C++17

* [flags](/docs/en/flags.md)
* [format](/docs/en/format.md)
* [strings](/docs/en/strings.md) 

    strings library contains string utilities, such as trim, split. also include a 
    compatible version of string_view.

<a name="examples"> </a>

## examples

<a name="papers"> </a>

## papers

* [Working Draft N4687](/docs/documnet/n4687.pdf)

<a name="topic"> </a>

## topics

* [memory](/docs/en/topic/memory.md) 
* [concurrent](/docs/en/topic/concurrent.md)