//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../math/geometry/aligned_box.hpp"
#include "../math/quants/vec.hpp"
#include "../memory/actions.hpp"
#include "../memory/addr.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "../vector.hpp"
#include "__tree_store.hpp"
#include "__tree_walk.hpp"

namespace micron
{
namespace __subdiv
{

template<typename Value, typename F, usize Dim, usize Capacity, usize MaxDepth>
  requires micron::is_copy_constructible_v<Value> && (Dim >= 2 && Dim <= 3)
class pr_tree
{
public:
  using box_type = micron::math::geometry::aligned_box<F, Dim>;
  using point_type = micron::math::vec<F, Dim>;
  // value + spatial-key exposed for the FP layer
  using mapped_type = Value;
  using spatial_key_type = point_type;

private:
  using node_idx = __tree_store::node_idx;
  static constexpr node_idx nil = __tree_store::nil;
  static constexpr usize NCHILD = usize(1) << Dim;
  static constexpr u16 B = static_cast<u16>(Capacity);
  static_assert(Capacity >= 1, "bucket capacity must be >= 1");

  struct internal_node {
    u8 leaf;      // 0
    u8 _pad[3];
    node_idx kids[NCHILD];

    internal_node() noexcept : leaf(0)
    {
      for ( usize i = 0; i < NCHILD; ++i ) kids[i] = nil;
    }
  };

  struct leaf_node {
    u8 leaf;      // 1
    u8 _pad;
    u16 count;
    node_idx overflow;
    point_type pts[B];
    alignas(Value) byte vals_raw[sizeof(Value) * B];

    leaf_node() noexcept : leaf(1), _pad(0), count(0), overflow(nil) { }

    ~leaf_node()
    {
      Value *v = vals();
      for ( u16 i = 0; i < count; ++i ) v[i].~Value();
    }

    Value *
    vals() noexcept
    {
      return static_cast<Value *>(static_cast<void *>(vals_raw));
    }

    const Value *
    vals() const noexcept
    {
      return static_cast<const Value *>(static_cast<const void *>(vals_raw));
    }
  };

  static constexpr usize __slot_bytes = (sizeof(internal_node) > sizeof(leaf_node)) ? sizeof(internal_node) : sizeof(leaf_node);

  __tree_store::node_arena<__slot_bytes, 64> __arena;
  node_idx __root;
  usize __size;
  box_type __universe;

  [[gnu::always_inline]] leaf_node &
  lnode(node_idx i) noexcept
  {
    return *reinterpret_cast<leaf_node *>(__arena.raw(i));
  }

  [[gnu::always_inline]] internal_node &
  inode(node_idx i) noexcept
  {
    return *reinterpret_cast<internal_node *>(__arena.raw(i));
  }

  [[gnu::always_inline]] bool
  is_leaf(node_idx i) noexcept
  {
    return __arena.raw(i)[0] != 0;
  }

  static usize
  octant(const box_type &nb, const point_type &p) noexcept
  {
    point_type c = nb.center();
    usize oc = 0;
    for ( usize d = 0; d < Dim; ++d )
      if ( p.data[d] >= c.data[d] ) oc |= (usize(1) << d);
    return oc;
  }

  static box_type
  child_box(const box_type &nb, usize oc) noexcept
  {
    point_type c = nb.center();
    box_type cb;
    for ( usize d = 0; d < Dim; ++d ) {
      if ( oc & (usize(1) << d) ) {
        cb.min_corner.data[d] = c.data[d];
        cb.max_corner.data[d] = nb.max_corner.data[d];
      } else {
        cb.min_corner.data[d] = nb.min_corner.data[d];
        cb.max_corner.data[d] = c.data[d];
      }
    }
    return cb;
  }

  static F
  mindist2(const point_type &p, const box_type &b) noexcept
  {
    F s = F(0);
    for ( usize d = 0; d < Dim; ++d ) {
      F v = p.data[d];
      if ( v < b.min_corner.data[d] ) {
        F dd = b.min_corner.data[d] - v;
        s += dd * dd;
      } else if ( v > b.max_corner.data[d] ) {
        F dd = v - b.max_corner.data[d];
        s += dd * dd;
      }
    }
    return s;
  }

  static F
  dist2(const point_type &a, const point_type &b) noexcept
  {
    F s = F(0);
    for ( usize d = 0; d < Dim; ++d ) {
      F dd = a.data[d] - b.data[d];
      s += dd * dd;
    }
    return s;
  }

  node_idx
  alloc_leaf()
  {
    node_idx i = __arena.allocate();
    new (__arena.raw(i)) leaf_node();
    return i;
  }

  node_idx
  alloc_internal()
  {
    node_idx i = __arena.allocate();
    new (__arena.raw(i)) internal_node();
    return i;
  }

  void
  destroy_node(node_idx i) noexcept
  {
    if ( is_leaf(i) )
      lnode(i).~leaf_node();
    else
      inode(i).~internal_node();
    __arena.deallocate(i);
  }

  void
  free_subtree(node_idx i) noexcept
  {
    if ( i == nil ) return;
    if ( is_leaf(i) ) {
      // free the overflow chain iteratively (could be long for coincident points)
      node_idx o = lnode(i).overflow;
      destroy_node(i);
      while ( o != nil ) {
        node_idx nx = lnode(o).overflow;
        destroy_node(o);
        o = nx;
      }
      return;
    }
    internal_node &I = inode(i);
    node_idx kids[NCHILD];
    for ( usize c = 0; c < NCHILD; ++c ) kids[c] = I.kids[c];
    for ( usize c = 0; c < NCHILD; ++c ) free_subtree(kids[c]);
    destroy_node(i);
  }

  void
  add_to_leaf(node_idx n, const point_type &p, const Value &v) noexcept
  {
    while ( true ) {
      leaf_node &L = lnode(n);
      if ( L.count < B ) {
        L.pts[L.count] = p;
        new (micron::addr(L.vals()[L.count])) Value(v);
        ++L.count;
        return;
      }
      if ( L.overflow == nil ) {
        node_idx o = alloc_leaf();      // may move slab
        lnode(n).overflow = o;
      }
      n = lnode(n).overflow;
    }
  }

  node_idx
  insert_rec(node_idx n, const box_type &nb, const point_type &p, const Value &v, usize depth, bool &added)
  {
    if ( is_leaf(n) ) {
      if ( lnode(n).count < B || depth >= MaxDepth ) {
        add_to_leaf(n, p, v);
        added = true;
        return n;
      }
      // full and can subdivide: snapshot, free, rebuild as internal, reinsert
      const u16 cnt = lnode(n).count;
      point_type opts[B];
      alignas(Value) byte oraw[sizeof(Value) * B];
      Value *ov = static_cast<Value *>(static_cast<void *>(oraw));
      {
        leaf_node &L = lnode(n);
        for ( u16 i = 0; i < cnt; ++i ) {
          opts[i] = L.pts[i];
          new (micron::addr(ov[i])) Value(L.vals()[i]);
        }
      }
      destroy_node(n);
      node_idx in = alloc_internal();
      bool a;
      for ( u16 i = 0; i < cnt; ++i ) in = insert_rec(in, nb, opts[i], ov[i], depth, a);
      in = insert_rec(in, nb, p, v, depth, added);
      for ( u16 i = 0; i < cnt; ++i ) ov[i].~Value();
      return in;
    }
    usize oc = octant(nb, p);
    box_type cb = child_box(nb, oc);
    node_idx child = inode(n).kids[oc];
    if ( child == nil ) child = alloc_leaf();      // may move slab; re-fetch below
    node_idx nc = insert_rec(child, cb, p, v, depth + 1, added);
    inode(n).kids[oc] = nc;
    return n;
  }

  template<typename Fn>
  void
  query_rec(node_idx n, const box_type &nb, const box_type &q, Fn &fn) noexcept
  {
    if ( is_leaf(n) ) {
      node_idx c = n;
      while ( c != nil ) {
        leaf_node &L = lnode(c);
        for ( u16 i = 0; i < L.count; ++i )
          if ( q.contains(L.pts[i]) ) fn(static_cast<const point_type &>(L.pts[i]), L.vals()[i]);
        c = L.overflow;
      }
      return;
    }
    internal_node &I = inode(n);
    for ( usize oc = 0; oc < NCHILD; ++oc ) {
      if ( I.kids[oc] == nil ) continue;
      box_type cb = child_box(nb, oc);
      if ( cb.intersects(q) ) query_rec(I.kids[oc], cb, q, fn);
    }
  }

  template<typename Fn>
  void
  radius_rec(node_idx n, const box_type &nb, const point_type &c, F r2, Fn &fn) noexcept
  {
    if ( is_leaf(n) ) {
      node_idx cc = n;
      while ( cc != nil ) {
        leaf_node &L = lnode(cc);
        for ( u16 i = 0; i < L.count; ++i )
          if ( dist2(c, L.pts[i]) <= r2 ) fn(static_cast<const point_type &>(L.pts[i]), L.vals()[i]);
        cc = L.overflow;
      }
      return;
    }
    internal_node &I = inode(n);
    for ( usize oc = 0; oc < NCHILD; ++oc ) {
      if ( I.kids[oc] == nil ) continue;
      box_type cb = child_box(nb, oc);
      if ( mindist2(c, cb) <= r2 ) radius_rec(I.kids[oc], cb, c, r2, fn);
    }
  }

  template<typename Fn>
  void
  full_rec(node_idx n, Fn &fn) noexcept
  {
    if ( is_leaf(n) ) {
      node_idx c = n;
      while ( c != nil ) {
        leaf_node &L = lnode(c);
        for ( u16 i = 0; i < L.count; ++i ) fn(static_cast<const point_type &>(L.pts[i]), L.vals()[i]);
        c = L.overflow;
      }
      return;
    }
    internal_node &I = inode(n);
    for ( usize oc = 0; oc < NCHILD; ++oc )
      if ( I.kids[oc] != nil ) full_rec(I.kids[oc], fn);
  }

  struct knn_item {
    F d;
    point_type p;
    const Value *v;
  };

  static void
  knn_insert(fvector<knn_item> &best, usize k, const knn_item &it)
  {
    usize pos = best.size();
    best.push_back(it);
    while ( pos > 0 && best[pos - 1].d > best[pos].d ) {
      knn_item tmp = best[pos - 1];
      best[pos - 1] = best[pos];
      best[pos] = tmp;
      --pos;
    }
    if ( best.size() > k ) best.pop_back();
  }

  void
  knn_rec(node_idx n, const box_type &nb, const point_type &p, usize k, fvector<knn_item> &best) noexcept
  {
    if ( is_leaf(n) ) {
      node_idx c = n;
      while ( c != nil ) {
        leaf_node &L = lnode(c);
        for ( u16 i = 0; i < L.count; ++i ) {
          F d = dist2(p, L.pts[i]);
          if ( best.size() < k || d < best[best.size() - 1].d ) knn_insert(best, k, knn_item{ d, L.pts[i], micron::addr(L.vals()[i]) });
        }
        c = L.overflow;
      }
      return;
    }
    internal_node &I = inode(n);
    usize order[NCHILD];
    F od[NCHILD];
    usize m = 0;
    for ( usize oc = 0; oc < NCHILD; ++oc ) {
      if ( I.kids[oc] == nil ) continue;
      order[m] = oc;
      od[m] = mindist2(p, child_box(nb, oc));
      ++m;
    }
    for ( usize i = 1; i < m; ++i ) {
      usize co = order[i];
      F key = od[i];
      usize j = i;
      while ( j > 0 && od[j - 1] > key ) {
        od[j] = od[j - 1];
        order[j] = order[j - 1];
        --j;
      }
      od[j] = key;
      order[j] = co;
    }
    for ( usize t = 0; t < m; ++t ) {
      if ( best.size() >= k && od[t] >= best[best.size() - 1].d ) break;
      knn_rec(I.kids[order[t]], child_box(nb, order[t]), p, k, best);
    }
  }

public:
  using category_type = tree_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using size_type = usize;

  ~pr_tree()
  {
    if ( __root != nil ) free_subtree(__root);
  }

  explicit pr_tree(const box_type &universe) : __arena(), __root(nil), __size(0), __universe(universe) { }

  template<class Fn>
    requires(micron::is_invocable_v<Fn, pr_tree &>)
  pr_tree(const box_type &universe, Fn build) : pr_tree(universe)
  {
    build(*this);
  }

  pr_tree() : __arena(), __root(nil), __size(0)
  {
    for ( usize d = 0; d < Dim; ++d ) {
      __universe.min_corner.data[d] = F(-1048576);
      __universe.max_corner.data[d] = F(1048576);
    }
  }

  pr_tree(const pr_tree &) = delete;
  pr_tree &operator=(const pr_tree &) = delete;

  pr_tree(pr_tree &&o) noexcept : __arena(micron::move(o.__arena)), __root(o.__root), __size(o.__size), __universe(o.__universe)
  {
    o.__root = nil;
    o.__size = 0;
  }

  pr_tree &
  operator=(pr_tree &&o) noexcept
  {
    if ( this != &o ) {
      if ( __root != nil ) free_subtree(__root);
      __arena = micron::move(o.__arena);
      __root = o.__root;
      __size = o.__size;
      __universe = o.__universe;
      o.__root = nil;
      o.__size = 0;
    }
    return *this;
  }

  usize
  size() const noexcept
  {
    return __size;
  }

  bool
  empty() const noexcept
  {
    return __size == 0;
  }

  const box_type &
  bounds() const noexcept
  {
    return __universe;
  }

  void
  clear()
  {
    if ( __root != nil ) free_subtree(__root);
    __root = nil;
    __size = 0;
    __arena.reset();
  }

  // pre-size the node arena so insert() never allocates past this point (reserve at init,
  // where a throw is acceptable; the arena slab is retained across clear())
  void
  reserve_nodes(usize n)
  {
    __arena.reserve(n);
  }

  [[nodiscard, gnu::always_inline]] usize
  nodes_used() const noexcept
  {
    return __arena.slots_used();
  }

  [[nodiscard, gnu::always_inline]] usize
  nodes_reserved() const noexcept
  {
    return __arena.slots_reserved();
  }

  bool
  insert(const point_type &p, const Value &v)
  {
    if ( !__universe.contains(p) ) return false;
    if ( __root == nil ) __root = alloc_leaf();
    bool added = false;
    __root = insert_rec(__root, __universe, p, v, 0, added);
    if ( added ) ++__size;
    return added;
  }

  bool
  erase(const point_type &p, const Value &v)
  {
    if ( __root == nil || !__universe.contains(p) ) return false;
    node_idx n = __root;
    box_type nb = __universe;
    while ( !is_leaf(n) ) {
      usize oc = octant(nb, p);
      node_idx c = inode(n).kids[oc];
      if ( c == nil ) return false;
      nb = child_box(nb, oc);
      n = c;
    }

    node_idx cc = n;
    while ( cc != nil ) {
      leaf_node &L = lnode(cc);
      for ( u16 i = 0; i < L.count; ++i ) {
        if ( L.pts[i] == p && L.vals()[i] == v ) {
          L.vals()[i].~Value();
          for ( u16 j = i; j + 1 < L.count; ++j ) {
            L.pts[j] = L.pts[j + 1];
            new (micron::addr(L.vals()[j])) Value(micron::move(L.vals()[j + 1]));
            L.vals()[j + 1].~Value();
          }
          --L.count;
          --__size;
          return true;
        }
      }
      cc = L.overflow;
    }
    return false;
  }

  template<typename Fn>
  void
  query(const box_type &q, Fn &&fn) noexcept
  {
    if ( __root != nil ) query_rec(__root, __universe, q, fn);
  }

  template<typename Fn>
  void
  query_radius(const point_type &c, F r, Fn &&fn) noexcept
  {
    if ( __root != nil ) radius_rec(__root, __universe, c, r * r, fn);
  }

  template<typename Fn>
  void
  nearest(const point_type &p, usize k, Fn &&fn) noexcept
  {
    if ( __root == nil || k == 0 ) return;
    fvector<knn_item> best;
    best.reserve(k + 1);
    knn_rec(__root, __universe, p, k, best);
    for ( usize i = 0; i < best.size(); ++i ) fn(static_cast<const point_type &>(best[i].p), *best[i].v, best[i].d);
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) noexcept
  {
    if ( __root != nil ) full_rec(__root, fn);
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const noexcept
  {
    const_cast<pr_tree *>(this)->for_each([&](const point_type &p, Value &v) { fn(p, static_cast<const Value &>(v)); });
  }

  template<class Fn>
  auto
  map(Fn fn) const
  {
    if constexpr ( micron::is_invocable_v<Fn, const point_type &, const Value &> ) {
      using V2 = micron::remove_cvref_t<micron::invoke_result_t<Fn, const point_type &, const Value &>>;
      pr_tree<V2, F, Dim, Capacity, MaxDepth> out(this->bounds());
      for_each([&](const point_type &p, const Value &v) { out.insert(p, fn(p, v)); });
      return out;
    } else {
      using V2 = micron::remove_cvref_t<micron::invoke_result_t<Fn, const Value &>>;
      pr_tree<V2, F, Dim, Capacity, MaxDepth> out(this->bounds());
      for_each([&](const point_type &p, const Value &v) { out.insert(p, fn(v)); });
      return out;
    }
  }

  template<class Acc, class LeafElemFn, class InnerFn>
  Acc
  cata(const Acc &empty, LeafElemFn leaf_elem, InnerFn inner) const
  {
    if ( __root == nil ) return empty;
    return const_cast<pr_tree *>(this)->template __cata_rec<Acc>(__root, empty, leaf_elem, inner);
  }

  template<class Acc, class LeafElemFn, class InnerFn>
  Acc
  __cata_rec(node_idx n, const Acc &empty, LeafElemFn &leaf_elem, InnerFn &inner)
  {
    if ( is_leaf(n) ) {
      Acc acc = empty;
      node_idx c = n;
      while ( c != nil ) {
        leaf_node &L = lnode(c);
        for ( u16 i = 0; i < L.count; ++i )
          acc = leaf_elem(micron::move(acc), static_cast<const point_type &>(L.pts[i]), static_cast<const Value &>(L.vals()[i]));
        c = L.overflow;
      }
      return acc;
    }
    internal_node &I = inode(n);
    Acc kids[NCHILD];
    for ( usize c = 0; c < NCHILD; ++c ) kids[c] = (I.kids[c] != nil) ? __cata_rec<Acc>(I.kids[c], empty, leaf_elem, inner) : empty;
    return inner(kids, NCHILD);
  }

  template<class Fn>
  walk_ctl
  traverse(Fn fn) const
  {
    pr_tree *self = const_cast<pr_tree *>(this);
    if ( self->__root == nil ) return walk_ctl::continue_;
    return self->__traverse_rec(self->__root, fn);
  }

  template<class Fn>
  walk_ctl
  __traverse_rec(node_idx n, Fn &fn)
  {
    if ( is_leaf(n) ) {
      node_idx c = n;
      while ( c != nil ) {
        leaf_node &L = lnode(c);
        for ( u16 i = 0; i < L.count; ++i )
          if ( micron::__impl::invoke_walk(fn, static_cast<const point_type &>(L.pts[i]), static_cast<const Value &>(L.vals()[i]))
               == walk_ctl::stop )
            return walk_ctl::stop;
        c = L.overflow;
      }
      return walk_ctl::continue_;
    }
    internal_node &I = inode(n);
    for ( usize oc = 0; oc < NCHILD; ++oc )
      if ( I.kids[oc] != nil )
        if ( __traverse_rec(I.kids[oc], fn) == walk_ctl::stop ) return walk_ctl::stop;
    return walk_ctl::continue_;
  }
};

};      // namespace __subdiv
};      // namespace micron
