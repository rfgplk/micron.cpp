//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../except.hpp"
#include "../memory/addr.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "__tree_store.hpp"
#include "__tree_walk.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron::radix_tree
//
// a compressed radix trie (Patricia) over sequence keys

namespace micron
{

template<typename Key, typename Value>
  requires micron::is_copy_constructible_v<Key> && micron::is_move_constructible_v<Key> && micron::is_copy_constructible_v<Value>
           && micron::is_move_constructible_v<Value>
class radix_tree
{
public:
  using category_type = tree_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using size_type = usize;
  using key_type = Key;
  using mapped_type = Value;

private:
  struct node {
    Key frag;           // the edge label leading into this node
    Value value;        // meaningful only when has_value
    node *child;        // first child
    node *sibling;      // next sibling; the chain is keyed by frag[0], which is unique per level
    bool has_value;

    node() : frag(), value(), child(nullptr), sibling(nullptr), has_value(false) { }
  };

  using __pool_t = __tree_store::block_pool<node>;

  __pool_t __pool;
  node *__root;
  usize __size;

  template<class... A>
  node *
  __mk(A &&...a)
  {
    node *p = __pool.take();
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      return new (static_cast<void *>(p)) node(micron::forward<A>(a)...);
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      __pool.give(p);
      throw;
    }
#endif
  }

  void
  __free(node *p) noexcept
  {
    p->~node();
    __pool.give(p);
  }

  static usize
  __match_at(const Key &frag, const Key &k, usize off) noexcept
  {
    const usize fn = frag.size();
    const usize kn = k.size();
    usize i = 0;
    while ( i < fn && off + i < kn && frag[i] == k[off + i] ) ++i;
    return i;
  }

  static node *
  __find_edge(node *parent, const Key &k, usize off) noexcept
  {
    const auto c0 = k[off];
    for ( node *c = parent->child; c; c = c->sibling )
      if ( c->frag[0] == c0 ) return c;
    return nullptr;
  }

  static void
  __link(node *parent, node *c) noexcept
  {
    c->sibling = parent->child;
    parent->child = c;
  }

  static void
  __unlink(node *parent, node *c) noexcept
  {
    if ( parent->child == c ) {
      parent->child = c->sibling;
      return;
    }
    for ( node *p = parent->child; p; p = p->sibling )
      if ( p->sibling == c ) {
        p->sibling = c->sibling;
        return;
      }
  }

  void
  __destroy_values() noexcept
  {
    if constexpr ( !(micron::is_trivially_destructible_v<Key> && micron::is_trivially_destructible_v<Value>)) {
      node *stack = nullptr;
      if ( __root ) {
        for ( node *c = __root->child; c; ) {
          node *nx = c->sibling;
          c->sibling = stack;
          stack = c;
          c = nx;
        }
      }
      while ( stack ) {
        node *n = stack;
        stack = n->sibling;
        for ( node *c = n->child; c; ) {
          node *nx = c->sibling;
          c->sibling = stack;
          stack = c;
          c = nx;
        }
        n->~node();
      }
      if ( __root ) __root->~node();
    }
  }

  void
  __release_all() noexcept
  {
    __destroy_values();
    __pool.release();
    __root = nullptr;
    __size = 0;
  }

  void
  __ensure_root()
  {
    if ( !__root ) __root = __mk();
  }

  template<class Fn>
  static walk_ctl
  __walk(node *n, Key &acc, Fn &fn)
  {
    for ( node *c = n->child; c; c = c->sibling ) {
      const usize base = acc.size();
      acc += c->frag;
      if ( c->has_value ) {
        if ( micron::__impl::invoke_walk(fn, static_cast<const Key &>(acc), c->value) == walk_ctl::stop ) return walk_ctl::stop;
      }
      if ( __walk(c, acc, fn) == walk_ctl::stop ) return walk_ctl::stop;
      acc = acc.substr(0, base);
    }
    return walk_ctl::continue_;
  }

public:
  radix_tree() noexcept : __pool(), __root(nullptr), __size(0) { }

  template<class Fn>
    requires(micron::is_invocable_v<Fn, radix_tree &>)
  explicit radix_tree(Fn build) : radix_tree()
  {
    build(*this);
  }

  radix_tree(const radix_tree &) = delete;
  radix_tree &operator=(const radix_tree &) = delete;

  radix_tree(radix_tree &&o) noexcept : __pool(micron::move(o.__pool)), __root(o.__root), __size(o.__size)
  {
    o.__root = nullptr;
    o.__size = 0;
  }

  radix_tree &
  operator=(radix_tree &&o) noexcept
  {
    if ( this != &o ) {
      __release_all();
      __pool = micron::move(o.__pool);
      __root = o.__root;
      __size = o.__size;
      o.__root = nullptr;
      o.__size = 0;
    }
    return *this;
  }

  ~radix_tree() { __release_all(); }

  [[nodiscard]] usize
  size() const noexcept
  {
    return __size;
  }

  [[nodiscard]] bool
  empty() const noexcept
  {
    return __size == 0;
  }

  void
  clear() noexcept
  {
    __release_all();
  }

  Value *
  find(const Key &k) noexcept
  {
    if ( !__root || k.size() == 0 ) return nullptr;
    node *cur = __root;
    usize off = 0;
    while ( off < k.size() ) {
      node *e = __find_edge(cur, k, off);
      if ( !e ) return nullptr;
      const usize m = __match_at(e->frag, k, off);
      if ( m != e->frag.size() ) return nullptr;
      off += m;
      if ( off == k.size() ) return e->has_value ? micron::addr(e->value) : nullptr;
      cur = e;
    }
    return nullptr;
  }

  const Value *
  find(const Key &k) const noexcept
  {
    return const_cast<radix_tree *>(this)->find(k);
  }

  [[nodiscard]] bool
  contains(const Key &k) const noexcept
  {
    return const_cast<radix_tree *>(this)->find(k) != nullptr;
  }

  [[nodiscard]] usize
  count(const Key &k) const noexcept
  {
    return contains(k) ? 1u : 0u;
  }

  Value &
  at(const Key &k)
  {
    Value *v = find(k);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("radix_tree::at(): key not found");
    return *v;
  }

  const Value &
  at(const Key &k) const
  {
    const Value *v = find(k);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("radix_tree::at(): key not found");
    return *v;
  }

  Value *
  longest_prefix_match(const Key &k) noexcept
  {
    if ( !__root ) return nullptr;
    node *best = nullptr;
    node *cur = __root;
    usize off = 0;
    while ( off < k.size() ) {
      node *e = __find_edge(cur, k, off);
      if ( !e ) break;
      const usize m = __match_at(e->frag, k, off);
      if ( m != e->frag.size() ) break;
      off += m;
      if ( e->has_value ) best = e;
      if ( off == k.size() ) break;
      cur = e;
    }
    return best ? micron::addr(best->value) : nullptr;
  }

  const Value *
  longest_prefix_match(const Key &k) const noexcept
  {
    return const_cast<radix_tree *>(this)->longest_prefix_match(k);
  }

  template<class VV>
  bool
  insert_or_assign(const Key &k, VV &&v)
  {
    if ( k.size() == 0 ) return false;
    __ensure_root();
    node *cur = __root;
    usize off = 0;

    for ( ;; ) {
      node *e = __find_edge(cur, k, off);
      if ( !e ) {

        node *n = __mk();
        n->frag = k.substr(off);
        n->value = micron::forward<VV>(v);
        n->has_value = true;
        __link(cur, n);
        ++__size;
        return true;
      }

      const usize m = __match_at(e->frag, k, off);
      if ( m == e->frag.size() ) {
        off += m;
        if ( off == k.size() ) {
          const bool fresh = !e->has_value;
          e->value = micron::forward<VV>(v);
          e->has_value = true;
          if ( fresh ) ++__size;
          return fresh;
        }
        cur = e;
        continue;
      }

      node *tail = __mk();
      tail->frag = e->frag.substr(m);
      tail->child = e->child;
      tail->has_value = e->has_value;
      if ( e->has_value ) tail->value = micron::move(e->value);

      e->frag = e->frag.substr(0, m);
      e->child = nullptr;
      e->has_value = false;
      __link(e, tail);

      if ( off + m == k.size() ) {

        e->value = micron::forward<VV>(v);
        e->has_value = true;
      } else {
        node *branch = __mk();
        branch->frag = k.substr(off + m);
        branch->value = micron::forward<VV>(v);
        branch->has_value = true;
        __link(e, branch);
      }
      ++__size;
      return true;
    }
  }

  bool
  insert(const Key &k, const Value &v)
  {
    if ( contains(k) ) return false;
    return insert_or_assign(k, v);
  }

  bool
  insert(const Key &k, Value &&v)
  {
    if ( contains(k) ) return false;
    return insert_or_assign(k, micron::move(v));
  }

  Value &
  operator[](const Key &k)
  {
    Value *p = find(k);
    if ( p ) return *p;
    insert_or_assign(k, Value());
    return *find(k);
  }

  bool
  erase(const Key &k)
  {
    if ( !__root || k.size() == 0 ) return false;

    node *parent = __root;
    node *cur = nullptr;
    usize off = 0;
    node *path_parent[64];
    node *path_node[64];
    usize depth = 0;

    while ( off < k.size() ) {
      node *e = __find_edge(parent, k, off);
      if ( !e ) return false;
      const usize m = __match_at(e->frag, k, off);
      if ( m != e->frag.size() ) return false;
      off += m;
      if ( depth < 64 ) {
        path_parent[depth] = parent;
        path_node[depth] = e;
        ++depth;
      }
      if ( off == k.size() ) {
        cur = e;
        break;
      }
      parent = e;
    }
    if ( !cur || !cur->has_value ) return false;

    cur->has_value = false;
    cur->value = Value();
    --__size;

    while ( depth > 0 ) {
      node *n = path_node[depth - 1];
      node *pp = path_parent[depth - 1];
      if ( n->has_value || n->child ) break;
      __unlink(pp, n);
      __free(n);
      --depth;
    }
    return true;
  }

  void
  swap(radix_tree &o) noexcept
  {
    __pool.swap(o.__pool);
    micron::swap(__root, o.__root);
    micron::swap(__size, o.__size);
  }

  template<class Fn>
  void
  for_each(Fn &&fn)
  {
    if ( !__root ) return;
    Key acc;
    auto wrap = [&](const Key &kk, Value &vv) { fn(kk, vv); };
    __walk(__root, acc, wrap);
  }

  template<class Fn>
  void
  for_each(Fn &&fn) const
  {
    const_cast<radix_tree *>(this)->for_each([&](const Key &kk, Value &vv) { fn(kk, static_cast<const Value &>(vv)); });
  }

  template<class Fn>
  walk_ctl
  traverse(Fn fn) const
  {
    radix_tree *self = const_cast<radix_tree *>(this);
    if ( !self->__root ) return walk_ctl::continue_;
    Key acc;
    return __walk(self->__root, acc, fn);
  }

  [[nodiscard]] usize
  blocks_allocated() const noexcept
  {
    return __pool.blocks();
  }

  [[nodiscard]] usize
  pool_bytes() const noexcept
  {
    return __pool.bytes_held();
  }

  [[nodiscard]] static constexpr usize
  node_bytes() noexcept
  {
    return sizeof(node);
  }
};

template<typename Key, typename Value> using radix = radix_tree<Key, Value>;

};      // namespace micron
