.. Copyright 2023 The Elastic AI Search Authors.


typed test
=================================================

see the add test again :ref:`add test <add_test>`. it is a template function.
we need to test it with different types, such as int, float, double, etc. Write
a just for a type, and then copy it for other types. It is a waste of time and
low efficiency. We can use the template to solve this problem. with the typed
test, we can write a test for a type, and then use the typed test to test it.

see the following example:

..  code-block:: c++

    #define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
    #include "doctest/doctest.h"
    template <typename T>
    void add(T a, T b) {
        return a + b;
    }

    using IntTypes = std::tuple<int8_t, uint8_t, int16_t, uint16_t, int32_t,
            uint32_t, int64_t, uint64_t>;


    TEST_CASE_TEMPLATE_DEFINE("add test", T, test_id) {
        CHECK(add(T(1), T(2)) == T(3));
    }

    TEST_CASE_TEMPLATE_APPLY(test_id, IntTypes);

the ``TEST_CASE_TEMPLATE_DEFINE`` is used to define a template test case. the
first parameter is the name of the test case, the second parameter is the type
that calls the test case. the third parameter is the id of the test case. the
``TEST_CASE_TEMPLATE_APPLY`` is used to apply the template test case.

after we define the template test case. next, we can call it with different types
by ``TEST_CASE_TEMPLATE_APPLY``. the first parameter is the test_id of the template
we have defined. the second parameter is the types we want to test. the types can
be a type, a tuple of types, or a type list. in the doctest implementation, it finally
calls it by std::tuple.

