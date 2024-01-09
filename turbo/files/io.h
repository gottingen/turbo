// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//
// Created by jeff on 24-1-8.
//

#ifndef TURBO_FILES_IO_H_
#define TURBO_FILES_IO_H_

#include "turbo/files/io/iobuf.h"
#include "turbo/files/io/fd_guard.h"
#include "turbo/files/io/fd_utility.h"
#include "turbo/files/io/iobuf_printer.h"

namespace turbo {

    using turbo::files_internal::IOBuf;
    using turbo::files_internal::IOBufBuilder;
    using turbo::files_internal::IOBufAppender;
    using turbo::files_internal::IOBufCutter;
    using turbo::files_internal::IOBufAsZeroCopyInputStream;
    using turbo::files_internal::IOBufAsZeroCopyOutputStream;
    using turbo::files_internal::IOPortal;
    using turbo::files_internal::IOBufBytesIterator;

}  // namespace turbo

#endif  // TURBO_FILES_IO_H_