
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "turbo/json/robust_json.h"
#include <fstream>
#include <cerrno>
#include "turbo/strings/numbers.h"
#include "turbo/json/ostreamwrapper.h"
#include "turbo/json/writer.h"
#include "turbo/files/filesystem.h"
#include "turbo/files/sequential_read_file.h"

#define ROBUST_JSON_DEFINE_BOTH_CAST(T, TPL_FUNC)                  \
  template <>                                                      \
  template <>                                                      \
  T robust_json<rapidjson::Value>::cast<T>() const noexcept {       \
    return TPL_FUNC;                                               \
  }                                                                \
  template <>                                                      \
  template <>                                                      \
  T robust_json<const rapidjson::Value>::cast<T>() const noexcept { \
    return TPL_FUNC;                                               \
  }

#define ROBUST_JSON_DEFINE_BOTH_AS(T, TPL_FUNC)                                 \
  template <>                                                                   \
  template <>                                                                   \
  std::optional<T> robust_json<rapidjson::Value>::as<T>() const noexcept {       \
    return TPL_FUNC;                                                            \
  }                                                                             \
  template <>                                                                   \
  template <>                                                                   \
  std::optional<T> robust_json<const rapidjson::Value>::as<T>() const noexcept { \
    return TPL_FUNC;                                                            \
  }


RAPIDJSON_NAMESPACE_BEGIN

    turbo::Status robust_parse(const std::string &file_path, rapidjson::Document *doc) {
        std::error_code ec;
        auto r = turbo::filesystem::exists(file_path, ec);
        if (ec) {
            return turbo::ErrnoToStatus(ec.value(), "check file exists error");
        }
        if(!r) {
            return turbo::NotFoundError("not exists file");
        }

        turbo::SequentialReadFile file;
        auto rs = file.open(file_path);
        if(!rs.ok()) {
            return rs;
        }
        std::string content;
        rs = file.read(&content);
        if(!rs.ok()) {
            return rs;
        }
        if (!static_cast<rapidjson::ParseResult>(doc->Parse(content.c_str()))) {
            return turbo::DataLossError("json parse error");
        }
        return turbo::OkStatus();
    }


    turbo::Status robust_write(const rapidjson::Document &d, const std::string &file_path) {
        std::ofstream ofs(file_path);
        if (!ofs) {
            return turbo::ErrnoToStatus(errno, "open file error");
        }
        rapidjson::OStreamWrapper osw(ofs);
        rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
        if (!d.Accept(writer)) {
            return  turbo::ErrnoToStatus(errno, "write file error");
        }
        return turbo::OkStatus();
    }


    template<typename OptionalRefRapidJsonValue>
    std::string_view cast_string_view(OptionalRefRapidJsonValue v_) {
        if (v_.has_value() && v_.value().get().IsString()) {
            return std::string_view{v_.value().get().GetString(), v_.value().get().GetStringLength()};
        }
        return std::string_view{};
    }

    ROBUST_JSON_DEFINE_BOTH_CAST(std::string_view, cast_string_view(v_));

    template<typename OptionalRefRapidJsonValue>
    bool cast_bool(OptionalRefRapidJsonValue v_) {
        if (!v_.has_value()) {
            return false;
        }
        auto &v = v_.value().get();
        if (v.IsBool()) {
            return v.GetBool();
        }
        if (v.IsNull()) {
            return false;
        }
        if (v.IsString()) {
            return !(v.GetStringLength() == 0 || (v.GetStringLength() == 1 && *v.GetString() == '0'));
        }
        if (v.IsInt64()) {
            return v.GetInt64() != 0;
        }
        return true;
    }

    ROBUST_JSON_DEFINE_BOTH_CAST(bool, cast_bool(v_));

    const rapidjson::Value kEmptyConstArray{rapidjson::kArrayType};

    template<typename OptionalRefRapidJsonValue>
    rapidjson::Value::ConstArray cast_const_array(OptionalRefRapidJsonValue v_) {
        if (v_.has_value() && v_.value().get().IsArray()) {
            return static_cast<const rapidjson::Value &>(v_.value().get()).GetArray();
        }
        return kEmptyConstArray.GetArray();
    }

    ROBUST_JSON_DEFINE_BOTH_CAST(rapidjson::Value::ConstArray, cast_const_array(v_));

    template<typename OptionalRefRapidJsonValue>
    std::optional<rapidjson::Value::ConstObject> as_const_object(OptionalRefRapidJsonValue v_) {
        if (v_.has_value() && v_.value().get().IsObject()) {
            return std::make_optional(static_cast<const rapidjson::Value &>(v_.value().get()).GetObject());
        }
        return std::nullopt;
    }

    ROBUST_JSON_DEFINE_BOTH_AS(rapidjson::Value::ConstObject, as_const_object(v_));

    template<>
    template<>
    std::optional<rapidjson::Value::Object>
    robust_json<rapidjson::Value>::as<rapidjson::Value::Object>() const noexcept {
        if (v_.has_value() && v_.value().get().IsObject()) {
            return std::make_optional(v_.value().get().GetObject());
        }
        return std::nullopt;
    }

    template<typename OptionalRefRapidJsonValue>
    std::optional<rapidjson::Value::ConstArray> as_const_array(OptionalRefRapidJsonValue v_) {
        if (v_.has_value() && v_.value().get().IsArray()) {
            return std::make_optional(static_cast<const rapidjson::Value &>(v_.value().get()).GetArray());
        }
        return std::nullopt;
    }

    ROBUST_JSON_DEFINE_BOTH_AS(rapidjson::Value::ConstArray, as_const_array(v_));

    template<>
    template<>
    std::optional<rapidjson::Value::Array> robust_json<rapidjson::Value>::as<rapidjson::Value::Array>() const noexcept {
        if (v_.has_value() && v_.value().get().IsArray()) {
            return std::make_optional(v_.value().get().GetArray());
        }
        return std::nullopt;
    }

    template<typename OptionalRefRapidJsonValue>
    std::optional<uint64_t> as_uint64(OptionalRefRapidJsonValue v_) {
        if (v_.has_value()) {
            auto &v = v_.value().get();
            if (v.IsUint64()) {
                // compatible with kNumberUintFlag/kNumberUint64Flag values
                return std::make_optional(v.GetUint64());
            }
            if (v.IsString()) {
                uint64_t n;
                if (!turbo::SimpleAtoi(std::string_view{v.GetString(), v.GetStringLength()}, &n)) {
                    return std::nullopt;
                }
                return n;
            }
        }
        return std::nullopt;
    }

    ROBUST_JSON_DEFINE_BOTH_AS(uint64_t, as_uint64(v_));

    template<typename OptionalRefRapidJsonValue>
    std::optional<uint64_t> as_int64(OptionalRefRapidJsonValue v_) {
        if (v_.has_value()) {
            auto &v = v_.value().get();
            if (v.IsInt64()) {
                // compatible with kNumberIntFlag/kNumberUintFlag/kNumberInt64Flag values
                return std::make_optional(v.GetInt64());
            }
            if (v.IsString()) {
                uint64_t n;
                if (!turbo::SimpleAtoi(std::string_view{v.GetString(), v.GetStringLength()}, &n)) {
                    return std::nullopt;
                }
                return n;
            }
        }
        return std::nullopt;
    }

    ROBUST_JSON_DEFINE_BOTH_AS(int64_t, as_int64(v_));

    template<typename OptionalRefRapidJsonValue>
    std::optional<double> as_double(OptionalRefRapidJsonValue v_) {
        if (v_.has_value()) {
            auto &v = v_.value().get();
            if (v.IsNumber()) {
                // compatible with any number values
                return std::make_optional(v.GetDouble());
            }
            if (v.IsString()) {
                double n;
                if (!turbo::SimpleAtod(std::string_view{v.GetString(), v.GetStringLength()}, &n)) {
                    return std::nullopt;
                }
                return n;
            }
        }
        return std::nullopt;
    }

    ROBUST_JSON_DEFINE_BOTH_AS(double, as_double(v_));


RAPIDJSON_NAMESPACE_END