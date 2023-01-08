
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_FILES_TEMP_FILE_H_
#define FLARE_FILES_TEMP_FILE_H_

#include "flare/base/profile.h"
#include "flare/files/filesystem.h"

namespace flare {

    // Create a temporary file in current directory, which will be deleted when
    // corresponding temp_file object destructs, typically for unit testing.
    //
    // Usage:
    //   {
    //      temp_file tmpfile;           // A temporay file shall be created
    //      tmpfile.save("some text");  // Write into the temporary file
    //   }
    //   // The temporary file shall be removed due to destruction of tmpfile

    class temp_file {
    public:
        // Create a temporary file in current directory. If |ext| is given,
        // filename will be temp_file_XXXXXX.|ext|, temp_file_XXXXXX otherwise.
        // If temporary file cannot be created, all save*() functions will
        // return -1. If |ext| is too long, filename will be truncated.
        temp_file();

        explicit temp_file(const char *ext);

        // The temporary file is removed in destructor.
        ~temp_file();

        // Save |content| to file, overwriting existing file.
        // Returns 0 when successful, -1 otherwise.
        int save(const char *content);

        // Save |fmt| and associated values to file, overwriting existing file.
        // Returns 0 when successful, -1 otherwise.
        int save_format(const char *fmt, ...) __attribute__((format (printf, 2, 3)));

        // Save binary data |buf| (|count| bytes) to file, overwriting existing file.
        // Returns 0 when successful, -1 otherwise.
        int save_bin(const void *buf, size_t count);

        // Get name of the temporary file.
        const char *fname() const { return _fname; }

    private:
        // temp_file is associated with file, copying makes no sense.
        FLARE_DISALLOW_COPY_AND_ASSIGN(temp_file);

        int _reopen_if_necessary();

        int _fd;                // file descriptor
        int _ever_opened;
        char _fname[24];        // name of the file
    };

} // namespace flare

#endif  // FLARE_FILES_TEMP_FILE_H_
