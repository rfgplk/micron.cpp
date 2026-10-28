//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/heap/binary_heap.hpp"
#include "../../src/maps/heap_swiss.hpp"
#include "../../src/maps/hopscotch.hpp"
#include "../../src/maps/robin.hpp"
#include "../../src/queue/conqueue.hpp"
#include "../../src/queue/queue.hpp"
#include "../../src/sets/heap_swiss_set.hpp"
#include "../../src/sets/hopscotch_set.hpp"
#include "../../src/sets/robin_set.hpp"
#include "../../src/stacks/constack.hpp"
#include "../../src/stacks/fstack.hpp"
#include "../../src/stacks/stack.hpp"
#include "../../src/string/istring.hpp"
#include "../../src/string/rope.hpp"
#include "../../src/string/string.hpp"
#include "../../src/string/string_view.hpp"
#include "../../src/trees/b.hpp"
#include "../../src/trees/radix.hpp"
#include "../../src/trees/rb.hpp"
#include "../snowball/snowball.hpp"

extern "C" const char *
__ubsan_default_options()
{
  return "halt_on_error=1:exitcode=86";
}

struct tracked {
  static inline int live = 0;
  int value = 0;

  tracked() { ++live; }

  tracked(int v) : value(v) { ++live; }

  tracked(const tracked &o) : value(o.value) { ++live; }

  tracked(tracked &&o) noexcept : value(o.value) { ++live; }

  tracked &operator=(const tracked &) = default;
  tracked &operator=(tracked &&) = default;

  ~tracked() { --live; }

  bool operator==(const tracked &) const = default;
};

template<class Stack>
static void
empty_stack()
{
  Stack zero(usize{ 0 });
  zero.clear();
  zero.reserve(0);
  Stack copy(zero);
  Stack moved(micron::move(zero));
  zero.clear();
  zero = copy;
  copy = micron::move(moved);
  sb::require_true(zero.empty() && copy.empty() && moved.empty());
  zero.push(tracked{ 42 });
  sb::require_true(zero.top().value == 42);
  zero.clear();
}

template<class Queue>
static void
empty_queue()
{
  Queue zero(usize{ 0 });
  zero.clear();
  zero.reserve(0);
  Queue moved(micron::move(zero));
  zero.clear();
  sb::require_true(zero.begin() == zero.end() && moved.begin() == moved.end());
  zero.push(tracked{ 42 });
  sb::require_true(zero.size() == 1);
  zero.clear();
  moved = micron::move(zero);
  sb::require_true(zero.empty() && moved.empty());
}

static void
moved_robin_ranges()
{
  micron::robin_map<int, int> source(0);
  micron::robin_map<int, int> destination(micron::move(source));
  sb::require_true(source.raw_begin() == nullptr && source.raw_end() == nullptr);
  const auto &constant = source;
  sb::require_true(constant.raw_begin() == nullptr && constant.raw_end() == nullptr);
  sb::require_true(source.begin() == source.end());
  sb::require_true(static_cast<usize>(destination.raw_end() - destination.raw_begin()) == destination.max_size());
}

static void
moved_string_clear()
{
  micron::hstring<char> source("content");
  micron::hstring<char> destination(micron::move(source));
  source.clear();
  sb::require_true(source.empty() && source.begin() == source.end());
  sb::require_true(destination == "content");
  micron::hstring<char> copy(source);
  sb::require_true(copy.empty());
  source = "reused";
  sb::require_true(source == "reused");
}

int
main(int argc, char **argv)
{
  if ( argc > 1 ) {
    if ( argv[1][0] == 'r' )
      moved_robin_ranges();
    else
      moved_string_clear();
    return 1;
  }

  sb::test_case("zero-capacity stack and queue lifetimes");
  empty_stack<micron::stack<tracked, 0>>();
  empty_stack<micron::fstack<tracked, 0>>();
  empty_stack<micron::constack<tracked, 0>>();
  empty_queue<micron::queue<tracked, 0>>();
  empty_queue<micron::conqueue<tracked, 0>>();
  sb::require_true(tracked::live == 0);
  sb::end_test_case();

  sb::test_case("empty maps and their set adapters survive moves");
  moved_robin_ranges();
  {
    micron::hopscotch_map<int, int> source(0);
    micron::hopscotch_map<int, int> moved(micron::move(source));
    sb::require_true(source.begin() == source.end() && moved.begin() == moved.end());
    micron::heap_swiss_map<int, int> heap(0);
    micron::heap_swiss_map<int, int> heap_moved(micron::move(heap));
    heap.clear();
    sb::require_true(heap.begin() == heap.end() && heap_moved.begin() == heap_moved.end());
    micron::robin_set<int> robin(0);
    micron::robin_set<int> robin_moved(micron::move(robin));
    micron::hopscotch_set<int> hopscotch;
    micron::hopscotch_set<int> hopscotch_moved(micron::move(hopscotch));
    micron::heap_swiss_set<int> swiss(0);
    micron::heap_swiss_set<int> swiss_moved(micron::move(swiss));
    sb::require_true(robin.empty() && robin_moved.empty() && hopscotch.empty() && hopscotch_moved.empty() && swiss.empty()
                     && swiss_moved.empty());
  }
  sb::end_test_case();

  sb::test_case("empty strings and views preserve empty ranges");
  moved_string_clear();
  {
    micron::hstring<char> zero(usize{ 0 });
    micron::istring<char> immutable(usize{ 0 });
    micron::istring<char> immutable_moved(micron::move(immutable));
    immutable.clear();
    sb::require_true(zero.empty() && immutable.begin() == immutable.end() && immutable_moved.empty());
    micron::rope<char> rope;
    sb::require_true(rope.into_chars().is_empty());
    micron::string_view<micron::hstring<char>> view(static_cast<const char *>(nullptr), usize{ 0 });
    sb::require_true(view.empty() && view.size() == 0 && view.substr(0, 0).empty());
  }
  sb::end_test_case();

  sb::test_case("empty trees and a zero-capacity heap release cleanly");
  {
    micron::binary_heap<int> heap(0);
    micron::binary_heap<int> moved(micron::move(heap));
    heap.clear();
    moved.clear();
    sb::require_true(heap.size() == 0 && moved.size() == 0);
    micron::rb_tree<int> red_black;
    micron::rb_tree<int> red_black_moved(micron::move(red_black));
    micron::b_tree<int, int> btree;
    micron::b_tree<int, int> btree_moved(micron::move(btree));
    micron::radix_tree<micron::hstring<char>, int> radix;
    micron::radix_tree<micron::hstring<char>, int> radix_moved(micron::move(radix));
    sb::require_true(red_black.empty() && red_black_moved.empty() && btree.empty() && btree_moved.empty() && radix.empty()
                     && radix_moved.empty());
  }
  sb::end_test_case();
  return 1;
}
