//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//
// Created by jeff on 24-6-28.
//

#pragma once

#include <string_view>
#include <turbo/flags/marshalling.h>
#include <turbo/container/flat_hash_set.h>
#include <turbo/strings/match.h>

namespace turbo {

    template<typename T>
    struct AllPassValidator {
        static bool validate(std::string_view value, std::string *error) noexcept {
            (void) value;
            (void) error;
            return true;
        }
    };

    template<typename T>
    struct EqValidatorComparator {
        static bool validate(T lhs, T rhs) noexcept {
            return lhs == rhs;
        }
    };

    template<typename T>
    struct GtValidatorComparator {
        static bool validate(T lhs, T rhs) noexcept {
            return lhs > rhs;
        }
    };

    template<typename T>
    struct GEValidatorComparator {
        static bool validate(T lhs, T rhs) noexcept {
            return lhs >= rhs;
        }
    };

    template<typename T>
    struct LeValidatorComparator {
        static bool validate(T lhs, T rhs) noexcept {
            return lhs <= rhs;
        }
    };

    template<typename T>
    struct LtValidatorComparator {
        static bool validate(T lhs, T rhs) noexcept {
            return lhs < rhs;
        }
    };

    template<typename T>
    struct InSetComparator {
        static bool validate(T lhs, const flat_hash_set<T> &c) noexcept {
            return c.find(lhs) != c.end();
        }
    };

    template<typename T>
    struct OutSetComparator {
        static bool validate(T lhs, const flat_hash_set<T> &c) noexcept {
            return c.find(lhs) == c.end();
        }
    };

    template<typename T, T Min, typename CM, std::enable_if_t<
            std::is_integral_v<T> || std::is_floating_point_v<T>, int> = 0>
    struct UnaryValidator {
        static bool validate(std::string_view value, std::string *error) noexcept {
            T tmp;
            if (!turbo::parse_flag(value, &tmp, error)) {
                return false;
            }
            if (!CM::validate(tmp, Min)) {
                if (error) {
                    *error = "value must be greater than or equal to " + std::to_string(Min);
                }
                return false;
            }
            return true;
        }
    };

    template<typename T, T Min>
    using GeValidator = UnaryValidator<T, Min, GEValidatorComparator<T>>;

    template<typename T, T Min>
    using GtValidator = UnaryValidator<T, Min, GtValidatorComparator<T>>;

    template<typename T, T Max>
    using LeValidator = UnaryValidator<T, Max, LeValidatorComparator<T>>;

    template<typename T, T Max>
    using LtValidator = UnaryValidator<T, Max, LtValidatorComparator<T>>;

    template<typename T, T Min, T Max, typename LCM, typename RCM,
            std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, int> = 0>
    struct BinaryValidator {
        static bool validate(std::string_view value, std::string *error) noexcept {
            T tmp;
            if (!turbo::parse_flag(value, &tmp, error)) {
                return false;
            }
            if (!LCM::validate(tmp, Min) || !RCM::validate(tmp, Max)) {
                if (error) {
                    *error = "value must be in the range [" + std::to_string(Min) + ", " + std::to_string(Max) + "]";
                }
                return false;
            }
            return true;
        }
    };

    template<typename T, T Min, T Max>
    using ClosedClosedInRangeValidator = BinaryValidator<T, Min, Max, GEValidatorComparator<T>, LeValidatorComparator<T>>;

    template<typename T, T Min, T Max>
    using ClosedOpenInRangeValidator = BinaryValidator<T, Min, Max, GEValidatorComparator<T>, LtValidatorComparator<T>>;

    template<typename T, T Min, T Max>
    using OpenClosedInRangeValidator = BinaryValidator<T, Min, Max, GtValidatorComparator<T>, LeValidatorComparator<T>>;

    template<typename T, T Min, T Max>
    using OpenOpenInRangeValidator = BinaryValidator<T, Min, Max, GtValidatorComparator<T>, LtValidatorComparator<T>>;

    template<typename T, T Min, T Max>
    using ClosedClosedOutRangeValidator = BinaryValidator<T, Min, Max, LtValidatorComparator<T>, GtValidatorComparator<T>>;

    template<typename T, T Min, T Max>
    using ClosedOpenOutRangeValidator = BinaryValidator<T, Min, Max, LtValidatorComparator<T>, GEValidatorComparator<T>>;

    template<typename T, T Min, T Max>
    using OpenClosedOutRangeValidator = BinaryValidator<T, Min, Max, LeValidatorComparator<T>, GtValidatorComparator<T>>;

    template<typename T, T Min, T Max>
    using OpenOpenOutRangeValidator = BinaryValidator<T, Min, Max, LeValidatorComparator<T>, GEValidatorComparator<T>>;

    template<typename T, const turbo::flat_hash_set<T> &C, typename CM, std::enable_if_t<
            (std::is_integral_v<T> || std::is_floating_point_v<T>), int> = 0>
    struct SetValidator {
        static bool validate(std::string_view value, std::string *error) noexcept {
            T tmp;
            if (!turbo::parse_flag(value, &tmp, error)) {
                return false;
            }
            if (!CM::validate(tmp, C)) {
                if (error) {
                    *error = "value must be in the set";
                }
                return false;
            }
            return true;
        }
    };

    template<typename T, const turbo::flat_hash_set<T> &C>
    using InSetValidator = SetValidator<T, C, InSetComparator<T>>;

    template<typename T, const turbo::flat_hash_set<T> &C>
    using OutSetValidator = SetValidator<T, C, OutSetComparator<T>>;

    template <const std::string_view &prefix>
    struct StartsWithValidator {
        static bool validate(std::string_view value, std::string *error) noexcept {
            if (!turbo::starts_with(value, prefix)) {
                if (error) {
                    *error = "value must start with " + std::string(prefix);
                }
                return false;
            }
            return true;
        }
    };

    template <const std::string_view &prefix>
    struct StartsWithIgnoreCaseValidator {
        static bool validate(std::string_view value, std::string *error) noexcept {
            if (!turbo::starts_with_ignore_case(value, prefix)) {
                if (error) {
                    *error = "value must start with " + std::string(prefix);
                }
                return false;
            }
            return true;
        }
    };

    template <const std::string_view &suffix>
    struct EndsWithValidator {
        static bool validate(std::string_view value, std::string *error) noexcept {
            if (!turbo::ends_with(value, suffix)) {
                if (error) {
                    *error = "value must ends with " + std::string(suffix);
                }
                return false;
            }
            return true;
        }
    };

    template <const std::string_view &suffix>
    struct EndsWithIgnoreCaseValidator {
        static bool validate(std::string_view value, std::string *error) noexcept {
            if (!turbo::ends_with_ignore_case(value, suffix)) {
                if (error) {
                    *error = "value must ends with " + std::string(suffix);
                }
                return false;
            }
            return true;
        }
    };

    template <const std::string_view &frag>
    struct ContainsValidator {
        static bool validate(std::string_view value, std::string *error) noexcept {
            if (!turbo::str_contains(value, frag)) {
                if (error) {
                    *error = "value must contains " + std::string(frag);
                }
                return false;
            }
            return true;
        }
    };

    template <const std::string_view &frag>
    struct ContainsIgnoreCaseValidator {
        static bool validate(std::string_view value, std::string *error) noexcept {
            if (!turbo::str_contains_ignore_case(value, frag)) {
                if (error) {
                    *error = "value must contains " + std::string(frag);
                }
                return false;
            }
            return true;
        }
    };
}  // namespace turbo
