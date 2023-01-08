

#ifndef FLARE_CONTAINER_FIND_CSTR_H_
#define FLARE_CONTAINER_FIND_CSTR_H_

#include <string>
#include <map>
#include <algorithm>
#include "flare/thread/thread.h"

// Find c-string in maps with std::string as keys without memory allocations.
// Example:
//   std::map<std::string, int> string_map;
//
//   string_map.find("hello");         // constructed a temporary std::string
//                                     // which needs memory allocation.
//
//   find_cstr(string_map, "hello");   // no allocation.
//
// You can specialize find_cstr for other maps.
// Possible prototypes are:
//   const_iterator find_cstr(const Map& map, const char* key);
//   iterator       find_cstr(Map& map, const char* key);
//   const_iterator find_cstr(const Map& map, const char* key, size_t length);
//   iterator       find_cstr(Map& map, const char* key, size_t length);

namespace flare::container {

struct StringMapThreadLocalTemp {
    bool initialized;
    char buf[sizeof(std::string)];

    static void delete_tls(void* buf) {
        StringMapThreadLocalTemp* temp = (StringMapThreadLocalTemp*)buf;
        if (temp->initialized) {
            temp->initialized = false;
            std::string* temp_string = (std::string*)temp->buf;
            temp_string->~basic_string();
        }
    }

    inline std::string* get_string(const char* key) {
        if (!initialized) {
            initialized = true;
            std::string* tmp = new (buf) std::string(key);
            flare::thread::atexit(delete_tls, this);
            return tmp;
        } else {
            std::string* tmp = (std::string*)buf;
            tmp->assign(key);
            return tmp;
        }
    }

    inline std::string* get_string(const char* key, size_t length) {
        if (!initialized) {
            initialized = true;
            std::string* tmp = new (buf) std::string(key, length);
            flare::thread::atexit(delete_tls, this);
            return tmp;
        } else {
            std::string* tmp = (std::string*)buf;
            tmp->assign(key, length);
            return tmp;
        }
    }

    inline std::string* get_lowered_string(const char* key) {
        std::string* tmp = get_string(key);
        std::transform(tmp->begin(), tmp->end(), tmp->begin(), ::tolower);
        return tmp;
    }

    inline std::string* get_lowered_string(const char* key, size_t length) {
        std::string* tmp = get_string(key, length);
        std::transform(tmp->begin(), tmp->end(), tmp->begin(), ::tolower);
        return tmp;
    }
};

extern thread_local StringMapThreadLocalTemp tls_stringmap_temp;

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::const_iterator
find_cstr(const std::map<std::string, T, C, A>& m, const char* key) {
    return m.find(*tls_stringmap_temp.get_string(key));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::iterator
find_cstr(std::map<std::string, T, C, A>& m, const char* key) {
    return m.find(*tls_stringmap_temp.get_string(key));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::const_iterator
find_cstr(const std::map<std::string, T, C, A>& m,
          const char* key, size_t length) {
    return m.find(*tls_stringmap_temp.get_string(key, length));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::iterator
find_cstr(std::map<std::string, T, C, A>& m,
          const char* key, size_t length) {
    return m.find(*tls_stringmap_temp.get_string(key, length));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::const_iterator
find_lowered_cstr(const std::map<std::string, T, C, A>& m, const char* key) {
    return m.find(*tls_stringmap_temp.get_lowered_string(key));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::iterator
find_lowered_cstr(std::map<std::string, T, C, A>& m, const char* key) {
    return m.find(*tls_stringmap_temp.get_lowered_string(key));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::const_iterator
find_lowered_cstr(const std::map<std::string, T, C, A>& m,
                  const char* key, size_t length) {
    return m.find(*tls_stringmap_temp.get_lowered_string(key, length));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::iterator
find_lowered_cstr(std::map<std::string, T, C, A>& m,
                  const char* key, size_t length) {
    return m.find(*tls_stringmap_temp.get_lowered_string(key, length));
}

}  // namespace flare::container

#endif  // FLARE_CONTAINER_FIND_CSTR_H_
