
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_RAPAIDJSON_ROBUST_JSON_H_
#define FLARE_RAPAIDJSON_ROBUST_JSON_H_

#include <turbo/rapidjson/document.h>
#include <turbo/rapidjson/error/en.h>
#include <turbo/rapidjson/reader.h>
#include <turbo/rapidjson/stringbuffer.h>
#include <turbo/rapidjson/writer.h>
#include <turbo/log/logging.h>
#include <turbo/base/result_status.h>

#include <functional>
#include <optional>
#include <string_view>

RAPIDJSON_NAMESPACE_BEGIN

    flare::result_status robust_parse(const std::string& file_path, rapidjson::Document* doc);

    flare::result_status robust_write(const rapidjson::Document& d, const std::string& file_path);

    template<typename RapidJsonValue>
    class robust_json {
        static_assert(std::is_same_v<RapidJsonValue, rapidjson::Value> ||
                      std::is_same_v<RapidJsonValue, const rapidjson::Value>,
                      "only support const or non-const rapidjson::Value");

    public:
        explicit robust_json(RapidJsonValue &v) : v_(std::ref(v)) {}

        robust_json() : v_(std::nullopt) {}

        /**
         *  if current node is a rapidjson::Value
         * @return
         */
        explicit operator bool() const noexcept { return v_.has_value(); }

        /**
         *
         * @return
         */
        [[nodiscard]] RapidJsonValue &raw() const {
            FLARE_CHECK(v_.has_value());
            return v_.value().get();
        }

        robust_json operator[](const std::string_view &key) const noexcept {
            if (!v_.has_value() || !v_.value().get().IsObject()) {
                return robust_json{};
            }
            auto itr = v_.value().get().FindMember(rapidjson::Value{rapidjson::StringRef(key)});
            if (itr == v_.value().get().MemberEnd()) {
                return robust_json{};
            }
            return robust_json{itr->value};
        }

        robust_json operator[](size_t i) const noexcept {
            if (!v_.has_value() || !v_.value().get().IsArray() || i >= v_.value().get().Size()) {
                return robust_json{};
            }
            return robust_json{v_.value().get()[i]};
        }

        template<typename TargetValue>
        TargetValue cast() const noexcept {
            return TargetValue::RobustJsonNotImplementedForThisTargetType();
        }

        template<typename TargetValue>
        std::optional<TargetValue> as() const noexcept {
            return TargetValue::RobustJsonNotImplementedForThisTargetType();
        }

    private:
        std::optional<std::reference_wrapper<RapidJsonValue>> v_;
    };

#define ROBUST_JSON_DECLARE_BOTH_CAST(T)                    \
  template <>                                               \
  template <>                                               \
  T robust_json<rapidjson::Value>::cast<T>() const noexcept; \
  template <>                                               \
  template <>                                               \
  T robust_json<const rapidjson::Value>::cast<T>() const noexcept;

#define ROBUST_JSON_DECLARE_BOTH_AS(T)                                   \
  template <>                                                            \
  template <>                                                            \
  std::optional<T> robust_json<rapidjson::Value>::as<T>() const noexcept; \
  template <>                                                            \
  template <>                                                            \
  std::optional<T> robust_json<const rapidjson::Value>::as<T>() const noexcept;

    ROBUST_JSON_DECLARE_BOTH_CAST(std::string_view);

    ROBUST_JSON_DECLARE_BOTH_CAST(bool);

    ROBUST_JSON_DECLARE_BOTH_CAST(rapidjson::Value::ConstArray);

    ROBUST_JSON_DECLARE_BOTH_AS(rapidjson::Value::ConstObject);

    template<>
    template<>
    std::optional<rapidjson::Value::Object>
    robust_json<rapidjson::Value>::as<rapidjson::Value::Object>() const noexcept;

    ROBUST_JSON_DECLARE_BOTH_AS(rapidjson::Value::ConstArray);

    template<>
    template<>
    std::optional<rapidjson::Value::Array> robust_json<rapidjson::Value>::as<rapidjson::Value::Array>() const noexcept;

    ROBUST_JSON_DECLARE_BOTH_AS(uint64_t);

    ROBUST_JSON_DECLARE_BOTH_AS(int64_t);

    ROBUST_JSON_DECLARE_BOTH_AS(double);


RAPIDJSON_NAMESPACE_END

#endif  // FLARE_RAPAIDJSON_ROBUST_JSON_H_
