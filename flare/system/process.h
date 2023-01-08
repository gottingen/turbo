//
// Created by liyinbin on 2022/9/5.
//

#ifndef FLARE_SYSTEM_PROCESS_H_
#define FLARE_SYSTEM_PROCESS_H_

#include <sys/types.h>
#include <ostream>

namespace flare {

    // Read command line of this program. If `with_args' is true, args are
    // included and separated with spaces.
    // Returns length of the command line on sucess, -1 otherwise.
    // NOTE: `buf' does not end with zero.
    ssize_t read_command_line(char *buf, size_t len, bool with_args);

    // Read the stdout of child process executing `cmd'.
    // Returns the exit status(0-255) of cmd and all the output is stored in
    // |os|. -1 otherwise and errno is set appropriately.
    int read_command_output(std::ostream &os, const char *cmd);

}  // namespace flare
#endif  // FLARE_SYSTEM_PROCESS_H_
