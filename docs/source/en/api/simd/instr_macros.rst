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

Instruction set macros
======================

Each of these macros corresponds to an instruction set supported by turbo. They
can be used to filter arch-specific code.

.. doxygengroup:: turbo_simd_config_macro
   :project: turbo-docs
   :content-only:

Changing Default architecture
*****************************

You can change the default instruction set used by simd (when none is provided
explicitely) by setting the ``TURBO_SIMD_DEFAULT_ARCH`` macro to, say, ``turbo::simd::avx2``.
A common usage is to set it to ``turbo::simd::unsupported`` as a way to detect
instantiation of batches with the default architecture.
