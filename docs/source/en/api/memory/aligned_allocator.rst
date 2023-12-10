.. Copyright 2023 The Turbo Authors.


alignment manipulation
======================

aligned memory allocator
------------------------

.. doxygenclass:: turbo::aligned_allocator
   :project: turbo-docs
   :members:

alignment checker
------------------

.. doxygenfunction:: turbo::is_aligned
   :project: turbo-docs

.. _turbo_alignment_tag:

alignment checker
------------------

The following empty types are used for tag dispatching:

.. doxygenstruct:: turbo::aligned_mode
   :project: turbo-docs

.. doxygenstruct:: turbo::unaligned_mode
   :project: turbo-docs