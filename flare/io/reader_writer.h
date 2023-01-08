// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// Date: Wed Aug  8 05:51:33 PDT 2018

#ifndef FLARE_IO_READER_WRITER_H_
#define FLARE_IO_READER_WRITER_H_

#include <sys/uio.h>                             // iovec

namespace flare {

    // Abstraction for reading data.
    // The simplest implementation is to embed a file descriptor and read from it.
    class base_reader {
    public:
        virtual ~base_reader() {}

        // Semantics of parameters and return value are same as readv(2) except that
        // there's no `fd'.
        virtual ssize_t readv(const iovec *iov, int iovcnt) = 0;
    };

    // Abstraction for writing data.
    // The simplest implementation is to embed a file descriptor and writev into it.
    class base_writer {
    public:
        virtual ~base_writer() {}

        // Semantics of parameters and return value are same as writev(2) except that
        // there's no `fd'.
        // writev is required to submit data gathered by multiple appends in one
        // run and enable the possibility of atomic writes.
        virtual ssize_t writev(const iovec *iov, int iovcnt) = 0;
    };

}  // namespace flare

#endif  // FLARE_IO_READER_WRITER_H_
