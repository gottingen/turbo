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

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <turbo/strings/uri.h>

namespace turbo {

    TEST(uri_escape, Basics) {
        ASSERT_EQ(uri_escape(""), "");
        ASSERT_EQ(uri_escape("foo123"), "foo123");
        ASSERT_EQ(uri_escape("/El Niño/"), "%2FEl%20Ni%C3%B1o%2F");
        ASSERT_EQ(uri_escape("arrow.apache.org"), "arrow.apache.org");
        ASSERT_EQ(uri_escape("192.168.1.1"), "192.168.1.1");
    }

    TEST(uri_encode_host, Basics) {
        ASSERT_EQ(uri_encode_host("::1"), "[::1]");
        ASSERT_EQ(uri_encode_host("arrow.apache.org"), "arrow.apache.org");
        ASSERT_EQ(uri_encode_host("192.168.1.1"), "192.168.1.1");
    }

    TEST(is_valid_uri_scheme, Basics) {
        ASSERT_FALSE(is_valid_uri_scheme(""));
        ASSERT_FALSE(is_valid_uri_scheme(":"));
        ASSERT_FALSE(is_valid_uri_scheme("."));
        ASSERT_TRUE(is_valid_uri_scheme("a"));
        ASSERT_TRUE(is_valid_uri_scheme("file"));
        ASSERT_TRUE(is_valid_uri_scheme("local-file"));
        ASSERT_TRUE(is_valid_uri_scheme("s3"));
        ASSERT_TRUE(is_valid_uri_scheme("grpc+https"));
        ASSERT_TRUE(is_valid_uri_scheme("file.local"));
        ASSERT_FALSE(is_valid_uri_scheme("3s"));
        ASSERT_FALSE(is_valid_uri_scheme("-file"));
        ASSERT_FALSE(is_valid_uri_scheme("local/file"));
        ASSERT_FALSE(is_valid_uri_scheme("filé"));
    }

    TEST(Uri, Empty) {
        Uri uri;
        ASSERT_EQ(uri.scheme(), "");
    }

    TEST(Uri, ParseSimple) {
        Uri uri;
        {
            // An ephemeral string object shouldn't invalidate results
            std::string s = "https://arrow.apache.org";
            ASSERT_TRUE(uri.parse(s));
            s.replace(0, s.size(), s.size(), 'X');  // replace contents
        }
        ASSERT_EQ(uri.scheme(), "https");
        ASSERT_EQ(uri.host(), "arrow.apache.org");
        ASSERT_EQ(uri.port_text(), "");
    }

    TEST(Uri, ParsePath) {
        // The various edge cases below (leading and trailing slashes) have been
        // checked against several Python URI parsing modules: `uri`, `rfc3986`, `rfc3987`

        Uri uri;

        auto check_case = [&](std::string uri_string, std::string scheme, bool has_host,
                              std::string host, std::string path) -> void {
            ASSERT_TRUE(uri.parse(uri_string));
            ASSERT_EQ(uri.scheme(), scheme);
            ASSERT_EQ(uri.has_host(), has_host);
            ASSERT_EQ(uri.host(), host);
            ASSERT_EQ(uri.path(), path);
        };

        // Relative path
        check_case("unix:tmp/flight.sock", "unix", false, "", "tmp/flight.sock");

        // Absolute path
        check_case("unix:/tmp/flight.sock", "unix", false, "", "/tmp/flight.sock");
        check_case("unix://localhost/tmp/flight.sock", "unix", true, "localhost",
                   "/tmp/flight.sock");
        check_case("unix:///tmp/flight.sock", "unix", true, "", "/tmp/flight.sock");

        // Empty path
        check_case("unix:", "unix", false, "", "");
        check_case("unix://localhost", "unix", true, "localhost", "");

        // With trailing slash
        check_case("unix:/", "unix", false, "", "/");
        check_case("unix:tmp/", "unix", false, "", "tmp/");
        check_case("unix://localhost/", "unix", true, "localhost", "/");
        check_case("unix:/tmp/flight/", "unix", false, "", "/tmp/flight/");
        check_case("unix://localhost/tmp/flight/", "unix", true, "localhost", "/tmp/flight/");
        check_case("unix:///tmp/flight/", "unix", true, "", "/tmp/flight/");

// With query string
        check_case("unix:?", "unix", false, "", "");
        check_case("unix:?foo", "unix", false, "", "");
        check_case("unix:?foo=bar", "unix", false, "", "");
        check_case("unix:/?", "unix", false, "", "/");
        check_case("unix:/?foo", "unix", false, "", "/");
        check_case("unix:/?foo=bar", "unix", false, "", "/");
        check_case("unix://localhost/tmp?", "unix", true, "localhost", "/tmp");
        check_case("unix://localhost/tmp?foo", "unix", true, "localhost", "/tmp");
        check_case("unix://localhost/tmp?foo=bar", "unix", true, "localhost", "/tmp");

// With escaped path characters
        check_case("unix://localhost/tmp/some%20path/100%25%20%C3%A9l%C3%A9phant", "unix", true,
                   "localhost", "/tmp/some path/100% éléphant");
    }

    TEST(Uri, ParseQuery) {
        Uri uri;

        auto check_case = [&](std::string uri_string, std::string query_string,
                              std::vector<std::pair<std::string, std::string>> items) -> void {
            ASSERT_TRUE(uri.parse(uri_string));
            ASSERT_EQ(uri.query_string(), query_string);
            std::vector<std::pair<std::string, std::string>> ret;
            auto result = uri.query_items(&ret);
            ASSERT_TRUE(result);
            ASSERT_EQ(ret, items);
        };

        check_case("unix://localhost/tmp", "", {});
        check_case("unix://localhost/tmp?", "", {});
        check_case("unix://localhost/tmp?foo=bar", "foo=bar", {{"foo", "bar"}});
        check_case("unix:?foo=bar", "foo=bar", {{"foo", "bar"}});
        check_case("unix:?a=b&c=d", "a=b&c=d", {{"a", "b"},
                                                {"c", "d"}});

// With escaped values
        check_case("unix:?a=some+value&b=c", "a=some+value&b=c",
                   {{"a", "some value"},
                    {"b", "c"}});
        check_case("unix:?a=some%20value%2Fanother&b=c", "a=some%20value%2Fanother&b=c",
                   {{"a", "some value/another"},
                    {"b", "c"}});
    }

    TEST(Uri, ParseHostPort) {
        Uri uri;

        ASSERT_TRUE(uri.parse("http://localhost:80"));
        ASSERT_EQ(uri.scheme(), "http");
        ASSERT_EQ(uri.host(), "localhost");
        ASSERT_EQ(uri.port_text(), "80");
        ASSERT_EQ(uri.port(), 80);
        ASSERT_EQ(uri.username(), "");
        ASSERT_EQ(uri.password(), "");

        ASSERT_TRUE(uri.parse("http://1.2.3.4"));
        ASSERT_EQ(uri.scheme(), "http");
        ASSERT_EQ(uri.host(), "1.2.3.4");
        ASSERT_EQ(uri.port_text(), "");
        ASSERT_EQ(uri.port(), -1);
        ASSERT_EQ(uri.username(), "");
        ASSERT_EQ(uri.password(), "");

        ASSERT_TRUE(uri.parse("http://1.2.3.4:"));
        ASSERT_EQ(uri.scheme(), "http");
        ASSERT_EQ(uri.host(), "1.2.3.4");
        ASSERT_EQ(uri.port_text(), "");
        ASSERT_EQ(uri.port(), -1);
        ASSERT_EQ(uri.username(), "");
        ASSERT_EQ(uri.password(), "");

        ASSERT_TRUE(uri.parse("http://1.2.3.4:80"));
        ASSERT_EQ(uri.scheme(), "http");
        ASSERT_EQ(uri.host(), "1.2.3.4");
        ASSERT_EQ(uri.port_text(), "80");
        ASSERT_EQ(uri.port(), 80);
        ASSERT_EQ(uri.username(), "");
        ASSERT_EQ(uri.password(), "");

        ASSERT_TRUE(uri.parse("http://[::1]"));
        ASSERT_EQ(uri.scheme(), "http");
        ASSERT_EQ(uri.host(), "::1");
        ASSERT_EQ(uri.port_text(), "");
        ASSERT_EQ(uri.port(), -1);
        ASSERT_EQ(uri.username(), "");
        ASSERT_EQ(uri.password(), "");

        ASSERT_TRUE(uri.parse("http://[::1]:"));
        ASSERT_EQ(uri.scheme(), "http");
        ASSERT_EQ(uri.host(), "::1");
        ASSERT_EQ(uri.port_text(), "");
        ASSERT_EQ(uri.port(), -1);
        ASSERT_EQ(uri.username(), "");
        ASSERT_EQ(uri.password(), "");

        ASSERT_TRUE(uri.parse("http://[::1]:80"));
        ASSERT_EQ(uri.scheme(), "http");
        ASSERT_EQ(uri.host(), "::1");
        ASSERT_EQ(uri.port_text(), "80");
        ASSERT_EQ(uri.port(), 80);
        ASSERT_EQ(uri.username(), "");
        ASSERT_EQ(uri.password(), "");
    }

    TEST(Uri, ParseUserPass) {
        Uri uri;

        ASSERT_TRUE(uri.parse("http://someuser@localhost:80"));
        ASSERT_EQ(uri.scheme(), "http");
        ASSERT_EQ(uri.host(), "localhost");
        ASSERT_EQ(uri.username(), "someuser");
        ASSERT_EQ(uri.password(), "");

        ASSERT_TRUE(uri.parse("http://someuser:@localhost:80"));
        ASSERT_EQ(uri.scheme(), "http");
        ASSERT_EQ(uri.host(), "localhost");
        ASSERT_EQ(uri.username(), "someuser");
        ASSERT_EQ(uri.password(), "");

        ASSERT_TRUE(uri.parse("http://someuser:somepass@localhost:80"));
        ASSERT_EQ(uri.scheme(), "http");
        ASSERT_EQ(uri.host(), "localhost");
        ASSERT_EQ(uri.username(), "someuser");
        ASSERT_EQ(uri.password(), "somepass");

        ASSERT_TRUE(uri.parse("http://someuser:somepass@localhost"));
        ASSERT_EQ(uri.scheme(), "http");
        ASSERT_EQ(uri.host(), "localhost");
        ASSERT_EQ(uri.username(), "someuser");
        ASSERT_EQ(uri.password(), "somepass");

// With %-encoding
        ASSERT_TRUE(uri.parse("http://some%20user%2Fname:somepass@localhost"));
        ASSERT_EQ(uri.scheme(), "http");
        ASSERT_EQ(uri.host(), "localhost");
        ASSERT_EQ(uri.username(), "some user/name");
        ASSERT_EQ(uri.password(), "somepass");

        ASSERT_TRUE(uri.parse("http://some%20user%2Fname:some%20pass%2Fword@localhost"));
        ASSERT_EQ(uri.scheme(), "http");
        ASSERT_EQ(uri.host(), "localhost");
        ASSERT_EQ(uri.username(), "some user/name");
        ASSERT_EQ(uri.password(), "some pass/word");
    }

    TEST(Uri, FileScheme) {
// "file" scheme URIs
// https://en.wikipedia.org/wiki/File_URI_scheme
// https://tools.ietf.org/html/rfc8089
        Uri uri;

        auto check_file_no_host = [&](std::string uri_string, std::string path) -> void {
            ASSERT_TRUE(uri.parse(uri_string));
            ASSERT_TRUE(uri.is_file_scheme());
            ASSERT_EQ(uri.scheme(), "file");
            ASSERT_EQ(uri.host(), "");
            ASSERT_EQ(uri.path(), path);
            ASSERT_EQ(uri.username(), "");
            ASSERT_EQ(uri.password(), "");
        };

        auto check_notfile_no_host = [&](std::string uri_string, std::string path) -> void {
            ASSERT_TRUE(uri.parse(uri_string));
            ASSERT_FALSE(uri.is_file_scheme());
            ASSERT_NE(uri.scheme(), "file");
            ASSERT_EQ(uri.host(), "");
            ASSERT_EQ(uri.path(), path);
            ASSERT_EQ(uri.username(), "");
            ASSERT_EQ(uri.password(), "");
        };

        auto check_file_with_host = [&](std::string uri_string, std::string host,
                                        std::string path) -> void {
            ASSERT_TRUE(uri.parse(uri_string));
            ASSERT_TRUE(uri.is_file_scheme());
            ASSERT_EQ(uri.scheme(), "file");
            ASSERT_EQ(uri.host(), host);
            ASSERT_EQ(uri.path(), path);
            ASSERT_EQ(uri.username(), "");
            ASSERT_EQ(uri.password(), "");
        };

        // Relative paths are not accepted in "file" URIs.
        ASSERT_FALSE(uri.parse("file:"));
        ASSERT_FALSE(uri.parse("file:foo/bar"));

        // Absolute paths
        // (no authority)
        check_file_no_host("file:/", "/");
        check_file_no_host("file:/foo1/bar", "/foo1/bar");
        // (empty authority)
        check_file_no_host("file:///", "/");
        check_file_no_host("file:///foo2/bar", "/foo2/bar");
        // (not file scheme)
        check_notfile_no_host("s3:/", "/");
        check_notfile_no_host("s3:///foo3/bar", "/foo3/bar");
        // (non-empty authority)
        check_file_with_host("file://localhost/", "localhost", "/");
        check_file_with_host("file://localhost/foo/bar", "localhost", "/foo/bar");
        check_file_with_host("file://hostname.com/", "hostname.com", "/");
        check_file_with_host("file://hostname.com/foo/bar", "hostname.com", "/foo/bar");
// (authority with special chars, not 100% sure this is the right behavior)
        check_file_with_host("file://some%20host/foo/bar", "some host", "/foo/bar");

#ifdef _WIN32
        // Relative paths
          ASSERT_FALSE(uri.parse("file:/C:foo/bar"));
          // (NOTE: "file:/C:" is currently parsed as an absolute URI pointing to "C:/")

          // Absolute paths
          // (no authority)
          check_file_no_host("file:/C:/", "C:/");
          check_file_no_host("file:/C:/foo/bar", "C:/foo/bar");
          // (empty authority)
          check_file_no_host("file:///D:/", "D:/");
          check_file_no_host("file:///D:/foo/bar", "D:/foo/bar");
          // (not file scheme; so slash is prepended)
          check_notfile_no_host("hive:///E:/", "/E:/");
          check_notfile_no_host("hive:/E:/foo/bar", "/E:/foo/bar");
          // (non-empty authority)
          check_file_with_host("file://server/share/", "server", "/share/");
          check_file_with_host("file://server/share/foo/bar", "server", "/share/foo/bar");
#endif
    }

    TEST(Uri, ParseError) {
        Uri uri;

        ASSERT_FALSE(uri.parse("http://a:b:c:d"));
        ASSERT_FALSE(uri.parse("http://localhost:z"));
        ASSERT_FALSE(uri.parse("http://localhost:-1"));
        ASSERT_FALSE(uri.parse("http://localhost:99999"));

// Scheme-less URIs (forbidden by RFC 3986, and ambiguous to parse)
        ASSERT_FALSE(uri.parse("localhost"));
        ASSERT_FALSE(uri.parse("/foo/bar"));
        ASSERT_FALSE(uri.parse("foo/bar"));
        ASSERT_FALSE(uri.parse(""));
    }

    TEST(uri_from_absolute_path, Basics) {
#ifdef _WIN32
        std::string ret;
        ASSERT_TRUE(uri_from_absolute_path("C:\\foo\\bar", &ret));
        ASSERT_EQ("file:///C:/foo/bar", ret);
        ASSERT_TRUE(uri_from_absolute_path("C:/foo/bar", &ret));
        ASSERT_EQ("file:///C:/foo/bar", ret);
        ASSERT_TRUE(uri_from_absolute_path("C:/some path/100% éléphant", &ret));
        ASSERT_EQ("file:///C:/some%20path/100%25%20%C3%A9l%C3%A9phant", ret);
        ASSERT_TRUE(uri_from_absolute_path("\\\\some\\share\\foo\\bar", &ret));
        ASSERT_EQ("file://some/share/foo/bar", ret);
        ASSERT_TRUE(uri_from_absolute_path("//some/share/foo/bar", &ret));
        ASSERT_EQ("file://some/share/foo/bar", ret);
        ASSERT_TRUE(uri_from_absolute_path("//some share/some path/100% éléphant", &ret));
        ASSERT_EQ("file://some%20share/some%20path/100%25%20%C3%A9l%C3%A9phant", ret););
#else
        std::string ret;
        ASSERT_TRUE(uri_from_absolute_path("/", &ret));
        ASSERT_EQ("file:///", ret);
        ASSERT_TRUE(uri_from_absolute_path("/tmp/foo/bar", &ret));
        ASSERT_EQ("file:///tmp/foo/bar", ret);
        ASSERT_TRUE(uri_from_absolute_path("/some path/100% éléphant", &ret));
        ASSERT_EQ("file:///some%20path/100%25%20%C3%A9l%C3%A9phant", ret);
#endif
    }

}  // namespace turbo
