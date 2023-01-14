
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <cxxabi.h>                              // __cxa_demangle
#include <string>                                // std::string
#include <cstdlib>                              // free()

namespace turbo::base {

    // Try to convert mangled |name| to human-readable name.
    // Returns:
    //   |name|    -  Fail to demangle |name|
    //   otherwise -  demangled name
    std::string demangle(const char *name) {
        // mangled_name
        //   A nullptr-terminated character string containing the name to
        //   be demangled.
        // output_buffer:
        //   A region of memory, allocated with malloc, of *length bytes,
        //   into which the demangled name is stored. If output_buffer is
        //   not long enough, it is expanded using realloc. output_buffer
        //   may instead be nullptr; in that case, the demangled name is placed
        //   in a region of memory allocated with malloc.
        // length
        //   If length is non-nullptr, the length of the buffer containing the
        //   demangled name is placed in *length.
        // status
        //   *status is set to one of the following values:
        //    0: The demangling operation succeeded.
        //   -1: A memory allocation failure occurred.
        //   -2: mangled_name is not a valid name under the C++ ABI
        //       mangling rules.
        //   -3: One of the arguments is invalid.
        int status = 0;
        char *buf = abi::__cxa_demangle(name, nullptr, nullptr, &status);
        if (status == 0) {
            std::string s(buf);
            free(buf);
            return s;
        }
        return std::string(name);
    }

}  // namespace turbo::base

