
#include "testing/gtest_wrap.h"

#include <sys/types.h>
#include <sys/socket.h>                // socketpair
#include <errno.h>                     // errno
#include <fcntl.h>                     // O_RDONLY
#include "turbo/files/temp_file.h"      // temp_file
#include <turbo/container/flat_map.h>
#include "turbo/times/time.h"                 // Timer
#include "turbo/base/fd_utility.h"           // make_non_blocking
#include <turbo/io/cord_buf.h>
#include "turbo/log/logging.h"
#include "turbo/base/fd_guard.h"
#include "turbo/base/errno.h"
#include <turbo/base/fast_rand.h>

#if BAZEL_TEST
#include "test/cord_buf.pb.h"
#else

#include "cord_buf.pb.h"

#endif   // BAZEL_TEST

namespace turbo {
    namespace iobuf {
        extern void *(*blockmem_allocate)(size_t);

        extern void (*blockmem_deallocate)(void *);

        extern void reset_blockmem_allocate_and_deallocate();

        extern int32_t block_shared_count(turbo::cord_buf::Block const *b);

        extern uint32_t block_cap(turbo::cord_buf::Block const *b);

        extern cord_buf::Block *get_tls_block_head();

        extern int get_tls_block_count();

        extern void remove_tls_block_chain();

        extern cord_buf::Block *acquire_tls_block();

        extern cord_buf::Block *share_tls_block();

        extern void release_tls_block_chain(cord_buf::Block *b);

        extern uint32_t block_cap(cord_buf::Block const *b);

        extern uint32_t block_size(cord_buf::Block const *b);

        extern cord_buf::Block *get_portal_next(cord_buf::Block const *b);
    }
}

namespace {

    const size_t BLOCK_OVERHEAD = 32; //impl dependent
    const size_t DEFAULT_PAYLOAD = turbo::cord_buf::DEFAULT_BLOCK_SIZE - BLOCK_OVERHEAD;

    void check_tls_block() {
        ASSERT_EQ((turbo::cord_buf::Block *) nullptr, turbo::iobuf::get_tls_block_head());
        printf("tls_block of turbo::cord_buf was deleted\n");
    }

    const int TURBO_ALLOW_UNUSED check_dummy = turbo::thread::atexit(check_tls_block);

    static turbo::container::FlatSet<void *> s_set;

    void *debug_block_allocate(size_t block_size) {
        void *b = operator new(block_size, std::nothrow);
        s_set.insert(b);
        return b;
    }

    void debug_block_deallocate(void *b) {
        if (1ul != s_set.erase(b)) {
            ASSERT_TRUE(false) << "Bad block=" << b;
        } else {
            operator delete(b);
        }
    }

    inline bool is_debug_allocator_enabled() {
        return (turbo::iobuf::blockmem_allocate == debug_block_allocate);
    }

    void install_debug_allocator() {
        if (!is_debug_allocator_enabled()) {
            turbo::iobuf::remove_tls_block_chain();
            s_set.init(1024);
            turbo::iobuf::blockmem_allocate = debug_block_allocate;
            turbo::iobuf::blockmem_deallocate = debug_block_deallocate;
            TURBO_LOG(INFO) << "<Installed debug create/destroy>";
        }
    }

    void show_prof_and_rm(const char *bin_name, const char *filename, size_t topn) {
        char cmd[1024];
        if (topn != 0) {
            snprintf(cmd, sizeof(cmd),
                     "if [ -e %s ] ; then CPUPROFILE_FREQUENCY=1000 ./pprof --text %s %s | head -%lu; rm -f %s; fi",
                     filename, bin_name, filename, topn + 1, filename);
        } else {
            snprintf(cmd, sizeof(cmd),
                     "if [ -e %s ] ; then CPUPROFILE_FREQUENCY=1000 ./pprof --text %s %s; rm -f %s; fi", filename,
                     bin_name, filename, filename);
        }
        ASSERT_EQ(0, system(cmd));
    }

    static void check_memory_leak() {
        if (is_debug_allocator_enabled()) {
            turbo::cord_buf::Block *p = turbo::iobuf::get_tls_block_head();
            size_t n = 0;
            while (p) {
                ASSERT_TRUE(s_set.seek(p)) << "Memory leak: " << p;
                p = turbo::iobuf::get_portal_next(p);
                ++n;
            }
            ASSERT_EQ(n, s_set.size());
            ASSERT_EQ(n, (size_t) turbo::iobuf::get_tls_block_count());
        }
    }

    class CordBufTest : public ::testing::Test {
    protected:
        CordBufTest() {};

        virtual ~CordBufTest() {};

        virtual void SetUp() {
        };

        virtual void TearDown() {
            check_memory_leak();
        };
    };

    std::string to_str(const turbo::cord_buf &p) {
        return p.to_string();
    }

    TEST_F(CordBufTest, append_zero) {
        int fds[2];
        ASSERT_EQ(0, pipe(fds));
        turbo::IOPortal p;
        ASSERT_EQ(0, p.append_from_file_descriptor(fds[0], 0));
        ASSERT_EQ(0, close(fds[0]));
        ASSERT_EQ(0, close(fds[1]));
    }

    TEST_F(CordBufTest, pop_front) {
        install_debug_allocator();

        turbo::cord_buf buf;
        ASSERT_EQ(0UL, buf.pop_front(1));   // nothing happened

        std::string s = "hello";
        buf.append(s);
        ASSERT_EQ(s, to_str(buf));
        ASSERT_EQ(0UL, buf.pop_front(0));   // nothing happened
        ASSERT_EQ(s, to_str(buf));

        ASSERT_EQ(1UL, buf.pop_front(1));
        s.erase(0, 1);
        ASSERT_EQ(s, to_str(buf));
        ASSERT_EQ(s.length(), buf.length());
        ASSERT_FALSE(buf.empty());

        ASSERT_EQ(s.length(), buf.pop_front(INT_MAX));
        s.clear();
        ASSERT_EQ(s, to_str(buf));
        ASSERT_EQ(0UL, buf.length());
        ASSERT_TRUE(buf.empty());

        for (size_t i = 0; i < DEFAULT_PAYLOAD * 3 / 2; ++i) {
            s.push_back(i);
        }
        buf.append(s);
        ASSERT_EQ(1UL, buf.pop_front(1));
        s.erase(0, 1);
        ASSERT_EQ(s, to_str(buf));
        ASSERT_EQ(s.length(), buf.length());
        ASSERT_FALSE(buf.empty());

        ASSERT_EQ(s.length(), buf.pop_front(INT_MAX));
        s.clear();
        ASSERT_EQ(s, to_str(buf));
        ASSERT_EQ(0UL, buf.length());
        ASSERT_TRUE(buf.empty());
    }

    TEST_F(CordBufTest, pop_back) {
        install_debug_allocator();

        turbo::cord_buf buf;
        ASSERT_EQ(0UL, buf.pop_back(1));   // nothing happened

        std::string s = "hello";
        buf.append(s);
        ASSERT_EQ(s, to_str(buf));
        ASSERT_EQ(0UL, buf.pop_back(0));   // nothing happened
        ASSERT_EQ(s, to_str(buf));

        ASSERT_EQ(1UL, buf.pop_back(1));
        s.resize(s.size() - 1);
        ASSERT_EQ(s, to_str(buf));
        ASSERT_EQ(s.length(), buf.length());
        ASSERT_FALSE(buf.empty());

        ASSERT_EQ(s.length(), buf.pop_back(INT_MAX));
        s.clear();
        ASSERT_EQ(s, to_str(buf));
        ASSERT_EQ(0UL, buf.length());
        ASSERT_TRUE(buf.empty());

        for (size_t i = 0; i < DEFAULT_PAYLOAD * 3 / 2; ++i) {
            s.push_back(i);
        }
        buf.append(s);
        ASSERT_EQ(1UL, buf.pop_back(1));
        s.resize(s.size() - 1);
        ASSERT_EQ(s, to_str(buf));
        ASSERT_EQ(s.length(), buf.length());
        ASSERT_FALSE(buf.empty());

        ASSERT_EQ(s.length(), buf.pop_back(INT_MAX));
        s.clear();
        ASSERT_EQ(s, to_str(buf));
        ASSERT_EQ(0UL, buf.length());
        ASSERT_TRUE(buf.empty());
    }

    TEST_F(CordBufTest, append) {
        install_debug_allocator();

        turbo::cord_buf b;
        ASSERT_EQ(0UL, b.length());
        ASSERT_TRUE(b.empty());
        ASSERT_EQ(-1, b.append(nullptr));
        ASSERT_EQ(0, b.append(""));
        ASSERT_EQ(0, b.append(std::string()));
        ASSERT_EQ(-1, b.append(nullptr, 1));
        ASSERT_EQ(0, b.append("dummy", 0));
        ASSERT_EQ(0UL, b.length());
        ASSERT_TRUE(b.empty());
        ASSERT_EQ(0, b.append("1"));
        ASSERT_EQ(1UL, b.length());
        ASSERT_FALSE(b.empty());
        ASSERT_EQ("1", to_str(b));
        const std::string s = "22";
        ASSERT_EQ(0, b.append(s));
        ASSERT_EQ(3UL, b.length());
        ASSERT_FALSE(b.empty());
        ASSERT_EQ("122", to_str(b));
    }

    TEST_F(CordBufTest, appendv) {
        install_debug_allocator();

        turbo::cord_buf b;
        const_iovec vec[] = {{"hello1",  6},
                             {" world1", 7},
                             {"hello2",  6},
                             {" world2", 7},
                             {"hello3",  6},
                             {" world3", 7},
                             {"hello4",  6},
                             {" world4", 7},
                             {"hello5",  6},
                             {" world5", 7}};
        ASSERT_EQ(0, b.appendv(vec, TURBO_ARRAY_SIZE(vec)));
        ASSERT_EQ("hello1 world1hello2 world2hello3 world3hello4 world4hello5 world5",
                  b.to_string());

        // Make some iov_len shorter to test if iov_len works.
        vec[2].iov_len = 4;  // "hello2"
        vec[5].iov_len = 3;  // " world3"
        b.clear();
        ASSERT_EQ(0, b.appendv(vec, TURBO_ARRAY_SIZE(vec)));
        ASSERT_EQ("hello1 world1hell world2hello3 wohello4 world4hello5 world5",
                  b.to_string());

        // Append some long stuff.
        const size_t full_len = DEFAULT_PAYLOAD * 9;
        char *str = (char *) malloc(full_len);
        ASSERT_TRUE(str);
        const size_t len1 = full_len / 6;
        const size_t len2 = full_len / 3;
        const size_t len3 = full_len - len1 - len2;
        ASSERT_GT(len1, (size_t) DEFAULT_PAYLOAD);
        ASSERT_GT(len2, (size_t) DEFAULT_PAYLOAD);
        ASSERT_GT(len3, (size_t) DEFAULT_PAYLOAD);
        ASSERT_EQ(full_len, len1 + len2 + len3);

        for (size_t i = 0; i < full_len; ++i) {
            str[i] = i * 7;
        }
        const_iovec vec2[] = {{str,               len1},
                              {str + len1,        len2},
                              {str + len1 + len2, len3}};
        b.clear();
        ASSERT_EQ(0, b.appendv(vec2, TURBO_ARRAY_SIZE(vec2)));
        ASSERT_EQ(full_len, b.size());
        ASSERT_EQ(0, memcmp(str, b.to_string().data(), full_len));
    }

    TEST_F(CordBufTest, reserve) {
        turbo::cord_buf b;
        ASSERT_EQ(turbo::cord_buf::INVALID_AREA, b.reserve(0));
        const size_t NRESERVED1 = 5;
        const turbo::cord_buf::Area a1 = b.reserve(NRESERVED1);
        ASSERT_TRUE(a1 != turbo::cord_buf::INVALID_AREA);
        ASSERT_EQ(NRESERVED1, b.size());
        b.append("hello world");
        ASSERT_EQ(0, b.unsafe_assign(a1, "prefix")); // `x' will not be copied
        ASSERT_EQ("prefihello world", b.to_string());
        ASSERT_EQ((size_t) 16, b.size());

        // pop/append sth. from back-side and assign again.
        ASSERT_EQ((size_t) 5, b.pop_back(5));
        ASSERT_EQ("prefihello ", b.to_string());
        b.append("blahblahfoobar");
        ASSERT_EQ(0, b.unsafe_assign(a1, "goodorbad")); // `x' will not be copied
        ASSERT_EQ("goodohello blahblahfoobar", b.to_string());

        // append a long string and assign again.
        std::string s1(DEFAULT_PAYLOAD * 3, '\0');
        for (size_t i = 0; i < s1.size(); ++i) {
            s1[i] = i * 7;
        }
        ASSERT_EQ(DEFAULT_PAYLOAD * 3, s1.size());
        // remove everything after reserved area
        ASSERT_GE(b.size(), NRESERVED1);
        b.pop_back(b.size() - NRESERVED1);
        ASSERT_EQ(NRESERVED1, b.size());
        b.append(s1);
        ASSERT_EQ(0, b.unsafe_assign(a1, "appleblahblah"));
        ASSERT_EQ("apple" + s1, b.to_string());

        // Reserve long
        b.pop_back(b.size() - NRESERVED1);
        ASSERT_EQ(NRESERVED1, b.size());
        const size_t NRESERVED2 = DEFAULT_PAYLOAD * 3;
        const turbo::cord_buf::Area a2 = b.reserve(NRESERVED2);
        ASSERT_EQ(NRESERVED1 + NRESERVED2, b.size());
        b.append(s1);
        ASSERT_EQ(NRESERVED1 + NRESERVED2 + s1.size(), b.size());
        std::string s2(NRESERVED2, 0);
        for (size_t i = 0; i < s2.size(); ++i) {
            s2[i] = i * 13;
        }
        ASSERT_EQ(NRESERVED2, s2.size());
        ASSERT_EQ(0, b.unsafe_assign(a2, s2.data()));
        ASSERT_EQ("apple" + s2 + s1, b.to_string());
        ASSERT_EQ(0, b.unsafe_assign(a1, "orangeblahblah"));
        ASSERT_EQ("orang" + s2 + s1, b.to_string());
    }

    struct FakeBlock {
        int nshared;

        FakeBlock() : nshared(1) {}
    };

    TEST_F(CordBufTest, iobuf_as_queue) {
        install_debug_allocator();

        // If INITIAL_CAP gets bigger, creating turbo::cord_buf::Block are very
        // small. Since We don't access turbo::cord_buf::Block::data in this case.
        // We replace turbo::cord_buf::Block with FakeBlock with only nshared (in
        // the same offset)
        FakeBlock *blocks[turbo::cord_buf::INITIAL_CAP + 16];
        const size_t NBLOCKS = TURBO_ARRAY_SIZE(blocks);
        turbo::cord_buf::BlockRef r[NBLOCKS];
        const size_t LENGTH = 7UL;
        for (size_t i = 0; i < NBLOCKS; ++i) {
            ASSERT_TRUE((blocks[i] = new FakeBlock));
            r[i].offset = 1;
            r[i].length = LENGTH;
            r[i].block = (turbo::cord_buf::Block *) blocks[i];
        }

        turbo::cord_buf p;

        // Empty
        ASSERT_EQ(0UL, p._ref_num());
        ASSERT_EQ(-1, p._pop_front_ref());
        ASSERT_EQ(0UL, p.length());

        // Add one ref
        p._push_back_ref(r[0]);
        ASSERT_EQ(1UL, p._ref_num());
        ASSERT_EQ(LENGTH, p.length());
        ASSERT_EQ(r[0], p._front_ref());
        ASSERT_EQ(r[0], p._back_ref());
        ASSERT_EQ(r[0], p._ref_at(0));
        ASSERT_EQ(2, turbo::iobuf::block_shared_count(r[0].block));

        // Add second ref
        p._push_back_ref(r[1]);
        ASSERT_EQ(2UL, p._ref_num());
        ASSERT_EQ(LENGTH * 2, p.length());
        ASSERT_EQ(r[0], p._front_ref());
        ASSERT_EQ(r[1], p._back_ref());
        ASSERT_EQ(r[0], p._ref_at(0));
        ASSERT_EQ(r[1], p._ref_at(1));
        ASSERT_EQ(2, turbo::iobuf::block_shared_count(r[1].block));

        // Pop a ref
        ASSERT_EQ(0, p._pop_front_ref());
        ASSERT_EQ(1UL, p._ref_num());
        ASSERT_EQ(LENGTH, p.length());

        ASSERT_EQ(r[1], p._front_ref());
        ASSERT_EQ(r[1], p._back_ref());
        ASSERT_EQ(r[1], p._ref_at(0));
        //ASSERT_EQ(1, turbo::iobuf::block_shared_count(r[0].block));

        // Pop second
        ASSERT_EQ(0, p._pop_front_ref());
        ASSERT_EQ(0UL, p._ref_num());
        ASSERT_EQ(0UL, p.length());
        //ASSERT_EQ(1, r[1].block->nshared);

        // Add INITIAL_CAP+2 refs, r[0] and r[1] are used, don't use again
        for (size_t i = 0; i < turbo::cord_buf::INITIAL_CAP + 2; ++i) {
            p._push_back_ref(r[i + 2]);
            ASSERT_EQ(i + 1, p._ref_num());
            ASSERT_EQ(p._ref_num() * LENGTH, p.length());
            ASSERT_EQ(r[2], p._front_ref()) << i;
            ASSERT_EQ(r[i + 2], p._back_ref());
            for (size_t j = 0; j <= i; j += std::max(1UL, i / 20) /*not check all*/) {
                ASSERT_EQ(r[j + 2], p._ref_at(j));
            }
            ASSERT_EQ(2, turbo::iobuf::block_shared_count(r[i + 2].block));
        }

        // Pop them all
        const size_t saved_ref_num = p._ref_num();
        while (p._ref_num() >= 2UL) {
            const size_t last_ref_num = p._ref_num();
            ASSERT_EQ(0, p._pop_front_ref());
            ASSERT_EQ(last_ref_num, p._ref_num() + 1);
            ASSERT_EQ(p._ref_num() * LENGTH, p.length());
            const size_t f = saved_ref_num - p._ref_num() + 2;
            ASSERT_EQ(r[f], p._front_ref());
            ASSERT_EQ(r[saved_ref_num + 1], p._back_ref());
            for (size_t j = 0; j < p._ref_num(); j += std::max(1UL, p._ref_num() / 20)) {
                ASSERT_EQ(r[j + f], p._ref_at(j));
            }
            //ASSERT_EQ(1, r[f-1].block->nshared);
        }

        ASSERT_EQ(1ul, p._ref_num());
        // Pop last one
        ASSERT_EQ(0, p._pop_front_ref());
        ASSERT_EQ(0UL, p._ref_num());
        ASSERT_EQ(0UL, p.length());
        //ASSERT_EQ(1, r[saved_ref_num+1].block->nshared);

        // Delete blocks
        for (size_t i = 0; i < NBLOCKS; ++i) {
            delete blocks[i];
        }
    }

    TEST_F(CordBufTest, iobuf_sanity) {
        install_debug_allocator();

        TURBO_LOG(INFO) << "sizeof(turbo::cord_buf)=" << sizeof(turbo::cord_buf)
                  << " sizeof(IOPortal)=" << sizeof(turbo::IOPortal);

        turbo::cord_buf b1;
        std::string s1 = "hello world";
        const char c1 = 'A';
        const std::string s2 = "too simple";
        std::string s1c = s1;
        s1c.erase(0, 1);

        // Append a c-std::string
        ASSERT_EQ(0, b1.append(s1.c_str()));
        ASSERT_EQ(s1.length(), b1.length());
        ASSERT_EQ(s1, to_str(b1));
        ASSERT_EQ(1UL, b1._ref_num());

        // Append a char
        ASSERT_EQ(0, b1.push_back(c1));
        ASSERT_EQ(s1.length() + 1, b1.length());
        ASSERT_EQ(s1 + c1, to_str(b1));
        ASSERT_EQ(1UL, b1._ref_num());

        // Append a std::string
        ASSERT_EQ(0, b1.append(s2));
        ASSERT_EQ(s1.length() + 1 + s2.length(), b1.length());
        ASSERT_EQ(s1 + c1 + s2, to_str(b1));
        ASSERT_EQ(1UL, b1._ref_num());

        // Pop first char
        ASSERT_EQ(1UL, b1.pop_front(1));
        ASSERT_EQ(1UL, b1._ref_num());
        ASSERT_EQ(s1.length() + s2.length(), b1.length());
        ASSERT_EQ(s1c + c1 + s2, to_str(b1));

        // Pop all
        ASSERT_EQ(0UL, b1.pop_front(0));
        ASSERT_EQ(s1.length() + s2.length(), b1.pop_front(b1.length() + 1));
        ASSERT_TRUE(b1.empty());
        ASSERT_EQ(0UL, b1.length());
        ASSERT_EQ(0UL, b1._ref_num());
        ASSERT_EQ("", to_str(b1));

        // Restore
        ASSERT_EQ(0, b1.append(s1.c_str()));
        ASSERT_EQ(0, b1.push_back(c1));
        ASSERT_EQ(0, b1.append(s2));

        // Cut first char
        turbo::cord_buf p;
        b1.cutn(&p, 0);
        b1.cutn(&p, 1);
        ASSERT_EQ(s1.substr(0, 1), to_str(p));
        ASSERT_EQ(s1c + c1 + s2, to_str(b1));

        // Cut another two and append to p
        b1.cutn(&p, 2);
        ASSERT_EQ(s1.substr(0, 3), to_str(p));
        std::string s1d = s1;
        s1d.erase(0, 3);
        ASSERT_EQ(s1d + c1 + s2, to_str(b1));

        // Clear
        b1.clear();
        ASSERT_TRUE(b1.empty());
        ASSERT_EQ(0UL, b1.length());
        ASSERT_EQ(0UL, b1._ref_num());
        ASSERT_EQ("", to_str(b1));
        ASSERT_EQ(s1.substr(0, 3), to_str(p));
    }

    TEST_F(CordBufTest, copy_and_assign) {
        install_debug_allocator();

        const size_t TARGET_SIZE = turbo::cord_buf::DEFAULT_BLOCK_SIZE * 2;
        turbo::cord_buf buf0;
        buf0.append("hello");
        ASSERT_EQ(1u, buf0._ref_num());

        // Copy-construct from SmallView
        turbo::cord_buf buf1 = buf0;
        ASSERT_EQ(1u, buf1._ref_num());
        ASSERT_EQ(buf0, buf1);

        buf1.resize(TARGET_SIZE, 'z');
        ASSERT_LT(2u, buf1._ref_num());
        ASSERT_EQ(TARGET_SIZE, buf1.size());

        // Copy-construct from BigView
        turbo::cord_buf buf2 = buf1;
        ASSERT_EQ(buf1, buf2);

        // assign BigView to SmallView
        turbo::cord_buf buf3;
        buf3 = buf1;
        ASSERT_EQ(buf1, buf3);

        // assign BigView to BigView
        turbo::cord_buf buf4;
        buf4.resize(TARGET_SIZE, 'w');
        ASSERT_NE(buf1, buf4);
        buf4 = buf1;
        ASSERT_EQ(buf1, buf4);
    }

    TEST_F(CordBufTest, compare) {
        install_debug_allocator();

        const char *SEED = "abcdefghijklmnqopqrstuvwxyz";
        turbo::cord_buf seedbuf;
        seedbuf.append(SEED);
        const int REP = 100;
        turbo::cord_buf b1;
        for (int i = 0; i < REP; ++i) {
            b1.append(seedbuf);
            b1.append(SEED);
        }
        turbo::cord_buf b2;
        for (int i = 0; i < REP * 2; ++i) {
            b2.append(SEED);
        }
        ASSERT_EQ(b1, b2);

        turbo::cord_buf b3 = b2;

        b2.push_back('0');
        ASSERT_NE(b1, b2);
        ASSERT_EQ(b1, b3);

        b1.clear();
        b2.clear();
        ASSERT_EQ(b1, b2);
    }

    TEST_F(CordBufTest, append_and_cut_it_all) {
        turbo::cord_buf b;
        const size_t N = 32768UL;
        for (size_t i = 0; i < N; ++i) {
            ASSERT_EQ(0, b.push_back(i));
        }
        ASSERT_EQ(N, b.length());
        turbo::cord_buf p;
        b.cutn(&p, N);
        ASSERT_TRUE(b.empty());
        ASSERT_EQ(N, p.length());
    }

    TEST_F(CordBufTest, copy_to) {
        turbo::cord_buf b;
        const std::string seed = "abcdefghijklmnopqrstuvwxyz";
        std::string src;
        for (size_t i = 0; i < 1000; ++i) {
            src.append(seed);
        }
        b.append(src);
        ASSERT_GT(b.size(), DEFAULT_PAYLOAD);
        std::string s1;
        ASSERT_EQ(src.size(), b.copy_to(&s1));
        ASSERT_EQ(src, s1);

        std::string s2;
        ASSERT_EQ(32u, b.copy_to(&s2, 32));
        ASSERT_EQ(src.substr(0, 32), s2);

        std::string s3;
        const std::string expected = src.substr(DEFAULT_PAYLOAD - 1, 33);
        ASSERT_EQ(33u, b.copy_to(&s3, 33, DEFAULT_PAYLOAD - 1));
        ASSERT_EQ(expected, s3);

        ASSERT_EQ(33u, b.append_to(&s3, 33, DEFAULT_PAYLOAD - 1));
        ASSERT_EQ(expected + expected, s3);

        turbo::cord_buf b1;
        ASSERT_EQ(src.size(), b.append_to(&b1));
        ASSERT_EQ(src, b1.to_string());

        turbo::cord_buf b2;
        ASSERT_EQ(32u, b.append_to(&b2, 32));
        ASSERT_EQ(src.substr(0, 32), b2.to_string());

        turbo::cord_buf b3;
        ASSERT_EQ(33u, b.append_to(&b3, 33, DEFAULT_PAYLOAD - 1));
        ASSERT_EQ(expected, b3.to_string());

        ASSERT_EQ(33u, b.append_to(&b3, 33, DEFAULT_PAYLOAD - 1));
        ASSERT_EQ(expected + expected, b3.to_string());
    }

    TEST_F(CordBufTest, cut_by_single_text_delim) {
        install_debug_allocator();

        turbo::cord_buf b;
        turbo::cord_buf p;
        std::vector<turbo::cord_buf> ps;
        std::string s1 = "1234567\n12\n\n2567";
        ASSERT_EQ(0, b.append(s1));
        ASSERT_EQ(s1.length(), b.length());

        for (; b.cut_until(&p, "\n") == 0;) {
            ps.push_back(p);
            p.clear();
        }

        ASSERT_EQ(3UL, ps.size());
        ASSERT_EQ("1234567", to_str(ps[0]));
        ASSERT_EQ("12", to_str(ps[1]));
        ASSERT_EQ("", to_str(ps[2]));
        ASSERT_EQ("2567", to_str(b));

        b.append("\n");
        ASSERT_EQ(0, b.cut_until(&p, "\n"));
        ASSERT_EQ("2567", to_str(p));
        ASSERT_EQ("", to_str(b));
    }

    TEST_F(CordBufTest, cut_by_multiple_text_delim) {
        install_debug_allocator();

        turbo::cord_buf b;
        turbo::cord_buf p;
        std::vector<turbo::cord_buf> ps;
        std::string s1 = "\r\n1234567\r\n12\r\n\n\r2567";
        ASSERT_EQ(0, b.append(s1));
        ASSERT_EQ(s1.length(), b.length());

        for (; b.cut_until(&p, "\r\n") == 0;) {
            ps.push_back(p);
            p.clear();
        }

        ASSERT_EQ(3UL, ps.size());
        ASSERT_EQ("", to_str(ps[0]));
        ASSERT_EQ("1234567", to_str(ps[1]));
        ASSERT_EQ("12", to_str(ps[2]));
        ASSERT_EQ("\n\r2567", to_str(b));

        b.append("\r\n");
        ASSERT_EQ(0, b.cut_until(&p, "\r\n"));
        ASSERT_EQ("\n\r2567", to_str(p));
        ASSERT_EQ("", to_str(b));
    }

    TEST_F(CordBufTest, append_a_lot_and_cut_them_all) {
        install_debug_allocator();

        turbo::cord_buf b;
        turbo::cord_buf p;
        std::string s1 = "12345678901234567";
        const size_t N = 10000;
        for (size_t i = 0; i < N; ++i) {
            b.append(s1);
        }
        ASSERT_EQ(N * s1.length(), b.length());

        while (b.length() >= 7) {
            b.cutn(&p, 7);
        }
        size_t remain = s1.length() * N % 7;
        ASSERT_EQ(remain, b.length());
        ASSERT_EQ(s1.substr(s1.length() - remain, remain), to_str(b));
        ASSERT_EQ(s1.length() * N / 7 * 7, p.length());
    }

    TEST_F(CordBufTest, cut_into_fd_tiny) {
        install_debug_allocator();

        turbo::IOPortal b1, b2;
        std::string ref;
        int fds[2];

        for (int j = 10; j > 0; --j) {
            ref.push_back(j);
        }
        b1.append(ref);
        ASSERT_EQ(1UL, b1.pop_front(1));
        ref.erase(0, 1);
        ASSERT_EQ(ref, to_str(b1));
        TURBO_LOG(INFO) << "ref.size=" << ref.size();

        //ASSERT_EQ(0, pipe(fds));
        ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));

        turbo::base::make_non_blocking(fds[0]);
        turbo::base::make_non_blocking(fds[1]);

        while (!b1.empty() || b2.length() != ref.length()) {
            size_t b1len = b1.length(), b2len = b2.length();
            errno = 0;
            printf("b1.length=%lu - %ld (%m), ",
                   b1len, b1.cut_into_file_descriptor(fds[1]));
            printf("b2.length=%lu + %ld (%m)\n",
                   b2len, b2.append_from_file_descriptor(fds[0], LONG_MAX));
        }
        printf("b1.length=%lu, b2.length=%lu\n", b1.length(), b2.length());

        ASSERT_EQ(ref, to_str(b2));

        close(fds[0]);
        close(fds[1]);
    }

    TEST_F(CordBufTest, cut_multiple_into_fd_tiny) {
        install_debug_allocator();

        turbo::cord_buf *b1[10];
        turbo::IOPortal b2;
        std::string ref;
        int fds[2];

        for (size_t j = 0; j < TURBO_ARRAY_SIZE(b1); ++j) {
            std::string s;
            for (int i = 10; i > 0; --i) {
                s.push_back(j * 10 + i);
            }
            ref.append(s);
            turbo::IOPortal *b = new turbo::IOPortal();
            b->append(s);
            b1[j] = b;
        }

        ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
        turbo::base::make_non_blocking(fds[0]);
        turbo::base::make_non_blocking(fds[1]);

        ASSERT_EQ((ssize_t) ref.length(),
                  turbo::cord_buf::cut_multiple_into_file_descriptor(
                          fds[1], b1, TURBO_ARRAY_SIZE(b1)));
        for (size_t j = 0; j < TURBO_ARRAY_SIZE(b1); ++j) {
            ASSERT_TRUE(b1[j]->empty());
            delete (turbo::IOPortal *) b1[j];
            b1[j] = nullptr;
        }
        ASSERT_EQ((ssize_t) ref.length(),
                  b2.append_from_file_descriptor(fds[0], LONG_MAX));
        ASSERT_EQ(ref, to_str(b2));

        close(fds[0]);
        close(fds[1]);
    }

    TEST_F(CordBufTest, cut_into_fd_a_lot_of_data) {
        install_debug_allocator();

        turbo::IOPortal b0, b1, b2;
        std::string s, ref;
        int fds[2];

        for (int j = rand() % 7777 + 300000; j > 0; --j) {
            ref.push_back(j);
            s.push_back(j);

            if (s.length() == 1024UL || j == 1) {
                b0.append(s);
                ref.append("tailing");
                b0.cutn(&b1, b0.length());
                ASSERT_EQ(0, b1.append("tailing"));
                s.clear();
            }
        }

        ASSERT_EQ(ref.length(), b1.length());
        ASSERT_EQ(ref, to_str(b1));
        ASSERT_TRUE(b0.empty());
        TURBO_LOG(INFO) << "ref.size=" << ref.size();

        //ASSERT_EQ(0, pipe(fds));
        ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
        turbo::base::make_non_blocking(fds[0]);
        turbo::base::make_non_blocking(fds[1]);
        const int sockbufsize = 16 * 1024 - 17;
        ASSERT_EQ(0, setsockopt(fds[1], SOL_SOCKET, SO_SNDBUF,
                                (const char *) &sockbufsize, sizeof(int)));
        ASSERT_EQ(0, setsockopt(fds[0], SOL_SOCKET, SO_RCVBUF,
                                (const char *) &sockbufsize, sizeof(int)));

        while (!b1.empty() || b2.length() != ref.length()) {
            size_t b1len = b1.length(), b2len = b2.length();
            errno = 0;
            printf("b1.length=%lu - %ld (%m), ", b1len, b1.cut_into_file_descriptor(fds[1]));
            printf("b2.length=%lu + %ld (%m)\n", b2len, b2.append_from_file_descriptor(fds[0], LONG_MAX));
        }
        printf("b1.length=%lu, b2.length=%lu\n", b1.length(), b2.length());

        ASSERT_EQ(ref, to_str(b2));

        close(fds[0]);
        close(fds[1]);
    }

    TEST_F(CordBufTest, cut_by_delim_perf) {
        turbo::iobuf::reset_blockmem_allocate_and_deallocate();

        turbo::cord_buf b;
        turbo::cord_buf p;
        std::vector<turbo::cord_buf> ps;
        std::string s1 = "123456789012345678901234567890\n";
        const size_t N = 100000;
        for (size_t i = 0; i < N; ++i) {
            ASSERT_EQ(0, b.append(s1));
        }
        ASSERT_EQ(N * s1.length(), b.length());

        turbo::stop_watcher t;
        //ProfilerStart("cutd.prof");
        t.start();
        for (; b.cut_until(&p, "\n") == 0;) {}
        t.stop();
        //ProfilerStop();

        TURBO_LOG(INFO) << "IOPortal::cut_by_delim takes "
                  << t.n_elapsed() / N << "ns, tp="
                  << s1.length() * N * 1000.0 / t.n_elapsed() << "MB/s";
        show_prof_and_rm("test_iobuf", "cutd.prof", 10);
    }


    TEST_F(CordBufTest, cut_perf) {
        turbo::iobuf::reset_blockmem_allocate_and_deallocate();

        turbo::cord_buf b;
        turbo::cord_buf p;
        const size_t length = 60000000UL;
        const size_t REP = 10;
        turbo::stop_watcher t;
        std::string s = "1234567890";
        const bool push_char = false;

        if (!push_char) {
            //ProfilerStart("iobuf_append.prof");
            t.start();
            for (size_t j = 0; j < REP; ++j) {
                b.clear();
                for (size_t i = 0; i < length / s.length(); ++i) {
                    b.append(s);
                }
            }
            t.stop();
            //ProfilerStop();
            TURBO_LOG(INFO) << "IOPortal::append(std::string_" << s.length() << ") takes "
                      << t.n_elapsed() * s.length() / length / REP << "ns, tp="
                      << REP * length * 1000.0 / t.n_elapsed() << "MB/s";
        } else {
            //ProfilerStart("iobuf_pushback.prof");
            t.start();
            for (size_t i = 0; i < length; ++i) {
                b.push_back(i);
            }
            t.stop();
            //ProfilerStop();
            TURBO_LOG(INFO) << "IOPortal::push_back(char) takes "
                      << t.n_elapsed() / length << "ns, tp="
                      << length * 1000.0 / t.n_elapsed() << "MB/s";
        }

        ASSERT_EQ(length, b.length());

        size_t w[3] = {16, 128, 1024};
        //char name[32];

        for (size_t i = 0; i < TURBO_ARRAY_SIZE(w); ++i) {
            // snprintf(name, sizeof(name), "iobuf_cut%lu.prof", w[i]);
            // ProfilerStart(name);

            t.start();
            while (b.length() >= w[i] + 12) {
                b.cutn(&p, 12);
                b.cutn(&p, w[i]);
            }
            t.stop();

            //ProfilerStop();

            TURBO_LOG(INFO) << "IOPortal::cutn(12+" << w[i] << ") takes "
                      << t.n_elapsed() * (w[i] + 12) / length << "ns, tp="
                      << length * 1000.0 / t.n_elapsed() << "MB/s";

            ASSERT_LT(b.length(), w[i] + 12);

            t.start();
            b.append(p);
            t.stop();
            TURBO_LOG(INFO) << "IOPortal::append(turbo::cord_buf) takes "
                      << t.n_elapsed() / p._ref_num() << "ns, tp="
                      << length * 1000.0 / t.n_elapsed() << "MB/s";

            p.clear();
            ASSERT_EQ(length, b.length());
        }

        show_prof_and_rm("test_iobuf", "./iobuf_append.prof", 10);
        show_prof_and_rm("test_iobuf", "./iobuf_pushback.prof", 10);
    }

    TEST_F(CordBufTest, append_store_append_cut) {
        turbo::iobuf::reset_blockmem_allocate_and_deallocate();

        std::string ref;
        ref.resize(rand() % 376813 + 19777777);
        for (size_t j = 0; j < ref.size(); ++j) {
            ref[j] = j;
        }

        turbo::IOPortal b1, b2;
        std::vector<turbo::cord_buf> ps;
        ssize_t nr;
        size_t HINT = 16 * 1024UL;
        turbo::stop_watcher t;
        size_t w[3] = {16, 128, 1024};
        char name[64];
        char profname[64];
        char cmd[512];
        bool write_to_dev_null = true;
        size_t nappend, ncut;

        turbo::temp_file f;
        ASSERT_EQ(0, f.save_bin(ref.data(), ref.length()));

        for (size_t i = 0; i < TURBO_ARRAY_SIZE(w); ++i) {
            ps.reserve(ref.size() / (w[i] + 12) + 1);
            // TURBO_LOG(INFO) << "ps.cap=" << ps.capacity();

            const int ifd = open(f.fname(), O_RDONLY);
            ASSERT_TRUE(ifd > 0);
            if (write_to_dev_null) {
                snprintf(name, sizeof(name), "/dev/null");
            } else {
                snprintf(name, sizeof(name), "iobuf_asac%lu.output", w[i]);
                snprintf(cmd, sizeof(cmd), "cmp %s %s", f.fname(), name);
            }
            const int ofd = open(name, O_CREAT | O_WRONLY, 0666);
            ASSERT_TRUE(ofd > 0) << strerror(errno);
            snprintf(profname, sizeof(profname), "iobuf_asac%lu.prof", w[i]);

            //ProfilerStart(profname);
            t.start();

            nappend = ncut = 0;
            while ((nr = b1.append_from_file_descriptor(ifd, HINT)) > 0) {
                ++nappend;
                while (b1.length() >= w[i] + 12) {
                    turbo::cord_buf p;
                    b1.cutn(&p, 12);
                    b1.cutn(&p, w[i]);
                    ps.push_back(p);
                }
            }
            for (size_t j = 0; j < ps.size(); ++j) {
                b2.append(ps[j]);
                if (b2.length() >= HINT) {
                    b2.cut_into_file_descriptor(ofd);
                }
            }
            b2.cut_into_file_descriptor(ofd);
            b1.cut_into_file_descriptor(ofd);

            close(ifd);
            close(ofd);
            t.stop();
            //ProfilerStop();

            ASSERT_TRUE(b1.empty());
            ASSERT_TRUE(b2.empty());
            //TURBO_LOG(INFO) << "ps.size=" << ps.size();
            ps.clear();
            TURBO_LOG(INFO) << "Bandwidth of append(" << f.fname() << ")->cut(12+" << w[i]
                      << ")->store->append->cut(" << name << ") is "
                      << ref.length() * 1000.0 / t.n_elapsed() << "MB/s";

            if (!write_to_dev_null) {
                ASSERT_EQ(0, system(cmd));
            }
            remove(name);
        }

        for (size_t i = 0; i < TURBO_ARRAY_SIZE(w); ++i) {
            snprintf(name, sizeof(name), "iobuf_asac%lu.prof", w[i]);
            show_prof_and_rm("test_iobuf", name, 10);
        }
    }

    TEST_F(CordBufTest, conversion_with_protobuf) {
        const int REP = 1000;
        proto::Misc m1;
        m1.set_required_enum(proto::CompressTypeGzip);
        m1.set_required_uint64(0xdeadbeef);
        m1.set_optional_uint64(10000);
        m1.set_required_string("hello iobuf");
        m1.set_optional_string("hello iobuf again");
        for (int i = 0; i < REP; ++i) {
            char buf[16];
            snprintf(buf, sizeof(buf), "value%d", i);
            m1.add_repeated_string(buf);
        }
        m1.set_required_bool(true);
        m1.set_required_int32(0xbeefdead);

        turbo::cord_buf buf;
        const std::string header("just-make-sure-wrapper-does-not-clear-cord_buf");
        ASSERT_EQ(0, buf.append(header));
        turbo::cord_buf_as_zero_copy_output_stream out_wrapper(&buf);
        ASSERT_EQ(0, out_wrapper.ByteCount());
        ASSERT_TRUE(m1.SerializeToZeroCopyStream(&out_wrapper));
        ASSERT_EQ((size_t) m1.ByteSizeLong() + header.size(), buf.length());
        ASSERT_EQ(m1.ByteSizeLong(), static_cast<size_t>(out_wrapper.ByteCount()));

        ASSERT_EQ(header.size(), buf.pop_front(header.size()));
        turbo::cord_buf_as_zero_copy_input_stream in_wrapper(buf);
        ASSERT_EQ(0, in_wrapper.ByteCount());
        {
            const void *dummy_blk = nullptr;
            int dummy_size = 0;
            ASSERT_TRUE(in_wrapper.Next(&dummy_blk, &dummy_size));
            ASSERT_EQ(dummy_size, in_wrapper.ByteCount());
            in_wrapper.BackUp(1);
            ASSERT_EQ(dummy_size - 1, in_wrapper.ByteCount());
            const void *dummy_blk2 = nullptr;
            int dummy_size2 = 0;
            ASSERT_TRUE(in_wrapper.Next(&dummy_blk2, &dummy_size2));
            ASSERT_EQ(1, dummy_size2);
            ASSERT_EQ((char *) dummy_blk + dummy_size - 1, (char *) dummy_blk2);
            ASSERT_EQ(dummy_size, in_wrapper.ByteCount());
            in_wrapper.BackUp(dummy_size);
            ASSERT_EQ(0, in_wrapper.ByteCount());
        }
        proto::Misc m2;
        ASSERT_TRUE(m2.ParseFromZeroCopyStream(&in_wrapper));
        ASSERT_EQ(m1.ByteSizeLong(), in_wrapper.ByteCount());
        ASSERT_EQ(m2.ByteSizeLong(), in_wrapper.ByteCount());

        ASSERT_EQ(m1.required_enum(), m2.required_enum());
        ASSERT_FALSE(m2.has_optional_enum());
        ASSERT_EQ(m1.required_uint64(), m2.required_uint64());
        ASSERT_TRUE(m2.has_optional_uint64());
        ASSERT_EQ(m1.optional_uint64(), m2.optional_uint64());
        ASSERT_EQ(m1.required_string(), m2.required_string());
        ASSERT_TRUE(m2.has_optional_string());
        ASSERT_EQ(m1.optional_string(), m2.optional_string());
        ASSERT_EQ(REP, m2.repeated_string_size());
        for (int i = 0; i < REP; ++i) {
            ASSERT_EQ(m1.repeated_string(i), m2.repeated_string(i));
        }
        ASSERT_EQ(m1.required_bool(), m2.required_bool());
        ASSERT_FALSE(m2.has_optional_bool());
        ASSERT_EQ(m1.required_int32(), m2.required_int32());
        ASSERT_FALSE(m2.has_optional_int32());
    }

    TEST_F(CordBufTest, extended_backup) {
        for (int i = 0; i < 2; ++i) {
            std::cout << "i=" << i << std::endl;
            // Consume the left TLS block so that cases are easier to check.
            turbo::iobuf::remove_tls_block_chain();
            turbo::cord_buf src;
            const int BLKSIZE = (i == 0 ? 1024 : turbo::cord_buf::DEFAULT_BLOCK_SIZE);
            const int PLDSIZE = BLKSIZE - BLOCK_OVERHEAD;
            turbo::cord_buf_as_zero_copy_output_stream out_stream1(&src, BLKSIZE);
            turbo::cord_buf_as_zero_copy_output_stream out_stream2(&src);
            turbo::cord_buf_as_zero_copy_output_stream &out_stream =
                    (i == 0 ? out_stream1 : out_stream2);
            void *blk1 = nullptr;
            int size1 = 0;
            ASSERT_TRUE(out_stream.Next(&blk1, &size1));
            ASSERT_EQ(PLDSIZE, size1);
            ASSERT_EQ(size1, out_stream.ByteCount());
            void *blk2 = nullptr;
            int size2 = 0;
            ASSERT_TRUE(out_stream.Next(&blk2, &size2));
            ASSERT_EQ(PLDSIZE, size2);
            ASSERT_EQ(size1 + size2, out_stream.ByteCount());
            // BackUp a size that's valid for all ZeroCopyOutputStream
            out_stream.BackUp(PLDSIZE / 2);
            ASSERT_EQ(size1 + size2 - PLDSIZE / 2, out_stream.ByteCount());
            void *blk3 = nullptr;
            int size3 = 0;
            ASSERT_TRUE(out_stream.Next(&blk3, &size3));
            ASSERT_EQ((char *) blk2 + PLDSIZE / 2, blk3);
            ASSERT_EQ(PLDSIZE / 2, size3);
            ASSERT_EQ(size1 + size2, out_stream.ByteCount());

            // BackUp a size that's undefined in regular ZeroCopyOutputStream
            out_stream.BackUp(PLDSIZE * 2);
            ASSERT_EQ(0, out_stream.ByteCount());
            void *blk4 = nullptr;
            int size4 = 0;
            ASSERT_TRUE(out_stream.Next(&blk4, &size4));
            ASSERT_EQ(PLDSIZE, size4);
            ASSERT_EQ(size4, out_stream.ByteCount());
            if (i == 1) {
                ASSERT_EQ(blk1, blk4);
            }
            void *blk5 = nullptr;
            int size5 = 0;
            ASSERT_TRUE(out_stream.Next(&blk5, &size5));
            ASSERT_EQ(PLDSIZE, size5);
            ASSERT_EQ(size4 + size5, out_stream.ByteCount());
            if (i == 1) {
                ASSERT_EQ(blk2, blk5);
            }
        }
    }

    TEST_F(CordBufTest, backup_iobuf_never_called_next) {
        {
            // Consume the left TLS block so that later cases are easier
            // to check.
            turbo::cord_buf dummy;
            turbo::cord_buf_as_zero_copy_output_stream dummy_stream(&dummy);
            void *dummy_data = nullptr;
            int dummy_size = 0;
            ASSERT_TRUE(dummy_stream.Next(&dummy_data, &dummy_size));
        }
        turbo::cord_buf src;
        const size_t N = DEFAULT_PAYLOAD * 2;
        src.resize(N);
        ASSERT_EQ(2u, src.backing_block_num());
        ASSERT_EQ(N, src.size());
        turbo::cord_buf_as_zero_copy_output_stream out_stream(&src);
        out_stream.BackUp(1); // also succeed.
        ASSERT_EQ(-1, out_stream.ByteCount());
        ASSERT_EQ(DEFAULT_PAYLOAD * 2 - 1, src.size());
        ASSERT_EQ(2u, src.backing_block_num());
        void *data0 = nullptr;
        int size0 = 0;
        ASSERT_TRUE(out_stream.Next(&data0, &size0));
        ASSERT_EQ(1, size0);
        ASSERT_EQ(0, out_stream.ByteCount());
        ASSERT_EQ(2u, src.backing_block_num());
        void *data1 = nullptr;
        int size1 = 0;
        ASSERT_TRUE(out_stream.Next(&data1, &size1));
        ASSERT_EQ(size1, out_stream.ByteCount());
        ASSERT_EQ(3u, src.backing_block_num());
        ASSERT_EQ(N + size1, src.size());
        void *data2 = nullptr;
        int size2 = 0;
        ASSERT_TRUE(out_stream.Next(&data2, &size2));
        ASSERT_EQ(size1 + size2, out_stream.ByteCount());
        ASSERT_EQ(4u, src.backing_block_num());
        ASSERT_EQ(N + size1 + size2, src.size());
        TURBO_LOG(INFO) << "Backup1";
        out_stream.BackUp(size1); // intended size1, not size2 to make this BackUp
        // to cross boundary of blocks.
        ASSERT_EQ(size2, out_stream.ByteCount());
        ASSERT_EQ(N + size2, src.size());
        TURBO_LOG(INFO) << "Backup2";
        out_stream.BackUp(size2);
        ASSERT_EQ(0, out_stream.ByteCount());
        ASSERT_EQ(N, src.size());
    }

    void *backup_thread(void *arg) {
        turbo::cord_buf_as_zero_copy_output_stream *wrapper =
                (turbo::cord_buf_as_zero_copy_output_stream *) arg;
        wrapper->BackUp(1024);
        return nullptr;
    }

    TEST_F(CordBufTest, backup_in_another_thread) {
        turbo::cord_buf buf;
        turbo::cord_buf_as_zero_copy_output_stream wrapper(&buf);
        size_t alloc_size = 0;
        for (int i = 0; i < 10; ++i) {
            void *data;
            int len;
            ASSERT_TRUE(wrapper.Next(&data, &len));
            alloc_size += len;
        }
        ASSERT_EQ(alloc_size, buf.length());
        for (int i = 0; i < 10; ++i) {
            void *data;
            int len;
            ASSERT_TRUE(wrapper.Next(&data, &len));
            alloc_size += len;
            pthread_t tid;
            pthread_create(&tid, nullptr, backup_thread, &wrapper);
            pthread_join(tid, nullptr);
        }
        ASSERT_EQ(alloc_size - 1024 * 10, buf.length());
    }

    TEST_F(CordBufTest, own_block) {
        turbo::cord_buf buf;
        const ssize_t BLOCK_SIZE = 1024;
        turbo::cord_buf::Block *saved_tls_block = turbo::iobuf::get_tls_block_head();
        turbo::cord_buf_as_zero_copy_output_stream wrapper(&buf, BLOCK_SIZE);
        int alloc_size = 0;
        for (int i = 0; i < 100; ++i) {
            void *data;
            int size;
            ASSERT_TRUE(wrapper.Next(&data, &size));
            alloc_size += size;
            if (size > 1) {
                wrapper.BackUp(1);
                alloc_size -= 1;
            }
        }
        ASSERT_EQ(static_cast<size_t>(alloc_size), buf.length());
        ASSERT_EQ(saved_tls_block, turbo::iobuf::get_tls_block_head());
        ASSERT_EQ(turbo::iobuf::block_cap(buf._front_ref().block), BLOCK_SIZE - BLOCK_OVERHEAD);
    }

    struct Foo1 {
        explicit Foo1(int x2) : x(x2) {}

        int x;
    };

    struct Foo2 {
        std::vector<Foo1> foo1;
    };

    std::ostream &operator<<(std::ostream &os, const Foo1 &foo1) {
        return os << "foo1.x=" << foo1.x;
    }

    std::ostream &operator<<(std::ostream &os, const Foo2 &foo2) {
        for (size_t i = 0; i < foo2.foo1.size(); ++i) {
            os << "foo2[" << i << "]=" << foo2.foo1[i];
        }
        return os;
    }

    TEST_F(CordBufTest, as_ostream) {
        turbo::iobuf::reset_blockmem_allocate_and_deallocate();

        turbo::cord_buf_builder builder;
        TURBO_LOG(INFO) << "sizeof(cord_buf_builder)=" << sizeof(builder) << std::endl
                  << "sizeof(cord_buf)=" << sizeof(turbo::cord_buf) << std::endl
                  << "sizeof(cord_buf_as_zero_copy_output_stream)="
                  << sizeof(turbo::cord_buf_as_zero_copy_output_stream) << std::endl
                  << "sizeof(zero_copy_stream_as_stream_buf)="
                  << sizeof(turbo::zero_copy_stream_as_stream_buf) << std::endl
                  << "sizeof(ostream)=" << sizeof(std::ostream);
        int x = -1;
        builder << 2 << " " << x << " " << 1.1 << " hello ";
        ASSERT_EQ("2 -1 1.1 hello ", builder.buf().to_string());

        builder << "world!";
        ASSERT_EQ("2 -1 1.1 hello world!", builder.buf().to_string());

        builder.buf().clear();
        builder << "world!";
        ASSERT_EQ("world!", builder.buf().to_string());

        Foo2 foo2;
        for (int i = 0; i < 10000; ++i) {
            foo2.foo1.push_back(Foo1(i));
        }
        builder.buf().clear();
        builder << "<before>" << foo2 << "<after>";
        std::ostringstream oss;
        oss << "<before>" << foo2 << "<after>";
        ASSERT_EQ(oss.str(), builder.buf().to_string());

        turbo::cord_buf target;
        builder.move_to(target);
        ASSERT_TRUE(builder.buf().empty());
        ASSERT_EQ(oss.str(), target.to_string());

        std::ostringstream oss2;
        oss2 << target;
        ASSERT_EQ(oss.str(), oss2.str());
    }

    TEST_F(CordBufTest, append_from_fd_with_offset) {
        turbo::temp_file file;
        file.save("dummy");
        turbo::base::fd_guard fd(open(file.fname(), O_RDWR | O_TRUNC));
        ASSERT_TRUE(fd >= 0) << file.fname() << ' ' << turbo_error();
        turbo::IOPortal buf;
        char dummy[10 * 1024];
        buf.append(dummy, sizeof(dummy));
        ASSERT_EQ((ssize_t) sizeof(dummy), buf.cut_into_file_descriptor(fd));
        for (size_t i = 0; i < sizeof(dummy); ++i) {
            turbo::IOPortal b0;
            ASSERT_EQ(sizeof(dummy) - i, (size_t) b0.pappend_from_file_descriptor(fd, i, sizeof(dummy)))
                                        << turbo_error();
            char tmp[sizeof(dummy)];
            ASSERT_EQ(0, memcmp(dummy + i, b0.fetch(tmp, b0.length()), b0.length()));
        }

    }

    static std::atomic<int> s_nthread(0);
    static long number_per_thread = 1024;

    void *cut_into_fd(void *arg) {
        int fd = (int) (long) arg;
        const long start_num = number_per_thread *
                               s_nthread.fetch_add(1, std::memory_order_relaxed);
        off_t offset = start_num * sizeof(int);
        for (int i = 0; i < number_per_thread; ++i) {
            int to_write = start_num + i;
            turbo::cord_buf out;
            out.append(&to_write, sizeof(int));
            TURBO_CHECK_EQ(out.pcut_into_file_descriptor(fd, offset + sizeof(int) * i),
                     (ssize_t) sizeof(int));
        }
        return nullptr;
    }

    TEST_F(CordBufTest, cut_into_fd_with_offset_multithreaded) {
        s_nthread.store(0);
        number_per_thread = 10240;
        pthread_t threads[8];
        long fd = open(".out.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
        ASSERT_TRUE(fd >= 0) << turbo_error();
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(threads); ++i) {
            ASSERT_EQ(0, pthread_create(&threads[i], nullptr, cut_into_fd, (void *) fd));
        }
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
        for (int i = 0; i < number_per_thread * (int) TURBO_ARRAY_SIZE(threads); ++i) {
            off_t offset = i * sizeof(int);
            turbo::IOPortal in;
            ASSERT_EQ((ssize_t) sizeof(int), in.pappend_from_file_descriptor(fd, offset, sizeof(int)));
            int j;
            ASSERT_EQ(sizeof(j), in.cutn(&j, sizeof(j)));
            ASSERT_EQ(i, j);
        }
    }

    TEST_F(CordBufTest, slice) {
        size_t N = 100000;
        std::string expected;
        expected.reserve(N);
        turbo::cord_buf buf;
        for (size_t i = 0; i < N; ++i) {
            expected.push_back(i % 26 + 'a');
            buf.push_back(i % 26 + 'a');
        }
        const size_t block_count = buf.backing_block_num();
        std::string actual;
        actual.reserve(expected.size());
        for (size_t i = 0; i < block_count; ++i) {
            std::string_view p = buf.backing_block(i);
            ASSERT_FALSE(p.empty());
            actual.append(p.data(), p.size());
        }
        ASSERT_TRUE(expected == actual);
    }

    TEST_F(CordBufTest, swap) {
        turbo::cord_buf a;
        a.append("I'am a");
        turbo::cord_buf b;
        b.append("I'am b");
        std::swap(a, b);
        ASSERT_TRUE(a.equals("I'am b"));
        ASSERT_TRUE(b.equals("I'am a"));
    }

    TEST_F(CordBufTest, resize) {
        turbo::cord_buf a;
        a.resize(100);
        std::string as;
        as.resize(100);
        turbo::cord_buf b;
        b.resize(100, 'b');
        std::string bs;
        bs.resize(100, 'b');
        ASSERT_EQ(100u, a.length());
        ASSERT_EQ(100u, b.length());
        ASSERT_TRUE(a.equals(as));
        ASSERT_TRUE(b.equals(bs));
    }

    TEST_F(CordBufTest, iterate_bytes) {
        turbo::cord_buf a;
        a.append("hello world");
        std::string saved_a = a.to_string();
        size_t n = 0;
        turbo::cord_buf_bytes_iterator it(a);
        for (; it != nullptr; ++it, ++n) {
            ASSERT_EQ(saved_a[n], *it);
        }
        ASSERT_EQ(saved_a.size(), n);
        ASSERT_TRUE(saved_a == a);

        // append more to the iobuf, iterator should still be ended.
        a.append(", this is iobuf");
        ASSERT_TRUE(it == nullptr);

        // append more-than-one-block data to the iobuf
        for (int i = 0; i < 1024; ++i) {
            a.append("ab33jm4hgaklkv;2[25lj4lkj312kl4j321kl4j3k");
        }
        saved_a = a.to_string();
        n = 0;
        for (turbo::cord_buf_bytes_iterator it2(a); it2 != nullptr; it2++/*intended post++*/, ++n) {
            ASSERT_EQ(saved_a[n], *it2);
        }
        ASSERT_EQ(saved_a.size(), n);
        ASSERT_TRUE(saved_a == a);
    }

    TEST_F(CordBufTest, appender) {
        turbo::cord_buf_appender appender;
        ASSERT_EQ(0, appender.append("hello", 5));
        ASSERT_EQ("hello", appender.buf());
        ASSERT_EQ(0, appender.push_back(' '));
        ASSERT_EQ(0, appender.append("world", 5));
        ASSERT_EQ("hello world", appender.buf());
        turbo::cord_buf buf2;
        appender.move_to(buf2);
        ASSERT_EQ("", appender.buf());
        ASSERT_EQ("hello world", buf2);
        std::string str;
        for (int i = 0; i < 10000; ++i) {
            char buf[128];
            int len = snprintf(buf, sizeof(buf), "1%d2%d3%d4%d5%d", i, i, i, i, i);
            appender.append(buf, len);
            str.append(buf, len);
        }
        ASSERT_EQ(str, appender.buf());
        turbo::cord_buf buf3;
        appender.move_to(buf3);
        ASSERT_EQ("", appender.buf());
        ASSERT_EQ(str, buf3);
    }

    TEST_F(CordBufTest, appender_perf) {
        const size_t N1 = 100000;
        turbo::stop_watcher tm1;
        tm1.start();
        turbo::cord_buf buf1;
        for (size_t i = 0; i < N1; ++i) {
            buf1.push_back(i);
        }
        tm1.stop();

        turbo::stop_watcher tm2;
        tm2.start();
        turbo::cord_buf_appender appender1;
        for (size_t i = 0; i < N1; ++i) {
            appender1.push_back(i);
        }
        tm2.stop();

        TURBO_LOG(INFO) << "cord_buf.push_back=" << tm1.n_elapsed() / N1
                  << "ns cord_buf_appender.push_back=" << tm2.n_elapsed() / N1
                  << "ns";

        const size_t N2 = 50000;
        const std::string s = "a repeatly appended string";
        std::string str2;
        turbo::cord_buf buf2;
        tm1.start();
        for (size_t i = 0; i < N2; ++i) {
            buf2.append(s);
        }
        tm1.stop();

        tm2.start();
        turbo::cord_buf_appender appender2;
        for (size_t i = 0; i < N2; ++i) {
            appender2.append(s);
        }
        tm2.stop();

        turbo::stop_watcher tm3;
        tm3.start();
        for (size_t i = 0; i < N2; ++i) {
            str2.append(s);
        }
        tm3.stop();

        TURBO_LOG(INFO) << "cord_buf.append=" << tm1.n_elapsed() / N2
                  << "ns cord_buf_appender.append=" << tm2.n_elapsed() / N2
                  << "ns string.append=" << tm3.n_elapsed() / N2
                  << "ns (string-length=" << s.size() << ')';
    }

    TEST_F(CordBufTest, printed_as_binary) {
        turbo::cord_buf buf;
        std::string str;
        for (int i = 0; i < 256; ++i) {
            buf.push_back((char) i);
            str.push_back((char) i);
        }
        const char *const OUTPUT =
                "\\00\\01\\02\\03\\04\\05\\06\\07\\b\\t\\n\\0B\\0C\\r\\0E\\0F"
                "\\10\\11\\12\\13\\14\\15\\16\\17\\18\\19\\1A\\1B\\1C\\1D\\1E"
                "\\1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUV"
                "WXYZ[\\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\\7F\\80\\81\\82"
                "\\83\\84\\85\\86\\87\\88\\89\\8A\\8B\\8C\\8D\\8E\\8F\\90\\91"
                "\\92\\93\\94\\95\\96\\97\\98\\99\\9A\\9B\\9C\\9D\\9E\\9F\\A0"
                "\\A1\\A2\\A3\\A4\\A5\\A6\\A7\\A8\\A9\\AA\\AB\\AC\\AD\\AE\\AF"
                "\\B0\\B1\\B2\\B3\\B4\\B5\\B6\\B7\\B8\\B9\\BA\\BB\\BC\\BD\\BE"
                "\\BF\\C0\\C1\\C2\\C3\\C4\\C5\\C6\\C7\\C8\\C9\\CA\\CB\\CC\\CD"
                "\\CE\\CF\\D0\\D1\\D2\\D3\\D4\\D5\\D6\\D7\\D8\\D9\\DA\\DB\\DC"
                "\\DD\\DE\\DF\\E0\\E1\\E2\\E3\\E4\\E5\\E6\\E7\\E8\\E9\\EA\\EB"
                "\\EC\\ED\\EE\\EF\\F0\\F1\\F2\\F3\\F4\\F5\\F6\\F7\\F8\\F9\\FA"
                "\\FB\\FC\\FD\\FE\\FF";
        std::ostringstream os;
        os << turbo::to_printable(buf, 256);
        ASSERT_STREQ(OUTPUT, os.str().c_str());
        os.str("");
        os << turbo::to_printable(str, 256);
        ASSERT_STREQ(OUTPUT, os.str().c_str());
    }

    TEST_F(CordBufTest, copy_to_string_from_iterator) {
        turbo::cord_buf b0;
        for (size_t i = 0; i < 1 * 1024 * 1024lu; ++i) {
            b0.push_back(turbo::base::fast_rand_in('a', 'z'));
        }
        turbo::cord_buf b1(b0);
        turbo::cord_buf_bytes_iterator iter(b0);
        size_t nc = 0;
        while (nc < b0.length()) {
            size_t to_copy = turbo::base::fast_rand_in(1024lu, 64 * 1024lu);
            turbo::cord_buf b;
            b1.cutn(&b, to_copy);
            std::string s;
            const size_t copied = iter.copy_and_forward(&s, to_copy);
            ASSERT_GT(copied, 0u);
            ASSERT_TRUE(b.equals(s));
            nc += copied;
        }
        ASSERT_EQ(nc, b0.length());
    }

    static void *my_free_params = nullptr;

    static void my_free(void *m) {
        free(m);
        my_free_params = m;
    }

    TEST_F(CordBufTest, append_user_data_and_consume) {
        turbo::cord_buf b0;
        const int REP = 16;
        const size_t len = REP * 256;
        char *data = (char *) malloc(len);
        for (int i = 0; i < 256; ++i) {
            for (int j = 0; j < REP; ++j) {
                data[i * REP + j] = (char) i;
            }
        }
        my_free_params = nullptr;
        ASSERT_EQ(0, b0.append_user_data(data, len, my_free));
        ASSERT_EQ(1UL, b0._ref_num());
        turbo::cord_buf::BlockRef r = b0._front_ref();
        ASSERT_EQ(1, turbo::iobuf::block_shared_count(r.block));
        ASSERT_EQ(len, b0.size());
        std::string out;
        ASSERT_EQ(len, b0.cutn(&out, len));
        ASSERT_TRUE(b0.empty());
        ASSERT_EQ(data, my_free_params);

        ASSERT_EQ(len, out.size());
        // note: cannot memcmp with data which is already free-ed
        for (int i = 0; i < 256; ++i) {
            for (int j = 0; j < REP; ++j) {
                ASSERT_EQ((char) i, out[i * REP + j]);
            }
        }
    }

    TEST_F(CordBufTest, append_user_data_and_share) {
        turbo::cord_buf b0;
        const int REP = 16;
        const size_t len = REP * 256;
        char *data = (char *) malloc(len);
        for (int i = 0; i < 256; ++i) {
            for (int j = 0; j < REP; ++j) {
                data[i * REP + j] = (char) i;
            }
        }
        my_free_params = nullptr;
        ASSERT_EQ(0, b0.append_user_data(data, len, my_free));
        ASSERT_EQ(1UL, b0._ref_num());
        turbo::cord_buf::BlockRef r = b0._front_ref();
        ASSERT_EQ(1, turbo::iobuf::block_shared_count(r.block));
        ASSERT_EQ(len, b0.size());

        {
            turbo::cord_buf bufs[256];
            for (int i = 0; i < 256; ++i) {
                ASSERT_EQ((size_t) REP, b0.cutn(&bufs[i], REP));
                ASSERT_EQ(len - (i + 1) * REP, b0.size());
                if (i != 255) {
                    ASSERT_EQ(1UL, b0._ref_num());
                    turbo::cord_buf::BlockRef r = b0._front_ref();
                    ASSERT_EQ(i + 2, turbo::iobuf::block_shared_count(r.block));
                } else {
                    ASSERT_EQ(0UL, b0._ref_num());
                    ASSERT_TRUE(b0.empty());
                }
            }
            ASSERT_EQ(nullptr, my_free_params);
            for (int i = 0; i < 256; ++i) {
                std::string out = bufs[i].to_string();
                ASSERT_EQ((size_t) REP, out.size());
                for (int j = 0; j < REP; ++j) {
                    ASSERT_EQ((char) i, out[j]);
                }
            }
        }
        ASSERT_EQ(data, my_free_params);
    }

    TEST_F(CordBufTest, share_tls_block) {
        turbo::iobuf::remove_tls_block_chain();
        turbo::cord_buf::Block *b = turbo::iobuf::acquire_tls_block();
        ASSERT_EQ(0u, turbo::iobuf::block_size(b));

        turbo::cord_buf::Block *b2 = turbo::iobuf::share_tls_block();
        turbo::cord_buf buf;
        for (size_t i = 0; i < turbo::iobuf::block_cap(b2); i++) {
            buf.push_back('x');
        }
        // after pushing to b2, b2 is full but it is still head of tls block.
        ASSERT_NE(b, b2);
        turbo::iobuf::release_tls_block_chain(b);
        ASSERT_EQ(b, turbo::iobuf::share_tls_block());
        // After releasing b, now tls block is b(not full) -> b2(full) -> nullptr
        for (size_t i = 0; i < turbo::iobuf::block_cap(b); i++) {
            buf.push_back('x');
        }
        // now tls block is b(full) -> b2(full) -> nullptr
        turbo::cord_buf::Block *head_block = turbo::iobuf::share_tls_block();
        ASSERT_EQ(0u, turbo::iobuf::block_size(head_block));
        ASSERT_NE(b, head_block);
        ASSERT_NE(b2, head_block);
    }

    TEST_F(CordBufTest, acquire_tls_block) {
        turbo::iobuf::remove_tls_block_chain();
        turbo::cord_buf::Block *b = turbo::iobuf::acquire_tls_block();
        const size_t block_cap = turbo::iobuf::block_cap(b);
        turbo::cord_buf buf;
        for (size_t i = 0; i < block_cap; i++) {
            buf.append("x");
        }
        ASSERT_EQ(1, turbo::iobuf::get_tls_block_count());
        turbo::cord_buf::Block *head = turbo::iobuf::get_tls_block_head();
        ASSERT_EQ(turbo::iobuf::block_cap(head), turbo::iobuf::block_size(head));
        turbo::iobuf::release_tls_block_chain(b);
        ASSERT_EQ(2, turbo::iobuf::get_tls_block_count());
        for (size_t i = 0; i < block_cap; i++) {
            buf.append("x");
        }
        ASSERT_EQ(2, turbo::iobuf::get_tls_block_count());
        head = turbo::iobuf::get_tls_block_head();
        ASSERT_EQ(turbo::iobuf::block_cap(head), turbo::iobuf::block_size(head));
        b = turbo::iobuf::acquire_tls_block();
        ASSERT_EQ(0, turbo::iobuf::get_tls_block_count());
        ASSERT_NE(turbo::iobuf::block_cap(b), turbo::iobuf::block_size(b));
    }

} // namespace