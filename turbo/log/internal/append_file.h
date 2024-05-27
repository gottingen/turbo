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

#include <cstdio>
#include <turbo/log/file_write.h>

namespace turbo::log_internal {

    class AppendFile : public turbo::FileWriter {
    public:
        AppendFile() = default;

        ~AppendFile() override;

        // Initialize the file writer with the given path.
        // Returns 0 on success, or an error code on failure.
        // as the meaning that this function should open the
        // file ready for writing, and return 0 on success.
        int initialize(turbo::string_view path) override;

        // reinitialize the file writer with the given path.
        // Returns 0 on success, or an error code on failure.
        // as the meaning that this function should open the
        // file ready for writing, and return 0 on success.
        // for that, some time, the file may be removed or
        // the file may be renamed by other process, such as
        //  some body remove it in the shell. avoid the log
        //  write to a black hole, we should reopen the file
        //  and write the log to the new file.
        int reopen() override;

        // Write the given message to the file.
        ssize_t write(turbo::string_view message) override;

        // Flush the file writer.
        void flush() override;

        // Close the file writer.
        void close() override;

    private:
        std::string _path;
        char _buffer[64 * 1024];
        FILE *_file = nullptr;

    };
}  // namespace turbo::log_internal