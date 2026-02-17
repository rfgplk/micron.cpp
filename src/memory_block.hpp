//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "algorithm/memory.hpp"
#include "allocator.hpp"
#include "concepts.hpp"
#include "except.hpp"
#include "tags.hpp"
#include "types.hpp"

namespace micron
{

template <class Alloc = micron::allocator_serial<>> class memory_block : public Alloc
{
  chunk<byte> memory;

public:
  using category_type = buffer_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using safety_type = safe_tag;
  using value_type = byte;
  using reference = byte &;
  using const_reference = const byte &;
  using pointer = byte *;
  using const_pointer = const byte *;
  using iterator = byte *;
  using const_iterator = const byte *;

  ~memory_block()
  {
    if ( !memory.zero() )
      this->destroy(memory);
  }

  memory_block() = delete;

  explicit memory_block(size_t n) : memory(this->create(n)) {}

  memory_block(const memory_block &o) : Alloc(), memory(this->create(o.size())) { micron::memcpy(memory.ptr, o.memory.ptr, o.size()); }

  explicit memory_block(chunk<byte> &&m) : memory(micron::move(m)) {}

  memory_block(memory_block &&o) noexcept : Alloc(micron::move(o)), memory(micron::move(o.memory))
  {
    o.memory.ptr = nullptr;
    o.memory.len = 0;
  }

  memory_block &
  operator=(const memory_block &o)
  {
    if ( this != &o ) {
      if ( memory.len != o.size() ) {
        this->destroy(memory);
        memory = this->create(o.size());
      }
      micron::memcpy(memory.ptr, o.memory.ptr, o.size());
    }
    return *this;
  }

  memory_block &
  operator=(memory_block &&o) noexcept
  {
    if ( this != &o ) {
      this->destroy(memory);
      Alloc::operator=(micron::move(o));
      memory = micron::move(o.memory);
      o.memory.ptr = nullptr;
      o.memory.len = 0;
    }
    return *this;
  }

  memory_block &
  operator=(const byte &b)
  {
    micron::memset(memory.ptr, b, memory.len);
    return *this;
  }

  void
  resize(size_t n)
  {
    if ( n <= memory.len )
      return;
    chunk<byte> tmp = micron::move(memory);
    memory = this->create(n);
    micron::memcpy(memory.ptr, tmp.ptr, tmp.len);
    this->destroy(tmp);
  }

  void
  recreate(size_t n)
  {
    this->destroy(memory);
    memory = this->create(n);
  }

  byte &
  operator[](size_t n)
  {
    if constexpr ( micron::is_same_v<safety_type, safe_tag> ) {
      if ( n >= memory.len ) [[unlikely]]
        exc<except::runtime_error>("micron::memory [] out of bounds");
    }
    return memory.ptr[n];
  }

  const byte &
  operator[](size_t n) const
  {
    if constexpr ( micron::is_same_v<safety_type, safe_tag> ) {
      if ( n >= memory.len ) [[unlikely]]
        exc<except::runtime_error>("micron::memory [] out of bounds");
    }
    return memory.ptr[n];
  }

  byte *
  at_pointer(size_t n)
  {
    if constexpr ( micron::is_same_v<safety_type, safe_tag> ) {
      if ( n >= memory.len ) [[unlikely]]
        exc<except::runtime_error>("micron::memory at_pointer out of bounds");
    }
    return memory.ptr + n;
  }

  byte &
  at(size_t n)
  {
    if constexpr ( micron::is_same_v<safety_type, safe_tag> ) {
      if ( n >= memory.len ) [[unlikely]]
        exc<except::runtime_error>("micron::memory at out of bounds");
    }
    return memory.ptr[n];
  }

  byte *
  operator&()
  {
    return memory.ptr;
  }

  iterator
  begin()
  {
    return memory.ptr;
  }

  iterator
  end()
  {
    return memory.ptr + memory.len;
  }

  const_iterator
  begin() const
  {
    return memory.ptr;
  }

  const_iterator
  end() const
  {
    return memory.ptr + memory.len;
  }

  const_iterator
  cbegin() const
  {
    return memory.ptr;
  }

  const_iterator
  cend() const
  {
    return memory.ptr + memory.len;
  }

  size_t
  size() const
  {
    return memory.len;
  }

  const chunk<byte> &
  chunk_ref() const
  {
    return memory;
  }
};

using buffer = memory_block<>;
};     // namespace micron
