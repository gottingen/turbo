
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <testing/filesystem_test_util.h>
#include "testing/gtest_wrap.h"

TEST(directory, entry) {
    TemporaryDirectory t;
    std::error_code ec;
    auto de = flare::directory_entry(t.path());
    EXPECT_TRUE(de.file_path() == t.path());
    EXPECT_TRUE((flare::file_path) de == t.path());
    EXPECT_TRUE(de.exists());
    EXPECT_TRUE(!de.is_block_file());
    EXPECT_TRUE(!de.is_character_file());
    EXPECT_TRUE(de.is_directory());
    EXPECT_TRUE(!de.is_fifo());
    EXPECT_TRUE(!de.is_other());
    EXPECT_TRUE(!de.is_regular_file());
    EXPECT_TRUE(!de.is_socket());
    EXPECT_TRUE(!de.is_symlink());
    EXPECT_TRUE(de.status().type() == flare::file_type::directory);
    ec.clear();
    EXPECT_TRUE(de.status(ec).type() == flare::file_type::directory);
    EXPECT_TRUE(!ec);
    EXPECT_NO_THROW(de.refresh());
    flare::directory_entry none;
    EXPECT_THROW(none.refresh(), flare::filesystem_error);
    ec.clear();
    EXPECT_NO_THROW(none.refresh(ec));
    EXPECT_TRUE(ec);
    EXPECT_THROW(de.assign(""), flare::filesystem_error);
    ec.clear();
    EXPECT_NO_THROW(de.assign("", ec));
    EXPECT_TRUE(ec);
    generateFile(t.path() / "foo", 1234);
    auto now = flare::file_time_type::clock::now();
    EXPECT_NO_THROW(de.assign(t.path() / "foo"));
    EXPECT_NO_THROW(de.assign(t.path() / "foo", ec));
    EXPECT_TRUE(!ec);
    de = flare::directory_entry(t.path() / "foo");
    EXPECT_TRUE(de.file_path() == t.path() / "foo");
    EXPECT_TRUE(de.exists());
    EXPECT_TRUE(de.exists(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_block_file());
    EXPECT_TRUE(!de.is_block_file(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_character_file());
    EXPECT_TRUE(!de.is_character_file(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_directory());
    EXPECT_TRUE(!de.is_directory(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_fifo());
    EXPECT_TRUE(!de.is_fifo(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_other());
    EXPECT_TRUE(!de.is_other(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(de.is_regular_file());
    EXPECT_TRUE(de.is_regular_file(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_socket());
    EXPECT_TRUE(!de.is_socket(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_symlink());
    EXPECT_TRUE(!de.is_symlink(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(de.file_size() == 1234);
    EXPECT_TRUE(de.file_size(ec) == 1234);
    EXPECT_TRUE(std::abs(std::chrono::duration_cast<std::chrono::seconds>(de.last_write_time() - now).count()) < 3);
    ec.clear();
    EXPECT_TRUE(std::abs(std::chrono::duration_cast<std::chrono::seconds>(de.last_write_time(ec) - now).count()) < 3);
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(de.hard_link_count() == 1);
    EXPECT_TRUE(de.hard_link_count(ec) == 1);
    EXPECT_TRUE(!ec);
    EXPECT_THROW(de.replace_filename("bar"), flare::filesystem_error);
    EXPECT_NO_THROW(de.replace_filename("foo"));
    ec.clear();
    EXPECT_NO_THROW(de.replace_filename("bar", ec));
    EXPECT_TRUE(ec);
    auto de2none = flare::directory_entry();
    ec.clear();
    EXPECT_TRUE(de2none.hard_link_count(ec) == static_cast<uintmax_t>(-1));
    EXPECT_THROW(de2none.hard_link_count(), flare::filesystem_error);
    EXPECT_TRUE(ec);
    ec.clear();
    EXPECT_NO_THROW(de2none.last_write_time(ec));
    EXPECT_THROW(de2none.last_write_time(), flare::filesystem_error);
    EXPECT_TRUE(ec);
    ec.clear();
    EXPECT_THROW(de2none.file_size(), flare::filesystem_error);
    EXPECT_TRUE(de2none.file_size(ec) == static_cast<uintmax_t>(-1));
    EXPECT_TRUE(ec);
    ec.clear();
    EXPECT_TRUE(de2none.status().type() == flare::file_type::not_found);
    EXPECT_TRUE(de2none.status(ec).type() == flare::file_type::not_found);
    EXPECT_TRUE(ec);
    generateFile(t.path() / "a");
    generateFile(t.path() / "b");
    auto d1 = flare::directory_entry(t.path() / "a");
    auto d2 = flare::directory_entry(t.path() / "b");
    EXPECT_TRUE(d1 < d2);
    EXPECT_TRUE(!(d2 < d1));
    EXPECT_TRUE(d1 <= d2);
    EXPECT_TRUE(!(d2 <= d1));
    EXPECT_TRUE(d2 > d1);
    EXPECT_TRUE(!(d1 > d2));
    EXPECT_TRUE(d2 >= d1);
    EXPECT_TRUE(!(d1 >= d2));
    EXPECT_TRUE(d1 != d2);
    EXPECT_TRUE(!(d2 != d2));
    EXPECT_TRUE(d1 == d1);
    EXPECT_TRUE(!(d1 == d2));
}

TEST(directory, iterator) {
    {
        TemporaryDirectory t;
        EXPECT_TRUE(flare::directory_iterator(t.path()) == flare::directory_iterator());
        generateFile(t.path() / "test", 1234);
        EXPECT_TRUE(flare::directory_iterator(t.path()) != flare::directory_iterator());
        auto iter = flare::directory_iterator(t.path());
        flare::directory_iterator iter2(iter);
        flare::directory_iterator iter3, iter4;
        iter3 = iter;
        EXPECT_TRUE(iter->file_path().filename() == "test");
        EXPECT_TRUE(iter2->file_path().filename() == "test");
        EXPECT_TRUE(iter3->file_path().filename() == "test");
        iter4 = std::move(iter3);
        EXPECT_TRUE(iter4->file_path().filename() == "test");
        EXPECT_TRUE(iter->file_path() == t.path() / "test");
        EXPECT_TRUE(!iter->is_symlink());
        EXPECT_TRUE(iter->is_regular_file());
        EXPECT_TRUE(!iter->is_directory());
        EXPECT_TRUE(iter->file_size() == 1234);
        EXPECT_TRUE(++iter == flare::directory_iterator());
        EXPECT_THROW(flare::directory_iterator(t.path() / "non-existing"), flare::filesystem_error);
        int cnt = 0;
        for (auto de : flare::directory_iterator(t.path())) {
            ++cnt;
        }
        EXPECT_TRUE(cnt == 1);
    }
    if (is_symlink_creation_supported()) {
        TemporaryDirectory t;
        flare::file_path td = t.path() / "testdir";
        EXPECT_TRUE(flare::directory_iterator(t.path()) == flare::directory_iterator());
        generateFile(t.path() / "test", 1234);
        flare::create_directory(td);
        EXPECT_NO_THROW(flare::create_symlink(t.path() / "test", td / "testlink"));
        std::error_code ec;
        EXPECT_TRUE(flare::directory_iterator(td) != flare::directory_iterator());
        auto iter = flare::directory_iterator(td);
        EXPECT_TRUE(iter->file_path().filename() == "testlink");
        EXPECT_TRUE(iter->file_path() == td / "testlink");
        EXPECT_TRUE(iter->is_symlink());
        EXPECT_TRUE(iter->is_regular_file());
        EXPECT_TRUE(!iter->is_directory());
        EXPECT_TRUE(iter->file_size() == 1234);
        EXPECT_TRUE(++iter == flare::directory_iterator());
    }
    {
// Issue #8: check if resources are freed when iterator reaches end()
        TemporaryDirectory t(TempOpt::change_path);
        auto p = flare::file_path("test/");
        flare::create_directory(p);
        auto iter = flare::directory_iterator(p);
        while (iter != flare::directory_iterator()) {
            ++iter;
        }
        EXPECT_TRUE(flare::remove_all(p) == 1);
        EXPECT_NO_THROW(flare::create_directory(p));
    }
}

TEST(directory, riterator) {
    {
        auto iter = flare::recursive_directory_iterator(".");
        iter.pop();
        EXPECT_TRUE(iter == flare::recursive_directory_iterator());
    }
    {
        TemporaryDirectory t;
        EXPECT_TRUE(flare::recursive_directory_iterator(t.path()) == flare::recursive_directory_iterator());
        generateFile(t.path() / "test", 1234);
        EXPECT_TRUE(flare::recursive_directory_iterator(t.path()) != flare::recursive_directory_iterator());
        auto iter = flare::recursive_directory_iterator(t.path());
        EXPECT_TRUE(iter->file_path().filename() == "test");
        EXPECT_TRUE(iter->file_path() == t.path() / "test");
        EXPECT_TRUE(!iter->is_symlink());
        EXPECT_TRUE(iter->is_regular_file());
        EXPECT_TRUE(!iter->is_directory());
        EXPECT_TRUE(iter->file_size() == 1234);
        EXPECT_TRUE(++iter == flare::recursive_directory_iterator());
    }

    {
        TemporaryDirectory t;
        flare::file_path td = t.path() / "testdir";
        flare::create_directories(td);
        generateFile(td / "test", 1234);
        EXPECT_TRUE(flare::recursive_directory_iterator(t.path()) != flare::recursive_directory_iterator());
        auto iter = flare::recursive_directory_iterator(t.path());

        EXPECT_TRUE(iter->file_path().filename() == "testdir");
        EXPECT_TRUE(iter->file_path() == td);
        EXPECT_TRUE(!iter->is_symlink());
        EXPECT_TRUE(!iter->is_regular_file());
        EXPECT_TRUE(iter->is_directory());

        EXPECT_TRUE(++iter != flare::recursive_directory_iterator());

        EXPECT_TRUE(iter->file_path().filename() == "test");
        EXPECT_TRUE(iter->file_path() == td / "test");
        EXPECT_TRUE(!iter->is_symlink());
        EXPECT_TRUE(iter->is_regular_file());
        EXPECT_TRUE(!iter->is_directory());
        EXPECT_TRUE(iter->file_size() == 1234);

        EXPECT_TRUE(++iter == flare::recursive_directory_iterator());
    }
    {
        TemporaryDirectory t;
        std::error_code ec;
        EXPECT_TRUE(flare::recursive_directory_iterator(t.path(), flare::directory_options::none)
                    == flare::recursive_directory_iterator());
        EXPECT_TRUE(flare::recursive_directory_iterator(t.path(), flare::directory_options::none, ec)
                    == flare::recursive_directory_iterator());
        EXPECT_TRUE(!ec);
        EXPECT_TRUE(flare::recursive_directory_iterator(t.path(), ec) == flare::recursive_directory_iterator());
        EXPECT_TRUE(!ec);
        generateFile(t.path() / "test");
        flare::recursive_directory_iterator rd1(t.path());
        EXPECT_TRUE(flare::recursive_directory_iterator(rd1) != flare::recursive_directory_iterator());
        flare::recursive_directory_iterator rd2(t.path());
        EXPECT_TRUE(flare::recursive_directory_iterator(std::move(rd2)) != flare::recursive_directory_iterator());
        flare::recursive_directory_iterator rd3(t.path(), flare::directory_options::skip_permission_denied);
        EXPECT_TRUE(rd3.options() == flare::directory_options::skip_permission_denied);
        flare::recursive_directory_iterator rd4;
        rd4 = std::move(rd3);
        EXPECT_TRUE(rd4 != flare::recursive_directory_iterator());
        EXPECT_NO_THROW(++rd4);
        EXPECT_TRUE(rd4 == flare::recursive_directory_iterator());
        flare::recursive_directory_iterator rd5;
        rd5 = rd4;
    }
    {
        TemporaryDirectory t(TempOpt::change_path);
        generateFile("a");
        flare::create_directory("d1");
        flare::create_directory("d1/d2");
        generateFile("d1/b");
        generateFile("d1/c");
        generateFile("d1/d2/d");
        generateFile("e");
        auto iter = flare::recursive_directory_iterator(".");
        std::multimap<std::string, int> result;
        while (iter != flare::recursive_directory_iterator()) {
            result.insert(std::make_pair(iter->file_path().generic_string(), iter.depth()));
            ++iter;
        }
        std::stringstream os;
        for (auto p : result) {
            os << "[" << p.first << "," << p.second << "],";
        }
        EXPECT_TRUE(os.str() == "[./a,0],[./d1,0],[./d1/b,1],[./d1/c,1],[./d1/d2,1],[./d1/d2/d,2],[./e,0],");
    }
    {
        TemporaryDirectory t(TempOpt::change_path);
        generateFile("a");
        flare::create_directory("d1");
        flare::create_directory("d1/d2");
        generateFile("d1/b");
        generateFile("d1/c");
        generateFile("d1/d2/d");
        generateFile("e");
        std::multiset<std::string> result;
        for (auto de : flare::recursive_directory_iterator(".")) {
            result.insert(de.file_path().generic_string());
        }
        std::stringstream os;
        for (auto p : result) {
            os << p << ",";
        }
        EXPECT_TRUE(os.str() == "./a,./d1,./d1/b,./d1/c,./d1/d2,./d1/d2/d,./e,");
    }
    {
        TemporaryDirectory t(TempOpt::change_path);
        generateFile("a");
        flare::create_directory("d1");
        flare::create_directory("d1/d2");
        generateFile("d1/d2/b");
        generateFile("e");
        auto iter = flare::recursive_directory_iterator(".");
        std::multimap<std::string, int> result;
        while (iter != flare::recursive_directory_iterator()) {
            result.insert(std::make_pair(iter->file_path().generic_string(), iter.depth()));
            if (iter->file_path() == "./d1/d2") {
                iter.disable_recursion_pending();
            }
            ++iter;
        }
        std::stringstream os;
        for (auto p : result) {
            os << "[" << p.first << "," << p.second << "],";
        }
        EXPECT_TRUE(os.str() == "[./a,0],[./d1,0],[./d1/d2,1],[./e,0],");
    }
    {
        TemporaryDirectory t(TempOpt::change_path);
        generateFile("a");
        flare::create_directory("d1");
        flare::create_directory("d1/d2");
        generateFile("d1/d2/b");
        generateFile("e");
        auto iter = flare::recursive_directory_iterator(".");
        std::multimap<std::string, int> result;
        while (iter != flare::recursive_directory_iterator()) {
            result.insert(std::make_pair(iter->file_path().generic_string(), iter.depth()));
            if (iter->file_path() == "./d1/d2") {
                iter.pop();
            } else {
                ++iter;
            }
        }
        std::stringstream os;
        for (auto p : result) {
            os << "[" << p.first << "," << p.second << "],";
        }
        EXPECT_TRUE(os.str() == "[./a,0],[./d1,0],[./d1/d2,1],[./e,0],");
    }
}

TEST(directory, absolute) {
    EXPECT_TRUE(flare::absolute("") == flare::current_path() / "");
    EXPECT_TRUE(flare::absolute(flare::current_path()) == flare::current_path());
    EXPECT_TRUE(flare::absolute(".") == flare::current_path() / ".");
    EXPECT_TRUE((flare::absolute("..") == flare::current_path().parent_path()
                 || flare::absolute("..") == flare::current_path() / ".."));
    EXPECT_TRUE(flare::absolute("foo") == flare::current_path() / "foo");
    std::error_code ec;
    EXPECT_TRUE(flare::absolute("", ec) == flare::current_path() / "");
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(flare::absolute("foo", ec) == flare::current_path() / "foo");
    EXPECT_TRUE(!ec);
}
