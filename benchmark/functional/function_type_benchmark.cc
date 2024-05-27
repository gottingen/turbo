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

#include <functional>
#include <memory>
#include <string>

#include <benchmark/benchmark.h>
#include <turbo/base/attributes.h>
#include <turbo/functional/any_invocable.h>
#include <turbo/functional/function_ref.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace {

int dummy = 0;

void FreeFunction() { benchmark::DoNotOptimize(dummy); }

struct TrivialFunctor {
  void operator()() const { benchmark::DoNotOptimize(dummy); }
};

struct LargeFunctor {
  void operator()() const { benchmark::DoNotOptimize(this); }
  std::string a, b, c;
};

template <typename Function, typename... Args>
void TURBO_ATTRIBUTE_NOINLINE CallFunction(Function f, Args&&... args) {
  f(std::forward<Args>(args)...);
}

template <typename Function, typename Callable, typename... Args>
void ConstructAndCallFunctionBenchmark(benchmark::State& state,
                                       const Callable& c, Args&&... args) {
  for (auto _ : state) {
    CallFunction<Function>(c, std::forward<Args>(args)...);
  }
}

void BM_TrivialStdFunction(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<std::function<void()>>(state,
                                                           TrivialFunctor{});
}
BENCHMARK(BM_TrivialStdFunction);

void BM_TrivialFunctionRef(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<FunctionRef<void()>>(state,
                                                         TrivialFunctor{});
}
BENCHMARK(BM_TrivialFunctionRef);

void BM_TrivialAnyInvocable(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<AnyInvocable<void()>>(state,
                                                          TrivialFunctor{});
}
BENCHMARK(BM_TrivialAnyInvocable);

void BM_LargeStdFunction(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<std::function<void()>>(state,
                                                           LargeFunctor{});
}
BENCHMARK(BM_LargeStdFunction);

void BM_LargeFunctionRef(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<FunctionRef<void()>>(state, LargeFunctor{});
}
BENCHMARK(BM_LargeFunctionRef);


void BM_LargeAnyInvocable(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<AnyInvocable<void()>>(state,
                                                          LargeFunctor{});
}
BENCHMARK(BM_LargeAnyInvocable);

void BM_FunPtrStdFunction(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<std::function<void()>>(state, FreeFunction);
}
BENCHMARK(BM_FunPtrStdFunction);

void BM_FunPtrFunctionRef(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<FunctionRef<void()>>(state, FreeFunction);
}
BENCHMARK(BM_FunPtrFunctionRef);

void BM_FunPtrAnyInvocable(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<AnyInvocable<void()>>(state, FreeFunction);
}
BENCHMARK(BM_FunPtrAnyInvocable);

// Doesn't include construction or copy overhead in the loop.
template <typename Function, typename Callable, typename... Args>
void CallFunctionBenchmark(benchmark::State& state, const Callable& c,
                           Args... args) {
  Function f = c;
  for (auto _ : state) {
    benchmark::DoNotOptimize(&f);
    f(args...);
  }
}

struct FunctorWithTrivialArgs {
  void operator()(int a, int b, int c) const {
    benchmark::DoNotOptimize(a);
    benchmark::DoNotOptimize(b);
    benchmark::DoNotOptimize(c);
  }
};

void BM_TrivialArgsStdFunction(benchmark::State& state) {
  CallFunctionBenchmark<std::function<void(int, int, int)>>(
      state, FunctorWithTrivialArgs{}, 1, 2, 3);
}
BENCHMARK(BM_TrivialArgsStdFunction);

void BM_TrivialArgsFunctionRef(benchmark::State& state) {
  CallFunctionBenchmark<FunctionRef<void(int, int, int)>>(
      state, FunctorWithTrivialArgs{}, 1, 2, 3);
}
BENCHMARK(BM_TrivialArgsFunctionRef);

void BM_TrivialArgsAnyInvocable(benchmark::State& state) {
  CallFunctionBenchmark<AnyInvocable<void(int, int, int)>>(
      state, FunctorWithTrivialArgs{}, 1, 2, 3);
}
BENCHMARK(BM_TrivialArgsAnyInvocable);

struct FunctorWithNonTrivialArgs {
  void operator()(std::string a, std::string b, std::string c) const {
    benchmark::DoNotOptimize(&a);
    benchmark::DoNotOptimize(&b);
    benchmark::DoNotOptimize(&c);
  }
};

void BM_NonTrivialArgsStdFunction(benchmark::State& state) {
  std::string a, b, c;
  CallFunctionBenchmark<
      std::function<void(std::string, std::string, std::string)>>(
      state, FunctorWithNonTrivialArgs{}, a, b, c);
}
BENCHMARK(BM_NonTrivialArgsStdFunction);

void BM_NonTrivialArgsFunctionRef(benchmark::State& state) {
  std::string a, b, c;
  CallFunctionBenchmark<
      FunctionRef<void(std::string, std::string, std::string)>>(
      state, FunctorWithNonTrivialArgs{}, a, b, c);
}
BENCHMARK(BM_NonTrivialArgsFunctionRef);

void BM_NonTrivialArgsAnyInvocable(benchmark::State& state) {
  std::string a, b, c;
  CallFunctionBenchmark<
      AnyInvocable<void(std::string, std::string, std::string)>>(
      state, FunctorWithNonTrivialArgs{}, a, b, c);
}
BENCHMARK(BM_NonTrivialArgsAnyInvocable);

}  // namespace
TURBO_NAMESPACE_END
}  // namespace turbo
