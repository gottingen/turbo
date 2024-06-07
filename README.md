turbo * c++ common library
====
[Documentation](https://turbo-docs.readthedocs.io/) |
[Acknowledge](https://turbo-docs.readthedocs.io/) |
[Contributors](CONTRIBUTORS.md) |
[Release Notes](NEWS.md)

<div align="center">
<img src=docs/source/image/ticon.png width=240 height=200 />
</div>

Turbo is a library of c++17 components designed with practicality
and efficiently in mind. However, it seems like stl, but it just 
augments to stl. Turbo is the foundation of gottingen ai inf.

[online docs](https://turbo-docs.readthedocs.io/)


# build

Since designed the Turbo, easy to build and ingest is one of its goals.
so we make it's using as easy as we can.
```shell
git clone https://github.com/gottingen/turbo.git
pip install carbin
cd turbo
carbin install
mkdir build && cd build
cmake ..
make -j 8
make test
```

for easy to ingest, Turbo have package on anaconda. When you on conda environment,
you can get it as below.

```shell
    conda install -c mgottingen turbo
```

it also can be install by [carbin](https://github.com/gottingen/carbin).

```shell
pip install carbin
carbin install gottingen/turbo
```

# Try Try Try

* Read [overview](https://turbo-docs.readthedocs.io/en/latest/en/overview.html) to know the goals of turbo and its advantages. 
* Read [getting start](https://turbo-docs.readthedocs.io/en/latest/) for building turbo and work with [examples](examples).
* Api reference is [here](https://turbo-docs.readthedocs.io/en/latest/en/api/base.html).
* Carbin is a tool to manage c++ project, it's [here](https://carbin.readthedocs.io/en/latest/).
* Modules:
  * platform compact cross platform
  * base basic types and functions
  * fiber & flow fiber and task control flow -- **good performance**
    * Fiber
    * FiberMutex
    * FiberCond
    * FiberBarrier
    * TaskFlow
  * simd simd instructions abstraction to batch processing -- **good performance**
  * flags cmd line tools help to parse cmd line
  * strings strings and std::string_view processing
  * concurrent threading and lock options
    * Barrier
    * CallOnce
    * SpinLock
    * ThreadLocal
  * times time and date processing -- **good performance**
    * Time
    * Duration
    * CivilTime
    * TimeZone
  * format  string format and table format -- **good performance**
    * format string
    * println
    * table format
  * logging logging and log to file -- **good performance**
  * files c++17 filesystem and convenient file api
    * SequentialReadFile
    * SequentialWriteFile
    * RandomAccessFile
    * RandomWriteFile
  * hash hash framework and hash functions -- **good performance**
    * city hash and bytes
    * murmur hash
    * xxhash
  * crypto
    * md5
    * sha1
    * sha256
    * sha512
    * crc32
  * Unicode multi engine unicode support scalar and AVX2 -- **good performance**
    * utf8
    * utf16
    * utf32
  * profiling profiling with write-most variables                 -- **good performance**
    * Counter
    * Histogram
    * Gauge
  * random random number generator                                -- **good performance**
    * Random
    * RandomEngine
    * RandomDevice
  * container                                                    -- **good performance**
    * small_vector stack allocated vector
    * flat_hash_map goode performance hash map 
    * flat_hash_set goode performance 
    * flat_tree_map goode performance 

# Acknowledgement

* [Turbo](github.com/abseil/abseil-cpp)
* [Folly](github.com/facebook/folly)
* [Onnxruntime](github.com/microsoft/onnxruntime)
* [brpc](github.com/apache/brpc)
* [tvm](github.com/apache/tvm)