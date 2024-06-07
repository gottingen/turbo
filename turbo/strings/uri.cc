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

#include <turbo/strings/uri.h>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <string_view>
#include <vector>
#include <turbo/strings/uriparser/Uri.h>
#include <turbo/strings/numbers.h>
#include <turbo/base/internal/raw_logging.h>

namespace turbo {

    namespace {

        std::string_view TextRangeToView(const UriTextRangeStructA &range) {
            if (range.first == nullptr) {
                return "";
            } else {
                return {range.first, static_cast<size_t>(range.afterLast - range.first)};
            }
        }

        std::string TextRangeToString(const UriTextRangeStructA &range) {
            return std::string(TextRangeToView(range));
        }

        // There can be a difference between an absent field and an empty field.
        // For example, in "unix:/tmp/foo", the host is absent, while in
        // "unix:///tmp/foo", the host is empty but present.
        // This function helps distinguish.
        bool IsTextRangeSet(const UriTextRangeStructA &range) { return range.first != nullptr; }

#ifdef _WIN32
        bool IsDriveSpec(const std::string_view s) {
  return (s.length() >= 2 && s[1] == ':' &&
          ((s[0] >= 'A' && s[0] <= 'Z') || (s[0] >= 'a' && s[0] <= 'z')));
}
#endif

    }  // namespace

    std::string uri_escape(std::string_view s) {
        if (s.empty()) {
            // Avoid passing null pointer to uriEscapeExA
            return std::string(s);
        }
        std::string escaped;
        escaped.resize(3 * s.length());

        auto end = uriEscapeExA(s.data(), s.data() + s.length(), &escaped[0],
                /*spaceToPlus=*/URI_FALSE, /*normalizeBreaks=*/URI_FALSE);
        escaped.resize(end - &escaped[0]);
        return escaped;
    }

    std::string uri_unescape(std::string_view s) {
        std::string result(s);
        if (!result.empty()) {
            auto end = uriUnescapeInPlaceA(&result[0]);
            result.resize(end - &result[0]);
        }
        return result;
    }

    std::string uri_encode_host(std::string_view host) {
        // Fairly naive check: if it contains a ':', it's IPv6 and needs
        // brackets, else it's OK
        if (host.find(":") != std::string::npos) {
            std::string result = "[";
            result += host;
            result += ']';
            return result;
        } else {
            return std::string(host);
        }
    }

    bool is_valid_uri_scheme(std::string_view s) {
        auto is_alpha = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); };
        auto is_scheme_char = [&](char c) {
            return is_alpha(c) || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
        };

        if (s.empty()) {
            return false;
        }
        if (!is_alpha(s[0])) {
            return false;
        }
        return std::all_of(s.begin() + 1, s.end(), is_scheme_char);
    }

    struct Uri::Impl {
        Impl() { memset(&uri_, 0, sizeof(uri_)); }

        ~Impl() { uriFreeUriMembersA(&uri_); }

        void Reset() {
            uriFreeUriMembersA(&uri_);
            memset(&uri_, 0, sizeof(uri_));
            data_.clear();
            string_rep_.clear();
            path_segments_.clear();
            port_ = -1;
        }

        const std::string &KeepString(const std::string &s) {
            data_.push_back(s);
            return data_.back();
        }

        UriUriA uri_;
        // Keep alive strings that uriparser stores pointers to
        std::vector<std::string> data_;
        std::string string_rep_;
        int32_t port_ = -1;
        std::vector<std::string_view> path_segments_;
        bool is_file_uri_;
        bool is_absolute_path_;
    };

    Uri::Uri() : impl_(new Impl) {}

    Uri::~Uri() = default;

    Uri::Uri(Uri &&u) : impl_(std::move(u.impl_)) {}

    Uri &Uri::operator=(Uri &&u) {
        impl_ = std::move(u.impl_);
        return *this;
    }

    std::string Uri::scheme() const { return TextRangeToString(impl_->uri_.scheme); }

    bool Uri::is_file_scheme() const { return impl_->is_file_uri_; }

    std::string Uri::host() const {
        // XXX for now we're assuming that %-encoding is expected, but this could be
        // scheme-dependent (for example, http(s) may expect IDNA instead?)
        return uri_unescape(TextRangeToView(impl_->uri_.hostText));
    }

    bool Uri::has_host() const { return IsTextRangeSet(impl_->uri_.hostText); }

    std::string Uri::port_text() const { return TextRangeToString(impl_->uri_.portText); }

    int32_t Uri::port() const { return impl_->port_; }

    std::string Uri::username() const {
        auto userpass = TextRangeToView(impl_->uri_.userInfo);
        auto sep_pos = userpass.find_first_of(':');
        if (sep_pos != std::string_view::npos) {
            userpass = userpass.substr(0, sep_pos);
        }
        return uri_unescape(userpass);
    }

    std::string Uri::password() const {
        auto userpass = TextRangeToView(impl_->uri_.userInfo);
        auto sep_pos = userpass.find_first_of(':');
        if (sep_pos == std::string_view::npos) {
            return "";
        }
        return uri_unescape(userpass.substr(sep_pos + 1));
    }

    std::string Uri::path() const {
        const auto &segments = impl_->path_segments_;

        bool must_prepend_slash = impl_->is_absolute_path_;
#ifdef _WIN32
        // On Windows, "file:///C:/foo" should have path "C:/foo", not "/C:/foo",
  // despite it being absolute.
  // (see https://tools.ietf.org/html/rfc8089#page-13)
  if (impl_->is_absolute_path_ && impl_->is_file_uri_ && segments.size() > 0 &&
      IsDriveSpec(segments[0])) {
    must_prepend_slash = false;
  }
#endif

        std::stringstream ss;
        if (must_prepend_slash) {
            ss << "/";
        }
        bool first = true;
        for (const auto &seg: segments) {
            if (!first) {
                ss << "/";
            }
            first = false;
            ss << uri_unescape(seg);
        }
        return std::move(ss).str();
    }

    std::string Uri::query_string() const { return TextRangeToString(impl_->uri_.query); }

    bool
    Uri::query_items(turbo::Nonnull<std::vector<std::pair<std::string, std::string>> *> ret, std::string *err) const {
        const auto &query = impl_->uri_.query;
        UriQueryListA *query_list;
        int item_count;
        std::vector<std::pair<std::string, std::string>> items;

        if (query.first == nullptr) {
            ret->swap(items);
            return true;
        }
        if (uriDissectQueryMallocA(&query_list, &item_count, query.first, query.afterLast) != URI_SUCCESS) {
            if (err != nullptr) {
                *err = "Cannot parse query string: '" + query_string() + "'";
            }
            return false;
        }
        std::unique_ptr<UriQueryListA, decltype(&uriFreeQueryListA)> query_guard(
                query_list, uriFreeQueryListA);

        items.reserve(item_count);
        while (query_list != nullptr) {
            if (query_list->value != nullptr) {
                items.emplace_back(query_list->key, query_list->value);
            } else {
                items.emplace_back(query_list->key, "");
            }
            query_list = query_list->next;
        }
        ret->swap(items);
        return true;
    }

    const std::string &Uri::ToString() const { return impl_->string_rep_; }

    bool Uri::parse(const std::string &uri_string, std::string *error_message) {
        impl_->Reset();

        const auto &s = impl_->KeepString(uri_string);
        impl_->string_rep_ = s;
        const char *error_pos;
        if (uriParseSingleUriExA(&impl_->uri_, s.data(), s.data() + s.size(), &error_pos) !=
            URI_SUCCESS) {
            if (error_message != nullptr) {
                *error_message = "Cannot parse URI: '" + uri_string + "'";
            }
            return false;
        }

        const auto scheme = TextRangeToView(impl_->uri_.scheme);
        if (scheme.empty()) {
            if (error_message != nullptr)
                *error_message = "URI has empty scheme: '" + uri_string + "'";
            return false;
        }
        impl_->is_file_uri_ = (scheme == "file");

        // Gather path segments
        auto path_seg = impl_->uri_.pathHead;
        while (path_seg != nullptr) {
            impl_->path_segments_.push_back(TextRangeToView(path_seg->text));
            path_seg = path_seg->next;
        }

        // Decide whether URI path is absolute
        impl_->is_absolute_path_ = false;
        if (impl_->uri_.absolutePath == URI_TRUE) {
            impl_->is_absolute_path_ = true;
        } else if (has_host() && impl_->path_segments_.size() > 0) {
            // When there's a host (even empty), uriparser considers the path relative.
            // Several URI parsers for Python all consider it absolute, though.
            // For example, the path for "file:///tmp/foo" is "/tmp/foo", not "tmp/foo".
            // Similarly, the path for "file://localhost/" is "/".
            // However, the path for "file://localhost" is "".
            impl_->is_absolute_path_ = true;
        }
#ifdef _WIN32
        // There's an exception on Windows: "file:/C:foo/bar" is relative.
  if (impl_->is_file_uri_ && impl_->path_segments_.size() > 0) {
    const auto& first_seg = impl_->path_segments_[0];
    if (IsDriveSpec(first_seg) && (first_seg.length() >= 3 && first_seg[2] != '/')) {
      impl_->is_absolute_path_ = false;
    }
  }
#endif

        if (impl_->is_file_uri_ && !impl_->is_absolute_path_) {
            if (error_message != nullptr)
                *error_message = "File URI path must be absolute: '" + uri_string + "'";
            return false;
        }

        // Parse port number
        auto port_text = TextRangeToView(impl_->uri_.portText);
        if (!port_text.empty()) {
            uint32_t port_num;
            if (!turbo::simple_atoi(port_text, &port_num)) {
                if (error_message != nullptr)
                    *error_message = "Invalid port number '" + std::string(port_text) + "' in URI '" + uri_string + "'";
                return false;
            }
            if(port_num > std::numeric_limits<int16_t>::max()) {
                if (error_message != nullptr)
                    *error_message = "Port number '" + std::string(port_text) + "' in URI '" + uri_string + "' is too large";
                return false;
            }
            impl_->port_ = port_num;
        }

        return true;
    }

    bool Uri::from_string(const std::string &uri_string, turbo::Nonnull<Uri *> uri,
                         std::string *error_message) {
        return uri->parse(uri_string, error_message);
    }

    bool uri_from_absolute_path(std::string_view path, turbo::Nonnull<std::string *> uri, std::string *error_message) {
        if (path.empty()) {
            if (error_message != nullptr) {
                *error_message = "uri_from_absolute_path expected an absolute path, got an empty string";
            }
            return false;
        }
#ifdef _WIN32
        // Turn "/" separators into "\", as Windows recognizes both but uriparser
  // only the latter.
  std::string fixed_path(path);
  std::replace(fixed_path.begin(), fixed_path.end(), '/', '\\');
  out.resize(8 + 3 * fixed_path.length() + 1);
  int r = uriWindowsFilenameToUriStringA(fixed_path.data(), out.data());
  // uriWindowsFilenameToUriStringA basically only fails if a null pointer is given.
  TURBO_INTERNAL_CHECK(r == 0, "uriWindowsFilenameToUriStringA unexpectedly failed");
#else
        uri->resize(7 + 3 * path.length() + 1);
        int r = uriUnixFilenameToUriStringA(path.data(), uri->data());
        // same as above (uriWindowsFilenameToUriStringA)
        TURBO_INTERNAL_CHECK(r == 0, "uriUnixFilenameToUriStringA unexpectedly failed");
#endif
        uri->resize(strlen(uri->data()));
        return true;
    }

}  // namespace turbo
