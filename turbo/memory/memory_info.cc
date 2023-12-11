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

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "turbo/memory/memory_info.h"

#ifdef TURBO_PLATFORM_WINDOWS
#include <windows.h>
#include <psapi.h>
#undef min
#undef max
#include <windows.h>
#include <Psapi.h>
#undef min
#undef max
#elif defined(TURBO_PLATFORM_LINUX)

#include <sys/types.h>
#include <sys/sysinfo.h>

#elif defined(TURBO_PLATFORM_FREEBSD)
#include <paths.h>
#include <fcntl.h>
#include <kvm.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#endif

namespace turbo {

    uint64_t get_system_memory() {
#ifdef TURBO_PLATFORM_WINDOWS
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        return static_cast<uint64_t>(memInfo.ullTotalPageFile);
#elif defined(TURBO_PLATFORM_LINUX)
        struct sysinfo memInfo;
        sysinfo (&memInfo);
        auto totalVirtualMem = memInfo.totalram;

        totalVirtualMem += memInfo.totalswap;
        totalVirtualMem *= memInfo.mem_unit;
        return static_cast<uint64_t>(totalVirtualMem);
#elif defined(TURBO_PLATFORM_FREEBSD)
        kvm_t *kd;
        u_int pageCnt;
        size_t pageCntLen = sizeof(pageCnt);
        u_int pageSize;
        struct kvm_swap kswap;
        uint64_t totalVirtualMem;

        pageSize = static_cast<u_int>(getpagesize());

        sysctlbyname("vm.stats.vm.v_page_count", &pageCnt, &pageCntLen, NULL, 0);
        totalVirtualMem = pageCnt * pageSize;

        kd = kvm_open(NULL, _PATH_DEVNULL, NULL, O_RDONLY, "kvm_open");
        kvm_getswapinfo(kd, &kswap, 1, 0);
        kvm_close(kd);
        totalVirtualMem += kswap.ksw_total * pageSize;

        return totalVirtualMem;
#else
        return 0;
#endif
    }

    size_t get_total_memory_used() {
#ifdef TURBO_PLATFORM_WINDOWS
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        return static_cast<uint64_t>(memInfo.ullTotalPageFile - memInfo.ullAvailPageFile);
#elif defined(TURBO_PLATFORM_LINUX)
        struct sysinfo memInfo;
        sysinfo(&memInfo);
        auto virtualMemUsed = memInfo.totalram - memInfo.freeram;

        virtualMemUsed += memInfo.totalswap - memInfo.freeswap;
        virtualMemUsed *= memInfo.mem_unit;

        return static_cast<uint64_t>(virtualMemUsed);
#elif defined(TURBO_PLATFORM_FREEBSD)
        kvm_t *kd;
        u_int pageSize;
        u_int pageCnt, freeCnt;
        size_t pageCntLen = sizeof(pageCnt);
        size_t freeCntLen = sizeof(freeCnt);
        struct kvm_swap kswap;
        uint64_t virtualMemUsed;

        pageSize = static_cast<u_int>(getpagesize());

        sysctlbyname("vm.stats.vm.v_page_count", &pageCnt, &pageCntLen, NULL, 0);
        sysctlbyname("vm.stats.vm.v_free_count", &freeCnt, &freeCntLen, NULL, 0);
        virtualMemUsed = (pageCnt - freeCnt) * pageSize;

        kd = kvm_open(NULL, _PATH_DEVNULL, NULL, O_RDONLY, "kvm_open");
        kvm_getswapinfo(kd, &kswap, 1, 0);
        kvm_close(kd);
        virtualMemUsed += kswap.ksw_used * pageSize;

        return virtualMemUsed;
#else
        return 0;
#endif
    }

    uint64_t get_process_memory_used() {
#ifdef TURBO_PLATFORM_WINDOWS
        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PPROCESS_MEMORY_COUNTERS>(&pmc), sizeof(pmc));
        return static_cast<uint64_t>(pmc.PrivateUsage);
#elif defined(TURBO_PLATFORM_LINUX)
        auto parseLine =
                [](char *line) -> int {
                    auto i = strlen(line);

                    while (*line < '0' || *line > '9') {
                        line++;
                    }

                    line[i - 3] = '\0';
                    i = atoi(line);
                    return i;
                };

        auto file = fopen("/proc/self/status", "r");
        auto result = -1;
        char line[128];

        while (fgets(line, 128, file) != nullptr) {
            if (strncmp(line, "VmSize:", 7) == 0) {
                result = parseLine(line);
                break;
            }
        }

        fclose(file);
        return static_cast<uint64_t>(result) * 1024;
#elif defined(TURBO_PLATFORM_FREEBSD)
        struct kinfo_proc info;
        size_t infoLen = sizeof(info);
        int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };

        sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &infoLen, NULL, 0);
        return static_cast<uint64_t>(info.ki_rssize * getpagesize());
#else
        return 0;
#endif
    }

    size_t get_physical_memory() {
#ifdef TURBO_PLATFORM_WINDOWS
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        return static_cast<uint64_t>(memInfo.ullTotalPhys);
#elif defined(TURBO_PLATFORM_LINUX)
        struct sysinfo memInfo;
        sysinfo(&memInfo);

        auto totalPhysMem = memInfo.totalram;

        totalPhysMem *= memInfo.mem_unit;
        return static_cast<uint64_t>(totalPhysMem);
#elif defined(TURBO_PLATFORM_FREEBSD)
        u_long physMem;
        size_t physMemLen = sizeof(physMem);
        int mib[] = { CTL_HW, HW_PHYSMEM };

        sysctl(mib, sizeof(mib) / sizeof(*mib), &physMem, &physMemLen, NULL, 0);
        return physMem;
#else
        return 0;
#endif
    }
}
