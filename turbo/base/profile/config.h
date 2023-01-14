
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef TURBO_BASE_INTERNAL_CONFIG_H_
#define TURBO_BASE_INTERNAL_CONFIG_H_

#if defined(_MSC_VER)
#if defined(TURBO_BUILD_DLL)
#define TURBO_EXPORT __declspec(dllexport)
#elif defined(TURBO_CONSUME_DLL)
#define TURBO_EXPORT __declspec(dllimport)
#else
#define TURBO_EXPORT
#endif
#else
#define TURBO_EXPORT __attribute__((visibility("default")))
#endif  // defined(_MSC_VER)

#endif  // TURBO_BASE_INTERNAL_CONFIG_H_
