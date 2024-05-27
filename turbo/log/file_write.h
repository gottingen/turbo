//
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

#pragma once

#include <turbo/base/config.h>
#include <turbo/strings/string_view.h>

namespace turbo {
    TURBO_NAMESPACE_BEGIN

    class FileWriter {
    public:
        virtual ~FileWriter() = default;

        // Initialize the file writer with the given path.
        // Returns 0 on success, or an error code on failure.
        // as the meaning that this function should open the
        // file ready for writing, and return 0 on success.
        virtual int initialize(turbo::string_view path) = 0;

        // reinitialize the file writer with the given path.
        // Returns 0 on success, or an error code on failure.
        // as the meaning that this function should open the
        // file ready for writing, and return 0 on success.
        // for that, some time, the file may be removed or
        // the file may be renamed by other process, such as
        //  some body remove it in the shell. avoid the log
        //  write to a black hole, we should reopen the file
        //  and write the log to the new file.
        virtual int reopen() = 0;

        // Write the given message to the file.
        virtual ssize_t write(turbo::string_view message) = 0;

        // Flush the file writer.
        virtual void flush() = 0;

        // Close the file writer.
        virtual void close() = 0;
    };

    TURBO_NAMESPACE_END
}  // namespace turbo
