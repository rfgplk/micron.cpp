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

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//  immutable_table
//
//  persistent ordered associative container for integral types
//  implemented via a binary radix/patricia trie with reference counted nodes

template <typename K, typename V>
  requires micron::integral<K> && micron::integral<V>
class immutable_table
{
  using __uint = micron::make_unsigned_t<K>;
  static constexpr u32 __key_bits = sizeof(K) * 8;

  // XOR sign bit for signed types to preserve ordering in unsigned domain
  static inline __attribute__((always_inline)) __uint
  __to_ord(K k) noexcept
  {
    if constexpr ( micron::is_signed_v<K> )
      return static_cast<__uint>(k) ^ (__uint(1) << (__key_bits - 1));
    else
      return static_cast<__uint>(k);
  }

  // MSB = position 0
  static inline __attribute__((always_inline)) u32
  __bit_at(__uint k, u32 pos) noexcept
  {
    return (k >> (__key_bits - 1 - pos)) & 1;
  }

  // NOTE: __builtin_clz counts leading zeros in a word BUT we need positions relative to __key_bits
  static inline __attribute__((always_inline)) u32
  __clz_impl(__uint v) noexcept
  {
    if constexpr ( sizeof(__uint) <= 4 ) {
      return static_cast<u32>(__builtin_clz(static_cast<unsigned int>(v))) - (32 - __key_bits);
    } else {
      return static_cast<u32>(__builtin_clzll(static_cast<unsigned long long>(v))) - (64 - __key_bits);
    }
  }

  // position of first differing bit between two ordered keys
  static inline u32
  __diff_pos(__uint a, __uint b) noexcept
  {
    __uint diff = a ^ b;
    if ( !diff ) [[unlikely]]
      return __key_bits;
    return __clz_impl(diff);
  }

  struct __leaf {
    mutable u32 refs;
    K key;
    V value;
  };

  struct __branch {
    mutable u32 refs;
    u32 bit_pos;
    uintptr_t child[2];     // tagged pointers
  };

  static inline __attribute__((always_inline)) bool
  __is_leaf(uintptr_t p) noexcept
  {
    return p & 1;
  }

  static inline __attribute__((always_inline)) __leaf *
  __to_leaf(uintptr_t p) noexcept
  {
    return reinterpret_cast<__leaf *>(p & ~uintptr_t(1));
  }

  static inline __attribute__((always_inline)) __branch *
  __to_branch(uintptr_t p) noexcept
  {
    return reinterpret_cast<__branch *>(p);
  }

  static inline __attribute__((always_inline)) uintptr_t
  __tag_leaf(__leaf *l) noexcept
  {
    return reinterpret_cast<uintptr_t>(l) | uintptr_t(1);
  }

  static inline __attribute__((always_inline)) uintptr_t
  __tag_branch(__branch *b) noexcept
  {
    return reinterpret_cast<uintptr_t>(b);
  }

  // uniform refcount access
  static inline __attribute__((always_inline)) u32 &
  __refs(uintptr_t p) noexcept
  {
    return *reinterpret_cast<u32 *>(p & ~uintptr_t(1));
  }

  static inline __leaf *
  __make_leaf(K k, V v)
  {
    auto *l = reinterpret_cast<__leaf *>(abc::alloc(sizeof(__leaf)));
    l->refs = 1;
    l->key = k;
    l->value = v;
    return l;
  }

  // takes ownership of child refs passed in
  static inline __branch *
  __make_branch(u32 bp, uintptr_t c0, uintptr_t c1)
  {
    auto *b = reinterpret_cast<__branch *>(abc::alloc(sizeof(__branch)));
    b->refs = 1;
    b->bit_pos = bp;
    b->child[0] = c0;
    b->child[1] = c1;
    return b;
  }

  static inline void
  __dealloc_leaf(__leaf *l)
  {
    abc::dealloc(reinterpret_cast<byte *>(l));
  }

  static inline void
  __dealloc_branch(__branch *b)
  {
    abc::dealloc(reinterpret_cast<byte *>(b));
  }

  // NOTE: saturating
  static inline __attribute__((always_inline)) void
  __retain_tagged(uintptr_t p)
  {
    if ( p && __refs(p) < __UINT32_MAX__ ) [[likely]]
      ++__refs(p);
  }

  // tail-iterates right child, stacks left child
  static inline void
  __release_tagged(uintptr_t p)
  {
    constexpr usize __stack_cap = 48;
    uintptr_t stack[__stack_cap];
    usize depth = 0;

    while ( p ) [[likely]] {
      u32 &r = __refs(p);
      if ( r == __UINT32_MAX__ ) [[unlikely]]
        break;
      if ( --r != 0 ) [[likely]]
        break;

      if ( __is_leaf(p) ) [[unlikely]] {
        __dealloc_leaf(__to_leaf(p));
        break;
      }

      __branch *b = __to_branch(p);
      uintptr_t c0 = b->child[0];
      uintptr_t c1 = b->child[1];
      __builtin_prefetch(reinterpret_cast<void *>(c1 & ~uintptr_t(1)));
      __dealloc_branch(b);

      if ( c0 ) {
        if ( depth < __stack_cap ) [[likely]]
          stack[depth++] = c0;
        else
          __release_tagged(c0);
      }
      p = c1;
    }
    while ( depth > 0 ) __release_tagged(stack[--depth]);
  }

  // descend to the leaf that the search key reaches
  static inline const __leaf *
  __find_leaf(uintptr_t node, __uint ord)
  {
    while ( !__is_leaf(node) ) {
      __branch *b = __to_branch(node);
      u32 dir = __bit_at(ord, b->bit_pos);
      node = b->child[dir];
    }
    return __to_leaf(node);
  }

  static uintptr_t
  __replace_impl(uintptr_t node, __uint ord, K key, V val)
  {
    if ( __is_leaf(node) ) [[unlikely]]
      return __tag_leaf(__make_leaf(key, val));

    __branch *b = __to_branch(node);
    u32 dir = __bit_at(ord, b->bit_pos);
    uintptr_t new_child = __replace_impl(b->child[dir], ord, key, val);
    uintptr_t other = b->child[dir ^ 1];
    __retain_tagged(other);
    return __tag_branch(__make_branch(b->bit_pos, dir == 0 ? new_child : other, dir == 0 ? other : new_child));
  }

  static uintptr_t
  __splice_impl(uintptr_t node, __uint ord, K key, V val, u32 dpos)
  {
    // splice above this node: new branch at dpos
    if ( __is_leaf(node) || __to_branch(node)->bit_pos > dpos ) {
      u32 dir = __bit_at(ord, dpos);
      __retain_tagged(node);
      uintptr_t nl = __tag_leaf(__make_leaf(key, val));
      return __tag_branch(__make_branch(dpos, dir == 0 ? nl : node, dir == 0 ? node : nl));
    }

    // copy this branch, recurse into correct child
    __branch *b = __to_branch(node);
    u32 dir = __bit_at(ord, b->bit_pos);
    uintptr_t new_child = __splice_impl(b->child[dir], ord, key, val, dpos);
    uintptr_t other = b->child[dir ^ 1];
    __retain_tagged(other);
    return __tag_branch(__make_branch(b->bit_pos, dir == 0 ? new_child : other, dir == 0 ? other : new_child));
  }

  static uintptr_t
  __erase_impl(uintptr_t node, __uint ord, K key, bool &erased)
  {
    if ( __is_leaf(node) ) {
      if ( __to_leaf(node)->key == key ) [[likely]] {
        erased = true;
        return 0;
      }
      erased = false;
      return 0;
    }

    __branch *b = __to_branch(node);
    u32 dir = __bit_at(ord, b->bit_pos);

    bool child_erased = false;
    uintptr_t new_child = __erase_impl(b->child[dir], ord, key, child_erased);

    if ( !child_erased ) [[likely]] {
      erased = false;
      return 0;
    }

    erased = true;
    uintptr_t sibling = b->child[dir ^ 1];

    if ( new_child == 0 ) {
      // deleted child was a leaf
      __retain_tagged(sibling);
      return sibling;
    }

    // copy
    __retain_tagged(sibling);
    return __tag_branch(__make_branch(b->bit_pos, dir == 0 ? new_child : sibling, dir == 0 ? sibling : new_child));
  }

  uintptr_t __root;
  usize __length;

  immutable_table(uintptr_t root, usize len) : __root(root), __length(len) {}

public:
  using category_type = map_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef usize size_type;
  typedef K key_type;
  typedef V mapped_type;

  ~immutable_table() { __release_tagged(__root); }

  immutable_table() : __root(0), __length(0) {}

  // O(1) copy
  immutable_table(const immutable_table &o) : __root(o.__root), __length(o.__length) { __retain_tagged(__root); }

  immutable_table &
  operator=(const immutable_table &o)
  {
    if ( this != &o ) [[likely]] {
      __release_tagged(__root);
      __root = o.__root;
      __length = o.__length;
      __retain_tagged(__root);
    }
    return *this;
  }

  // O(1) move
  immutable_table(immutable_table &&o) noexcept : __root(o.__root), __length(o.__length)
  {
    o.__root = 0;
    o.__length = 0;
  }

  immutable_table &
  operator=(immutable_table &&o) noexcept
  {
    if ( this != &o ) [[likely]] {
      __release_tagged(__root);
      __root = o.__root;
      __length = o.__length;
      o.__root = 0;
      o.__length = 0;
    }
    return *this;
  }

  immutable_table
  insert(K key, V val) const
  {
    if ( !__root ) [[unlikely]]
      return immutable_table(__tag_leaf(__make_leaf(key, val)), 1);

    __uint ord = __to_ord(key);
    const __leaf *existing = __find_leaf(__root, ord);

    if ( existing->key == key ) [[unlikely]] {
      // update value
      uintptr_t nr = __replace_impl(__root, ord, key, val);
      return immutable_table(nr, __length);
    }

    // new key
    u32 dpos = __diff_pos(ord, __to_ord(existing->key));
    uintptr_t nr = __splice_impl(__root, ord, key, val, dpos);
    return immutable_table(nr, __length + 1);
  }

  immutable_table
  set(K key, V val) const
  {
    return insert(key, val);
  }

  template <typename... Args>
  immutable_table
  emplace(K key, Args &&...args) const
  {
    return insert(key, V(micron::forward<Args>(args)...));
  }

  immutable_table
  erase(K key) const
  {
    if ( !__root ) [[unlikely]]
      return *this;

    bool erased = false;
    __uint ord = __to_ord(key);
    uintptr_t nr = __erase_impl(__root, ord, key, erased);

    if ( !erased ) [[likely]]
      return *this;
    if ( !nr ) return immutable_table(0, 0);
    return immutable_table(nr, __length - 1);
  }

  immutable_table
  clear() const
  {
    return immutable_table();
  }

  const V *
  find(K key) const
  {
    if ( !__root ) [[unlikely]]
      return nullptr;
    const __leaf *l = __find_leaf(__root, __to_ord(key));
    return l->key == key ? &l->value : nullptr;
  }

  bool
  contains(K key) const
  {
    return find(key) != nullptr;
  }

  const V &
  at(K key) const
  {
    const V *v = find(key);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("micron::immutable_table at(): key not found");
    return *v;
  }

  const V &
  operator[](K key) const
  {
    if ( !__root ) [[unlikely]] {
      static const V __default{};
      return __default;
    }
    const __leaf *l = __find_leaf(__root, __to_ord(key));
    if ( l->key == key ) [[likely]]
      return l->value;
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
    return reinterpret_cast<const void *>(__root);
  }

  bool
  operator==(const immutable_table &o) const
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
  operator!=(const immutable_table &o) const
  {
    return !(*this == o);
  }

  template <typename Fn>
  immutable_table
  update(K key, Fn &&fn) const
  {
    const V *v = find(key);
    if ( !v ) [[unlikely]]
      return *this;
    return insert(key, fn(*v));
  }

  template <typename Fn>
  immutable_table
  update_or(K key, V default_val, Fn &&fn) const
  {
    const V *v = find(key);
    if ( v ) return insert(key, fn(*v));
    return insert(key, fn(default_val));
  }

  class const_iterator
  {
    static constexpr usize __max_depth = __key_bits;
    const __branch *__stack[__max_depth];
    usize __depth;
    const __leaf *__current;

    void
    __descend_left(uintptr_t node)
    {
      while ( !__is_leaf(node) ) {
        __stack[__depth++] = __to_branch(node);
        node = __to_branch(node)->child[0];
      }
      __current = __to_leaf(node);
    }

  public:
    const_iterator() : __depth(0), __current(nullptr) {}

    explicit const_iterator(uintptr_t root) : __depth(0), __current(nullptr)
    {
      if ( root ) __descend_left(root);
    }

    bool
    operator==(const const_iterator &o) const
    {
      return __current == o.__current;
    }

    bool
    operator!=(const const_iterator &o) const
    {
      return __current != o.__current;
    }

    K
    key() const
    {
      return __current->key;
    }

    V
    value() const
    {
      return __current->value;
    }

    const __leaf *
    operator->() const
    {
      return __current;
    }

    const __leaf &
    operator*() const
    {
      return *__current;
    }

    const_iterator &
    operator++()
    {
      if ( __depth == 0 ) [[unlikely]] {
        __current = nullptr;
        return *this;
      }
      const __branch *b = __stack[--__depth];
      __descend_left(b->child[1]);
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

  template <typename Fn>
  void
  for_each(Fn &&fn) const
  {
    if ( !__root ) [[unlikely]]
      return;

    constexpr usize __max_depth = __key_bits;
    const __branch *stack[__max_depth];
    usize depth = 0;
    uintptr_t node = __root;

    for ( ;; ) {
      while ( !__is_leaf(node) ) {
        stack[depth++] = __to_branch(node);
        node = __to_branch(node)->child[0];
      }
      fn(__to_leaf(node)->key, __to_leaf(node)->value);

      if ( depth == 0 ) return;
      node = stack[--depth]->child[1];
    }
  }
};

};     // namespace micron
