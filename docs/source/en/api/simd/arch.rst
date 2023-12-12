.. Copyright 2023 The Turbo Authors.



Architecture manipulation
=========================

turbo's simd provides an high level description of the instruction sets it manipulates.
The mentionned types are primarily used as template parameters for :ref:`batch
<simd-batch-ref>`, and when interacting with :cpp:func:`turbo::simd::dispatch()`.

The best available architecture is available at compile time through
``turbo::simd::best_arch`` which also happens to be ``turbo::simd::default_arch``.

.. doxygengroup:: architectures
   :project: turbo-docs
   :members:
