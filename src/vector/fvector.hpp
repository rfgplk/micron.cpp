//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include <initializer_list>     // nigh impossible to implement without invoking the darkest of sorceries :c
#include <type_traits>

#include "../algorithm/algorithm.hpp"
#include "../algorithm/mem.hpp"
#include "../allocation/chunks.hpp"
#include "../allocator.hpp"
#include "../container_safety.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/memory.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"
#include "../sort/heap.hpp"
#include "../sort/quick.hpp"
#include "../tags.hpp"
#include "../types.hpp"
namespace micron
{
// Fast vector class, equivalent to vector but with no bounds checking
template <typename T, class Alloc = micron::allocator_serial<>>
  requires std::is_copy_constructible_v<T> && std::is_move_constructible_v<T>
class fvector : private Alloc, public contiguous_memory_no_copy<T>
{

  // shallow copy routine
  inline void
  shallow_copy(T *dest, T *src, size_t cnt)
  {
    micron::memcpy256(reinterpret_cast<byte *>(dest), reinterpret_cast<byte *>(src),
                      cnt * (sizeof(T) / sizeof(byte)));     // always is page aligned, 256 is
                                                             // fine, just realign back to bytes
  };
  // deep copy routine, nec. if obj. has const/dest (can be ignored but WILL
  // cause segfaulting if underlying doesn't account for double deletes)
  inline void
  deep_copy(T *dest, T *src, size_t cnt)
  {
    for ( size_t i = 0; i < cnt; i++ )
      dest[i] = src[i];
  };
  inline void
  __impl_copy(T *dest, T *src, size_t cnt)
  {
    if constexpr ( std::is_class<T>::value ) {
      deep_copy(dest, src, cnt);
    } else {
      shallow_copy(dest, src, cnt);
    }
  }

public:
  using category_type = vector_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef size_t size_type;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;
  ~fvector()
  {
    if ( contiguous_memory_no_copy<T>::memory == nullptr )
      return;
    clear();
    this->destroy(to_chunk(contiguous_memory_no_copy<T>::memory, contiguous_memory_no_copy<T>::capacity));
  }
  fvector(const std::initializer_list<T> &lst) : contiguous_memory_no_copy<T>(this->create(sizeof(T) * lst.size()))
  {
    size_t i = 0;
    for ( T value : lst ) {
      new (&contiguous_memory_no_copy<T>::memory[i++]) T(micron::move(value));
    }
    contiguous_memory_no_copy<T>::length = lst.size();
  };
  fvector()
      : contiguous_memory_no_copy<T>(this->create((Alloc::auto_size() >= sizeof(T) ? Alloc::auto_size() : sizeof(T)))) {
        };
  fvector(size_t n) : contiguous_memory_no_copy<T>(this->create(n * sizeof(T)))
  {
    for ( size_t i = 0; i < n; i++ )
      new (&contiguous_memory_no_copy<T>::memory[i]) T();
    contiguous_memory_no_copy<T>::length = n;
  };
  template <typename... Args> fvector(size_t n, Args... args) : contiguous_memory_no_copy<T>(this->create(n * sizeof(T)))
  {
    for ( size_t i = 0; i < n; i++ )
      new (&contiguous_memory_no_copy<T>::memory[i]) T(args...);
    contiguous_memory_no_copy<T>::length = n;
  };

  fvector(size_t n, const T &init_value) : contiguous_memory_no_copy<T>(this->create(n * sizeof(T)))
  {
    for ( size_t i = 0; i < n; i++ )
      new (&contiguous_memory_no_copy<T>::memory[i]) T(init_value);
    contiguous_memory_no_copy<T>::length = n;
  };
  fvector(size_t n, T &&init_value) : contiguous_memory_no_copy<T>(this->create(n * sizeof(T)))
  {
    for ( size_t i = 0; i < n; i++ )
      new (&contiguous_memory_no_copy<T>::memory[i]) T(micron::move(init_value));
    contiguous_memory_no_copy<T>::length = n;
  };
  fvector(const fvector &) = delete;     // reg vectors SHOULDNT be copied, for perf
  fvector(chunk<byte> *m) : contiguous_memory_no_copy<T>(m) {};
  fvector(chunk<byte> *&&m) : contiguous_memory_no_copy<T>(m) { m = nullptr; };
  template <typename C = T> fvector(fvector<C> &&o) : contiguous_memory_no_copy<T>(micron::move(o)) {}
  fvector(fvector &&o) : contiguous_memory_no_copy<T>(micron::move(o)) {}
  fvector &operator=(const fvector &) = delete;     // same reasoning as above
  fvector &
  operator=(fvector &&o)
  {
    if ( contiguous_memory_no_copy<T>::memory ) {
      // kill old memory first
      clear();
      this->destroy(to_chunk(contiguous_memory_no_copy<T>::memory, contiguous_memory_no_copy<T>::capacity));
    }
    contiguous_memory_no_copy<T>::memory = o.memory;
    contiguous_memory_no_copy<T>::length = o.length;
    contiguous_memory_no_copy<T>::capacity = o.capacity;
    o.memory = 0;
    o.length = 0;
    o.capacity = 0;
    return *this;
  }
  template <typename... Args>
  fvector &
  operator+=(Args &&...args)
  {
    push_back(micron::move(args)...);
    return *this;
  }
  // equivalent of .data() sortof
  chunk<byte>
  operator*()
  {
    return { reinterpret_cast<byte *>(contiguous_memory_no_copy<T>::memory), contiguous_memory_no_copy<T>::capacity };
  }
  bool
  operator!() const
  {
    return empty();
  }
  // overload this to always point to mem
  byte *
  operator&() volatile
  {
    return reinterpret_cast<byte *>(contiguous_memory_no_copy<T>::memory);
  }

  inline __attribute__((always_inline)) slice<T>
  operator[]()
  {
    // meant to be safe so this is here
    return slice<T>(begin(), last());
  }
  inline __attribute__((always_inline)) const slice<T>
  operator[]() const
  {
    // meant to be safe so this is here
    return slice<T>(begin(), last());
  }
  // copies vector out
  inline __attribute__((always_inline)) const slice<T>
  operator[](size_t from, size_t to) const
  {
    return slice<T>(get(from), get(to));
  }
  inline __attribute__((always_inline)) slice<T>
  operator[](size_t from, size_t to)
  {
    return slice<T>(get(from), get(to));
  }
  inline __attribute__((always_inline)) const T &
  operator[](size_t n) const
  {
    return (contiguous_memory_no_copy<T>::memory)[n];
  }
  inline __attribute__((always_inline)) T &
  operator[](size_t n)
  {
    return (contiguous_memory_no_copy<T>::memory)[n];
  }
  inline __attribute__((always_inline)) T &
  at(size_t n)
  {
    return (contiguous_memory_no_copy<T>::memory)[n];
  }
  size_t
  at_n(iterator i) const
  {
    return static_cast<size_t>(i - begin());
  }
  T *
  itr(size_t n)
  {
    return &(contiguous_memory_no_copy<T>::memory)[n];
  }
  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline fvector &
  append(const fvector<F> &o)
  {
    if(o.empty())
      return *this;
    if ( contiguous_memory_no_copy<T>::length + o.length >= contiguous_memory_no_copy<T>::capacity )
      reserve(contiguous_memory_no_copy<T>::capacity + o.max_size());
    __impl_copy(micron::addr(contiguous_memory_no_copy<T>::memory[contiguous_memory_no_copy<T>::length]),
                micron::addr(o.memory[0]), o.length);
    // micron::memcpy(&(contiguous_memory_no_copy<T>::memory)[contiguous_memory_no_copy<T>::length],
    // &o.memory[0],
    //                o.length);
    contiguous_memory_no_copy<T>::length += o.length;
    return *this;
  }
  template <typename C = T>
  void
  swap(fvector<C> &o)
  {
    auto tmp = contiguous_memory_no_copy<C>(contiguous_memory_no_copy<T>::memory, contiguous_memory_no_copy<T>::length,
                                            contiguous_memory_no_copy<T>::capacity);
    contiguous_memory_no_copy<T>::memory = o.memory;
    contiguous_memory_no_copy<T>::length = o.length;
    contiguous_memory_no_copy<T>::capacity = o.capacity;
    o.memory = tmp.memory;
    o.length = tmp.length;
    o.capacity = tmp.capacity;
  }
  size_t
  max_size() const
  {
    return contiguous_memory_no_copy<T>::capacity;
  }
  size_t
  size() const
  {
    return contiguous_memory_no_copy<T>::length;
  }
  void
  set_size(const size_t n)
  {
    contiguous_memory_no_copy<T>::length = n;
  }
  bool
  empty() const
  {
    return contiguous_memory_no_copy<T>::length == 0 ? true : false;
  }

  // grow container
  inline void
  reserve(const size_t n)
  {
    if ( n < contiguous_memory_no_copy<T>::capacity )
      return;
    contiguous_memory_no_copy<T>::accept_new_memory(
        this->grow(reinterpret_cast<byte *>(contiguous_memory_no_copy<T>::memory),
                   contiguous_memory_no_copy<T>::capacity * sizeof(T), sizeof(T) * n));
  }
  inline void
  try_reserve(const size_t n)
  {
    if ( n < contiguous_memory_no_copy<T>::capacity )
      throw except::memory_error("micron vector failed to reserve memory");
    contiguous_memory_no_copy<T>::accept_new_memory(
        this->grow(reinterpret_cast<byte *>(contiguous_memory_no_copy<T>::memory),
                   contiguous_memory_no_copy<T>::capacity * sizeof(T), sizeof(T) * n));
  }
  inline slice<byte>
  into_bytes()
  {
    return slice<byte>(
        reinterpret_cast<byte *>(&contiguous_memory_no_copy<T>::memory[0]),
        reinterpret_cast<byte *>(&contiguous_memory_no_copy<T>::memory[contiguous_memory_no_copy<T>::length]));
  }
  inline fvector<T>
  clone(void)
  {
    return fvector<T>(*this);
  }
  template <typename F>
  inline F
  clone_to(void)
  {
    return F(*this);
  }
  void
  fill(const T &v)
  {
    for ( size_t i = 0; i < contiguous_memory_no_copy<T>::length; i++ )
      contiguous_memory_no_copy<T>::memory[i] = v;
  }
  void
  recreate(const size_t n = 0)     // needed if moved out
  {
    if ( contiguous_memory_no_copy<T>::memory == nullptr ) {
      if ( !n )
        contiguous_memory_no_copy<T>::accept_new_memory(
            this->create((Alloc::auto_size() >= sizeof(T) ? Alloc::auto_size() : sizeof(T))));
      else
        contiguous_memory_no_copy<T>::accept_new_memory(this->create(n * sizeof(T)));
    }
  }
  // resize to how much and fill with a value v
  void
  resize(size_t n, const T &v)
  {
    if ( !(n > contiguous_memory_no_copy<T>::length) ) {
      return;
    }
    if ( n >= contiguous_memory_no_copy<T>::capacity ) {
      reserve(sizeof(T) * n);
    }
    T *f_ptr = contiguous_memory_no_copy<T>::memory;
    for ( size_t i = contiguous_memory_no_copy<T>::length; i < n; i++ )
      new (&f_ptr[i]) T(v);

    contiguous_memory_no_copy<T>::length = n;
  }
  void
  resize(size_t n)
  {
    if ( !(n > contiguous_memory_no_copy<T>::length) ) {
      return;
    }
    if ( n >= contiguous_memory_no_copy<T>::capacity ) {
      reserve(sizeof(T) * n);
    }
    T *f_ptr = contiguous_memory_no_copy<T>::memory;
    for ( size_t i = contiguous_memory_no_copy<T>::length; i < n; i++ )
      new (&f_ptr[i]) T{};

    contiguous_memory_no_copy<T>::length = n;
  }
  template <typename... Args>
    requires(std::is_class_v<T>)
  inline void
  emplace_back(Args &&...v)
  {
    if ( contiguous_memory_no_copy<T>::length < contiguous_memory_no_copy<T>::capacity ) {
      new (&contiguous_memory_no_copy<T>::memory[contiguous_memory_no_copy<T>::length++])
          T(micron::move(micron::forward<Args>(v)...));
      return;
    } else {
      reserve(contiguous_memory_no_copy<T>::capacity + 1);
      new (&contiguous_memory_no_copy<T>::memory[contiguous_memory_no_copy<T>::length++])
          T(micron::move(micron::forward<Args>(v)...));
    }
  }
  template <typename V = T>
    requires(!std::is_class_v<T>) && (!std::is_class_v<V>)
  inline void emplace_back(V v)
  {
    if ( contiguous_memory_no_copy<T>::length < contiguous_memory_no_copy<T>::capacity ) {
      contiguous_memory_no_copy<T>::memory[contiguous_memory_no_copy<T>::length++] = static_cast<T>(v);
      return;
    } else {
      reserve(contiguous_memory_no_copy<T>::capacity + 1);
      contiguous_memory_no_copy<T>::memory[contiguous_memory_no_copy<T>::length++] = static_cast<T>(v);
    }
  }
  inline iterator
  get(const size_t n)
  {
    return &(contiguous_memory_no_copy<T>::memory[n]);
  }
  inline const_iterator
  get(const size_t n) const
  {
    return &(contiguous_memory_no_copy<T>::memory[n]);
  }
  inline const_iterator
  cget(const size_t n) const
  {
    return &(contiguous_memory_no_copy<T>::memory[n]);
  }
  inline iterator
  find(const T &o)
  {
    T *f_ptr = contiguous_memory_no_copy<T>::memory;
    for ( size_t i = 0; i < contiguous_memory_no_copy<T>::length; i++ )
      if ( f_ptr[i] == o )
        return &f_ptr[i];
    return nullptr;
  }
  inline const_iterator
  find(const T &o) const
  {
    T *f_ptr = contiguous_memory_no_copy<T>::memory;
    for ( size_t i = 0; i < contiguous_memory_no_copy<T>::length; i++ )
      if ( f_ptr[i] == o )
        return &f_ptr[i];
    return nullptr;
  }
  inline iterator
  begin()
  {
    return (contiguous_memory_no_copy<T>::memory);
  }
  inline const_iterator
  begin() const
  {
    return (contiguous_memory_no_copy<T>::memory);
  }
  inline const_iterator
  cbegin() const
  {
    return (contiguous_memory_no_copy<T>::memory);
  }
  inline iterator
  end()
  {
    return (contiguous_memory_no_copy<T>::memory) + (contiguous_memory_no_copy<T>::length);
  }
  inline const_iterator
  end() const
  {
    return (contiguous_memory_no_copy<T>::memory) + (contiguous_memory_no_copy<T>::length);
  }
  inline iterator
  last()
  {
    return (contiguous_memory_no_copy<T>::memory) + (contiguous_memory_no_copy<T>::length - 1);
  }
  inline const_iterator
  last() const
  {
    return (contiguous_memory_no_copy<T>::memory) + (contiguous_memory_no_copy<T>::length - 1);
  }
  inline const_iterator
  cend() const
  {
    return (contiguous_memory_no_copy<T>::memory) + (contiguous_memory_no_copy<T>::length);
  }
  inline iterator
  insert(size_t n, const T &val)
  {
    if ( !contiguous_memory_no_copy<T>::length ) {
      push_back(val);
      return begin();
    }
    if ( contiguous_memory_no_copy<T>::length + 1 > contiguous_memory_no_copy<T>::capacity )
      reserve(contiguous_memory_no_copy<T>::capacity + 1);
    T *its = &(contiguous_memory_no_copy<T>::memory)[n];
    T *ite = &(contiguous_memory_no_copy<T>::memory)[contiguous_memory_no_copy<T>::length - 1];
    micron::memmove(its + 1, its, ite - its);
    //*its = (val);
    new (its) T(val);
    contiguous_memory_no_copy<T>::length++;
    return its;
  }
  inline iterator
  insert_at(size_t n, T &&val)
  {
    if ( !contiguous_memory_no_copy<T>::length ) {
      push_back(val);
      return begin();
    }
    if ( contiguous_memory_no_copy<T>::length + 1 > contiguous_memory_no_copy<T>::capacity )
      reserve(contiguous_memory_no_copy<T>::capacity + 1);
    T *its = itr(n);
    T *ite = end();
    micron::memmove(its + 1, its, (ite - its));
    //*its = (val);
    new (its) T(micron::move(val));
    contiguous_memory_no_copy<T>::length++;
    return its;
  }

  inline iterator
  insert(iterator it, T &&val)
  {
    if ( !contiguous_memory_no_copy<T>::length ) {
      push_back(val);
      return begin();
    }
    if ( (contiguous_memory_no_copy<T>::length + sizeof(T)) >= contiguous_memory_no_copy<T>::capacity ) {
      size_t dif = static_cast<size_t>(it - contiguous_memory_no_copy<T>::memory);
      reserve(contiguous_memory_no_copy<T>::capacity + 1);
      it = contiguous_memory_no_copy<T>::memory + dif;
    }     // invalidated if
    T *ite = end();
    micron::memmove(it + 1, it, ite - it);
    new (it) T(micron::move(val));
    //*it = (val);
    contiguous_memory_no_copy<T>::length++;
    return it;
  }
  inline iterator
  insert(iterator it, const T &val)
  {
    if ( !contiguous_memory_no_copy<T>::length ) {
      push_back(val);
      return begin();
    }
    if ( contiguous_memory_no_copy<T>::length >= contiguous_memory_no_copy<T>::capacity ) {
      size_t dif = it - contiguous_memory_no_copy<T>::memory;
      reserve(contiguous_memory_no_copy<T>::capacity + 1);
      it = contiguous_memory_no_copy<T>::memory + dif;
    }
    T *ite = end();
    micron::memmove(it + 1, it, ite - it);
    //*it = (val);
    new (it) T(val);
    contiguous_memory_no_copy<T>::length++;
    return it;
  }
  inline void
  sort(void)
  {
    if ( size() < 100000 )
      micron::sort::heap(*this);     // NOTE: need an inplace sort since *this cannot be copied
    else
      micron::sort::quick(*this);
  }
  inline iterator
  insert_sort(T &&val)     // NOTE: we won't check if this is presort, bad things will happen if it isn't
  {
    if ( !contiguous_memory_no_copy<T>::length ) {
      push_back(val);
      return begin();
    }
    if ( (contiguous_memory_no_copy<T>::length + 1) >= contiguous_memory_no_copy<T>::capacity ) {
      reserve(contiguous_memory_no_copy<T>::capacity + 1);
    }     // invalidated if
    T *ite = end();
    size_t i = 0;
    size_t lm = size();
    for ( ; i < lm; i++ ) {
      if ( contiguous_memory_no_copy<T>::memory[i] >= val ) {
        break;
      }
    }
    T *it = &contiguous_memory_no_copy<T>::memory[i];
    micron::memmove(it + 1, it, ite - it);
    new (it) T(micron::move(val));
    contiguous_memory_no_copy<T>::length++;
    return it;
  }

  inline fvector &
  assign(const size_t cnt, const T &val)
  {
    if ( (cnt * (sizeof(T) / sizeof(byte))) >= contiguous_memory_no_copy<T>::capacity ) {
      reserve(contiguous_memory_no_copy<T>::capacity + (cnt) * sizeof(T));
    }
    clear();     // clear the vec
    for ( size_t i = 0; i < cnt; i++ ) {
      //(contiguous_memory_no_copy<T>::memory)[i] = val;
      new (&contiguous_memory_no_copy<T>::memory[i]) T(val);
    }
    contiguous_memory_no_copy<T>::length = cnt;
    return *this;
  }
  inline void
  push_back(const T &v)
  {
    if ( (contiguous_memory_no_copy<T>::length + 1 <= contiguous_memory_no_copy<T>::capacity) ) {
      new (&contiguous_memory_no_copy<T>::memory[contiguous_memory_no_copy<T>::length++]) T(v);
      return;
    } else {
      reserve(contiguous_memory_no_copy<T>::capacity * sizeof(T) + 1);
      new (&contiguous_memory_no_copy<T>::memory[contiguous_memory_no_copy<T>::length++]) T(v);
    }
  }
  inline void
  push_back(T &&v)
  {
    if ( (contiguous_memory_no_copy<T>::length + 1) <= contiguous_memory_no_copy<T>::capacity ) {
      new (&contiguous_memory_no_copy<T>::memory[contiguous_memory_no_copy<T>::length++]) T(micron::move(v));
      return;
    } else {
      reserve(contiguous_memory_no_copy<T>::capacity * sizeof(T) + 1);
      new (&contiguous_memory_no_copy<T>::memory[contiguous_memory_no_copy<T>::length++]) T(micron::move(v));
    }
  }

  inline void
  pop_back()
  {
    if constexpr ( std::is_class<T>::value ) {
      (contiguous_memory_no_copy<T>::memory)[(contiguous_memory_no_copy<T>::length - 1)].~T();
    } else {
      (contiguous_memory_no_copy<T>::memory)[(contiguous_memory_no_copy<T>::length - 1)] = 0x0;
    }
    czero<sizeof(T) / sizeof(byte)>(
        (byte *)micron::voidify(&(contiguous_memory_no_copy<T>::memory)[contiguous_memory_no_copy<T>::length-- - 1]));
  }

  inline void
  erase(const_iterator n)
  {
    if constexpr ( std::is_class<T>::value ) {
      *n->~T();
    } else {
    }
    for ( size_t i = n; i < (contiguous_memory_no_copy<T>::length - 1); i++ )
      (*n)[i] = micron::move((contiguous_memory_no_copy<T>::memory)[i + 1]);

    czero<sizeof(T) / sizeof(byte)>(
        (byte *)micron::voidify(&(contiguous_memory_no_copy<T>::memory)[contiguous_memory_no_copy<T>::length-- - 1]));
  }
  inline void
  erase(const size_t n)
  {
    if constexpr ( std::is_class<T>::value ) {
      ~(contiguous_memory_no_copy<T>::memory)[n]();
    } else {
    }
    for ( size_t i = n; i < (contiguous_memory_no_copy<T>::length - 1); i++ )
      (contiguous_memory_no_copy<T>::memory)[i] = micron::move((contiguous_memory_no_copy<T>::memory)[i + 1]);
    czero<sizeof(T) / sizeof(byte)>(
        (byte *)micron::voidify(&(contiguous_memory_no_copy<T>::memory)[contiguous_memory_no_copy<T>::length-- - 1]));
    contiguous_memory_no_copy<T>::length--;
  }
  inline void
  clear()
  {
    if ( !contiguous_memory_no_copy<T>::length )
      return;
    if constexpr ( std::is_class<T>::value ) {
      for ( size_t i = 0; i < contiguous_memory_no_copy<T>::length; i++ )
        (contiguous_memory_no_copy<T>::memory)[i].~T();
    }
    micron::zero((byte *)micron::voidify(&(contiguous_memory_no_copy<T>::memory)[0]),
                 contiguous_memory_no_copy<T>::capacity * (sizeof(T) / sizeof(byte)));
    contiguous_memory_no_copy<T>::length = 0;
  }

  inline const T &
  front() const
  {
    return (contiguous_memory_no_copy<T>::memory)[0];
  }
  inline const T &
  back() const
  {
    return (contiguous_memory_no_copy<T>::memory)[contiguous_memory_no_copy<T>::length - 1];
  }
  inline T &
  front()
  {
    return (contiguous_memory_no_copy<T>::memory)[0];
  }
  inline T &
  back()
  {
    return (contiguous_memory_no_copy<T>::memory)[contiguous_memory_no_copy<T>::length - 1];
  }
  // access at element
};
};     // namespace micron
