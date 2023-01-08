
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef FLARE_BASE_INTERNAL_CONFIG_H_
#define FLARE_BASE_INTERNAL_CONFIG_H_

#if defined(_MSC_VER)
#if defined(FLARE_BUILD_DLL)
#define FLARE_EXPORT __declspec(dllexport)
#elif defined(FLARE_CONSUME_DLL)
#define FLARE_EXPORT __declspec(dllimport)
#else
#define FLARE_EXPORT
#endif
#else
#define FLARE_EXPORT
#endif  // defined(_MSC_VER)

#endif  // FLARE_BASE_INTERNAL_CONFIG_H_
