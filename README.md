turbo * c++ common library
====

Turbo is a library of c++17 components designed with practicality
and efficiently in mind. However, it seems like stl, but it just 
augments to stl.


# build

Since designed the Turbo, easy to build and ingest is one of its goals.
so we make it's using as easy as we can. details see [build](docs/build.md)

for easy to ingest, Turbo have package on anaconda. When you on conda environment,
you can get it as below.

```shell
    conda install -c mgottingen turbo
```

or 

```shell
    conda install -c conda-forge turbo
```

# Try Try Try

* Read [overview](docs/overview.md) to know the goals of turbo and its advantages. 
* Read [getting start](docs/getting_start.md) for building turbo and work with [examples](examples).
* Modules:
  * [platform](docs/platform.md) 
  * [Status](docs/status.md)
  * [base](docs/base.md)

# Acknowledgement

* [Abseil](github.com/abseil/abseil-cpp)
* [Folly](github.com/facebook/folly)
* [Onnxruntime](github.com/microsoft/onnxruntime)
* [brpc](github.com/apache/brpc)
* [tvm](github.com/apache/tvm)
* [EASTL](https://github.com/electronicarts/EASTL)