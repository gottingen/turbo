Status User Guide
===

Turbo Status component contains some parts.

* turbo module has a help class for registry modules for status
* turbo error has a help class for registry errors for status
* A status library containing a turbo::Status class 
for holding error handling information, a set of canonical
`turbo error` error codes, and associated utilities for
generating and propagating status codes.
* A `statusor` library containing the `turbo::StatusOr<T>` class 
template for use in returning either a `turbo::Status` error 
or an object of type T. (This StatusOr<T> abstraction is 
similar to [C++23’s std::expected](https://en.cppreference.com/w/cpp/utility/expected)

# turbo error

Within `Gottingen group`, `turbo error`  is the primary mechanism 
to gracefully handle errors across API boundaries (and 
in particular across RPC boundaries and ai software). The error 
code range (-32768, 32768), with the help macro `TURBO_REGISTER_ERRNO`,
you can register the error you mind. if there is an error has
been register before, it will case compile error.

```c++
    TURBO_REGISTER_ERRNO(turbo::kOk, "OK");
    //TURBO_REGISTER_ERRNO(turbo::kOk, "ok"); this will rise to a compile error.
```

Method to get error description

```c++
    const char *TurboError(int error_code);
```

# turbo module

turbo module designed to discriminate different module registered 
in the system, by default, module `0` has been registered in 
the system named `TURBO`. for example, there need to develop
a new module 'turbo runtime', and register it as below

```c++
    constexpr unsigned short int TURBO_RUNTIME = 2;
    TURBO_REGISTER_MODULE_INDEX(TURBO_RUNTIME,"TURBO_RUNTIME");
```

There a global function to get module description

```c++
    const char *TurboModule(int module_index);
```
# get start with turbo::Status

Based on `turbo error` and `turbo module`, `turbo::Status`
is the primary mechanism to gracefully handle errors across
API boundaries (and in particular across RPC boundaries).
Some of these errors may be recoverable, but others may not.
Most functions which can produce a recoverable error should
be designed to return either an `turbo::Status` or the 
similar turbo::StatusOr<T>, which holds either an object 
of type T or an error.

Example:

```c++
turbo::Status MyFunction(absl::string_view filename, ...) {
  ...
  // encounter error
  if (error condition) {
    return turbo::invalid_argument_error("bad mode");
  }
  // else, return OK
  return absl::ok_status();
}
```

Most operations in gottingen code return an turbo::Status 
(abbreviated Status in the text below). A Status is designed 
to either return “OK” or one of a number of different error 
codes, corresponding to typical error conditions. In almost
all cases, when using turbo::Status you should use the 
canonical error codes (of type turbo::error). These canonical
codes are understood across the codebase and will be accepted 
across all API and RPC boundaries. A function which has a 
return value of Status must be handled (and is marked 
TURBO_MUST_USE_RESULT).




