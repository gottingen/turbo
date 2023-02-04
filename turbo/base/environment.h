//
// Created by 李寅斌 on 2023/2/3.
//

#ifndef TURBO_BASE_ENVIRONMENT_H_
#define TURBO_BASE_ENVIRONMENT_H_

#include "turbo/platform/port.h"
#include <cstdlib>
#include <string>

namespace turbo {



// Function: get_env
inline std::string get_env(const std::string& str) {
#ifdef _MSC_VER
  char *ptr = nullptr;
  size_t len = 0;

  if(_dupenv_s(&ptr, &len, str.c_str()) == 0 && ptr != nullptr) {
    std::string res(ptr, len);
    std::free(ptr);
    return res;
  }
  return "";

#else
  auto ptr = std::getenv(str.c_str());
  return ptr ? ptr : "";
#endif
}

// Function: has_env
inline bool has_env(const std::string& str) {
#ifdef _MSC_VER
  char *ptr = nullptr;
  size_t len = 0;

  if(_dupenv_s(&ptr, &len, str.c_str()) == 0 && ptr != nullptr) {
    std::string res(ptr, len);
    std::free(ptr);
    return true;
  }
  return false;

#else
  auto ptr = std::getenv(str.c_str());
  return ptr ? true : false;
#endif
}

}  // namespace turbo

#endif  // TURBO_BASE_ENVIRONMENT_H_
