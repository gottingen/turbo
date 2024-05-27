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

// Generates gaussian_distribution.cc
//
// $ blaze run :gaussian_distribution_gentables > gaussian_distribution.cc
//
#include <turbo/random/gaussian_distribution.h>

#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <string>

#include <turbo/base/macros.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {
namespace {

template <typename T, size_t N>
void FormatArrayContents(std::ostream* os, T (&data)[N]) {
  if (!std::numeric_limits<T>::is_exact) {
    // Note: T is either an integer or a float.
    // float requires higher precision to ensure that values are
    // reproduced exactly.
    // Trivia: C99 has hexadecimal floating point literals, but C++11 does not.
    // Using them would remove all concern of precision loss.
    os->precision(std::numeric_limits<T>::max_digits10 + 2);
  }
  *os << "    {";
  std::string separator = "";
  for (size_t i = 0; i < N; ++i) {
    *os << separator << data[i];
    if ((i + 1) % 3 != 0) {
      separator = ", ";
    } else {
      separator = ",\n     ";
    }
  }
  *os << "}";
}

}  // namespace

class TableGenerator : public gaussian_distribution_base {
 public:
  TableGenerator();
  void Print(std::ostream* os);

  using gaussian_distribution_base::kMask;
  using gaussian_distribution_base::kR;
  using gaussian_distribution_base::kV;

 private:
  Tables tables_;
};

// Ziggurat gaussian initialization.  For an explanation of the algorithm, see
// the Marsaglia paper, "The Ziggurat Method for Generating Random Variables".
//   http://www.jstatsoft.org/v05/i08/
//
// Further details are available in the Doornik paper
//   https://www.doornik.com/research/ziggurat.pdf
//
TableGenerator::TableGenerator() {
  // The constants here should match the values in gaussian_distribution.h
  static constexpr int kC = kMask + 1;

  static_assert((TURBO_ARRAYSIZE(tables_.x) == kC + 1),
                "xArray must be length kMask + 2");

  static_assert((TURBO_ARRAYSIZE(tables_.x) == TURBO_ARRAYSIZE(tables_.f)),
                "fx and x arrays must be identical length");

  auto f = [](double x) { return std::exp(-0.5 * x * x); };
  auto f_inv = [](double x) { return std::sqrt(-2.0 * std::log(x)); };

  tables_.x[0] = kV / f(kR);
  tables_.f[0] = f(tables_.x[0]);

  tables_.x[1] = kR;
  tables_.f[1] = f(tables_.x[1]);

  tables_.x[kC] = 0.0;
  tables_.f[kC] = f(tables_.x[kC]);  // 1.0

  for (int i = 2; i < kC; i++) {
    double v = (kV / tables_.x[i - 1]) + tables_.f[i - 1];
    tables_.x[i] = f_inv(v);
    tables_.f[i] = v;
  }
}

void TableGenerator::Print(std::ostream* os) {
  *os << "// BEGIN GENERATED CODE; DO NOT EDIT\n"
         "// clang-format off\n"
         "\n"
         "#include \"turbo/random/gaussian_distribution.h\"\n"
         "\n"
         "namespace turbo {\n"
         "TURBO_NAMESPACE_BEGIN\n"
         "namespace random_internal {\n"
         "\n"
         "const gaussian_distribution_base::Tables\n"
         "    gaussian_distribution_base::zg_ = {\n";
  FormatArrayContents(os, tables_.x);
  *os << ",\n";
  FormatArrayContents(os, tables_.f);
  *os << "};\n"
         "\n"
         "}  // namespace random_internal\n"
         "TURBO_NAMESPACE_END\n"
         "}  // namespace turbo\n"
         "\n"
         "// clang-format on\n"
         "// END GENERATED CODE";
  *os << std::endl;
}

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

int main(int, char**) {
  std::cerr << "\nCopy the output to gaussian_distribution.cc" << std::endl;
  turbo::random_internal::TableGenerator generator;
  generator.Print(&std::cout);
  return 0;
}
