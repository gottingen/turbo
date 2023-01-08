
#ifndef FLARE_CONTAINER_CASE_IGNORED_FLAT_MAP_H_
#define FLARE_CONTAINER_CASE_IGNORED_FLAT_MAP_H_

#include "flare/container/flat_map.h"
#include "flare/strings/ascii.h"

namespace flare::container {

    struct CaseIgnoredHasher {
        size_t operator()(const std::string &s) const {
            std::size_t result = 0;
            for (std::string::const_iterator i = s.begin(); i != s.end(); ++i) {
                result = result * 101 + flare::ascii::to_lower(*i);
            }
            return result;
        }

        size_t operator()(const char *s) const {
            std::size_t result = 0;
            for (; *s; ++s) {
                result = result * 101 + flare::ascii::to_lower(*s);
            }
            return result;
        }
    };

    struct CaseIgnoredEqual {
        bool operator()(const std::string &s1, const std::string &s2) const {
            return s1.size() == s2.size() &&
                   strcasecmp(s1.c_str(), s2.c_str()) == 0;
        }

        bool operator()(const std::string &s1, const char *s2) const { return strcasecmp(s1.c_str(), s2) == 0; }
    };

    template<typename T>
    class CaseIgnoredFlatMap : public flare::container::FlatMap<
            std::string, T, CaseIgnoredHasher, CaseIgnoredEqual> {
    };

    class CaseIgnoredFlatSet : public flare::container::FlatSet<
            std::string, CaseIgnoredHasher, CaseIgnoredEqual> {
    };

} // namespace flare::container

#endif  // FLARE_CONTAINER_CASE_IGNORED_FLAT_MAP_H_
