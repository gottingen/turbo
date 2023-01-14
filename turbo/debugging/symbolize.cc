
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

#include "turbo/debugging/symbolize.h"

#if defined(TURBO_INTERNAL_HAVE_ELF_SYMBOLIZE)
#include "turbo/debugging/symbolize_elf.h"
#elif defined(_WIN32)
// The Windows Symbolizer only works if PDB files containing the debug info
// are available to the program at runtime.
#include "turbo/debugging/symbolize_win32.inc"
#else

#include "turbo/debugging/symbolize_unimplemented.inc"

#endif
