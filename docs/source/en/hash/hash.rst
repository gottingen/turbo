.. Copyright 2023 The Turbo Authors.

.. _turbo_hash_usage:

hash usage
=============================

The ``turbo::Hash<T, Tag>`` module provides a set of functions and classes for hashing. The library consists
of the following modules:

- **turbo::Hash<T, Tag>**, A concrete hash functor object. which you can use out of the box.
- **turbo::Mixer<R,Tag>**, A concrete mixer functor object for small keys.
- **turbo::hash_engine**, A hash engine for hashing a sequence of bytes. like xxhash, murmurhash, etc.

This library is designed to be used as a replacement for the standard library's hash functions, like
``std::hash``. The standard library's hash functions are not guaranteed to be stable across different.
It provides several advantages over them:

- It can hash objects of almost any standard type, including std::pair, std::tuple,
  and most standard containers.

- It can be extended to support user-defined types. Our goal is that if it makes sense
  to hash an object of type Foo, then Turbo::Hash<Foo> will just work. These extensions
  are easy to write and efficient to execute.

- The underlying hash algorithm can be changed without modifying user code, which allows
  us to improve it over time. For example, to improve performance and to defend against some
  hash-flooding attacks.

- Turbo can easily switch hash algorithms, such as city hash murmur3, xxhash, etc.

Using Turbo::Hash
=============================

The ``turbo::Hash`` module is designed to be used as a replacement for the standard library's
hash functions, like ``std::hash``.

It can be used as a hash functor object, like this:

.. code-block:: cpp

    #include <turbo/hash.hpp>
    #include <iostream>
    #include <string>

    int main()
    {
        std::unordered_map<std::string, int, turbo::Hash<std::string>> m;
        m["foo"] = 1;
        m["bar"] = 2;
        m["baz"] = 3;
        for (auto& p : m) {
            std::cout << p.first << " => " << p.second << '\n';
        }
    }

The ``turbo::Hash`` module can also be used as a base class for user-defined hash functions.
For example, if you have a class ``Foo`` that you want to hash, you can write:

.. code-block:: cpp

    #include <turbo/hash.hpp>
    #include <iostream>
    #include <string>

    struct Foo {
        int x;
        int y;
    };

    struct FooHash : turbo::Hash<Foo> {
        size_t operator()(const Foo& foo) const {
            return turbo::Hash<Foo>::operator()(foo.x, foo.y);
        }
    };

    int main()
    {
        std::unordered_map<Foo, int, FooHash> m;
        m[Foo{1, 2}] = 1;
        m[Foo{3, 4}] = 2;
        m[Foo{5, 6}] = 3;
        for (auto& p : m) {
            std::cout << p.first.x << ',' << p.first.y << " => " << p.second << '\n';
        }
    }

manage hash algorithm
-----------------------------

The ``turbo::Hash`` module can easily switch hash algorithms, such as city hash murmur3, xxhash, etc.

.. code-block:: cpp

    #include <turbo/hash.hpp>
    #include <iostream>
    #include <string>

    int main()
    {
        std::unordered_map<std::string, int, turbo::Hash<std::string, turbo::xx_hash_tag>> m;
        m["foo"] = 1;
        m["bar"] = 2;
        m["baz"] = 3;
        for (auto& p : m) {
            std::cout << p.first << " => " << p.second << '\n';
        }
    }

you can list all supported hash algorithms by ``turbo::turbo::supported_hash_engines`` variable.
and find the in ``turbo/hash/xx/xx.h`` and other files like ``**_hash_tag``.

manage default hash algorithm

The ``turbo::Hash`` module can easily switch default hash algorithms, such as city hash murmur3, xxhash, etc.

The Macro ``TURBO_HASH_DEFAULT_ENGINE`` can be used to set the default hash algorithm. The default hash
algorithm is ``turbo::bytes_hash_engine``. You can set it to ``turbo::xx_hash_engine`` or other hash
algorithm.

.. code-block:: cpp

    #define TURBO_HASH_DEFAULT_ENGINE turbo::xx_hash_engine
    #include <turbo/hash.hpp>
    #include <iostream>
    #include <string>

    int main()
    {
        std::unordered_map<std::string, int, turbo::Hash<std::string>> m;
        m["foo"] = 1;
        m["bar"] = 2;
        m["baz"] = 3;
        for (auto& p : m) {
            std::cout << p.first << " => " << p.second << '\n';
        }
    }

make your defined type hashable
-------------------------------------------------

If you want to make your defined type hashable by turbo::Hash, you need to define a hash function
``hash_value`` in your type. The overload should combine state with the existing hash state
(denoted as H in the template below), and your class must provide an equality operator.

.. code-block:: cpp

    #include <turbo/hash.hpp>
    #include <iostream>
    #include <string>

    struct Foo {
        int x;
        std::string y;

    };

    bool operator==(const Foo& lhs, const Foo& rhs) {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    template <typename H>
    void hash_value(H& h, const Foo& foo) {
        return H::combine(std::move(h), m.v, m.str, m.b);
    }

..  note::

    The ``hash_value`` function should be defined in the same namespace as your type.
    If you can't do that, you can define a specialization of ``turbo::hash`` instead.


compatible with std::hash
---------------------------------

The ``turbo::Hash`` module is compatible with ``std::hash``. You can use ``std::hash`` to hash
your type, and use ``turbo::Hash`` to hash your type. The following example shows how to use
``std::hash`` to hash your type.

.. code-block:: cpp

    #include <turbo/hash.hpp>
    #include <iostream>
    #include <string>

    struct Foo {
        int x;
        std::string y;

    };

    bool operator==(const Foo& lhs, const Foo& rhs) {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    namespace std {
        template <>
        struct hash<Foo> {
            size_t operator()(const Foo& foo) const {
                return x + std::hash<std::string>()(y);
            }
        };
    }

    int main()
    {
        std::unordered_map<Foo, int, turbo::Hash<Foo>> m;
        m[Foo{1, "foo"}] = 1;
        m[Foo{2, "bar"}] = 2;
        m[Foo{3, "baz"}] = 3;
        for (auto& p : m) {
            std::cout << p.first.x << ',' << p.first.y << " => " << p.second << '\n';
        }
    }

This will also work. But we recommend that you use ``turbo::Hash`` to hash your type.

hash select
---------------------------------

Sometimes you have defined ``hash_value`` for your type, but also defined ``std::hash`` for your type.
at this case, turbo::Hash will use select ``hash_value`` to hash your type if you use turbo::Hash.

.. code-block:: cpp

    #include <turbo/hash.hpp>
    #include <iostream>
    #include <string>

    struct Bar {
        int x;
    };

    bool operator==(const Bar& lhs, const Bar& rhs) {
        return lhs.x == rhs.x;
    }

    template <typename H>
    void hash_value(H& h, const Bar& bar) {
        return H::combine(std::move(h), bar.x);
    }

    namespace std {
        template <>
        struct hash<Bar> {
            size_t operator()(const Bar& bar) const {
                return bar.x;
            }
        };
    }

    int main()
    {
        std::cout << turbo::Hash<Bar>()(Bar{1}) << std::endl;
        std::cout << std::hash<Bar>()(Bar{1}) << std::endl;
    }

The output are different. turbo::Hash will use ``hash_value`` to hash your type. std::hash will use
``std::hash`` to hash your type.

..  note::

    If you want to use ``std::hash`` to hash your type, you need to define ``std::hash`` in ``std``
    namespace. and use ``std::hash`` to hash your type. If you want to use ``turbo::Hash`` to hash
    and use ``std`` defined algorithm to hash your type, you should not define ``hash_value`` for
    your type.


tools for hash
---------------------------------

we provide some tools for hash. you can use it to hash your type. After you have installed turbo,
there is a command line tool named ``turbo`` in your install directory. you can use it to hash
your type. and debug some things.

.. code-block:: console

   > turbo hash --help
    Usage: turbo hash [OPTIONS] [ARGS]...
    ...
   > turbo hash -s "hello world"
    +----------+---------------------+
    | engine   | bytes_hash          |
    +----------+---------------------+
    | original | hello world         |
    +----------+---------------------+
    | hash     | 7051363063550010326 |
    +----------+---------------------+
    > turbo hash -s "hello world" -e xx
    +----------+---------------------+
    | engine   | xx_hash             |
    +----------+---------------------+
    | original | hello world         |
    +----------+---------------------+
    | hash     | 8962184958574551130 |
    +----------+---------------------+

advanced usage
===============================








