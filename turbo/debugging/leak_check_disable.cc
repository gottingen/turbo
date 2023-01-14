
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

// Disable LeakSanitizer when this file is linked in.
// This function overrides __lsan_is_turned_off from sanitizer/lsan_interface.h
extern "C" int __lsan_is_turned_off();
extern "C" int __lsan_is_turned_off() {
    return 1;
}
