.. Copyright 2023 The Turbo Authors.

.. _turbo_simd_vectorized_code:



Writing vectorized code
=======================

Assume that we have a simple function that computes the mean of two vectors, something like:

.. literalinclude:: ../../../../tests/simd/doc/writing_vectorized_code.cc

How can we used `turbo::simd` to take advantage of vectorization?

Explicit use of an instruction set
----------------------------------

`turbo::simd` provides the template class :cpp:class:`turbo::simd::batch` parametrized by ``T`` and ``A`` types where ``T`` is the type of the values involved in SIMD
instructions and ``A`` is the target architecture. If you know which instruction set is available on your machine, you can directly use the corresponding specialization
of ``batch``. For instance, assuming the AVX instruction set is available, the previous code can be vectorized the following way:

.. literalinclude:: ../../../../tests/simd/doc/explicit_use_of_an_instruction_set_mean.cc


However, if you want to write code that is portable, you cannot rely on the use of ``turbo::simd::batch<double, turbo::simd::avx>``.
Indeed this won't compile on a CPU where only SSE2 instruction set is available for instance. Fortunately, if you don't set the second template parameter, `turbo::simd` picks the best architecture among the one available, based on the compiler flag you use.


Aligned vs unaligned memory
---------------------------

In the previous example, you may have noticed the :cpp:func:`turbo::simd::batch::load_unaligned` and :cpp:func:`turbo::simd::batch::store_unaligned` functions. These
are meant for loading values from contiguous dynamically allocated memory into SIMD registers and
reciprocally. When dealing with memory transfer operations, some instructions sets required the memory
to be aligned by a given amount, others can handle both aligned and unaligned modes. In that latter case,
operating on aligned memory is generally faster than operating on unaligned memory.

`turbo::simd` provides an aligned memory allocator, namely :cpp:class:`turbo::simd::aligned_allocator` which follows the standard requirements, so it can be used
with STL containers. Let's change the previous code so it can take advantage of this allocator:

.. literalinclude:: ../../../../tests/simd/doc/explicit_use_of_an_instruction_set_mean_aligned.cc


Memory alignment and tag dispatching
------------------------------------

You may need to write code that can operate on any type of vectors or arrays, not only the STL ones. In that
case, you cannot make assumption on the memory alignment of the container. `turbo::simd` provides a tag dispatching
mechanism that allows you to easily write such a generic code:


.. literalinclude:: ../../../../tests/simd/doc/explicit_use_of_an_instruction_set_mean_tag_dispatch.cc


Here, the ``Tag`` template parameter can be :cpp:class:`turbo::simd::aligned_mode` or :cpp:class:`turbo::simd::unaligned_mode`. Assuming the existence
of a ``get_alignment_tag`` meta-function in the code, the previous code can be invoked this way:

.. code::

    mean(a, b, res, get_alignment_tag<decltype(a)>());

Writing arch-independent code
-----------------------------

If your code may target either SSE2, AVX2 or AVX512 instruction set, `turbo::simd`
make it possible to make your code even more generic by using the architecture
as a template parameter:

.. literalinclude:: ../../../../tests/simd/doc/explicit_use_of_an_instruction_set_mean_arch_independent.cc

This can be useful to implement runtime dispatching, based on the instruction set detected at runtime. `turbo::simd` provides a generic machinery :cpp:func:`turbo::simd::dispatch()` to implement
this pattern. Based on the above example, instead of calling ``mean{}(arch, a, b, res, tag)``, one can use ``turbo::simd::dispatch(mean{})(a, b, res, tag)``. More about this can be found in the :ref:`Arch Dispatching` section.
