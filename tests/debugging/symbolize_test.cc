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

#include <turbo/debugging/symbolize.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#endif

#include <cstring>
#include <iostream>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <turbo/base/casts.h>
#include <turbo/base/config.h>
#include <turbo/base/internal/per_thread_tls.h>
#include <turbo/base/optimization.h>
#include <turbo/debugging/internal/stack_consumption.h>
#include <turbo/log/check.h>
#include <turbo/log/log.h>
#include <turbo/memory/memory.h>
#include <turbo/strings/string_view.h>

#if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

using testing::Contains;

#ifdef _WIN32
#define TURBO_SYMBOLIZE_TEST_NOINLINE __declspec(noinline)
#else
#define TURBO_SYMBOLIZE_TEST_NOINLINE TURBO_ATTRIBUTE_NOINLINE
#endif

// Functions to symbolize. Use C linkage to avoid mangled names.
extern "C" {
TURBO_SYMBOLIZE_TEST_NOINLINE void nonstatic_func() {
  // The next line makes this a unique function to prevent the compiler from
  // folding identical functions together.
  volatile int x = __LINE__;
  static_cast<void>(x);
  TURBO_BLOCK_TAIL_CALL_OPTIMIZATION();
}

TURBO_SYMBOLIZE_TEST_NOINLINE static void static_func() {
  // The next line makes this a unique function to prevent the compiler from
  // folding identical functions together.
  volatile int x = __LINE__;
  static_cast<void>(x);
  TURBO_BLOCK_TAIL_CALL_OPTIMIZATION();
}
}  // extern "C"

struct Foo {
  static void func(int x);
};

// A C++ method that should have a mangled name.
TURBO_SYMBOLIZE_TEST_NOINLINE void Foo::func(int) {
  // The next line makes this a unique function to prevent the compiler from
  // folding identical functions together.
  volatile int x = __LINE__;
  static_cast<void>(x);
  TURBO_BLOCK_TAIL_CALL_OPTIMIZATION();
}

// Create functions that will remain in different text sections in the
// final binary when linker option "-z,keep-text-section-prefix" is used.
int TURBO_ATTRIBUTE_SECTION_VARIABLE(.text.unlikely) unlikely_func() {
  return 0;
}

int TURBO_ATTRIBUTE_SECTION_VARIABLE(.text.hot) hot_func() { return 0; }

int TURBO_ATTRIBUTE_SECTION_VARIABLE(.text.startup) startup_func() { return 0; }

int TURBO_ATTRIBUTE_SECTION_VARIABLE(.text.exit) exit_func() { return 0; }

int /*TURBO_ATTRIBUTE_SECTION_VARIABLE(.text)*/ regular_func() { return 0; }

// Thread-local data may confuse the symbolizer, ensure that it does not.
// Variable sizes and order are important.
#if TURBO_PER_THREAD_TLS
static TURBO_PER_THREAD_TLS_KEYWORD char symbolize_test_thread_small[1];
static TURBO_PER_THREAD_TLS_KEYWORD char
    symbolize_test_thread_big[2 * 1024 * 1024];
#endif

#if !defined(__EMSCRIPTEN__)
static void *GetPCFromFnPtr(void *ptr) { return ptr; }

// Used below to hopefully inhibit some compiler/linker optimizations
// that may remove kHpageTextPadding, kPadding0, and kPadding1 from
// the binary.
static volatile bool volatile_bool = false;

// Force the binary to be large enough that a THP .text remap will succeed.
static constexpr size_t kHpageSize = 1 << 21;
const char kHpageTextPadding[kHpageSize * 4] TURBO_ATTRIBUTE_SECTION_VARIABLE(
        .text) = "";

#else
static void *GetPCFromFnPtr(void *ptr) {
  return EM_ASM_PTR(
      { return wasmOffsetConverter.convert(wasmTable.get($0).name, 0); }, ptr);
}

#endif  // !defined(__EMSCRIPTEN__)

static char try_symbolize_buffer[4096];

// A wrapper function for turbo::Symbolize() to make the unit test simple.  The
// limit must be < sizeof(try_symbolize_buffer).  Returns null if
// turbo::Symbolize() returns false, otherwise returns try_symbolize_buffer with
// the result of turbo::Symbolize().
static const char *TrySymbolizeWithLimit(void *pc, int limit) {
  CHECK_LE(limit, sizeof(try_symbolize_buffer))
      << "try_symbolize_buffer is too small";

  // Use the heap to facilitate heap and buffer sanitizer tools.
  auto heap_buffer = turbo::make_unique<char[]>(sizeof(try_symbolize_buffer));
  bool found = turbo::Symbolize(pc, heap_buffer.get(), limit);
  if (found) {
    CHECK_LT(static_cast<int>(
                 strnlen(heap_buffer.get(), static_cast<size_t>(limit))),
             limit)
        << "turbo::Symbolize() did not properly terminate the string";
    strncpy(try_symbolize_buffer, heap_buffer.get(),
            sizeof(try_symbolize_buffer) - 1);
    try_symbolize_buffer[sizeof(try_symbolize_buffer) - 1] = '\0';
  }

  return found ? try_symbolize_buffer : nullptr;
}

// A wrapper for TrySymbolizeWithLimit(), with a large limit.
static const char *TrySymbolize(void *pc) {
  return TrySymbolizeWithLimit(pc, sizeof(try_symbolize_buffer));
}

#if defined(TURBO_INTERNAL_HAVE_ELF_SYMBOLIZE) ||    \
    defined(TURBO_INTERNAL_HAVE_DARWIN_SYMBOLIZE) || \
    defined(TURBO_INTERNAL_HAVE_EMSCRIPTEN_SYMBOLIZE)

// Test with a return address.
void TURBO_ATTRIBUTE_NOINLINE TestWithReturnAddress() {
#if defined(TURBO_HAVE_ATTRIBUTE_NOINLINE)
  void *return_address = __builtin_return_address(0);
  const char *symbol = TrySymbolize(return_address);
  CHECK_NE(symbol, nullptr) << "TestWithReturnAddress failed";
  CHECK_STREQ(symbol, "main") << "TestWithReturnAddress failed";
  std::cout << "TestWithReturnAddress passed" << std::endl;
#endif
}

TEST(Symbolize, Cached) {
  // Compilers should give us pointers to them.
  EXPECT_STREQ("nonstatic_func",
               TrySymbolize(GetPCFromFnPtr((void *)(&nonstatic_func))));
  // The name of an internal linkage symbol is not specified; allow either a
  // mangled or an unmangled name here.
  const char *static_func_symbol =
      TrySymbolize(GetPCFromFnPtr((void *)(&static_func)));
  EXPECT_TRUE(strcmp("static_func", static_func_symbol) == 0 ||
              strcmp("static_func()", static_func_symbol) == 0);

  EXPECT_TRUE(nullptr == TrySymbolize(nullptr));
}

TEST(Symbolize, Truncation) {
  constexpr char kNonStaticFunc[] = "nonstatic_func";
  EXPECT_STREQ("nonstatic_func",
               TrySymbolizeWithLimit(GetPCFromFnPtr((void *)(&nonstatic_func)),
                                     strlen(kNonStaticFunc) + 1));
  EXPECT_STREQ("nonstatic_...",
               TrySymbolizeWithLimit(GetPCFromFnPtr((void *)(&nonstatic_func)),
                                     strlen(kNonStaticFunc) + 0));
  EXPECT_STREQ("nonstatic...",
               TrySymbolizeWithLimit(GetPCFromFnPtr((void *)(&nonstatic_func)),
                                     strlen(kNonStaticFunc) - 1));
  EXPECT_STREQ("n...", TrySymbolizeWithLimit(
                           GetPCFromFnPtr((void *)(&nonstatic_func)), 5));
  EXPECT_STREQ("...", TrySymbolizeWithLimit(
                          GetPCFromFnPtr((void *)(&nonstatic_func)), 4));
  EXPECT_STREQ("..", TrySymbolizeWithLimit(
                         GetPCFromFnPtr((void *)(&nonstatic_func)), 3));
  EXPECT_STREQ(
      ".", TrySymbolizeWithLimit(GetPCFromFnPtr((void *)(&nonstatic_func)), 2));
  EXPECT_STREQ(
      "", TrySymbolizeWithLimit(GetPCFromFnPtr((void *)(&nonstatic_func)), 1));
  EXPECT_EQ(nullptr, TrySymbolizeWithLimit(
                         GetPCFromFnPtr((void *)(&nonstatic_func)), 0));
}

TEST(Symbolize, SymbolizeWithDemangling) {
  Foo::func(100);
#ifdef __EMSCRIPTEN__
  // Emscripten's online symbolizer is more precise with arguments.
  EXPECT_STREQ("Foo::func(int)",
               TrySymbolize(GetPCFromFnPtr((void *)(&Foo::func))));
#else
  EXPECT_STREQ("Foo::func()",
               TrySymbolize(GetPCFromFnPtr((void *)(&Foo::func))));
#endif
}

TEST(Symbolize, SymbolizeSplitTextSections) {
  EXPECT_STREQ("unlikely_func()",
               TrySymbolize(GetPCFromFnPtr((void *)(&unlikely_func))));
  EXPECT_STREQ("hot_func()", TrySymbolize(GetPCFromFnPtr((void *)(&hot_func))));
  EXPECT_STREQ("startup_func()",
               TrySymbolize(GetPCFromFnPtr((void *)(&startup_func))));
  EXPECT_STREQ("exit_func()",
               TrySymbolize(GetPCFromFnPtr((void *)(&exit_func))));
  EXPECT_STREQ("regular_func()",
               TrySymbolize(GetPCFromFnPtr((void *)(&regular_func))));
}

// Tests that verify that Symbolize stack footprint is within some limit.
#ifdef TURBO_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION

static void *g_pc_to_symbolize;
static char g_symbolize_buffer[4096];
static char *g_symbolize_result;

static void SymbolizeSignalHandler(int signo) {
  if (turbo::Symbolize(g_pc_to_symbolize, g_symbolize_buffer,
                      sizeof(g_symbolize_buffer))) {
    g_symbolize_result = g_symbolize_buffer;
  } else {
    g_symbolize_result = nullptr;
  }
}

// Call Symbolize and figure out the stack footprint of this call.
static const char *SymbolizeStackConsumption(void *pc, int *stack_consumed) {
  g_pc_to_symbolize = pc;
  *stack_consumed = turbo::debugging_internal::GetSignalHandlerStackConsumption(
      SymbolizeSignalHandler);
  return g_symbolize_result;
}

static int GetStackConsumptionUpperLimit() {
  // Symbolize stack consumption should be within 2kB.
  int stack_consumption_upper_limit = 2048;
#if defined(TURBO_HAVE_ADDRESS_SANITIZER) || \
    defined(TURBO_HAVE_MEMORY_SANITIZER) || defined(TURBO_HAVE_THREAD_SANITIZER)
  // Account for sanitizer instrumentation requiring additional stack space.
  stack_consumption_upper_limit *= 5;
#endif
  return stack_consumption_upper_limit;
}

TEST(Symbolize, SymbolizeStackConsumption) {
  int stack_consumed = 0;

  const char *symbol =
      SymbolizeStackConsumption((void *)(&nonstatic_func), &stack_consumed);
  EXPECT_STREQ("nonstatic_func", symbol);
  EXPECT_GT(stack_consumed, 0);
  EXPECT_LT(stack_consumed, GetStackConsumptionUpperLimit());

  // The name of an internal linkage symbol is not specified; allow either a
  // mangled or an unmangled name here.
  symbol = SymbolizeStackConsumption((void *)(&static_func), &stack_consumed);
  EXPECT_TRUE(strcmp("static_func", symbol) == 0 ||
              strcmp("static_func()", symbol) == 0);
  EXPECT_GT(stack_consumed, 0);
  EXPECT_LT(stack_consumed, GetStackConsumptionUpperLimit());
}

TEST(Symbolize, SymbolizeWithDemanglingStackConsumption) {
  Foo::func(100);
  int stack_consumed = 0;

  const char *symbol =
      SymbolizeStackConsumption((void *)(&Foo::func), &stack_consumed);

  EXPECT_STREQ("Foo::func()", symbol);
  EXPECT_GT(stack_consumed, 0);
  EXPECT_LT(stack_consumed, GetStackConsumptionUpperLimit());
}

#endif  // TURBO_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION

#if !defined(TURBO_INTERNAL_HAVE_DARWIN_SYMBOLIZE) && \
    !defined(TURBO_INTERNAL_HAVE_EMSCRIPTEN_SYMBOLIZE)
// Use a 64K page size for PPC.
const size_t kPageSize = 64 << 10;
// We place a read-only symbols into the .text section and verify that we can
// symbolize them and other symbols after remapping them.
const char kPadding0[kPageSize * 4] TURBO_ATTRIBUTE_SECTION_VARIABLE(.text) = "";
const char kPadding1[kPageSize * 4] TURBO_ATTRIBUTE_SECTION_VARIABLE(.text) = "";

static int FilterElfHeader(struct dl_phdr_info *info, size_t size, void *data) {
  for (int i = 0; i < info->dlpi_phnum; i++) {
    if (info->dlpi_phdr[i].p_type == PT_LOAD &&
        info->dlpi_phdr[i].p_flags == (PF_R | PF_X)) {
      const void *const vaddr =
          turbo::bit_cast<void *>(info->dlpi_addr + info->dlpi_phdr[i].p_vaddr);
      const auto segsize = info->dlpi_phdr[i].p_memsz;

      const char *self_exe;
      if (info->dlpi_name != nullptr && info->dlpi_name[0] != '\0') {
        self_exe = info->dlpi_name;
      } else {
        self_exe = "/proc/self/exe";
      }

      turbo::debugging_internal::RegisterFileMappingHint(
          vaddr, reinterpret_cast<const char *>(vaddr) + segsize,
          info->dlpi_phdr[i].p_offset, self_exe);

      return 1;
    }
  }

  return 1;
}

TEST(Symbolize, SymbolizeWithMultipleMaps) {
  // Force kPadding0 and kPadding1 to be linked in.
  if (volatile_bool) {
    LOG(INFO) << kPadding0;
    LOG(INFO) << kPadding1;
  }

  // Verify we can symbolize everything.
  char buf[512];
  memset(buf, 0, sizeof(buf));
  turbo::Symbolize(kPadding0, buf, sizeof(buf));
  EXPECT_STREQ("kPadding0", buf);

  memset(buf, 0, sizeof(buf));
  turbo::Symbolize(kPadding1, buf, sizeof(buf));
  EXPECT_STREQ("kPadding1", buf);

  // Specify a hint for the executable segment.
  dl_iterate_phdr(FilterElfHeader, nullptr);

  // Reload at least one page out of kPadding0, kPadding1
  const char *ptrs[] = {kPadding0, kPadding1};

  for (const char *ptr : ptrs) {
    const int kMapFlags = MAP_ANONYMOUS | MAP_PRIVATE;
    void *addr = mmap(nullptr, kPageSize, PROT_READ, kMapFlags, 0, 0);
    ASSERT_NE(addr, MAP_FAILED);

    // kPadding[0-1] is full of zeroes, so we can remap anywhere within it, but
    // we ensure there is at least a full page of padding.
    void *remapped = reinterpret_cast<void *>(
        reinterpret_cast<uintptr_t>(ptr + kPageSize) & ~(kPageSize - 1ULL));

    const int kMremapFlags = (MREMAP_MAYMOVE | MREMAP_FIXED);
    void *ret = mremap(addr, kPageSize, kPageSize, kMremapFlags, remapped);
    ASSERT_NE(ret, MAP_FAILED);
  }

  // Invalidate the symbolization cache so we are forced to rely on the hint.
  turbo::Symbolize(nullptr, buf, sizeof(buf));

  // Verify we can still symbolize.
  const char *expected[] = {"kPadding0", "kPadding1"};
  const size_t offsets[] = {0, kPageSize, 2 * kPageSize, 3 * kPageSize};

  for (int i = 0; i < 2; i++) {
    for (size_t offset : offsets) {
      memset(buf, 0, sizeof(buf));
      turbo::Symbolize(ptrs[i] + offset, buf, sizeof(buf));
      EXPECT_STREQ(expected[i], buf);
    }
  }
}

// Appends string(*args->arg) to args->symbol_buf.
static void DummySymbolDecorator(
    const turbo::debugging_internal::SymbolDecoratorArgs *args) {
  std::string *message = static_cast<std::string *>(args->arg);
  strncat(args->symbol_buf, message->c_str(),
          args->symbol_buf_size - strlen(args->symbol_buf) - 1);
}

TEST(Symbolize, InstallAndRemoveSymbolDecorators) {
  int ticket_a;
  std::string a_message("a");
  EXPECT_GE(ticket_a = turbo::debugging_internal::InstallSymbolDecorator(
                DummySymbolDecorator, &a_message),
            0);

  int ticket_b;
  std::string b_message("b");
  EXPECT_GE(ticket_b = turbo::debugging_internal::InstallSymbolDecorator(
                DummySymbolDecorator, &b_message),
            0);

  int ticket_c;
  std::string c_message("c");
  EXPECT_GE(ticket_c = turbo::debugging_internal::InstallSymbolDecorator(
                DummySymbolDecorator, &c_message),
            0);

  // Use addresses 4 and 8 here to ensure that we always use valid addresses
  // even on systems that require instructions to be 32-bit aligned.
  char *address = reinterpret_cast<char *>(4);
  EXPECT_STREQ("abc", TrySymbolize(address));

  EXPECT_TRUE(turbo::debugging_internal::RemoveSymbolDecorator(ticket_b));

  EXPECT_STREQ("ac", TrySymbolize(address + 4));

  // Cleanup: remove all remaining decorators so other stack traces don't
  // get mystery "ac" decoration.
  EXPECT_TRUE(turbo::debugging_internal::RemoveSymbolDecorator(ticket_a));
  EXPECT_TRUE(turbo::debugging_internal::RemoveSymbolDecorator(ticket_c));
}

// Some versions of Clang with optimizations enabled seem to be able
// to optimize away the .data section if no variables live in the
// section. This variable should get placed in the .data section, and
// the test below checks for the existence of a .data section.
static int in_data_section = 1;

TEST(Symbolize, ForEachSection) {
  int fd = TEMP_FAILURE_RETRY(open("/proc/self/exe", O_RDONLY));
  ASSERT_NE(fd, -1);

  std::vector<std::string> sections;
  ASSERT_TRUE(turbo::debugging_internal::ForEachSection(
      fd, [&sections](const std::string_view name, const ElfW(Shdr) &) {
        sections.emplace_back(name);
        return true;
      }));

  // Check for the presence of common section names.
  EXPECT_THAT(sections, Contains(".text"));
  EXPECT_THAT(sections, Contains(".rodata"));
  EXPECT_THAT(sections, Contains(".bss"));
  ++in_data_section;
  EXPECT_THAT(sections, Contains(".data"));

  close(fd);
}
#endif  // !TURBO_INTERNAL_HAVE_DARWIN_SYMBOLIZE &&
        // !TURBO_INTERNAL_HAVE_EMSCRIPTEN_SYMBOLIZE

// x86 specific tests.  Uses some inline assembler.
extern "C" {
inline void *TURBO_ATTRIBUTE_ALWAYS_INLINE inline_func() {
  void *pc = nullptr;
#if defined(__i386__)
  __asm__ __volatile__("call 1f;\n 1: pop %[PC]" : [PC] "=r"(pc));
#elif defined(__x86_64__)
  __asm__ __volatile__("leaq 0(%%rip),%[PC];\n" : [PC] "=r"(pc));
#endif
  return pc;
}

void *TURBO_ATTRIBUTE_NOINLINE non_inline_func() {
  void *pc = nullptr;
#if defined(__i386__)
  __asm__ __volatile__("call 1f;\n 1: pop %[PC]" : [PC] "=r"(pc));
#elif defined(__x86_64__)
  __asm__ __volatile__("leaq 0(%%rip),%[PC];\n" : [PC] "=r"(pc));
#endif
  return pc;
}

void TURBO_ATTRIBUTE_NOINLINE TestWithPCInsideNonInlineFunction() {
#if defined(TURBO_HAVE_ATTRIBUTE_NOINLINE) && \
    (defined(__i386__) || defined(__x86_64__))
  void *pc = non_inline_func();
  const char *symbol = TrySymbolize(pc);
  CHECK_NE(symbol, nullptr) << "TestWithPCInsideNonInlineFunction failed";
  CHECK_STREQ(symbol, "non_inline_func")
      << "TestWithPCInsideNonInlineFunction failed";
  std::cout << "TestWithPCInsideNonInlineFunction passed" << std::endl;
#endif
}

void TURBO_ATTRIBUTE_NOINLINE TestWithPCInsideInlineFunction() {
#if defined(TURBO_HAVE_ATTRIBUTE_ALWAYS_INLINE) && \
    (defined(__i386__) || defined(__x86_64__))
  void *pc = inline_func();  // Must be inlined.
  const char *symbol = TrySymbolize(pc);
  CHECK_NE(symbol, nullptr) << "TestWithPCInsideInlineFunction failed";
  CHECK_STREQ(symbol, __FUNCTION__) << "TestWithPCInsideInlineFunction failed";
  std::cout << "TestWithPCInsideInlineFunction passed" << std::endl;
#endif
}
}

#if defined(__arm__) && TURBO_HAVE_ATTRIBUTE(target) && \
    ((__ARM_ARCH >= 7) || !defined(__ARM_PCS_VFP))
// Test that we correctly identify bounds of Thumb functions on ARM.
//
// Thumb functions have the lowest-order bit set in their addresses in the ELF
// symbol table. This requires some extra logic to properly compute function
// bounds. To test this logic, nudge a Thumb function right up against an ARM
// function and try to symbolize the ARM function.
//
// A naive implementation will simply use the Thumb function's entry point as
// written in the symbol table and will therefore treat the Thumb function as
// extending one byte further in the instruction stream than it actually does.
// When asked to symbolize the start of the ARM function, it will identify an
// overlap between the Thumb and ARM functions, and it will return the name of
// the Thumb function.
//
// A correct implementation, on the other hand, will null out the lowest-order
// bit in the Thumb function's entry point. It will correctly compute the end of
// the Thumb function, it will find no overlap between the Thumb and ARM
// functions, and it will return the name of the ARM function.
//
// Unfortunately we cannot perform this test on armv6 or lower systems that use
// the hard float ABI because gcc refuses to compile thumb functions on such
// systems with a "sorry, unimplemented: Thumb-1 hard-float VFP ABI" error.

__attribute__((target("thumb"))) int ArmThumbOverlapThumb(int x) {
  return x * x * x;
}

__attribute__((target("arm"))) int ArmThumbOverlapArm(int x) {
  return x * x * x;
}

void TURBO_ATTRIBUTE_NOINLINE TestArmThumbOverlap() {
#if defined(TURBO_HAVE_ATTRIBUTE_NOINLINE)
  const char *symbol = TrySymbolize((void *)&ArmThumbOverlapArm);
  CHECK_NE(symbol, nullptr) << "TestArmThumbOverlap failed";
  CHECK_STREQ("ArmThumbOverlapArm()", symbol) << "TestArmThumbOverlap failed";
  std::cout << "TestArmThumbOverlap passed" << std::endl;
#endif
}

#endif  // defined(__arm__) && TURBO_HAVE_ATTRIBUTE(target) && ((__ARM_ARCH >= 7)
        // || !defined(__ARM_PCS_VFP))

#elif defined(_WIN32)
#if !defined(TURBO_CONSUME_DLL)

TEST(Symbolize, Basics) {
  EXPECT_STREQ("nonstatic_func", TrySymbolize((void *)(&nonstatic_func)));

  // The name of an internal linkage symbol is not specified; allow either a
  // mangled or an unmangled name here.
  const char *static_func_symbol = TrySymbolize((void *)(&static_func));
  ASSERT_TRUE(static_func_symbol != nullptr);
  EXPECT_TRUE(strstr(static_func_symbol, "static_func") != nullptr);

  EXPECT_TRUE(nullptr == TrySymbolize(nullptr));
}

TEST(Symbolize, Truncation) {
  constexpr char kNonStaticFunc[] = "nonstatic_func";
  EXPECT_STREQ("nonstatic_func",
               TrySymbolizeWithLimit((void *)(&nonstatic_func),
                                     strlen(kNonStaticFunc) + 1));
  EXPECT_STREQ("nonstatic_...",
               TrySymbolizeWithLimit((void *)(&nonstatic_func),
                                     strlen(kNonStaticFunc) + 0));
  EXPECT_STREQ("nonstatic...",
               TrySymbolizeWithLimit((void *)(&nonstatic_func),
                                     strlen(kNonStaticFunc) - 1));
  EXPECT_STREQ("n...", TrySymbolizeWithLimit((void *)(&nonstatic_func), 5));
  EXPECT_STREQ("...", TrySymbolizeWithLimit((void *)(&nonstatic_func), 4));
  EXPECT_STREQ("..", TrySymbolizeWithLimit((void *)(&nonstatic_func), 3));
  EXPECT_STREQ(".", TrySymbolizeWithLimit((void *)(&nonstatic_func), 2));
  EXPECT_STREQ("", TrySymbolizeWithLimit((void *)(&nonstatic_func), 1));
  EXPECT_EQ(nullptr, TrySymbolizeWithLimit((void *)(&nonstatic_func), 0));
}

TEST(Symbolize, SymbolizeWithDemangling) {
  const char *result = TrySymbolize((void *)(&Foo::func));
  ASSERT_TRUE(result != nullptr);
  EXPECT_TRUE(strstr(result, "Foo::func") != nullptr) << result;
}

#endif  // !defined(TURBO_CONSUME_DLL)
#else   // Symbolizer unimplemented
TEST(Symbolize, Unimplemented) {
  char buf[64];
  EXPECT_FALSE(turbo::Symbolize((void *)(&nonstatic_func), buf, sizeof(buf)));
  EXPECT_FALSE(turbo::Symbolize((void *)(&static_func), buf, sizeof(buf)));
  EXPECT_FALSE(turbo::Symbolize((void *)(&Foo::func), buf, sizeof(buf)));
}

#endif

int main(int argc, char **argv) {
#if !defined(__EMSCRIPTEN__)
  // Make sure kHpageTextPadding is linked into the binary.
  if (volatile_bool) {
    LOG(INFO) << kHpageTextPadding;
  }
#endif  // !defined(__EMSCRIPTEN__)

#if TURBO_PER_THREAD_TLS
  // Touch the per-thread variables.
  symbolize_test_thread_small[0] = 0;
  symbolize_test_thread_big[0] = 0;
#endif

  turbo::InitializeSymbolizer(argv[0]);
  testing::InitGoogleTest(&argc, argv);

#if defined(TURBO_INTERNAL_HAVE_ELF_SYMBOLIZE) ||        \
    defined(TURBO_INTERNAL_HAVE_EMSCRIPTEN_SYMBOLIZE) || \
    defined(TURBO_INTERNAL_HAVE_DARWIN_SYMBOLIZE)
  TestWithPCInsideInlineFunction();
  TestWithPCInsideNonInlineFunction();
  TestWithReturnAddress();
#if defined(__arm__) && TURBO_HAVE_ATTRIBUTE(target) && \
    ((__ARM_ARCH >= 7) || !defined(__ARM_PCS_VFP))
  TestArmThumbOverlap();
#endif
#endif

  return RUN_ALL_TESTS();
}
