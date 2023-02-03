#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "turbo/container/serializer.h"
#include "gtest/gtest.h"
#include <random>

// ----------------------------------------------------------------------------
// Random generator utilities
// ----------------------------------------------------------------------------

// Function: random_engine
inline std::default_random_engine& random_engine() {
  thread_local std::default_random_engine gen{0};
  return gen;
}

// Function: random
// Randomly generate a floating value in the given range.
template <typename T>
std::enable_if_t<std::is_floating_point<T>::value, T> random(
  const T from = -1.0,
  const T to = 1.0
) {
  return std::uniform_real_distribution<T>(from, to)(random_engine());
}

// Function: random
// Randomly generate an integer value.
template <typename T>
std::enable_if_t<std::is_integral<T>::value, T> random(
  const T from = std::numeric_limits<T>::lowest(),
  const T to = std::numeric_limits<T>::max()
) {
  return std::uniform_int_distribution<T>(from, to)(random_engine());
}

// Function: random
// Randomly generate a string.
template <typename T>
std::enable_if_t<std::is_same<T, std::string>::value, T> random(
  const std::string::value_type from = ' ',
  const std::string::value_type to = '~',
  const std::string::size_type len = 16
) {
  std::string str(len, ' ');
  for(auto& c : str) {
    c = random<int>(from, to) % 128;
  }
  return str;
}

// ----------------------------------------------------------------------------

// Struct: PODs
struct PODs {

  unsigned      pod_uint32 = random<decltype(pod_uint32)>();
  int           pod_int32  = random<decltype(pod_int32)>();
  unsigned long long pod_uint64 = random<decltype(pod_uint64)>();
  long long     pod_int64  = random<decltype(pod_int64)>();
  float_t       pod_float  = random<decltype(pod_float)>();
  double_t      pod_double = random<decltype(pod_double)>();

  template <typename ArchiverT>
  auto save( ArchiverT& ar ) const {
    return ar(
      pod_uint32,
      pod_int32,
      pod_uint64,
      pod_int64,
      pod_float,
      pod_double
    );
  }

  template <typename ArchiverT>
  auto load( ArchiverT& ar ) {
    return ar(
      pod_uint32,
      pod_int32,
      pod_uint64,
      pod_int64,
      pod_float,
      pod_double
    );
  }

  bool operator == (const PODs& rhs) const {
    return pod_uint32 == rhs.pod_uint32 &&
           pod_int32  == rhs.pod_int32  &&
           pod_uint64 == rhs.pod_uint64 &&
           pod_int64  == rhs.pod_int64  &&
           pod_float  == rhs.pod_float  &&
           pod_double == rhs.pod_double;
  }

  bool operator != (const PODs& rhs) const {
    return !(*this == rhs);
  }
};

// Procedure: test_pod
// The templated procedure for testing POD. Caller must specify the output
// and input archiver type.
void test_pod() {

  // Output stream.
  std::ostringstream os;
  turbo::Serializer oar(os);

  const auto o_uint32 = random<unsigned>();
  const auto o_int32  = random<int>();
  const auto o_uint64 = random<unsigned long long>();
  const auto o_int64  = random<long long>();
  const auto o_float  = random<float>();
  const auto o_double = random<double>();

  auto o_sz = oar(
    o_uint32,
    o_int32,
    o_uint64,
    o_int64,
    o_float,
    o_double
  );

  //EXPECT_TRUE(o_sz == os.out_avail());

  // InputStreamBuffer
  std::istringstream is(os.str());
  turbo::Deserializer iar(is);

  auto i_uint32 = random<unsigned>();
  auto i_int32  = random<int>();
  auto i_uint64 = random<unsigned long long>();
  auto i_int64  = random<long long>();
  auto i_float  = random<float>();
  auto i_double = random<double>();

  auto i_sz = iar(
    i_uint32,
    i_int32,
    i_uint64,
    i_int64,
    i_float,
    i_double
  );
  EXPECT_TRUE(is.rdbuf()->in_avail() == 0);

  EXPECT_TRUE(i_sz == o_sz);
  EXPECT_TRUE(o_uint32 == i_uint32);
  EXPECT_TRUE(o_int32 == i_int32);
  EXPECT_TRUE(o_uint64 == i_uint64);
  EXPECT_TRUE(o_int64 == i_int64);
  EXPECT_TRUE(o_float == i_float);
  EXPECT_TRUE(o_double == i_double);
}

// Procedure: test_struct
// The templated procedure for testing POD. Caller must specify the output
// and input archiver type.
void test_struct() {

  for(size_t i=0; i<64; ++i) {

    // POD struct.
    PODs o_pods;
    PODs i_pods;

    // Outputstream
    std::ostringstream os;
    turbo::Serializer oar(os);
    auto o_sz = oar(o_pods);
    //EXPECT_TRUE(o_sz == os.out_avail());

    // Inputstream
    std::istringstream is(os.str());
    turbo::Deserializer iar(is);
    auto i_sz = iar(i_pods);
    EXPECT_TRUE(is.rdbuf()->in_avail() == 0);

    EXPECT_TRUE(o_sz == i_sz);
    EXPECT_TRUE(o_pods == i_pods);
  }
}

// Procedure: test_string
// Template for testing basic strings. Caller must specify the output and input archiver type.
template <typename T>
void test_string() {

  for(size_t i=0; i<4096; i++) {

    // Outputstream
    std::ostringstream os;
    turbo::Serializer oar(os);

    T o_char_str = random<T>();
    auto o_sz = oar(o_char_str);
    //EXPECT_TRUE(o_sz == os.out_avail());

    // Inputstream
    std::istringstream is(os.str());
    turbo::Deserializer iar(is);

    T i_char_str;
    auto i_sz = iar(i_char_str);
    EXPECT_TRUE(is.rdbuf()->in_avail() == 0);

    EXPECT_TRUE(o_sz == i_sz);
    EXPECT_TRUE(o_char_str == i_char_str);
  }
}

#define TEST_SEQ_CONT_BODY(container)                          \
                                                               \
for(size_t i=0; i<64; i++) {                                   \
  const size_t num_data = random<size_t>(1, 1024);             \
  std::ostringstream os;                                       \
  turbo::Serializer oar(os);                                      \
                                                               \
  std::container <int>     o_int32s  (num_data);               \
  std::container <long long>     o_int64s  (num_data);         \
  std::container <char>        o_chars   (num_data);           \
  std::container <float>       o_floats  (num_data);           \
  std::container <double>      o_doubles (num_data);           \
  std::container <std::string> o_strings (num_data);           \
  std::container <PODs>        o_podses  (num_data);           \
                                                               \
  for(auto& v : o_int32s)  v = random<int>();                  \
  for(auto& v : o_int64s)  v = random<long long>();            \
  for(auto& v : o_chars)   v = random<int>();                  \
  for(auto& v : o_floats)  v = random<float>();                \
  for(auto& v : o_doubles) v = random<double>();               \
  for(auto& v : o_strings) v = random<std::string>();          \
                                                               \
  auto o_sz = oar(o_int32s, o_int64s, o_chars, o_floats, o_doubles, o_strings, o_podses);\
                                                               \
  std::istringstream is(os.str());                             \
  turbo::Deserializer iar(is);                                    \
                                                               \
  std::container <int>     i_int32s;                           \
  std::container <long long>     i_int64s;                     \
  std::container <char>        i_chars;                        \
  std::container <float>       i_floats;                       \
  std::container <double>      i_doubles;                      \
  std::container <std::string> i_strings;                      \
  std::container <PODs>        i_podses;                       \
                                                               \
  auto i_sz = iar(i_int32s, i_int64s, i_chars, i_floats, i_doubles, i_strings, i_podses);\
                                   \
  EXPECT_TRUE(o_sz == i_sz);           \
  EXPECT_TRUE(o_int32s == i_int32s);   \
  EXPECT_TRUE(o_int64s == i_int64s);   \
  EXPECT_TRUE(o_chars == i_chars);     \
  EXPECT_TRUE(o_floats == i_floats);   \
  EXPECT_TRUE(o_doubles == i_doubles); \
  EXPECT_TRUE(o_strings == i_strings); \
  EXPECT_TRUE(o_podses == i_podses);   \
}


#define TEST_MAP_CONT_BODY(container)                                            \
                                                                                 \
for (size_t i = 0; i < 64; i++) {                                              \
  const size_t num_data = random<size_t>(1, 1024);                               \
  std::ostringstream os;                                                         \
  turbo::Serializer oar(os);                                                            \
                                                                                 \
  std::container<int, int> o_int32s;                                     \
  std::container<long long, long long> o_int64s;                                     \
  std::container<char, char> o_chars;                                            \
  std::container<float, float> o_floats;                                         \
  std::container<double, double> o_doubles;                                      \
  std::container<std::string, std::string> o_strings;                            \
                                                                                 \
  for (size_t j = 0; j < num_data; j++) {                                        \
    o_int32s.emplace(random<int>(), random<int>());                      \
    o_int64s.emplace(random<long long>(), random<long long>());                      \
    o_chars.emplace(random<int>(), random<int>());                             \
    o_floats.emplace(random<float_t>(), random<float_t>());                      \
    o_doubles.emplace(random<double_t>(), random<double_t>());                   \
    o_strings.emplace(random<std::string>(), random<std::string>());             \
  }                                                                              \
                                                                                 \
  auto o_sz = oar(o_int32s, o_int64s, o_chars, o_floats, o_doubles , o_strings); \
                                                                                 \
  std::istringstream is(os.str());                                               \
  turbo::Deserializer iar(is);                                                            \
                                                                                 \
  std::container<int, int> i_int32s;                                     \
  std::container<long long, long long> i_int64s;                                     \
  std::container<char, char> i_chars;                                            \
  std::container<float, float> i_floats;                                         \
  std::container<double, double> i_doubles;                                      \
  std::container<std::string, std::string> i_strings;                            \
                                                                                 \
  auto i_sz = iar(i_int32s, i_int64s, i_chars, i_floats, i_doubles , i_strings); \
  EXPECT_TRUE(0 == is.rdbuf()->in_avail());                                          \
                                                                                 \
  EXPECT_TRUE(o_sz == i_sz);                                                         \
  EXPECT_TRUE(o_int32s == i_int32s);                                                 \
  EXPECT_TRUE(o_int64s == i_int64s);                                                 \
  EXPECT_TRUE(o_chars == i_chars);                                                   \
  EXPECT_TRUE(o_floats == i_floats);                                                 \
  EXPECT_TRUE(o_doubles == i_doubles);                                               \
  EXPECT_TRUE(o_strings == i_strings);                                               \
}


#define TEST_SET_CONT_BODY(container)                                           \
                                                                                \
for (size_t i = 0; i < 64; i++) {                                             \
  const size_t num_data = random<size_t>(1, 1024);                              \
  std::ostringstream os;                                                        \
  turbo::Serializer oar(os);                                             \
                                                                                \
  std::container<int> o_int32s;                                             \
  std::container<long long> o_int64s;                                             \
  std::container<char> o_chars;                                                 \
  std::container<float> o_floats;                                               \
  std::container<double> o_doubles;                                             \
  std::container<std::string> o_strings;                                        \
                                                                                \
  for (size_t j = 0; j < num_data; j++) {                                       \
    o_int32s.emplace(random<int>());                                        \
    o_int64s.emplace(random<long long>());                                        \
    o_chars.emplace(random<int>());                                            \
    o_floats.emplace(random<float_t>());                                        \
    o_doubles.emplace(random<double_t>());                                      \
    o_strings.emplace(random<std::string>());                                   \
  }                                                                             \
  auto o_sz = oar(o_int32s, o_int64s, o_chars, o_floats, o_doubles, o_strings); \
                                                                                \
  std::istringstream is(os.str());                                              \
  turbo::Deserializer iar(is);                                             \
                                                                                \
  std::container<int> i_int32s;                                             \
  std::container<long long> i_int64s;                                             \
  std::container<char> i_chars;                                                 \
  std::container<float> i_floats;                                               \
  std::container<double> i_doubles;                                             \
  std::container<std::string> i_strings;                                        \
                                                                                \
  auto i_sz = iar(i_int32s, i_int64s, i_chars, i_floats, i_doubles, i_strings); \
  EXPECT_TRUE(0 == is.rdbuf()->in_avail());                                         \
                                                                                \
  EXPECT_TRUE(o_sz == i_sz);                                                        \
  EXPECT_TRUE(o_int32s == i_int32s);                                                \
  EXPECT_TRUE(o_int64s == i_int64s);                                                \
  EXPECT_TRUE(o_chars == i_chars);                                                  \
  EXPECT_TRUE(o_floats == i_floats);                                                \
  EXPECT_TRUE(o_doubles == i_doubles);                                              \
  EXPECT_TRUE(o_strings == i_strings);                                              \
}

// Procedure: test_array
// Template procedure for testing array container.
void test_array() {

  for(size_t i=0; i<64; ++i) {

    // Output
    std::array<char, 1> ochar;
    std::array<int, 512> oint;
    std::array<double, 1024> odouble;
    std::array<std::string, 2048> ostring;

    for(auto &i : ochar) i = random<int>();
    for(auto &i : oint) i = random<int>();
    for(auto &i : odouble) i = random<double>();
    for(auto &i : ostring) i = random<std::string>();

    std::ostringstream os;
    turbo::Serializer oar(os);
    auto osz = oar(ochar, oint, odouble, ostring);

    // Input
    std::array<char, 1> ichar;
    std::array<int, 512> iint;
    std::array<double, 1024> idouble;
    std::array<std::string, 2048> istring;

    std::istringstream is(os.str());
    turbo::Deserializer iar(is);

    auto isz = iar(ichar, iint, idouble, istring);
    EXPECT_TRUE(0 == is.rdbuf()->in_avail());

    EXPECT_TRUE(osz == isz);
    EXPECT_TRUE(ochar == ichar);
    EXPECT_TRUE(oint == iint);
    EXPECT_TRUE(odouble == idouble);
    EXPECT_TRUE(ostring == istring);
  }
}

// Procedure: test_variant
void test_variant() {

  for (size_t i = 0; i < 64; i++) {

    // Single POD variant.
    std::variant<int> opod1 = random<int>();
    std::variant<int> ipod1 = random<int>();

    // Multiple POD variant
    std::variant<int, double> opod2 = random<double>();
    std::variant<int, double> ipod2 = random<int>();

    // Multiple POD variant
    std::variant<int, double, bool> opod3 = random<int>()%2;
    std::variant<int, double, bool> ipod3 = random<double>();

    // Mixing float and string
    std::variant<float, std::string> omix2 = random<std::string>();
    std::variant<float, std::string> imix2 = random<float>();

    // Recursive variant
    std::variant<int, decltype(omix2)> orec2 = omix2;
    std::variant<int, decltype(omix2)> irec2 = random<int>();

    // Output archiver
    std::ostringstream os;
    turbo::Serializer oar(os);
    auto osz = oar(opod1, opod2, opod3, omix2, orec2);

    // Input archiver
    std::istringstream is(os.str());
    turbo::Deserializer iar(is);
    auto isz = iar(ipod1, ipod2, ipod3, imix2, irec2);
    EXPECT_TRUE(0 == is.rdbuf()->in_avail());

    EXPECT_TRUE(osz == isz);
    EXPECT_TRUE(opod1 == ipod1);
    EXPECT_TRUE(opod2 == ipod2);
    EXPECT_TRUE(opod3 == ipod3);
    EXPECT_TRUE(omix2 == imix2);
    EXPECT_TRUE(orec2 == irec2);
  }
}

// Procedure: test_time_point
void test_time_point() {

  for(auto i=0; i<64; ++i) {

    auto o_tpt1 = std::chrono::system_clock::now();
    auto o_tpt2 = std::chrono::steady_clock::now();
    auto o_tpt3 = std::chrono::high_resolution_clock::now();

    auto o_dur1 = std::chrono::system_clock::now() - o_tpt1;
    auto o_dur2 = std::chrono::steady_clock::now() - o_tpt2;
    auto o_dur3 = std::chrono::high_resolution_clock::now() - o_tpt3;

    // Output archiver
    std::ostringstream os;
    turbo::Serializer oar(os);
    auto osz = oar(o_tpt1, o_tpt2, o_tpt3, o_dur1, o_dur2, o_dur3);

    decltype(o_tpt1) i_tpt1;
    decltype(o_tpt2) i_tpt2;
    decltype(o_tpt3) i_tpt3;
    decltype(o_dur1) i_dur1;
    decltype(o_dur2) i_dur2;
    decltype(o_dur3) i_dur3;

    // Input archiver
    std::istringstream is(os.str());
    turbo::Deserializer iar(is);
    auto isz = iar(i_tpt1, i_tpt2, i_tpt3, i_dur1, i_dur2, i_dur3);

    EXPECT_TRUE(0 == is.rdbuf()->in_avail());
    EXPECT_TRUE(osz == isz);

    EXPECT_TRUE(o_tpt1 == i_tpt1);
    EXPECT_TRUE(o_tpt2 == i_tpt2);
    EXPECT_TRUE(o_tpt3 == i_tpt3);
    EXPECT_TRUE(o_dur1 == i_dur1);
    EXPECT_TRUE(o_dur2 == i_dur2);
    EXPECT_TRUE(o_dur3 == i_dur3);
  }
}

// Procedure: test_optional
void test_optional() {

  for(auto i=0; i<64; ++i) {

    std::optional<bool> o_nbool, i_nbool{true};
    std::optional<bool> o_ybool{true}, i_ybool;

    std::optional<std::string> o_nstr, i_nstr{random<std::string>()};
    std::optional<std::string> o_ystr{random<std::string>()}, i_ystr;

    // Output archiver
    std::ostringstream os;
    turbo::Serializer oar(os);
    auto osz = oar(o_nbool, o_ybool, o_nstr, o_ystr);

    // Input archiver
    std::istringstream is(os.str());
    turbo::Deserializer iar(is);
    auto isz = iar(i_nbool, i_ybool, i_nstr, i_ystr);
    EXPECT_TRUE(0 == is.rdbuf()->in_avail());
    EXPECT_TRUE(osz == isz);

    EXPECT_TRUE(o_nbool == i_nbool);
    EXPECT_TRUE(o_ybool == i_ybool);
    EXPECT_TRUE(o_nstr == i_nstr);
    EXPECT_TRUE(o_ystr == i_ystr);
  }
}

// Procedure: test_tuple
void test_tuple() {

  for(auto i=0; i<64; ++i) {

    std::tuple<> o0, i0;
    std::tuple<char> o1 {'a'}, i1 {'b'};
    std::tuple<int, double> o2 {1, 2.4}, i2 {3, 0.9};
    std::tuple<std::string, std::vector<int>, float> o3{"123", {1, 2, 3}, 4.5f}, i3;
    std::tuple<int, std::tuple<int, int>, int> o4 {1, {2, 3}, 4}, i4;

    // Output archiver
    std::ostringstream os;
    turbo::Serializer oar(os);
    auto osz = oar(o0, o1, o2, o3, o4);

    // Input archiver
    std::istringstream is(os.str());
    turbo::Deserializer iar(is);
    auto isz = iar(i0, i1, i2, i3, i4);

    EXPECT_TRUE(0 == is.rdbuf()->in_avail());
    EXPECT_TRUE(osz == isz);

    EXPECT_TRUE(o0 == i0);
    EXPECT_TRUE(o1 == i1);
    EXPECT_TRUE(o2 == i2);
    EXPECT_TRUE(o3 == i3);
    EXPECT_TRUE(o4 == i4);
  }
}

// ----------------------------------------------------------------------------

// POD
TEST(serializer, POD) {
  test_pod();
}

// POD-struct
TEST(serializer, POD_Struct) {
  test_struct();
}

// std::string
TEST(serializer, string) {
  test_string<std::string>();
}

// std::vector
TEST(serializer, std_vector) {
  TEST_SEQ_CONT_BODY(vector)
}

// std::deque
TEST(serializer, deque) {
  TEST_SEQ_CONT_BODY(deque)
}

// std::list
TEST(serializer, list) {
  TEST_SEQ_CONT_BODY(list)
}

// std::forward_list
TEST(serializer, forward_list) {
  TEST_SEQ_CONT_BODY(forward_list)
}

// std::map
TEST(serializer, map) {
  TEST_MAP_CONT_BODY(map);
}

// std::unordered_map
TEST(serializer, unordered_map) {
  TEST_MAP_CONT_BODY(unordered_map);
}

// std::set
TEST(serializer, set) {
  TEST_SET_CONT_BODY(set);
}

// std::unordered_set
TEST(serializer, unordered_set) {
  TEST_SET_CONT_BODY(unordered_set);
}

// std::array
TEST(serializer, array) {
  test_array();
}

// std::chrono::time_point
TEST(serializer, time_point) {
  test_time_point();
}

// std::optional
TEST(serializer, optional) {
  test_optional();
}

// std::tuple
TEST(serializer, tuple) {
  test_tuple();
}
