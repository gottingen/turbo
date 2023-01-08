
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef FLARE_BASE_TYPE_INDEX_H_
#define FLARE_BASE_TYPE_INDEX_H_

#include <functional>
#include <typeindex>

namespace flare::base {


    namespace detail {

        // For each type, there is only one instance of `type_index_entity`. `type_index`
        // keeps a reference to the entity, which is used for comparison and other
        // stuff.
        struct type_index_entity {
            std::type_index runtime_type_index;
        };

        template <class T>
        inline const type_index_entity kTypeIndexEntity{std::type_index(typeid(T))};

    }  // namespace detail

    // Due to QoI issue in libstdc++, which uses `strcmp` for comparing
    // `std::type_index`, we roll our own.
    //
    // Note our own does NOT support runtime type, only compile time type is
    // recognized.
    class type_index {
    public:
        constexpr type_index() noexcept : entity_(nullptr) {}

        // In case you need `std::type_index` of the corresponding type, this method
        // is provided for you convenience.
        //
        // Keep in mind, though, that this method can be slow. In most cases you
        // should only use it for logging purpose.
        std::type_index get_runtime_type_index() const {
            return entity_->runtime_type_index;
        }

        constexpr bool operator==(type_index t) const noexcept {
            return entity_ == t.entity_;
        }
        constexpr bool operator!=(type_index t) const noexcept {
            return entity_ != t.entity_;
        }
        constexpr bool operator<(type_index t) const noexcept {
            return entity_ < t.entity_;
        }

        // If C++20 is usable, you get all other comparison operators (!=, <=, >, ...)
        // automatically. I'm not gonna implement them as we don't use them for the
        // time being.

    private:
        template <class T>
        friend constexpr type_index get_type_index();
        friend struct std::hash<type_index>;

        constexpr explicit type_index(const detail::type_index_entity* entity) noexcept
                : entity_(entity) {}

    private:
        const detail::type_index_entity* entity_;
    };

    template <class T>
    constexpr type_index get_type_index() {
        return type_index(&detail::kTypeIndexEntity<T>);
    }

}  // namespace flare::base


namespace std {

    template <>
    struct hash<flare::base::type_index> {
        size_t operator()(const flare::base::type_index& type) const noexcept {
            return std::hash<const void*>{}(type.entity_);
        }
    };

}  // namespace std

#endif  // FLARE_BASE_TYPE_INDEX_H_
