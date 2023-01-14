
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_LOG_INIT_H_
#define TURBO_LOG_INIT_H_


class turbo_initializer {
public:
    typedef void (*void_function)(void);
    turbo_initializer(const char*, void_function f) {
        f();
    }
};

#define REGISTER_MODULE_INITIALIZER(name, body)                 \
  namespace {                                                   \
    static void turbo_init_module_##name () { body; }          \
    turbo_initializer turbo_initializer_module_##name(#name,   \
            turbo_init_module_##name);                         \
  }


#endif  // TURBO_LOG_INIT_H_
