
#include <cstdlib>
#include <gflags/gflags.h>
#include "flare/metrics/gflag.h"

namespace flare {

    metrics_gflag::metrics_gflag(const std::string_view &gflag_name) {
        expose(gflag_name, "");
    }

    metrics_gflag::metrics_gflag(const std::string_view &prefix,
                                 const std::string_view &gflag_name)
            : _gflag_name(gflag_name.data(), gflag_name.size()) {
        expose_as(prefix, gflag_name, "");
    }

    void metrics_gflag::describe(std::ostream &os, bool quote_string) const {
        google::CommandLineFlagInfo info;
        if (!google::GetCommandLineFlagInfo(gflag_name().c_str(), &info)) {
            if (quote_string) {
                os << '"';
            }
            os << "Unknown gflag=" << gflag_name();
            if (quote_string) {
                os << '"';
            }
        } else {
            if (quote_string && info.type == "string") {
                os << '"' << info.current_value << '"';
            } else {
                os << info.current_value;
            }
        }
    }

    std::string metrics_gflag::get_value() const {
        std::string str;
        if (!google::GetCommandLineOption(gflag_name().c_str(), &str)) {
            return "Unknown gflag=" + gflag_name();
        }
        return str;
    }

    bool metrics_gflag::set_value(const char *value) {
        return !google::SetCommandLineOption(gflag_name().c_str(), value).empty();
    }

}  // namespace flare
