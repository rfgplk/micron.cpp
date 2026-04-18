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
#include "../memory/new.hpp"

#include "../except.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../algorithm/memory.hpp"
#include "../slice.hpp"

#include "unitypes.hpp"

namespace micron
{

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// rope is a persistent immutable string implemented as a balanced binary tree (rope, get it?) with refcounted nodes
//
// leaf nodes hold contiguous character segments (up to __max_leaf bytes)
// internal nodes store total subtree length

constexpr const char __rope_null_str[1] = "";
constexpr const wide __rope_null_wstr[1] = L"";
constexpr const unicode32 __rope_null_u32str[1] = U"";

template <is_scalar_literal T = schar, bool Sf = true> class rope
{
  // maximum size of each leaf
  static constexpr usize __max_leaf = 256;

  //  64-bit
  //    [0]   left        (nullptr for leaf)
  //    [8]   right       (nullptr for leaf)
  //    [16]  refs
  //    [20]  pad
  //    [24]  __length    (total chars in subtree)
  //    [32]  T data[]    (leaf only, variable length)

  struct __node {
    __node *left;
    __node *right;
    mutable u32 refs;
    usize __length;
  };

  static_assert(alignof(__node) >= alignof(T), "node alignment must satisfy T alignment");

  static inline __attribute__((always_inline)) bool
  __is_leaf(const __node *n) noexcept
  {
    return n->left == nullptr;
  }

  static inline __attribute__((always_inline)) T *
  __leaf_data(__node *n) noexcept
  {
    return reinterpret_cast<T *>(reinterpret_cast<byte *>(n) + sizeof(__node));
  }

  static inline __attribute__((always_inline)) const T *
  __leaf_data(const __node *n) noexcept
  {
    return reinterpret_cast<const T *>(reinterpret_cast<const byte *>(n) + sizeof(__node));
  }

  static inline __node *
  __make_leaf(const T *data, usize len)
  {
    auto *n = reinterpret_cast<__node *>(abc::alloc(sizeof(__node) + len * sizeof(T)));
    n->left = nullptr;
    n->right = nullptr;
    n->refs = 1;
    n->__length = len;
    if ( len ) micron::memcpy(__leaf_data(n), data, len);
    return n;
  }

  static inline __node *
  __make_leaf_fill(T ch, usize len)
  {
    auto *n = reinterpret_cast<__node *>(abc::alloc(sizeof(__node) + len * sizeof(T)));
    n->left = nullptr;
    n->right = nullptr;
    n->refs = 1;
    n->__length = len;
    micron::typeset<T>(__leaf_data(n), ch, len);
    return n;
  }

  static inline __node *
  __make_leaf_concat(const __node *a, const __node *b)
  {
    usize total = a->__length + b->__length;
    auto *n = reinterpret_cast<__node *>(abc::alloc(sizeof(__node) + total * sizeof(T)));
    n->left = nullptr;
    n->right = nullptr;
    n->refs = 1;
    n->__length = total;
    micron::memcpy(__leaf_data(n), __leaf_data(a), a->__length);
    micron::memcpy(__leaf_data(n) + a->__length, __leaf_data(b), b->__length);
    return n;
  }

  static inline __node *
  __make_leaf_push(const __node *src, T ch)
  {
    usize newlen = src->__length + 1;
    auto *n = reinterpret_cast<__node *>(abc::alloc(sizeof(__node) + newlen * sizeof(T)));
    n->left = nullptr;
    n->right = nullptr;
    n->refs = 1;
    n->__length = newlen;
    micron::memcpy(__leaf_data(n), __leaf_data(src), src->__length);
    __leaf_data(n)[src->__length] = ch;
    return n;
  }

  static inline __node *
  __make_branch(__node *l, __node *r)
  {
    auto *n = reinterpret_cast<__node *>(abc::alloc(sizeof(__node)));
    n->left = l;
    n->right = r;
    n->refs = 1;
    n->__length = (l ? l->__length : 0) + (r ? r->__length : 0);
    return n;
  }

  static inline __attribute__((always_inline)) __node *
  __retain(__node *n) noexcept
  {
    if ( n ) [[likely]]
      ++n->refs;
    return n;
  }

  static inline void
  __release(__node *n) noexcept
  {
    constexpr usize __stack_cap = 64;
    __node *stack[__stack_cap];
    usize depth = 0;

    while ( n ) [[likely]] {
      if ( --n->refs != 0 ) [[likely]]
        break;
      if ( __is_leaf(n) ) {
        abc::dealloc(reinterpret_cast<byte *>(n));
        break;
      }
      __node *l = n->left;
      __node *r = n->right;
      abc::dealloc(reinterpret_cast<byte *>(n));
      if ( l ) {
        if ( depth < __stack_cap ) [[likely]]
          stack[depth++] = l;
        else
          __release(l);
      }
      n = r;
    }
    while ( depth > 0 ) __release(stack[--depth]);
  }

  // construct a balanced tree from contiguous data
  static __node *
  __build_balanced(const T *data, usize len)
  {
    if ( len == 0 ) return nullptr;
    if ( len <= __max_leaf ) return __make_leaf(data, len);
    usize mid = len / 2;
    __node *left = __build_balanced(data, mid);
    __node *right = __build_balanced(data + mid, len - mid);
    return __make_branch(left, right);
  }

  // construct a balanced tree filled with ch
  static __node *
  __build_fill(T ch, usize len)
  {
    if ( len == 0 ) return nullptr;
    if ( len <= __max_leaf ) return __make_leaf_fill(ch, len);
    usize mid = len / 2;
    __node *left = __build_fill(ch, mid);
    __node *right = __build_fill(ch, len - mid);
    return __make_branch(left, right);
  }

  static __node *
  __concat(__node *l, __node *r)
  {
    if ( !l ) return r;
    if ( !r ) return l;

    // merge small leaves to prevent degenerate single-char chains
    if ( __is_leaf(l) && __is_leaf(r) && l->__length + r->__length <= __max_leaf ) {
      __node *merged = __make_leaf_concat(l, r);
      __release(l);
      __release(r);
      return merged;
    }

    return __make_branch(l, r);
  }

  // NOTE : does _NOT_ consume n
  struct __split_result {
    __node *left;
    __node *right;
  };

  static __split_result
  __split(const __node *n, usize idx)
  {
    if ( !n ) return { nullptr, nullptr };
    if ( idx == 0 ) return { nullptr, __retain(const_cast<__node *>(n)) };
    if ( idx >= n->__length ) return { __retain(const_cast<__node *>(n)), nullptr };

    if ( __is_leaf(n) ) {
      const T *data = __leaf_data(n);
      __node *left = __make_leaf(data, idx);
      __node *right = __make_leaf(data + idx, n->__length - idx);
      return { left, right };
    }

    usize left_len = n->left->__length;

    if ( idx < left_len ) {
      auto [ll, lr] = __split(n->left, idx);
      __node *new_right = __concat(lr, __retain(n->right));
      return { ll, new_right };
    } else if ( idx > left_len ) {
      auto [rl, rr] = __split(n->right, idx - left_len);
      __node *new_left = __concat(__retain(n->left), rl);
      return { new_left, rr };
    } else {
      return { __retain(n->left), __retain(n->right) };
    }
  }

  static inline const T &
  __char_at(const __node *n, usize idx)
  {
    while ( !__is_leaf(n) ) {
      usize left_len = n->left->__length;
      if ( idx < left_len )
        n = n->left;
      else {
        idx -= left_len;
        n = n->right;
      }
    }
    return __leaf_data(n)[idx];
  }

  // pathcopy the right spine, extend or append the rightmost leaf
  static __node *
  __push_back_impl(const __node *n, T ch)
  {
    if ( !n ) return __make_leaf(&ch, 1);

    if ( __is_leaf(n) ) {
      if ( n->__length < __max_leaf ) return __make_leaf_push(n, ch);
      __node *tail = __make_leaf(&ch, 1);
      return __make_branch(__retain(const_cast<__node *>(n)), tail);
    }

    // recurse on right, share left
    __node *new_right = __push_back_impl(n->right, ch);
    return __make_branch(__retain(n->left), new_right);
  }

  // apply fn to to each leaf
  template <typename Fn>
  static bool
  __for_each_chunk(const __node *root, Fn &&fn)
  {
    if ( !root ) return true;

    constexpr usize __max_depth = 64;
    const __node *stack[__max_depth];
    usize depth = 0;
    const __node *cur = root;

    for ( ;; ) {
      while ( cur && !__is_leaf(cur) ) {
        stack[depth++] = cur;
        cur = cur->left;
      }
      if ( cur ) {
        if ( !fn(__leaf_data(cur), cur->__length) ) return false;
      }
      if ( depth > 0 )
        cur = stack[--depth]->right;
      else
        break;
    }
    return true;
  }

  static inline int
  __cmp_raw(const __node *root, usize alen, const T *b, usize blen) noexcept
  {
    usize common = alen < blen ? alen : blen;
    usize pos = 0;
    int result = 0;

    __for_each_chunk(root, [&](const T *data, usize len) -> bool {
      usize check = len;
      if ( pos + check > common ) check = common - pos;
      for ( usize i = 0; i < check; i++ ) {
        auto ca = static_cast<unsigned char>(data[i]);
        auto cb = static_cast<unsigned char>(b[pos + i]);
        if ( ca < cb ) {
          result = -1;
          return false;
        }
        if ( ca > cb ) {
          result = 1;
          return false;
        }
      }
      pos += check;
      return pos < common;
    });

    if ( result != 0 ) return result;
    if ( alen < blen ) return -1;
    if ( alen > blen ) return 1;
    return 0;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%
  // safeties

  inline constexpr __attribute__((always_inline)) bool
  __index_check(usize n) const
  {
    return n >= __length;
  }

  inline constexpr __attribute__((always_inline)) bool
  __valid_cnt(usize cnt) const
  {
    return cnt > micron::numeric_limits<ssize_t>::max();
  }

  inline constexpr __attribute__((always_inline)) bool
  __range_pos_cnt(usize pos, usize cnt) const
  {
    return pos > __length || (pos + cnt) > __length;
  }

  inline constexpr __attribute__((always_inline)) bool
  __null_check(const void *ptr) const
  {
    return ptr == nullptr;
  }

  inline constexpr __attribute__((always_inline)) bool
  __erase_check(usize ind, usize cnt) const
  {
    return ind > __length || cnt > __length - ind;
  }

  template <auto Fn, typename E, typename... Args>
  inline __attribute__((always_inline)) void
  __safety_check(const char *msg, Args &&...args) const
  {
    if constexpr ( Sf == true ) {
      if ( (this->*Fn)(micron::forward<Args>(args)...) ) exc<E>(msg);
    }
  }

  __node *__root;
  usize __length;
  mutable T *__flat;

  // private constructor for internal copying
  rope(__node *root, usize len) : __root(root), __length(len), __flat(nullptr) {}

  void
  __free_flat(void) const
  {
    if ( __flat ) {
      abc::dealloc(reinterpret_cast<byte *>(__flat));
      __flat = nullptr;
    }
  }

  void
  __ensure_flat(void) const
  {
    if ( __flat ) return;
    __flat = reinterpret_cast<T *>(abc::alloc((__length + 1) * sizeof(T)));
    usize pos = 0;
    __for_each_chunk(__root, [&](const T *data, usize len) -> bool {
      micron::memcpy(&__flat[pos], data, len);
      pos += len;
      return true;
    });
    __flat[__length] = T(0);
  }

public:
  static constexpr usize npos = ~usize(0);

  using category_type = string_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef usize size_type;
  typedef const T &reference;
  typedef const T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef const T *pointer;
  typedef const T *const_pointer;

  class const_iterator
  {
    static constexpr usize __max_depth = 64;
    const __node *__stack[__max_depth];
    usize __depth;
    const __node *__leaf;
    usize __pos;
    usize __index;     // global character index

    void
    __push_to_leftmost(const __node *n)
    {
      while ( n && !__is_leaf(n) ) {
        __stack[__depth++] = n;
        n = n->left;
      }
      __leaf = n;
      __pos = 0;
    }

  public:
    const_iterator() : __depth(0), __leaf(nullptr), __pos(0), __index(0) {}

    explicit const_iterator(const __node *root, usize start_index = 0) : __depth(0), __leaf(nullptr), __pos(0), __index(start_index)
    {
      if ( root ) __push_to_leftmost(root);
    }

    usize
    index(void) const
    {
      return __index;
    }

    bool
    operator==(const const_iterator &o) const
    {
      if ( !__leaf && !o.__leaf ) return true;
      return __leaf == o.__leaf && __pos == o.__pos;
    }

    bool
    operator!=(const const_iterator &o) const
    {
      return !(*this == o);
    }

    const T &
    operator*(void) const
    {
      return __leaf_data(__leaf)[__pos];
    }

    const T *
    operator->(void) const
    {
      return &__leaf_data(__leaf)[__pos];
    }

    const_iterator &
    operator++(void)
    {
      ++__index;
      if ( ++__pos < __leaf->__length ) return *this;
      if ( __depth > 0 ) {
        const __node *parent = __stack[--__depth];
        __push_to_leftmost(parent->right);
      } else {
        __leaf = nullptr;
        __pos = 0;
      }
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

  // destructor first always
  ~rope(void)
  {
    __free_flat();
    __release(__root);
  }

  rope(void) : __root(nullptr), __length(0), __flat(nullptr) {}

  constexpr rope([[maybe_unused]] const usize n) : __root(nullptr), __length(0), __flat(nullptr) {}

  rope(usize cnt, T ch) : __root(__build_fill(ch, cnt)), __length(cnt), __flat(nullptr) {}

  rope(const char *str) : __flat(nullptr)
  {
    usize len = micron::strlen(str);
    __root = __build_balanced(reinterpret_cast<const T *>(str), len);
    __length = len;
  }

  template <usize M, typename F> rope(const F (&str)[M]) : __flat(nullptr)
  {
    usize len = M - 1;     // exclude null terminator
    __root = __build_balanced(reinterpret_cast<const T *>(str), len);
    __length = len;
  }

  // O(1) copy
  rope(const rope &o) : __root(__retain(o.__root)), __length(o.__length), __flat(nullptr) {}

  rope(rope &&o) noexcept : __root(o.__root), __length(o.__length), __flat(o.__flat)
  {
    o.__root = nullptr;
    o.__length = 0;
    o.__flat = nullptr;
  }

  template <typename F>
  rope(const rope<F> &o) : __root(__retain(reinterpret_cast<__node *>(o.__root))), __length(o.__length), __flat(nullptr)
  {
  }

  template <is_string S> rope(const S &o) : __flat(nullptr)
  {
    __root = __build_balanced(reinterpret_cast<const T *>(o.c_str()), o.size());
    __length = o.size();
  }

  template <is_container C>
    requires(sizeof(typename C::value_type) == sizeof(T))
  rope(const C &o) : __flat(nullptr)
  {
    __root = __build_balanced(reinterpret_cast<const T *>(o.data()), o.size());
    __length = o.size();
  }

  rope(const T restrict *__start, const T *__end) : __flat(nullptr)
  {
    if ( __start >= __end ) {
      __root = nullptr;
      __length = 0;
      return;
    }
    usize len = static_cast<usize>(__end - __start);
    __root = __build_balanced(__start, len);
    __length = len;
  }

  rope &
  operator=(const rope &o)
  {
    if ( this != &o ) [[likely]] {
      __free_flat();
      __release(__root);
      __root = __retain(o.__root);
      __length = o.__length;
    }
    return *this;
  }

  rope &
  operator=(rope &&o) noexcept
  {
    if ( this != &o ) [[likely]] {
      __free_flat();
      __release(__root);
      __root = o.__root;
      __length = o.__length;
      __flat = o.__flat;
      o.__root = nullptr;
      o.__length = 0;
      o.__flat = nullptr;
    }
    return *this;
  }

  template <usize M, typename F = T>
  rope &
  operator=(const F (&str)[M])
  {
    __free_flat();
    __release(__root);
    usize len = M - 1;
    __root = __build_balanced(reinterpret_cast<const T *>(str), len);
    __length = len;
    return *this;
  }

  chunk<byte>
  operator*(void)
  {
    __ensure_flat();
    return { reinterpret_cast<byte *>(__flat), __length };
  }

  inline bool
  operator!(void) const
  {
    return empty();
  }

  bool
  empty(void) const
  {
    return __length == 0;
  }

  usize
  size(void) const
  {
    return __length;
  }

  usize
  max_size(void) const
  {
    return __length;
  }

  usize
  capacity() const noexcept
  {
    return __length;
  }

  const T *
  data(void) const
  {
    __ensure_flat();
    return __flat;
  }

  const T *
  cdata(void) const
  {
    return data();
  }

  inline const char *
  c_str(void) const
  {
    if ( !__root ) return __rope_null_str;
    __ensure_flat();
    return reinterpret_cast<const char *>(__flat);
  }

  inline const wide *
  w_str(void) const
  {
    if ( !__root ) return __rope_null_wstr;
    __ensure_flat();
    return reinterpret_cast<const wide *>(__flat);
  }

  inline const unicode32 *
  uni_str(void) const
  {
    if ( !__root ) return __rope_null_u32str;
    __ensure_flat();
    return reinterpret_cast<const unicode32 *>(__flat);
  }

  inline slice<T>
  into_chars(void) const
  {
    __ensure_flat();
    return slice<const T>(&__flat[0], &__flat[__length]);
  }

  inline slice<byte>
  into_bytes(void) const
  {
    __ensure_flat();
    return slice<byte>(reinterpret_cast<const byte *>(&__flat[0]), reinterpret_cast<const byte *>(&__flat[__length]));
  }

  inline rope
  clone(void) const
  {
    return rope(*this);
  }

  template <typename F>
  inline F
  clone(void) const
  {
    return F(*this);
  }

  rope
  flatten(void) const
  {
    if ( !__root ) return rope();
    __ensure_flat();
    return rope(__build_balanced(__flat, __length), __length);
  }

  inline const T &
  front(void) const
  {
    return __char_at(__root, 0);
  }

  inline const T &
  back(void) const
  {
    return __char_at(__root, __length - 1);
  }

  inline const T &
  at(const usize n) const
  {
    __safety_check<&rope::__index_check, except::library_error>("micron::rope at() out of range", n);
    return __char_at(__root, n);
  }

  inline T
  operator[](const usize n) const
  {
    return __char_at(__root, n);
  }

  template <typename F>
  usize
  find(F ch, usize pos = 0) const
  {
    if ( !__root || pos >= __length ) return npos;
    usize idx = 0;
    usize result = npos;

    __for_each_chunk(__root, [&](const T *data, usize len) -> bool {
      usize start = (pos > idx) ? pos - idx : 0;
      for ( usize i = start; i < len; i++ ) {
        if ( data[i] == static_cast<T>(ch) ) {
          result = idx + i;
          return false;
        }
      }
      idx += len;
      return true;
    });

    return result;
  }

  inline usize
  find_substr(const T *needle, usize needle_len, usize pos = 0) const
  {
    if ( needle_len == 0 || needle_len > __length ) return npos;
    __ensure_flat();
    usize limit = __length - needle_len;
    for ( usize i = pos; i <= limit; ++i ) {
      usize j = 0;
      while ( j < needle_len && __flat[i + j] == needle[j] ) ++j;
      if ( j == needle_len ) return i;
    }
    return npos;
  }

  template <typename F>
  usize
  find(const rope<F> &str, usize pos = 0) const
  {
    if ( str.empty() ) return npos;
    str.__ensure_flat();
    return find_substr(reinterpret_cast<const T *>(str.__flat), str.__length, pos);
  }

  const_iterator
  begin(void) const
  {
    return const_iterator(__root);
  }

  const_iterator
  end(void) const
  {
    return const_iterator();
  }

  const_iterator
  cbegin(void) const
  {
    return const_iterator(__root);
  }

  const_iterator
  cend(void) const
  {
    return const_iterator();
  }

  const_iterator
  last(void) const
  {
    // NOTE: prefer back() for accessing the last element
    return const_iterator(__root, __length > 0 ? __length - 1 : 0);
  }

  rope
  clear(void) const
  {
    return rope();
  }

  rope
  fast_clear(void) const
  {
    return rope();
  }

  template <typename F = T>
  rope
  push_back(F ch) const
  {
    __node *nr = __push_back_impl(__root, static_cast<T>(ch));
    return rope(nr, __length + 1);
  }

  template <typename F = T, usize M>
  rope
  push_back(const F (&str)[M]) const
  {
    usize len = M - 1;
    if ( len == 0 ) return *this;
    __node *leaf = __make_leaf(reinterpret_cast<const T *>(str), len);
    return rope(__concat(__retain(__root), leaf), __length + len);
  }

  template <typename F = T>
  rope
  push_back(const rope<F> &o) const
  {
    if ( o.empty() ) return *this;
    return rope(__concat(__retain(__root), __retain(o.__root)), __length + o.__length);
  }

  rope
  pop_back(void) const
  {
    if ( __length == 0 ) return rope();
    auto [left, right] = __split(__root, __length - 1);
    __release(right);
    return rope(left, __length - 1);
  }

  inline rope
  append(const buffer &f, usize n) const
  {
    if ( n == 0 ) return *this;
    __node *leaf = __build_balanced(reinterpret_cast<const T *>(&f[0]), n);
    return rope(__concat(__retain(__root), leaf), __length + n);
  }

  template <typename F>
  inline rope
  append(const slice<F> &f, usize n) const
  {
    if ( n == 0 ) return *this;
    __node *leaf = __build_balanced(reinterpret_cast<const T *>(&f[0]), n);
    return rope(__concat(__retain(__root), leaf), __length + n);
  }

  template <typename F>
  inline rope
  append(const F *f, usize n) const
  {
    if ( n == 0 ) return *this;
    usize len = n - 1;     // match hstring convention (includes null)
    __node *leaf = __build_balanced(reinterpret_cast<const T *>(f), len);
    return rope(__concat(__retain(__root), leaf), __length + len);
  }

  template <typename F = T, usize M>
  inline rope
  append(const F (&str)[M]) const
  {
    usize len = M - 1;
    if ( len == 0 ) return *this;
    __node *leaf = __build_balanced(reinterpret_cast<const T *>(str), len);
    return rope(__concat(__retain(__root), leaf), __length + len);
  }

  template <typename F = T>
  inline rope
  append(const rope<F> &o) const
  {
    if ( o.empty() ) return *this;
    return rope(__concat(__retain(__root), __retain(o.__root)), __length + o.__length);
  }

  template <is_string S>
  inline rope
  append(const S &o) const
  {
    if ( o.size() == 0 ) return *this;
    __node *leaf = __build_balanced(reinterpret_cast<const T *>(o.c_str()), o.size());
    return rope(__concat(__retain(__root), leaf), __length + o.size());
  }

  template <typename F = T>
  rope
  insert(usize ind, F ch, usize cnt = 1) const
  {
    __safety_check<&rope::__valid_cnt, except::library_error>("micron::rope insert() invalid count", cnt);

    __node *fill = __build_fill(static_cast<T>(ch), cnt);
    auto [left, right] = __split(__root, ind);
    __node *result = __concat(__concat(left, fill), right);
    return rope(result, __length + cnt);
  }

  template <typename F = T, usize M>
  rope
  insert(usize ind, const F (&str)[M], usize cnt = 1) const
  {
    __safety_check<&rope::__valid_cnt, except::library_error>("micron::rope insert() invalid count", cnt);

    usize str_len = M - 1;
    usize total = str_len * cnt;
    if ( total == 0 ) return *this;

    // build the insertion payload (repeat str cnt times)
    __node *payload = nullptr;
    for ( usize i = 0; i < cnt; ++i ) {
      __node *seg = __make_leaf(reinterpret_cast<const T *>(str), str_len);
      payload = payload ? __concat(payload, seg) : seg;
    }

    auto [left, right] = __split(__root, ind);
    __node *result = __concat(__concat(left, payload), right);
    return rope(result, __length + total);
  }

  template <typename F = T>
  rope
  insert(usize ind, const rope<F> &o) const
  {
    if ( o.empty() ) return *this;
    auto [left, right] = __split(__root, ind);
    __node *result = __concat(__concat(left, __retain(o.__root)), right);
    return rope(result, __length + o.__length);
  }

  template <is_string S>
  rope
  insert(usize ind, const S &o) const
  {
    if ( o.size() == 0 ) return *this;
    __node *seg = __build_balanced(reinterpret_cast<const T *>(o.c_str()), o.size());
    auto [left, right] = __split(__root, ind);
    __node *result = __concat(__concat(left, seg), right);
    return rope(result, __length + o.size());
  }

  template <typename F = T>
  rope
  insert(const_iterator itr, F ch, usize cnt = 1) const
  {
    return insert(itr.index(), ch, cnt);
  }

  template <typename F = T, usize M>
  rope
  insert(const_iterator itr, const F (&str)[M], usize cnt = 1) const
  {
    return insert(itr.index(), str, cnt);
  }

  template <typename F = T>
  rope
  insert(const_iterator itr, const rope<F> &o) const
  {
    return insert(itr.index(), o);
  }

  template <is_string S>
  rope
  insert(const_iterator itr, const S &o) const
  {
    return insert(itr.index(), o);
  }

  template <typename F = T, typename I = usize>
    requires micron::is_arithmetic_v<I>
  rope
  erase(I __ind, usize cnt = 1) const
  {
    usize ind = static_cast<usize>(__ind);

    __safety_check<&rope::__valid_cnt, except::library_error>("micron::rope erase() invalid count", cnt);
    __safety_check<&rope::__erase_check, except::library_error>("micron::rope erase() out of range", ind, cnt);

    if ( cnt == 0 ) return *this;

    auto [left, rest] = __split(__root, ind);
    auto [removed, right] = __split(rest ? rest : nullptr, cnt);
    __release(rest);
    __release(removed);
    __node *result = __concat(left, right);
    return rope(result ? result : nullptr, __length - cnt);
  }

  template <typename F = T>
  rope
  erase(const_iterator itr, usize cnt = 1) const
  {
    return erase(itr.index(), cnt);
  }

  template <typename F = T>
  rope
  substr(usize pos = 0, usize cnt = npos) const
  {
    if ( cnt == npos ) cnt = (pos <= __length) ? __length - pos : 0;

    __safety_check<&rope::__range_pos_cnt, except::library_error>("micron::rope substr() invalid range", pos, cnt);

    if ( pos == 0 && cnt == __length ) return *this;

    auto [left, rest] = __split(__root, pos);
    auto [sub, right] = __split(rest, cnt);
    __release(left);
    __release(rest);
    __release(right);
    return rope(sub, cnt);
  }

  template <typename F = T>
  rope
  substr(const_iterator _start, const_iterator _end) const
  {
    usize pos = _start.index();
    usize end_pos = _end.index();
    if ( end_pos <= pos ) return rope();
    return substr(pos, end_pos - pos);
  }

  template <typename F = T, typename I = usize>
    requires micron::is_arithmetic_v<I>
  rope
  truncate(I n) const
  {
    usize idx = static_cast<usize>(n);
    if ( idx >= __length ) return *this;
    auto [left, right] = __split(__root, idx);
    __release(right);
    return rope(left, idx);
  }

  rope
  truncate(const_iterator itr) const
  {
    return truncate(itr.index());
  }

  void reserve(usize) = delete;

  void try_reserve(usize) const = delete;

  rope
  resize(usize n, const T ch) const
  {
    if ( n <= __length ) return truncate(n);
    usize fill = n - __length;
    __node *tail = __build_fill(ch, fill);
    return rope(__concat(__retain(__root), tail), n);
  }

  template <typename F = T>
  rope
  remove(const char *needle) const
  {
    __safety_check<&rope::__null_check, except::library_error>("micron::rope remove() null needle", static_cast<const void *>(needle));

    usize needle_len = micron::strlen(needle);
    if ( needle_len == 0 || !__root ) return *this;

    usize pos = find_substr(reinterpret_cast<const T *>(needle), needle_len);
    if ( pos == npos ) return *this;

    return erase(pos, needle_len);
  }

  template <typename F = T>
  rope
  remove(const rope<F> &needle) const
  {
    if ( needle.empty() ) return *this;

    needle.__ensure_flat();
    usize pos = find_substr(reinterpret_cast<const T *>(needle.__flat), needle.__length);
    if ( pos == npos ) return *this;

    return erase(pos, needle.__length);
  }

  template <is_string S>
  rope
  remove(const S &needle) const
  {
    if ( needle.size() == 0 ) return *this;

    usize pos = find_substr(reinterpret_cast<const T *>(needle.c_str()), needle.size());
    if ( pos == npos ) return *this;

    return erase(pos, needle.size());
  }

  template <typename F = T>
  rope
  remove_all(const char *needle) const
  {
    __safety_check<&rope::__null_check, except::library_error>("micron::rope remove_all() null needle", static_cast<const void *>(needle));

    usize needle_len = micron::strlen(needle);
    if ( needle_len == 0 || !__root ) return *this;

    rope cur = *this;
    usize pos = 0;
    while ( (pos = cur.find_substr(reinterpret_cast<const T *>(needle), needle_len, pos)) != npos ) cur = cur.erase(pos, needle_len);
    return cur;
  }

  template <typename F = T>
  rope
  remove_all(const rope<F> &needle) const
  {
    if ( needle.empty() ) return *this;

    needle.__ensure_flat();
    rope cur = *this;
    usize pos = 0;
    while ( (pos = cur.find_substr(reinterpret_cast<const T *>(needle.__flat), needle.__length, pos)) != npos )
      cur = cur.erase(pos, needle.__length);
    return cur;
  }

  template <is_string S>
  rope
  remove_all(const S &needle) const
  {
    if ( needle.size() == 0 ) return *this;

    rope cur = *this;
    usize pos = 0;
    while ( (pos = cur.find_substr(reinterpret_cast<const T *>(needle.c_str()), needle.size(), pos)) != npos )
      cur = cur.erase(pos, needle.size());
    return cur;
  }

  rope &
  operator+=(const T d)
  {
    *this = push_back(d);
    return *this;
  }

  template <typename F = T>
  rope &
  operator+=(const F *data)
  {
    usize len = micron::strlen(data);
    __node *leaf = __build_balanced(reinterpret_cast<const T *>(data), len);
    *this = rope(__concat(__retain(__root), leaf), __length + len);
    return *this;
  }

  template <typename F = T, usize M>
  rope &
  operator+=(const F (&str)[M])
  {
    *this = append(str);
    return *this;
  }

  template <typename F = T>
  rope &
  operator+=(const rope<F> &data)
  {
    *this = append(data);
    return *this;
  }

  template <is_string S>
  rope &
  operator+=(const S &data)
  {
    *this = append(data);
    return *this;
  }

  rope &
  operator+=(const buffer &data)
  {
    *this = append(data, data.size());
    return *this;
  }

  template <typename F = T>
  rope &
  operator+=(const slice<F> &data)
  {
    __node *leaf = __build_balanced(reinterpret_cast<const T *>(&data[0]), data.size());
    *this = rope(__concat(__retain(__root), leaf), __length + data.size());
    return *this;
  }

  const void *
  identity() const noexcept
  {
    return __root;
  }

  explicit inline
  operator bool() const noexcept
  {
    return !empty();
  }

  bool
  operator==(const rope &o) const
  {
    if ( __root == o.__root ) [[unlikely]]
      return true;
    if ( __length != o.__length ) return false;
    auto a = begin(), ae = end();
    auto b = o.begin();
    for ( ; a != ae; ++a, ++b ) {
      if ( *a != *b ) return false;
    }
    return true;
  }

  bool
  operator!=(const rope &o) const
  {
    return !(*this == o);
  }

  inline bool
  operator==(const T *data) const
  {
    __safety_check<&rope::__null_check, except::library_error>("micron::rope operator==() null pointer", static_cast<const void *>(data));
    return __cmp_raw(__root, __length, data, micron::strlen(data)) == 0;
  }

  inline bool
  operator!=(const T *data) const
  {
    __safety_check<&rope::__null_check, except::library_error>("micron::rope operator!=() null pointer", static_cast<const void *>(data));
    return __cmp_raw(__root, __length, data, micron::strlen(data)) != 0;
  }

  inline bool
  operator<(const T *data) const
  {
    __safety_check<&rope::__null_check, except::library_error>("micron::rope operator<() null pointer", static_cast<const void *>(data));
    return __cmp_raw(__root, __length, data, micron::strlen(data)) < 0;
  }

  inline bool
  operator>(const T *data) const
  {
    __safety_check<&rope::__null_check, except::library_error>("micron::rope operator>() null pointer", static_cast<const void *>(data));
    return __cmp_raw(__root, __length, data, micron::strlen(data)) > 0;
  }

  inline bool
  operator<=(const T *data) const
  {
    __safety_check<&rope::__null_check, except::library_error>("micron::rope operator<=() null pointer", static_cast<const void *>(data));
    return __cmp_raw(__root, __length, data, micron::strlen(data)) <= 0;
  }

  inline bool
  operator>=(const T *data) const
  {
    __safety_check<&rope::__null_check, except::library_error>("micron::rope operator>=() null pointer", static_cast<const void *>(data));
    return __cmp_raw(__root, __length, data, micron::strlen(data)) >= 0;
  }

  template <typename F = T, usize M>
  inline bool
  operator==(const F (&data)[M]) const
  {
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(&data[0]), M - 1) == 0;
  }

  template <typename F = T, usize M>
  inline bool
  operator!=(const F (&data)[M]) const
  {
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(&data[0]), M - 1) != 0;
  }

  template <typename F = T, usize M>
  inline bool
  operator<(const F (&data)[M]) const
  {
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(&data[0]), M - 1) < 0;
  }

  template <typename F = T, usize M>
  inline bool
  operator>(const F (&data)[M]) const
  {
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(&data[0]), M - 1) > 0;
  }

  template <typename F = T, usize M>
  inline bool
  operator<=(const F (&data)[M]) const
  {
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(&data[0]), M - 1) <= 0;
  }

  template <typename F = T, usize M>
  inline bool
  operator>=(const F (&data)[M]) const
  {
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(&data[0]), M - 1) >= 0;
  }

  template <typename F = T>
  inline bool
  operator==(const rope<F> &data) const
  {
    if ( __root == data.__root ) [[unlikely]]
      return true;
    if ( __length != data.__length ) return false;
    data.__ensure_flat();
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(data.__flat), data.__length) == 0;
  }

  template <typename F = T>
  inline bool
  operator!=(const rope<F> &data) const
  {
    return !(*this == data);
  }

  template <typename F = T>
  inline bool
  operator<(const rope<F> &data) const
  {
    data.__ensure_flat();
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(data.__flat), data.__length) < 0;
  }

  template <typename F = T>
  inline bool
  operator>(const rope<F> &data) const
  {
    data.__ensure_flat();
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(data.__flat), data.__length) > 0;
  }

  template <typename F = T>
  inline bool
  operator<=(const rope<F> &data) const
  {
    data.__ensure_flat();
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(data.__flat), data.__length) <= 0;
  }

  template <typename F = T>
  inline bool
  operator>=(const rope<F> &data) const
  {
    data.__ensure_flat();
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(data.__flat), data.__length) >= 0;
  }

  // comparison with is_string types (hstring, sstring, etc.)
  template <is_string S>
  inline bool
  operator==(const S &str) const
  {
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(str.c_str()), str.size()) == 0;
  }

  template <is_string S>
  inline bool
  operator!=(const S &str) const
  {
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(str.c_str()), str.size()) != 0;
  }

  template <is_string S>
  inline bool
  operator<(const S &str) const
  {
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(str.c_str()), str.size()) < 0;
  }

  template <is_string S>
  inline bool
  operator>(const S &str) const
  {
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(str.c_str()), str.size()) > 0;
  }

  template <is_string S>
  inline bool
  operator<=(const S &str) const
  {
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(str.c_str()), str.size()) <= 0;
  }

  template <is_string S>
  inline bool
  operator>=(const S &str) const
  {
    return __cmp_raw(__root, __length, reinterpret_cast<const T *>(str.c_str()), str.size()) >= 0;
  }

  template <typename Fn>
  void
  for_each(Fn &&fn) const
  {
    __for_each_chunk(__root, [&](const T *data, usize len) -> bool {
      for ( usize i = 0; i < len; ++i ) fn(static_cast<const T &>(data[i]));
      return true;
    });
  }

  template <typename Fn>
  void
  for_each_chunk(Fn &&fn) const
  {
    __for_each_chunk(__root, [&](const T *data, usize len) -> bool {
      fn(data, len);
      return true;
    });
  }

  template <is_scalar_literal F, bool> friend class rope;
};

};     // namespace micron
