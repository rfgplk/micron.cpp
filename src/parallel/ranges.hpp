//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "compaction.hpp"
#include "elementwise.hpp"
#include "find.hpp"
#include "for_each.hpp"
#include "numeric.hpp"
#include "reduce.hpp"
#include "sort.hpp"

namespace micron
{
namespace parallel
{

template<class C, class Out, class Fn>
[[nodiscard]] auto
transform_range(C &__c, Out __out, Fn __fn)
{
  return transform(__c.begin(), __c.end(), __out, __fn);
}

template<class C, class Pred>
[[nodiscard]] auto
all_of_range(C &__c, Pred __p)
{
  return all_of(__c.begin(), __c.end(), __p);
}

template<class C, class Pred>
[[nodiscard]] auto
any_of_range(C &__c, Pred __p)
{
  return any_of(__c.begin(), __c.end(), __p);
}

template<class C, class Pred>
[[nodiscard]] auto
none_of_range(C &__c, Pred __p)
{
  return none_of(__c.begin(), __c.end(), __p);
}

template<class C, class Pred>
[[nodiscard]] auto
count_if_range(C &__c, Pred __p)
{
  return count_if(__c.begin(), __c.end(), __p);
}

template<class C>
[[nodiscard]] auto
sum_range(C &__c)
{
  return sum(__c.begin(), __c.end());
}

template<class C>
[[nodiscard]] auto
mean_range(C &__c)
{
  return mean(__c.begin(), __c.end());
}

template<class C>
[[nodiscard]] auto
min_range(C &__c)
{
  return min(__c.begin(), __c.end());
}

template<class C>
[[nodiscard]] auto
max_range(C &__c)
{
  return max(__c.begin(), __c.end());
}

template<class C>
[[nodiscard]] auto
is_sorted_range(C &__c)
{
  return is_sorted(__c.begin(), __c.end());
}

template<class C, class V>
[[nodiscard]] auto
count_range(C &__c, V __value)
{
  return count(__c.begin(), __c.end(), __value);
}

template<class C, class V>
[[nodiscard]] auto
contains_range(C &__c, V __value)
{
  return contains(__c.begin(), __c.end(), __value);
}

template<class C, class V>
[[nodiscard]] auto
find_range(C &__c, V __value)
{
  return find(__c.begin(), __c.end(), __value);
}

template<class C, class Pred>
[[nodiscard]] auto
find_if_range(C &__c, Pred __p)
{
  return find_if(__c.begin(), __c.end(), __p);
}

template<class C, class V>
[[nodiscard]] auto
fill_range(C &__c, V __value)
{
  return fill(__c.begin(), __c.end(), __value);
}

template<class C>
[[nodiscard]] auto
reverse_range(C &__c)
{
  return reverse(__c.begin(), __c.end());
}

namespace sort
{
template<class C, class Cmp = __pless>
[[nodiscard]] auto
merge_range(C &__c, Cmp __comp = Cmp{})
{
  return merge(__c.begin(), __c.end(), __comp);
}

template<class C, class Cmp = __pless>
[[nodiscard]] auto
quick_range(C &__c, Cmp __comp = Cmp{})
{
  return quick(__c.begin(), __c.end(), __comp);
}

template<class C, class Cmp = __pless>
[[nodiscard]] auto
sort_range(C &__c, Cmp __comp = Cmp{})
{
  return sort(__c.begin(), __c.end(), __comp);
}

template<class C>
[[nodiscard]] auto
radix_range(C &__c)
{
  return radix(__c.begin(), __c.end());
}
};      // namespace sort

};      // namespace parallel
};      // namespace micron
