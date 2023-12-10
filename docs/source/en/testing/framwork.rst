.. Copyright 2023 The Elastic AI Search Authors.


framework chose
=================================================

Turbo and the entire EA project have many functions and have
thousands of interfaces. In order to ensure the correctness of
the interfaces, unit testing is essential. After many detailed
comparisons, we finally chose doctest as our unit testing framework.
`doctest <https://github.com/doctest/doctest>`_ is a lightweight
and easy-to-use C++ unit testing framework. It is very suitable for
testing the interfaces of the EA project. Another reason is that
doctest is a header-only library, which is very convenient to use
and does not require any configuration.

install doctest is very simple, just copy the doctest.h file to
the include directory of the project, and then include it in the
test file.

Turbo used gtest as the unit testing framework in the previous
stage, and then gradually migrated to doctest.
