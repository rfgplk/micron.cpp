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
#include "../memory/new.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{

//  pvector
//  persistent(immutable) resizeable vector class
//  difference between pvector and ivector, is that ivector is copies itself for persistence (good for small-ish array, horrible for large
//  ones) this is implemented as a B-ary trie with pathcopying

template <is_movable_object T, usize K = 5, usize H = 3, bool Sf = true>
  requires(K > 0 && K <= 6 && H > 0 && H <= 8)
class pvector
{
  static constexpr usize B = usize(1) << K;
  static constexpr usize __mask = B - 1;

  static constexpr usize
  __cpow(usize base, usize exp)
  {
    usize r = 1;
    for ( usize i = 0; i < exp; ++i ) r *= base;
    return r;
  }

  static_assert(__cpow(B, H) > 0, "pvector: capacity overflow");

  struct __node {
    mutable u32 refs;
    void *children[B];
  };

  struct __leaf {
    mutable u32 refs;
    alignas(alignof(T)) T values[B];
  };

  static inline __leaf *
  __as_leaf(void *p) noexcept
  {
    return static_cast<__leaf *>(p);
  }

  static inline const __leaf *
  __as_leaf(const void *p) noexcept
  {
    return static_cast<const __leaf *>(p);
  }

  static inline __node *
  __as_node(void *p) noexcept
  {
    return static_cast<__node *>(p);
  }

  static inline const __node *
  __as_node(const void *p) noexcept
  {
    return static_cast<const __node *>(p);
  }

  inline bool
  __bounds_check(usize n) const noexcept
  {
    return n >= __size;
  }

  inline bool
  __empty_check(void) const noexcept
  {
    return __size == 0;
  }

  inline bool
  __capacity_check(usize n) const noexcept
  {
    return n >= capacity;
  }

  inline bool
  __range_check(usize from, usize to) const noexcept
  {
    return from >= to || to > __size;
  }

  inline bool
  __insert_pos_check(usize pos) const noexcept
  {
    return pos > __size;
  }

  inline bool
  __full_check(void) const noexcept
  {
    return __size >= capacity;
  }

  inline bool
  __overflow_check(usize add) const noexcept
  {
    return __size + add > capacity;
  }

  template <auto Fn, typename E, typename... Args>
  inline __attribute__((always_inline)) void
  __safety_check(const char *msg, Args &&...args) const
  {
    if constexpr ( Sf ) {
      if ( (this->*Fn)(micron::forward<Args>(args)...) ) exc<E>(msg);
    }
  }

  static inline __node *
  __alloc_internal(void)
  {
    __node *n = reinterpret_cast<__node *>(abc::alloc(sizeof(__node)));
    n->refs = 1;
    for ( usize i = 0; i < B; ++i ) n->children[i] = nullptr;
    return n;
  }

  static inline __leaf *
  __alloc_leaf(void)
  {
    return reinterpret_cast<__leaf *>(abc::alloc(sizeof(__leaf)));
  }

  static inline void
  __dealloc_leaf(__leaf *l)
  {
    if constexpr ( !micron::is_trivially_destructible_v<T> ) {
      for ( usize i = 0; i < B; ++i ) l->values[i].~T();
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
      ++__as_leaf(p)->refs;
    else
      ++__as_node(p)->refs;
    return p;
  }

  template <usize Lvl>
  static void
  __release(void *p)
  {
    if ( !p ) [[unlikely]]
      return;

    if constexpr ( Lvl == 0 ) {
      __leaf *l = __as_leaf(p);
      if ( --l->refs == 0 ) [[unlikely]]
        __dealloc_leaf(l);
    } else {
      __node *n = __as_node(p);
      if ( --n->refs == 0 ) [[unlikely]] {
        for ( usize i = 0; i < B; ++i ) __release<Lvl - 1>(n->children[i]);
        abc::dealloc(reinterpret_cast<byte *>(n));
      }
    }
  }

  template <usize Lvl, typename Vf>
  static void *
  __set_impl(void *p, usize idx, Vf &&val)
  {
    if constexpr ( Lvl == 0 ) {
      const usize slot = idx & __mask;
      __leaf *fresh = __alloc_leaf();
      fresh->refs = 1;

      if ( p ) {
        const __leaf *old = __as_leaf(p);
        if constexpr ( micron::is_trivially_copyable_v<T> )
          micron::bytecpy(reinterpret_cast<byte *>(fresh->values), reinterpret_cast<const byte *>(old->values), B * sizeof(T));
        else
          for ( usize i = 0; i < B; ++i ) new (micron::addr(fresh->values[i])) T(old->values[i]);
      } else {
        for ( usize i = 0; i < B; ++i ) new (micron::addr(fresh->values[i])) T();
      }

      if constexpr ( micron::is_trivially_copyable_v<T> )
        fresh->values[slot] = static_cast<Vf &&>(val);
      else {
        fresh->values[slot].~T();
        new (micron::addr(fresh->values[slot])) T(static_cast<Vf &&>(val));
      }
      return static_cast<void *>(fresh);

    } else {
      const usize slot = (idx >> (Lvl * K)) & __mask;
      __node *fresh = __alloc_internal();

      if ( p ) {
        const __node *old = __as_node(p);
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
      return static_cast<void *>(fresh);
    }
  }

  static inline const T &
  __default_val(void)
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
      return __as_leaf(p)->values[idx & __mask];
    } else {
      const usize slot = (idx >> (Lvl * K)) & __mask;
      return __get_impl<Lvl - 1>(__as_node(p)->children[slot], idx);
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
      return static_cast<void *>(l);
    } else {
      static constexpr usize child_span = __cpow(B, Lvl);
      __node *n = __alloc_internal();
      for ( usize i = 0; i < B; ++i ) n->children[i] = __build_from<Lvl - 1>(data, count, base + i * child_span);
      return static_cast<void *>(n);
    }
  }

  template <usize Lvl>
  static void *
  __build_filled(const T &val, usize count, usize base)
  {
    if ( base >= count ) [[unlikely]]
      return nullptr;

    if constexpr ( Lvl == 0 ) {
      __leaf *l = __alloc_leaf();
      l->refs = 1;
      for ( usize i = 0; i < B; ++i ) {
        if ( base + i < count )
          new (micron::addr(l->values[i])) T(val);
        else
          new (micron::addr(l->values[i])) T();
      }
      return static_cast<void *>(l);
    } else {
      static constexpr usize child_span = __cpow(B, Lvl);
      __node *n = __alloc_internal();
      for ( usize i = 0; i < B; ++i ) n->children[i] = __build_filled<Lvl - 1>(val, count, base + i * child_span);
      return static_cast<void *>(n);
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
        const __leaf *old = __as_leaf(p);
        if constexpr ( micron::is_trivially_copyable_v<T> )
          micron::bytecpy(reinterpret_cast<byte *>(fresh->values), reinterpret_cast<const byte *>(old->values), B * sizeof(T));
        else
          for ( usize i = 0; i < B; ++i ) new (micron::addr(fresh->values[i])) T(old->values[i]);
      } else {
        for ( usize i = 0; i < B; ++i ) new (micron::addr(fresh->values[i])) T();
      }

      T result = fn(static_cast<const T &>(fresh->values[slot]));
      if constexpr ( micron::is_trivially_copyable_v<T> )
        fresh->values[slot] = micron::move(result);
      else {
        fresh->values[slot].~T();
        new (micron::addr(fresh->values[slot])) T(micron::move(result));
      }
      return static_cast<void *>(fresh);

    } else {
      const usize slot = (idx >> (Lvl * K)) & __mask;
      __node *fresh = __alloc_internal();

      if ( p ) {
        const __node *old = __as_node(p);
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
      return static_cast<void *>(fresh);
    }
  }

  template <usize Lvl, typename Fn>
  static void
  __for_each_impl(const void *p, usize base, usize sz, Fn &&fn)
  {
    if ( !p || base >= sz ) return;

    if constexpr ( Lvl == 0 ) {
      const __leaf *l = __as_leaf(p);
      for ( usize i = 0; i < B && base + i < sz; ++i ) fn(base + i, static_cast<const T &>(l->values[i]));
    } else {
      static constexpr usize child_span = __cpow(B, Lvl);
      const __node *n = __as_node(p);
      for ( usize i = 0; i < B; ++i ) {
        usize child_base = base + i * child_span;
        if ( child_base >= sz ) break;
        __for_each_impl<Lvl - 1>(n->children[i], child_base, sz, static_cast<Fn &&>(fn));
      }
    }
  }

  static void
  __sift_down(T *arr, usize n, usize i)
    requires requires(T a, T b) {
      { a < b };
    }
  {
    for ( ;; ) {
      usize largest = i;
      usize l = 2 * i + 1;
      usize r = 2 * i + 2;
      if ( l < n && arr[largest] < arr[l] ) largest = l;
      if ( r < n && arr[largest] < arr[r] ) largest = r;
      if ( largest == i ) break;
      T tmp = micron::move(arr[i]);
      arr[i] = micron::move(arr[largest]);
      arr[largest] = micron::move(tmp);
      i = largest;
    }
  }

  static void
  __heap_sort(T *arr, usize n)
    requires requires(T a, T b) {
      { a < b };
    }
  {
    if ( n < 2 ) return;
    for ( usize i = n / 2; i-- > 0; ) __sift_down(arr, n, i);
    for ( usize i = n - 1; i > 0; --i ) {
      T tmp = micron::move(arr[0]);
      arr[0] = micron::move(arr[i]);
      arr[i] = micron::move(tmp);
      __sift_down(arr, i, 0);
    }
  }

  void *__root;
  usize __size;

  static constexpr usize __root_level = H - 1;

  explicit pvector(void *root, usize sz) : __root(root), __size(sz) {}

public:
  using category_type = vector_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef usize size_type;
  typedef T value_type;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef const T *const_pointer;
  static constexpr size_type capacity = __cpow(B, H);
  static constexpr usize branching = B;
  static constexpr usize height = H;
  static constexpr bool safe = Sf;

  ~pvector(void) { __release<__root_level>(__root); }

  pvector(void) : __root(nullptr), __size(0) {}

  // O(1) copy
  pvector(const pvector &o) : __root(__retain<__root_level>(o.__root)), __size(o.__size) {}

  pvector &
  operator=(const pvector &o)
  {
    if ( this != &o ) [[likely]] {
      __release<__root_level>(__root);
      __root = __retain<__root_level>(o.__root);
      __size = o.__size;
    }
    return *this;
  }

  // O(1) move
  pvector(pvector &&o) noexcept : __root(o.__root), __size(o.__size)
  {
    o.__root = nullptr;
    o.__size = 0;
  }

  pvector &
  operator=(pvector &&o) noexcept
  {
    if ( this != &o ) [[likely]] {
      __release<__root_level>(__root);
      __root = o.__root;
      __size = o.__size;
      o.__root = nullptr;
      o.__size = 0;
    }
    return *this;
  }

  explicit pvector(size_type n) : __root(nullptr), __size(0)
  {
    if ( n > capacity ) exc<except::runtime_error>("micron::pvector size exceeds capacity");
    if ( n == 0 ) return;
    __root = __build_filled<__root_level>(T{}, n, 0);
    __size = n;
  }

  pvector(size_type n, const T &val) : __root(nullptr), __size(0)
  {
    if ( n > capacity ) exc<except::runtime_error>("micron::pvector size exceeds capacity");
    if ( n == 0 ) return;
    __root = __build_filled<__root_level>(val, n, 0);
    __size = n;
  }

  // initializer list
  pvector(const std::initializer_list<T> &&lst) : __root(nullptr), __size(0)
  {
    if ( lst.size() > capacity ) exc<except::runtime_error>("micron::pvector init_list exceeds capacity");
    if ( lst.size() == 0 ) return;
    __root = __build_from<__root_level>(lst.begin(), lst.size(), 0);
    __size = lst.size();
  }

  pvector(const T *data, usize count) : __root(nullptr), __size(0)
  {
    if ( count > capacity ) exc<except::runtime_error>("micron::pvector data exceeds capacity");
    if ( count == 0 || !data ) return;
    __root = __build_from<__root_level>(data, count, 0);
    __size = count;
  }

  pvector(const T &first, usize count) : pvector(micron::addr(first), count) {}

  template <usize N> pvector(const T (&arr)[N]) : __root(nullptr), __size(0)
  {
    static_assert(N <= capacity, "micron::pvector array exceeds capacity");
    if constexpr ( N == 0 ) return;
    __root = __build_from<__root_level>(arr, N, 0);
    __size = N;
  }

  template <typename C>
    requires(is_iterable_container<C> && !micron::is_same_v<micron::remove_cvref_t<C>, pvector>)
  pvector(const C &c) : __root(nullptr), __size(0)
  {
    const usize n = static_cast<usize>(c.size());
    if ( n > capacity ) exc<except::runtime_error>("micron::pvector container exceeds capacity");
    if ( n == 0 ) return;
    if constexpr ( micron::is_same_v<typename C::value_type, T> && requires { c.data(); } ) {
      __root = __build_from<__root_level>(c.data(), n, 0);
    } else {
      // iterate manually
      void *root = nullptr;
      usize i = 0;
      for ( auto it = c.cbegin(); it != c.cend() && i < n; ++it, ++i ) {
        void *next = __set_impl<__root_level>(root, i, static_cast<T>(*it));
        __release<__root_level>(root);
        root = next;
      }
      __root = root;
    }
    __size = n;
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn, usize>)
  explicit pvector(Fn &&fn, usize count) : __root(nullptr), __size(0)
  {
    if ( count > capacity ) exc<except::runtime_error>("micron::pvector generator count exceeds capacity");
    void *root = nullptr;
    for ( usize i = 0; i < count; ++i ) {
      void *next = __set_impl<__root_level>(root, i, fn(i));
      __release<__root_level>(root);
      root = next;
    }
    __root = root;
    __size = count;
  }

  const T &
  at(usize idx) const
  {
    __safety_check<&pvector::__bounds_check, except::runtime_error>("micron::pvector at() out of bounds", idx);
    return __get_impl<__root_level>(__root, idx);
  }

  // single index
  const T &
  operator[](usize idx) const
  {
    return __get_impl<__root_level>(__root, idx);
  }

  [[nodiscard]] pvector
  operator[](usize from, usize to) const
  {
    __safety_check<&pvector::__range_check, except::runtime_error>("micron::pvector operator[] invalid range", from, to);
    const usize cnt = to - from;
    if ( cnt == 0 ) return pvector();
    void *root = nullptr;
    for ( usize i = 0; i < cnt; ++i ) {
      const T &v = __get_impl<__root_level>(__root, from + i);
      void *next = __set_impl<__root_level>(root, i, v);
      __release<__root_level>(root);
      root = next;
    }
    return pvector(root, cnt);
  }

  const T &
  get(usize idx) const
  {
    return __get_impl<__root_level>(__root, idx);
  }

  const T &
  front(void) const
  {
    __safety_check<&pvector::__empty_check, except::runtime_error>("micron::pvector front() on empty vector");
    return __get_impl<__root_level>(__root, 0);
  }

  const T &
  back(void) const
  {
    __safety_check<&pvector::__empty_check, except::runtime_error>("micron::pvector back() on empty vector");
    return __get_impl<__root_level>(__root, __size - 1);
  }

  size_type
  size(void) const noexcept
  {
    return __size;
  }

  size_type
  max_size() const noexcept
  {
    return capacity;
  }

  bool
  empty(void) const noexcept
  {
    return __size == 0;
  }

  const void *
  identity(void) const noexcept
  {
    return __root;
  }

  bool
  operator!(void) const noexcept
  {
    return empty();
  }

  [[nodiscard]] pvector
  push_back(const T &val) const
  {
    __safety_check<&pvector::__full_check, except::runtime_error>("micron::pvector push_back() capacity exceeded");
    void *nr = __set_impl<__root_level>(__root, __size, val);
    return pvector(nr, __size + 1);
  }

  [[nodiscard]] pvector
  push_back(T &&val) const
  {
    __safety_check<&pvector::__full_check, except::runtime_error>("micron::pvector push_back() capacity exceeded");
    void *nr = __set_impl<__root_level>(__root, __size, micron::move(val));
    return pvector(nr, __size + 1);
  }

  template <typename... Args>
  [[nodiscard]] pvector
  emplace_back(Args &&...args) const
  {
    __safety_check<&pvector::__full_check, except::runtime_error>("micron::pvector emplace_back() capacity exceeded");
    T val(micron::forward<Args>(args)...);
    void *nr = __set_impl<__root_level>(__root, __size, micron::move(val));
    return pvector(nr, __size + 1);
  }

  [[nodiscard]] pvector
  pop_back(void) const
  {
    __safety_check<&pvector::__empty_check, except::runtime_error>("micron::pvector pop_back() on empty vector");
    void *nr = __set_impl<__root_level>(__root, __size - 1, T{});
    return pvector(nr, __size - 1);
  }

  [[nodiscard]] pvector
  set(usize idx, const T &val) const
  {
    __safety_check<&pvector::__bounds_check, except::runtime_error>("micron::pvector set() out of bounds", idx);
    void *nr = __set_impl<__root_level>(__root, idx, val);
    return pvector(nr, __size);
  }

  [[nodiscard]] pvector
  set(usize idx, T &&val) const
  {
    __safety_check<&pvector::__bounds_check, except::runtime_error>("micron::pvector set() out of bounds", idx);
    void *nr = __set_impl<__root_level>(__root, idx, micron::move(val));
    return pvector(nr, __size);
  }

  template <typename Fn>
  [[nodiscard]] pvector
  update(usize idx, Fn &&fn) const
  {
    __safety_check<&pvector::__bounds_check, except::runtime_error>("micron::pvector update() out of bounds", idx);
    void *nr = __update_impl<__root_level>(__root, idx, static_cast<Fn &&>(fn));
    return pvector(nr, __size);
  }

  [[nodiscard]] pvector
  insert(usize pos, const T &val) const
  {
    __safety_check<&pvector::__full_check, except::runtime_error>("micron::pvector insert() capacity exceeded");
    __safety_check<&pvector::__insert_pos_check, except::runtime_error>("micron::pvector insert() position out of bounds", pos);

    void *root = __retain<__root_level>(__root);
    for ( usize i = __size; i > pos; --i ) {
      const T &v = __get_impl<__root_level>(root, i - 1);
      void *next = __set_impl<__root_level>(root, i, v);
      __release<__root_level>(root);
      root = next;
    }
    void *final_root = __set_impl<__root_level>(root, pos, val);
    __release<__root_level>(root);
    return pvector(final_root, __size + 1);
  }

  [[nodiscard]] pvector
  insert(usize pos, T &&val) const
  {
    __safety_check<&pvector::__full_check, except::runtime_error>("micron::pvector insert() capacity exceeded");
    __safety_check<&pvector::__insert_pos_check, except::runtime_error>("micron::pvector insert() position out of bounds", pos);

    void *root = __retain<__root_level>(__root);
    for ( usize i = __size; i > pos; --i ) {
      const T &v = __get_impl<__root_level>(root, i - 1);
      void *next = __set_impl<__root_level>(root, i, v);
      __release<__root_level>(root);
      root = next;
    }
    void *final_root = __set_impl<__root_level>(root, pos, micron::move(val));
    __release<__root_level>(root);
    return pvector(final_root, __size + 1);
  }

  [[nodiscard]] pvector
  insert(usize pos, const T &val, usize cnt) const
  {
    __safety_check<&pvector::__overflow_check, except::runtime_error>("micron::pvector insert() capacity exceeded", cnt);
    __safety_check<&pvector::__insert_pos_check, except::runtime_error>("micron::pvector insert() position out of bounds", pos);

    void *root = __retain<__root_level>(__root);
    for ( usize i = __size + cnt - 1; i >= pos + cnt; --i ) {
      const T &v = __get_impl<__root_level>(root, i - cnt);
      void *next = __set_impl<__root_level>(root, i, v);
      __release<__root_level>(root);
      root = next;
    }
    for ( usize i = pos; i < pos + cnt; ++i ) {
      void *next = __set_impl<__root_level>(root, i, val);
      __release<__root_level>(root);
      root = next;
    }
    return pvector(root, __size + cnt);
  }

  [[nodiscard]] pvector
  erase(usize pos) const
  {
    __safety_check<&pvector::__bounds_check, except::runtime_error>("micron::pvector erase() out of bounds", pos);

    void *root = __retain<__root_level>(__root);
    for ( usize i = pos; i < __size - 1; ++i ) {
      const T &v = __get_impl<__root_level>(root, i + 1);
      void *next = __set_impl<__root_level>(root, i, v);
      __release<__root_level>(root);
      root = next;
    }
    void *final_root = __set_impl<__root_level>(root, __size - 1, T{});
    __release<__root_level>(root);
    return pvector(final_root, __size - 1);
  }

  [[nodiscard]] pvector
  erase(usize from, usize to) const
  {
    __safety_check<&pvector::__range_check, except::runtime_error>("micron::pvector erase() invalid range", from, to);

    const usize cnt = to - from;
    void *root = __retain<__root_level>(__root);
    for ( usize i = from; i < __size - cnt; ++i ) {
      const T &v = __get_impl<__root_level>(root, i + cnt);
      void *next = __set_impl<__root_level>(root, i, v);
      __release<__root_level>(root);
      root = next;
    }
    for ( usize i = __size - cnt; i < __size; ++i ) {
      void *next = __set_impl<__root_level>(root, i, T{});
      __release<__root_level>(root);
      root = next;
    }
    return pvector(root, __size - cnt);
  }

  [[nodiscard]] pvector
  clear(void) const
  {
    return pvector();
  }

  [[nodiscard]] pvector
  resize(usize n) const
  {
    if ( n > capacity ) [[unlikely]]
      exc<except::runtime_error>("micron::pvector resize() exceeds capacity");
    if ( n == __size ) return pvector(*this);

    void *root = __retain<__root_level>(__root);
    if ( n < __size ) {
      for ( usize i = n; i < __size; ++i ) {
        void *next = __set_impl<__root_level>(root, i, T{});
        __release<__root_level>(root);
        root = next;
      }
    } else {
      for ( usize i = __size; i < n; ++i ) {
        void *next = __set_impl<__root_level>(root, i, T{});
        __release<__root_level>(root);
        root = next;
      }
    }
    return pvector(root, n);
  }

  [[nodiscard]] pvector
  resize(usize n, const T &val) const
  {
    if ( n > capacity ) [[unlikely]]
      exc<except::runtime_error>("micron::pvector resize() exceeds capacity");
    if ( n == __size ) return pvector(*this);

    void *root = __retain<__root_level>(__root);
    if ( n < __size ) {
      for ( usize i = n; i < __size; ++i ) {
        void *next = __set_impl<__root_level>(root, i, T{});
        __release<__root_level>(root);
        root = next;
      }
    } else {
      for ( usize i = __size; i < n; ++i ) {
        void *next = __set_impl<__root_level>(root, i, val);
        __release<__root_level>(root);
        root = next;
      }
    }
    return pvector(root, n);
  }

  [[nodiscard]] pvector
  fill(const T &val) const
  {
    return pvector(__size, val);
  }

  [[nodiscard]] pvector
  assign(usize count, const T &val) const
  {
    return pvector(count, val);
  }

  [[nodiscard]] pvector
  clone(void) const
  {
    return pvector(*this);
  }

  [[nodiscard]] pvector
  append(const pvector &o) const
  {
    __safety_check<&pvector::__overflow_check, except::runtime_error>("micron::pvector append() exceeds capacity", o.__size);

    void *root = __retain<__root_level>(__root);
    for ( usize i = 0; i < o.__size; ++i ) {
      const T &v = o.get(i);
      void *next = __set_impl<__root_level>(root, __size + i, v);
      __release<__root_level>(root);
      root = next;
    }
    return pvector(root, __size + o.__size);
  }

  [[nodiscard]] pvector
  append(const T *data, usize count) const
  {
    __safety_check<&pvector::__overflow_check, except::runtime_error>("micron::pvector append() exceeds capacity", count);

    void *root = __retain<__root_level>(__root);
    for ( usize i = 0; i < count; ++i ) {
      void *next = __set_impl<__root_level>(root, __size + i, data[i]);
      __release<__root_level>(root);
      root = next;
    }
    return pvector(root, __size + count);
  }

  [[nodiscard]] pvector
  append(const T &first, usize count) const
  {
    return append(&first, count);
  }

  [[nodiscard]] pvector
  remove(const T &val) const
  {
    void *root = nullptr;
    usize new_size = 0;
    for ( usize i = 0; i < __size; ++i ) {
      const T &v = get(i);
      if ( !(v == val) ) {
        void *next = __set_impl<__root_level>(root, new_size, v);
        __release<__root_level>(root);
        root = next;
        ++new_size;
      }
    }
    return pvector(root, new_size);
  }

  [[nodiscard]] pvector
  operator+(const pvector &o) const
  {
    return append(o);
  }

  [[nodiscard]] pvector
  operator+(const T &scalar) const
    requires requires(T a, T b) {
      { a + b };
    }
  {
    void *root = nullptr;
    for ( usize i = 0; i < __size; ++i ) {
      T val = get(i) + scalar;
      void *next = __set_impl<__root_level>(root, i, micron::move(val));
      __release<__root_level>(root);
      root = next;
    }
    return pvector(root, __size);
  }

  [[nodiscard]] pvector
  operator-(const T &scalar) const
    requires requires(T a, T b) {
      { a - b };
    }
  {
    void *root = nullptr;
    for ( usize i = 0; i < __size; ++i ) {
      T val = get(i) - scalar;
      void *next = __set_impl<__root_level>(root, i, micron::move(val));
      __release<__root_level>(root);
      root = next;
    }
    return pvector(root, __size);
  }

  [[nodiscard]] pvector
  operator*(const T &scalar) const
    requires requires(T a, T b) {
      { a * b };
    }
  {
    void *root = nullptr;
    for ( usize i = 0; i < __size; ++i ) {
      T val = get(i) * scalar;
      void *next = __set_impl<__root_level>(root, i, micron::move(val));
      __release<__root_level>(root);
      root = next;
    }
    return pvector(root, __size);
  }

  [[nodiscard]] pvector
  operator/(const T &scalar) const
    requires requires(T a, T b) {
      { a / b };
    }
  {
    void *root = nullptr;
    for ( usize i = 0; i < __size; ++i ) {
      T val = get(i) / scalar;
      void *next = __set_impl<__root_level>(root, i, micron::move(val));
      __release<__root_level>(root);
      root = next;
    }
    return pvector(root, __size);
  }

  [[nodiscard]] pvector
  operator%(const T &scalar) const
    requires requires(T a, T b) {
      { a % b };
    }
  {
    void *root = nullptr;
    for ( usize i = 0; i < __size; ++i ) {
      T val = get(i) % scalar;
      void *next = __set_impl<__root_level>(root, i, micron::move(val));
      __release<__root_level>(root);
      root = next;
    }
    return pvector(root, __size);
  }

  [[nodiscard]] pvector
  add(const T &n) const
    requires requires(T a, T b) {
      { a + b };
    }
  {
    return operator+(n);
  }

  [[nodiscard]] pvector
  sub(const T &n) const
    requires requires(T a, T b) {
      { a - b };
    }
  {
    return operator-(n);
  }

  [[nodiscard]] pvector
  mul(const T &n) const
    requires requires(T a, T b) {
      { a * b };
    }
  {
    return operator*(n);
  }

  [[nodiscard]] pvector
  div(const T &n) const
    requires requires(T a, T b) {
      { a / b };
    }
  {
    return operator/(n);
  }

  T
  sum(void) const
    requires requires(T a, T b) {
      { a += b };
    }
  {
    T s{};
    for_each([&s](usize, const T &v) { s += v; });
    return s;
  }

  T
  product(void) const
    requires requires(T a, T b) {
      { a *= b };
    }
  {
    if ( __size == 0 ) return T{};
    T m = get(0);
    for ( usize i = 1; i < __size; ++i ) m *= get(i);
    return m;
  }

  T
  difference(void) const
    requires requires(T a, T b) {
      { a -= b };
    }
  {
    if ( __size == 0 ) return T{};
    T d = get(0);
    for ( usize i = 1; i < __size; ++i ) d -= get(i);
    return d;
  }

  T
  quotient(void) const
    requires requires(T a, T b) {
      { a /= b };
    }
  {
    if ( __size == 0 ) return T{};
    T q = get(0);
    for ( usize i = 1; i < __size; ++i ) q /= get(i);
    return q;
  }

  T
  min(void) const
    requires requires(T a, T b) {
      { a < b } -> micron::convertible_to<bool>;
    }
  {
    __safety_check<&pvector::__empty_check, except::runtime_error>("micron::pvector min() on empty vector");
    T m = get(0);
    for ( usize i = 1; i < __size; ++i ) {
      const T &v = get(i);
      if ( v < m ) m = v;
    }
    return m;
  }

  T
  max(void) const
    requires requires(T a, T b) {
      { a < b } -> micron::convertible_to<bool>;
    }
  {
    __safety_check<&pvector::__empty_check, except::runtime_error>("micron::pvector max() on empty vector");
    T m = get(0);
    for ( usize i = 1; i < __size; ++i ) {
      const T &v = get(i);
      if ( m < v ) m = v;
    }
    return m;
  }

  template <typename Fn>
  T
  reduce(Fn &&fn) const
  {
    __safety_check<&pvector::__empty_check, except::runtime_error>("micron::pvector reduce() on empty vector");
    T acc = get(0);
    for ( usize i = 1; i < __size; ++i ) acc = fn(acc, get(i));
    return acc;
  }

  template <typename Fn>
  T
  reduce(const T &init, Fn &&fn) const
  {
    T acc = init;
    for ( usize i = 0; i < __size; ++i ) acc = fn(acc, get(i));
    return acc;
  }

  bool
  all(const T &o) const
  {
    for ( usize i = 0; i < __size; ++i )
      if ( !(get(i) == o) ) return false;
    return true;
  }

  bool
  any(const T &o) const
  {
    for ( usize i = 0; i < __size; ++i )
      if ( get(i) == o ) return true;
    return false;
  }

  template <typename Fn>
  bool
  all_of(Fn &&pred) const
  {
    for ( usize i = 0; i < __size; ++i )
      if ( !pred(get(i)) ) return false;
    return true;
  }

  template <typename Fn>
  bool
  any_of(Fn &&pred) const
  {
    for ( usize i = 0; i < __size; ++i )
      if ( pred(get(i)) ) return true;
    return false;
  }

  template <typename Fn>
  bool
  none_of(Fn &&pred) const
  {
    return !any_of(static_cast<Fn &&>(pred));
  }

  [[nodiscard]] pvector
  sort(void) const
    requires requires(T a, T b) {
      { a < b };
    }
  {
    if ( __size < 2 ) return pvector(*this);

    byte *buf = reinterpret_cast<byte *>(abc::alloc(__size * sizeof(T)));
    T *arr = reinterpret_cast<T *>(buf);
    for ( usize i = 0; i < __size; ++i ) new (micron::addr(arr[i])) T(get(i));

    __heap_sort(arr, __size);

    void *root = __build_from<__root_level>(arr, __size, 0);

    if constexpr ( !micron::is_trivially_destructible_v<T> ) {
      for ( usize i = 0; i < __size; ++i ) arr[i].~T();
    }
    abc::dealloc(buf);

    return pvector(root, __size);
  }

  bool
  contains(const T &val) const
  {
    for ( usize i = 0; i < __size; ++i )
      if ( get(i) == val ) return true;
    return false;
  }

  bool
  operator==(const pvector &o) const
  {
    if ( __size != o.__size ) return false;
    if ( __root == o.__root ) return true;
    for ( usize i = 0; i < __size; ++i )
      if ( !(get(i) == o.get(i)) ) return false;
    return true;
  }

  bool
  operator!=(const pvector &o) const
  {
    return !(*this == o);
  }

  template <typename Fn>
  void
  for_each(Fn &&fn) const
  {
    __for_each_impl<__root_level>(__root, 0, __size, static_cast<Fn &&>(fn));
  }

  template <typename Fn>
  [[nodiscard]] pvector
  map(Fn &&fn) const
  {
    void *root = nullptr;
    for ( usize i = 0; i < __size; ++i ) {
      T val = fn(get(i));
      void *next = __set_impl<__root_level>(root, i, micron::move(val));
      __release<__root_level>(root);
      root = next;
    }
    return pvector(root, __size);
  }

  template <typename Fn>
  [[nodiscard]] pvector
  filter(Fn &&pred) const
  {
    void *root = nullptr;
    usize new_size = 0;
    for ( usize i = 0; i < __size; ++i ) {
      const T &v = get(i);
      if ( pred(v) ) {
        void *next = __set_impl<__root_level>(root, new_size, v);
        __release<__root_level>(root);
        root = next;
        ++new_size;
      }
    }
    return pvector(root, new_size);
  }

  class const_iterator
  {
    const pvector *__owner;
    usize __idx;

  public:
    const_iterator(void) : __owner(nullptr), __idx(0) {}

    const_iterator(const pvector *p, usize i) : __owner(p), __idx(i) {}

    const T &
    operator*(void) const
    {
      return __owner->get(__idx);
    }

    const T *
    operator->(void) const
    {
      return &__owner->get(__idx);
    }

    const_iterator &
    operator++(void)
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
    operator--(void)
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
    index(void) const
    {
      return __idx;
    }
  };

  using iterator = const_iterator;

  const_iterator
  begin(void) const
  {
    return const_iterator(this, 0);
  }

  const_iterator
  end(void) const
  {
    return const_iterator(this, __size);
  }

  const_iterator
  cbegin(void) const
  {
    return const_iterator(this, 0);
  }

  const_iterator
  cend(void) const
  {
    return const_iterator(this, __size);
  }

  const_iterator
  last(void) const
  {
    __safety_check<&pvector::__empty_check, except::runtime_error>("micron::pvector last() on empty vector");
    return const_iterator(this, __size - 1);
  }

  const_iterator
  find(const T &val) const
  {
    for ( usize i = 0; i < __size; ++i )
      if ( get(i) == val ) return const_iterator(this, i);
    return end();
  }

  void
  swap(pvector &o) noexcept
  {
    void *tmp_root = __root;
    __root = o.__root;
    o.__root = tmp_root;

    usize tmp_size = __size;
    __size = o.__size;
    o.__size = tmp_size;
  }

  static constexpr bool
  is_pod(void)
  {
    return micron::is_pod_v<T>;
  }

  static constexpr bool
  is_class(void)
  {
    return micron::is_class_v<T>;
  }

  static constexpr bool
  is_trivial(void) noexcept
  {
    return micron::is_trivial_v<T>;
  }
};

};     // namespace micron
