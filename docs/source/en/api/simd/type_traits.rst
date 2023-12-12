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

.. _Type Traits:

Type Traits
===========

`turbo::simd` provides a few type traits to interact with scalar and batch types in an
uniformeous manner.


Type check:

+---------------------------------------+----------------------------------------------------+
| :cpp:class:`is_batch`                 | batch type detection                               |
+---------------------------------------+----------------------------------------------------+
| :cpp:class:`is_batch_bool`            | mask batch type detection                          |
+---------------------------------------+----------------------------------------------------+
| :cpp:class:`is_batch_complex`         | complex batch type detection                       |
+---------------------------------------+----------------------------------------------------+

Type access:

+---------------------------------------+----------------------------------------------------+
| :cpp:class:`scalar_type`              | batch element type                                 |
+---------------------------------------+----------------------------------------------------+
| :cpp:class:`mask_type`                | batch mask type                                    |
+---------------------------------------+----------------------------------------------------+

----

.. doxygengroup:: batch_traits
   :project: turbo-docs
   :content-only:
