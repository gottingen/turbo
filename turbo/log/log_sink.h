// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//
// -----------------------------------------------------------------------------
// File: log/log_sink.h
// -----------------------------------------------------------------------------
//
// This header declares the interface class `turbo::LogSink`.

#pragma once

#include <turbo/base/config.h>
#include <turbo/log/log_entry.h>

namespace turbo {

    // turbo::LogSink
    //
    // `turbo::LogSink` is an interface which can be extended to intercept and
    // process particular messages (with `LOG.ToSinkOnly()` or
    // `LOG.ToSinkAlso()`) or all messages (if registered with
    // `turbo::add_log_sink`).  Implementations must not take any locks that might be
    // held by the `LOG` caller.
    class LogSink {
    public:
        virtual ~LogSink() = default;

        // LogSink::Send()
        //
        // `Send` is called synchronously during the log statement.  `Send` must be
        // thread-safe.
        //
        // It is safe to use `LOG` within an implementation of `Send`.  `ToSinkOnly`
        // and `ToSinkAlso` are safe in general but can be used to create an infinite
        // loop if you try.
        virtual void Send(const turbo::LogEntry &entry) = 0;

        // LogSink::Flush()
        //
        // Sinks that buffer messages should override this method to flush the buffer
        // and return.  `Flush` must be thread-safe.
        virtual void Flush() {}

    protected:
        LogSink() = default;

        // Implementations may be copyable and/or movable.
        LogSink(const LogSink &) = default;

        LogSink &operator=(const LogSink &) = default;

    private:
        // https://lld.llvm.org/missingkeyfunction.html#missing-key-function
        virtual void KeyFunction() const final;  // NOLINT(readability/inheritance)
    };

}  // namespace turbo
