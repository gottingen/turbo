// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef TURBO_META_OPTIONAL_REF_H_
#define TURBO_META_OPTIONAL_REF_H_

#include <cmath>
#include <ostream>
#include <type_traits>
#include <utility>
#include "turbo/meta/internal/optional_meta.h"
#include "turbo/meta/closure.h"
#include "turbo/meta/type_traits.h"
#include "turbo/meta/internal/optional_internal.h"

namespace turbo {
    template<class T, class B>
    auto optional(T &&t, B &&b) noexcept;

    /************************
     * optional declaration *
     ************************/

    /**
     * @class optional_ref
     * @brief Optional value handler.
     *
     * The optional_ref is an optional proxy. It holds a value (or a reference on a value) and a flag (or reference on a flag)
     * indicating whether the element should be considered missing.
     *
     * optional_ref is different from std::optional
     *
     *  - no `operator->()` that returns a pointer.
     *  - no `operator*()` that returns a value.
     *
     * The only way to access the underlying value and flag is with the `value` and `value_or` methods.
     *
     *  - no explicit convertion to bool. This may lead to confusion when the underlying value type is boolean too.
     *
     * @tparam CT Closure type for the value.
     * @tparam CB Closure type for the missing flag. A falsy flag means that the value is missing.
     *
     * \ref optional_ref is used both as a value type (with CT and CB being value types) and reference type for containers
     * with CT and CB being reference types. In other words, it serves as a reference proxy.
     *
     */
    template<class CT, class CB>
    class optional_ref {
    public:

        using self_type = optional_ref<CT, CB>;
        using value_closure = CT;
        using flag_closure = CB;

        using value_type = std::decay_t<CT>;
        using flag_type = std::decay_t<CB>;

        // Constructors
        inline optional_ref()
                : m_value(), m_flag(false) {
        }

        template<class T,
                std::enable_if_t<
                        conjunction<
                                negation<std::is_same<optional_ref<CT, CB>, std::decay_t<T>>>,
                                std::is_constructible<CT, T &&>,
                                std::is_convertible<T &&, CT>
                        >::value,
                        bool
                > = true>
        inline constexpr optional_ref(T &&rhs)
                : m_value(std::forward<T>(rhs)), m_flag(true) {
        }

        template<class T,
                std::enable_if_t<
                        conjunction<
                                negation<std::is_same<optional_ref<CT, CB>, std::decay_t<T>>>,
                                std::is_constructible<CT, T &&>,
                                negation<std::is_convertible<T &&, CT>>
                        >::value,
                        bool
                > = false>
        inline explicit constexpr optional_ref(T &&value)
                : m_value(std::forward<T>(value)), m_flag(true) {
        }

        template<class CTO, class CBO,
                std::enable_if_t<
                        conjunction<
                                negation<std::is_same<optional_ref<CT, CB>, optional_ref<CTO, CBO>>>,
                                std::is_constructible<CT, std::add_lvalue_reference_t<std::add_const_t<CTO>>>,
                                std::is_constructible<CB, std::add_lvalue_reference_t<std::add_const_t<CBO>>>,
                                conjunction<
                                        std::is_convertible<std::add_lvalue_reference_t<std::add_const_t<CTO>>, CT>,
                                        std::is_convertible<std::add_lvalue_reference_t<std::add_const_t<CBO>>, CB>
                                >,
                                negation<detail::converts_from_optional_ref<CT, CTO, CBO>>
                        >::value,
                        bool
                > = true>
        inline constexpr optional_ref(const optional_ref<CTO, CBO> &rhs)
                : m_value(rhs.value()), m_flag(rhs.has_value()) {
        }

        template<class CTO, class CBO,
                std::enable_if_t<
                        conjunction<
                                negation<std::is_same<optional_ref<CT, CB>, optional_ref<CTO, CBO>>>,
                                std::is_constructible<CT, std::add_lvalue_reference_t<std::add_const_t<CTO>>>,
                                std::is_constructible<CB, std::add_lvalue_reference_t<std::add_const_t<CBO>>>,
                                disjunction<
                                        negation<std::is_convertible<std::add_lvalue_reference_t<std::add_const_t<CTO>>, CT>>,
                                        negation<std::is_convertible<std::add_lvalue_reference_t<std::add_const_t<CBO>>, CB>>
                                >,
                                negation<detail::converts_from_optional_ref<CT, CTO, CBO>>
                        >::value,
                        bool
                > = false>
        inline explicit constexpr optional_ref(const optional_ref<CTO, CBO> &rhs)
                : m_value(rhs.value()), m_flag(rhs.has_value()) {
        }

        template<class CTO, class CBO,
                std::enable_if_t<
                        conjunction<
                                negation<std::is_same<optional_ref<CT, CB>, optional_ref<CTO, CBO>>>,
                                std::is_constructible<CT, std::conditional_t<std::is_reference<CT>::value, const std::decay_t<CTO> &, std::decay_t<CTO> &&>>,
                                std::is_constructible<CB, std::conditional_t<std::is_reference<CB>::value, const std::decay_t<CBO> &, std::decay_t<CBO> &&>>,
                                conjunction<
                                        std::is_convertible<std::conditional_t<std::is_reference<CT>::value, const std::decay_t<CTO> &, std::decay_t<CTO> &&>, CT>,
                                        std::is_convertible<std::conditional_t<std::is_reference<CB>::value, const std::decay_t<CBO> &, std::decay_t<CBO> &&>, CB>
                                >,
                                negation<detail::converts_from_optional_ref<CT, CTO, CBO>>
                        >::value,
                        bool
                > = true>
        inline constexpr optional_ref(optional_ref<CTO, CBO> &&rhs)
                : m_value(std::move(rhs).value()), m_flag(std::move(rhs).has_value()) {
        }

        template<class CTO, class CBO,
                std::enable_if_t<
                        conjunction<
                                negation<std::is_same<optional_ref<CT, CB>, optional_ref<CTO, CBO>>>,
                                std::is_constructible<CT, std::conditional_t<std::is_reference<CT>::value, const std::decay_t<CTO> &, std::decay_t<CTO> &&>>,
                                std::is_constructible<CB, std::conditional_t<std::is_reference<CB>::value, const std::decay_t<CBO> &, std::decay_t<CBO> &&>>,
                                disjunction<
                                        negation<std::is_convertible<std::conditional_t<std::is_reference<CT>::value, const std::decay_t<CTO> &, std::decay_t<CTO> &&>, CT>>,
                                        negation<std::is_convertible<std::conditional_t<std::is_reference<CB>::value, const std::decay_t<CBO> &, std::decay_t<CBO> &&>, CB>>
                                >,
                                negation<detail::converts_from_optional_ref<CT, CTO, CBO>>
                        >::value,
                        bool
                > = false>
        inline explicit constexpr optional_ref(optional_ref<CTO, CBO> &&rhs)
                : m_value(std::move(rhs).value()), m_flag(std::move(rhs).has_value()) {
        }

        optional_ref(value_type &&, flag_type &&);

        optional_ref(std::add_lvalue_reference_t<CT>, std::add_lvalue_reference_t<CB>);

        optional_ref(value_type &&, std::add_lvalue_reference_t<CB>);

        optional_ref(std::add_lvalue_reference_t<CT>, flag_type &&);

        // Assignment
        template<class T>
        std::enable_if_t<
                conjunction<
                        negation<std::is_same<optional_ref<CT, CB>, std::decay_t<T>>>,
                        std::is_assignable<std::add_lvalue_reference_t<CT>, T>
                >::value,
                optional_ref &>
        inline operator=(T &&rhs) {
            m_value = std::forward<T>(rhs);
            m_flag = true;
            return *this;
        }

        template<class CTO, class CBO>
        std::enable_if_t<conjunction<
                negation<std::is_same<optional_ref<CT, CB>, optional_ref<CTO, CBO>>>,
                std::is_assignable<std::add_lvalue_reference_t<CT>, CTO>,
                negation<detail::converts_from_optional_ref<CT, CTO, CBO>>,
                negation<detail::assigns_from_xoptional<CT, CTO, CBO>>
        >::value,
                optional_ref &>
        inline operator=(const optional_ref<CTO, CBO> &rhs) {
            m_value = rhs.value();
            m_flag = rhs.has_value();
            return *this;
        }

        template<class CTO, class CBO>
        std::enable_if_t<conjunction<
                negation<std::is_same<optional_ref<CT, CB>, optional_ref<CTO, CBO>>>,
                std::is_assignable<std::add_lvalue_reference_t<CT>, CTO>,
                negation<detail::converts_from_optional_ref<CT, CTO, CBO>>,
                negation<detail::assigns_from_xoptional<CT, CTO, CBO>>
        >::value,
                optional_ref &>
        inline operator=(optional_ref<CTO, CBO> &&rhs) {
            m_value = std::move(rhs).value();
            m_flag = std::move(rhs).has_value();
            return *this;
        }

        // Operators
        template<class CTO, class CBO>
        optional_ref &operator+=(const optional_ref<CTO, CBO> &);

        template<class CTO, class CBO>
        optional_ref &operator-=(const optional_ref<CTO, CBO> &);

        template<class CTO, class CBO>
        optional_ref &operator*=(const optional_ref<CTO, CBO> &);

        template<class CTO, class CBO>
        optional_ref &operator/=(const optional_ref<CTO, CBO> &);

        template<class CTO, class CBO>
        optional_ref &operator%=(const optional_ref<CTO, CBO> &);

        template<class CTO, class CBO>
        optional_ref &operator&=(const optional_ref<CTO, CBO> &);

        template<class CTO, class CBO>
        optional_ref &operator|=(const optional_ref<CTO, CBO> &);

        template<class CTO, class CBO>
        optional_ref &operator^=(const optional_ref<CTO, CBO> &);

        template<class T, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T>)>
        optional_ref &operator+=(const T &);

        template<class T, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T>)>
        optional_ref &operator-=(const T &);

        template<class T, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T>)>
        optional_ref &operator*=(const T &);

        template<class T, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T>)>
        optional_ref &operator/=(const T &);

        template<class T, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T>)>
        optional_ref &operator%=(const T &);

        template<class T, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T>)>
        optional_ref &operator&=(const T &);

        template<class T, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T>)>
        optional_ref &operator|=(const T &);

        template<class T, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T>)>
        optional_ref &operator^=(const T &);

        // Access
        std::add_lvalue_reference_t<CT> value() & noexcept;

        std::add_lvalue_reference_t<std::add_const_t<CT>> value() const & noexcept;

        std::conditional_t<std::is_reference<CT>::value, apply_cv_t<CT, value_type> &, value_type> value() && noexcept;

        std::conditional_t<std::is_reference<CT>::value, const value_type &, value_type> value() const && noexcept;

        template<class U>
        value_type value_or(U &&) const & noexcept;

        template<class U>
        value_type value_or(U &&) const && noexcept;

        // Access
        std::add_lvalue_reference_t<CB> has_value() & noexcept;

        std::add_lvalue_reference_t<std::add_const_t<CB>> has_value() const & noexcept;

        std::conditional_t<std::is_reference<CB>::value, apply_cv_t<CB, flag_type> &, flag_type>
        has_value() && noexcept;

        std::conditional_t<std::is_reference<CB>::value, const flag_type &, flag_type> has_value() const && noexcept;

        // Swap
        void swap(optional_ref &other);

        // Comparison
        template<class CTO, class CBO>
        bool equal(const optional_ref<CTO, CBO> &rhs) const noexcept;

        template<class CTO, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<CTO>)>
        bool equal(const CTO &rhs) const noexcept;

        closure_pointer<self_type &> operator&() &;

        closure_pointer<const self_type &> operator&() const &;

        closure_pointer<self_type> operator&() &&;

    private:

        template<class CTO, class CBO>
        friend
        class optional_ref;

        CT m_value;
        CB m_flag;
    };

    // value

    template<class T, class U = disable_xoptional<std::decay_t<T>>>
    T &&value(T &&v) {
        return std::forward<T>(v);
    }

    template<class CT, class CB>
    decltype(auto) value(optional_ref<CT, CB> &&v) {
        return std::move(v).value();
    }

    template<class CT, class CB>
    decltype(auto) value(optional_ref<CT, CB> &v) {
        return v.value();
    }

    template<class CT, class CB>
    decltype(auto) value(const optional_ref<CT, CB> &v) {
        return v.value();
    }

    // has_value

    template<class T, class U = disable_xoptional<std::decay_t<T>>>
    bool has_value(T &&) {
        return true;
    }

    template<class CT, class CB>
    decltype(auto) has_value(optional_ref<CT, CB> &&v) {
        return std::move(v).has_value();
    }

    template<class CT, class CB>
    decltype(auto) has_value(optional_ref<CT, CB> &v) {
        return v.has_value();
    }

    template<class CT, class CB>
    decltype(auto) has_value(const optional_ref<CT, CB> &v) {
        return v.has_value();
    }

    /***************************************
     * optional and missing implementation *
     ***************************************/

    /**
     * @brief Returns an \ref optional_ref holding closure types on the specified parameters
     *
     * @tparam t the optional value
     * @tparam b the boolean flag
     */
    template<class T, class B>
    inline auto optional(T &&t, B &&b) noexcept {
        using optional_type = optional_ref<closure_type_t<T>, closure_type_t<B>>;
        return optional_type(std::forward<T>(t), std::forward<B>(b));
    }

    /**
     * @brief Returns an \ref optional_ref for a missig value
     */
    template<class T>
    optional_ref<T, bool> missing() noexcept {
        return optional_ref<T, bool>(T(), false);
    }

    /****************************
     * optional_ref implementation *
     ****************************/

    // Constructors
    template<class CT, class CB>
    optional_ref<CT, CB>::optional_ref(value_type &&value, flag_type &&flag)
            : m_value(std::move(value)), m_flag(std::move(flag)) {
    }

    template<class CT, class CB>
    optional_ref<CT, CB>::optional_ref(std::add_lvalue_reference_t<CT> value, std::add_lvalue_reference_t<CB> flag)
            : m_value(value), m_flag(flag) {
    }

    template<class CT, class CB>
    optional_ref<CT, CB>::optional_ref(value_type &&value, std::add_lvalue_reference_t<CB> flag)
            : m_value(std::move(value)), m_flag(flag) {
    }

    template<class CT, class CB>
    optional_ref<CT, CB>::optional_ref(std::add_lvalue_reference_t<CT> value, flag_type &&flag)
            : m_value(value), m_flag(std::move(flag)) {
    }

    // Operators
    template<class CT, class CB>
    template<class CTO, class CBO>
    auto optional_ref<CT, CB>::operator+=(const optional_ref<CTO, CBO> &rhs) -> optional_ref & {
        m_flag = m_flag && rhs.m_flag;
        if (m_flag) {
            m_value += rhs.m_value;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class CTO, class CBO>
    auto optional_ref<CT, CB>::operator-=(const optional_ref<CTO, CBO> &rhs) -> optional_ref & {
        m_flag = m_flag && rhs.m_flag;
        if (m_flag) {
            m_value -= rhs.m_value;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class CTO, class CBO>
    auto optional_ref<CT, CB>::operator*=(const optional_ref<CTO, CBO> &rhs) -> optional_ref & {
        m_flag = m_flag && rhs.m_flag;
        if (m_flag) {
            m_value *= rhs.m_value;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class CTO, class CBO>
    auto optional_ref<CT, CB>::operator/=(const optional_ref<CTO, CBO> &rhs) -> optional_ref & {
        m_flag = m_flag && rhs.m_flag;
        if (m_flag) {
            m_value /= rhs.m_value;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class CTO, class CBO>
    auto optional_ref<CT, CB>::operator%=(const optional_ref<CTO, CBO> &rhs) -> optional_ref & {
        m_flag = m_flag && rhs.m_flag;
        if (m_flag) {
            m_value %= rhs.m_value;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class CTO, class CBO>
    auto optional_ref<CT, CB>::operator&=(const optional_ref<CTO, CBO> &rhs) -> optional_ref & {
        m_flag = m_flag && rhs.m_flag;
        if (m_flag) {
            m_value &= rhs.m_value;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class CTO, class CBO>
    auto optional_ref<CT, CB>::operator|=(const optional_ref<CTO, CBO> &rhs) -> optional_ref & {
        m_flag = m_flag && rhs.m_flag;
        if (m_flag) {
            m_value |= rhs.m_value;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class CTO, class CBO>
    auto optional_ref<CT, CB>::operator^=(const optional_ref<CTO, CBO> &rhs) -> optional_ref & {
        m_flag = m_flag && rhs.m_flag;
        if (m_flag) {
            m_value ^= rhs.m_value;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class T, check_requires<is_not_xoptional_nor_xmasked_value<T>>>
    auto optional_ref<CT, CB>::operator+=(const T &rhs) -> optional_ref & {
        if (m_flag) {
            m_value += rhs;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class T, check_requires<is_not_xoptional_nor_xmasked_value<T>>>
    auto optional_ref<CT, CB>::operator-=(const T &rhs) -> optional_ref & {
        if (m_flag) {
            m_value -= rhs;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class T, check_requires<is_not_xoptional_nor_xmasked_value<T>>>
    auto optional_ref<CT, CB>::operator*=(const T &rhs) -> optional_ref & {
        if (m_flag) {
            m_value *= rhs;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class T, check_requires<is_not_xoptional_nor_xmasked_value<T>>>
    auto optional_ref<CT, CB>::operator/=(const T &rhs) -> optional_ref & {
        if (m_flag) {
            m_value /= rhs;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class T, check_requires<is_not_xoptional_nor_xmasked_value<T>>>
    auto optional_ref<CT, CB>::operator%=(const T &rhs) -> optional_ref & {
        if (m_flag) {
            m_value %= rhs;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class T, check_requires<is_not_xoptional_nor_xmasked_value<T>>>
    auto optional_ref<CT, CB>::operator&=(const T &rhs) -> optional_ref & {
        if (m_flag) {
            m_value &= rhs;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class T, check_requires<is_not_xoptional_nor_xmasked_value<T>>>
    auto optional_ref<CT, CB>::operator|=(const T &rhs) -> optional_ref & {
        if (m_flag) {
            m_value |= rhs;
        }
        return *this;
    }

    template<class CT, class CB>
    template<class T, check_requires<is_not_xoptional_nor_xmasked_value<T>>>
    auto optional_ref<CT, CB>::operator^=(const T &rhs) -> optional_ref & {
        if (m_flag) {
            m_value ^= rhs;
        }
        return *this;
    }

    // Access
    template<class CT, class CB>
    auto optional_ref<CT, CB>::value() & noexcept -> std::add_lvalue_reference_t<CT> {
        return m_value;
    }

    template<class CT, class CB>
    auto optional_ref<CT, CB>::value() const & noexcept -> std::add_lvalue_reference_t<std::add_const_t<CT>> {
        return m_value;
    }

    template<class CT, class CB>
    auto
    optional_ref<CT, CB>::value() && noexcept -> std::conditional_t<std::is_reference<CT>::value, apply_cv_t<CT, value_type> &, value_type> {
        return m_value;
    }

    template<class CT, class CB>
    auto
    optional_ref<CT, CB>::value() const && noexcept -> std::conditional_t<std::is_reference<CT>::value, const value_type &, value_type> {
        return m_value;
    }

    template<class CT, class CB>
    template<class U>
    auto optional_ref<CT, CB>::value_or(U &&default_value) const & noexcept -> value_type {
        return m_flag ? m_value : std::forward<U>(default_value);
    }

    template<class CT, class CB>
    template<class U>
    auto optional_ref<CT, CB>::value_or(U &&default_value) const && noexcept -> value_type {
        return m_flag ? m_value : std::forward<U>(default_value);
    }

    // Access
    template<class CT, class CB>
    auto optional_ref<CT, CB>::has_value() & noexcept -> std::add_lvalue_reference_t<CB> {
        return m_flag;
    }

    template<class CT, class CB>
    auto optional_ref<CT, CB>::has_value() const & noexcept -> std::add_lvalue_reference_t<std::add_const_t<CB>> {
        return m_flag;
    }

    template<class CT, class CB>
    auto
    optional_ref<CT, CB>::has_value() && noexcept -> std::conditional_t<std::is_reference<CB>::value, apply_cv_t<CB, flag_type> &, flag_type> {
        return m_flag;
    }

    template<class CT, class CB>
    auto
    optional_ref<CT, CB>::has_value() const && noexcept -> std::conditional_t<std::is_reference<CB>::value, const flag_type &, flag_type> {
        return m_flag;
    }

    // Swap
    template<class CT, class CB>
    void optional_ref<CT, CB>::swap(optional_ref &other) {
        std::swap(m_value, other.m_value);
        std::swap(m_flag, other.m_flag);
    }

    // Comparison
    template<class CT, class CB>
    template<class CTO, class CBO>
    auto optional_ref<CT, CB>::equal(const optional_ref<CTO, CBO> &rhs) const noexcept -> bool {
        return (!m_flag && !rhs.m_flag) || (m_value == rhs.m_value && (m_flag && rhs.m_flag));
    }

    template<class CT, class CB>
    template<class CTO, check_requires<is_not_xoptional_nor_xmasked_value<CTO>>>
    bool optional_ref<CT, CB>::equal(const CTO &rhs) const noexcept {
        return m_flag ? (m_value == rhs) : false;
    }

    template<class CT, class CB>
    inline auto optional_ref<CT, CB>::operator&() & -> closure_pointer<self_type &> {
        return closure_pointer<self_type &>(*this);
    }

    template<class CT, class CB>
    inline auto optional_ref<CT, CB>::operator&() const & -> closure_pointer<const self_type &> {
        return closure_pointer<const self_type &>(*this);
    }

    template<class CT, class CB>
    inline auto optional_ref<CT, CB>::operator&() && -> closure_pointer<self_type> {
        return closure_pointer<self_type>(std::move(*this));
    }

    // External operators
    template<class T, class B, class OC, class OT>
    inline std::basic_ostream<OC, OT> &operator<<(std::basic_ostream<OC, OT> &out, const optional_ref<T, B> &v) {
        if (v.has_value()) {
            out << v.value();
        } else {
            out << "N/A";
        }
        return out;
    }

#ifdef __CLING__
    template <class T, class B>
    nlohmann::json mime_bundle_repr(const optional_ref<T, B>& v)
    {
        auto bundle = nlohmann::json::object();
        std::stringstream tmp;
        tmp << v;
        bundle["text/plain"] = tmp.str();
        return bundle;
    }
#endif

    template<class T1, class B1, class T2, class B2>
    inline auto operator==(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> bool {
        return e1.equal(e2);
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline bool operator==(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept {
        return e1.equal(e2);
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline bool operator==(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept {
        return e2.equal(e1);
    }

    template<class T, class B>
    inline auto operator+(const optional_ref<T, B> &e) noexcept
    -> optional_ref<std::decay_t<T>> {
        return e;
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator!=(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> bool {
        return !e1.equal(e2);
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline bool operator!=(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept {
        return !e1.equal(e2);
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline bool operator!=(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept {
        return !e2.equal(e1);
    }

    // Operations
    template<class T, class B>
    inline auto operator-(const optional_ref<T, B> &e) noexcept
    -> optional_ref<std::decay_t<T>> {
        using value_type = std::decay_t<T>;
        return e.has_value() ? -e.value() : missing<value_type>();
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator+(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() && e2.has_value() ? e1.value() + e2.value() : missing<value_type>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator+(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e2.has_value() ? e1 + e2.value() : missing<value_type>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator+(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() ? e1.value() + e2 : missing<value_type>();
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator-(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() && e2.has_value() ? e1.value() - e2.value() : missing<value_type>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator-(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e2.has_value() ? e1 - e2.value() : missing<value_type>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator-(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() ? e1.value() - e2 : missing<value_type>();
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator*(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() && e2.has_value() ? e1.value() * e2.value() : missing<value_type>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator*(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e2.has_value() ? e1 * e2.value() : missing<value_type>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator*(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() ? e1.value() * e2 : missing<value_type>();
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator/(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() && e2.has_value() ? e1.value() / e2.value() : missing<value_type>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator/(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e2.has_value() ? e1 / e2.value() : missing<value_type>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator/(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() ? e1.value() / e2 : missing<value_type>();
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator%(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() && e2.has_value() ? e1.value() % e2.value() : missing<value_type>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator%(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e2.has_value() ? e1 % e2.value() : missing<value_type>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator%(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() ? e1.value() % e2 : missing<value_type>();
    }

    template<class T, class B>
    inline auto operator~(const optional_ref<T, B> &e) noexcept
    -> optional_ref<std::decay_t<T>> {
        using value_type = std::decay_t<T>;
        return e.has_value() ? ~e.value() : missing<value_type>();
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator&(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() && e2.has_value() ? e1.value() & e2.value() : missing<value_type>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator&(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e2.has_value() ? e1 & e2.value() : missing<value_type>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator&(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() ? e1.value() & e2 : missing<value_type>();
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator|(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() && e2.has_value() ? e1.value() | e2.value() : missing<value_type>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator|(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e2.has_value() ? e1 | e2.value() : missing<value_type>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator|(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() ? e1.value() | e2 : missing<value_type>();
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator^(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() && e2.has_value() ? e1.value() ^ e2.value() : missing<value_type>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator^(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e2.has_value() ? e1 ^ e2.value() : missing<value_type>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator^(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() ? e1.value() ^ e2 : missing<value_type>();
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator||(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() && e2.has_value() ? e1.value() || e2.value() : missing<value_type>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator||(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e2.has_value() ? e1 || e2.value() : missing<value_type>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator||(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() ? e1.value() || e2 : missing<value_type>();
    }


    template<class T1, class B1, class T2, class B2>
    inline auto operator&&(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() && e2.has_value() ? e1.value() && e2.value() : missing<value_type>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator&&(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e2.has_value() ? e1 && e2.value() : missing<value_type>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator&&(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> common_optional_t<T1, T2> {
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;
        return e1.has_value() ? e1.value() && e2 : missing<value_type>();
    }

    template<class T, class B>
    inline auto operator!(const optional_ref<T, B> &e) noexcept
    -> optional_ref<bool> {
        return e.has_value() ? !e.value() : missing<bool>();
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator<(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<bool> {
        return e1.has_value() && e2.has_value() ? e1.value() < e2.value() : missing<bool>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator<(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<bool> {
        return e2.has_value() ? e1 < e2.value() : missing<bool>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator<(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> optional_ref<bool> {
        return e1.has_value() ? e1.value() < e2 : missing<bool>();
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator<=(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<bool> {
        return e1.has_value() && e2.has_value() ? e1.value() <= e2.value() : missing<bool>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator<=(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<bool> {
        return e2.has_value() ? e1 <= e2.value() : missing<bool>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator<=(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> optional_ref<bool> {
        return e1.has_value() ? e1.value() <= e2 : missing<bool>();
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator>(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<bool> {
        return e1.has_value() && e2.has_value() ? e1.value() > e2.value() : missing<bool>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator>(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<bool> {
        return e2.has_value() ? e1 > e2.value() : missing<bool>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator>(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> optional_ref<bool> {
        return e1.has_value() ? e1.value() > e2 : missing<bool>();
    }

    template<class T1, class B1, class T2, class B2>
    inline auto operator>=(const optional_ref<T1, B1> &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<bool> {
        return e1.has_value() && e2.has_value() ? e1.value() >= e2.value() : missing<bool>();
    }

    template<class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)>
    inline auto operator>=(const T1 &e1, const optional_ref<T2, B2> &e2) noexcept
    -> optional_ref<bool> {
        return e2.has_value() ? e1 >= e2.value() : missing<bool>();
    }

    template<class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)>
    inline auto operator>=(const optional_ref<T1, B1> &e1, const T2 &e2) noexcept
    -> optional_ref<bool> {
        return e1.has_value() ? e1.value() >= e2 : missing<bool>();
    }

#define UNARY_OPTIONAL(NAME)                                                 \
    template <class T, class B>                                              \
    inline auto NAME(const optional_ref<T, B>& e)                               \
    {                                                                        \
        using std::NAME;                                                     \
        return e.has_value() ? NAME(e.value()) : missing<std::decay_t<T>>(); \
    }

#define UNARY_BOOL_OPTIONAL(NAME)                                       \
    template <class T, class B>                                         \
    inline optional_ref<bool> NAME(const optional_ref<T, B>& e)               \
    {                                                                   \
        using std::NAME;                                                \
        return e.has_value() ? bool(NAME(e.value())) : missing<bool>(); \
    }

#define BINARY_OPTIONAL_1(NAME)                                                                   \
    template <class T1, class B1, class T2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)> \
    inline auto NAME(const optional_ref<T1, B1>& e1, const T2& e2)                                   \
        -> common_optional_t<T1, T2>                                                              \
    {                                                                                             \
        using std::NAME;                                                                          \
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;                \
        return e1.has_value() ? NAME(e1.value(), e2) : missing<value_type>();                     \
    }


#define BINARY_OPTIONAL_2(NAME)                                                                   \
    template <class T1, class T2, class B2, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)> \
    inline auto NAME(const T1& e1, const optional_ref<T2, B2>& e2)                                   \
        -> common_optional_t<T1, T2>                                                              \
    {                                                                                             \
        using std::NAME;                                                                          \
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;                \
        return e2.has_value() ? NAME(e1, e2.value()) : missing<value_type>();                     \
    }

#define BINARY_OPTIONAL_12(NAME)                                                                        \
    template <class T1, class B1, class T2, class B2>                                                   \
    inline auto NAME(const optional_ref<T1, B1>& e1, const optional_ref<T2, B2>& e2)                          \
    {                                                                                                   \
        using std::NAME;                                                                                \
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>>;                      \
        return e1.has_value() && e2.has_value() ? NAME(e1.value(), e2.value()) : missing<value_type>(); \
    }

#define BINARY_OPTIONAL(NAME) \
    BINARY_OPTIONAL_1(NAME)   \
    BINARY_OPTIONAL_2(NAME)   \
    BINARY_OPTIONAL_12(NAME)

#define TERNARY_OPTIONAL_1(NAME)                                                                                                                    \
    template <class T1, class B1, class T2, class T3, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>, is_not_xoptional_nor_xmasked_value<T3>)> \
    inline auto NAME(const optional_ref<T1, B1>& e1, const T2& e2, const T3& e3)                                                                       \
        -> common_optional_t<T1, T2, T3>                                                                                                            \
    {                                                                                                                                               \
        using std::NAME;                                                                                                                            \
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>, std::decay_t<T3>>;                                                \
        return e1.has_value() ? NAME(e1.value(), e2, e3) : missing<value_type>();                                                                   \
    }

#define TERNARY_OPTIONAL_2(NAME)                                                                                                                    \
    template <class T1, class T2, class B2, class T3, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>, is_not_xoptional_nor_xmasked_value<T3>)> \
    inline auto NAME(const T1& e1, const optional_ref<T2, B2>& e2, const T3& e3)                                                                       \
        -> common_optional_t<T1, T2, T3>                                                                                                            \
    {                                                                                                                                               \
        using std::NAME;                                                                                                                            \
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>, std::decay_t<T3>>;                                                \
        return e2.has_value() ? NAME(e1, e2.value(), e3) : missing<value_type>();                                                                   \
    }

#define TERNARY_OPTIONAL_3(NAME)                                                                                                                    \
    template <class T1, class T2, class T3, class B3, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>, is_not_xoptional_nor_xmasked_value<T2>)> \
    inline auto NAME(const T1& e1, const T2& e2, const optional_ref<T3, B3>& e3)                                                                       \
        -> common_optional_t<T1, T2, T3>                                                                                                            \
    {                                                                                                                                               \
        using std::NAME;                                                                                                                            \
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>, std::decay_t<T3>>;                                                \
        return e3.has_value() ? NAME(e1, e2, e3.value()) : missing<value_type>();                                                                   \
    }

#define TERNARY_OPTIONAL_12(NAME)                                                                                     \
    template <class T1, class B1, class T2, class B2, class T3, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T3>)> \
    inline auto NAME(const optional_ref<T1, B1>& e1, const optional_ref<T2, B2>& e2, const T3& e3)                          \
        -> common_optional_t<T1, T2, T3>                                                                              \
    {                                                                                                                 \
        using std::NAME;                                                                                              \
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>, std::decay_t<T3>>;                  \
        return (e1.has_value() && e2.has_value()) ? NAME(e1.value(), e2.value(), e3) : missing<value_type>();         \
    }

#define TERNARY_OPTIONAL_13(NAME)                                                                                     \
    template <class T1, class B1, class T2, class T3, class B3, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T2>)> \
    inline auto NAME(const optional_ref<T1, B1>& e1, const T2& e2, const optional_ref<T3, B3>& e3)                          \
        -> common_optional_t<T1, T2, T3>                                                                              \
    {                                                                                                                 \
        using std::NAME;                                                                                              \
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>, std::decay_t<T3>>;                  \
        return (e1.has_value() && e3.has_value()) ? NAME(e1.value(), e2, e3.value()) : missing<value_type>();         \
    }

#define TERNARY_OPTIONAL_23(NAME)                                                                                     \
    template <class T1, class T2, class B2, class T3, class B3, TURBO_REQUIRES(is_not_xoptional_nor_xmasked_value<T1>)> \
    inline auto NAME(const T1& e1, const optional_ref<T2, B2>& e2, const optional_ref<T3, B3>& e3)                          \
        -> common_optional_t<T1, T2, T3>                                                                              \
    {                                                                                                                 \
        using std::NAME;                                                                                              \
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>, std::decay_t<T3>>;                  \
        return (e2.has_value() && e3.has_value()) ? NAME(e1, e2.value(), e3.value()) : missing<value_type>();         \
    }

#define TERNARY_OPTIONAL_123(NAME)                                                                                                      \
    template <class T1, class B1, class T2, class B2, class T3, class B3>                                                               \
    inline auto NAME(const optional_ref<T1, B1>& e1, const optional_ref<T2, B2>& e2, const optional_ref<T3, B3>& e3)                             \
    {                                                                                                                                   \
        using std::NAME;                                                                                                                \
        using value_type = std::common_type_t<std::decay_t<T1>, std::decay_t<T2>, std::decay_t<T3>>;                                    \
        return (e1.has_value() && e2.has_value() && e3.has_value()) ? NAME(e1.value(), e2.value(), e3.value()) : missing<value_type>(); \
    }

#define TERNARY_OPTIONAL(NAME) \
    TERNARY_OPTIONAL_1(NAME)   \
    TERNARY_OPTIONAL_2(NAME)   \
    TERNARY_OPTIONAL_3(NAME)   \
    TERNARY_OPTIONAL_12(NAME)  \
    TERNARY_OPTIONAL_13(NAME)  \
    TERNARY_OPTIONAL_23(NAME)  \
    TERNARY_OPTIONAL_123(NAME)

    UNARY_OPTIONAL(abs)

    UNARY_OPTIONAL(fabs)

    BINARY_OPTIONAL(fmod)

    BINARY_OPTIONAL(remainder)

    TERNARY_OPTIONAL(fma)

    BINARY_OPTIONAL(fmax)

    BINARY_OPTIONAL(fmin)

    BINARY_OPTIONAL(fdim)

    UNARY_OPTIONAL(exp)

    UNARY_OPTIONAL(exp2)

    UNARY_OPTIONAL(expm1)

    UNARY_OPTIONAL(log)

    UNARY_OPTIONAL(log10)

    UNARY_OPTIONAL(log2)

    UNARY_OPTIONAL(log1p)

    BINARY_OPTIONAL(pow)

    UNARY_OPTIONAL(sqrt)

    UNARY_OPTIONAL(cbrt)

    BINARY_OPTIONAL(hypot)

    UNARY_OPTIONAL(sin)

    UNARY_OPTIONAL(cos)

    UNARY_OPTIONAL(tan)

    UNARY_OPTIONAL(acos)

    UNARY_OPTIONAL(asin)

    UNARY_OPTIONAL(atan)

    BINARY_OPTIONAL(atan2)

    UNARY_OPTIONAL(sinh)

    UNARY_OPTIONAL(cosh)

    UNARY_OPTIONAL(tanh)

    UNARY_OPTIONAL(acosh)

    UNARY_OPTIONAL(asinh)

    UNARY_OPTIONAL(atanh)

    UNARY_OPTIONAL(erf)

    UNARY_OPTIONAL(erfc)

    UNARY_OPTIONAL(tgamma)

    UNARY_OPTIONAL(lgamma)

    UNARY_OPTIONAL(ceil)

    UNARY_OPTIONAL(floor)

    UNARY_OPTIONAL(trunc)

    UNARY_OPTIONAL(round)

    UNARY_OPTIONAL(nearbyint)

    UNARY_OPTIONAL(rint)

    UNARY_BOOL_OPTIONAL(isfinite)

    UNARY_BOOL_OPTIONAL(isinf)

    UNARY_BOOL_OPTIONAL(isnan)

#undef TERNARY_OPTIONAL
#undef TERNARY_OPTIONAL_123
#undef TERNARY_OPTIONAL_23
#undef TERNARY_OPTIONAL_13
#undef TERNARY_OPTIONAL_12
#undef TERNARY_OPTIONAL_3
#undef TERNARY_OPTIONAL_2
#undef TERNARY_OPTIONAL_1
#undef BINARY_OPTIONAL
#undef BINARY_OPTIONAL_12
#undef BINARY_OPTIONAL_2
#undef BINARY_OPTIONAL_1
#undef UNARY_OPTIONAL

    /*************************
     * select implementation *
     *************************/

    template<class B, class T1, class T2, TURBO_REQUIRES(at_least_one_xoptional<B, T1, T2>)>
    inline common_optional_t<T1, T2> select(const B &cond, const T1 &v1, const T2 &v2) noexcept {
        using bool_type = common_optional_t<B>;
        using return_type = common_optional_t<T1, T2>;
        bool_type opt_cond(cond);
        return opt_cond.has_value() ?
               opt_cond.value() ? return_type(v1) : return_type(v2) :
               missing<typename return_type::value_type>();
    }
}

#endif  // TURBO_META_OPTIONAL_REF_H_

