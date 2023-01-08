
#include <fcntl.h>                      // open
#include <stdio.h>                      // snprintf
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>                     // read, gitpid
#include <sstream>                      // std::ostringstream
#include "flare/base/fd_guard.h"             // flare::base::fd_guard
#include "flare/log/logging.h"
#include "flare/system/process.h"
#include "flare/base/profile.h"
#include <gflags/gflags.h>

#if defined(FLARE_PLATFORM_LINUX)
// clone is a linux specific syscall
#include <sched.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

namespace flare {


#if defined(FLARE_PLATFORM_LINUX)

    const int CHILD_STACK_SIZE = 256 * 1024;

    struct ChildArgs {
        const char* cmd;
        int pipe_fd0;
        int pipe_fd1;
    };

    int launch_child_process(void* args) {
        ChildArgs* cargs = (ChildArgs*)args;
        dup2(cargs->pipe_fd1, STDOUT_FILENO);
        close(cargs->pipe_fd0);
        close(cargs->pipe_fd1);
        execl("/bin/sh", "sh", "-c", cargs->cmd, nullptr);
        _exit(1);
    }

    int read_command_output_through_clone(std::ostream& os, const char* cmd) {
        int pipe_fd[2];
        if (pipe(pipe_fd) != 0) {
            FLARE_PLOG(ERROR) << "Fail to pipe";
            return -1;
        }
        int saved_errno = 0;
        int wstatus = 0;
        pid_t cpid;
        int rc = 0;
        ChildArgs args = { cmd, pipe_fd[0], pipe_fd[1] };
        char buffer[1024];

        char* child_stack = nullptr;
        char* child_stack_mem = (char*)malloc(CHILD_STACK_SIZE);
        if (!child_stack_mem) {
            FLARE_LOG(ERROR) << "Fail to alloc stack for the child process";
            rc = -1;
            goto END;
        }
        child_stack = child_stack_mem + CHILD_STACK_SIZE;
                                   // ^ Assume stack grows downward
        cpid = clone(launch_child_process, child_stack,
                     __WCLONE | CLONE_VM | SIGCHLD | CLONE_UNTRACED, &args);
        if (cpid < 0) {
            FLARE_PLOG(ERROR) << "Fail to clone child process";
            rc = -1;
            goto END;
        }
        close(pipe_fd[1]);
        pipe_fd[1] = -1;

        for (;;) {
            const ssize_t nr = read(pipe_fd[0], buffer, sizeof(buffer));
            if (nr > 0) {
                os.write(buffer, nr);
                continue;
            } else if (nr == 0) {
                break;
            } else if (errno != EINTR) {
                FLARE_LOG(ERROR) << "Encountered error while reading for the pipe";
                break;
            }
        }

        close(pipe_fd[0]);
        pipe_fd[0] = -1;

        for (;;) {
            pid_t wpid = waitpid(cpid, &wstatus, WNOHANG | __WALL);
            if (wpid > 0) {
                break;
            }
            if (wpid == 0) {
                usleep(1000);
                continue;
            }
            rc = -1;
            goto END;
        }

        if (WIFEXITED(wstatus)) {
            rc = WEXITSTATUS(wstatus);
            goto END;
        }

        if (WIFSIGNALED(wstatus)) {
            os << "Child process(" << cpid << ") was killed by signal "
               << WTERMSIG(wstatus);
        }

        rc = -1;
        errno = ECHILD;

    END:
        saved_errno = errno;
        if (child_stack_mem) {
            free(child_stack_mem);
        }
        if (pipe_fd[0] >= 0) {
            close(pipe_fd[0]);
        }
        if (pipe_fd[1] >= 0) {
            close(pipe_fd[1]);
        }
        errno = saved_errno;
        return rc;
    }

    DEFINE_bool(run_command_through_clone, false,
                "(Linux specific) Run command with clone syscall to "
                "avoid the costly page table duplication");

#endif // FLARE_PLATFORM_LINUX

    int read_command_output_through_popen(std::ostream &os, const char *cmd) {
        FILE *pipe = popen(cmd, "r");
        if (pipe == nullptr) {
            return -1;
        }
        char buffer[1024];
        for (;;) {
            size_t nr = fread(buffer, 1, sizeof(buffer), pipe);
            if (nr != 0) {
                os.write(buffer, nr);
            }
            if (nr != sizeof(buffer)) {
                if (feof(pipe)) {
                    break;
                } else if (ferror(pipe)) {
                    FLARE_LOG(ERROR) << "Encountered error while reading for the pipe";
                    break;
                }
                // retry;
            }
        }

        const int wstatus = pclose(pipe);

        if (wstatus < 0) {
            return wstatus;
        }
        if (WIFEXITED(wstatus)) {
            return WEXITSTATUS(wstatus);
        }
        if (WIFSIGNALED(wstatus)) {
            os << "Child process was killed by signal "
               << WTERMSIG(wstatus);
        }
        errno = ECHILD;
        return -1;
    }


    int read_command_output(std::ostream &os, const char *cmd) {
#if !defined(FLARE_PLATFORM_LINUX)
        return read_command_output_through_popen(os, cmd);
#else
        return FLAGS_run_command_through_clone
            ? read_command_output_through_clone(os, cmd)
            : read_command_output_through_popen(os, cmd);
#endif
    }

    ssize_t read_command_line(char* buf, size_t len, bool with_args) {
#if defined(FLARE_PLATFORM_LINUX)
        flare::base::fd_guard fd(open("/proc/self/cmdline", O_RDONLY));
    if (fd < 0) {
        FLARE_LOG(ERROR) << "Fail to open /proc/self/cmdline";
        return -1;
    }
    ssize_t nr = read(fd, buf, len);
    if (nr <= 0) {
        FLARE_LOG(ERROR) << "Fail to read /proc/self/cmdline";
        return -1;
    }
#elif defined(FLARE_PLATFORM_OSX)
        static pid_t pid = getpid();
        std::ostringstream oss;
        char cmdbuf[32];
        snprintf(cmdbuf, sizeof(cmdbuf), "ps -p %ld -o command=", (long)pid);
        if (flare::read_command_output(oss, cmdbuf) != 0) {
            FLARE_LOG(ERROR) << "Fail to read cmdline";
            return -1;
        }
        const std::string& result = oss.str();
        ssize_t nr = std::min(result.size(), len);
        memcpy(buf, result.data(), nr);
#else
#error Not Implemented
#endif

        if (with_args) {
            if ((size_t)nr == len) {
                return len;
            }
            for (ssize_t i = 0; i < nr; ++i) {
                if (buf[i] == '\0') {
                    buf[i] = '\n';
                }
            }
            return nr;
        } else {
            for (ssize_t i = 0; i < nr; ++i) {
                // The command in macos is separated with space and ended with '\n'
                if (buf[i] == '\0' || buf[i] == '\n' || buf[i] == ' ') {
                    return i;
                }
            }
            if ((size_t)nr == len) {
                FLARE_LOG(ERROR) << "buf is not big enough";
                return -1;
            }
            return nr;
        }
    }

} // namespace flare