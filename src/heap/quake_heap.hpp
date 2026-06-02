//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../concepts.hpp"
#include "../memory/memory.hpp"
#include "../simd/reduce.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../vector/fvector.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//  quake_heap
//
//  genuine quake heap; a forest of binary tournament trees; root list is stored as SoA
template<typename T> struct __quake_less {
  constexpr bool
  operator()(const T &a, const T &b) const
  {
    return a < b;
  }
};

template<typename T> struct __quake_greater {
  constexpr bool
  operator()(const T &a, const T &b) const
  {
    return a > b;
  }
};

template<typename T, class Compare>
inline constexpr bool __quake_simd_enabled
    = micron::simd::reducible_key<T> && (micron::is_same_v<Compare, __quake_less<T>> || micron::is_same_v<Compare, __quake_greater<T>>);

template<typename T, class Compare = micron::__quake_less<T>, class Alloc = micron::allocator_serial<>, u32 AlphaNum = 3, u32 AlphaDen = 4>
class quake_heap
{
  static_assert(AlphaDen != 0 && AlphaNum < AlphaDen, "quake_heap: alpha = AlphaNum/AlphaDen must lie in (0,1)");

  // tree height is O(log n); 64 levels covers any n addressable in 64 bits
  static constexpr u32 kMaxLevels = 64;
  static constexpr bool __simd_on = __quake_simd_enabled<T, Compare>;
  static constexpr bool __simd_min = micron::is_same_v<Compare, micron::__quake_less<T>>;      // argmin vs argmax

  struct entry;

  struct node {
    entry *e;
    node *left;
    node *right;
    node *parent;
    u32 height;
    u32 fpos;
  };

  struct entry {
    T value;
    node *high;
  };

  struct __nokeys {
  };

  using fkeys_t = micron::conditional_t<__simd_on, micron::fvector<T, Alloc>, __nokeys>;

  micron::fvector<node *, Alloc> forest;      // SoA root list
  fkeys_t fkeys;                              // parallel key cache (SIMD only)
  node *min_node;                             // comparator-extreme root
  usize total;                                // element count
  u32 counts[kMaxLevels];                     // n_i : nodes at each height
  u32 levels;                                 // number of occupied count slots (max height + 1)
  Compare comp;

  node *__node_pool;
  entry *__entry_pool;

  [[gnu::always_inline]] inline node *
  __acquire_node()
  {
    if ( __node_pool ) {
      node *n = __node_pool;
      __node_pool = *reinterpret_cast<node **>(n);
      return n;
    }
    return static_cast<node *>(::operator new(sizeof(node)));
  }

  [[gnu::always_inline]] inline void
  __recycle_node(node *n) noexcept
  {
    *reinterpret_cast<node **>(n) = __node_pool;
    __node_pool = n;
  }

  template<typename... A>
  [[gnu::always_inline]] inline entry *
  __acquire_entry(A &&...a)
  {
    entry *e;
    if ( __entry_pool ) {
      e = __entry_pool;
      __entry_pool = *reinterpret_cast<entry **>(e);
    } else {
      e = static_cast<entry *>(::operator new(sizeof(entry)));
    }
    new (static_cast<void *>(e)) entry{ T(micron::forward<A>(a)...), nullptr };
    return e;
  }

  [[gnu::always_inline]] inline void
  __recycle_entry(entry *e) noexcept
  {
    e->~entry();
    *reinterpret_cast<entry **>(e) = __entry_pool;
    __entry_pool = e;
  }

  inline void
  __drain_pools() noexcept
  {
    while ( __node_pool ) {
      node *nx = *reinterpret_cast<node **>(__node_pool);
      ::operator delete(static_cast<void *>(__node_pool));
      __node_pool = nx;
    }
    while ( __entry_pool ) {
      entry *nx = *reinterpret_cast<entry **>(__entry_pool);
      ::operator delete(static_cast<void *>(__entry_pool));
      __entry_pool = nx;
    }
  }

  [[gnu::always_inline]] inline void
  __push_root(node *n)
  {
    n->fpos = static_cast<u32>(forest.size());
    n->parent = nullptr;
    forest.push_back(n);
    if constexpr ( __simd_on ) fkeys.push_back(n->e->value);
  }

  [[gnu::always_inline]] inline void
  __erase_root(u32 pos)
  {
    const u32 last = static_cast<u32>(forest.size()) - 1;
    if ( pos != last ) {
      node *m = forest[last];
      forest[pos] = m;
      m->fpos = pos;
      if constexpr ( __simd_on ) fkeys[pos] = fkeys[last];
    }
    forest.pop_back();
    if constexpr ( __simd_on ) fkeys.pop_back();
  }

  inline void
  __bump_levels(u32 h) noexcept
  {
    if ( h + 1 > levels ) levels = h + 1;
  }

  node *
  __link(node *a, node *b)
  {
    node *w, *l;
    if ( comp(b->e->value, a->e->value) ) {
      w = b;
      l = a;
    } else {
      w = a;
      l = b;
    }
    node *p = __acquire_node();
    p->e = w->e;
    p->left = w;
    p->right = l;
    p->parent = nullptr;
    p->height = w->height + 1;
    w->parent = p;
    l->parent = p;
    w->e->high = p;
    if ( p->height < kMaxLevels ) counts[p->height]++;
    __bump_levels(p->height);
    return p;
  }

  void
  __shake(node *r)
  {
    __erase_root(r->fpos);
    entry *te = r->e;
    node *cur = r;
    while ( cur ) {
      if ( cur->height < kMaxLevels && counts[cur->height] ) counts[cur->height]--;
      node *L = cur->left;
      node *R = cur->right;
      node *win = nullptr;
      if ( L && L->e == te ) {
        win = L;
        if ( R ) __push_root(R);
      } else if ( R && R->e == te ) {
        win = R;
        if ( L ) __push_root(L);
      } else {      // leaf (no child carries te)
        if ( L ) __push_root(L);
        if ( R ) __push_root(R);
      }
      __recycle_node(cur);
      cur = win;
    }
  }

  void
  __consolidate()
  {
    node *by_h[kMaxLevels];
    for ( u32 i = 0; i < kMaxLevels; ++i ) by_h[i] = nullptr;

    const u32 cnt = static_cast<u32>(forest.size());
    for ( u32 i = 0; i < cnt; ++i ) {
      node *r = forest[i];
      r->parent = nullptr;
      u32 h = r->height;
      while ( h < kMaxLevels && by_h[h] ) {
        node *o = by_h[h];
        by_h[h] = nullptr;
        r = __link(r, o);
        h = r->height;
      }
      if ( h < kMaxLevels ) by_h[h] = r;
    }

    forest.clear();
    if constexpr ( __simd_on ) fkeys.clear();
    for ( u32 h = 0; h < kMaxLevels; ++h )
      if ( by_h[h] ) __push_root(by_h[h]);
  }

  void
  __decap(node *n, u32 keep)
  {
    if ( n->height <= keep ) {
      n->parent = nullptr;
      n->e->high = n;
      __push_root(n);
      return;
    }
    if ( n->left ) __decap(n->left, keep);
    if ( n->right ) __decap(n->right, keep);
    __recycle_node(n);
  }

  void
  __quake()
  {
    if ( levels < 2 ) return;
    const usize viol = micron::simd::first_quake_violation(counts, levels, AlphaNum, AlphaDen);
    if ( viol >= levels ) return;      // invariant holds everywhere

    const u32 keep = static_cast<u32>(viol);

    node *roots[kMaxLevels];
    u32 rc = 0;
    const u32 cnt = static_cast<u32>(forest.size());
    for ( u32 i = 0; i < cnt && rc < kMaxLevels; ++i ) roots[rc++] = forest[i];

    forest.clear();
    if constexpr ( __simd_on ) fkeys.clear();
    for ( u32 h = keep + 1; h < levels; ++h ) counts[h] = 0;
    levels = keep + 1;

    for ( u32 i = 0; i < rc; ++i ) __decap(roots[i], keep);
  }

  inline void
  __update_min()
  {
    if ( forest.size() == 0 ) {
      min_node = nullptr;
      return;
    }
    usize idx;
    if constexpr ( __simd_on ) {
      idx = __simd_min ? micron::simd::argmin<T>(fkeys.data(), fkeys.size()) : micron::simd::argmax<T>(fkeys.data(), fkeys.size());
    } else {
      idx = 0;
      const usize m = forest.size();
      for ( usize k = 1; k < m; ++k )
        if ( comp(forest[k]->e->value, forest[idx]->e->value) ) idx = k;
    }
    min_node = forest[idx];
  }

  void
  __free_subtree(node *n) noexcept
  {
    if ( !n ) return;
    __free_subtree(n->left);
    __free_subtree(n->right);
    if ( n->e->high == n ) __recycle_entry(n->e);
    __recycle_node(n);
  }

  inline void
  __zero_counts() noexcept
  {
    for ( u32 i = 0; i < kMaxLevels; ++i ) counts[i] = 0;
    levels = 0;
  }

public:
  using category_type = theap_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  using value_type = T;
  using size_type = usize;
  using reference = T &;
  using const_reference = const T &;

  struct handle {
    entry *e = nullptr;

    constexpr bool
    valid() const noexcept
    {
      return e != nullptr;
    }
  };

private:
  handle
  __insert_entry(entry *e)
  {
    node *n = __acquire_node();
    n->e = e;
    n->left = nullptr;
    n->right = nullptr;
    n->parent = nullptr;
    n->height = 0;
    e->high = n;
    __push_root(n);
    if ( counts[0]++ == 0 && levels < 1 ) levels = 1;
    __bump_levels(0);
    ++total;
    if ( !min_node || comp(e->value, min_node->e->value) ) min_node = n;
    return handle{ e };
  }

public:
  ~quake_heap()
  {
    clear();
    __drain_pools();
  }

  quake_heap() : forest(), fkeys(), min_node(nullptr), total(0), levels(0), comp(), __node_pool(nullptr), __entry_pool(nullptr)
  {
    for ( u32 i = 0; i < kMaxLevels; ++i ) counts[i] = 0;
  }

  explicit quake_heap(const Compare &c) : quake_heap() { comp = c; }

  quake_heap(const quake_heap &) = delete;
  quake_heap &operator=(const quake_heap &) = delete;

  quake_heap(quake_heap &&o) noexcept
      : forest(micron::move(o.forest)), fkeys(micron::move(o.fkeys)), min_node(o.min_node), total(o.total), levels(o.levels),
        comp(micron::move(o.comp)), __node_pool(o.__node_pool), __entry_pool(o.__entry_pool)
  {
    for ( u32 i = 0; i < kMaxLevels; ++i ) counts[i] = o.counts[i];
    o.min_node = nullptr;
    o.total = 0;
    o.levels = 0;
    o.__node_pool = nullptr;
    o.__entry_pool = nullptr;
    for ( u32 i = 0; i < kMaxLevels; ++i ) o.counts[i] = 0;
  }

  quake_heap &
  operator=(quake_heap &&o) noexcept
  {
    if ( this != &o ) {
      clear();
      __drain_pools();
      forest = micron::move(o.forest);
      if constexpr ( __simd_on ) fkeys = micron::move(o.fkeys);
      min_node = o.min_node;
      total = o.total;
      levels = o.levels;
      comp = micron::move(o.comp);
      __node_pool = o.__node_pool;
      __entry_pool = o.__entry_pool;
      for ( u32 i = 0; i < kMaxLevels; ++i ) counts[i] = o.counts[i];
      o.min_node = nullptr;
      o.total = 0;
      o.levels = 0;
      o.__node_pool = nullptr;
      o.__entry_pool = nullptr;
      for ( u32 i = 0; i < kMaxLevels; ++i ) o.counts[i] = 0;
    }
    return *this;
  }

  bool
  empty() const noexcept
  {
    return total == 0;
  }

  usize
  size() const noexcept
  {
    return total;
  }

  const_reference
  find_min() const
  {
    if ( !min_node ) exc<except::library_error>("quake_heap::find_min() empty");
    return min_node->e->value;
  }

  const_reference
  peek() const
  {
    return find_min();
  }

  handle
  insert(const T &v)
  {
    return __insert_entry(__acquire_entry(v));
  }

  handle
  insert(T &&v)
  {
    return __insert_entry(__acquire_entry(micron::move(v)));
  }

  template<typename... Args>
  handle
  emplace(Args &&...args)
  {
    return __insert_entry(__acquire_entry(micron::forward<Args>(args)...));
  }

  void
  decrease_key(handle h, const T &nv)
  {
    entry *e = h.e;
    if ( !e ) return;
    if ( !comp(nv, e->value) ) return;
    e->value = nv;
    node *x = e->high;
    if ( x->parent ) {
      node *p = x->parent;
      if ( p->left == x )
        p->left = nullptr;
      else if ( p->right == x )
        p->right = nullptr;
      __push_root(x);
    } else if constexpr ( __simd_on ) {
      fkeys[x->fpos] = e->value;
    }
    if ( !min_node || comp(e->value, min_node->e->value) ) min_node = x;
  }

  T
  extract_min()
  {
    if ( !min_node ) exc<except::library_error>("quake_heap::extract_min() empty");
    node *r = min_node;
    entry *e = r->e;
    T result = micron::move(e->value);
    __shake(r);
    __recycle_entry(e);
    --total;
    min_node = nullptr;
    __consolidate();
    __quake();
    __update_min();
    return result;
  }

  void
  erase(handle h)
  {
    entry *e = h.e;
    if ( !e || total == 0 ) return;
    node *x = e->high;
    if ( x->parent ) {
      node *p = x->parent;
      if ( p->left == x )
        p->left = nullptr;
      else if ( p->right == x )
        p->right = nullptr;
      __push_root(x);
    }
    min_node = x;
    (void)extract_min();
  }

  void
  meld(quake_heap &&o)
  {
    if ( o.total == 0 ) return;
    const u32 ocnt = static_cast<u32>(o.forest.size());
    node *omin = o.min_node;
    for ( u32 i = 0; i < ocnt; ++i ) __push_root(o.forest[i]);
    for ( u32 h = 0; h < o.levels; ++h ) counts[h] += o.counts[h];
    if ( o.levels > levels ) levels = o.levels;
    total += o.total;
    if ( !min_node || (omin && comp(omin->e->value, min_node->e->value)) ) min_node = omin;

    o.forest.set_size(0);
    if constexpr ( __simd_on ) o.fkeys.set_size(0);
    o.min_node = nullptr;
    o.total = 0;
    for ( u32 i = 0; i < kMaxLevels; ++i ) o.counts[i] = 0;
    o.levels = 0;
  }

  void
  clear() noexcept
  {
    const usize m = forest.size();
    for ( usize i = 0; i < m; ++i ) __free_subtree(forest[i]);
    forest.clear();
    if constexpr ( __simd_on ) fkeys.clear();
    __zero_counts();
    total = 0;
    min_node = nullptr;
  }
};

};      // namespace micron
