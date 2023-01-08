
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

#include "flare/debugging/symbolize.h"

#if defined(FLARE_INTERNAL_HAVE_ELF_SYMBOLIZE)
#include "flare/debugging/symbolize_elf.h"
#elif defined(_WIN32)
// The Windows Symbolizer only works if PDB files containing the debug info
// are available to the program at runtime.
#include "flare/debugging/symbolize_win32.inc"
#else

#include "flare/debugging/symbolize_unimplemented.inc"

#endif
