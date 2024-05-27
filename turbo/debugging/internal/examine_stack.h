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

#ifndef TURBO_DEBUGGING_INTERNAL_EXAMINE_STACK_H_
#define TURBO_DEBUGGING_INTERNAL_EXAMINE_STACK_H_

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace debugging_internal {

// Type of function used for printing in stack trace dumping, etc.
// We avoid closures to keep things simple.
typedef void OutputWriter(const char*, void*);

// RegisterDebugStackTraceHook() allows to register a single routine
// `hook` that is called each time DumpStackTrace() is called.
// `hook` may be called from a signal handler.
typedef void (*SymbolizeUrlEmitter)(void* const stack[], int depth,
                                    OutputWriter* writer, void* writer_arg);

// Registration of SymbolizeUrlEmitter for use inside of a signal handler.
// This is inherently unsafe and must be signal safe code.
void RegisterDebugStackTraceHook(SymbolizeUrlEmitter hook);
SymbolizeUrlEmitter GetDebugStackTraceHook();

// Returns the program counter from signal context, or nullptr if
// unknown. `vuc` is a ucontext_t*. We use void* to avoid the use of
// ucontext_t on non-POSIX systems.
void* GetProgramCounter(void* const vuc);

// Uses `writer` to dump the program counter, stack trace, and stack
// frame sizes.
void DumpPCAndFrameSizesAndStackTrace(void* const pc, void* const stack[],
                                      int frame_sizes[], int depth,
                                      int min_dropped_frames,
                                      bool symbolize_stacktrace,
                                      OutputWriter* writer, void* writer_arg);

// Dump current stack trace omitting the topmost `min_dropped_frames` stack
// frames.
void DumpStackTrace(int min_dropped_frames, int max_num_frames,
                    bool symbolize_stacktrace, OutputWriter* writer,
                    void* writer_arg);

}  // namespace debugging_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_DEBUGGING_INTERNAL_EXAMINE_STACK_H_
