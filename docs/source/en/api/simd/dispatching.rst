.. Copyright 2023 The Turbo Authors.



.. raw:: html

   <style>
   .rst-content table.docutils {
       width: 100%;
       table-layout: fixed;
   }

   table.docutils .line-block {
       margin-left: 0;
       margin-bottom: 0;
   }

   table.docutils code.literal {
       color: initial;
   }

   code.docutils {
       background: initial;
   }
   </style>


.. _Arch Dispatching:

Arch Dispatching
================

`turbo::simd` provides a generic way to dispatch a function call based on the architecture the code was compiled for and the architectures available at runtime.
The :cpp:func:`turbo::simd::dispatch` function takes a functor whose call operator takes an architecture parameter as first operand, followed by any number of arguments ``Args...`` and turn it into a
dispatching functor that takes ``Args...`` as arguments.

.. doxygenfunction:: turbo::simd::dispatch
    :project: turbo-docs

Following code showcases a usage of the :cpp:func:`turbo::simd::dispatch` function:

.. code-block:: c++

    #include "sum.h"

    // Create the dispatching function, specifying the architecture we want to
    // target.
    auto dispatched = turbo::simd::dispatch<turbo::simd::arch_list<turbo::simd::avx2, turbo::simd::sse2>>(sum{});

    // Call the appropriate implementation based on runtime information.
    float res = dispatched(data, 17);

This code does *not* require any architecture-specific flags. The architecture
specific details follow.

The ``sum.h`` header contains the function being actually called, in an
architecture-agnostic description:

.. literalinclude:: ../../../../../tests/simd/doc/sum.h


The SSE2 and AVX2 version needs to be provided in other compilation units, compiled with the appropriate flags, for instance:

.. literalinclude:: ../../../../../tests/simd/doc/sum_avx2.cc

.. literalinclude:: ../../../../../tests/simd/doc/sum_sse2.cc

