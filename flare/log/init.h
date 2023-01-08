
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_LOG_INIT_H_
#define FLARE_LOG_INIT_H_


class flare_initializer {
public:
    typedef void (*void_function)(void);
    flare_initializer(const char*, void_function f) {
        f();
    }
};

#define REGISTER_MODULE_INITIALIZER(name, body)                 \
  namespace {                                                   \
    static void flare_init_module_##name () { body; }          \
    flare_initializer flare_initializer_module_##name(#name,   \
            flare_init_module_##name);                         \
  }


#endif  // FLARE_LOG_INIT_H_
