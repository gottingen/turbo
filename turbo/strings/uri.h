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
// Created by jeff on 24-6-1.
//
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <turbo/base/macros.h>
#include <turbo/base/nullability.h>
#include <turbo/strings/string_view.h>

namespace turbo {

    /// \brief A parsed URI
    class TURBO_DLL Uri {
            public:
            Uri();
            ~Uri();
            Uri(Uri&&);
            Uri& operator=(Uri&&);

            // XXX Should we use std::string_view instead?  These functions are
            // not performance-critical.

            /// The URI scheme, such as "http", or the empty string if the URI has no
            /// explicit scheme.
            std::string scheme() const;

            /// Convenience function that returns true if the scheme() is "file"
            bool is_file_scheme() const;

            /// Whether the URI has an explicit host name.  This may return true if
            /// the URI has an empty host (e.g. "file:///tmp/foo"), while it returns
            /// false is the URI has not host component at all (e.g. "file:/tmp/foo").
            bool has_host() const;
            /// The URI host name, such as "localhost", "127.0.0.1" or "::1", or the empty
            /// string is the URI does not have a host component.
            std::string host() const;

            /// The URI port number, as a string such as "80", or the empty string is the URI
            /// does not have a port number component.
            std::string port_text() const;
            /// The URI port parsed as an integer, or -1 if the URI does not have a port
            /// number component.
            int32_t port() const;

            /// The username specified in the URI.
            std::string username() const;
            /// The password specified in the URI.
            std::string password() const;

            /// The URI path component.
            std::string path() const;

            /// The URI query string
            std::string query_string() const;

            /// The URI query items
            ///
            /// Note this API doesn't allow differentiating between an empty value
            /// and a missing value, such in "a&b=1" vs. "a=&b=1".
            bool query_items(turbo::Nonnull<std::vector<std::pair<std::string, std::string>>*>, std::string* err = nullptr) const;

            /// Get the string representation of this URI.
            const std::string& ToString() const;

            /// Factory function to parse a URI from its string representation.
            bool parse(const std::string& uri_string, std::string *error_message = nullptr);

            /// Factory function to parse a URI from its string representation.
            static bool from_string(const std::string& uri_string, turbo::Nonnull<Uri*>, std::string* error_message = nullptr);

            private:
            struct Impl;
            std::unique_ptr<Impl> impl_;
    };

    /// Percent-encode the input string, for use e.g. as a URI query parameter.
    ///
    /// This will escape directory separators, making this function unsuitable
    /// for encoding URI paths directly. See uri_from_absolute_path() instead.
    TURBO_DLL std::string uri_escape(turbo::string_view s);

    TURBO_DLL std::string uri_unescape(turbo::string_view s);

    /// Encode a host for use within a URI, such as "localhost",
    /// "127.0.0.1", or "[::1]".
    TURBO_DLL std::string uri_encode_host(turbo::string_view host);

    /// Whether the string is a syntactically valid URI scheme according to RFC 3986.
    TURBO_DLL bool is_valid_uri_scheme(turbo::string_view s);

    /// Create a file uri from a given absolute path
    TURBO_DLL bool uri_from_absolute_path(turbo::string_view path, turbo::Nullable<std::string*> result, std::string* error_message = nullptr);

}  // namespace turbo
