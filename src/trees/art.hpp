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

namespace micron
{

// NOTE: IN DEV
// adaptive radix tree map;
template<typename K, typename V>
  requires micron::is_copy_constructible_v<K> and micron::is_copy_constructible_v<V> and micron::is_move_constructible_v<V>
class art
{
  enum class __node_kind : u8 { leaf = 0, n4 = 1, n16 = 2, n48 = 3, n256 = 4 };

  struct __node_base {
    __node_kind kind;
    u8 num_children;

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

  static u8
  __byte_at(hash64_t h, usize depth) noexcept
  {
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
      auto *q = new __n16();
      q->num_children = p->num_children;
      for ( u8 i = 0; i < p->num_children; ++i ) {
        q->keys[i] = p->keys[i];
        q->children[i] = p->children[i];
      }
      delete p;
      return q;
    }
    case __node_kind::n16: {
      auto *p = static_cast<__n16 *>(n);
      auto *q = new __n48();
      q->num_children = p->num_children;
      for ( u8 i = 0; i < p->num_children; ++i ) {
        q->idx[p->keys[i]] = i + 1;
        q->children[i] = p->children[i];
      }
      delete p;
      return q;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      auto *q = new __n256();
      q->num_children = p->num_children;
      for ( int b = 0; b < 256; ++b ) {
        if ( p->idx[b] != 0 ) q->children[b] = p->children[p->idx[b] - 1];
      }
      delete p;
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
            delete p;
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
          return n;
        }
      }
      return n;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      u8 ix = p->idx[b];
      if ( ix == 0 ) return n;
      p->children[ix - 1] = nullptr;
      p->idx[b] = 0;
      --p->num_children;
      return n;
    }
    case __node_kind::n256: {
      auto *p = static_cast<__n256 *>(n);
      if ( p->children[b] ) {
        p->children[b] = nullptr;
        --p->num_children;
      }
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
        delete cur;
        cur = nx;
      }
      return;
    }
    switch ( n->kind ) {
    case __node_kind::n4: {
      auto *p = static_cast<__n4 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i ) __release(p->children[i]);
      delete p;
      break;
    }
    case __node_kind::n16: {
      auto *p = static_cast<__n16 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i ) __release(p->children[i]);
      delete p;
      break;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      for ( u8 i = 0; i < 48; ++i )
        if ( p->children[i] ) __release(p->children[i]);
      delete p;
      break;
    }
    case __node_kind::n256: {
      auto *p = static_cast<__n256 *>(n);
      for ( int i = 0; i < 256; ++i ) __release(p->children[i]);
      delete p;
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
      return new __leaf(h, k, micron::move(v));
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
            cur->next = new __leaf(h, k, micron::move(v));
            inserted = true;
            return n;
          }
          cur = cur->next;
        }
      }

      u8 b_old = __byte_at(lf->hash, depth);
      u8 b_new = __byte_at(h, depth);
      auto *inner = new __n4();
      __node_base *new_root = inner;
      if ( b_old == b_new ) {

        __node_base *deeper = __split_descend(lf, h, k, micron::move(v), depth + 1);
        new_root = __add_child(new_root, b_old, deeper);
        inserted = true;
        return new_root;
      }
      __node_base *new_leaf = new __leaf(h, k, micron::move(v));
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

    auto *new_leaf = new __leaf(h, k, micron::move(v));
    inserted = true;
    return __add_child(n, b, new_leaf);
  }

  __node_base *
  __split_descend(__leaf *lf, hash64_t h, const K &k, V v, usize depth)
  {
    u8 b_old = __byte_at(lf->hash, depth);
    u8 b_new = __byte_at(h, depth);
    if ( b_old != b_new || depth >= 8u ) {
      auto *inner = new __n4();
      __node_base *new_root = inner;
      __node_base *new_leaf = new __leaf(h, k, micron::move(v));
      if ( b_old != b_new ) {
        new_root = __add_child(new_root, b_old, lf);
        new_root = __add_child(new_root, b_new, new_leaf);
      } else {

        __leaf *cur = lf;
        while ( cur->next ) cur = cur->next;
        cur->next = static_cast<__leaf *>(new_leaf);
        delete inner;
        return lf;
      }
      return new_root;
    }
    auto *inner = new __n4();
    __node_base *deeper = __split_descend(lf, h, k, micron::move(v), depth + 1);
    return __add_child(inner, b_old, deeper);
  }

  __node_base *
  __insert_collide(__leaf *old, hash64_t h, const K &k, V v, usize depth, bool &inserted)
  {

    auto *first = new __n4();
    __node_base *head = first;
    __n4 *cur = first;
    usize d = depth;
    while ( d < 8u ) {
      u8 b_old = __byte_at(old->hash, d);
      u8 b_new = __byte_at(h, d);
      if ( b_old != b_new ) {
        cur = static_cast<__n4 *>(__add_child(cur, b_old, old));
        cur = static_cast<__n4 *>(__add_child(cur, b_new, new __leaf(h, k, micron::move(v))));
        inserted = true;
        return head;
      }

      auto *next = new __n4();
      cur = static_cast<__n4 *>(__add_child(cur, b_old, next));
      cur = next;
      ++d;
    }

    __node_base *grown = __add_child(cur, 0u, old);
    grown = __add_child(grown, 1u, new __leaf(h, k, micron::move(v)));

    if ( grown != cur ) {

      if ( head == cur ) {
        head = grown;
      } else {

        __node_base *p = head;
        while ( p && p->kind != __node_kind::leaf ) {

          auto *pn = static_cast<__n4 *>(p);
          for ( u8 i = 0; i < pn->num_children; ++i ) {
            if ( pn->children[i] == cur ) {
              pn->children[i] = grown;
              p = nullptr;
              break;
            }
          }
          if ( !p ) break;
          if ( static_cast<__n4 *>(p)->num_children == 0 ) break;
          p = static_cast<__n4 *>(p)->children[0];
        }
      }
    }
    inserted = true;
    return head;
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
              delete cur;
              return true;
            }
            delete cur;
            n = nullptr;
            return true;
          }
          prev->next = cur->next;
          cur->next = nullptr;
          delete cur;
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
  __scan_for_match(__node_base *n, hash64_t h, const K &k)
  {
    if ( !n ) return nullptr;
    if ( n->kind == __node_kind::leaf ) {
      auto *lf = static_cast<__leaf *>(n);
      if ( lf->hash == h && lf->key == k ) return lf;
      return nullptr;
    }
    switch ( n->kind ) {
    case __node_kind::n4: {
      auto *p = static_cast<__n4 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i ) {
        __leaf *r = __scan_for_match(p->children[i], h, k);
        if ( r ) return r;
      }
      return nullptr;
    }
    case __node_kind::n16: {
      auto *p = static_cast<__n16 *>(n);
      for ( u8 i = 0; i < p->num_children; ++i ) {
        __leaf *r = __scan_for_match(p->children[i], h, k);
        if ( r ) return r;
      }
      return nullptr;
    }
    case __node_kind::n48: {
      auto *p = static_cast<__n48 *>(n);
      for ( u8 i = 0; i < 48; ++i )
        if ( p->children[i] ) {
          __leaf *r = __scan_for_match(p->children[i], h, k);
          if ( r ) return r;
        }
      return nullptr;
    }
    case __node_kind::n256: {
      auto *p = static_cast<__n256 *>(n);
      for ( int i = 0; i < 256; ++i )
        if ( p->children[i] ) {
          __leaf *r = __scan_for_match(p->children[i], h, k);
          if ( r ) return r;
        }
      return nullptr;
    }
    default:
      return nullptr;
    }
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

public:
  using category_type = tree_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using size_type = usize;
  using key_type = K;
  using mapped_type = V;

  ~art() { __release(__root); }

  art() = default;

  art(const art &) = delete;
  art &operator=(const art &) = delete;

  art(art &&o) noexcept : __root(o.__root), __size(o.__size)
  {
    o.__root = nullptr;
    o.__size = 0;
  }

  art &
  operator=(art &&o) noexcept
  {
    if ( this == &o ) return *this;
    __release(__root);
    __root = o.__root;
    __size = o.__size;
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
    __release(__root);
    __root = nullptr;
    __size = 0;
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

  template<typename Fn>
  void
  for_each(Fn &&fn)
  {
    __walk(__root, micron::forward<Fn>(fn));
  }
};

};      // namespace micron
