//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits.hpp"
#include "../hash/hash.hpp"
#include "../types.hpp"

#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/new.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "__tree_store.hpp"

namespace micron
{

// adaptive radix tree map
template<typename K, typename V>
  requires micron::is_copy_constructible_v<K> and micron::is_copy_constructible_v<V> and micron::is_move_constructible_v<V>
class art
{
  enum class __node_kind : u8 { leaf = 0, n4 = 1, n16 = 2, n48 = 3, n256 = 4 };

  struct __node_base {
    __node_kind kind;
    u16 num_children;

    __node_base(__node_kind k) : kind(k), num_children(0) { }
  };

  struct __leaf: __node_base {
    hash64_t hash;
    K key;
    V value;
    __leaf *next;

    __leaf(hash64_t h, const K &k, const V &v) : __node_base(__node_kind::leaf), hash(h), key(k), value(v), next(nullptr) { }

    __leaf(hash64_t h, const K &k, V &&v) : __node_base(__node_kind::leaf), hash(h), key(k), value(micron::move(v)), next(nullptr) { }
  };

  struct __n4: __node_base {
    u8 keys[4];
    __node_base *children[4];

    __n4() : __node_base(__node_kind::n4)
    {
      for ( int i = 0; i < 4; ++i ) {
        keys[i] = 0;
        children[i] = nullptr;
      }
    }
  };

  struct __n16: __node_base {
    u8 keys[16];
    __node_base *children[16];

    __n16() : __node_base(__node_kind::n16)
    {
      for ( int i = 0; i < 16; ++i ) {
        keys[i] = 0;
        children[i] = nullptr;
      }
    }
  };

  struct __n48: __node_base {
    u8 idx[256];
    __node_base *children[48];

    __n48() : __node_base(__node_kind::n48)
    {
      for ( int i = 0; i < 256; ++i ) idx[i] = 0;
      for ( int i = 0; i < 48; ++i ) children[i] = nullptr;
    }
  };

  struct __n256: __node_base {
    __node_base *children[256];

    __n256() : __node_base(__node_kind::n256)
    {
      for ( int i = 0; i < 256; ++i ) children[i] = nullptr;
    }
  };

  __node_base *__root = nullptr;
  usize __size = 0;

  // %%%%%%%%%%%%%%%%%%%%%
  // node pools

  __tree_store::block_pool<__leaf> __pleaf;
  __tree_store::block_pool<__n4> __p4;
  __tree_store::block_pool<__n16> __p16;
  __tree_store::block_pool<__n48> __p48;
  __tree_store::block_pool<__n256> __p256;

  template<class N, class... A>
  [[gnu::always_inline]] N *
  __mk(__tree_store::block_pool<N> &pool, A &&...a)
  {
    N *mem = pool.take();
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      return new (static_cast<void *>(mem)) N(micron::forward<A>(a)...);
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      pool.give(mem);
      throw;
    }
#endif
  }

  template<class N>
  [[gnu::always_inline]] void
  __rm(__tree_store::block_pool<N> &pool, N *p) noexcept
  {
    p->~N();
    pool.give(p);
  }

  void
  __free_node(__node_base *n) noexcept
  {
    switch ( n->kind ) {
    case __node_kind::leaf:
      __rm(__pleaf, static_cast<__leaf *>(n));
      break;
    case __node_kind::n4:
      __rm(__p4, static_cast<__n4 *>(n));
      break;
    case __node_kind::n16:
      __rm(__p16, static_cast<__n16 *>(n));
      break;
    case __node_kind::n48:
      __rm(__p48, static_cast<__n48 *>(n));
      break;
    case __node_kind::n256:
      __rm(__p256, static_cast<__n256 *>(n));
      break;
    }
  }

  // K and V destructors
  void
  __destroy_leaves(__node_base *n) noexcept
  {
    if ( !n ) return;
    if ( n->kind == __node_kind::leaf ) {
      for ( __leaf *cur = static_cast<__leaf *>(n); cur; cur = cur->next ) cur->~__leaf();
      return;
    }
    switch ( n->kind ) {
    case __node_kind::n4: {
      auto *p = static_cast<__n4 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i ) __destroy_leaves(p->children[i]);
      break;
    }
    case __node_kind::n16: {
      auto *p = static_cast<__n16 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i ) __destroy_leaves(p->children[i]);
      break;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      for ( u16 i = 0; i < 48; ++i )
        if ( p->children[i] ) __destroy_leaves(p->children[i]);
      break;
    }
    case __node_kind::n256: {
      auto *p = static_cast<__n256 *>(n);
      for ( u16 i = 0; i < 256; ++i ) __destroy_leaves(p->children[i]);
      break;
    }
    default:
      break;
    }
  }

  void
  __release_all() noexcept
  {
    if constexpr ( !(micron::is_trivially_destructible_v<K> && micron::is_trivially_destructible_v<V>) )
      __destroy_leaves(__root);
    __pleaf.release();
    __p4.release();
    __p16.release();
    __p48.release();
    __p256.release();
    __root = nullptr;
    __size = 0;
  }

  static constexpr usize __hash_bytes = sizeof(hash64_t);

  static u8
  __byte_at(hash64_t h, usize depth) noexcept
  {
    if ( depth >= __hash_bytes ) [[unlikely]]
      return 0u;
    return static_cast<u8>((h >> (8u * depth)) & 0xFFu);
  }

  static __node_base *
  __find_child(__node_base *n, u8 b) noexcept
  {
    if ( !n || n->kind == __node_kind::leaf ) return nullptr;
    switch ( n->kind ) {
    case __node_kind::n4: {
      auto *p = static_cast<__n4 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i )
        if ( p->keys[i] == b ) return p->children[i];
      return nullptr;
    }
    case __node_kind::n16: {
      auto *p = static_cast<__n16 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i )
        if ( p->keys[i] == b ) return p->children[i];
      return nullptr;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      u8 ix = p->idx[b];
      return ix == 0 ? nullptr : p->children[ix - 1];
    }
    case __node_kind::n256: {
      auto *p = static_cast<__n256 *>(n);
      return p->children[b];
    }
    default:
      return nullptr;
    }
  }

  static __node_base **
  __find_child_slot(__node_base *n, u8 b) noexcept
  {
    if ( !n || n->kind == __node_kind::leaf ) return nullptr;
    switch ( n->kind ) {
    case __node_kind::n4: {
      auto *p = static_cast<__n4 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i )
        if ( p->keys[i] == b ) return &p->children[i];
      return nullptr;
    }
    case __node_kind::n16: {
      auto *p = static_cast<__n16 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i )
        if ( p->keys[i] == b ) return &p->children[i];
      return nullptr;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      u8 ix = p->idx[b];
      return ix == 0 ? nullptr : &p->children[ix - 1];
    }
    case __node_kind::n256: {
      auto *p = static_cast<__n256 *>(n);
      return p->children[b] ? &p->children[b] : nullptr;
    }
    default:
      return nullptr;
    }
  }

  __node_base *
  __grow(__node_base *n)
  {
    switch ( n->kind ) {
    case __node_kind::n4: {
      auto *p = static_cast<__n4 *>(n);
      auto *q = __mk(__p16);
      q->num_children = p->num_children;
      for ( u8 i = 0; i < 4u; ++i ) {
        q->keys[i] = p->keys[i];
        q->children[i] = p->children[i];
      }
      __rm(__p4, p);
      return q;
    }
    case __node_kind::n16: {
      auto *p = static_cast<__n16 *>(n);
      auto *q = __mk(__p48);
      q->num_children = p->num_children;
      for ( u8 i = 0; i < 16u; ++i ) {
        q->idx[p->keys[i]] = i + 1;
        q->children[i] = p->children[i];
      }
      __rm(__p16, p);
      return q;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      auto *q = __mk(__p256);
      q->num_children = p->num_children;
      for ( int b = 0; b < 256; ++b ) {
        if ( p->idx[b] != 0 ) q->children[b] = p->children[p->idx[b] - 1];
      }
      __rm(__p48, p);
      return q;
    }
    default:
      return n;
    }
  }

  __node_base *
  __add_child(__node_base *n, u8 b, __node_base *child)
  {
    switch ( n->kind ) {
    case __node_kind::n4: {
      auto *p = static_cast<__n4 *>(n);
      if ( p->num_children == 4 ) {
        n = __grow(n);
        return __add_child(n, b, child);
      }
      p->keys[p->num_children] = b;
      p->children[p->num_children] = child;
      ++p->num_children;
      return n;
    }
    case __node_kind::n16: {
      auto *p = static_cast<__n16 *>(n);
      if ( p->num_children == 16 ) {
        n = __grow(n);
        return __add_child(n, b, child);
      }
      p->keys[p->num_children] = b;
      p->children[p->num_children] = child;
      ++p->num_children;
      return n;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      if ( p->num_children == 48 ) {
        n = __grow(n);
        return __add_child(n, b, child);
      }
      p->idx[b] = p->num_children + 1;
      p->children[p->num_children] = child;
      ++p->num_children;
      return n;
    }
    case __node_kind::n256: {
      auto *p = static_cast<__n256 *>(n);
      if ( !p->children[b] ) ++p->num_children;
      p->children[b] = child;
      return n;
    }
    default:
      return n;
    }
  }

  __node_base *
  __shrink(__node_base *n)
  {
    switch ( n->kind ) {
    case __node_kind::n16: {
      auto *p = static_cast<__n16 *>(n);
      auto *q = __mk(__p4);
      q->num_children = p->num_children;
      for ( u8 i = 0; i < p->num_children; ++i ) {
        q->keys[i] = p->keys[i];
        q->children[i] = p->children[i];
      }
      __rm(__p16, p);
      return q;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      auto *q = __mk(__p16);
      u8 m = 0;
      for ( int bb = 0; bb < 256; ++bb )
        if ( p->idx[bb] != 0 ) {
          q->keys[m] = static_cast<u8>(bb);
          q->children[m] = p->children[p->idx[bb] - 1];
          ++m;
        }
      q->num_children = m;
      __rm(__p48, p);
      return q;
    }
    case __node_kind::n256: {
      auto *p = static_cast<__n256 *>(n);
      auto *q = __mk(__p48);
      u8 m = 0;
      for ( int bb = 0; bb < 256; ++bb )
        if ( p->children[bb] ) {
          q->children[m] = p->children[bb];
          q->idx[bb] = static_cast<u8>(m + 1);
          ++m;
        }
      q->num_children = m;
      __rm(__p256, p);
      return q;
    }
    default:
      return n;
    }
  }

  __node_base *
  __remove_child(__node_base *n, u8 b)
  {
    switch ( n->kind ) {
    case __node_kind::n4: {
      auto *p = static_cast<__n4 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i ) {
        if ( p->keys[i] == b ) {
          for ( u8 j = i; j + 1 < p->num_children; ++j ) {
            p->keys[j] = p->keys[j + 1];
            p->children[j] = p->children[j + 1];
          }
          --p->num_children;
          if ( p->num_children == 0 ) {
            __rm(__p4, p);
            return nullptr;
          }
          return n;
        }
      }
      return n;
    }
    case __node_kind::n16: {
      auto *p = static_cast<__n16 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i ) {
        if ( p->keys[i] == b ) {
          for ( u8 j = i; j + 1 < p->num_children; ++j ) {
            p->keys[j] = p->keys[j + 1];
            p->children[j] = p->children[j + 1];
          }
          --p->num_children;
          if ( p->num_children == 0 ) {
            __rm(__p16, p);
            return nullptr;
          }
          if ( p->num_children <= 3 ) return __shrink(n);
          return n;
        }
      }
      return n;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      u8 ix = p->idx[b];
      if ( ix == 0 ) return n;
      u16 slot = static_cast<u16>(ix - 1);
      u16 last = static_cast<u16>(p->num_children - 1);
      if ( slot != last ) {
        p->children[slot] = p->children[last];
        for ( int bb = 0; bb < 256; ++bb ) {
          if ( p->idx[bb] == last + 1 ) {
            p->idx[bb] = static_cast<u8>(slot + 1);
            break;
          }
        }
      }
      p->children[last] = nullptr;
      p->idx[b] = 0;
      --p->num_children;
      if ( p->num_children == 0 ) {
        __rm(__p48, p);
        return nullptr;
      }
      if ( p->num_children <= 12 ) return __shrink(n);
      return n;
    }
    case __node_kind::n256: {
      auto *p = static_cast<__n256 *>(n);
      if ( p->children[b] ) {
        p->children[b] = nullptr;
        --p->num_children;
      }
      if ( p->num_children == 0 ) {
        __rm(__p256, p);
        return nullptr;
      }
      if ( p->num_children <= 37 ) return __shrink(n);
      return n;
    }
    default:
      return n;
    }
  }

  void
  __release(__node_base *n)
  {
    if ( !n ) return;
    if ( n->kind == __node_kind::leaf ) {
      __leaf *cur = static_cast<__leaf *>(n);
      while ( cur ) {
        __leaf *nx = cur->next;
        __rm(__pleaf, cur);
        cur = nx;
      }
      return;
    }
    switch ( n->kind ) {
    case __node_kind::n4: {
      auto *p = static_cast<__n4 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i ) __release(p->children[i]);
      __rm(__p4, p);
      break;
    }
    case __node_kind::n16: {
      auto *p = static_cast<__n16 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i ) __release(p->children[i]);
      __rm(__p16, p);
      break;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      for ( u8 i = 0; i < 48; ++i )
        if ( p->children[i] ) __release(p->children[i]);
      __rm(__p48, p);
      break;
    }
    case __node_kind::n256: {
      auto *p = static_cast<__n256 *>(n);
      for ( int i = 0; i < 256; ++i ) __release(p->children[i]);
      __rm(__p256, p);
      break;
    }
    default:
      break;
    }
  }

  __node_base *
  __insert(__node_base *n, hash64_t h, const K &k, V v, usize depth, bool &inserted)
  {
    if ( !n ) {
      inserted = true;
      return __mk(__pleaf, h, k, micron::move(v));
    }
    if ( n->kind == __node_kind::leaf ) {
      auto *lf = static_cast<__leaf *>(n);
      if ( lf->hash == h ) {

        __leaf *cur = lf;
        while ( true ) {
          if ( cur->key == k ) {
            cur->value = micron::move(v);
            inserted = false;
            return n;
          }
          if ( !cur->next ) {
            cur->next = __mk(__pleaf, h, k, micron::move(v));
            inserted = true;
            return n;
          }
          cur = cur->next;
        }
      }

      if ( depth >= __hash_bytes ) [[unlikely]] {
        __leaf *cur = lf;
        while ( cur->next ) cur = cur->next;
        cur->next = __mk(__pleaf, h, k, micron::move(v));
        inserted = true;
        return n;
      }

      u8 b_old = __byte_at(lf->hash, depth);
      u8 b_new = __byte_at(h, depth);
      auto *inner = __mk(__p4);
      __node_base *new_root = inner;
      if ( b_old == b_new ) {

        __node_base *deeper = __split_descend(lf, h, k, micron::move(v), depth + 1);
        new_root = __add_child(new_root, b_old, deeper);
        inserted = true;
        return new_root;
      }
      __node_base *new_leaf = __mk(__pleaf, h, k, micron::move(v));
      new_root = __add_child(new_root, b_old, lf);
      new_root = __add_child(new_root, b_new, new_leaf);
      inserted = true;
      return new_root;
    }

    u8 b = __byte_at(h, depth);
    __node_base **slot = __find_child_slot(n, b);
    if ( slot ) {
      *slot = __insert(*slot, h, k, micron::move(v), depth + 1, inserted);
      return n;
    }

    auto *new_leaf = __mk(__pleaf, h, k, micron::move(v));
    inserted = true;
    return __add_child(n, b, new_leaf);
  }

  __node_base *
  __split_descend(__leaf *lf, hash64_t h, const K &k, V v, usize depth)
  {
    if ( depth >= __hash_bytes ) [[unlikely]] {
      __leaf *cur = lf;
      while ( cur->next ) cur = cur->next;
      cur->next = __mk(__pleaf, h, k, micron::move(v));
      return lf;
    }
    u8 b_old = __byte_at(lf->hash, depth);
    u8 b_new = __byte_at(h, depth);
    if ( b_old != b_new ) {
      __node_base *new_root = __mk(__p4);
      __node_base *new_leaf = __mk(__pleaf, h, k, micron::move(v));
      new_root = __add_child(new_root, b_old, lf);
      new_root = __add_child(new_root, b_new, new_leaf);
      return new_root;
    }
    auto *inner = __mk(__p4);
    __node_base *deeper = __split_descend(lf, h, k, micron::move(v), depth + 1);
    return __add_child(inner, b_old, deeper);
  }

  bool
  __erase(__node_base *&n, hash64_t h, const K &k, usize depth)
  {
    if ( !n ) return false;
    if ( n->kind == __node_kind::leaf ) {
      __leaf *head = static_cast<__leaf *>(n);

      __leaf *prev = nullptr;
      for ( __leaf *cur = head; cur; cur = cur->next ) {
        if ( cur->hash == h && cur->key == k ) {
          if ( !prev ) {

            if ( cur->next ) {
              n = cur->next;
              cur->next = nullptr;
              __rm(__pleaf, cur);
              return true;
            }
            __rm(__pleaf, cur);
            n = nullptr;
            return true;
          }
          prev->next = cur->next;
          cur->next = nullptr;
          __rm(__pleaf, cur);
          return true;
        }
        prev = cur;
      }
      return false;
    }
    u8 b = __byte_at(h, depth);
    __node_base **slot = __find_child_slot(n, b);
    if ( !slot ) return false;
    __node_base *child = *slot;
    bool e = __erase(child, h, k, depth + 1);
    if ( !e ) return false;
    if ( child == nullptr ) {
      n = __remove_child(n, b);
    } else {
      *slot = child;
    }
    return true;
  }

  __leaf *
  __find_leaf(__node_base *n, hash64_t h, const K &k, usize depth)
  {
    while ( n ) {
      if ( n->kind == __node_kind::leaf ) {
        for ( __leaf *lf = static_cast<__leaf *>(n); lf; lf = lf->next ) {
          if ( lf->hash == h && lf->key == k ) return lf;
        }
        return nullptr;
      }
      u8 b = __byte_at(h, depth);
      n = __find_child(n, b);
      ++depth;
    }
    return nullptr;
  }

  template<typename Fn>
  void
  __walk(__node_base *n, Fn &&fn)
  {
    if ( !n ) return;
    if ( n->kind == __node_kind::leaf ) {
      for ( auto *lf = static_cast<__leaf *>(n); lf; lf = lf->next ) fn(lf->key, lf->value);
      return;
    }
    switch ( n->kind ) {
    case __node_kind::n4: {
      auto *p = static_cast<__n4 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i ) __walk(p->children[i], micron::forward<Fn>(fn));
      break;
    }
    case __node_kind::n16: {
      auto *p = static_cast<__n16 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i ) __walk(p->children[i], micron::forward<Fn>(fn));
      break;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      for ( u8 i = 0; i < 48; ++i )
        if ( p->children[i] ) __walk(p->children[i], micron::forward<Fn>(fn));
      break;
    }
    case __node_kind::n256: {
      auto *p = static_cast<__n256 *>(n);
      for ( int i = 0; i < 256; ++i )
        if ( p->children[i] ) __walk(p->children[i], micron::forward<Fn>(fn));
      break;
    }
    default:
      break;
    }
  }

  static usize
  __bytes_rec(__node_base *n) noexcept
  {
    if ( !n ) return 0;
    if ( n->kind == __node_kind::leaf ) {
      usize t = 0;
      for ( __leaf *c = static_cast<__leaf *>(n); c; c = c->next ) t += sizeof(__leaf);
      return t;
    }
    switch ( n->kind ) {
    case __node_kind::n4: {
      auto *p = static_cast<__n4 *>(n);
      usize t = sizeof(__n4);
      for ( u8 i = 0; i < p->num_children; ++i ) t += __bytes_rec(p->children[i]);
      return t;
    }
    case __node_kind::n16: {
      auto *p = static_cast<__n16 *>(n);
      usize t = sizeof(__n16);
      for ( u8 i = 0; i < p->num_children; ++i ) t += __bytes_rec(p->children[i]);
      return t;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      usize t = sizeof(__n48);
      for ( u16 i = 0; i < 48; ++i )
        if ( p->children[i] ) t += __bytes_rec(p->children[i]);
      return t;
    }
    case __node_kind::n256: {
      auto *p = static_cast<__n256 *>(n);
      usize t = sizeof(__n256);
      for ( u16 i = 0; i < 256; ++i ) t += __bytes_rec(p->children[i]);
      return t;
    }
    default:
      return 0;
    }
  }

public:
  using category_type = tree_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using size_type = usize;
  using key_type = K;
  using mapped_type = V;

  ~art() { __release_all(); }

  art() = default;

  template<class Fn>
    requires(micron::is_invocable_v<Fn, art &>)
  explicit art(Fn build) : art()
  {
    build(*this);
  }

  art(const art &) = delete;
  art &operator=(const art &) = delete;

  art(art &&o) noexcept
      : __root(o.__root), __size(o.__size), __pleaf(micron::move(o.__pleaf)), __p4(micron::move(o.__p4)),
        __p16(micron::move(o.__p16)), __p48(micron::move(o.__p48)), __p256(micron::move(o.__p256))
  {
    o.__root = nullptr;
    o.__size = 0;
  }

  art &
  operator=(art &&o) noexcept
  {
    if ( this == &o ) return *this;
    __release_all();
    __root = o.__root;
    __size = o.__size;
    __pleaf = micron::move(o.__pleaf);
    __p4 = micron::move(o.__p4);
    __p16 = micron::move(o.__p16);
    __p48 = micron::move(o.__p48);
    __p256 = micron::move(o.__p256);
    o.__root = nullptr;
    o.__size = 0;
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
    __release_all();
  }

  // block count across all five pools
  [[nodiscard]] usize
  node_bytes_live() const noexcept
  {
    return __bytes_rec(const_cast<art *>(this)->__root);
  }

  [[nodiscard]] usize
  blocks_allocated() const noexcept
  {
    return __pleaf.blocks() + __p4.blocks() + __p16.blocks() + __p48.blocks() + __p256.blocks();
  }

  [[nodiscard]] usize
  pool_bytes() const noexcept
  {
    return __pleaf.bytes_held() + __p4.bytes_held() + __p16.bytes_held() + __p48.bytes_held() + __p256.bytes_held();
  }

  bool
  insert(const K &k, V v)
  {
    bool ins = false;
    __root = __insert(__root, hash<hash64_t>(k), k, micron::move(v), 0, ins);
    if ( ins ) ++__size;
    return ins;
  }

  bool
  insert_or_assign(const K &k, V v)
  {
    bool ins = false;
    hash64_t h = hash<hash64_t>(k);
    __root = __insert(__root, h, k, micron::move(v), 0, ins);
    if ( ins ) ++__size;
    return ins;
  }

  template<class Fn>
  V &
  update(const K &k, Fn fn)
  {
    V *cur = find(k);
    V nv = fn(static_cast<const V *>(cur));
    insert_or_assign(k, micron::move(nv));
    return *find(k);
  }

  template<class MakeV, class Modify>
  V &
  insert_or_modify(const K &k, MakeV make, Modify modify)
  {
    V *cur = find(k);
    if ( cur ) {
      modify(*cur);
      return *cur;
    }
    insert(k, make());
    return *find(k);
  }

  template<typename... Args>
  bool
  emplace(const K &k, Args &&...args)
  {
    return insert(k, V(micron::forward<Args>(args)...));
  }

  usize
  count(const K &k) const noexcept
  {
    return contains(k) ? 1u : 0u;
  }

  usize
  max_size() const noexcept
  {
    return __size;
  }

  void
  swap(art &o) noexcept
  {
    __pleaf.swap(o.__pleaf);
    __p4.swap(o.__p4);
    __p16.swap(o.__p16);
    __p48.swap(o.__p48);
    __p256.swap(o.__p256);
    micron::swap(__root, o.__root);
    micron::swap(__size, o.__size);
  }

  V *
  find(const K &k) noexcept
  {
    __leaf *lf = __find_leaf(__root, hash<hash64_t>(k), k, 0);
    return lf ? &lf->value : nullptr;
  }

  const V *
  find(const K &k) const noexcept
  {
    return const_cast<art *>(this)->find(k);
  }

  bool
  contains(const K &k) const noexcept
  {
    return find(k) != nullptr;
  }

  V &
  at(const K &k)
  {
    V *v = find(k);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("art::at(): key not found");
    return *v;
  }

  bool
  erase(const K &k)
  {
    bool ok = __erase(__root, hash<hash64_t>(k), k, 0);
    if ( ok ) --__size;
    return ok;
  }

  // explicit-stack cursor
  class const_iterator
  {
    struct __frame {
      const __node_base *n;
      u16 i;      // next child slot to try
    };

    __frame __st[__hash_bytes + 2]{};
    i32 __top = -1;
    const __leaf *__lf = nullptr;

    static const __node_base *
    __child_at(const __node_base *n, u16 i) noexcept
    {
      switch ( n->kind ) {
      case __node_kind::n4: {
        auto *p = static_cast<const __n4 *>(n);
        return i < p->num_children ? p->children[i] : nullptr;
      }
      case __node_kind::n16: {
        auto *p = static_cast<const __n16 *>(n);
        return i < p->num_children ? p->children[i] : nullptr;
      }
      case __node_kind::n48: {
        auto *p = static_cast<const __n48 *>(n);
        return i < 48u ? p->children[i] : nullptr;
      }
      case __node_kind::n256: {
        auto *p = static_cast<const __n256 *>(n);
        return i < 256u ? p->children[i] : nullptr;
      }
      default:
        return nullptr;
      }
    }

    static u16
    __slot_count(const __node_base *n) noexcept
    {
      switch ( n->kind ) {
      case __node_kind::n4:
        return static_cast<const __n4 *>(n)->num_children;
      case __node_kind::n16:
        return static_cast<const __n16 *>(n)->num_children;
      case __node_kind::n48:
        return 48u;
      case __node_kind::n256:
        return 256u;
      default:
        return 0u;
      }
    }

    // descend into n, pushing frames, until a leaf is reached
    void
    __descend(const __node_base *n)
    {
      while ( n ) {
        if ( n->kind == __node_kind::leaf ) {
          __lf = static_cast<const __leaf *>(n);
          return;
        }
        __st[++__top] = __frame{ n, 0 };
        n = __step_top();
      }
      __unwind();
    }

    const __node_base *
    __step_top() noexcept
    {
      __frame &f = __st[__top];
      const u16 cnt = __slot_count(f.n);
      while ( f.i < cnt ) {
        const __node_base *c = __child_at(f.n, f.i++);
        if ( c ) return c;
      }
      return nullptr;
    }

    // pop exhausted frames and resume the first one that still has children
    void
    __unwind()
    {
      while ( __top >= 0 ) {
        const __node_base *c = __step_top();
        if ( c ) {
          __descend(c);
          return;
        }
        --__top;
      }
      __lf = nullptr;
    }

  public:
    using value_type = micron::pair<const K &, const V &>;
    using reference = micron::pair<const K &, const V &>;
    using difference_type = ssize_t;

    constexpr const_iterator() = default;

    explicit const_iterator(const __node_base *root)
    {
      if ( root ) __descend(root);
    }

    reference
    operator*() const
    {
      return reference{ __lf->key, __lf->value };
    }

    const_iterator &
    operator++()
    {
      if ( __lf && __lf->next ) {
        __lf = __lf->next;
        return *this;
      }
      __lf = nullptr;
      __unwind();
      return *this;
    }

    const_iterator
    operator++(int)
    {
      const_iterator __t = *this;
      ++*this;
      return __t;
    }

    bool
    operator==(const const_iterator &__o) const noexcept
    {
      return __lf == __o.__lf;
    }

    bool
    operator!=(const const_iterator &__o) const noexcept
    {
      return __lf != __o.__lf;
    }
  };

  using iterator = const_iterator;

  const_iterator
  begin() const
  {
    return const_iterator{ __root };
  }

  const_iterator
  end() const
  {
    return const_iterator{};
  }

  const_iterator
  cbegin() const
  {
    return begin();
  }

  const_iterator
  cend() const
  {
    return end();
  }

  template<typename Fn>
  void
  for_each(Fn &&fn)
  {
    __walk(__root, micron::forward<Fn>(fn));
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const
  {
    const_cast<art *>(this)->__walk(__root, [&](const K &k, V &v) { fn(k, static_cast<const V &>(v)); });
  }

  template<class Fn>
  auto
  map(Fn fn) const
  {
    if constexpr ( micron::is_invocable_v<Fn, const K &, const V &> ) {
      using V2 = micron::remove_cvref_t<micron::invoke_result_t<Fn, const K &, const V &>>;
      art<K, V2> out;
      for_each([&](const K &k, const V &v) { out.insert(k, fn(k, v)); });
      return out;
    } else {
      using V2 = micron::remove_cvref_t<micron::invoke_result_t<Fn, const V &>>;
      art<K, V2> out;
      for_each([&](const K &k, const V &v) { out.insert(k, fn(v)); });
      return out;
    }
  }
};

};      // namespace micron
