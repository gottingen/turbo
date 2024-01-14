Release
==================================

v0.9.43 -
================================

addition
--------------------------------

* add test for atexit, thread_atexit.
* add temp file for temporary file operation.
* add iobuf for io buffer operation, which is a buffer for file and network io.
* add virtual zero copy stream for zero copy operation.
* add zero copy stream for zero copy operation.
* add zero copy iobuf for io operations, both network and file.
* add temp file for temporary file operation.
* add Filesystem Adaptor for file system operation. using can define it own tag Adaptors, and 
  using Filesystem<Tag> for a unified interface. default tag is system filesystem, eg some case, user
  may define the it based on rocksdb with kv, my define  a rocks_kv_tag, and using Filesystem<rocks_kv_tag>
  to access.

feature improvement
--------------------------------
* tidy file descriptor operation to file io module.
* promote the sequential write file io, avoid c-style file cache.
* add ignore case hash set/map

bug fix
--------------------------------

* atexit and thread_atexit will cause a crash when main thread and the variable already destructed.
* fix gcc12 compile error for <string> not directly included in hash engine
* fix gcc12 compile error for <cstddef> not directly included in compress tuple
* fix global operator `%` `+` `-` `*` `/` for Duration will modify rhs or lhs value.

v0.9.39 - v0.9.43
================================

addition
--------------------------------

* add support for fiber
* add support for taskflow
* add support for fiber mutex and condition variable both in pthread and fiber.
* add support for unicode and unicode view.
* add support simd(AVX2) implementation of utf8, utf16, utf32.
* add TDLOG_* for turbo log, the log on effect only when NDEBUG is not defined.
* add spin lock for small contention race, that replace mutex.
* add platform thread for thread low level operation.
* add memory info for low level memory operation.
* add intrusive list for no memory allocation list.
* add timer thread for timer operation.

feature improvement
--------------------------------
* improve turbo::Status for more friendly interface, remove the [[nodiscard]] attribute.
* using [[nodiscard]] for TURBO_MUST_USE_RESULT promote to c++17.
* using turbo::Time and Duration replace the microsecond and millisecond typed by
  int64_t, which Semantics is not clear. by using turbo::Time and Duration, the
  semantics is clear. Time is the time point, Duration is the time interval.
* remove TURBO_INLINE_NAMESPACE, because it is not necessary.
* remove macros for compact feature before c++17 for turbo require c++17.

bug fix
--------------------------------

* fix bug in turbo::format::format, when the format string is empty, it will cause a crash.
* fix bug in unicode test for random utf16 string generation.
* fix bug for TURBO_ASSERT, when NDEBUG is defined, it will cause a crash.
* fix bug for profiling operator<<, when the value not the same, but convert to the same type.

v0.9.36 - v0.9.39
================================

* add support for thread local storage.
* tidy implementation of format and table modules.
* add function for fast random number generation.
* remove turbo meta traits that c++17 now provides.
* add hash algorithm for xxhash.
* add hash algorithm for murmur3.
* add hash framework for multiple hash algorithms selection.
* support profiling of prometheus metrics.
* add support for profiling module.
* boost some implementations  to constexpr.
* add turbo cmd line tool.

# pre-release v0.9.36

# base version v0.9.35