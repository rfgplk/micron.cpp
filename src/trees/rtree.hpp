//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../except.hpp"
#include "../math/geometry/aligned_box.hpp"
#include "../math/ieee.hpp"
#include "../math/quants/vec.hpp"
#include "../memory/actions.hpp"
#include "../memory/addr.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "../vector.hpp"
#include "__tree_store.hpp"
#include "__tree_walk.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron::rtree
// R*-tree; spatial index over axis-aligned bounding boxes, minimizes overlap enlargement at leaf-parent level, point is the degenerate box
// (min=max)

namespace micron
{

template<typename Value, typename F = float, usize Dim = 2, usize MaxOverride = 0>
  requires micron::is_copy_constructible_v<Value> && (Dim >= 2 && Dim <= 16)
class rtree
{
public:
  using box_type = micron::math::geometry::aligned_box<F, Dim>;
  using point_type = micron::math::vec<F, Dim>;
  // value + spatial-key exposed for the FP layer
  using mapped_type = Value;
  using spatial_key_type = box_type;

private:
  using node_idx = __tree_store::node_idx;
  static constexpr node_idx nil = __tree_store::nil;

  static constexpr u16 M = static_cast<u16>(MaxOverride > 0 ? MaxOverride : 16);
  static_assert(MaxOverride == 0 || MaxOverride >= 4, "rtree max fanout must be >= 4");
  static constexpr u16 __minf = static_cast<u16>((2u * M) / 5u < 2u ? 2u : (2u * M) / 5u);        // min fill
  static constexpr u16 __prei = static_cast<u16>((3u * M) / 10u < 1u ? 1u : (3u * M) / 10u);      // reinsert count
  static constexpr u16 __cap = static_cast<u16>(M + 1);                                           // transient overflow

  struct internal_node {
    u8 leaf;      // 0
    u8 _pad;
    u16 count;
    box_type mbr[__cap];
    node_idx kids[__cap];

    internal_node(void) noexcept : leaf(0), _pad(0), count(0) { }
  };

  struct leaf_node {
    u8 leaf;      // 1
    u8 _pad;
    u16 count;
    box_type mbr[__cap];
    alignas(Value) byte vals_raw[sizeof(Value) * __cap];

    leaf_node(void) noexcept : leaf(1), _pad(0), count(0) { }

    ~leaf_node(void)
    {
      Value *v = vals();
      for ( u16 i = 0; i < count; ++i ) v[i].~Value();
    }

    Value *
    vals(void) noexcept
    {
      return static_cast<Value *>(static_cast<void *>(vals_raw));
    }

    const Value *
    vals(void) const noexcept
    {
      return static_cast<const Value *>(static_cast<const void *>(vals_raw));
    }
  };

  static constexpr usize __slot_bytes = (sizeof(internal_node) > sizeof(leaf_node)) ? sizeof(internal_node) : sizeof(leaf_node);

  struct pending {
    box_type b;
    Value v;
  };

  __tree_store::node_arena<__slot_bytes, 64> __arena;
  node_idx __root;
  usize __size;
  bool __did_reinsert;
  fvector<pending> __reins;

  template<typename Fn>
  void
  full_rec(node_idx n, Fn &fn) noexcept
  {
    if ( is_leaf(n) ) {
      leaf_node &L = lnode(n);
      for ( u16 i = 0; i < L.count; ++i ) fn(static_cast<const box_type &>(L.mbr[i]), L.vals()[i]);
      return;
    }
    internal_node &I = inode(n);
    for ( u16 i = 0; i < I.count; ++i ) full_rec(I.kids[i], fn);
  }

  struct knn_item {
    F d;
    box_type b;
    const Value *v;
  };

  void
  knn_rec(node_idx n, const point_type &p, usize k, fvector<knn_item> &best) noexcept
  {
    if ( is_leaf(n) ) {
      leaf_node &L = lnode(n);
      for ( u16 i = 0; i < L.count; ++i ) {
        F d = mindist2(p, L.mbr[i]);
        if ( best.size() < k || d < best[best.size() - 1].d ) knn_insert(best, k, knn_item{ d, L.mbr[i], micron::addr(L.vals()[i]) });
      }
      return;
    }
    internal_node &I = inode(n);
    u16 order[__cap];
    F od[__cap];
    for ( u16 i = 0; i < I.count; ++i ) {
      order[i] = i;
      od[i] = mindist2(p, I.mbr[i]);
    }
    for ( u16 i = 1; i < I.count; ++i ) {
      u16 c = order[i];
      F key = od[c];
      int j = static_cast<int>(i) - 1;
      while ( j >= 0 && od[order[j]] > key ) {
        order[j + 1] = order[j];
        --j;
      }
      order[j + 1] = c;
    }
    for ( u16 t = 0; t < I.count; ++t ) {
      F md = od[order[t]];
      if ( best.size() >= k && md >= best[best.size() - 1].d ) break;      // prune remaining (sorted)
      knn_rec(I.kids[order[t]], p, k, best);
    }
  }

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
    return is_leaf(i) ? lnode(i).count : inode(i).count;
  }

  [[gnu::always_inline]] box_type *
  mbr_of(node_idx i) noexcept
  {
    return is_leaf(i) ? lnode(i).mbr : inode(i).mbr;
  }

  static F
  area(const box_type &b) noexcept
  {
    F a = F(1);
    for ( usize d = 0; d < Dim; ++d ) {
      F e = b.max_corner.data[d] - b.min_corner.data[d];
      if ( e < F(0) ) return F(0);
      a *= e;
    }
    return a;
  }

  static F
  margin(const box_type &b) noexcept
  {
    F s = F(0);
    for ( usize d = 0; d < Dim; ++d ) s += (b.max_corner.data[d] - b.min_corner.data[d]);
    return s;
  }

  static F
  overlap(const box_type &a, const box_type &b) noexcept
  {
    F o = F(1);
    for ( usize d = 0; d < Dim; ++d ) {
      F lo = a.min_corner.data[d] > b.min_corner.data[d] ? a.min_corner.data[d] : b.min_corner.data[d];
      F hi = a.max_corner.data[d] < b.max_corner.data[d] ? a.max_corner.data[d] : b.max_corner.data[d];
      F e = hi - lo;
      if ( e <= F(0) ) return F(0);
      o *= e;
    }
    return o;
  }

  static box_type
  combine(const box_type &a, const box_type &b) noexcept
  {
    box_type r = a;
    r.extend(b);
    return r;
  }

  static F
  enlargement(const box_type &e, const box_type &b) noexcept
  {
    return area(combine(e, b)) - area(e);
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

  box_type
  subtree_mbr(node_idx i) noexcept
  {
    box_type r = box_type::empty();
    u16 n = entries(i);
    box_type *mb = mbr_of(i);
    for ( u16 j = 0; j < n; ++j ) r.extend(mb[j]);
    return r;
  }

  node_idx
  alloc_leaf(void)
  {
    node_idx i = __arena.allocate();
    new (__arena.raw(i)) leaf_node();
    return i;
  }

  node_idx
  alloc_internal(void)
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
      u16 n = I.count;
      node_idx kids[__cap];
      for ( u16 c = 0; c < n; ++c ) kids[c] = I.kids[c];
      for ( u16 c = 0; c < n; ++c ) free_subtree(kids[c]);
    }
    destroy_node(i);
  }

  u16
  choose_subtree(internal_node &I, const box_type &b) noexcept
  {
    const bool kids_are_leaves = is_leaf(I.kids[0]);
    u16 best = 0;
    if ( kids_are_leaves ) {
      F best_ov = F(0), best_en = F(0), best_ar = F(0);
      for ( u16 i = 0; i < I.count; ++i ) {
        box_type comb = combine(I.mbr[i], b);
        F ov = F(0);
        for ( u16 j = 0; j < I.count; ++j )
          if ( j != i ) ov += overlap(comb, I.mbr[j]) - overlap(I.mbr[i], I.mbr[j]);
        F en = area(comb) - area(I.mbr[i]);
        F ar = area(I.mbr[i]);
        if ( i == 0 || ov < best_ov || (ov == best_ov && en < best_en) || (ov == best_ov && en == best_en && ar < best_ar) ) {
          best = i;
          best_ov = ov;
          best_en = en;
          best_ar = ar;
        }
      }
    } else {
      F best_en = F(0), best_ar = F(0);
      for ( u16 i = 0; i < I.count; ++i ) {
        F en = enlargement(I.mbr[i], b);
        F ar = area(I.mbr[i]);
        if ( i == 0 || en < best_en || (en == best_en && ar < best_ar) ) {
          best = i;
          best_en = en;
          best_ar = ar;
        }
      }
    }
    return best;
  }

  static void
  sort_idx(u16 *idx, const box_type *mbr, u16 n, usize axis, bool by_max) noexcept
  {
    for ( u16 i = 1; i < n; ++i ) {
      u16 cur = idx[i];
      F key = by_max ? mbr[cur].max_corner.data[axis] : mbr[cur].min_corner.data[axis];
      int j = static_cast<int>(i) - 1;
      while ( j >= 0 ) {
        F kj = by_max ? mbr[idx[j]].max_corner.data[axis] : mbr[idx[j]].min_corner.data[axis];
        if ( kj <= key ) break;
        idx[j + 1] = idx[j];
        --j;
      }
      idx[j + 1] = cur;
    }
  }

  void
  choose_split(const box_type *mbr, u16 *out_perm, u16 &out_k) noexcept
  {
    const u16 n = __cap;
    u16 idx[__cap];
    usize best_axis = 0;
    F best_axis_margin = F(0);
    bool first_axis = true;
    for ( usize axis = 0; axis < Dim; ++axis ) {
      F margin_sum = F(0);
      for ( int mode = 0; mode < 2; ++mode ) {
        for ( u16 i = 0; i < n; ++i ) idx[i] = i;
        sort_idx(idx, mbr, n, axis, mode == 1);
        for ( u16 k = __minf; k <= n - __minf; ++k ) {
          box_type b1 = box_type::empty();
          box_type b2 = box_type::empty();
          for ( u16 t = 0; t < k; ++t ) b1.extend(mbr[idx[t]]);
          for ( u16 t = k; t < n; ++t ) b2.extend(mbr[idx[t]]);
          margin_sum += margin(b1) + margin(b2);
        }
      }
      if ( first_axis || margin_sum < best_axis_margin ) {
        best_axis = axis;
        best_axis_margin = margin_sum;
        first_axis = false;
      }
    }
    F best_ov = F(0), best_ar = F(0);
    bool first = true;
    for ( int mode = 0; mode < 2; ++mode ) {
      for ( u16 i = 0; i < n; ++i ) idx[i] = i;
      sort_idx(idx, mbr, n, best_axis, mode == 1);
      for ( u16 k = __minf; k <= n - __minf; ++k ) {
        box_type b1 = box_type::empty();
        box_type b2 = box_type::empty();
        for ( u16 t = 0; t < k; ++t ) b1.extend(mbr[idx[t]]);
        for ( u16 t = k; t < n; ++t ) b2.extend(mbr[idx[t]]);
        F ov = overlap(b1, b2);
        F ar = area(b1) + area(b2);
        if ( first || ov < best_ov || (ov == best_ov && ar < best_ar) ) {
          best_ov = ov;
          best_ar = ar;
          out_k = k;
          for ( u16 t = 0; t < n; ++t ) out_perm[t] = idx[t];
          first = false;
        }
      }
    }
  }

  void
  split_leaf(node_idx n, node_idx &out_right, box_type &out_rmbr)
  {
    u16 perm[__cap];
    u16 k = __minf;
    choose_split(lnode(n).mbr, perm, k);

    node_idx ridx = alloc_leaf();      // may move slab; re-fetch below
    leaf_node &L = lnode(n);
    leaf_node &R = lnode(ridx);

    box_type sbox[__cap];
    alignas(Value) byte sraw[sizeof(Value) * __cap];
    Value *sv = static_cast<Value *>(static_cast<void *>(sraw));
    for ( u16 t = 0; t < __cap; ++t ) {
      sbox[t] = L.mbr[perm[t]];
      new (micron::addr(sv[t])) Value(micron::move(L.vals()[perm[t]]));
    }
    for ( u16 i = 0; i < __cap; ++i ) L.vals()[i].~Value();

    for ( u16 t = 0; t < k; ++t ) {
      L.mbr[t] = sbox[t];
      new (micron::addr(L.vals()[t])) Value(micron::move(sv[t]));
    }
    L.count = k;
    for ( u16 t = k; t < __cap; ++t ) {
      R.mbr[t - k] = sbox[t];
      new (micron::addr(R.vals()[t - k])) Value(micron::move(sv[t]));
    }
    R.count = static_cast<u16>(__cap - k);
    for ( u16 t = 0; t < __cap; ++t ) sv[t].~Value();

    out_right = ridx;
    out_rmbr = subtree_mbr(ridx);
  }

  void
  split_internal(node_idx n, node_idx &out_right, box_type &out_rmbr)
  {
    u16 perm[__cap];
    u16 k = __minf;
    choose_split(inode(n).mbr, perm, k);

    node_idx ridx = alloc_internal();      // may move slab; re-fetch
    internal_node &I = inode(n);
    internal_node &R = inode(ridx);

    box_type sbox[__cap];
    node_idx skid[__cap];
    for ( u16 t = 0; t < __cap; ++t ) {
      sbox[t] = I.mbr[perm[t]];
      skid[t] = I.kids[perm[t]];
    }
    for ( u16 t = 0; t < k; ++t ) {
      I.mbr[t] = sbox[t];
      I.kids[t] = skid[t];
    }
    I.count = k;
    for ( u16 t = k; t < __cap; ++t ) {
      R.mbr[t - k] = sbox[t];
      R.kids[t - k] = skid[t];
    }
    R.count = static_cast<u16>(__cap - k);

    out_right = ridx;
    out_rmbr = subtree_mbr(ridx);
  }

  void
  reinsert_leaf(node_idx n)
  {
    leaf_node &L = lnode(n);
    box_type bb = subtree_mbr(n);
    point_type c = bb.center();

    u16 idx[__cap];
    F dist[__cap];
    for ( u16 i = 0; i < __cap; ++i ) {
      idx[i] = i;
      point_type ec = L.mbr[i].center();
      F dd = F(0);
      for ( usize d = 0; d < Dim; ++d ) {
        F df = ec.data[d] - c.data[d];
        dd += df * df;
      }
      dist[i] = dd;
    }
    // sort idx by dist ascending (insertion sort)
    for ( u16 i = 1; i < __cap; ++i ) {
      u16 cur = idx[i];
      F key = dist[cur];
      int j = static_cast<int>(i) - 1;
      while ( j >= 0 && dist[idx[j]] > key ) {
        idx[j + 1] = idx[j];
        --j;
      }
      idx[j + 1] = cur;
    }
    const u16 keep = static_cast<u16>(__cap - __prei);

    box_type sbox[__cap];
    alignas(Value) byte sraw[sizeof(Value) * __cap];
    Value *sv = static_cast<Value *>(static_cast<void *>(sraw));
    for ( u16 t = 0; t < __cap; ++t ) {
      sbox[t] = L.mbr[idx[t]];
      new (micron::addr(sv[t])) Value(micron::move(L.vals()[idx[t]]));
    }
    for ( u16 i = 0; i < __cap; ++i ) L.vals()[i].~Value();

    for ( u16 t = 0; t < keep; ++t ) {
      L.mbr[t] = sbox[t];
      new (micron::addr(L.vals()[t])) Value(micron::move(sv[t]));
    }
    L.count = keep;
    for ( u16 t = keep; t < __cap; ++t ) __reins.push_back(pending{ sbox[t], sv[t] });
    for ( u16 t = 0; t < __cap; ++t ) sv[t].~Value();
  }

  bool
  insert_descend(node_idx n, const box_type &b, const Value &v, node_idx &out_right, box_type &out_rmbr)
  {
    if ( is_leaf(n) ) {
      leaf_node &L = lnode(n);
      L.mbr[L.count] = b;
      new (micron::addr(L.vals()[L.count])) Value(v);
      ++L.count;
      if ( L.count <= M ) return false;
      if ( !__did_reinsert && n != __root ) {
        __did_reinsert = true;
        reinsert_leaf(n);
        return false;
      }
      split_leaf(n, out_right, out_rmbr);
      return true;
    }
    u16 ci = choose_subtree(inode(n), b);
    node_idx child = inode(n).kids[ci];
    node_idx cr;
    box_type crm;
    bool csplit = insert_descend(child, b, v, cr, crm);
    inode(n).mbr[ci].extend(b);
    if ( !csplit ) {
      // child may have shrunk via reinsert; its mbr stays a valid superset (correct).
      return false;
    }
    internal_node &I = inode(n);
    I.mbr[ci] = subtree_mbr(I.kids[ci]);
    I.mbr[I.count] = crm;
    I.kids[I.count] = cr;
    ++I.count;
    if ( I.count <= M ) return false;
    split_internal(n, out_right, out_rmbr);
    return true;
  }

  void
  insert_from_root(const box_type &b, const Value &v)
  {
    node_idx rr;
    box_type rm;
    bool s = insert_descend(__root, b, v, rr, rm);
    if ( s ) {
      node_idx nr = alloc_internal();      // may move slab; re-fetch
      internal_node &NR = inode(nr);
      NR.count = 2;
      NR.kids[0] = __root;
      NR.kids[1] = rr;
      NR.mbr[1] = rm;
      node_idx oldroot = __root;
      __root = nr;
      inode(nr).mbr[0] = subtree_mbr(oldroot);
    }
  }

  template<typename Fn>
  void
  query_rec(node_idx n, const box_type &q, Fn &fn) noexcept
  {
    if ( is_leaf(n) ) {
      leaf_node &L = lnode(n);
      for ( u16 i = 0; i < L.count; ++i )
        if ( L.mbr[i].intersects(q) ) fn(static_cast<const box_type &>(L.mbr[i]), L.vals()[i]);
      return;
    }
    internal_node &I = inode(n);
    for ( u16 i = 0; i < I.count; ++i )
      if ( I.mbr[i].intersects(q) ) query_rec(I.kids[i], q, fn);
  }

  void
  collect_data(node_idx n)
  {
    if ( is_leaf(n) ) {
      leaf_node &L = lnode(n);
      for ( u16 i = 0; i < L.count; ++i ) __reins.push_back(pending{ L.mbr[i], L.vals()[i] });
      return;
    }
    internal_node &I = inode(n);
    u16 c = I.count;
    node_idx kids[__cap];
    for ( u16 i = 0; i < c; ++i ) kids[i] = I.kids[i];
    for ( u16 i = 0; i < c; ++i ) collect_data(kids[i]);
  }

  static void
  remove_internal_entry(internal_node &I, u16 i) noexcept
  {
    for ( u16 j = i; j + 1 < I.count; ++j ) {
      I.mbr[j] = I.mbr[j + 1];
      I.kids[j] = I.kids[j + 1];
    }
    --I.count;
  }

  void
  remove_leaf_entry(leaf_node &L, u16 i) noexcept
  {
    L.vals()[i].~Value();
    for ( u16 j = i; j + 1 < L.count; ++j ) {
      L.mbr[j] = L.mbr[j + 1];
      new (micron::addr(L.vals()[j])) Value(micron::move(L.vals()[j + 1]));
      L.vals()[j + 1].~Value();
    }
    --L.count;
  }

  bool
  erase_rec(node_idx n, const box_type &b, const Value &v)
  {
    if ( is_leaf(n) ) {
      leaf_node &L = lnode(n);
      for ( u16 i = 0; i < L.count; ++i ) {
        if ( L.mbr[i].intersects(b) && L.vals()[i] == v ) {
          remove_leaf_entry(L, i);
          return true;
        }
      }
      return false;
    }
    internal_node &I = inode(n);
    for ( u16 ci = 0; ci < I.count; ++ci ) {
      if ( !I.mbr[ci].intersects(b) ) continue;
      if ( !erase_rec(I.kids[ci], b, v) ) continue;
      node_idx child = inode(n).kids[ci];
      if ( entries(child) < __minf ) {
        collect_data(child);      // gather orphaned data entries for reinsertion
        free_subtree(child);
        remove_internal_entry(inode(n), ci);
      } else {
        inode(n).mbr[ci] = subtree_mbr(child);
      }
      return true;
    }
    return false;
  }

public:
  using category_type = tree_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using size_type = usize;

  ~rtree(void)
  {
    if ( __root != nil ) free_subtree(__root);
  }

  rtree() : __arena(), __root(nil), __size(0), __did_reinsert(false), __reins() { }

  template<class Fn>
    requires(micron::is_invocable_v<Fn, rtree &>)
  explicit rtree(Fn build) : rtree()
  {
    build(*this);
  }

  rtree(const rtree &) = delete;
  rtree &operator=(const rtree &) = delete;

  rtree(rtree &&o) noexcept
      : __arena(micron::move(o.__arena)), __root(o.__root), __size(o.__size), __did_reinsert(false), __reins(micron::move(o.__reins))
  {
    o.__root = nil;
    o.__size = 0;
  }

  rtree &
  operator=(rtree &&o) noexcept
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
  size(void) const noexcept
  {
    return __size;
  }

  bool
  empty(void) const noexcept
  {
    return __size == 0;
  }

  void
  clear(void)
  {
    if ( __root != nil ) free_subtree(__root);
    __root = nil;
    __size = 0;
    __arena.reset();
    __reins.clear();
  }

  void
  insert(const box_type &b, const Value &v)
  {
    if ( __root == nil ) __root = alloc_leaf();
    __did_reinsert = false;
    __reins.clear();
    insert_from_root(b, v);
    ++__size;
    for ( usize i = 0; i < __reins.size(); ++i ) insert_from_root(__reins[i].b, __reins[i].v);
    __reins.clear();
  }

  void
  insert(const point_type &p, const Value &v)
  {
    box_type b;
    b.min_corner = p;
    b.max_corner = p;
    insert(b, v);
  }

  bool
  erase(const box_type &b, const Value &v)
  {
    if ( __root == nil ) return false;
    __reins.clear();
    bool removed = erase_rec(__root, b, v);
    if ( !removed ) return false;
    --__size;
    if ( __root != nil ) {
      if ( !is_leaf(__root) && inode(__root).count == 1 ) {
        node_idx old = __root;
        __root = inode(__root).kids[0];
        inode(old).count = 0;      // detach so destroy_node doesn't recurse
        destroy_node(old);
      } else if ( is_leaf(__root) && lnode(__root).count == 0 ) {
        destroy_node(__root);
        __root = nil;
      }
    }
    if ( !__reins.empty() ) {
      __did_reinsert = true;      // orphans must not re-trigger forced reinsert cascades
      fvector<pending> orphans(micron::move(__reins));
      __reins.clear();
      for ( usize i = 0; i < orphans.size(); ++i ) {
        if ( __root == nil ) __root = alloc_leaf();
        insert_from_root(orphans[i].b, orphans[i].v);
      }
    }
    return removed;
  }

  bool
  erase(const point_type &p, const Value &v)
  {
    box_type b;
    b.min_corner = p;
    b.max_corner = p;
    return erase(b, v);
  }

  template<typename Fn>
  void
  query(const box_type &q, Fn &&fn) noexcept
  {
    if ( __root != nil ) query_rec(__root, q, fn);
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
    const_cast<rtree *>(this)->for_each([&](const box_type &b, Value &v) { fn(b, static_cast<const Value &>(v)); });
  }

  template<class Fn>
  auto
  map(Fn fn) const
  {
    if constexpr ( micron::is_invocable_v<Fn, const box_type &, const Value &> ) {
      using V2 = micron::remove_cvref_t<micron::invoke_result_t<Fn, const box_type &, const Value &>>;
      rtree<V2, F, Dim, MaxOverride> out;
      for_each([&](const box_type &b, const Value &v) { out.insert(b, fn(b, v)); });
      return out;
    } else {
      using V2 = micron::remove_cvref_t<micron::invoke_result_t<Fn, const Value &>>;
      rtree<V2, F, Dim, MaxOverride> out;
      for_each([&](const box_type &b, const Value &v) { out.insert(b, fn(v)); });
      return out;
    }
  }

  template<class Acc, class LeafElemFn, class InnerFn>
  Acc
  cata(const Acc &empty, LeafElemFn leaf_elem, InnerFn inner) const
  {
    if ( __root == nil ) return empty;
    return const_cast<rtree *>(this)->template __cata_rec<Acc>(__root, empty, leaf_elem, inner);
  }

  template<class Acc, class LeafElemFn, class InnerFn>
  Acc
  __cata_rec(node_idx n, const Acc &empty, LeafElemFn &leaf_elem, InnerFn &inner)
  {
    if ( is_leaf(n) ) {
      leaf_node &L = lnode(n);
      Acc acc = empty;
      for ( u16 i = 0; i < L.count; ++i )
        acc = leaf_elem(micron::move(acc), static_cast<const box_type &>(L.mbr[i]), static_cast<const Value &>(L.vals()[i]));
      return acc;
    }
    internal_node &I = inode(n);
    Acc kids[__cap];
    for ( u16 i = 0; i < I.count; ++i ) kids[i] = __cata_rec<Acc>(I.kids[i], empty, leaf_elem, inner);
    return inner(kids, I.count);
  }

  template<class Fn>
  walk_ctl
  traverse(Fn fn) const
  {
    rtree *self = const_cast<rtree *>(this);
    if ( self->__root == nil ) return walk_ctl::continue_;
    return self->__traverse_rec(self->__root, fn);
  }

  template<class Fn>
  walk_ctl
  __traverse_rec(node_idx n, Fn &fn)
  {
    if ( is_leaf(n) ) {
      leaf_node &L = lnode(n);
      for ( u16 i = 0; i < L.count; ++i )
        if ( micron::__impl::invoke_walk(fn, static_cast<const box_type &>(L.mbr[i]), static_cast<const Value &>(L.vals()[i]))
             == walk_ctl::stop )
          return walk_ctl::stop;
      return walk_ctl::continue_;
    }
    internal_node &I = inode(n);
    for ( u16 i = 0; i < I.count; ++i )
      if ( __traverse_rec(I.kids[i], fn) == walk_ctl::stop ) return walk_ctl::stop;
    return walk_ctl::continue_;
  }

  template<typename Fn>
  void
  nearest(const point_type &p, usize k, Fn &&fn) noexcept
  {
    if ( __root == nil || k == 0 ) return;
    fvector<knn_item> best;
    best.reserve(k + 1);
    knn_rec(__root, p, k, best);
    for ( usize i = 0; i < best.size(); ++i ) fn(static_cast<const box_type &>(best[i].b), *best[i].v, best[i].d);
  }
};

};      // namespace micron
