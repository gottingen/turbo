turbo * c++ common library
====

Turbo is a library of c++17 components designed with practicality
and efficiently in mind. However, it seems like stl, but it just 
augments to stl.


# build

Since designed the Turbo, easy to build and ingest is one of its goals.
so we make it's using as easy as we can.
## dependencies

* cmake version > 3.15
* ApplyClang > 12
* gcc >= 9.3
* 
## on macbook

```shell
    git clone github.com/gottingen/turbo.git
    cd turbo
    mkdir build
    cd build
    cmake .. -DTURBO_BUILD_TESTING=ON
    make -j 4
```

## on ubuntu20.04
ubuntu 20.04 original gcc/g++ version is 9.3.
**prepare env**
```shell
    sudo apt-get install gcc g++
    sudo apt-get imstall cmake
```
**build** 
```shell
    git clone github.com/gottingen/turbo.git
    cd turbo
    mkdir build
    cd build
    cmake .. -DTURBO_BUILD_TESTING=ON
    make -j 4
```
## on centos7

centos7 we need to use scl to upgrade our compiler.

```shell
sudo yum install centos-release-scl
sudo yum install devtoolset-9-gcc*
scl enable devtoolset-9 bash
```

```shell
    git clone github.com/gottingen/turbo.git
    cd turbo
    mkdir build
    cd build
    cmake .. -DTURBO_BUILD_TESTING=ON
    make -j 4
```
## conda env


