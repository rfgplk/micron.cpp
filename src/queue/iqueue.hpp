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

namespace micron
{

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// immutable_queue is a persistent fifo container implemented as a Hood-Melville real-time queue
// ref Hood & Melville, "Real-Time Queue Operations in Pure LISP" (1981)

template<typename T>
  requires micron::is_copy_constructible_v<T> and micron::is_move_constructible_v<T> and micron::is_destructible_v<T>
class immutable_queue
{
  // 64-bit:
  //    [0]  next
  //    [8]  refs
  //    [12] (pad to alignof(T))
  //    [..] value

  struct __node {
    __node *next;
    mutable u32 refs;
    T value;
  };

  template<typename Tf>
  static inline __node *
  __make_node(Tf &&v, __node *nxt)
  {
    void *raw = nullptr;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      raw = ::operator new(sizeof(__node), static_cast<std::align_val_t>(alignof(__node)));
      return new (raw) __node{ nxt, 1, T(static_cast<Tf &&>(v)) };
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      if ( raw ) ::operator delete(raw, static_cast<std::align_val_t>(alignof(__node)));
      __release(nxt);
      throw;
    }
#endif
  }

  static inline void
  __dealloc_node(__node *n)
  {
    n->~__node();
    ::operator delete(n, static_cast<std::align_val_t>(alignof(__node)));
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
    while ( n ) [[likely]] {
      if ( --n->refs != 0 ) [[likely]]
        break;
      __node *nxt = n->next;
      __dealloc_node(n);
      n = nxt;
    }
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  //  rotation state machine
  //  idle: no rotation in progress
  //  reversing: simultaneously reversing front and rear lists
  //  appending: prepending reversed front elements onto reversed rear
  //  done: rotation result ready in rp

  enum __phase : u8 { __idle, __reversing, __appending, __done };

  // NOTE : all non-null pointers are owned references
  struct __rot {
    __phase phase;
    usize ok;        // count of valid front elements
    __node *f;       // remaining (reversing phase)
    __node *fp;      // reversed (reversing/appending phase)
    __node *r;       // rear remaining (reversing phase)
    __node *rp;      // result (all phases)
  };

  static inline __rot
  __idle_rot(void) noexcept
  {
    return { __idle, 0, nullptr, nullptr, nullptr, nullptr };
  }

  static inline void
  __retain_rot(const __rot &s) noexcept
  {
    __retain(s.f);
    __retain(s.fp);
    __retain(s.r);
    __retain(s.rp);
  }

  static inline void
  __release_rot(const __rot &s) noexcept
  {
    __release(s.f);
    __release(s.fp);
    __release(s.r);
    __release(s.rp);
  }

  struct __node_owner {
    __node *value;

    explicit __node_owner(__node *n = nullptr) noexcept : value(n) { }

    ~__node_owner() { __release(value); }

    __node *
    take() noexcept
    {
      __node *result = value;
      value = nullptr;
      return result;
    }
  };

  struct __rot_owner {
    __rot value;
    bool active;

    explicit __rot_owner(const __rot &s) noexcept : value(s), active(true) { }

    ~__rot_owner()
    {
      if ( active ) __release_rot(value);
    }

    void
    reset() noexcept
    {
      if ( active ) __release_rot(value);
      active = false;
    }

    __rot
    take() noexcept
    {
      active = false;
      return value;
    }

    __node *
    take_rp() noexcept
    {
      __node *result = value.rp;
      value.rp = nullptr;
      return result;
    }
  };

  // advance rotation by one step
  // ..exec(Reversing(ok, x::f, f', y::r, r'))  = Reversing(ok+1, f, x::f', r, y::r')
  // ..exec(Reversing(ok, [],    f', [y],  r'))  = Appending(ok, f', y::r')
  // ..exec(Appending(0,  _,     r'))            = Done(r')
  // ..exec(Appending(ok, x::f', r'))            = Appending(ok-1, f', x::r')
  // ..exec(state)                               = state

  static __rot
  __exec(const __rot &s)
  {
    switch ( s.phase ) {
    case __reversing:
      if ( s.f ) [[likely]] {
        __node_owner f(__retain(s.f->next));
        __node_owner fp(__make_node(s.f->value, __retain(s.fp)));
        __node_owner r(__retain(s.r->next));
        __node_owner rp(__make_node(s.r->value, __retain(s.rp)));
        return { __reversing, s.ok + 1, f.take(), fp.take(), r.take(), rp.take() };
      } else {
        //  front exhausted; rear has exactly one element left
        __node_owner fp(__retain(s.fp));
        __node_owner rp(__make_node(s.r->value, __retain(s.rp)));
        return { __appending, s.ok, nullptr, fp.take(), nullptr, rp.take() };
      }

    case __appending:
      if ( s.ok == 0 ) [[unlikely]] {
        return { __done, 0, nullptr, nullptr, nullptr, __retain(s.rp) };
      } else {
        __node_owner fp(__retain(s.fp->next));
        __node_owner rp(__make_node(s.fp->value, __retain(s.rp)));
        return { __appending, s.ok - 1, nullptr, fp.take(), nullptr, rp.take() };
      }

    default: {
      __rot copy = s;
      __retain_rot(copy);
      return copy;
    }
    }
  }

  // account for a dequeued front element
  // ..invalidate(Reversing(ok, f, f', r, r'))  = Reversing(ok-1, f, f', r, r')
  // ..invalidate(Appending(0,  f', x::r'))     = Done(r')        [drop x]
  // ..invalidate(Appending(ok, f', r'))         = Appending(ok-1, f', r')
  // ..invalidate(state)                         = state

  static __rot
  __invalidate(const __rot &s)
  {
    switch ( s.phase ) {
    case __reversing: {
      return { __reversing, s.ok - 1, __retain(s.f), __retain(s.fp), __retain(s.r), __retain(s.rp) };
    }
    case __appending: {
      if ( s.ok == 0 ) [[unlikely]] {
        //  drop top element of rp
        return { __done, 0, nullptr, nullptr, nullptr, __retain(s.rp->next) };
      } else {
        return { __appending, s.ok - 1, nullptr, __retain(s.fp), nullptr, __retain(s.rp) };
      }
    }
    default: {
      __rot copy = s;
      __retain_rot(copy);
      return copy;
    }
    }
  }

  // NOTE: takes ownership
  static immutable_queue
  __exec_twice(__node *f, usize fl, __node *r, usize rl, __rot state, __node *mid, usize ml)
  {
    __node_owner owned_f(f), owned_r(r), owned_mid(mid);
    __rot_owner owned_state(state);
    __rot_owner s1(__exec(owned_state.value));
    owned_state.reset();
    __rot_owner s2(__exec(s1.value));
    s1.reset();

    if ( s2.value.phase == __done ) [[unlikely]] {
      //  rotation complete: rp is the new front list
      //  transfer ownership of rp out of s2
      __node *new_f = s2.take_rp();
      return immutable_queue(new_f, fl, owned_r.take(), rl, __idle_rot(), nullptr, 0);
    }

    return immutable_queue(owned_f.take(), fl, owned_r.take(), rl, s2.take(), owned_mid.take(), ml);
  }

  // maintain |r| <= |f| invariant
  // init rotation if violated
  // NOTE: takes ownership
  static immutable_queue
  __check(__node *f, usize fl, __node *r, usize rl, __rot state, __node *mid, usize ml)
  {
    if ( rl <= fl ) [[likely]] {
      return __exec_twice(f, fl, r, rl, state, mid, ml);
    } else {
      //  invariant violated: begin rotation
      //  Reversing(0, f, nil, r, nil)
      __rot new_state = { __reversing, 0, __retain(f), nullptr, __retain(r), nullptr };
      __release_rot(state);
      __release(mid);      // rotations cannot overlap; normally null
      return __exec_twice(f, fl + rl, nullptr, 0, new_state, r, rl);
    }
  }

  __node *__front;
  usize __f_len;
  __node *__rear;
  usize __r_len;
  __rot __state;
  __node *__rotation_rear;
  usize __rotation_rear_len;

  // private constructor for internal copies
  immutable_queue(__node *f, usize fl, __node *r, usize rl, const __rot &s, __node *mid, usize ml)
      : __front(f), __f_len(fl), __rear(r), __r_len(rl), __state(s), __rotation_rear(mid), __rotation_rear_len(ml)
  {
  }

public:
  using category_type = buffer_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef usize size_type;
  typedef T value_type;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef const T *const_pointer;

  // destructor first
  ~immutable_queue(void)
  {
    __release(__front);
    __release(__rear);
    __release_rot(__state);
    __release(__rotation_rear);
  }

  immutable_queue(void)
      : __front(nullptr), __f_len(0), __rear(nullptr), __r_len(0), __state(__idle_rot()), __rotation_rear(nullptr), __rotation_rear_len(0)
  {
  }

  //  O(1) copy
  immutable_queue(const immutable_queue &o)
      : __front(__retain(o.__front)), __f_len(o.__f_len), __rear(__retain(o.__rear)), __r_len(o.__r_len), __state(o.__state),
        __rotation_rear(__retain(o.__rotation_rear)), __rotation_rear_len(o.__rotation_rear_len)
  {
    __retain_rot(__state);
  }

  immutable_queue &
  operator=(const immutable_queue &o)
  {
    // NOTE: important
    if ( this != &o ) [[likely]] {
      __release(__front);
      __release(__rear);
      __release_rot(__state);
      __release(__rotation_rear);
      __front = __retain(o.__front);
      __f_len = o.__f_len;
      __rear = __retain(o.__rear);
      __r_len = o.__r_len;
      __state = o.__state;
      __retain_rot(__state);
      __rotation_rear = __retain(o.__rotation_rear);
      __rotation_rear_len = o.__rotation_rear_len;
    }
    return *this;
  }

  //  O(1) move
  immutable_queue(immutable_queue &&o) noexcept
      : __front(o.__front), __f_len(o.__f_len), __rear(o.__rear), __r_len(o.__r_len), __state(o.__state),
        __rotation_rear(o.__rotation_rear), __rotation_rear_len(o.__rotation_rear_len)
  {
    o.__front = nullptr;
    o.__f_len = 0;
    o.__rear = nullptr;
    o.__r_len = 0;
    o.__state = __idle_rot();
    o.__rotation_rear = nullptr;
    o.__rotation_rear_len = 0;
  }

  immutable_queue &
  operator=(immutable_queue &&o) noexcept
  {
    if ( this != &o ) [[likely]] {
      __release(__front);
      __release(__rear);
      __release_rot(__state);
      __release(__rotation_rear);
      __front = o.__front;
      __f_len = o.__f_len;
      __rear = o.__rear;
      __r_len = o.__r_len;
      __state = o.__state;
      __rotation_rear = o.__rotation_rear;
      __rotation_rear_len = o.__rotation_rear_len;
      o.__front = nullptr;
      o.__f_len = 0;
      o.__rear = nullptr;
      o.__r_len = 0;
      o.__state = __idle_rot();
      o.__rotation_rear = nullptr;
      o.__rotation_rear_len = 0;
    }
    return *this;
  }

  // O(1)
  immutable_queue
  push(const T &v) const
  {
    __node *nr = __make_node(v, __retain(__rear));
    __rot s = __state;
    __retain_rot(s);
    return __check(__retain(__front), __f_len, nr, __r_len + 1, s, __retain(__rotation_rear), __rotation_rear_len);
  }

  immutable_queue
  push(T &&v) const
  {
    __node *nr = __make_node(micron::move(v), __retain(__rear));
    __rot s = __state;
    __retain_rot(s);
    return __check(__retain(__front), __f_len, nr, __r_len + 1, s, __retain(__rotation_rear), __rotation_rear_len);
  }

  template<typename... Args>
  immutable_queue
  emplace(Args &&...args) const
  {
    //  construct T in-place, then cons onto rear
    void *raw = nullptr;
    __node *n = nullptr;
    __node *next = __retain(__rear);
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      raw = ::operator new(sizeof(__node), static_cast<std::align_val_t>(alignof(__node)));
      n = new (raw) __node{ next, 1, T(micron::forward<Args>(args)...) };
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      if ( raw ) ::operator delete(raw, static_cast<std::align_val_t>(alignof(__node)));
      __release(next);
      throw;
    }
#endif

    __rot s = __state;
    __retain_rot(s);
    return __check(__retain(__front), __f_len, n, __r_len + 1, s, __retain(__rotation_rear), __rotation_rear_len);
  }

  // O(1)
  immutable_queue
  pop(void) const
  {
    if ( !__front ) [[unlikely]]
      return immutable_queue();

    __rot ns = __invalidate(__state);
    return __check(__retain(__front->next), __f_len - 1, __retain(__rear), __r_len, ns, __retain(__rotation_rear), __rotation_rear_len);
  }

  const T &
  front(void) const
  {
    return __front->value;
  }

  const T *
  peek(void) const
  {
    return __front ? micron::addressof(__front->value) : nullptr;
  }

  //  O(1) when rear is non-empty
  //  O(n) fallback when rear is empty (immediately after rotation)
  const T &
  last(void) const
  {
    if ( __rear ) [[likely]]
      return __rear->value;
    if ( __rotation_rear ) return __rotation_rear->value;
    //  rear empty: walk front to tail
    const __node *n = __front;
    while ( n->next ) n = n->next;
    return n->value;
  }

  const T &
  at(usize idx) const
  {
    if ( idx >= size() ) [[unlikely]]
      exc<except::library_error>("micron::immutable_queue at(): index out of range");

    const usize visible_front = __f_len - __rotation_rear_len;
    if ( idx < visible_front ) {
      const __node *n = __front;
      for ( usize i = 0; i < idx; ++i ) n = n->next;
      return n->value;
    }

    idx -= visible_front;
    const __node *n;
    usize rear_pos;
    if ( idx < __rotation_rear_len ) {
      rear_pos = __rotation_rear_len - idx - 1;
      n = __rotation_rear;
    } else {
      idx -= __rotation_rear_len;
      rear_pos = __r_len - idx - 1;
      n = __rear;
    }
    for ( usize i = 0; i < rear_pos; ++i ) n = n->next;
    return n->value;
  }

  usize
  size() const noexcept
  {
    return __f_len + __r_len;
  }

  bool
  empty() const noexcept
  {
    return __f_len + __r_len == 0;
  }

  immutable_queue
  clear(void) const
  {
    return immutable_queue();
  }

  const void *
  identity() const noexcept
  {
    return __front;
  }

  bool
  operator==(const immutable_queue &o) const
  {
    if ( __front == o.__front && __rear == o.__rear && __rotation_rear == o.__rotation_rear && size() == o.size() ) [[unlikely]]
      return true;
    if ( size() != o.size() ) return false;
    auto a = begin(), ae = end();
    auto b = o.begin();
    for ( ; a != ae; ++a, ++b ) {
      if ( *a != *b ) return false;
    }
    return true;
  }

  bool
  operator!=(const immutable_queue &o) const
  {
    return !(*this == o);
  }

  template<typename Fn>
  immutable_queue
  update_front(Fn &&fn) const
  {
    if ( !__front ) [[unlikely]]
      return *this;
    immutable_queue result;
    bool first = true;
    for_each([&](const T &value) {
      if ( first ) {
        result = result.push(fn(value));
        first = false;
      } else {
        result = result.push(value);
      }
    });
    return result;
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const
  {
    const __node *cur = __front;
    while ( cur ) {
      fn(static_cast<const T &>(cur->value));
      cur = cur->next;
    }

    const usize count = __rotation_rear_len + __r_len;
    if ( count == 0 ) return;
    auto *buf = static_cast<const __node **>(::operator new(count * sizeof(const __node *)));
    usize base = 0;
    cur = __rotation_rear;
    for ( usize i = 0; i < __rotation_rear_len; ++i, cur = cur->next ) buf[base + __rotation_rear_len - i - 1] = cur;
    base += __rotation_rear_len;
    cur = __rear;
    for ( usize i = 0; i < __r_len; ++i, cur = cur->next ) buf[base + __r_len - i - 1] = cur;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      for ( usize i = 0; i < count; ++i ) fn(static_cast<const T &>(buf[i]->value));
    } catch ( ... ) {
      ::operator delete(buf);
      throw;
    }
#else
    for ( usize i = 0; i < count; ++i ) fn(static_cast<const T &>(buf[i]->value));
#endif
    ::operator delete(buf);
  }

  class const_iterator
  {
    struct __backing {
      mutable u32 refs;
      usize count;
      const __node *nodes[1];
    };

    const __node *__cur;
    __backing *__back;
    usize __index;

    static __backing *
    __make_back(const __node *mid, usize ml, const __node *rear, usize rl)
    {
      const usize count = ml + rl;
      if ( count == 0 ) return nullptr;
      const usize bytes = sizeof(__backing) + (count - 1) * sizeof(const __node *);
      auto *back = static_cast<__backing *>(::operator new(bytes));
      back->refs = 1;
      back->count = count;
      const __node *node = mid;
      for ( usize i = 0; i < ml; ++i, node = node->next ) back->nodes[ml - i - 1] = node;
      node = rear;
      for ( usize i = 0; i < rl; ++i, node = node->next ) back->nodes[ml + rl - i - 1] = node;
      return back;
    }

    static __backing *
    __retain_back(__backing *back) noexcept
    {
      if ( back ) ++back->refs;
      return back;
    }

    static void
    __release_back(__backing *back) noexcept
    {
      if ( back && --back->refs == 0 ) ::operator delete(back);
    }

    const __node *
    __node_at() const noexcept
    {
      if ( __cur ) return __cur;
      return __back && __index < __back->count ? __back->nodes[__index] : nullptr;
    }

  public:
    const_iterator() : __cur(nullptr), __back(nullptr), __index(0) { }

    explicit const_iterator(const __node *front, const __node *mid, usize ml, const __node *rear, usize rl)
        : __cur(front), __back(__make_back(mid, ml, rear, rl)), __index(0)
    {
    }

    const_iterator(const const_iterator &o) : __cur(o.__cur), __back(__retain_back(o.__back)), __index(o.__index) { }

    const_iterator(const_iterator &&o) noexcept : __cur(o.__cur), __back(o.__back), __index(o.__index)
    {
      o.__cur = nullptr;
      o.__back = nullptr;
      o.__index = 0;
    }

    ~const_iterator() { __release_back(__back); }

    const_iterator &
    operator=(const const_iterator &o)
    {
      if ( this == micron::addr(o) ) return *this;
      __release_back(__back);
      __cur = o.__cur;
      __back = __retain_back(o.__back);
      __index = o.__index;
      return *this;
    }

    const_iterator &
    operator=(const_iterator &&o) noexcept
    {
      if ( this == micron::addr(o) ) return *this;
      __release_back(__back);
      __cur = o.__cur;
      __back = o.__back;
      __index = o.__index;
      o.__cur = nullptr;
      o.__back = nullptr;
      o.__index = 0;
      return *this;
    }

    bool
    operator==(const const_iterator &o) const
    {
      return __node_at() == o.__node_at();
    }

    bool
    operator!=(const const_iterator &o) const
    {
      return !(*this == o);
    }

    const T &
    value(void) const
    {
      return __node_at()->value;
    }

    const T &
    operator*(void) const
    {
      return value();
    }

    const T *
    operator->(void) const
    {
      return micron::addressof(__node_at()->value);
    }

    const_iterator &
    operator++(void)
    {
      if ( __cur ) {
        __cur = __cur->next;
      } else if ( __back && __index < __back->count )
        ++__index;
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
  begin(void) const
  {
    if ( empty() ) [[unlikely]]
      return const_iterator();
    return const_iterator(__front, __rotation_rear, __rotation_rear_len, __rear, __r_len);
  }

  const_iterator
  end(void) const
  {
    return const_iterator();
  }

  const_iterator
  cbegin(void) const
  {
    return begin();
  }

  const_iterator
  cend(void) const
  {
    return const_iterator();
  }
};

// alias N to nothing, for drop in replacement
template<typename T, auto N = 0> using iqueue = immutable_queue<T>;

};      // namespace micron
