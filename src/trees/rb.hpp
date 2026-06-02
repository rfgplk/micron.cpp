//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "../memory/new.hpp"
#include "../tags.hpp"
#include "../types.hpp"
#include "../vector/fvector.hpp"
#include "__tree_walk.hpp"

namespace micron
{

enum class RBColor : i32 { RED, BLACK };

template<typename T>
  requires micron::is_copy_constructible_v<T> && micron::is_move_constructible_v<T>
class rb_node
{
public:
  enum class kind_t { empty, leaf, inner };

  T data;
  RBColor color;
  rb_node *parent;
  rb_node *left;
  rb_node *right;
  kind_t kind;

  rb_node() = delete;

  explicit rb_node(const T &dt) noexcept(noexcept(T(dt)))
      : data(dt), color(RBColor::RED), parent(nullptr), left(nullptr), right(nullptr), kind(kind_t::leaf)
  {
  }

  explicit rb_node(T &&dt) noexcept(noexcept(T(micron::move(dt))))
      : data(micron::move(dt)), color(RBColor::RED), parent(nullptr), left(nullptr), right(nullptr), kind(kind_t::leaf)
  {
  }

  rb_node(const rb_node &o) = default;

  rb_node(rb_node &&o) noexcept(noexcept(T(micron::move(o.data))))
      : data(micron::move(o.data)), color(o.color), parent(o.parent), left(o.left), right(o.right), kind(o.kind)
  {
    o.parent = nullptr;
    o.left = nullptr;
    o.right = nullptr;
    o.kind = kind_t::empty;
  }

  rb_node &operator=(const rb_node &o) = default;

  rb_node &
  operator=(rb_node &&o) noexcept(noexcept(data = micron::move(o.data)))
  {
    if ( this != &o ) {
      data = micron::move(o.data);
      color = o.color;
      parent = o.parent;
      left = o.left;
      right = o.right;
      kind = o.kind;
      o.parent = nullptr;
      o.left = nullptr;
      o.right = nullptr;
      o.kind = kind_t::empty;
    }
    return *this;
  }

  ~rb_node() = default;

  bool
  is_leaf() const noexcept
  {
    return kind == kind_t::leaf;
  }

  bool
  is_inner() const noexcept
  {
    return kind == kind_t::inner;
  }

  bool
  is_empty() const noexcept
  {
    return kind == kind_t::empty;
  }

  rb_node *
  grandparent() const noexcept
  {
    return parent ? parent->parent : nullptr;
  }

  rb_node *
  uncle() const noexcept
  {
    if ( !parent || !parent->parent ) return nullptr;
    return parent == parent->parent->left ? parent->parent->right : parent->parent->left;
  }

  rb_node *
  sibling() const noexcept
  {
    if ( !parent ) return nullptr;
    return this == parent->left ? parent->right : parent->left;
  }

  void
  set_left(rb_node *l) noexcept
  {
    left = l;
    if ( l ) {
      l->parent = this;
      kind = kind_t::inner;
    }
  }

  void
  set_right(rb_node *r) noexcept
  {
    right = r;
    if ( r ) {
      r->parent = this;
      kind = kind_t::inner;
    }
  }

  void
  set_color(RBColor c) noexcept
  {
    color = c;
  }

  RBColor
  get_color() const noexcept
  {
    return color;
  }

  T &
  value() noexcept
  {
    return data;
  }

  const T &
  value() const noexcept
  {
    return data;
  }

  void
  detach() noexcept
  {
    parent = left = right = nullptr;
    kind = kind_t::empty;
  }
};

template<typename T> struct default_less {
  static bool
  lt(const T &a, const T &b)
  {
    return a < b;
  }
};

template<typename T, typename Less = default_less<T>>
  requires micron::is_copy_constructible_v<T> && micron::is_move_constructible_v<T>
class rb_tree
{
public:
  using node = rb_node<T>;
  using category_type = tree_tag;
  using value_type = T;

private:
  node *root_;
  usize size_;

  template<class... Args>
  static node *
  make_node(Args &&...args)
  {
    return new node(T(micron::forward<Args>(args)...));
  }

  static void
  destroy_node(node *p)
  {
    delete p;
  }

  static node *
  minimum(node *x)
  {
    while ( x && x->left ) x = x->left;
    return x;
  }

  static node *
  maximum(node *x)
  {
    while ( x && x->right ) x = x->right;
    return x;
  }

  void
  rotate_left(node *x) noexcept
  {
    node *y = x->right;
    x->right = y->left;
    if ( y->left ) y->left->parent = x;
    y->parent = x->parent;
    if ( !x->parent )
      root_ = y;
    else if ( x == x->parent->left )
      x->parent->left = y;
    else
      x->parent->right = y;
    y->left = x;
    x->parent = y;
  }

  void
  rotate_right(node *x) noexcept
  {
    node *y = x->left;
    x->left = y->right;
    if ( y->right ) y->right->parent = x;
    y->parent = x->parent;
    if ( !x->parent )
      root_ = y;
    else if ( x == x->parent->right )
      x->parent->right = y;
    else
      x->parent->left = y;
    y->right = x;
    x->parent = y;
  }

  void
  insert_fixup(node *z) noexcept
  {
    while ( z->parent && z->parent->color == RBColor::RED ) {
      if ( z->parent == z->parent->parent->left ) {
        node *y = z->parent->parent->right;
        if ( y && y->color == RBColor::RED ) {
          z->parent->color = RBColor::BLACK;
          y->color = RBColor::BLACK;
          z->parent->parent->color = RBColor::RED;
          z = z->parent->parent;
        } else {
          if ( z == z->parent->right ) {
            z = z->parent;
            rotate_left(z);
          }
          z->parent->color = RBColor::BLACK;
          z->parent->parent->color = RBColor::RED;
          rotate_right(z->parent->parent);
        }
      } else {
        node *y = z->parent->parent->left;
        if ( y && y->color == RBColor::RED ) {
          z->parent->color = RBColor::BLACK;
          y->color = RBColor::BLACK;
          z->parent->parent->color = RBColor::RED;
          z = z->parent->parent;
        } else {
          if ( z == z->parent->left ) {
            z = z->parent;
            rotate_right(z);
          }
          z->parent->color = RBColor::BLACK;
          z->parent->parent->color = RBColor::RED;
          rotate_left(z->parent->parent);
        }
      }
    }
    root_->color = RBColor::BLACK;
  }

  void
  transplant(node *u, node *v) noexcept
  {
    if ( !u->parent )
      root_ = v;
    else if ( u == u->parent->left )
      u->parent->left = v;
    else
      u->parent->right = v;
    if ( v ) v->parent = u->parent;
  }

  void
  erase_fixup(node *x, node *x_parent) noexcept
  {
    while ( (x != root_) && (!x || x->color == RBColor::BLACK) ) {
      if ( x == (x_parent ? x_parent->left : nullptr) ) {
        node *w = x_parent ? x_parent->right : nullptr;
        if ( w && w->color == RBColor::RED ) {
          w->color = RBColor::BLACK;
          x_parent->color = RBColor::RED;
          rotate_left(x_parent);
          w = x_parent->right;
        }
        if ( (!w || (!w->left || w->left->color == RBColor::BLACK)) && (!w || (!w->right || w->right->color == RBColor::BLACK)) ) {
          if ( w ) w->color = RBColor::RED;
          x = x_parent;
          x_parent = x ? x->parent : nullptr;
        } else {
          if ( !w || !w->right || w->right->color == RBColor::BLACK ) {
            if ( w && w->left ) w->left->color = RBColor::BLACK;
            if ( w ) w->color = RBColor::RED;
            if ( x_parent ) rotate_right(w);
            w = x_parent ? x_parent->right : nullptr;
          }
          if ( w ) w->color = x_parent ? x_parent->color : RBColor::BLACK;
          if ( x_parent ) x_parent->color = RBColor::BLACK;
          if ( w && w->right ) w->right->color = RBColor::BLACK;
          if ( x_parent ) rotate_left(x_parent);
          x = root_;
          x_parent = nullptr;
        }
      } else {
        node *w = x_parent ? x_parent->left : nullptr;
        if ( w && w->color == RBColor::RED ) {
          w->color = RBColor::BLACK;
          x_parent->color = RBColor::RED;
          rotate_right(x_parent);
          w = x_parent->left;
        }
        if ( (!w || (!w->right || w->right->color == RBColor::BLACK)) && (!w || (!w->left || w->left->color == RBColor::BLACK)) ) {
          if ( w ) w->color = RBColor::RED;
          x = x_parent;
          x_parent = x ? x->parent : nullptr;
        } else {
          if ( !w || !w->left || w->left->color == RBColor::BLACK ) {
            if ( w && w->right ) w->right->color = RBColor::BLACK;
            if ( w ) w->color = RBColor::RED;
            if ( x_parent ) rotate_left(w);
            w = x_parent ? x_parent->left : nullptr;
          }
          if ( w ) w->color = x_parent ? x_parent->color : RBColor::BLACK;
          if ( x_parent ) x_parent->color = RBColor::BLACK;
          if ( w && w->left ) w->left->color = RBColor::BLACK;
          if ( x_parent ) rotate_right(x_parent);
          x = root_;
          x_parent = nullptr;
        }
      }
    }
    if ( x ) x->color = RBColor::BLACK;
  }

  node *
  find_node(const T &key) const noexcept
  {
    node *cur = root_;
    while ( cur ) {
      if ( Less::lt(key, cur->data) )
        cur = cur->left;
      else if ( Less::lt(cur->data, key) )
        cur = cur->right;
      else
        return cur;
    }
    return nullptr;
  }

  static node *
  clone_subtree(node *src, node *parent)
  {
    if ( !src ) return nullptr;
    node *n = new node(src->data);
    n->color = src->color;
    n->parent = parent;
    n->kind = src->kind;
    n->left = clone_subtree(src->left, n);
    n->right = clone_subtree(src->right, n);
    return n;
  }

  static void
  destroy_subtree(node *x)
  {
    while ( x ) {
      if ( x->left ) {
        node *l = x->left;
        x->left = l->right;
        l->right = x;
        x = l;
      } else {
        node *r = x->right;
        destroy_node(x);
        x = r;
      }
    }
  }

  void
  __erase_node(node *z) noexcept
  {
    node *y = z;
    RBColor y_original_color = y->color;
    node *x = nullptr;
    node *x_parent = nullptr;

    if ( !z->left ) {
      x = z->right;
      x_parent = z->parent;
      transplant(z, z->right);
    } else if ( !z->right ) {
      x = z->left;
      x_parent = z->parent;
      transplant(z, z->left);
    } else {
      y = minimum(z->right);
      y_original_color = y->color;
      x = y->right;
      if ( y->parent == z ) {
        if ( x ) x->parent = y;
        x_parent = y;
      } else {
        transplant(y, y->right);
        y->right = z->right;
        if ( y->right ) y->right->parent = y;
        x_parent = y->parent;
      }
      transplant(z, y);
      y->left = z->left;
      if ( y->left ) y->left->parent = y;
      y->color = z->color;
    }

    destroy_node(z);
    --size_;

    if ( y_original_color == RBColor::BLACK ) erase_fixup(x, x_parent);
  }

  template<class NodeP, class Fn>
  static void
  __for_each_inorder(NodeP x, Fn &fn)
  {
    if ( !x ) return;
    __for_each_inorder(x->left, fn);
    fn(x->data);
    __for_each_inorder(x->right, fn);
  }

  template<class Acc, class NodeFn>
  static Acc
  __cata_rec(const node *x, const Acc &empty, NodeFn &node)
  {
    if ( !x ) return empty;
    Acc l = __cata_rec(x->left, empty, node);
    Acc r = __cata_rec(x->right, empty, node);
    return node(micron::move(l), x->data, micron::move(r));
  }

  template<traversal_order O, class Fn>
  static walk_ctl
  __traverse_rec(const node *x, Fn &fn)
  {
    if ( !x ) return walk_ctl::continue_;
    if constexpr ( O == traversal_order::preorder ) {
      walk_ctl c = micron::__impl::invoke_walk(fn, x->data);
      if ( c == walk_ctl::stop ) return walk_ctl::stop;
      if ( c == walk_ctl::skip_children ) return walk_ctl::continue_;
      if ( __traverse_rec<O>(x->left, fn) == walk_ctl::stop ) return walk_ctl::stop;
      return __traverse_rec<O>(x->right, fn);
    } else if constexpr ( O == traversal_order::postorder ) {
      if ( __traverse_rec<O>(x->left, fn) == walk_ctl::stop ) return walk_ctl::stop;
      if ( __traverse_rec<O>(x->right, fn) == walk_ctl::stop ) return walk_ctl::stop;
      return micron::__impl::invoke_walk(fn, x->data) == walk_ctl::stop ? walk_ctl::stop : walk_ctl::continue_;
    } else {      // inorder
      if ( __traverse_rec<O>(x->left, fn) == walk_ctl::stop ) return walk_ctl::stop;
      if ( micron::__impl::invoke_walk(fn, x->data) == walk_ctl::stop ) return walk_ctl::stop;
      return __traverse_rec<O>(x->right, fn);
    }
  }

  template<class Fn>
  static walk_ctl
  __traverse_level(const node *root, Fn &fn)
  {
    if ( !root ) return walk_ctl::continue_;
    micron::fvector<const node *> q;
    q.push_back(root);
    for ( usize head = 0; head < q.size(); ++head ) {
      const node *x = q[head];
      walk_ctl c = micron::__impl::invoke_walk(fn, x->data);
      if ( c == walk_ctl::stop ) return walk_ctl::stop;
      if ( c != walk_ctl::skip_children ) {
        if ( x->left ) q.push_back(x->left);
        if ( x->right ) q.push_back(x->right);
      }
    }
    return walk_ctl::continue_;
  }

public:
  rb_tree() : root_(nullptr), size_(0) { }

  template<class Fn>
    requires(micron::is_invocable_v<Fn, usize>)
  rb_tree(usize n, Fn gen) : root_(nullptr), size_(0)
  {
    for ( usize i = 0; i < n; ++i ) insert_or_assign(gen(i));
  }

  template<class Fn>
    requires(micron::is_invocable_v<Fn> && !micron::is_invocable_v<Fn, usize>)
  rb_tree(usize n, Fn gen) : root_(nullptr), size_(0)
  {
    for ( usize i = 0; i < n; ++i ) insert_or_assign(gen());
  }

  template<class Fn>
    requires(micron::is_invocable_v<Fn, rb_tree &>)
  explicit rb_tree(Fn build) : root_(nullptr), size_(0)
  {
    build(*this);
  }

  rb_tree(const rb_tree &o) : root_(clone_subtree(o.root_, nullptr)), size_(o.size_) { }

  rb_tree(rb_tree &&o) noexcept : root_(o.root_), size_(o.size_)
  {
    o.root_ = nullptr;
    o.size_ = 0;
  }

  rb_tree &
  operator=(const rb_tree &o)
  {
    if ( this != &o ) {
      destroy_subtree(root_);
      root_ = clone_subtree(o.root_, nullptr);
      size_ = o.size_;
    }
    return *this;
  }

  rb_tree &
  operator=(rb_tree &&o) noexcept
  {
    if ( this != &o ) {
      destroy_subtree(root_);
      root_ = o.root_;
      size_ = o.size_;
      o.root_ = nullptr;
      o.size_ = 0;
    }
    return *this;
  }

  template<class Fn>
    requires(micron::is_invocable_v<Fn, usize>)
  void
  assign(usize n, Fn gen)
  {
    clear();
    for ( usize i = 0; i < n; ++i ) insert_or_assign(gen(i));
  }

  template<class Fn>
    requires(micron::is_invocable_v<Fn> && !micron::is_invocable_v<Fn, usize>)
  void
  assign(usize n, Fn gen)
  {
    clear();
    for ( usize i = 0; i < n; ++i ) insert_or_assign(gen());
  }

  template<class Fn>
    requires(micron::is_invocable_v<Fn, rb_tree &>)
  void
  assign(Fn build)
  {
    clear();
    build(*this);
  }

  ~rb_tree() { destroy_subtree(root_); }

  bool
  empty() const noexcept
  {
    return size_ == 0;
  }

  usize
  size() const noexcept
  {
    return size_;
  }

  T *
  find(const T &key) noexcept
  {
    node *n = find_node(key);
    return n ? &n->data : nullptr;
  }

  const T *
  find(const T &key) const noexcept
  {
    node *n = find_node(key);
    return n ? &n->data : nullptr;
  }

  bool
  contains(const T &key) const noexcept
  {
    return find_node(key) != nullptr;
  }

  T *
  min() noexcept
  {
    node *m = minimum(root_);
    return m ? &m->data : nullptr;
  }

  const T *
  min() const noexcept
  {
    node *m = minimum(root_);
    return m ? &m->data : nullptr;
  }

  T *
  max() noexcept
  {
    node *m = maximum(root_);
    return m ? &m->data : nullptr;
  }

  const T *
  max() const noexcept
  {
    node *m = maximum(root_);
    return m ? &m->data : nullptr;
  }

  template<class U>
  T &
  insert_or_assign(U &&value)
  {
    node *y = nullptr;
    node *x = root_;
    while ( x ) {
      y = x;
      if ( Less::lt(value, x->data) )
        x = x->left;
      else if ( Less::lt(x->data, value) )
        x = x->right;
      else {
        x->data = micron::forward<U>(value);
        return x->data;
      }
    }
    node *z = make_node(micron::forward<U>(value));
    z->parent = y;
    z->left = nullptr;
    z->right = nullptr;
    z->color = RBColor::RED;
    z->kind = node::kind_t::leaf;
    if ( !y )
      root_ = z;
    else if ( Less::lt(z->data, y->data) )
      y->left = z;
    else
      y->right = z;
    insert_fixup(z);
    ++size_;
    return z->data;
  }

  T &
  insert(const T &v)
  {
    return insert_or_assign(v);
  }

  T &
  insert(T &&v)
  {
    return insert_or_assign(micron::move(v));
  }

  template<class Fn>
  T &
  update(const T &key, Fn fn)
  {
    const T *cur = find(key);
    return insert_or_assign(fn(cur));
  }

  template<class... Args>
  T &
  emplace(Args &&...args)
  {
    T key(micron::forward<Args>(args)...);
    node *y = nullptr;
    node *x = root_;
    while ( x ) {
      y = x;
      if ( Less::lt(key, x->data) )
        x = x->left;
      else if ( Less::lt(x->data, key) )
        x = x->right;
      else {
        x->data = micron::move(key);
        return x->data;
      }
    }
    node *z = make_node(micron::move(key));
    z->parent = y;
    z->left = nullptr;
    z->right = nullptr;
    z->color = RBColor::RED;
    z->kind = node::kind_t::leaf;
    if ( !y )
      root_ = z;
    else if ( Less::lt(z->data, y->data) )
      y->left = z;
    else
      y->right = z;
    insert_fixup(z);
    ++size_;
    return z->data;
  }

  bool
  erase(const T &key)
  {
    node *z = find_node(key);
    if ( !z ) return false;
    __erase_node(z);
    return true;
  }

  T
  extract_min()
  {
    node *z = minimum(root_);
    if ( !z ) [[unlikely]]
      exc<except::library_error>("rb_tree::extract_min(): empty tree");
    T data = micron::move(z->data);
    __erase_node(z);
    return data;
  }

  template<class Fn>
  void
  for_each(Fn &&fn)
  {
    __for_each_inorder(root_, fn);
  }

  template<class Fn>
  void
  for_each(Fn &&fn) const
  {
    __for_each_inorder(root_, fn);
  }

  template<class Fn>
  auto
  map(Fn fn) const
  {
    using U = micron::remove_cvref_t<micron::invoke_result_t<Fn, const T &>>;
    rb_tree<U> out;
    for_each([&](const T &e) { out.insert(fn(e)); });
    return out;
  }

  template<class Acc, class NodeFn>
  Acc
  cata(const Acc &empty, NodeFn node) const
  {
    return __cata_rec(root_, empty, node);
  }

  template<traversal_order O = traversal_order::inorder, class Fn>
  walk_ctl
  traverse(Fn fn) const
  {
    if constexpr ( O == traversal_order::level )
      return __traverse_level(root_, fn);
    else
      return __traverse_rec<O>(root_, fn);
  }

  void
  clear()
  {
    destroy_subtree(root_);
    root_ = nullptr;
    size_ = 0;
  }
};
};      // namespace micron
