// Copyright 2013-2023 Daniel Parker
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

#ifndef JSONCONS_JSONSCHEMA_SCHEMA_LOCATION_HPP
#define JSONCONS_JSONSCHEMA_SCHEMA_LOCATION_HPP

#include "turbo/jsoncons/config/jsoncons_config.h"
#include "turbo/jsoncons/uri.h"
#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/jsonpointer/jsonpointer.h"
#include "turbo/jsoncons/jsonschema/jsonschema_error.h"

namespace turbo {
namespace jsonschema {

    class schema_location
    {
        turbo::uri uri_;
        std::string identifier_;
    public:
        schema_location()
        {
        }

        schema_location(const std::string& uri)
        {
            auto pos = uri.find('#');
            if (pos != std::string::npos)
            {
                identifier_ = uri.substr(pos + 1); 
                unescape_percent(identifier_);
            }
            uri_ = turbo::uri(uri);
        }

        turbo::uri uri() const
        {
            return uri_;
        }

        bool has_fragment() const
        {
            return !identifier_.empty();
        }

        bool has_identifier() const
        {
            return !identifier_.empty() && identifier_.front() != '/';
        }

        turbo::string_view base() const
        {
            return uri_.base();
        }

        turbo::string_view path() const
        {
            return uri_.path();
        }

        bool is_absolute() const
        {
            return uri_.is_absolute();
        }

        std::string identifier() const
        {
            return identifier_;
        }

        std::string fragment() const
        {
            return identifier_;
        }

        schema_location resolve(const schema_location& uri) const
        {
            schema_location new_uri;
            new_uri.identifier_ = identifier_;
            new_uri.uri_ = uri_.resolve(uri.uri_);
            return new_uri;
        }

        int compare(const schema_location& other) const
        {
            int result = uri_.compare(other.uri_);
            if (result != 0) 
            {
                return result;
            }
            return result; 
        }

        schema_location append(const std::string& field) const
        {
            if (has_identifier())
                return *this;

            turbo::jsonpointer::json_pointer pointer(std::string(uri_.fragment()));
            pointer /= field;

            turbo::uri new_uri(uri_.scheme(),
                                  uri_.userinfo(),
                                  uri_.host(),
                                  uri_.port(),
                                  uri_.path(),
                                  uri_.query(),
                                  pointer.to_string());

            schema_location wrapper;
            wrapper.uri_ = new_uri;
            wrapper.identifier_ = pointer.to_string();

            return wrapper;
        }

        schema_location append(std::size_t index) const
        {
            if (has_identifier())
                return *this;

            turbo::jsonpointer::json_pointer pointer(std::string(uri_.fragment()));
            pointer /= index;

            turbo::uri new_uri(uri_.scheme(),
                                  uri_.userinfo(),
                                  uri_.host(),
                                  uri_.port(),
                                  uri_.path(),
                                  uri_.query(),
                                  pointer.to_string());

            schema_location wrapper;
            wrapper.uri_ = new_uri;
            wrapper.identifier_ = pointer.to_string();

            return wrapper;
        }

        std::string string() const
        {
            std::string s = uri_.string();
            return s;
        }

        friend bool operator==(const schema_location& lhs, const schema_location& rhs)
        {
            return lhs.compare(rhs) == 0;
        }

        friend bool operator!=(const schema_location& lhs, const schema_location& rhs)
        {
            return lhs.compare(rhs) != 0;
        }

        friend bool operator<(const schema_location& lhs, const schema_location& rhs)
        {
            return lhs.compare(rhs) < 0;
        }

        friend bool operator<=(const schema_location& lhs, const schema_location& rhs)
        {
            return lhs.compare(rhs) <= 0;
        }

        friend bool operator>(const schema_location& lhs, const schema_location& rhs)
        {
            return lhs.compare(rhs) > 0;
        }

        friend bool operator>=(const schema_location& lhs, const schema_location& rhs)
        {
            return lhs.compare(rhs) >= 0;
        }
    private:
        static void unescape_percent(std::string& s)
        {
            if (s.size() >= 3)
            {
                std::size_t pos = s.size() - 2;
                while (pos-- >= 1)
                {
                    if (s[pos] == '%')
                    {
                        std::string hex = s.substr(pos + 1, 2);
                        char ch = (char) std::strtoul(hex.c_str(), nullptr, 16);
                        s.replace(pos, 3, 1, ch);
                    }
                }
            }
        }
    };

} // namespace jsonschema
} // namespace turbo

#endif // JSONCONS_JSONSCHEMA_SCHEMA_LOCATION_HPP
