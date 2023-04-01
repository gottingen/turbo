build turbo
====

# build environment requirement

* Cmake 3.17 is the minimum supported version.
* gcc >= 7.3 or Apple Clang > 12

## centos

```shell
sudo yum install centos-release-scl
sudo yum install devtoolset-7-gcc*
scl enable devtoolset-7 bash
```
## ubuntu 20.04

```shell
sudo apt install gcc g++
```

## Macbook
 as It is.
 
## conda environment

### Apple Macbook
    
```shell
conda install clangxx clang
```

### Linux

```shell
conda install gxx=8.5
```
# build

```shell
git clone github.com/gottingen/turbo.git
cd turbo
mkdir build
cd build
cmake ..
make test
```

# build conda package


```shell
git clone github.com/gottingen/turbo.git
cd turbo/conda
conda build .
```
