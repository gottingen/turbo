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

#include <turbo/strings/charconv.h>

#include <cstdlib>
#include <cstring>
#include <string>

#include <benchmark/benchmark.h>

namespace {

void BM_Strtod_Pi(benchmark::State& state) {
  const char* pi = "3.14159";
  for (auto s : state) {
    benchmark::DoNotOptimize(pi);
    benchmark::DoNotOptimize(strtod(pi, nullptr));
  }
}
BENCHMARK(BM_Strtod_Pi);

void BM_Turbo_Pi(benchmark::State& state) {
  const char* pi = "3.14159";
  const char* pi_end = pi + strlen(pi);
  for (auto s : state) {
    benchmark::DoNotOptimize(pi);
    double v;
    turbo::from_chars(pi, pi_end, v);
    benchmark::DoNotOptimize(v);
  }
}
BENCHMARK(BM_Turbo_Pi);

void BM_Strtod_Pi_float(benchmark::State& state) {
  const char* pi = "3.14159";
  for (auto s : state) {
    benchmark::DoNotOptimize(pi);
    benchmark::DoNotOptimize(strtof(pi, nullptr));
  }
}
BENCHMARK(BM_Strtod_Pi_float);

void BM_Turbo_Pi_float(benchmark::State& state) {
  const char* pi = "3.14159";
  const char* pi_end = pi + strlen(pi);
  for (auto s : state) {
    benchmark::DoNotOptimize(pi);
    float v;
    turbo::from_chars(pi, pi_end, v);
    benchmark::DoNotOptimize(v);
  }
}
BENCHMARK(BM_Turbo_Pi_float);

void BM_Strtod_HardLarge(benchmark::State& state) {
  const char* num = "272104041512242479.e200";
  for (auto s : state) {
    benchmark::DoNotOptimize(num);
    benchmark::DoNotOptimize(strtod(num, nullptr));
  }
}
BENCHMARK(BM_Strtod_HardLarge);

void BM_Turbo_HardLarge(benchmark::State& state) {
  const char* numstr = "272104041512242479.e200";
  const char* numstr_end = numstr + strlen(numstr);
  for (auto s : state) {
    benchmark::DoNotOptimize(numstr);
    double v;
    turbo::from_chars(numstr, numstr_end, v);
    benchmark::DoNotOptimize(v);
  }
}
BENCHMARK(BM_Turbo_HardLarge);

void BM_Strtod_HardSmall(benchmark::State& state) {
  const char* num = "94080055902682397.e-242";
  for (auto s : state) {
    benchmark::DoNotOptimize(num);
    benchmark::DoNotOptimize(strtod(num, nullptr));
  }
}
BENCHMARK(BM_Strtod_HardSmall);

void BM_Turbo_HardSmall(benchmark::State& state) {
  const char* numstr = "94080055902682397.e-242";
  const char* numstr_end = numstr + strlen(numstr);
  for (auto s : state) {
    benchmark::DoNotOptimize(numstr);
    double v;
    turbo::from_chars(numstr, numstr_end, v);
    benchmark::DoNotOptimize(v);
  }
}
BENCHMARK(BM_Turbo_HardSmall);

void BM_Strtod_HugeMantissa(benchmark::State& state) {
  std::string huge(200, '3');
  const char* num = huge.c_str();
  for (auto s : state) {
    benchmark::DoNotOptimize(num);
    benchmark::DoNotOptimize(strtod(num, nullptr));
  }
}
BENCHMARK(BM_Strtod_HugeMantissa);

void BM_Turbo_HugeMantissa(benchmark::State& state) {
  std::string huge(200, '3');
  const char* num = huge.c_str();
  const char* num_end = num + 200;
  for (auto s : state) {
    benchmark::DoNotOptimize(num);
    double v;
    turbo::from_chars(num, num_end, v);
    benchmark::DoNotOptimize(v);
  }
}
BENCHMARK(BM_Turbo_HugeMantissa);

std::string MakeHardCase(int length) {
  // The number 1.1521...e-297 is exactly halfway between 12345 * 2**-1000 and
  // the next larger representable number.  The digits of this number are in
  // the string below.
  const std::string digits =
      "1."
      "152113937042223790993097181572444900347587985074226836242307364987727724"
      "831384300183638649152607195040591791364113930628852279348613864894524591"
      "272746490313676832900762939595690019745859128071117417798540258114233761"
      "012939937017879509401007964861774960297319002612457273148497158989073482"
      "171377406078223015359818300988676687994537274548940612510414856761641652"
      "513434981938564294004070500716200446656421722229202383105446378511678258"
      "370570631774499359748259931676320916632111681001853983492795053244971606"
      "922718923011680846577744433974087653954904214152517799883551075537146316"
      "168973685866425605046988661997658648354773076621610279716804960009043764"
      "038392994055171112475093876476783502487512538082706095923790634572014823"
      "78877699375152587890625" +
      std::string(5000, '0');
  // generate the hard cases on either side for the given length.
  // Lengths between 3 and 1000 are reasonable.
  return digits.substr(0, length) + "1e-297";
}

void BM_Strtod_Big_And_Difficult(benchmark::State& state) {
  std::string testcase = MakeHardCase(state.range(0));
  const char* begin = testcase.c_str();
  for (auto s : state) {
    benchmark::DoNotOptimize(begin);
    benchmark::DoNotOptimize(strtod(begin, nullptr));
  }
}
BENCHMARK(BM_Strtod_Big_And_Difficult)->Range(3, 5000);

void BM_Turbo_Big_And_Difficult(benchmark::State& state) {
  std::string testcase = MakeHardCase(state.range(0));
  const char* begin = testcase.c_str();
  const char* end = begin + testcase.size();
  for (auto s : state) {
    benchmark::DoNotOptimize(begin);
    double v;
    turbo::from_chars(begin, end, v);
    benchmark::DoNotOptimize(v);
  }
}
BENCHMARK(BM_Turbo_Big_And_Difficult)->Range(3, 5000);

}  // namespace

// ------------------------------------------------------------------------
// Benchmark                                 Time           CPU Iterations
// ------------------------------------------------------------------------
// BM_Strtod_Pi                             96 ns         96 ns    6337454
// BM_Turbo_Pi                               35 ns         35 ns   20031996
// BM_Strtod_Pi_float                       91 ns         91 ns    7745851
// BM_Turbo_Pi_float                         35 ns         35 ns   20430298
// BM_Strtod_HardLarge                     133 ns        133 ns    5288341
// BM_Turbo_HardLarge                       181 ns        181 ns    3855615
// BM_Strtod_HardSmall                     279 ns        279 ns    2517243
// BM_Turbo_HardSmall                       287 ns        287 ns    2458744
// BM_Strtod_HugeMantissa                  433 ns        433 ns    1604293
// BM_Turbo_HugeMantissa                    160 ns        160 ns    4403671
// BM_Strtod_Big_And_Difficult/3           236 ns        236 ns    2942496
// BM_Strtod_Big_And_Difficult/8           232 ns        232 ns    2983796
// BM_Strtod_Big_And_Difficult/64          437 ns        437 ns    1591951
// BM_Strtod_Big_And_Difficult/512        1738 ns       1738 ns     402519
// BM_Strtod_Big_And_Difficult/4096       3943 ns       3943 ns     176128
// BM_Strtod_Big_And_Difficult/5000       4397 ns       4397 ns     157878
// BM_Turbo_Big_And_Difficult/3              39 ns         39 ns   17799583
// BM_Turbo_Big_And_Difficult/8              43 ns         43 ns   16096859
// BM_Turbo_Big_And_Difficult/64            550 ns        550 ns    1259717
// BM_Turbo_Big_And_Difficult/512          4167 ns       4167 ns     171414
// BM_Turbo_Big_And_Difficult/4096         9160 ns       9159 ns      76297
// BM_Turbo_Big_And_Difficult/5000         9738 ns       9738 ns      70140
