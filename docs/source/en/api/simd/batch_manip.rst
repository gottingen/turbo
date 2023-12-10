.. Copyright 2023 The Turbo Authors.



Conditional expression
======================

+------------------------------+-------------------------------------------+
| :cpp:func:`select`           | conditional selection with mask           |
+------------------------------+-------------------------------------------+

----

.. doxygenfunction:: select(batch_bool<T, A> const &cond, batch<T, A> const &true_br, batch<T, A> const &false_br) noexcept
   :project: turbo-docs

.. doxygenfunction:: select(batch_bool_constant<batch<T, A>, Values...> const &cond, batch<T, A> const &true_br, batch<T, A> const &false_br) noexcept
   :project: turbo-docs


In the specific case when one needs to conditionnaly increment or decrement a
batch based on a mask, :cpp:func:`incr_if` and
:cpp:func:`decr_if` provide specialized version.
