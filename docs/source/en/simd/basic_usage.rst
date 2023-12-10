.. Copyright 2023 The Turbo Authors.

.. _turbo_simd_basic_usage:

Basic usage
===========

Manipulating abstract batches
-----------------------------

Here is an example that computes the mean of two batches, using the best
architecture available, based on compile time informations:

.. literalinclude::../../../../tests/simd/doc/manipulating_abstract_batches.cc

The batch can be a batch of 4 single precision floating point numbers (e.g. on
Neon) ot a batch of 8 (e.g. on AVX2).

Manipulating parametric batches
-------------------------------

The previous example can be made fully parametric, both in the batch type and
the underlying architecture. This is achieved as described in the following
example:

.. literalinclude::../../../../tests/simd/doc/manipulating_parametric_batches.cc

At its core, a :cpp:class:`turbo::simd::batch` is bound to the scalar type it contains, and to the
instruction set it can use to operate on its values.

Explicit use of an instruction set extension
--------------------------------------------

Here is an example that loads two batches of 4 double floating point values, and
computes their mean, explicitly using the AVX extension:

.. literalinclude::../../../../tests/simd/doc/explicit_use_of_an_instruction_set.cc

Note that in that case, the instruction set is explicilty specified in the batch type.

This example outputs:

.. code::

   (2.0, 3.0, 4.0, 5.0)

.. warning::

   If you allow your compiler to generate AVX2 instructions (e.g. through
   ``-mavx2``) there is nothing preventing it to optimize the above code to
   optimize the above code using AVX2 instructions.
