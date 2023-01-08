
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/times/internal/zone_info_source.h"
#include "flare/base/profile.h"

namespace flare::times_internal {

    // Defined out-of-line to avoid emitting a weak vtable in all TUs.
    zone_info_source::~zone_info_source() {}

    std::string zone_info_source::version() const { return std::string(); }


}  // namespace flare::times_internal

namespace flare::times_internal {
    namespace {

        // A default for zone_info_source_factory, which simply
        // defers to the fallback factory.
        std::unique_ptr<flare::times_internal::zone_info_source> default_factory(
                const std::string &name,
                const std::function<
                        std::unique_ptr<flare::times_internal::zone_info_source>(
                                const std::string &name)> &fallback_factory) {
            return fallback_factory(name);
        }

    }  // namespace
// A "weak" definition for zone_info_source_factory.
// The user may override this with their own "strong" definition (see
// zone_info_source.h).
#if !defined(__has_attribute)
#define __has_attribute(x) 0
#endif
// MinGW is GCC on Windows, so while it asserts __has_attribute(weak), the
// Windows linker cannot handle that. Nor does the MinGW compiler know how to
// pass "#pragma comment(linker, ...)" to the Windows linker.
#if (__has_attribute(weak) || defined(__GNUC__)) && !defined(__MINGW32__)
    ZoneInfoSourceFactory zone_info_source_factory __attribute__((weak)) = default_factory;
#elif defined(_MSC_VER) && !defined(__MINGW32__) && !defined(_LIBCPP_VERSION)
    extern ZoneInfoSourceFactory zone_info_source_factory;
    extern ZoneInfoSourceFactory default_factory;
    ZoneInfoSourceFactory default_factory = default_factory;
#if defined(_M_IX86)
#pragma comment( \
    linker,      \
    "/alternatename:?zone_info_source_factory@times_internal@flare@@3P6A?AV?$unique_ptr@VZoneInfoSource@times_internal@flare@@U?$default_delete@VZoneInfoSource@times_internal@flare@@@std@@@std@@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@5@ABV?$function@$$A6A?AV?$unique_ptr@VZoneInfoSource@times_internal@flare@@U?$default_delete@VZoneInfoSource@times_internal@flare@@@std@@@std@@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z@5@@ZA=?default_factory@times_internal@flare@@3P6A?AV?$unique_ptr@VZoneInfoSource@times_internal@flare@@U?$default_delete@VZoneInfoSource@times_internal@flare@@@std@@@std@@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@5@ABV?$function@$$A6A?AV?$unique_ptr@VZoneInfoSource@times_internal@flare@@U?$default_delete@VZoneInfoSource@times_internal@flare@@@std@@@std@@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z@5@@ZA")
#elif defined(_M_IA_64) || defined(_M_AMD64) || defined(_M_ARM64)
#pragma comment( \
    linker,      \
    "/alternatename:?zone_info_source_factory@times_internal@flare@@3P6A?AV?$unique_ptr@VZoneInfoSource@times_internal@flare@@U?$default_delete@VZoneInfoSource@times_internal@flare@@@std@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@5@AEBV?$function@$$A6A?AV?$unique_ptr@VZoneInfoSource@times_internal@flare@@U?$default_delete@VZoneInfoSource@times_internal@flare@@@std@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z@5@@ZEA=?default_factory@times_internal@flare@@3P6A?AV?$unique_ptr@VZoneInfoSource@times_internal@flare@@U?$default_delete@VZoneInfoSource@times_internal@flare@@@std@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@5@AEBV?$function@$$A6A?AV?$unique_ptr@VZoneInfoSource@times_internal@flare@@U?$default_delete@VZoneInfoSource@times_internal@flare@@@std@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z@5@@ZEA")
#else
#error Unsupported MSVC platform
#endif  // _M_<PLATFORM>
#else
    // Make it a "strong" definition if we have no other choice.
    ZoneInfoSourceFactory zone_info_source_factory = default_factory;
#endif

}  // namespace flare::times_internal
