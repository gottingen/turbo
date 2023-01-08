
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <testing/filesystem_test_util.h>
#include "testing/gtest_wrap.h"

TEST(path_preferred_separator, fs_path_generic) {
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path::preferred_separator == '\\');
#else
    EXPECT_TRUE(flare::file_path::preferred_separator
                == '/');
#endif
}

#ifndef FLARE_PLATFORM_WINDOWS
TEST(path, has_root_name) {
    if (!has_host_root_name_support()) {
        EXPECT_TRUE(
                "This implementation doesn't support path(\"//host\").has_root_name() == true [C++17 30.12.8.1 par. 4] on this platform, tests based on this are skipped. (Should be okay.)");
    }
}

#endif

TEST(path, ctor) {
    EXPECT_TRUE("/usr/local/bin" == flare::file_path("/usr/local/bin").generic_string());
    std::string str = "/usr/local/bin";
    std::u16string u16str = u"/usr/local/bin";
    std::u32string u32str = U"/usr/local/bin";
    EXPECT_TRUE(str == flare::file_path(str, flare::file_path::format::generic_format));
    EXPECT_TRUE(str == flare::file_path(str.begin(), str.end()));
    EXPECT_TRUE(flare::file_path(std::wstring(3, 67)) == "CCC");
    EXPECT_TRUE(str == flare::file_path(u16str.begin(), u16str.end()));
    EXPECT_TRUE(str == flare::file_path(u32str.begin(), u32str.end()));
#ifdef FLARE_FILESYSTEM_VERSION
    EXPECT_TRUE(flare::file_path("///foo/bar") == "/foo/bar");
    EXPECT_TRUE(flare::file_path("//foo//bar") == "//foo/bar");
#endif
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE("\\usr\\local\\bin" == flare::file_path("/usr/local/bin"));
        EXPECT_TRUE("C:\\usr\\local\\bin" == flare::file_path("C:\\usr\\local\\bin"));
#else
    EXPECT_TRUE("/usr/local/bin" == flare::file_path("/usr/local/bin"));
#endif
    if (has_host_root_name_support()) {
        EXPECT_TRUE("//host/foo/bar" == flare::file_path("//host/foo/bar"));
    }

#if !defined(FLARE_PLATFORM_WINDOWS) && !(defined(GCC_VERSION) && GCC_VERSION < 80100) && !defined(USE_STD_FS)
    std::locale loc;
    bool testUTF8Locale = false;
    try {
        if (const char *lang = std::getenv("LANG")) {
            loc = std::locale(lang);
        } else {
            loc = std::locale("en_US.UTF-8");
        }
        std::string name = loc.name();
        if (name.length() > 5
            && (name.substr(name.length() - 5) == "UTF-8" || name.substr(name.length() - 5) == "utf-8")) {
            testUTF8Locale = true;
        }
    }
    catch (std::runtime_error &) {
        EXPECT_TRUE("Couldn't create an UTF-8 locale!");
    }
    if (testUTF8Locale) {
        EXPECT_TRUE("/usr/local/bin" == flare::file_path("/usr/local/bin", loc));
        EXPECT_TRUE(str == flare::file_path(str.begin(), str.end(), loc));
        EXPECT_TRUE(str == flare::file_path(u16str.begin(), u16str.end(), loc));
        EXPECT_TRUE(str == flare::file_path(u32str.begin(), u32str.end(), loc));
    }
#endif
}

TEST(path, assign) {
    flare::file_path p1{"/foo/bar"};
    flare::file_path p2{"/usr/local"};
    flare::file_path p3;
    p3 = p1;
    EXPECT_TRUE(p1 == p3);
    p3 = flare::file_path{"/usr/local"};
    EXPECT_TRUE(p2 == p3);
#if defined(IS_WCHAR_PATH) || defined(FLARE_FS_USE_WCHAR_T)
    p3 = flare::file_path::string_type{L"/foo/bar"};
        EXPECT_TRUE(p1 == p3);
        p3.assign(flare::file_path::string_type{L"/usr/local"});
        EXPECT_TRUE(p2 == p3);
#else
    p3 = flare::file_path::string_type{"/foo/bar"};
    EXPECT_TRUE(p1 == p3);
    p3.assign(flare::file_path::string_type{"/usr/local"});
    EXPECT_TRUE(p2 == p3);
#endif
    p3 = std::u16string(u"/foo/bar");
    EXPECT_TRUE(p1 == p3);
    p3 = U"/usr/local";
    EXPECT_TRUE(p2 == p3);
    p3.assign(std::u16string(u"/foo/bar"));
    EXPECT_TRUE(p1 == p3);
    std::string s{"/usr/local"};
    p3.assign(s.begin(), s.end());
    EXPECT_TRUE(p2 == p3);
}

TEST(path, append) {
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path("foo") / "c:/bar" == "c:/bar");
        EXPECT_TRUE(flare::file_path("foo") / "c:" == "c:");
        EXPECT_TRUE(flare::file_path("c:") / "" == "c:");
        EXPECT_TRUE(flare::file_path("c:foo") / "/bar" == "c:/bar");
        EXPECT_TRUE(flare::file_path("c:foo") / "c:bar" == "c:foo/bar");
#else
    EXPECT_TRUE(flare::file_path("foo") / "" == "foo/");
    EXPECT_TRUE(flare::file_path("foo") / "/bar" == "/bar");
    EXPECT_TRUE(flare::file_path("/foo") / "/" == "/");
    if (has_host_root_name_support()) {
        EXPECT_TRUE(flare::file_path("//host/foo") / "/bar" == "/bar");
        EXPECT_TRUE(flare::file_path("//host") / "/" == "//host/");
        EXPECT_TRUE(flare::file_path("//host/foo") / "/" == "/");
    }
#endif
    EXPECT_TRUE(flare::file_path("/foo/bar") / "some///other" == "/foo/bar/some/other");
    flare::file_path p1{"/tmp/test"};
    flare::file_path p2{"foobar.txt"};
    flare::file_path p3 = p1 / p2;
    EXPECT_TRUE("/tmp/test/foobar.txt" == p3);
// TODO: append(first, last)
}

TEST(path, concat) {
    EXPECT_TRUE((flare::file_path("foo") += flare::file_path("bar")) == "foobar");
    EXPECT_TRUE((flare::file_path("foo") += flare::file_path("/bar")) == "foo/bar");

    EXPECT_TRUE((flare::file_path("foo") += std::string("bar")) == "foobar");
    EXPECT_TRUE((flare::file_path("foo") += std::string("/bar")) == "foo/bar");

    EXPECT_TRUE((flare::file_path("foo") += "bar") == "foobar");
    EXPECT_TRUE((flare::file_path("foo") += "/bar") == "foo/bar");

    EXPECT_TRUE((flare::file_path("foo") += 'b') == "foob");
    EXPECT_TRUE((flare::file_path("foo") += '/') == "foo/");

    EXPECT_TRUE((flare::file_path("foo") += std::string("bar")) == "foobar");
    EXPECT_TRUE((flare::file_path("foo") += std::string("/bar")) == "foo/bar");

    EXPECT_TRUE((flare::file_path("foo") += std::u16string(u"bar")) == "foobar");
    EXPECT_TRUE((flare::file_path("foo") += std::u16string(u"/bar")) == "foo/bar");

    EXPECT_TRUE((flare::file_path("foo") += std::u32string(U"bar")) == "foobar");
    EXPECT_TRUE((flare::file_path("foo") += std::u32string(U"/bar")) == "foo/bar");

    EXPECT_TRUE(flare::file_path("foo").concat("bar") == "foobar");
    EXPECT_TRUE(flare::file_path("foo").concat("/bar") == "foo/bar");
    std::string bar = "bar";
    EXPECT_TRUE(flare::file_path("foo").concat(bar.begin(), bar.end()) == "foobar");
#ifndef USE_STD_FS
    EXPECT_TRUE((flare::file_path("/foo/bar") += "/some///other") == "/foo/bar/some/other");
#endif
// TODO: contat(first, last)
}

TEST(path, modifiers) {
    flare::file_path p = flare::file_path("/foo/bar");
    p.clear();
    EXPECT_TRUE(p == "");

// make_preferred() is a no-op
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path("foo\\bar") == "foo/bar");
        EXPECT_TRUE(flare::file_path("foo\\bar").make_preferred() == "foo/bar");
#else
    EXPECT_TRUE(flare::file_path("foo\\bar") == "foo\\bar");
    EXPECT_TRUE(flare::file_path("foo\\bar").make_preferred() == "foo\\bar");
#endif
    EXPECT_TRUE(flare::file_path("foo/bar").make_preferred() == "foo/bar");

    EXPECT_TRUE(flare::file_path("foo/bar").remove_filename() == "foo/");
    EXPECT_TRUE(flare::file_path("foo/").remove_filename() == "foo/");
    EXPECT_TRUE(flare::file_path("/foo").remove_filename() == "/");
    EXPECT_TRUE(flare::file_path("/").remove_filename() == "/");

    EXPECT_TRUE(flare::file_path("/foo").replace_filename("bar") == "/bar");
    EXPECT_TRUE(flare::file_path("/").replace_filename("bar") == "/bar");
    EXPECT_TRUE(flare::file_path("/foo").replace_filename("b//ar") == "/b/ar");

    EXPECT_TRUE(flare::file_path("/foo/bar.txt").replace_extension("odf") == "/foo/bar.odf");
    EXPECT_TRUE(flare::file_path("/foo/bar.txt").replace_extension() == "/foo/bar");
    EXPECT_TRUE(flare::file_path("/foo/bar").replace_extension("odf") == "/foo/bar.odf");
    EXPECT_TRUE(flare::file_path("/foo/bar").replace_extension(".odf") == "/foo/bar.odf");
    EXPECT_TRUE(flare::file_path("/foo/bar.").replace_extension(".odf") == "/foo/bar.odf");
    EXPECT_TRUE(flare::file_path("/foo/bar/").replace_extension("odf") == "/foo/bar/.odf");

    flare::file_path p1 = "foo";
    flare::file_path p2 = "bar";
    p1.
            swap(p2);
    EXPECT_TRUE(p1 == "bar");
    EXPECT_TRUE(p2 == "foo");
}

TEST(path, observers) {
#ifdef FLARE_PLATFORM_WINDOWS
#if defined(IS_WCHAR_PATH) || defined(FLARE_FS_USE_WCHAR_T)
    EXPECT_TRUE(flare::u8path("\xc3\xa4\\\xe2\x82\xac").native() == flare::file_path::string_type(L"\u00E4\\\u20AC"));
    // EXPECT_TRUE(flare::u8path("\xc3\xa4\\\xe2\x82\xac").string() == std::string("ä\\€")); // MSVCs returns local DBCS encoding
#else
    EXPECT_TRUE(flare::u8path("\xc3\xa4\\\xe2\x82\xac").native() == flare::file_path::string_type("\xc3\xa4\\\xe2\x82\xac"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4\\\xe2\x82\xac").string() == std::string("\xc3\xa4\\\xe2\x82\xac"));
    EXPECT_TRUE(!::strcmp(flare::u8path("\xc3\xa4\\\xe2\x82\xac").c_str(), "\xc3\xa4\\\xe2\x82\xac"));
    EXPECT_TRUE((std::string)flare::u8path("\xc3\xa4\\\xe2\x82\xac") == std::string("\xc3\xa4\\\xe2\x82\xac"));
#endif
    EXPECT_TRUE(flare::u8path("\xc3\xa4\\\xe2\x82\xac").wstring() == std::wstring(L"\u00E4\\\u20AC"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4\\\xe2\x82\xac").u8string() == std::string("\xc3\xa4\\\xe2\x82\xac"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4\\\xe2\x82\xac").u16string() == std::u16string(u"\u00E4\\\u20AC"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4\\\xe2\x82\xac").u32string() == std::u32string(U"\U000000E4\\\U000020AC"));
#else
    EXPECT_TRUE(flare::u8path("\xc3\xa4/\xe2\x82\xac").native() ==
                flare::file_path::string_type(u8"\xc3\xa4/\xe2\x82\xac"));
    EXPECT_TRUE(!::strcmp(flare::u8path("\xc3\xa4/\xe2\x82\xac").c_str(), u8"\xc3\xa4/\xe2\x82\xac"));
    EXPECT_TRUE((std::string) flare::u8path("\xc3\xa4/\xe2\x82\xac") == std::string(u8"\xc3\xa4/\xe2\x82\xac"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4/\xe2\x82\xac").string() == std::string(u8"\xc3\xa4/\xe2\x82\xac"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4/\xe2\x82\xac").wstring() == std::wstring(L"ä/€"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4/\xe2\x82\xac").u8string() == std::string(u8"\xc3\xa4/\xe2\x82\xac"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4/\xe2\x82\xac").u16string() == std::u16string(u"\u00E4/\u20AC"));
    EXPECT_TRUE(
            "This check might fail on GCC8 (with \"Illegal byte sequence\") due to not detecting the valid unicode codepoint U+1D11E.");
    EXPECT_TRUE(
            flare::u8path("\xc3\xa4/\xe2\x82\xac\xf0\x9d\x84\x9e").u16string() ==
            std::u16string(u"\u00E4/\u20AC\U0001D11E"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4/\xe2\x82\xac").u32string() == std::u32string(U"\U000000E4/\U000020AC"));
#endif
}

TEST(path, geobs) {
#ifdef FLARE_PLATFORM_WINDOWS
#ifndef IS_WCHAR_PATH
    EXPECT_TRUE(flare::u8path("\xc3\xa4\\\xe2\x82\xac").generic_string() == std::string("\xc3\xa4/\xe2\x82\xac"));
#endif
#ifndef USE_STD_FS
    auto t = flare::u8path("\xc3\xa4\\\xe2\x82\xac").generic_string<char, std::char_traits<char>, TestAllocator<char>>();
    EXPECT_TRUE(t.c_str() == std::string("\xc3\xa4/\xe2\x82\xac"));
#endif
    EXPECT_TRUE(flare::u8path("\xc3\xa4\\\xe2\x82\xac").generic_wstring() == std::wstring(L"\U000000E4/\U000020AC"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4\\\xe2\x82\xac").generic_u8string() == std::string("\xc3\xa4/\xe2\x82\xac"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4\\\xe2\x82\xac").generic_u16string() == std::u16string(u"\u00E4/\u20AC"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4\\\xe2\x82\xac").generic_u32string() == std::u32string(U"\U000000E4/\U000020AC"));
#else
    EXPECT_TRUE(flare::u8path("\xc3\xa4/\xe2\x82\xac").generic_string() == std::string(u8"\xc3\xa4/\xe2\x82\xac"));
#if !defined(USE_STD_FS)
    auto t = flare::u8path("\xc3\xa4/\xe2\x82\xac").generic_string<char, std::char_traits<char>, TestAllocator<char>>();
    EXPECT_TRUE(t.c_str() == std::string(u8"\xc3\xa4/\xe2\x82\xac"));
#endif
    EXPECT_TRUE(flare::u8path("\xc3\xa4/\xe2\x82\xac").generic_wstring() == std::wstring(L"ä/€"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4/\xe2\x82\xac").generic_u8string() == std::string(u8"\xc3\xa4/\xe2\x82\xac"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4/\xe2\x82\xac").generic_u16string() == std::u16string(u"\u00E4/\u20AC"));
    EXPECT_TRUE(flare::u8path("\xc3\xa4/\xe2\x82\xac").generic_u32string() == std::u32string(U"\U000000E4/\U000020AC"));
#endif
}

TEST(path, compare) {
    EXPECT_TRUE(flare::file_path("/foo/b").compare("/foo/a") > 0);
    EXPECT_TRUE(flare::file_path("/foo/b").compare("/foo/b") == 0);
    EXPECT_TRUE(flare::file_path("/foo/b").compare("/foo/c") < 0);

    EXPECT_TRUE(flare::file_path("/foo/b").compare(std::string("/foo/a")) > 0);
    EXPECT_TRUE(flare::file_path("/foo/b").compare(std::string("/foo/b")) == 0);
    EXPECT_TRUE(flare::file_path("/foo/b").compare(std::string("/foo/c")) < 0);

    EXPECT_TRUE(flare::file_path("/foo/b").compare(flare::file_path("/foo/a")) > 0);
    EXPECT_TRUE(flare::file_path("/foo/b").compare(flare::file_path("/foo/b")) == 0);
    EXPECT_TRUE(flare::file_path("/foo/b").compare(flare::file_path("/foo/c")) < 0);
}

TEST(path, decomposition) {
// root_name()
    EXPECT_TRUE(flare::file_path("").root_name() == "");
    EXPECT_TRUE(flare::file_path(".").root_name() == "");
    EXPECT_TRUE(flare::file_path("..").root_name() == "");
    EXPECT_TRUE(flare::file_path("foo").root_name() == "");
    EXPECT_TRUE(flare::file_path("/").root_name() == "");
    EXPECT_TRUE(flare::file_path("/foo").root_name() == "");
    EXPECT_TRUE(flare::file_path("foo/").root_name() == "");
    EXPECT_TRUE(flare::file_path("/foo/").root_name() == "");
    EXPECT_TRUE(flare::file_path("foo/bar").root_name() == "");
    EXPECT_TRUE(flare::file_path("/foo/bar").root_name() == "");
    EXPECT_TRUE(flare::file_path("///foo/bar").root_name() == "");
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path("C:/foo").root_name() == "C:");
        EXPECT_TRUE(flare::file_path("C:\\foo").root_name() == "C:");
        EXPECT_TRUE(flare::file_path("C:foo").root_name() == "C:");
#endif

// root_directory()
    EXPECT_TRUE(flare::file_path("").root_directory() == "");
    EXPECT_TRUE(flare::file_path(".").root_directory() == "");
    EXPECT_TRUE(flare::file_path("..").root_directory() == "");
    EXPECT_TRUE(flare::file_path("foo").root_directory() == "");
    EXPECT_TRUE(flare::file_path("/").root_directory() == "/");
    EXPECT_TRUE(flare::file_path("/foo").root_directory() == "/");
    EXPECT_TRUE(flare::file_path("foo/").root_directory() == "");
    EXPECT_TRUE(flare::file_path("/foo/").root_directory() == "/");
    EXPECT_TRUE(flare::file_path("foo/bar").root_directory() == "");
    EXPECT_TRUE(flare::file_path("/foo/bar").root_directory() == "/");
    EXPECT_TRUE(flare::file_path("///foo/bar").root_directory() == "/");
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path("C:/foo").root_directory() == "/");
        EXPECT_TRUE(flare::file_path("C:\\foo").root_directory() == "/");
        EXPECT_TRUE(flare::file_path("C:foo").root_directory() == "");
#endif

// root_path()
    EXPECT_TRUE(flare::file_path("").root_path() == "");
    EXPECT_TRUE(flare::file_path(".").root_path() == "");
    EXPECT_TRUE(flare::file_path("..").root_path() == "");
    EXPECT_TRUE(flare::file_path("foo").root_path() == "");
    EXPECT_TRUE(flare::file_path("/").root_path() == "/");
    EXPECT_TRUE(flare::file_path("/foo").root_path() == "/");
    EXPECT_TRUE(flare::file_path("foo/").root_path() == "");
    EXPECT_TRUE(flare::file_path("/foo/").root_path() == "/");
    EXPECT_TRUE(flare::file_path("foo/bar").root_path() == "");
    EXPECT_TRUE(flare::file_path("/foo/bar").root_path() == "/");
    EXPECT_TRUE(flare::file_path("///foo/bar").root_path() == "/");
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path("C:/foo").root_path() == "C:/");
        EXPECT_TRUE(flare::file_path("C:\\foo").root_path() == "C:/");
        EXPECT_TRUE(flare::file_path("C:foo").root_path() == "C:");
#endif

// relative_path()
    EXPECT_TRUE(flare::file_path("").relative_path() == "");
    EXPECT_TRUE(flare::file_path(".").relative_path() == ".");
    EXPECT_TRUE(flare::file_path("..").relative_path() == "..");
    EXPECT_TRUE(flare::file_path("foo").relative_path() == "foo");
    EXPECT_TRUE(flare::file_path("/").relative_path() == "");
    EXPECT_TRUE(flare::file_path("/foo").relative_path() == "foo");
    EXPECT_TRUE(flare::file_path("foo/").relative_path() == "foo/");
    EXPECT_TRUE(flare::file_path("/foo/").relative_path() == "foo/");
    EXPECT_TRUE(flare::file_path("foo/bar").relative_path() == "foo/bar");
    EXPECT_TRUE(flare::file_path("/foo/bar").relative_path() == "foo/bar");
    EXPECT_TRUE(flare::file_path("///foo/bar").relative_path() == "foo/bar");
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path("C:/foo").relative_path() == "foo");
        EXPECT_TRUE(flare::file_path("C:\\foo").relative_path() == "foo");
        EXPECT_TRUE(flare::file_path("C:foo").relative_path() == "foo");
#endif

// parent_path()
    EXPECT_TRUE(flare::file_path("").parent_path() == "");
    EXPECT_TRUE(flare::file_path(".").parent_path() == "");
    EXPECT_TRUE(flare::file_path("..").parent_path() == "");  // unintuitive but as defined in the standard
    EXPECT_TRUE(flare::file_path("foo").parent_path() == "");
    EXPECT_TRUE(flare::file_path("/").parent_path() == "/");
    EXPECT_TRUE(flare::file_path("/foo").parent_path() == "/");
    EXPECT_TRUE(flare::file_path("foo/").parent_path() == "foo");
    EXPECT_TRUE(flare::file_path("/foo/").parent_path() == "/foo");
    EXPECT_TRUE(flare::file_path("foo/bar").parent_path() == "foo");
    EXPECT_TRUE(flare::file_path("/foo/bar").parent_path() == "/foo");
    EXPECT_TRUE(flare::file_path("///foo/bar").parent_path() == "/foo");
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path("C:/foo").parent_path() == "C:/");
        EXPECT_TRUE(flare::file_path("C:\\foo").parent_path() == "C:/");
        EXPECT_TRUE(flare::file_path("C:foo").parent_path() == "C:");
#endif

// filename()
    EXPECT_TRUE(flare::file_path("").filename() == "");
    EXPECT_TRUE(flare::file_path(".").filename() == ".");
    EXPECT_TRUE(flare::file_path("..").filename() == "..");
    EXPECT_TRUE(flare::file_path("foo").filename() == "foo");
    EXPECT_TRUE(flare::file_path("/").filename() == "");
    EXPECT_TRUE(flare::file_path("/foo").filename() == "foo");
    EXPECT_TRUE(flare::file_path("foo/").filename() == "");
    EXPECT_TRUE(flare::file_path("/foo/").filename() == "");
    EXPECT_TRUE(flare::file_path("foo/bar").filename() == "bar");
    EXPECT_TRUE(flare::file_path("/foo/bar").filename() == "bar");
    EXPECT_TRUE(flare::file_path("///foo/bar").filename() == "bar");
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path("C:/foo").filename() == "foo");
        EXPECT_TRUE(flare::file_path("C:\\foo").filename() == "foo");
        EXPECT_TRUE(flare::file_path("C:foo").filename() == "foo");
#endif

// stem()
    EXPECT_TRUE(flare::file_path("/foo/bar.txt").stem() == "bar");
    {
        flare::file_path p = "foo.bar.baz.tar";
        EXPECT_TRUE(p.extension() == ".tar");
        p = p.stem();
        EXPECT_TRUE(p.extension() == ".baz");
        p = p.stem();
        EXPECT_TRUE(p.extension() == ".bar");
        p = p.stem();
        EXPECT_TRUE(p == "foo");
    }
    EXPECT_TRUE(flare::file_path("/foo/.profile").stem() == ".profile");
    EXPECT_TRUE(flare::file_path(".bar").stem() == ".bar");
    EXPECT_TRUE(flare::file_path("..bar").stem() == ".");

// extension()
    EXPECT_TRUE(flare::file_path("/foo/bar.txt").extension() == ".txt");
    EXPECT_TRUE(flare::file_path("/foo/bar").extension() == "");
    EXPECT_TRUE(flare::file_path("/foo/.profile").extension() == "");
    EXPECT_TRUE(flare::file_path(".bar").extension() == "");
    EXPECT_TRUE(flare::file_path("..bar").extension() == ".bar");

    if (
            has_host_root_name_support()
            ) {
// //host-based root-names
        EXPECT_TRUE(flare::file_path("//host").root_name() == "//host");
        EXPECT_TRUE(flare::file_path("//host/foo").root_name() == "//host");
        EXPECT_TRUE(flare::file_path("//host").root_directory() == "");
        EXPECT_TRUE(flare::file_path("//host/foo").root_directory() == "/");
        EXPECT_TRUE(flare::file_path("//host").root_path() == "//host");
        EXPECT_TRUE(flare::file_path("//host/foo").root_path() == "//host/");
        EXPECT_TRUE(flare::file_path("//host").relative_path() == "");
        EXPECT_TRUE(flare::file_path("//host/foo").relative_path() == "foo");
        EXPECT_TRUE(flare::file_path("//host").parent_path() == "//host");
        EXPECT_TRUE(flare::file_path("//host/foo").parent_path() == "//host/");
        EXPECT_TRUE(flare::file_path("//host").filename() == "");
        EXPECT_TRUE(flare::file_path("//host/foo").filename() == "foo");
    }
}

TEST(path, query) {
// empty
    EXPECT_TRUE(flare::file_path("").empty());
    EXPECT_TRUE(!flare::file_path("foo").empty());

// has_root_path()
    EXPECT_TRUE(!flare::file_path("foo").has_root_path());
    EXPECT_TRUE(!flare::file_path("foo/bar").has_root_path());
    EXPECT_TRUE(flare::file_path("/foo").has_root_path());
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path("C:foo").has_root_path());
        EXPECT_TRUE(flare::file_path("C:/foo").has_root_path());
#endif

// has_root_name()
    EXPECT_TRUE(!flare::file_path("foo").has_root_name());
    EXPECT_TRUE(!flare::file_path("foo/bar").has_root_name());
    EXPECT_TRUE(!flare::file_path("/foo").has_root_name());
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path("C:foo").has_root_name());
        EXPECT_TRUE(flare::file_path("C:/foo").has_root_name());
#endif

// has_root_directory()
    EXPECT_TRUE(!flare::file_path("foo").has_root_directory());
    EXPECT_TRUE(!flare::file_path("foo/bar").has_root_directory());
    EXPECT_TRUE(flare::file_path("/foo").has_root_directory());
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(!flare::file_path("C:foo").has_root_directory());
        EXPECT_TRUE(flare::file_path("C:/foo").has_root_directory());
#endif

// has_relative_path()
    EXPECT_TRUE(!flare::file_path("").has_relative_path());
    EXPECT_TRUE(!flare::file_path("/").has_relative_path());
    EXPECT_TRUE(flare::file_path("/foo").has_relative_path());

// has_parent_path()
    EXPECT_TRUE(!flare::file_path("").has_parent_path());
    EXPECT_TRUE(!flare::file_path(".").has_parent_path());
    EXPECT_TRUE(!flare::file_path("..").has_parent_path());  // unintuitive but as defined in the standard
    EXPECT_TRUE(!flare::file_path("foo").has_parent_path());
    EXPECT_TRUE(flare::file_path("/").has_parent_path());
    EXPECT_TRUE(flare::file_path("/foo").has_parent_path());
    EXPECT_TRUE(flare::file_path("foo/").has_parent_path());
    EXPECT_TRUE(flare::file_path("/foo/").has_parent_path());

// has_filename()
    EXPECT_TRUE(flare::file_path("foo").has_filename());
    EXPECT_TRUE(flare::file_path("foo/bar").has_filename());
    EXPECT_TRUE(!flare::file_path("/foo/bar/").has_filename());

// has_stem()
    EXPECT_TRUE(flare::file_path("foo").has_stem());
    EXPECT_TRUE(flare::file_path("foo.bar").has_stem());
    EXPECT_TRUE(flare::file_path(".profile").has_stem());
    EXPECT_TRUE(!flare::file_path("/foo/").has_stem());

// has_extension()
    EXPECT_TRUE(!flare::file_path("foo").has_extension());
    EXPECT_TRUE(flare::file_path("foo.bar").has_extension());
    EXPECT_TRUE(!flare::file_path(".profile").has_extension());

// is_absolute()
    EXPECT_TRUE(!flare::file_path("foo/bar").is_absolute());
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(!flare::file_path("/foo").is_absolute());
        EXPECT_TRUE(!flare::file_path("c:foo").is_absolute());
        EXPECT_TRUE(flare::file_path("c:/foo").is_absolute());
#else
    EXPECT_TRUE(flare::file_path("/foo").is_absolute());
#endif

// is_relative()
    EXPECT_TRUE(flare::file_path("foo/bar").is_relative());
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path("/foo").is_relative());
        EXPECT_TRUE(flare::file_path("c:foo").is_relative());
        EXPECT_TRUE(!flare::file_path("c:/foo").is_relative());
#else
    EXPECT_TRUE(!flare::file_path("/foo").is_relative());
#endif

    if (has_host_root_name_support()) {
        EXPECT_TRUE(flare::file_path("//host").has_root_name());
        EXPECT_TRUE(flare::file_path("//host/foo").has_root_name());
        EXPECT_TRUE(flare::file_path("//host").has_root_path());
        EXPECT_TRUE(flare::file_path("//host/foo").has_root_path());
        EXPECT_TRUE(!flare::file_path("//host").has_root_directory());
        EXPECT_TRUE(flare::file_path("//host/foo").has_root_directory());
        EXPECT_TRUE(!flare::file_path("//host").has_relative_path());
        EXPECT_TRUE(flare::file_path("//host/foo").has_relative_path());
        EXPECT_TRUE(flare::file_path("//host/foo").is_absolute());
        EXPECT_TRUE(!flare::file_path("//host/foo").is_relative());
    }
}

TEST(path, gen) {
// lexically_normal()
    EXPECT_TRUE(flare::file_path("foo/./bar/..").lexically_normal() == "foo/");
    EXPECT_TRUE(flare::file_path("foo/.///bar/../").lexically_normal() == "foo/");
    EXPECT_TRUE(flare::file_path("/foo/../..").lexically_normal() == "/");
    EXPECT_TRUE(flare::file_path("foo/..").lexically_normal() == ".");
    EXPECT_TRUE(flare::file_path("ab/cd/ef/../../qw").lexically_normal() == "ab/qw");
    EXPECT_TRUE(flare::file_path("a/b/../../../c").lexically_normal() == "../c");
    EXPECT_TRUE(flare::file_path("../").lexically_normal() == "..");
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path("\\/\\///\\/").lexically_normal() == "/");
        EXPECT_TRUE(flare::file_path("a/b/..\\//..///\\/../c\\\\/").lexically_normal() == "../c/");
        EXPECT_TRUE(flare::file_path("..a/b/..\\//..///\\/../c\\\\/").lexically_normal() == "../c/");
        EXPECT_TRUE(flare::file_path("..\\").lexically_normal() == "..");
#endif

// lexically_relative()
    EXPECT_TRUE(flare::file_path("/a/d").lexically_relative("/a/b/c") == "../../d");
    EXPECT_TRUE(flare::file_path("/a/b/c").lexically_relative("/a/d") == "../b/c");
    EXPECT_TRUE(flare::file_path("a/b/c").lexically_relative("a") == "b/c");
    EXPECT_TRUE(flare::file_path("a/b/c").lexically_relative("a/b/c/x/y") == "../..");
    EXPECT_TRUE(flare::file_path("a/b/c").lexically_relative("a/b/c") == ".");
    EXPECT_TRUE(flare::file_path("a/b").lexically_relative("c/d") == "../../a/b");
    if (has_host_root_name_support()) {
        EXPECT_TRUE(flare::file_path("//host1/foo").lexically_relative("//host2.bar") == "");
    }
#ifdef FLARE_PLATFORM_WINDOWS
        EXPECT_TRUE(flare::file_path("c:/foo").lexically_relative("/bar") == "");
            EXPECT_TRUE(flare::file_path("c:foo").lexically_relative("c:/bar") == "");
            EXPECT_TRUE(flare::file_path("foo").lexically_relative("/bar") == "");
#else
    EXPECT_TRUE(flare::file_path("/foo").lexically_relative("bar") == "");
    EXPECT_TRUE(flare::file_path("foo").lexically_relative("/bar") == "");
#endif

// lexically_proximate()
    EXPECT_TRUE(flare::file_path("/a/d").lexically_proximate("/a/b/c") == "../../d");
    if (
            has_host_root_name_support()
            ) {
        EXPECT_TRUE(flare::file_path("//host1/a/d").lexically_proximate("//host2/a/b/c") == "//host1/a/d");
    }
    EXPECT_TRUE(flare::file_path("a/d").lexically_proximate("/a/b/c") == "a/d");
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE(flare::file_path("c:/a/d").lexically_proximate("c:/a/b/c") == "../../d");
        EXPECT_TRUE(flare::file_path("c:/a/d").lexically_proximate("d:/a/b/c") == "c:/a/d");
        EXPECT_TRUE(flare::file_path("c:/foo").lexically_proximate("/bar") == "c:/foo");
        EXPECT_TRUE(flare::file_path("c:foo").lexically_proximate("c:/bar") == "c:foo");
        EXPECT_TRUE(flare::file_path("foo").lexically_proximate("/bar") == "foo");
#else
    EXPECT_TRUE(flare::file_path("/foo").lexically_proximate("bar") == "/foo");
    EXPECT_TRUE(flare::file_path("foo").lexically_proximate("/bar") == "foo");
#endif
}

static std::string iterateResult(const flare::file_path &path) {
    std::ostringstream result;
    for (flare::file_path::const_iterator i = path.begin(); i != path.end(); ++i) {
        if (i != path.begin()) {
            result << ",";
        }
        result << i->generic_string();
    }
    return result.str();
}

static std::string reverseIterateResult(const flare::file_path &path) {
    std::ostringstream result;
    flare::file_path::const_iterator iter = path.end();
    bool first = true;
    if (iter != path.begin()) {
        do {
            --iter;
            if (!first) {
                result << ",";
            }
            first = false;
            result << iter->generic_string();
        } while (iter != path.begin());
    }
    return result.str();
}

TEST(path, iterators) {
    EXPECT_TRUE(iterateResult(flare::file_path()).empty());
    EXPECT_TRUE("." == iterateResult(flare::file_path(".")));
    EXPECT_TRUE(".." == iterateResult(flare::file_path("..")));
    EXPECT_TRUE("foo" == iterateResult(flare::file_path("foo")));
    EXPECT_TRUE("/" == iterateResult(flare::file_path("/")));
    EXPECT_TRUE("/,foo" == iterateResult(flare::file_path("/foo")));
    EXPECT_TRUE("foo," == iterateResult(flare::file_path("foo/")));
    EXPECT_TRUE("/,foo," == iterateResult(flare::file_path("/foo/")));
    EXPECT_TRUE("foo,bar" == iterateResult(flare::file_path("foo/bar")));
    EXPECT_TRUE("/,foo,bar" == iterateResult(flare::file_path("/foo/bar")));
    EXPECT_TRUE("/,foo,bar" == iterateResult(flare::file_path("///foo/bar")));
    EXPECT_TRUE("/,foo,bar," == iterateResult(flare::file_path("/foo/bar///")));
    EXPECT_TRUE("foo,.,bar,..," == iterateResult(flare::file_path("foo/.///bar/../")));
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE("C:,/,foo" == iterateResult(flare::file_path("C:/foo")));
#endif

    EXPECT_TRUE(reverseIterateResult(flare::file_path()).empty());
    EXPECT_TRUE("." == reverseIterateResult(flare::file_path(".")));
    EXPECT_TRUE(".." == reverseIterateResult(flare::file_path("..")));
    EXPECT_TRUE("foo" == reverseIterateResult(flare::file_path("foo")));
    EXPECT_TRUE("/" == reverseIterateResult(flare::file_path("/")));
    EXPECT_TRUE("foo,/" == reverseIterateResult(flare::file_path("/foo")));
    EXPECT_TRUE(",foo" == reverseIterateResult(flare::file_path("foo/")));
    EXPECT_TRUE(",foo,/" == reverseIterateResult(flare::file_path("/foo/")));
    EXPECT_TRUE("bar,foo" == reverseIterateResult(flare::file_path("foo/bar")));
    EXPECT_TRUE("bar,foo,/" == reverseIterateResult(flare::file_path("/foo/bar")));
    EXPECT_TRUE("bar,foo,/" == reverseIterateResult(flare::file_path("///foo/bar")));
    EXPECT_TRUE(",bar,foo,/" == reverseIterateResult(flare::file_path("/foo/bar///")));
    EXPECT_TRUE(",..,bar,.,foo" == reverseIterateResult(flare::file_path("foo/.///bar/../")));
#ifdef FLARE_PLATFORM_WINDOWS
    EXPECT_TRUE("foo,/,C:" == reverseIterateResult(flare::file_path("C:/foo")));
        EXPECT_TRUE("foo,C:" == reverseIterateResult(flare::file_path("C:foo")));
#endif
    {
        flare::file_path p1 = "/foo/bar/test.txt";
        flare::file_path p2;
        for (
            auto pe
                : p1) {
            p2 /=
                    pe;
        }
        EXPECT_TRUE(p1 == p2);
        EXPECT_TRUE("bar" == *(--flare::file_path("/foo/bar").end()));
        auto p = flare::file_path("/foo/bar");
        auto pi = p.end();
        pi--;
        EXPECT_TRUE("bar" == *pi);
    }

    if (
            has_host_root_name_support()
            ) {
        EXPECT_TRUE("foo" == *(--flare::file_path("//host/foo").end()));
        auto p = flare::file_path("//host/foo");
        auto pi = p.end();
        pi--;
        EXPECT_TRUE("foo" == *pi);
        EXPECT_TRUE("//host" == iterateResult(flare::file_path("//host")));
        EXPECT_TRUE("//host,/,foo" == iterateResult(flare::file_path("//host/foo")));
        EXPECT_TRUE("//host" == reverseIterateResult(flare::file_path("//host")));
        EXPECT_TRUE("foo,/,//host" == reverseIterateResult(flare::file_path("//host/foo")));
        {
            flare::file_path p1 = "//host/foo/bar/test.txt";
            flare::file_path p2;
            for (
                auto pe
                    : p1) {
                p2 /=
                        pe;
            }
            EXPECT_TRUE(p1 == p2);
        }
    }
}

TEST(path, nonmember) {
    flare::file_path p1("foo/bar");
    flare::file_path p2("some/other");
    flare::swap(p1, p2
    );
    EXPECT_TRUE(p1 == "some/other");
    EXPECT_TRUE(p2 == "foo/bar");
    EXPECT_TRUE(hash_value(p1));
    EXPECT_TRUE(p2 < p1);
    EXPECT_TRUE(p2 <= p1);
    EXPECT_TRUE(p1 <= p1);
    EXPECT_TRUE(!(p1 < p2));
    EXPECT_TRUE(!(p1 <= p2));
    EXPECT_TRUE(p1 > p2);
    EXPECT_TRUE(p1 >= p2);
    EXPECT_TRUE(p1 >= p1);
    EXPECT_TRUE(!(p2 > p1));
    EXPECT_TRUE(!(p2 >= p1));
    EXPECT_TRUE(p1 != p2);
    EXPECT_TRUE(p1 / p2 == "some/other/foo/bar");
}

TEST(path, extractor) {
    {
        std::ostringstream os;
        os << flare::file_path("/root/foo bar");
#ifdef FLARE_PLATFORM_WINDOWS
        EXPECT_TRUE(os.str() == "\"\\\\root\\\\foo bar\"");
#else
        EXPECT_TRUE(os.str() == "\"/root/foo bar\"");
#endif
    }
    {
        std::ostringstream os;
        os << flare::file_path("/root/foo\"bar");
#ifdef FLARE_PLATFORM_WINDOWS
        EXPECT_TRUE(os.str() == "\"\\\\root\\\\foo\\\"bar\"");
#else
        EXPECT_TRUE(os.str() == "\"/root/foo\\\"bar\"");
#endif
    }

    {
        std::istringstream is("\"/root/foo bar\"");
        flare::file_path p;
        is >>
           p;
        EXPECT_TRUE(p == flare::file_path("/root/foo bar"));
        EXPECT_TRUE((is.flags() & std::ios_base::skipws) == std::ios_base::skipws);
    }
    {
        std::istringstream is("\"/root/foo bar\"");
        is >>
           std::noskipws;
        flare::file_path p;
        is >>
           p;
        EXPECT_TRUE(p == flare::file_path("/root/foo bar"));
        EXPECT_TRUE((is.flags() & std::ios_base::skipws) != std::ios_base::skipws);
    }
    {
        std::istringstream is("\"/root/foo\\\"bar\"");
        flare::file_path p;
        is >>
           p;
        EXPECT_TRUE(p == flare::file_path("/root/foo\"bar"));
    }
    {
        std::istringstream is("/root/foo");
        flare::file_path p;
        is >>
           p;
        EXPECT_TRUE(p == flare::file_path("/root/foo"));
    }
}

TEST(path, factory) {
    EXPECT_TRUE(flare::u8path("foo/bar") == flare::file_path("foo/bar"));
    EXPECT_TRUE(flare::u8path("foo/bar") == flare::file_path("foo/bar"));
    std::string str("/foo/bar/test.txt");
    EXPECT_TRUE(flare::u8path(str.begin(), str.end()) == str);
}
