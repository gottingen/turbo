#pragma once

#include <atomic>

namespace turbo {

/**
@brief finds the median of three numbers of dereferenced iterators using
       the given comparator
*/
template <typename RandItr, typename C>
RandItr median_of_three(RandItr l, RandItr m, RandItr r, C cmp) {
  return cmp(*l, *m) ? (cmp(*m, *r) ? m : (cmp(*l, *r) ? r : l ))
                     : (cmp(*r, *m) ? m : (cmp(*r, *l) ? r : l ));
}

/**
@brief finds the pseudo median of a range of items using spreaded
       nine numbers
 */
template <typename RandItr, typename C>
RandItr pseudo_median_of_nine(RandItr beg, RandItr end, C cmp) {
  size_t N = std::distance(beg, end);
  size_t offset = N >> 3;
  return median_of_three(
    median_of_three(beg, beg+offset, beg+(offset*2), cmp),
    median_of_three(beg+(offset*3), beg+(offset*4), beg+(offset*5), cmp),
    median_of_three(beg+(offset*6), beg+(offset*7), end-1, cmp),
    cmp
  );
}

/**
@brief sorts two elements of dereferenced iterators using the given
       comparison function
*/
template<typename Iter, typename Compare>
void sort2(Iter a, Iter b, Compare comp) {
  if (comp(*b, *a)) std::iter_swap(a, b);
}

/**
@brief sorts three elements of dereferenced iterators using the given
       comparison function
*/
template<typename Iter, typename Compare>
void sort3(Iter a, Iter b, Iter c, Compare comp) {
  sort2(a, b, comp);
  sort2(b, c, comp);
  sort2(a, b, comp);
}


}  // namespace turbo

