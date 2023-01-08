
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

// Wrappers around lsan_interface functions.
// When lsan is not linked in, these functions are not available,
// therefore flare code which depends on these functions is conditioned on the
// definition of LEAK_SANITIZER.
#include "flare/debugging/leak_check.h"

#ifndef LEAK_SANITIZER

namespace flare::debugging {

bool have_leak_sanitizer() { return false; }

void do_ignore_leak(const void *) {}

void register_live_pointers(const void *, size_t) {}

void unregister_live_pointers(const void *, size_t) {}

leak_check_disabler::leak_check_disabler() {}

leak_check_disabler::~leak_check_disabler() {}

}  // namespace flare::debugging

#else

#include <sanitizer/lsan_interface.h>

namespace flare::debugging {

bool have_leak_sanitizer() { return true; }
void do_ignore_leak(const void* ptr) { __lsan_ignore_object(ptr); }
void register_live_pointers(const void* ptr, size_t size) {
  __lsan_register_root_region(ptr, size);
}
void unregister_live_pointers(const void* ptr, size_t size) {
  __lsan_unregister_root_region(ptr, size);
}
leak_check_disabler::leak_check_disabler() { __lsan_disable(); }
leak_check_disabler::~leak_check_disabler() { __lsan_enable(); }

}  // namespace flare::debugging

#endif  // LEAK_SANITIZER
