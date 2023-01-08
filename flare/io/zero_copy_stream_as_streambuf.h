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

// Date: Thu Nov 22 13:57:56 CST 2012

#ifndef FLARE_IO_ZERO_COPY_STREAM_AS_STREAMBUF_H_
#define FLARE_IO_ZERO_COPY_STREAM_AS_STREAMBUF_H_

#include <streambuf>
#include <google/protobuf/io/zero_copy_stream.h>

namespace flare {

// Wrap a ZeroCopyOutputStream into std::streambuf. Notice that before 
// destruction or shrink(), BackUp() of the stream are not called. In another
// word, if the stream is wrapped from cord_buf, the cord_buf may be larger than
// appended data.
    class zero_copy_stream_as_stream_buf : public std::streambuf {
    public:
        zero_copy_stream_as_stream_buf(google::protobuf::io::ZeroCopyOutputStream *stream)
                : _zero_copy_stream(stream) {}

        virtual ~zero_copy_stream_as_stream_buf();

        // BackUp() unused bytes. Automatically called in destructor.
        void shrink();

    protected:
        int overflow(int ch) override;

        int sync() override;

        std::streampos seekoff(std::streamoff off,
                               std::ios_base::seekdir way,
                               std::ios_base::openmode which) override;

    private:
        google::protobuf::io::ZeroCopyOutputStream *_zero_copy_stream;
    };

}  // namespace flare

#endif  // FLARE_IO_ZERO_COPY_STREAM_AS_STREAMBUF_H_
