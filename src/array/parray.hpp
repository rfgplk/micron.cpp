//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../bits/__container.hpp"
#include "../concepts.hpp"
#include "../except.hpp"
#include "../memory/addr.hpp"
#include "../memory/allocation/resources.hpp"
#include "../memory/memory.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{

//  parray persistent/immutable fixed-capacity array
//  implemented as a Bary trie with structural sharing

template <is_movable_object T, usize K = 5, usize H = 3>
  requires(K > 0 && K <= 6 && H > 0 && H <= 8)
class parray
{
  static constexpr usize B = static_cast<usize>(1) << K;
  static constexpr usize __mask = B - 1;

  static constexpr usize
  __cpow(usize base, usize exp)
  {
    usize r = 1;
    for ( usize i = 0; i < exp; ++i )
      r *= base;
    return r;
  }

  // verify capacity fits in usize
  static_assert(__cpow(B, H) > 0, "parray: capacity overflow");

  struct __node {
    mutable u32 refs;
    void *children[B];
  };

  struct __leaf {
    mutable u32 refs;
    alignas(alignof(T)) T values[B];
  };

  static inline __node *
  __alloc_internal()
  {
    auto *n = reinterpret_cast<__node *>(abc::alloc(sizeof(__node)));
    n->refs = 1;
    for ( usize i = 0; i < B; ++i )
      n->children[i] = nullptr;
    return n;
  }

  static inline __leaf *
  __alloc_leaf()
  {
    return reinterpret_cast<__leaf *>(abc::alloc(sizeof(__leaf)));
  }

  static inline __leaf *
  __make_default_leaf()
  {
    __leaf *l = __alloc_leaf();
    l->refs = 1;
    for ( usize i = 0; i < B; ++i )
      new (micron::addr(l->values[i])) T();
    return l;
  }

  static inline void
  __dealloc_leaf(__leaf *l)
  {
    if constexpr ( !micron::is_trivially_destructible_v<T> ) {
      for ( usize i = 0; i < B; ++i )
        l->values[i].~T();
    }
    abc::dealloc(reinterpret_cast<byte *>(l));
  }

  template <usize Lvl>
  static inline void *
  __retain(void *p)
  {
    if ( !p ) [[unlikely]]
      return nullptr;
    if constexpr ( Lvl == 0 )
      ++static_cast<__leaf *>(p)->refs;
    else
      ++static_cast<__node *>(p)->refs;
    return p;
  }

  template <usize Lvl>
  static void
  __release(void *p)
  {
    if ( !p ) [[unlikely]]
      return;

    if constexpr ( Lvl == 0 ) {
      __leaf *l = static_cast<__leaf *>(p);
      if ( --l->refs == 0 ) [[unlikely]]
        __dealloc_leaf(l);
    } else {
      __node *n = static_cast<__node *>(p);
      if ( --n->refs == 0 ) [[unlikely]] {
        for ( usize i = 0; i < B; ++i )
          __release<Lvl - 1>(n->children[i]);
        abc::dealloc(reinterpret_cast<byte *>(n));
      }
    }
  }

  template <usize Lvl, typename Vf>
  static void *
  __set_impl(void *p, usize idx, Vf &&val)
  {
    if constexpr ( Lvl == 0 ) {
      // leaf level
      const usize slot = idx & __mask;
      __leaf *fresh = __alloc_leaf();
      fresh->refs = 1;

      if ( p ) {
        const __leaf *old = static_cast<const __leaf *>(p);
        if constexpr ( micron::is_trivially_copyable_v<T> )
          micron::bytecpy(reinterpret_cast<byte *>(fresh->values), reinterpret_cast<const byte *>(old->values), B * sizeof(T));
        else
          for ( usize i = 0; i < B; ++i )
            new (micron::addr(fresh->values[i])) T(old->values[i]);
      } else {
        for ( usize i = 0; i < B; ++i )
          new (micron::addr(fresh->values[i])) T();
      }

      if constexpr ( micron::is_trivially_copyable_v<T> )
        fresh->values[slot] = static_cast<Vf &&>(val);
      else {
        fresh->values[slot].~T();
        new (micron::addr(fresh->values[slot])) T(static_cast<Vf &&>(val));
      }
      return fresh;

    } else {
      // internal level
      const usize slot = (idx >> (Lvl * K)) & __mask;
      __node *fresh = reinterpret_cast<__node *>(abc::alloc(sizeof(__node)));
      fresh->refs = 1;

      if ( p ) {
        const __node *old = static_cast<const __node *>(p);
        for ( usize i = 0; i < B; ++i ) {
          if ( i == slot )
            fresh->children[i] = __set_impl<Lvl - 1>(old->children[i], idx, static_cast<Vf &&>(val));
          else
            fresh->children[i] = __retain<Lvl - 1>(old->children[i]);
        }
      } else {
        for ( usize i = 0; i < B; ++i ) {
          if ( i == slot )
            fresh->children[i] = __set_impl<Lvl - 1>(nullptr, idx, static_cast<Vf &&>(val));
          else
            fresh->children[i] = nullptr;
        }
      }
      return fresh;
    }
  }

  static inline const T &
  __default_val()
  {
    static const T __d{};
    return __d;
  }

  template <usize Lvl>
  static inline const T &
  __get_impl(const void *p, usize idx)
  {
    if ( !p ) [[unlikely]]
      return __default_val();

    if constexpr ( Lvl == 0 ) {
      return static_cast<const __leaf *>(p)->values[idx & __mask];
    } else {
      const usize slot = (idx >> (Lvl * K)) & __mask;
      return __get_impl<Lvl - 1>(static_cast<const __node *>(p)->children[slot], idx);
    }
  }

  template <usize Lvl>
  static void *
  __build_from(const T *data, usize count, usize base)
  {
    if ( base >= count ) [[unlikely]]
      return nullptr;

    if constexpr ( Lvl == 0 ) {
      __leaf *l = __alloc_leaf();
      l->refs = 1;
      for ( usize i = 0; i < B; ++i ) {
        if ( base + i < count )
          new (micron::addr(l->values[i])) T(data[base + i]);
        else
          new (micron::addr(l->values[i])) T();
      }
      return l;
    } else {
      static constexpr usize child_span = __cpow(B, Lvl);
      __node *n = reinterpret_cast<__node *>(abc::alloc(sizeof(__node)));
      n->refs = 1;
      for ( usize i = 0; i < B; ++i )
        n->children[i] = __build_from<Lvl - 1>(data, count, base + i * child_span);
      return n;
    }
  }

  template <usize Lvl>
  static void *
  __build_filled(const T &val)
  {
    if constexpr ( Lvl == 0 ) {
      __leaf *l = __alloc_leaf();
      l->refs = 1;
      for ( usize i = 0; i < B; ++i )
        new (micron::addr(l->values[i])) T(val);
      return l;
    } else {
      void *child = __build_filled<Lvl - 1>(val);
      // child is shared by all B slots
      if constexpr ( Lvl - 1 == 0 )
        static_cast<__leaf *>(child)->refs = B;
      else
        static_cast<__node *>(child)->refs = B;

      __node *n = reinterpret_cast<__node *>(abc::alloc(sizeof(__node)));
      n->refs = 1;
      for ( usize i = 0; i < B; ++i )
        n->children[i] = child;
      return n;
    }
  }

  template <usize Lvl, typename Fn>
  static void *
  __update_impl(void *p, usize idx, Fn &&fn)
  {
    if constexpr ( Lvl == 0 ) {
      const usize slot = idx & __mask;
      __leaf *fresh = __alloc_leaf();
      fresh->refs = 1;

      if ( p ) {
        const __leaf *old = static_cast<const __leaf *>(p);
        if constexpr ( micron::is_trivially_copyable_v<T> )
          micron::bytecpy(reinterpret_cast<byte *>(fresh->values), reinterpret_cast<const byte *>(old->values), B * sizeof(T));
        else
          for ( usize i = 0; i < B; ++i )
            new (micron::addr(fresh->values[i])) T(old->values[i]);
      } else {
        for ( usize i = 0; i < B; ++i )
          new (micron::addr(fresh->values[i])) T();
      }

      T result = fn(static_cast<const T &>(fresh->values[slot]));
      if constexpr ( micron::is_trivially_copyable_v<T> )
        fresh->values[slot] = micron::move(result);
      else {
        fresh->values[slot].~T();
        new (micron::addr(fresh->values[slot])) T(micron::move(result));
      }
      return fresh;

    } else {
      const usize slot = (idx >> (Lvl * K)) & __mask;
      __node *fresh = reinterpret_cast<__node *>(abc::alloc(sizeof(__node)));
      fresh->refs = 1;

      if ( p ) {
        const __node *old = static_cast<const __node *>(p);
        for ( usize i = 0; i < B; ++i ) {
          if ( i == slot )
            fresh->children[i] = __update_impl<Lvl - 1>(old->children[i], idx, static_cast<Fn &&>(fn));
          else
            fresh->children[i] = __retain<Lvl - 1>(old->children[i]);
        }
      } else {
        for ( usize i = 0; i < B; ++i ) {
          if ( i == slot )
            fresh->children[i] = __update_impl<Lvl - 1>(nullptr, idx, static_cast<Fn &&>(fn));
          else
            fresh->children[i] = nullptr;
        }
      }
      return fresh;
    }
  }

  template <usize Lvl, typename Fn>
  static void
  __for_each_impl(const void *p, usize base, Fn &&fn)
  {
    if ( !p )
      return;

    if constexpr ( Lvl == 0 ) {
      const __leaf *l = static_cast<const __leaf *>(p);
      for ( usize i = 0; i < B; ++i )
        fn(base + i, static_cast<const T &>(l->values[i]));
    } else {
      static constexpr usize child_span = __cpow(B, Lvl);
      const __node *n = static_cast<const __node *>(p);
      for ( usize i = 0; i < B; ++i )
        __for_each_impl<Lvl - 1>(n->children[i], base + i * child_span, static_cast<Fn &&>(fn));
    }
  }

  template <usize Lvl>
  static bool
  __equal_impl(const void *a, const void *b)
  {
    if ( a == b ) [[unlikely]]
      return true;

    // both null == equal
    if ( !a && !b ) [[unlikely]]
      return true;

    // one null, one valid
    if ( !a || !b ) {
      const void *live = a ? a : b;
      if constexpr ( Lvl == 0 ) {
        const __leaf *l = static_cast<const __leaf *>(live);
        const T d{};
        for ( usize i = 0; i < B; ++i )
          if ( !(l->values[i] == d) )
            return false;
        return true;
      } else {
        const __node *n = static_cast<const __node *>(live);
        for ( usize i = 0; i < B; ++i )
          if ( !__equal_impl<Lvl - 1>(n->children[i], nullptr) )
            return false;
        return true;
      }
    }

    if constexpr ( Lvl == 0 ) {
      const __leaf *la = static_cast<const __leaf *>(a);
      const __leaf *lb = static_cast<const __leaf *>(b);
      if constexpr ( micron::is_trivially_copyable_v<T> ) {
        return micron::bytecmp(reinterpret_cast<const byte *>(la->values), reinterpret_cast<const byte *>(lb->values), B * sizeof(T)) == 0;
      } else {
        for ( usize i = 0; i < B; ++i )
          if ( !(la->values[i] == lb->values[i]) )
            return false;
        return true;
      }
    } else {
      const __node *na = static_cast<const __node *>(a);
      const __node *nb = static_cast<const __node *>(b);
      for ( usize i = 0; i < B; ++i )
        if ( !__equal_impl<Lvl - 1>(na->children[i], nb->children[i]) )
          return false;
      return true;
    }
  }

  template <typename Fn>
  parray
  __apply_scalar(const T &scalar, Fn &&fn) const
  {
    void *nr = __apply_scalar_impl<__root_level>(__root, scalar, static_cast<Fn &&>(fn));
    return parray(nr);
  }

  template <usize Lvl, typename Fn>
  static void *
  __apply_scalar_impl(const void *p, const T &scalar, Fn &&fn)
  {
    if constexpr ( Lvl == 0 ) {
      __leaf *fresh = __alloc_leaf();
      fresh->refs = 1;
      if ( p ) {
        const __leaf *old = static_cast<const __leaf *>(p);
        for ( usize i = 0; i < B; ++i )
          new (micron::addr(fresh->values[i])) T(fn(old->values[i], scalar));
      } else {
        const T d{};
        for ( usize i = 0; i < B; ++i )
          new (micron::addr(fresh->values[i])) T(fn(d, scalar));
      }
      return fresh;
    } else {
      __node *fresh = reinterpret_cast<__node *>(abc::alloc(sizeof(__node)));
      fresh->refs = 1;
      if ( p ) {
        const __node *old = static_cast<const __node *>(p);
        for ( usize i = 0; i < B; ++i )
          fresh->children[i] = __apply_scalar_impl<Lvl - 1>(old->children[i], scalar, fn);
      } else {
        for ( usize i = 0; i < B; ++i )
          fresh->children[i] = __apply_scalar_impl<Lvl - 1>(nullptr, scalar, fn);
      }
      return fresh;
    }
  }

  void *__root;

  static constexpr usize __root_level = H - 1;

  explicit parray(void *root) : __root(root) {}

public:
  using category_type = array_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef usize size_type;
  typedef T value_type;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef const T *const_pointer;
  static constexpr size_type length = __cpow(B, H);
  static constexpr size_type static_size = length;
  static constexpr usize branching = B;
  static constexpr usize height = H;

  ~parray() { __release<__root_level>(__root); }

  parray() : __root(nullptr) {}

  // O(1) copy
  parray(const parray &o) : __root(__retain<__root_level>(o.__root)) {}

  parray &
  operator=(const parray &o)
  {
    if ( this != &o ) [[likely]] {
      __release<__root_level>(__root);
      __root = __retain<__root_level>(o.__root);
    }
    return *this;
  }

  // O(1) move
  parray(parray &&o) noexcept : __root(o.__root) { o.__root = nullptr; }

  parray &
  operator=(parray &&o) noexcept
  {
    if ( this != &o ) [[likely]] {
      __release<__root_level>(__root);
      __root = o.__root;
      o.__root = nullptr;
    }
    return *this;
  }

  // broadcast fill
  explicit parray(const T &val) : __root(__build_filled<__root_level>(val)) {}

  // initializer list
  parray(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > length )
      exc<except::runtime_error>("micron::parray init_list too large for capacity");
    if ( lst.size() == 0 ) {
      __root = nullptr;
      return;
    }
    __root = __build_from<__root_level>(lst.begin(), lst.size(), 0);
  }

  // build from contiguous range
  parray(const T *data, usize count)
  {
    if ( count > length )
      exc<except::runtime_error>("micron::parray data exceeds capacity");
    if ( count == 0 ) {
      __root = nullptr;
      return;
    }
    __root = __build_from<__root_level>(data, count, 0);
  }

  // generator constructor
  template <typename Fn>
    requires(micron::is_invocable_v<Fn, usize>)
  explicit parray(Fn &&fn, usize count)
  {
    if ( count > length )
      exc<except::runtime_error>("micron::parray generator count exceeds capacity");
    // build incrementally
    void *root = nullptr;
    for ( usize i = 0; i < count; ++i ) {
      void *next = __set_impl<__root_level>(root, i, fn(i));
      __release<__root_level>(root);
      root = next;
    }
    __root = root;
  }

  [[nodiscard]] parray
  set(usize idx, const T &val) const
  {
    if ( idx >= length ) [[unlikely]]
      exc<except::runtime_error>("micron::parray set() out of range");
    void *nr = __set_impl<__root_level>(__root, idx, val);
    return parray(nr);
  }

  [[nodiscard]] parray
  set(usize idx, T &&val) const
  {
    if ( idx >= length ) [[unlikely]]
      exc<except::runtime_error>("micron::parray set() out of range");
    void *nr = __set_impl<__root_level>(__root, idx, micron::move(val));
    return parray(nr);
  }

  template <typename Fn>
  [[nodiscard]] parray
  update(usize idx, Fn &&fn) const
  {
    if ( idx >= length ) [[unlikely]]
      exc<except::runtime_error>("micron::parray update() out of range");
    void *nr = __update_impl<__root_level>(__root, idx, static_cast<Fn &&>(fn));
    return parray(nr);
  }

  [[nodiscard]] parray
  clear() const
  {
    return parray();
  }

  [[nodiscard]] parray
  fill(const T &val) const
  {
    return parray(val);
  }

  template <typename... Ps>
  [[nodiscard]] parray
  set_many(Ps &&...pairs) const
  {
    void *root = __retain<__root_level>(__root);
    // fold: for each pair, apply set and release old root
    auto apply_one = [&root](auto &&pair) {
      void *next = __set_impl<__root_level>(root, pair.a, static_cast<decltype(pair.b) &&>(pair.b));
      __release<__root_level>(root);
      root = next;
    };
    (apply_one(static_cast<Ps &&>(pairs)), ...);
    return parray(root);
  }

  const T &
  get(usize idx) const
  {
    return __get_impl<__root_level>(__root, idx);
  }

  const T &
  at(usize idx) const
  {
    if ( idx >= length ) [[unlikely]]
      exc<except::runtime_error>("micron::parray at() out of range");
    return __get_impl<__root_level>(__root, idx);
  }

  const T &
  operator[](usize idx) const
  {
    return __get_impl<__root_level>(__root, idx);
  }

  size_type
  size() const noexcept
  {
    return length;
  }

  size_type
  max_size() const noexcept
  {
    return length;
  }

  bool
  empty() const noexcept
  {
    return false;     // always has length elements, never empty
  }

  // O(1), true iff structurally identical root
  const void *
  identity() const noexcept
  {
    return __root;
  }

  bool
  operator==(const parray &o) const
  {
    return __equal_impl<__root_level>(__root, o.__root);
  }

  bool
  operator!=(const parray &o) const
  {
    return !(*this == o);
  }

  parray
  operator+(const parray &o) const
  {
    parray arr(*this);
    o.for_each([&arr](usize idx, const T &val) {
      T sum = arr.get(idx) + val;
      parray next = arr.set(idx, micron::move(sum));
      arr = micron::move(next);
    });
    return arr;
  }

  parray
  operator-(const parray &o) const
  {
    parray arr(*this);
    o.for_each([&arr](usize idx, const T &val) {
      T diff = arr.get(idx) - val;
      parray next = arr.set(idx, micron::move(diff));
      arr = micron::move(next);
    });
    return arr;
  }

  parray
  operator*(const parray &o) const
  {
    parray arr(*this);
    o.for_each([&arr](usize idx, const T &val) {
      T prod = arr.get(idx) * val;
      parray next = arr.set(idx, micron::move(prod));
      arr = micron::move(next);
    });
    return arr;
  }

  parray
  operator/(const parray &o) const
  {
    parray arr(*this);
    o.for_each([&arr](usize idx, const T &val) {
      T quot = arr.get(idx) / val;
      parray next = arr.set(idx, micron::move(quot));
      arr = micron::move(next);
    });
    return arr;
  }

  parray
  operator%(const parray &o) const
  {
    parray arr(*this);
    o.for_each([&arr](usize idx, const T &val) {
      T rem = arr.get(idx) % val;
      parray next = arr.set(idx, micron::move(rem));
      arr = micron::move(next);
    });
    return arr;
  }

  // compound scalar
  parray
  operator+=(const T &o) const
  {
    return __apply_scalar(o, [](const T &a, const T &b) { return a + b; });
  }

  parray
  operator-=(const T &o) const
  {
    return __apply_scalar(o, [](const T &a, const T &b) { return a - b; });
  }

  parray
  operator*=(const T &o) const
  {
    return __apply_scalar(o, [](const T &a, const T &b) { return a * b; });
  }

  parray
  operator/=(const T &o) const
  {
    return __apply_scalar(o, [](const T &a, const T &b) { return a / b; });
  }

  parray
  operator%=(const T &o) const
  {
    return __apply_scalar(o, [](const T &a, const T &b) { return a % b; });
  }

  // compound parray
  parray
  operator+=(const parray &o) const
  {
    return *this + o;
  }

  parray
  operator-=(const parray &o) const
  {
    return *this - o;
  }

  parray
  operator*=(const parray &o) const
  {
    return *this * o;
  }

  parray
  operator/=(const parray &o) const
  {
    return *this / o;
  }

  parray
  operator%=(const parray &o) const
  {
    return *this % o;
  }

  [[nodiscard]] parray
  add(const size_type n) const
  {
    return operator+=(static_cast<T>(n));
  }

  [[nodiscard]] parray
  sub(const size_type n) const
  {
    return operator-=(static_cast<T>(n));
  }

  [[nodiscard]] parray
  mul(const size_type n) const
  {
    return operator*=(static_cast<T>(n));
  }

  [[nodiscard]] parray
  div(const size_type n) const
  {
    return operator/=(static_cast<T>(n));
  }

  T
  sum() const
  {
    T s{};
    for_each([&s](usize, const T &v) { s += v; });
    return s;
  }

  T
  mul_reduce() const
  {
    T m = get(0);
    for_each([&m, first = true](usize, const T &v) mutable {
      if ( first ) {
        first = false;
        return;
      }
      m *= v;
    });
    return m;
  }

  bool
  all(const T &o) const
  {
    bool result = true;
    for_each([&result, &o](usize, const T &v) {
      if ( !(v == o) )
        result = false;
    });
    return result;
  }

  bool
  any(const T &o) const
  {
    bool result = false;
    for_each([&result, &o](usize, const T &v) {
      if ( v == o )
        result = true;
    });
    return result;
  }

  // visits only valid leaves
  template <typename Fn>
  void
  for_each(Fn &&fn) const
  {
    __for_each_impl<__root_level>(__root, 0, static_cast<Fn &&>(fn));
  }

  class const_iterator
  {
    const parray *__owner;
    usize __idx;

  public:
    const_iterator() : __owner(nullptr), __idx(0) {}

    const_iterator(const parray *p, usize i) : __owner(p), __idx(i) {}

    const T &
    operator*() const
    {
      return __owner->get(__idx);
    }

    const T *
    operator->() const
    {
      return &__owner->get(__idx);
    }

    const_iterator &
    operator++()
    {
      ++__idx;
      return *this;
    }

    const_iterator
    operator++(int)
    {
      const_iterator tmp = *this;
      ++__idx;
      return tmp;
    }

    const_iterator &
    operator--()
    {
      --__idx;
      return *this;
    }

    const_iterator
    operator--(int)
    {
      const_iterator tmp = *this;
      --__idx;
      return tmp;
    }

    bool
    operator==(const const_iterator &o) const
    {
      return __idx == o.__idx;
    }

    bool
    operator!=(const const_iterator &o) const
    {
      return __idx != o.__idx;
    }

    usize
    index() const
    {
      return __idx;
    }
  };

  using iterator = const_iterator;

  const_iterator
  begin() const
  {
    return const_iterator(this, 0);
  }

  const_iterator
  end() const
  {
    return const_iterator(this, length);
  }

  const_iterator
  cbegin() const
  {
    return const_iterator(this, 0);
  }

  const_iterator
  cend() const
  {
    return const_iterator(this, length);
  }

  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<T>;
  }

  static constexpr bool
  is_class()
  {
    return micron::is_class_v<T>;
  }

  static constexpr bool
  is_trivial() noexcept
  {
    return micron::is_trivial_v<T>;
  }
};

};     // namespace micron
