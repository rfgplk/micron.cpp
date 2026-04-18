//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../memory/actions.hpp"
#include "../memory/addr.hpp"
#include "../memory/allocation/resources.hpp"
#include "../memory/memory.hpp"

#include "../except.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "bits.hpp"

namespace micron
{

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//  immutable_map an immutable ordered associative container implemented as a left-leaning rb tree with refcounted nodes

template <typename K, typename V>
  requires micron::totally_ordered<K> && micron::is_movable_object<V>
class immutable_map
{
  //  Layout on 64-bit:
  //    [0]  __packed_left
  //    [8]  right
  //    [16] refs
  //    [20] (pad to alignof(K))
  //    [..] key
  //    [..] value

  struct __node {
    uintptr_t __packed_left;     // left pointer/red bit
    __node *right;
    mutable u32 refs;
    K key;
    V value;

    inline __attribute__((always_inline)) __node *
    left() const noexcept
    {
      return reinterpret_cast<__node *>(__packed_left & ~uintptr_t(1));
    }

    inline __attribute__((always_inline)) bool
    red() const noexcept
    {
      return __packed_left & 1;
    }

    inline __attribute__((always_inline)) void
    set_left(__node *l) noexcept
    {
      __packed_left = reinterpret_cast<uintptr_t>(l) | (__packed_left & 1);
    }

    inline __attribute__((always_inline)) void
    set_red(bool r) noexcept
    {
      __packed_left = (__packed_left & ~uintptr_t(1)) | uintptr_t(r);
    }

    inline __attribute__((always_inline)) void
    set_left_and_red(__node *l, bool r) noexcept
    {
      __packed_left = reinterpret_cast<uintptr_t>(l) | uintptr_t(r);
    }
  };

  template <typename Kf, typename Vf>
  static inline __node *
  __make_node(Kf &&k, Vf &&v, __node *l, __node *r, bool rd)
  {
    auto *n = reinterpret_cast<__node *>(abc::alloc(sizeof(__node)));
    if constexpr ( micron::is_trivially_copyable_v<K> )
      n->key = static_cast<Kf &&>(k);
    else
      new (micron::addr(n->key)) K(static_cast<Kf &&>(k));

    if constexpr ( micron::is_trivially_copyable_v<V> )
      n->value = static_cast<Vf &&>(v);
    else
      new (micron::addr(n->value)) V(static_cast<Vf &&>(v));

    n->set_left_and_red(l, rd);
    n->right = r;
    n->refs = 1;
    return n;
  }

  // construct V in-place from args
  template <typename Kf, typename... Args>
  static inline __node *
  __make_node_emplace(Kf &&k, __node *l, __node *r, bool rd, Args &&...args)
  {
    auto *n = reinterpret_cast<__node *>(abc::alloc(sizeof(__node)));
    if constexpr ( micron::is_trivially_copyable_v<K> )
      n->key = static_cast<Kf &&>(k);
    else
      new (micron::addr(n->key)) K(static_cast<Kf &&>(k));

    new (micron::addr(n->value)) V(micron::forward<Args>(args)...);
    n->set_left_and_red(l, rd);
    n->right = r;
    n->refs = 1;
    return n;
  }

  static inline __node *
  __clone_with(const __node *h, __node *left, __node *right, bool rd)
  {
    return __make_node(h->key, h->value, left, right, rd);
  }

  static inline void
  __dealloc_node(__node *n)
  {
    if constexpr ( !micron::is_trivially_destructible_v<K> ) n->key.~K();
    if constexpr ( !micron::is_trivially_destructible_v<V> ) n->value.~V();
    abc::dealloc(reinterpret_cast<byte *>(n));
  }

  static inline __attribute__((always_inline)) __node *
  __retain(__node *n)
  {
    if ( n ) [[likely]]
      ++n->refs;
    return n;
  }

  // tail-iterates right spine/stacks left children
  static inline void
  __release(__node *n)
  {
    constexpr usize __stack_cap = 64;
    __node *stack[__stack_cap];
    usize depth = 0;

    while ( n ) [[likely]] {
      if ( --n->refs != 0 ) [[likely]]
        break;
      __node *l = n->left();
      __node *r = n->right;
      __dealloc_node(n);
      if ( l ) {
        if ( depth < __stack_cap ) [[likely]]
          stack[depth++] = l;
        else
          __release(l);     // overflow
      }
      n = r;
    }
    while ( depth > 0 ) __release(stack[--depth]);
  }

  static inline __node *
  __copy_node(const __node *h)
  {
    return __make_node(h->key, h->value, __retain(h->left()), __retain(h->right), h->red());
  }

  // unique (refs==1): bump to 2
  // shared (refs>1): full copy
  //
  // WARNING: must call release manually
  static inline __node *
  __acquire_mutable(const __node *h)
  {
    if ( h->refs == 1 ) [[likely]] {
      __node *n = const_cast<__node *>(h);
      ++n->refs;
      return n;
    }
    return __copy_node(h);
  }

  static inline __attribute__((always_inline)) bool
  __is_red(const __node *n)
  {
    return n && n->red();
  }

  //
  //       h              x
  //      v v            v v
  //     a   x    to    h   c
  //        v v        v v
  //       b   c      a   b
  //
  //  h MUST be exclusive (refs==1)

  static inline __node *
  __rotate_left(__node *h)
  {
    __node *x = h->right;

    if ( x->refs == 1 ) [[likely]] {
      // both exclusive relink
      bool h_color = h->red();
      h->right = x->left();
      h->set_red(true);
      x->set_left_and_red(h, h_color);
      return x;
    }

    // x shared
    K xk = x->key;
    V xv = x->value;
    bool h_color = h->red();

    __node *xl = __retain(x->left());
    __node *xr = __retain(x->right);
    __release(x);

    h->right = xl;
    h->set_red(true);

    return __make_node(micron::move(xk), micron::move(xv), h, xr, h_color);
  }

  static inline __node *
  __rotate_right(__node *h)
  {
    __node *x = h->left();

    if ( x->refs == 1 ) [[likely]] {
      bool h_color = h->red();
      h->set_left(x->right);
      h->set_red(true);
      x->right = h;
      x->set_red(h_color);
      return x;
    }

    K xk = x->key;
    V xv = x->value;
    bool h_color = h->red();

    __node *xl = __retain(x->left());
    __node *xr = __retain(x->right);
    __release(x);

    h->set_left(xr);
    h->set_red(true);

    return __make_node(micron::move(xk), micron::move(xv), xl, h, h_color);
  }

  static inline __node *
  __flip_colors(__node *h)
  {
    h->set_red(!h->red());

    if ( __node *l = h->left() ) [[likely]] {
      if ( l->refs == 1 ) [[likely]] {
        l->set_red(!l->red());
      } else {
        __node *nl = __make_node(l->key, l->value, __retain(l->left()), __retain(l->right), !l->red());
        __release(l);
        h->set_left(nl);
      }
    }

    if ( __node *r = h->right ) [[likely]] {
      if ( r->refs == 1 ) [[likely]] {
        r->set_red(!r->red());
      } else {
        __node *nr = __make_node(r->key, r->value, __retain(r->left()), __retain(r->right), !r->red());
        __release(r);
        h->right = nr;
      }
    }

    return h;
  }

  static inline __node *
  __fixup(__node *h)
  {
    if ( __is_red(h->right) && !__is_red(h->left()) ) [[unlikely]]
      h = __rotate_left(h);
    if ( __is_red(h->left()) && h->left() && __is_red(h->left()->left()) ) [[unlikely]]
      h = __rotate_right(h);
    if ( __is_red(h->left()) && __is_red(h->right) ) [[unlikely]]
      h = __flip_colors(h);
    return h;
  }

  static inline __node *
  __move_red_left(__node *h)
  {
    h = __flip_colors(h);
    if ( h->right && __is_red(h->right->left()) ) [[unlikely]] {
      h->right = __rotate_right(h->right);
      h = __rotate_left(h);
      h = __flip_colors(h);
    }
    return h;
  }

  static inline __node *
  __move_red_right(__node *h)
  {
    h = __flip_colors(h);
    if ( h->left() && __is_red(h->left()->left()) ) [[unlikely]] {
      h = __rotate_right(h);
      h = __flip_colors(h);
    }
    return h;
  }

  template <typename Vf>
  static __node *
  __insert_impl(const __node *h, const K &k, Vf &&v, bool &inserted)
  {
    if ( !h ) [[unlikely]] {
      inserted = true;
      return __make_node(k, micron::forward<Vf>(v), nullptr, nullptr, true);
    }

    // cache first comparison
    const bool less = k < h->key;

    __node *n;
    if ( less ) [[likely]] {
      n = __clone_with(h, __insert_impl(h->left(), k, v, inserted), __retain(h->right), h->red());
    } else if ( h->key < k ) {
      n = __clone_with(h, __retain(h->left()), __insert_impl(h->right, k, v, inserted), h->red());
    } else {
      // update value
      n = __make_node(h->key, micron::forward<Vf>(v), __retain(h->left()), __retain(h->right), h->red());
      inserted = false;
    }

    return __fixup(n);
  }

  template <typename... Args>
  static __node *
  __emplace_impl(const __node *h, const K &k, bool &inserted, Args &&...args)
  {
    if ( !h ) [[unlikely]] {
      inserted = true;
      return __make_node_emplace(k, nullptr, nullptr, true, micron::forward<Args>(args)...);
    }

    const bool less = k < h->key;

    __node *n;
    if ( less ) [[likely]] {
      n = __clone_with(h, __emplace_impl(h->left(), k, inserted, micron::forward<Args>(args)...), __retain(h->right), h->red());
    } else if ( h->key < k ) {
      n = __clone_with(h, __retain(h->left()), __emplace_impl(h->right, k, inserted, micron::forward<Args>(args)...), h->red());
    } else {
      // reconstruct value in place
      n = __make_node_emplace(h->key, __retain(h->left()), __retain(h->right), h->red(), micron::forward<Args>(args)...);
      inserted = false;
    }

    return __fixup(n);
  }

  static const __node *
  __find_min(const __node *h)
  {
    while ( h && h->left() ) h = h->left();
    return h;
  }

  static __node *
  __erase_min_impl(const __node *h)
  {
    if ( !h ) [[unlikely]]
      return nullptr;

    if ( !h->left() ) return nullptr;

    __node *n = __acquire_mutable(h);

    if ( !__is_red(n->left()) && n->left() && !__is_red(n->left()->left()) ) [[unlikely]]
      n = __move_red_left(n);

    __node *old_left = n->left();
    n->set_left(__erase_min_impl(old_left));
    __release(old_left);

    return __fixup(n);
  }

  static __node *
  __erase_impl(const __node *h, const K &k, bool &erased)
  {
    if ( !h ) [[unlikely]] {
      erased = false;
      return nullptr;
    }

    __node *n = __acquire_mutable(h);
    const bool less = k < n->key;

    if ( less ) {
      if ( !n->left() ) [[unlikely]] {
        erased = false;
        return n;
      }

      if ( !__is_red(n->left()) && n->left() && !__is_red(n->left()->left()) ) [[unlikely]]
        n = __move_red_left(n);

      __node *old_left = n->left();
      n->set_left(__erase_impl(old_left, k, erased));
      __release(old_left);
    } else {
      if ( __is_red(n->left()) ) [[unlikely]]
        n = __rotate_right(n);

      const bool equal = !less && !(n->key < k);

      if ( equal && !n->right ) [[unlikely]] {
        erased = true;
        __release(n);
        return nullptr;
      }

      if ( !n->right ) [[unlikely]] {
        erased = false;
        return __fixup(n);
      }

      if ( !__is_red(n->right) && n->right && !__is_red(n->right->left()) ) [[unlikely]]
        n = __move_red_right(n);

      if ( equal ) [[unlikely]] {
        const __node *m = __find_min(n->right);

        if constexpr ( !micron::is_trivially_destructible_v<K> ) n->key.~K();
        if constexpr ( !micron::is_trivially_destructible_v<V> ) n->value.~V();

        if constexpr ( micron::is_trivially_copyable_v<K> )
          micron::bytecpy(reinterpret_cast<byte *>(micron::addr(n->key)), reinterpret_cast<const byte *>(micron::addr(m->key)), sizeof(K));
        else
          new (micron::addr(n->key)) K(m->key);

        if constexpr ( micron::is_trivially_copyable_v<V> )
          micron::bytecpy(reinterpret_cast<byte *>(micron::addr(n->value)), reinterpret_cast<const byte *>(micron::addr(m->value)),
                          sizeof(V));
        else
          new (micron::addr(n->value)) V(m->value);

        __node *old_right = n->right;
        n->right = __erase_min_impl(old_right);
        __release(old_right);
        erased = true;
      } else {
        __node *old_right = n->right;
        n->right = __erase_impl(old_right, k, erased);
        __release(old_right);
      }
    }

    return __fixup(n);
  }

  static inline const __node *
  __find_impl(const __node *h, const K &k)
  {
    while ( h ) [[likely]] {
      const bool less = k < h->key;
      if ( less )
        h = h->left();
      else if ( h->key < k )
        h = h->right;
      else
        return h;
    }
    return nullptr;
  }

  static inline __node *
  __ensure_black_root(__node *root)
  {
    if ( root && root->red() ) [[unlikely]]
      root->set_red(false);     // always unique
    return root;
  }

  __node *__root;
  usize __length;

  // private constructor for internals
  immutable_map(__node *root, usize len) : __root(root), __length(len) {}

public:
  using category_type = map_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef usize size_type;
  typedef K key_type;
  typedef V mapped_type;

  ~immutable_map() { __release(__root); }

  immutable_map() : __root(nullptr), __length(0) {}

  // O(1) copy
  immutable_map(const immutable_map &o) : __root(__retain(o.__root)), __length(o.__length) {}

  immutable_map &
  operator=(const immutable_map &o)
  {
    if ( this != &o ) [[likely]] {
      __release(__root);
      __root = __retain(o.__root);
      __length = o.__length;
    }
    return *this;
  }

  // O(1) move
  immutable_map(immutable_map &&o) noexcept : __root(o.__root), __length(o.__length)
  {
    o.__root = nullptr;
    o.__length = 0;
  }

  immutable_map &
  operator=(immutable_map &&o) noexcept
  {
    if ( this != &o ) [[likely]] {
      __release(__root);
      __root = o.__root;
      __length = o.__length;
      o.__root = nullptr;
      o.__length = 0;
    }
    return *this;
  }

  immutable_map
  insert(const K &k, const V &v) const
  {
    bool inserted = false;
    __node *nr = __ensure_black_root(__insert_impl(__root, k, v, inserted));
    return immutable_map(nr, inserted ? __length + 1 : __length);
  }

  immutable_map
  insert(const K &k, V &&v) const
  {
    bool inserted = false;
    __node *nr = __ensure_black_root(__insert_impl(__root, k, micron::move(v), inserted));
    return immutable_map(nr, inserted ? __length + 1 : __length);
  }

  immutable_map
  set(const K &k, const V &v) const
  {
    return insert(k, v);
  }

  immutable_map
  set(const K &k, V &&v) const
  {
    return insert(k, micron::move(v));
  }

  template <typename... Args>
  immutable_map
  emplace(const K &k, Args &&...args) const
  {
    bool inserted = false;
    __node *nr = __ensure_black_root(__emplace_impl(__root, k, inserted, micron::forward<Args>(args)...));
    return immutable_map(nr, inserted ? __length + 1 : __length);
  }

  immutable_map
  erase(const K &k) const
  {
    if ( !__root ) [[unlikely]]
      return immutable_map();
    bool erased = false;
    __retain(__root);
    __node *nr = __ensure_black_root(__erase_impl(__root, k, erased));
    __release(__root);
    return immutable_map(nr, erased ? __length - 1 : __length);
  }

  immutable_map
  clear() const
  {
    return immutable_map();
  }

  const V *
  find(const K &k) const
  {
    const __node *n = __find_impl(__root, k);
    return n ? &n->value : nullptr;
  }

  bool
  contains(const K &k) const
  {
    return __find_impl(__root, k) != nullptr;
  }

  const V &
  at(const K &k) const
  {
    const __node *n = __find_impl(__root, k);
    if ( !n ) [[unlikely]]
      exc<except::library_error>("micron::immutable_map at(): key not found");
    return n->value;
  }

  const V &
  operator[](const K &k) const
  {
    const __node *n = __find_impl(__root, k);
    if ( n ) [[likely]]
      return n->value;
    static const V __default{};
    return __default;
  }

  usize
  size() const noexcept
  {
    return __length;
  }

  bool
  empty() const noexcept
  {
    return __length == 0;
  }

  usize
  capacity() const noexcept
  {
    return __length;
  }

  float
  load_factor() const noexcept
  {
    return __length > 0 ? 1.0f : 0.0f;
  }

  const void *
  identity() const noexcept
  {
    return __root;
  }

  bool
  operator==(const immutable_map &o) const
  {
    if ( __root == o.__root ) [[unlikely]]
      return true;
    if ( __length != o.__length ) return false;
    auto a = begin(), ae = end();
    auto b = o.begin();
    for ( ; a != ae; ++a, ++b ) {
      if ( a.key() != b.key() || a.value() != b.value() ) return false;
    }
    return true;
  }

  bool
  operator!=(const immutable_map &o) const
  {
    return !(*this == o);
  }

  template <typename Fn>
  immutable_map
  update(const K &k, Fn &&fn) const
  {
    const __node *n = __find_impl(__root, k);
    if ( !n ) [[unlikely]]
      return *this;
    return insert(k, fn(n->value));
  }

  template <typename Fn>
  immutable_map
  update_or(const K &k, const V &default_val, Fn &&fn) const
  {
    const __node *n = __find_impl(__root, k);
    if ( n ) return insert(k, fn(n->value));
    return insert(k, fn(default_val));
  }

  //  max depth 64 supports trees up to 2^32 nodes

  class const_iterator
  {
    static constexpr usize __max_depth = 64;
    const __node *__stack[__max_depth];
    usize __depth;

    inline void
    __push_left(const __node *n)
    {
      while ( n ) [[likely]] {
        __stack[__depth++] = n;
        n = n->left();
      }
    }

  public:
    const_iterator() : __depth(0) {}

    explicit const_iterator(const __node *root) : __depth(0) { __push_left(root); }

    bool
    operator==(const const_iterator &o) const
    {
      if ( __depth != o.__depth ) return false;
      return __depth == 0 || __stack[__depth - 1] == o.__stack[o.__depth - 1];
    }

    bool
    operator!=(const const_iterator &o) const
    {
      if ( __depth != o.__depth ) return true;
      return __depth != 0 && __stack[__depth - 1] != o.__stack[o.__depth - 1];
    }

    const K &
    key() const
    {
      return __stack[__depth - 1]->key;
    }

    const V &
    value() const
    {
      return __stack[__depth - 1]->value;
    }

    const __node *
    operator->() const
    {
      return __stack[__depth - 1];
    }

    const __node &
    operator*() const
    {
      return *__stack[__depth - 1];
    }

    const_iterator &
    operator++()
    {
      const __node *n = __stack[--__depth];
      __push_left(n->right);
      return *this;
    }

    const_iterator
    operator++(int)
    {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }
  };

  using iterator = const_iterator;

  const_iterator
  begin() const
  {
    return const_iterator(__root);
  }

  const_iterator
  end() const
  {
    return const_iterator();
  }

  const_iterator
  cbegin() const
  {
    return const_iterator(__root);
  }

  const_iterator
  cend() const
  {
    return const_iterator();
  }

  //  morris in-order traversal
  //
  //  WARNING: modifies node->right pointers
  //  ONLY safe when no other version shares any node in this tree
  //  for shared trees, use for_each() or the iterator instead

  template <typename Fn>
  void
  for_each_morris(Fn &&fn) const
  {
    __node *cur = const_cast<__node *>(__root);

    while ( cur ) {
      if ( !cur->left() ) {
        fn(static_cast<const K &>(cur->key), static_cast<const V &>(cur->value));
        cur = cur->right;
      } else {
        __node *pred = cur->left();
        while ( pred->right && pred->right != cur ) pred = pred->right;

        if ( !pred->right ) {
          pred->right = cur;     // thread
          cur = cur->left();
        } else {
          pred->right = nullptr;     // unthread
          fn(static_cast<const K &>(cur->key), static_cast<const V &>(cur->value));
          cur = cur->right;
        }
      }
    }
  }

  template <typename Fn>
  void
  for_each(Fn &&fn) const
  {
    constexpr usize __max_depth = 64;
    const __node *stack[__max_depth];
    usize depth = 0;

    const __node *cur = __root;
    while ( cur || depth > 0 ) {
      while ( cur ) [[likely]] {
        stack[depth++] = cur;
        cur = cur->left();
      }
      cur = stack[--depth];
      fn(static_cast<const K &>(cur->key), static_cast<const V &>(cur->value));
      cur = cur->right;
    }
  }
};

};     // namespace micron
