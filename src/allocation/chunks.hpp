#pragma once

#include "../types.hpp"
#include "allocate_linux.hpp"

namespace micron
{

// universal contiguous memory object, allows copying & moving.
// meant to wrap a T* from the allocator
template <typename T> struct contiguous_memory {
  contiguous_memory() : memory(nullptr), length(0), capacity(0) {}
  contiguous_memory(T *ptr, size_t a, size_t b) : memory(ptr), length(a), capacity(b) {};
  contiguous_memory(chunk<byte> &&o)
      : memory(reinterpret_cast<T *>(o.ptr)), length(0), capacity(o.len / (sizeof(T) / sizeof(byte))) {};
  contiguous_memory(chunk<byte> &o)
      : memory(reinterpret_cast<T *>(o.ptr)), length(0), capacity(o.len / (sizeof(T) / sizeof(byte))) {};
  contiguous_memory(chunk<byte> *o)
      : memory(reinterpret_cast<T *>(o->ptr)), length(0), capacity(o->len / (sizeof(T) / sizeof(byte))) {};
  contiguous_memory(const contiguous_memory &o) : memory(o.memory), length(o.length), capacity(o.capacity) {};
  void
  accept_new_memory(const chunk<byte> &o)
  {
    memory = reinterpret_cast<T *>(o.ptr);
    capacity = o.len / (sizeof(T) / sizeof(byte));
  }

  contiguous_memory(contiguous_memory &&o) : memory(o.memory), length(o.length), capacity(o.capacity)
  {
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  };
  template <typename C = T>
  contiguous_memory(contiguous_memory<C> &&o) : memory(o.memory), length(o.length), capacity(o.capacity)
  {
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  };
  inline chunk<byte>
  operator*()
  {
    return { .ptr = reinterpret_cast<byte *>(memory), .len = capacity * sizeof(T) * sizeof(byte) };
  };
  contiguous_memory &
  operator=(const contiguous_memory &o)
  {
    memory = o.memory;
    length = o.length;
    capacity = o.capacity;
    return *this;
  }
  contiguous_memory &
  operator=(contiguous_memory &&o)
  {
    memory = o.memory;
    length = o.length;
    capacity = o.capacity;

    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
    return *this;
  };
  ~contiguous_memory()
  {
    memory = nullptr;
    length = 0;
    capacity = 0;
  }

  // TODO: FIX CONFLICTS AND RESTORE PROTECTED
//protected:
  T *memory;
  size_t length;
  size_t capacity;
};

// move only contiguous memory object, disallow copying explicitly.
// meant to wrap a T* from the allocator
template <typename T>
  requires std::is_copy_constructible_v<T> && std::is_move_constructible_v<T>
struct contiguous_memory_no_copy {
  contiguous_memory_no_copy()
    : memory(nullptr), length(0), capacity(0)
  {};
  contiguous_memory_no_copy(T *ptr, size_t a, size_t b) : memory(ptr), length(a), capacity(b) {};
  contiguous_memory_no_copy(chunk<byte> &&o)
      : memory(reinterpret_cast<T *>(o.ptr)), length(0), capacity(o.len / (sizeof(T) / sizeof(byte)))
  {
    o.ptr = nullptr;
    o.len = 0;
  };
  contiguous_memory_no_copy(const chunk<byte> &) = delete;
  contiguous_memory_no_copy(const contiguous_memory_no_copy &) = delete;     // move only!
  template <typename C>
  contiguous_memory_no_copy(contiguous_memory_no_copy<C> &&o)
      : memory(reinterpret_cast<T *>(o.memory)), length(o.length), capacity(o.capacity)
  {
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  };
  contiguous_memory_no_copy(contiguous_memory_no_copy &&o) : memory(o.memory), length(o.length), capacity(o.capacity)
  {

    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  };
  inline chunk<byte>
  data()
  {
    return { .ptr = reinterpret_cast<byte *>(memory), .len = capacity * sizeof(T) / sizeof(byte) };
  };
  contiguous_memory_no_copy &operator=(const contiguous_memory_no_copy &) = delete;
  contiguous_memory_no_copy &
  operator=(contiguous_memory_no_copy &&o)
  {
    memory = o.memory;
    length = o.length;
    capacity = o.capacity;

    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
    return *this;
  };
  void
  accept_new_memory(const chunk<byte> &o)
  {
    memory = reinterpret_cast<T *>(o.ptr);
    capacity = o.len / (sizeof(T) / sizeof(byte));
  }
  ~contiguous_memory_no_copy()
  {
    memory = nullptr;
    length = 0;
    capacity = 0;
  }

protected:
  T *memory;
  size_t length;
  size_t capacity;
};

// immutable memory object, for immutable objects.
// meant to wrap a T* from the allocator
template <typename T> struct immutable_memory {
  immutable_memory() 
    : memory(nullptr), length(0), capacity(0)
  {};
  template <typename V>
  requires std::is_null_pointer_v<V>
  immutable_memory(V) : memory(nullptr), length(0), capacity(0) {};
  immutable_memory(T *ptr, size_t a, size_t b) : memory(ptr), length(a), capacity(b) {};
  immutable_memory(chunk<byte> &&o)
      : memory(reinterpret_cast<T *>(o.ptr)), length(0), capacity(o.len / (sizeof(T) / sizeof(byte))) {};
  immutable_memory(const chunk<byte> &o)
      : memory(reinterpret_cast<T *>(o.ptr)), length(0), capacity(o.len / (sizeof(T) / sizeof(byte))) {};
  immutable_memory(const immutable_memory &) = delete;     // move only!
  template <typename C>
  immutable_memory(immutable_memory<C> &&o)
      : memory(reinterpret_cast<T *>(o.memory)), length(o.length), capacity(o.capacity)
  {
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  };
  immutable_memory(immutable_memory &&o) : memory(o.memory), length(o.length), capacity(o.capacity)
  {

    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  };
  inline chunk<byte>
  data()
  {
    return { .ptr = reinterpret_cast<byte *>(memory), .len = capacity * sizeof(T) / sizeof(byte) };
  };
  immutable_memory &operator=(const immutable_memory &) = delete;
  immutable_memory &
  operator=(immutable_memory &&o)
  {
    memory = o.memory;
    length = o.length;
    capacity = o.capacity;

    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
    return *this;
  };
  void
  accept_new_memory(const chunk<byte> &o)
  {
    memory = reinterpret_cast<T *>(o.ptr);
    capacity = o.len / (sizeof(T) / sizeof(byte));
  }
  ~immutable_memory()
  {
    memory = nullptr;
    length = 0;
    capacity = 0;
  }
protected:
  T *memory;
  size_t length;
  size_t capacity;
};

};     // namespace micron
