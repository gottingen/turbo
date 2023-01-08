
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_CONTAINER_CACHE_POLICY_H_
#define FLARE_CONTAINER_CACHE_POLICY_H_

#include <memory>
#include <string>

namespace flare {

        template<class Key, class Value>
        class cache_policy {
        public:
            cache_policy() = default;

            virtual ~cache_policy() = default;

            virtual void on_cache_set(const Key &key, const Value &value) = 0;

            virtual void on_cache_del(const Key &key, const Value &value) = 0;

            virtual void clear() = 0;

            virtual std::string to_string() const = 0;
        };

        template<class Key, class Value>
        class empty_cache_policy final : public cache_policy<Key, Value> {
        public:
            empty_cache_policy() = default;

            ~empty_cache_policy() = default;

            inline void on_cache_set(const Key &key, const Value &value) override {}

            inline void on_cache_del(const Key &key, const Value &value) override {}

            void clear() override {}

            inline std::string to_string() const override { return "{\"empty_cache_policy\":{}}"; }
        };

        template<class Key, class Value>
        using cache_policy_ptr = std::unique_ptr<cache_policy<Key, Value>>;
}  // namespace flare

#endif  // FLARE_CONTAINER_CACHE_POLICY_H_
