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
// Created by jeff on 24-6-2.
//

#include <type_traits>
#include <typeinfo>

#include <gtest/gtest.h>

#include <turbo/base/checked_cast.h>

namespace turbo {

    class Foo {
    public:
        virtual ~Foo() = default;
    };

    class Bar {
    };

    class FooSub : public Foo {
    };

    template<typename T>
    class Baz : public Foo {
    };

    TEST(CheckedCast, TestInvalidSubclassCast) {
        static_assert(std::is_polymorphic<Foo>::value, "Foo is not polymorphic");

        Foo foo;
        FooSub foosub;
        const Foo &foosubref = foosub;
        Baz<double> baz;
        const Foo &bazref = baz;

#ifndef NDEBUG  // debug mode
        // illegal pointer cast
        ASSERT_EQ(nullptr, checked_cast<Bar *>(&foo));

        // illegal reference cast
        ASSERT_THROW(checked_cast<const Bar &>(foosubref), std::bad_cast);

        // legal reference casts
        ASSERT_NO_THROW(checked_cast<const FooSub &>(foosubref));
        ASSERT_NO_THROW(checked_cast<const Baz<double> &>(bazref));
#else  // release mode
        // failure modes for the invalid casts occur at compile time

          // legal pointer cast
          ASSERT_NE(nullptr, checked_cast<const FooSub*>(&foosubref));

          // legal reference casts: this is static_cast in a release build, so ASSERT_NO_THROW
          // doesn't make a whole lot of sense here.
          auto& x = checked_cast<const FooSub&>(foosubref);
          ASSERT_EQ(&foosubref, &x);

          auto& y = checked_cast<const Baz<double>&>(bazref);
          ASSERT_EQ(&bazref, &y);
#endif
    }

}  // namespace turbo
