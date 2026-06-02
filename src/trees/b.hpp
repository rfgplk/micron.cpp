//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/addr.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "__tree_store.hpp"
#include "__tree_walk.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron::b_tree
// ordered, arena-backed B+-tree map, values live only in leaves; internal nodes hold separator keys + child
// leaves are doubly linked; insert/erase is top-down

// default ordering policy
template<typename T> struct b_default_less {
  static bool
  lt(const T &a, const T &b)
  {
    return a < b;
  }
};

template<typename K, typename V, typename Compare = b_default_less<K>, usize DegreeOverride = 0>
  requires micron::is_copy_constructible_v<K> && micron::is_move_constructible_v<V>
class b_tree
{
  using node_idx = __tree_store::node_idx;
  static constexpr node_idx nil = __tree_store::nil;

  static constexpr usize __kv = sizeof(K) + sizeof(V);
  static constexpr usize __auto_t = (__kv <= 16) ? 16u : ((__kv <= 64) ? 8u : 4u);
  static constexpr usize T = (DegreeOverride > 0) ? DegreeOverride : __auto_t;
  static_assert(T >= 2, "b_tree minimum degree must be >= 2");

  static constexpr u16 MAXK = static_cast<u16>(2 * T - 1);      // max keys / leaf entries
  static constexpr u16 MAXC = static_cast<u16>(2 * T);          // max children
  static constexpr u16 MINK = static_cast<u16>(T - 1);          // min keys (non-root)

  struct internal_node {
    u8 leaf;      // 0
    u8 _pad;
    u16 nkeys;
    node_idx kids[MAXC];
    alignas(K) byte keys_raw[sizeof(K) * MAXK];

    internal_node() noexcept : leaf(0), _pad(0), nkeys(0) { }

    ~internal_node()
    {
      K *k = keys();
      for ( u16 i = 0; i < nkeys; ++i ) k[i].~K();
    }

    K *
    keys() noexcept
    {
      return static_cast<K *>(static_cast<void *>(keys_raw));
    }

    const K *
    keys() const noexcept
    {
      return static_cast<const K *>(static_cast<const void *>(keys_raw));
    }
  };

  struct leaf_node {
    u8 leaf;      // 1
    u8 _pad;
    u16 count;
    node_idx next;
    node_idx prev;
    alignas(K) byte keys_raw[sizeof(K) * MAXK];
    alignas(V) byte vals_raw[sizeof(V) * MAXK];

    leaf_node() noexcept : leaf(1), _pad(0), count(0), next(nil), prev(nil) { }

    ~leaf_node()
    {
      K *k = keys();
      V *v = vals();
      for ( u16 i = 0; i < count; ++i ) {
        k[i].~K();
        v[i].~V();
      }
    }

    K *
    keys() noexcept
    {
      return static_cast<K *>(static_cast<void *>(keys_raw));
    }

    const K *
    keys() const noexcept
    {
      return static_cast<const K *>(static_cast<const void *>(keys_raw));
    }

    V *
    vals() noexcept
    {
      return static_cast<V *>(static_cast<void *>(vals_raw));
    }

    const V *
    vals() const noexcept
    {
      return static_cast<const V *>(static_cast<const void *>(vals_raw));
    }
  };

  static constexpr usize __slot_bytes = (sizeof(internal_node) > sizeof(leaf_node)) ? sizeof(internal_node) : sizeof(leaf_node);

  __tree_store::node_arena<__slot_bytes, 64> __arena;
  node_idx __root;
  usize __size;

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

  [[gnu::always_inline]] u16
  entries(node_idx i) noexcept
  {
    return is_leaf(i) ? lnode(i).count : inode(i).nkeys;
  }

  static bool
  eq(const K &a, const K &b)
  {
    return !Compare::lt(a, b) && !Compare::lt(b, a);
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
    if ( !is_leaf(i) ) {
      internal_node &I = inode(i);
      u16 nk = I.nkeys;
      node_idx kids[MAXC];
      for ( u16 c = 0; c <= nk; ++c ) kids[c] = I.kids[c];
      for ( u16 c = 0; c <= nk; ++c ) free_subtree(kids[c]);
    }
    destroy_node(i);
  }

  [[gnu::always_inline]] u16
  child_index(internal_node &I, const K &key) noexcept
  {
    u16 ci = 0;
    while ( ci < I.nkeys && !Compare::lt(key, I.keys()[ci]) ) ++ci;
    return ci;
  }

  [[gnu::always_inline]] u16
  leaf_lb(leaf_node &L, const K &key) noexcept
  {
    u16 p = 0;
    while ( p < L.count && Compare::lt(L.keys()[p], key) ) ++p;
    return p;
  }

  void
  split_child(node_idx parent, u16 ci)
  {
    node_idx cidx = inode(parent).kids[ci];
    const bool child_leaf = is_leaf(cidx);
    node_idx ridx = child_leaf ? alloc_leaf() : alloc_internal();      // may move the slab

    if ( child_leaf ) {
      leaf_node &C = lnode(cidx);
      leaf_node &R = lnode(ridx);
      const u16 right_n = static_cast<u16>(MAXK - T);
      __tree_store::relocate_n(R.keys(), C.keys() + T, right_n);
      __tree_store::relocate_n(R.vals(), C.vals() + T, right_n);
      C.count = static_cast<u16>(T);
      R.count = right_n;
      // splice R after C in the leaf list
      R.next = C.next;
      R.prev = cidx;
      if ( C.next != nil ) lnode(C.next).prev = ridx;
      C.next = ridx;
      internal_node &P = inode(parent);
      __tree_store::shift_right(P.keys(), ci, P.nkeys);
      __tree_store::shift_right(P.kids, static_cast<u16>(ci + 1), static_cast<u16>(P.nkeys + 1));
      new (micron::addr(P.keys()[ci])) K(lnode(ridx).keys()[0]);
      P.kids[ci + 1] = ridx;
      ++P.nkeys;
    } else {
      internal_node &C = inode(cidx);
      internal_node &R = inode(ridx);
      const u16 right_k = static_cast<u16>(MAXK - T);      // = T-1
      K median(micron::move(C.keys()[T - 1]));
      C.keys()[T - 1].~K();
      __tree_store::relocate_n(R.keys(), C.keys() + T, right_k);
      for ( u16 i = 0; i <= right_k; ++i ) R.kids[i] = C.kids[T + i];
      R.nkeys = right_k;
      C.nkeys = static_cast<u16>(T - 1);
      internal_node &P = inode(parent);
      __tree_store::shift_right(P.keys(), ci, P.nkeys);
      __tree_store::shift_right(P.kids, static_cast<u16>(ci + 1), static_cast<u16>(P.nkeys + 1));
      new (micron::addr(P.keys()[ci])) K(micron::move(median));
      P.kids[ci + 1] = ridx;
      ++P.nkeys;
    }
  }

  template<class VV>
  V *
  put(const K &key, VV &&val, bool overwrite, bool &inserted)
  {
    inserted = false;
    if ( __root == nil ) __root = alloc_leaf();
    if ( entries(__root) == MAXK ) {
      node_idx nr = alloc_internal();
      inode(nr).kids[0] = __root;
      __root = nr;
      split_child(__root, 0);
    }
    node_idx cur = __root;
    while ( !is_leaf(cur) ) {
      u16 ci = child_index(inode(cur), key);
      if ( entries(inode(cur).kids[ci]) == MAXK ) {
        split_child(cur, ci);
        ci = child_index(inode(cur), key);
      }
      cur = inode(cur).kids[ci];
    }
    leaf_node &L = lnode(cur);
    u16 pos = leaf_lb(L, key);
    if ( pos < L.count && eq(L.keys()[pos], key) ) {
      if ( overwrite ) {
        L.vals()[pos].~V();
        new (micron::addr(L.vals()[pos])) V(micron::forward<VV>(val));
      }
      return micron::addr(L.vals()[pos]);
    }
    __tree_store::shift_right(L.keys(), pos, L.count);
    __tree_store::shift_right(L.vals(), pos, L.count);
    new (micron::addr(L.keys()[pos])) K(key);
    new (micron::addr(L.vals()[pos])) V(micron::forward<VV>(val));
    ++L.count;
    ++__size;
    inserted = true;
    return micron::addr(L.vals()[pos]);
  }

  bool
  erase_descend(node_idx cur, const K &key)
  {
    while ( !is_leaf(cur) ) {
      u16 ci = child_index(inode(cur), key);
      if ( entries(inode(cur).kids[ci]) == MINK ) {
        fill_child(cur, ci);
        ci = child_index(inode(cur), key);
      }
      cur = inode(cur).kids[ci];
    }
    leaf_node &L = lnode(cur);
    u16 pos = leaf_lb(L, key);
    if ( pos < L.count && eq(L.keys()[pos], key) ) {
      __tree_store::erase_at(L.keys(), pos, L.count);
      __tree_store::erase_at(L.vals(), pos, L.count);
      --L.count;
      return true;
    }
    return false;
  }

  void
  fill_child(node_idx parent, u16 ci)
  {
    internal_node &P = inode(parent);
    if ( ci > 0 && entries(P.kids[ci - 1]) > MINK )
      borrow_prev(parent, ci);
    else if ( ci < P.nkeys && entries(P.kids[ci + 1]) > MINK )
      borrow_next(parent, ci);
    else if ( ci < P.nkeys )
      merge(parent, ci);
    else
      merge(parent, static_cast<u16>(ci - 1));
  }

  void
  borrow_prev(node_idx parent, u16 ci)
  {
    internal_node &P = inode(parent);
    node_idx lft = P.kids[ci - 1];
    node_idx cur = P.kids[ci];
    if ( is_leaf(cur) ) {
      leaf_node &Lf = lnode(lft);
      leaf_node &C = lnode(cur);
      __tree_store::shift_right(C.keys(), 0, C.count);
      __tree_store::shift_right(C.vals(), 0, C.count);
      new (micron::addr(C.keys()[0])) K(micron::move(Lf.keys()[Lf.count - 1]));
      new (micron::addr(C.vals()[0])) V(micron::move(Lf.vals()[Lf.count - 1]));
      Lf.keys()[Lf.count - 1].~K();
      Lf.vals()[Lf.count - 1].~V();
      --Lf.count;
      ++C.count;
      P.keys()[ci - 1].~K();
      new (micron::addr(P.keys()[ci - 1])) K(C.keys()[0]);
    } else {
      internal_node &Lf = inode(lft);
      internal_node &C = inode(cur);
      __tree_store::shift_right(C.keys(), 0, C.nkeys);
      __tree_store::shift_right(C.kids, 0, static_cast<u16>(C.nkeys + 1));
      new (micron::addr(C.keys()[0])) K(micron::move(P.keys()[ci - 1]));      // separator down
      C.kids[0] = Lf.kids[Lf.nkeys];
      ++C.nkeys;
      P.keys()[ci - 1].~K();
      new (micron::addr(P.keys()[ci - 1])) K(micron::move(Lf.keys()[Lf.nkeys - 1]));      // left's last key up
      Lf.keys()[Lf.nkeys - 1].~K();
      --Lf.nkeys;
    }
  }

  void
  borrow_next(node_idx parent, u16 ci)
  {
    internal_node &P = inode(parent);
    node_idx cur = P.kids[ci];
    node_idx rgt = P.kids[ci + 1];
    if ( is_leaf(cur) ) {
      leaf_node &C = lnode(cur);
      leaf_node &Rf = lnode(rgt);
      new (micron::addr(C.keys()[C.count])) K(micron::move(Rf.keys()[0]));
      new (micron::addr(C.vals()[C.count])) V(micron::move(Rf.vals()[0]));
      ++C.count;
      __tree_store::erase_at(Rf.keys(), 0, Rf.count);
      __tree_store::erase_at(Rf.vals(), 0, Rf.count);
      --Rf.count;
      P.keys()[ci].~K();
      new (micron::addr(P.keys()[ci])) K(Rf.keys()[0]);
    } else {
      internal_node &C = inode(cur);
      internal_node &Rf = inode(rgt);
      new (micron::addr(C.keys()[C.nkeys])) K(micron::move(P.keys()[ci]));      // separator down
      C.kids[C.nkeys + 1] = Rf.kids[0];
      ++C.nkeys;
      P.keys()[ci].~K();
      new (micron::addr(P.keys()[ci])) K(micron::move(Rf.keys()[0]));      // right's first key up
      __tree_store::erase_at(Rf.keys(), 0, Rf.nkeys);
      for ( u16 i = 0; i < Rf.nkeys; ++i ) Rf.kids[i] = Rf.kids[i + 1];
      --Rf.nkeys;
    }
  }

  void
  merge(node_idx parent, u16 m)
  {
    internal_node &P = inode(parent);
    node_idx lft = P.kids[m];
    node_idx rgt = P.kids[m + 1];
    if ( is_leaf(lft) ) {
      leaf_node &L = lnode(lft);
      leaf_node &R = lnode(rgt);
      __tree_store::relocate_n(L.keys() + L.count, R.keys(), R.count);
      __tree_store::relocate_n(L.vals() + L.count, R.vals(), R.count);
      L.count = static_cast<u16>(L.count + R.count);
      R.count = 0;
      L.next = R.next;
      if ( R.next != nil ) lnode(R.next).prev = lft;
    } else {
      internal_node &L = inode(lft);
      internal_node &R = inode(rgt);
      const u16 ln = L.nkeys;
      new (micron::addr(L.keys()[ln])) K(micron::move(P.keys()[m]));      // separator down
      __tree_store::relocate_n(L.keys() + ln + 1, R.keys(), R.nkeys);
      for ( u16 i = 0; i <= R.nkeys; ++i ) L.kids[ln + 1 + i] = R.kids[i];
      L.nkeys = static_cast<u16>(ln + 1 + R.nkeys);
      R.nkeys = 0;
    }
    destroy_node(rgt);
    // drop separator m and child pointer (m+1) from the parent
    __tree_store::erase_at(P.keys(), m, P.nkeys);
    for ( u16 i = static_cast<u16>(m + 1); i < P.nkeys; ++i ) P.kids[i] = P.kids[i + 1];
    --P.nkeys;
  }

  node_idx
  leftmost_leaf(node_idx i) noexcept
  {
    if ( i == nil ) return nil;
    while ( !is_leaf(i) ) i = inode(i).kids[0];
    return i;
  }

public:
  using category_type = tree_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using size_type = usize;
  using key_type = K;
  using mapped_type = V;

  ~b_tree()
  {
    if ( __root != nil ) free_subtree(__root);
  }

  b_tree() : __arena(), __root(nil), __size(0) { }

  template<class Fn>
    requires(micron::is_invocable_v<Fn, b_tree &>)
  explicit b_tree(Fn build) : b_tree()
  {
    build(*this);
  }

  b_tree(const b_tree &) = delete;
  b_tree &operator=(const b_tree &) = delete;

  b_tree(b_tree &&o) noexcept : __arena(micron::move(o.__arena)), __root(o.__root), __size(o.__size)
  {
    o.__root = nil;
    o.__size = 0;
  }

  b_tree &
  operator=(b_tree &&o) noexcept
  {
    if ( this != &o ) {
      if ( __root != nil ) free_subtree(__root);
      __arena = micron::move(o.__arena);
      __root = o.__root;
      __size = o.__size;
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

  void
  clear()
  {
    if ( __root != nil ) free_subtree(__root);
    __root = nil;
    __size = 0;
    __arena.reset();
  }

  V *
  find(const K &key) noexcept
  {
    node_idx cur = __root;
    while ( cur != nil ) {
      if ( is_leaf(cur) ) {
        leaf_node &L = lnode(cur);
        u16 pos = leaf_lb(L, key);
        if ( pos < L.count && eq(L.keys()[pos], key) ) return micron::addr(L.vals()[pos]);
        return nullptr;
      }
      cur = inode(cur).kids[child_index(inode(cur), key)];
    }
    return nullptr;
  }

  const V *
  find(const K &key) const noexcept
  {
    return const_cast<b_tree *>(this)->find(key);
  }

  bool
  contains(const K &key) const noexcept
  {
    return const_cast<b_tree *>(this)->find(key) != nullptr;
  }

  usize
  count(const K &key) const noexcept
  {
    return contains(key) ? 1u : 0u;
  }

  V &
  at(const K &key)
  {
    V *v = find(key);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("b_tree::at(): key not found");
    return *v;
  }

  const V &
  at(const K &key) const
  {
    const V *v = find(key);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("b_tree::at(): key not found");
    return *v;
  }

  bool
  insert(const K &key, V val)
  {
    bool ins;
    put(key, micron::move(val), false, ins);
    return ins;
  }

  bool
  insert_or_assign(const K &key, V val)
  {
    bool ins;
    put(key, micron::move(val), true, ins);
    return ins;
  }

  template<typename... Args>
  bool
  emplace(const K &key, Args &&...args)
  {
    bool ins;
    put(key, V(micron::forward<Args>(args)...), false, ins);
    return ins;
  }

  V &
  operator[](const K &key)
  {
    bool ins;
    return *put(key, V(), false, ins);
  }

  template<class Fn>
  V &
  update(const K &key, Fn fn)
  {
    V *cur = find(key);
    V nv = fn(static_cast<const V *>(cur));
    V &slot = operator[](key);
    slot = micron::move(nv);
    return slot;
  }

  template<class MakeV, class Modify>
  V &
  insert_or_modify(const K &key, MakeV make, Modify modify)
  {
    V *cur = find(key);
    if ( cur ) {
      modify(*cur);
      return *cur;
    }
    V &slot = operator[](key);
    slot = make();
    return slot;
  }

  bool
  erase(const K &key)
  {
    if ( __root == nil ) return false;
    bool removed = erase_descend(__root, key);
    if ( removed ) --__size;
    if ( __root != nil ) {
      if ( !is_leaf(__root) && inode(__root).nkeys == 0 ) {
        node_idx old = __root;
        __root = inode(__root).kids[0];
        destroy_node(old);
      } else if ( is_leaf(__root) && lnode(__root).count == 0 ) {
        destroy_node(__root);
        __root = nil;
      }
    }
    return removed;
  }

  void
  swap(b_tree &o) noexcept
  {
    micron::swap(__arena, o.__arena);
    micron::swap(__root, o.__root);
    micron::swap(__size, o.__size);
  }

  struct kv_ref {
    const K &key;
    V &value;
  };

  class iterator
  {
    b_tree *t;
    node_idx leaf;
    u16 pos;

  public:
    iterator(b_tree *tp, node_idx l, u16 p) noexcept : t(tp), leaf(l), pos(p) { }

    kv_ref
    operator*() const noexcept
    {
      leaf_node &L = t->lnode(leaf);
      return kv_ref{ L.keys()[pos], L.vals()[pos] };
    }

    iterator &
    operator++() noexcept
    {
      leaf_node &L = t->lnode(leaf);
      ++pos;
      if ( pos >= L.count ) {
        leaf = L.next;
        pos = 0;
      }
      return *this;
    }

    bool
    operator==(const iterator &o) const noexcept
    {
      return leaf == o.leaf && pos == o.pos;
    }

    bool
    operator!=(const iterator &o) const noexcept
    {
      return !(*this == o);
    }
  };

  iterator
  begin() noexcept
  {
    return iterator(this, leftmost_leaf(__root), 0);
  }

  iterator
  end() noexcept
  {
    return iterator(this, nil, 0);
  }

  iterator
  lower_bound(const K &key) noexcept
  {
    node_idx cur = __root;
    while ( cur != nil && !is_leaf(cur) ) cur = inode(cur).kids[child_index(inode(cur), key)];
    if ( cur == nil ) return end();
    leaf_node &L = lnode(cur);
    u16 pos = leaf_lb(L, key);
    if ( pos >= L.count ) {
      node_idx nx = L.next;
      return nx == nil ? end() : iterator(this, nx, 0);
    }
    return iterator(this, cur, pos);
  }

  iterator
  upper_bound(const K &key) noexcept
  {
    iterator it = lower_bound(key);
    if ( it != end() ) {
      kv_ref kv = *it;
      if ( eq(kv.key, key) ) ++it;
    }
    return it;
  }

  template<typename Fn>
  void
  for_each(Fn &&fn)
  {
    node_idx l = leftmost_leaf(__root);
    while ( l != nil ) {
      leaf_node &L = lnode(l);
      for ( u16 i = 0; i < L.count; ++i ) fn(static_cast<const K &>(L.keys()[i]), L.vals()[i]);
      l = L.next;
    }
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const
  {
    const_cast<b_tree *>(this)->for_each([&](const K &k, V &v) { fn(k, static_cast<const V &>(v)); });
  }

  template<class Fn>
  auto
  map(Fn fn) const
  {
    if constexpr ( micron::is_invocable_v<Fn, const K &, const V &> ) {
      using V2 = micron::remove_cvref_t<micron::invoke_result_t<Fn, const K &, const V &>>;
      b_tree<K, V2, Compare, DegreeOverride> out;
      for_each([&](const K &k, const V &v) { out.insert(k, fn(k, v)); });
      return out;
    } else {
      using V2 = micron::remove_cvref_t<micron::invoke_result_t<Fn, const V &>>;
      b_tree<K, V2, Compare, DegreeOverride> out;
      for_each([&](const K &k, const V &v) { out.insert(k, fn(v)); });
      return out;
    }
  }

  template<class LeafFn, class InnerFn>
  auto
  cata(LeafFn leaf, InnerFn inner) const
  {
    using Acc = micron::remove_cvref_t<micron::invoke_result_t<LeafFn, const K *, const V *, u16>>;
    if ( __root == nil ) return Acc{};
    return const_cast<b_tree *>(this)->template __cata_rec<Acc>(__root, leaf, inner);
  }

  template<class Acc, class LeafFn, class InnerFn>
  Acc
  __cata_rec(node_idx n, LeafFn &leaf, InnerFn &inner)
  {
    if ( is_leaf(n) ) {
      leaf_node &L = lnode(n);
      return leaf(L.keys(), L.vals(), L.count);
    }
    internal_node &I = inode(n);
    const u16 nk = I.nkeys;
    const u16 nkids = static_cast<u16>(nk + 1);
    Acc kids[MAXC];
    for ( u16 i = 0; i < nkids; ++i ) kids[i] = __cata_rec<Acc>(I.kids[i], leaf, inner);
    return inner(I.keys(), nk, kids, nkids);
  }

  template<class Fn>
  walk_ctl
  traverse(Fn fn) const
  {
    b_tree *self = const_cast<b_tree *>(this);
    node_idx l = self->leftmost_leaf(self->__root);
    while ( l != nil ) {
      leaf_node &L = self->lnode(l);
      for ( u16 i = 0; i < L.count; ++i )
        if ( micron::__impl::invoke_walk(fn, static_cast<const K &>(L.keys()[i]), static_cast<const V &>(L.vals()[i])) == walk_ctl::stop )
          return walk_ctl::stop;
      l = L.next;
    }
    return walk_ctl::continue_;
  }
};

};      // namespace micron
