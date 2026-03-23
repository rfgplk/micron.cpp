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

template <typename T>
  requires micron::is_movable_object<T>
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

  template <typename Tf>
  static inline __node *
  __make_node(Tf &&v, __node *nxt)
  {
    auto *n = reinterpret_cast<__node *>(abc::alloc(sizeof(__node)));
    if constexpr ( micron::is_trivially_copyable_v<T> )
      n->value = static_cast<Tf &&>(v);
    else
      new (micron::addr(n->value)) T(static_cast<Tf &&>(v));

    n->next = nxt;
    n->refs = 1;
    return n;
  }

  static inline void
  __dealloc_node(__node *n)
  {
    if constexpr ( !micron::is_trivially_destructible_v<T> )
      n->value.~T();
    abc::dealloc(reinterpret_cast<byte *>(n));
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
    usize ok;       // count of valid front elements
    __node *f;      // remaining (reversing phase)
    __node *fp;     // reversed (reversing/appending phase)
    __node *r;      // rear remaining (reversing phase)
    __node *rp;     // result (all phases)
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
    case __reversing :
      if ( s.f ) [[likely]] {
        return { __reversing,         s.ok + 1,
                 __retain(s.f->next), __make_node(s.f->value, __retain(s.fp)),
                 __retain(s.r->next), __make_node(s.r->value, __retain(s.rp)) };
      } else {
        //  front exhausted; rear has exactly one element left
        return { __appending, s.ok, nullptr, __retain(s.fp), nullptr, __make_node(s.r->value, __retain(s.rp)) };
      }

    case __appending :
      if ( s.ok == 0 ) [[unlikely]] {
        return { __done, 0, nullptr, nullptr, nullptr, __retain(s.rp) };
      } else {
        return { __appending, s.ok - 1, nullptr, __retain(s.fp->next), nullptr, __make_node(s.fp->value, __retain(s.rp)) };
      }

    default : {
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
    case __reversing : {
      return { __reversing, s.ok - 1, __retain(s.f), __retain(s.fp), __retain(s.r), __retain(s.rp) };
    }
    case __appending : {
      if ( s.ok == 0 ) [[unlikely]] {
        //  drop top element of rp
        return { __done, 0, nullptr, nullptr, nullptr, __retain(s.rp->next) };
      } else {
        return { __appending, s.ok - 1, nullptr, __retain(s.fp), nullptr, __retain(s.rp) };
      }
    }
    default : {
      __rot copy = s;
      __retain_rot(copy);
      return copy;
    }
    }
  }

  // NOTE: takes ownership
  static immutable_queue
  __exec_twice(__node *f, usize fl, __node *r, usize rl, __rot state)
  {
    __rot s1 = __exec(state);
    __release_rot(state);
    __rot s2 = __exec(s1);
    __release_rot(s1);

    if ( s2.phase == __done ) [[unlikely]] {
      //  rotation complete: rp is the new front list
      //  transfer ownership of rp out of s2
      __node *new_f = s2.rp;
      __release(f);     // old front replaced
      return immutable_queue(new_f, fl, r, rl, __idle_rot());
    }

    return immutable_queue(f, fl, r, rl, s2);
  }

  // maintain |r| <= |f| invariant
  // init rotation if violated
  // NOTE: takes ownership
  static immutable_queue
  __check(__node *f, usize fl, __node *r, usize rl, __rot state)
  {
    if ( rl <= fl ) [[likely]] {
      return __exec_twice(f, fl, r, rl, state);
    } else {
      //  invariant violated: begin rotation
      //  Reversing(0, f, nil, r, nil)
      __rot new_state = { __reversing, 0, __retain(f), nullptr, __retain(r), nullptr };
      __release_rot(state);
      __release(r);     // rear now only referenced by rotation state
      return __exec_twice(f, fl + rl, nullptr, 0, new_state);
    }
  }

  __node *__front;
  usize __f_len;
  __node *__rear;
  usize __r_len;
  __rot __state;

  // private constructor for internal copies
  immutable_queue(__node *f, usize fl, __node *r, usize rl, const __rot &s) : __front(f), __f_len(fl), __rear(r), __r_len(rl), __state(s) {}

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
  }

  immutable_queue(void) : __front(nullptr), __f_len(0), __rear(nullptr), __r_len(0), __state(__idle_rot()) {}

  //  O(1) copy
  immutable_queue(const immutable_queue &o)
      : __front(__retain(o.__front)), __f_len(o.__f_len), __rear(__retain(o.__rear)), __r_len(o.__r_len), __state(o.__state)
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
      __front = __retain(o.__front);
      __f_len = o.__f_len;
      __rear = __retain(o.__rear);
      __r_len = o.__r_len;
      __state = o.__state;
      __retain_rot(__state);
    }
    return *this;
  }

  //  O(1) move
  immutable_queue(immutable_queue &&o) noexcept
      : __front(o.__front), __f_len(o.__f_len), __rear(o.__rear), __r_len(o.__r_len), __state(o.__state)
  {
    o.__front = nullptr;
    o.__f_len = 0;
    o.__rear = nullptr;
    o.__r_len = 0;
    o.__state = __idle_rot();
  }

  immutable_queue &
  operator=(immutable_queue &&o) noexcept
  {
    if ( this != &o ) [[likely]] {
      __release(__front);
      __release(__rear);
      __release_rot(__state);
      __front = o.__front;
      __f_len = o.__f_len;
      __rear = o.__rear;
      __r_len = o.__r_len;
      __state = o.__state;
      o.__front = nullptr;
      o.__f_len = 0;
      o.__rear = nullptr;
      o.__r_len = 0;
      o.__state = __idle_rot();
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
    return __check(__retain(__front), __f_len, nr, __r_len + 1, s);
  }

  immutable_queue
  push(T &&v) const
  {
    __node *nr = __make_node(micron::move(v), __retain(__rear));
    __rot s = __state;
    __retain_rot(s);
    return __check(__retain(__front), __f_len, nr, __r_len + 1, s);
  }

  template <typename... Args>
  immutable_queue
  emplace(Args &&...args) const
  {
    //  construct T in-place, then cons onto rear
    auto *n = reinterpret_cast<__node *>(abc::alloc(sizeof(__node)));
    new (micron::addr(n->value)) T(micron::forward<Args>(args)...);
    n->next = __retain(__rear);
    n->refs = 1;

    __rot s = __state;
    __retain_rot(s);
    return __check(__retain(__front), __f_len, n, __r_len + 1, s);
  }

  // O(1)
  immutable_queue
  pop(void) const
  {
    if ( !__front ) [[unlikely]]
      return immutable_queue();

    __rot ns = __invalidate(__state);
    return __check(__retain(__front->next), __f_len - 1, __retain(__rear), __r_len, ns);
  }

  const T &
  front(void) const
  {
    return __front->value;
  }

  const T *
  peek(void) const
  {
    return __front ? &__front->value : nullptr;
  }

  //  O(1) when rear is non-empty
  //  O(n) fallback when rear is empty (immediately after rotation)
  const T &
  last(void) const
  {
    if ( __rear ) [[likely]]
      return __rear->value;
    //  rear empty: walk front to tail
    const __node *n = __front;
    while ( n->next )
      n = n->next;
    return n->value;
  }

  const T &
  at(usize idx) const
  {
    if ( idx >= size() ) [[unlikely]]
      exc<except::library_error>("micron::immutable_queue at(): index out of range");

    if ( idx < __f_len ) {
      const __node *n = __front;
      for ( usize i = 0; i < idx; ++i )
        n = n->next;
      return n->value;
    }

    //  index falls in the rear list (reversed order)
    //  logical index within rear: idx - __f_len
    //  rear list position (from head): __r_len - 1 - (idx - __f_len)
    usize rear_pos = __r_len - 1 - (idx - __f_len);
    const __node *n = __rear;
    for ( usize i = 0; i < rear_pos; ++i )
      n = n->next;
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
    if ( __front == o.__front && __rear == o.__rear ) [[unlikely]]
      return true;
    if ( size() != o.size() )
      return false;
    auto a = begin(), ae = end();
    auto b = o.begin();
    for ( ; a != ae; ++a, ++b ) {
      if ( *a != *b )
        return false;
    }
    return true;
  }

  bool
  operator!=(const immutable_queue &o) const
  {
    return !(*this == o);
  }

  template <typename Fn>
  immutable_queue
  update_front(Fn &&fn) const
  {
    if ( !__front ) [[unlikely]]
      return *this;
    return pop().push(fn(__front->value));
  }

  template <typename Fn>
  void
  for_each(Fn &&fn) const
  {
    const __node *cur = __front;
    while ( cur ) {
      fn(static_cast<const T &>(cur->value));
      cur = cur->next;
    }

    if ( !__rear )
      return;

    constexpr usize __stack_cap = 128;
    if ( __r_len <= __stack_cap ) {
      const __node *stack[__stack_cap];
      usize depth = 0;
      cur = __rear;
      while ( cur ) {
        stack[depth++] = cur;
        cur = cur->next;
      }
      while ( depth > 0 )
        fn(static_cast<const T &>(stack[--depth]->value));
    } else {
      //  heap fallback for very large rear lists
      auto *buf = reinterpret_cast<const __node **>(abc::alloc(__r_len * sizeof(const __node *)));
      usize depth = 0;
      cur = __rear;
      while ( cur ) {
        buf[depth++] = cur;
        cur = cur->next;
      }
      while ( depth > 0 )
        fn(static_cast<const T &>(buf[--depth]->value));
      abc::dealloc(reinterpret_cast<byte *>(buf));
    }
  }

  class const_iterator
  {
    static constexpr usize __max_depth = 128;
    const __node *__cur;                    // current node in front phase
    const __node *__stack[__max_depth];     // rear nodes collected in reverse
    usize __rear_top;                       // next index to read (counts down)

    void
    __load_rear(const __node *rear)
    {
      usize depth = 0;
      const __node *n = rear;
      while ( n && depth < __max_depth ) [[likely]] {
        __stack[depth++] = n;
        n = n->next;
      }
      __rear_top = depth;     // will decrement to access in FIFO order
    }

  public:
    const_iterator() : __cur(nullptr), __rear_top(0) {}

    explicit const_iterator(const __node *front, const __node *rear) : __cur(front), __rear_top(0) { __load_rear(rear); }

    bool
    operator==(const const_iterator &o) const
    {
      if ( __cur != o.__cur )
        return false;
      if ( __cur )
        return true;     // both in front phase, same node
      return __rear_top == o.__rear_top;
    }

    bool
    operator!=(const const_iterator &o) const
    {
      return !(*this == o);
    }

    const T &
    value(void) const
    {
      if ( __cur )
        return __cur->value;
      return __stack[__rear_top - 1]->value;
    }

    const T &
    operator*(void) const
    {
      return value();
    }

    const T *
    operator->(void) const
    {
      if ( __cur )
        return &__cur->value;
      return &__stack[__rear_top - 1]->value;
    }

    const_iterator &
    operator++(void)
    {
      if ( __cur ) {
        __cur = __cur->next;
      } else if ( __rear_top > 0 ) {
        --__rear_top;
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

  const_iterator
  begin(void) const
  {
    if ( empty() ) [[unlikely]]
      return const_iterator();
    return const_iterator(__front, __rear);
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
template <typename T, auto N = 0> using iqueue = immutable_queue<T>;

};     // namespace micron
