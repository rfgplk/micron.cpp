#pragma once

#include <initializer_list>
#include <type_traits>

#include "../algorithm/mem.hpp"
#include "../allocation/chunks.hpp"
#include "../allocator.hpp"
#include "../container_safety.hpp"
#include "../memory/actions.hpp"
#include "../memory/memory.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"
#include "../tags.hpp"
#include "../types.hpp"

#include "vector.hpp"

namespace micron
{
// (Immutable) vector class. ivector, contiguous in memory, O(1) access,
// iterators never invalidated, always safe, immutable, always thread safe as
// fast as raw arrays
template <typename T, class Alloc = micron::allocator_serial<>>
  requires std::is_copy_constructible_v<T> && std::is_move_constructible_v<T>
class ivector : private Alloc, public immutable_memory<T>
{
  // grow container, private - only int. can call
  inline void
  reserve(size_t n)
  {
    immutable_memory<T>::accept_new_memory(this->grow(reinterpret_cast<byte *>(immutable_memory<T>::memory),
                                                      immutable_memory<T>::capacity * sizeof(T), sizeof(T) * n));
  }

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
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;

  typedef T *iterator;
  typedef const T *const_iterator;

  // disallow empty creation, for simplicity
  ivector() = delete;

  // initialize empty with n reserve
  template <typename S = size_t>
    requires std::is_arithmetic_v<S>
  ivector(S n) : immutable_memory<T>(this->create(n * sizeof(T)))
  {
    immutable_memory<T>::length = 0;
  };
  // two main functions when it comes to copying over data
  ivector(const ivector &o) : immutable_memory<T>(this->create(o.capacity()))
  {
    micron::memcpy256(&immutable_memory<T>::memory[0], o.itr(),
                      o.capacity);     // always is page aligned, 256 is fine.
    immutable_memory<T>::length = o.size();
  }
  ivector &
  operator=(const ivector &o)
  {
    if ( o.capacity >= immutable_memory<T>::capacity )
      reserve(o.capacity);
    micron::memcpy256(&immutable_memory<T>::memory[0], o.itr(),
                      o.capacity);     // always is page aligned, 256 is fine.
    immutable_memory<T>::length = o.size();
  };
  // data must be page aligned and cleanly / 256, otherwise will not function
  // correctly. if using an avx512 cpu, replace with 512 (but not needed,
  // will be faster like this regardless)

  // identical to regular vector
  ivector(const std::initializer_list<T> &lst) : immutable_memory<T>(this->create(sizeof(T) * lst.size()))
  {
    if constexpr ( std::is_class_v<T> ) {
      size_t i = 0;
      for ( T &&value : lst ) {
        new (&contiguous_memory_no_copy<T>::memory[i++]) T(micron::move(value));
      }
      contiguous_memory_no_copy<T>::length = lst.size();
    } else {
      size_t i = 0;
      for ( T value : lst ) {
        contiguous_memory_no_copy<T>::memory[i++] = value;
      }
      contiguous_memory_no_copy<T>::length = lst.size();
    }
  };
  ivector(const vector<T>& o)
 : immutable_memory<T>(this->create(o.size()))
  {
    micron::memcpy(&immutable_memory<T>::memory[0], o.data(), o.size());
    immutable_memory<T>::length = o.size();
  }
  ivector(size_t n) : immutable_memory<T>(this->create(n * sizeof(T)))
  {
    for ( size_t i = 0; i < n; i++ )
      new (&immutable_memory<T>::memory[i]) T();
    immutable_memory<T>::length = n;
  };
  template <typename... Args> ivector(size_t n, Args... args) : immutable_memory<T>(this->create(n * sizeof(T)))
  {
    for ( size_t i = 0; i < n; i++ )
      new (&immutable_memory<T>::memory[i]) T(args...);
    immutable_memory<T>::length = n;
  };

  ivector(size_t n, const T &init_value) : immutable_memory<T>(this->create(n * sizeof(T)))
  {
    for ( size_t i = 0; i < n; i++ )
      new (&immutable_memory<T>::memory[i]) T(init_value);
    immutable_memory<T>::length = n;
  };
  ivector(size_t n, T &&init_value) : immutable_memory<T>(this->create(n * sizeof(T)))
  {
    for ( size_t i = 0; i < n; i++ )
      new (&immutable_memory<T>::memory[i]) T(micron::move(init_value));
    immutable_memory<T>::length = n;
  };
  ivector(chunk<byte> *m) : immutable_memory<T>(m) {};
  ivector(chunk<byte> *&&m) : immutable_memory<T>(m) { m = nullptr; };
  template <typename C = T> ivector(ivector<C> &&o) : immutable_memory<T>(o.data()) { o.~immutable_memory<T>(); }
  ivector &
  operator=(ivector &&o)
  {
    immutable_memory<T>::memory = o.memory;
    immutable_memory<T>::length = o.length;
    immutable_memory<T>::capacity = o.capacity;
    o.memory = 0;
    o.length = 0;
    o.capacity = 0;
    return *this;
  }
  ~ivector()
  {
    if ( immutable_memory<T>::memory == nullptr )
      return;

    if constexpr ( std::is_class<T>::value ) {
      for ( size_t i = 0; i < immutable_memory<T>::length; i++ )
        (immutable_memory<T>::memory)[i].~T();
    }
    this->destroy(to_chunk(immutable_memory<T>::memory, immutable_memory<T>::capacity));
  }
  // equivalent of .data() sortof
  chunk<byte>
  operator*()
  {
    return { reinterpret_cast<byte *>(immutable_memory<T>::memory), immutable_memory<T>::capacity };
  }
  // always direct
  inline const T &
  operator[](size_t n) const
  {
    return (immutable_memory<T>::memory)[n];
  }
  T
  at(size_t n) const
  {
    if ( n >= immutable_memory<T>::length )
      throw except::library_error("micron::ivector at() out of bounds");
    return (immutable_memory<T>::memory)[n];
  }
  size_t
  at_n(iterator i) const
  {
    if ( i - begin() >= immutable_memory<T>::length )
      throw except::library_error("micron::ivector at_n() out of bounds");
    return static_cast<size_t>(i - begin());
  }
  // return const iterator, immutable
  const_iterator
  itr(size_t n) const
  {
    if ( n >= immutable_memory<T>::length )
      throw except::library_error("micron::ivector itr() out of bounds");
    return &(immutable_memory<T>::memory)[n];
  }
  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline ivector<T>
  append(const ivector<F> &o) const
  {
    micron::ivector<T> buf(immutable_memory<T>::capacity + o.capacity);
    __impl_copy(&buf.memory[0], &immutable_memory<T>::memory[0], immutable_memory<T>::capacity);
    __impl_copy(&buf.memory[immutable_memory<T>::length], &o.memory[0], o.capacity);
    buf.length = immutable_memory<T>::length + o.length;
    return buf;
  }
  template <typename C = T>
  void
  swap(ivector<C> &&o)
  {
    micron::swap(immutable_memory<T>::memory, o.memory);
    micron::swap(immutable_memory<T>::length, o.length);
    micron::swap(immutable_memory<T>::capacity, o.capacity);
  }
  size_t
  capacity() const
  {
    return immutable_memory<T>::capacity;
  }
  size_t
  size() const
  {
    return immutable_memory<T>::length;
  }

  // no resize
  // + instead of +=
  ivector
  operator+(const ivector &o)
  {
    return append(o);     // equivalent
  }

  template <typename... Args>
  inline ivector
  emplace_back(Args &&...v)
  {
    micron::ivector<T> buf(immutable_memory<T>::capacity + sizeof...(Args) * sizeof(T));
    __impl_copy(&buf.memory[0], &immutable_memory<T>::memory[0], immutable_memory<T>::capacity);
    buf.length = immutable_memory<T>::length + 1;     // by one

    new (&buf.memory[buf.size()]) T(micron::move(micron::forward<Args>(v)...));
  }

  inline const_iterator
  get(const size_t n)
  {
    if ( n > immutable_memory<T>::length )
      throw except::library_error("micron::ivector get() out of range");
    return &(immutable_memory<T>::memory[n]);
  }
  inline const_iterator
  cget(const size_t n)
  {
    if ( n > immutable_memory<T>::length )
      throw except::library_error("micron::ivector cget() out of range");
    return &(immutable_memory<T>::memory[n]);
  }
  inline const_iterator
  find(const T &o)
  {
    T *f_ptr = immutable_memory<T>::memory;
    for ( size_t i = 0; i < immutable_memory<T>::length; i++ )
      if ( f_ptr[i] == o )
        return &f_ptr[i];
    return nullptr;
  }
  inline const_iterator
  begin() const
  {
    return (immutable_memory<T>::memory);
  }
  inline iterator
  begin()
  {
    return (immutable_memory<T>::memory);
  }
  inline const_iterator
  cbegin() const
  {
    return (immutable_memory<T>::memory);
  }
  inline iterator
  end()
  {
    return (immutable_memory<T>::memory) + (immutable_memory<T>::length);
  }
  inline const_iterator
  end() const
  {
    return (immutable_memory<T>::memory) + (immutable_memory<T>::length);
  }
  inline const_iterator
  cend() const
  {
    return (immutable_memory<T>::memory) + (immutable_memory<T>::length);
  }
  inline slice<byte>
  into_bytes()
  {
    return slice<byte>(reinterpret_cast<byte *>(&immutable_memory<T>::memory[0]),
                       reinterpret_cast<byte *>(&immutable_memory<T>::memory[immutable_memory<T>::length]));
  }
  inline ivector<T>
  insert(size_t n, const T &val)
  {
    micron::ivector<T> buf(immutable_memory<T>::capacity + sizeof(T));
    __impl_copy(&buf.memory[0], &immutable_memory<T>::memory[0], immutable_memory<T>::capacity);
    auto i = immutable_memory<T>::length;
    buf.length = i + 1;
    T *its = &(buf.memory)[n];
    T *ite = &(buf.memory)[i - 1];
    micron::memmove(its + 1, its, ite - its);
    //*its = (val);
    new (its) T(val);
    return buf;
  }
  inline ivector<T>
  insert(size_t n, T &&val)
  {
    micron::ivector<T> buf(immutable_memory<T>::capacity + sizeof(T));
    __impl_copy(&buf.memory[0], &immutable_memory<T>::memory[0], immutable_memory<T>::capacity);
    auto i = immutable_memory<T>::length;
    buf.length = i + 1;
    T *its = &(buf.memory)[n];
    T *ite = &(buf.memory)[i - 1];
    micron::memmove(its + 1, its, ite - its);
    //*its = (val);
    new (its) T(val);
    return buf;
  }

  inline ivector<T>
  insert(const_iterator it, T &&val)
  {
    micron::ivector<T> buf(immutable_memory<T>::capacity + sizeof(T));
    __impl_copy(&buf.memory[0], &immutable_memory<T>::memory[0], immutable_memory<T>::capacity);
    auto i = immutable_memory<T>::length;
    buf.length = i + 1;
    T *ite = &(buf.memory)[i - 1];
    micron::memmove(it + 1, it, ite - it);
    new (it) T(micron::move(val));
    return buf;
  }
  inline ivector<T>
  insert(const_iterator it, const T &val)
  {
    micron::ivector<T> buf(immutable_memory<T>::capacity + sizeof(T));
    __impl_copy(&buf.memory[0], &immutable_memory<T>::memory[0], immutable_memory<T>::capacity);
    auto i = immutable_memory<T>::length;
    buf.length = i + 1;
    T *ite = &(buf.memory)[i - 1];
    micron::memmove(it + 1, it, ite - it);
    new (it) T(val);
    return buf;
  }
  inline ivector<T>
  assign(const size_t cnt, const T &val)
  {
    micron::ivector<T> buf(immutable_memory<T>::capacity + (sizeof(T) * cnt));
    auto *i = buf.begin();
    for ( size_t j = 0; j < cnt; j++ ) {
      new ((T *)(i + j)) T(val);
    }
    buf.length = immutable_memory<T>::length + cnt;
    return buf;
  }
  inline ivector<T>
  push_back(const T &v)
  {
    micron::ivector<T> buf(immutable_memory<T>::capacity + (sizeof(T)));
    __impl_copy(&buf.memory[0], &immutable_memory<T>::memory[0], immutable_memory<T>::capacity);

    new (&buf.itr() + immutable_memory<T>::length) T(v);

    buf.length = immutable_memory<T>::length + 1;
    return buf;
  }
  inline ivector<T>
  push_back(T &&v)
  {
    micron::ivector<T> buf(immutable_memory<T>::capacity + (sizeof(T)));
    __impl_copy(&buf.memory[0], &immutable_memory<T>::memory[0], immutable_memory<T>::capacity);

    new (buf.itr() + immutable_memory<T>::length) T(micron::move(v));

    buf.length = immutable_memory<T>::length + 1;
    return buf;
  }
  inline ivector<T>
  push_front(const T &v)
  {
    micron::ivector<T> buf(immutable_memory<T>::capacity + (sizeof(T)));
    __impl_copy(&buf.memory[1], &immutable_memory<T>::memory[0], immutable_memory<T>::capacity);

    new (&buf.itr()) T(v);

    buf.length = immutable_memory<T>::length + 1;
    return buf;
  }
  inline ivector<T>
  push_front(T &&v)
  {
    micron::ivector<T> buf(immutable_memory<T>::capacity + (sizeof(T)));
    __impl_copy(&buf.memory[1], &immutable_memory<T>::memory[0], immutable_memory<T>::capacity);

    new (buf.itr()) T(micron::move(v));

    buf.length = immutable_memory<T>::length + 1;
    return buf;
  }

  inline void
  erase(const_iterator n)
  {
    if constexpr ( std::is_class<T>::value ) {
      *n->~T();
    } else {
    }
    for ( size_t i = n; i < (immutable_memory<T>::length - 1); i++ )
      (*n)[i] = micron::move((immutable_memory<T>::memory)[i + 1]);

    czero<sizeof(T) / sizeof(byte)>(
        (byte *)micron::voidify(&(immutable_memory<T>::memory)[immutable_memory<T>::length-- - 1]));
  }
  inline void
  erase(const size_t n)
  {
    if constexpr ( std::is_class<T>::value ) {
      ~(immutable_memory<T>::memory)[n]();
    } else {
    }
    for ( size_t i = n; i < (immutable_memory<T>::length - 1); i++ )
      (immutable_memory<T>::memory)[i] = micron::move((immutable_memory<T>::memory)[i + 1]);
    czero<sizeof(T) / sizeof(byte)>(
        (byte *)micron::voidify(&(immutable_memory<T>::memory)[immutable_memory<T>::length-- - 1]));
    immutable_memory<T>::length--;
  }
  inline ivector<T>
  clear()
  {
    return ivector<T>(micron::move(immutable_memory<T>::capacity));
  }
  inline T
  front()
  {
    return (immutable_memory<T>::memory)[0];
  }
  inline T
  back()
  {
    return (immutable_memory<T>::memory)[immutable_memory<T>::length - 1];
  }
  // access at element
};
template <typename T>
auto
to_persist(micron::vector<T> &vec)
{
  return ivector<T>();
}

};     // namespace micron
