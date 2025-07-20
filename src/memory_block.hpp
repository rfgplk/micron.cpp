//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include <xmmintrin.h>

#include "algorithm/mem.hpp"
#include "allocator.hpp"
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
  typedef byte value_type;
  typedef byte &reference;
  typedef byte &ref;
  typedef const byte &const_reference;
  typedef const byte &const_ref;
  typedef byte *pointer;
  typedef const byte *const_pointer;
  typedef byte *iterator;
  typedef const byte *const_iterator;

  ~memory_block() {
    this->destroy(memory);
  }

  memory_block() = delete;
  memory_block(size_t n) : memory(this->create(n)) {
  };
  memory_block(const memory_block &o) : Alloc(), memory(o.memory) {}
  memory_block(chunk<byte> *m) : Alloc(), memory(m) {}
  memory_block(chunk<byte> *&&m) : Alloc(), memory(m) { }
  memory_block(memory_block &&o) : memory(o.memory) { }
  memory_block &
  operator=(const memory_block &o)
  {
    memory.ptr = o.memory.ptr;
    memory.len = o.memory.len;
    return *this;
  }
  memory_block &
  operator=(memory_block &&o)
  {
    memory.ptr = o.memory.ptr;
    memory.len = o.memory.len;
    o.memory.ptr = 0;
    o.memory.len = 0;
    return *this;
  }
  memory_block &
  operator=(chunk<byte> *o)
  {
    memory.ptr = o->ptr;
    memory.len = o->len;
    return *this;
  }
  memory_block &
  operator=(chunk<byte> &&o)
  {
    memory.ptr = o.ptr;
    memory.len = o.len;
    o.ptr = 0;
    o.len = 0;
    return *this;
  }
  memory_block& operator=(const byte& b)
  {
    micron::memset(memory.ptr, b, memory.len);
    return *this;
  }
  // resize and move elements
  inline void
  resize(const size_t n) {
    if(n <= memory.len)
      return;
    chunk<byte> tmp = micron::move(memory);
    memory = this->create(n);
    micron::memcpy(memory.ptr, tmp.ptr, tmp.len);
    this->destroy(tmp); // delete old memory
  }

  // resize and DONT copy memory (eqv to assigning a new block)
  inline void
  recreate(const size_t n) {
    this->destroy(memory); // delete old memory
    memory = this->create(n);
  }
  // overload this to always point to mem
  byte *
  operator&()
  {
    return memory.ptr;
  }
  // access at element
  inline byte &
  operator[](size_t n)
  {
    if constexpr ( std::is_same_v<safety_type, safe_tag> ) {
      if ( n >= memory.len ) [[unlikely]]
        throw except::runtime_error("micron::memory [] out of bounds");
    }
    return memory.ptr[n];
  }
  inline const byte &
  operator[](size_t n) const
  {
    if constexpr ( std::is_same_v<safety_type, safe_tag> ) {
      if ( n >= memory.len ) [[unlikely]]
        throw except::runtime_error("micron::memory [] out of bounds");
    }
    return memory.ptr[n];
  }
  // get underlying mem ptr
  chunk<byte> *
  operator->()
  {
    return &memory;
  }
  byte &
  at(size_t n)
  {
    if constexpr ( std::is_same_v<safety_type, safe_tag> ) {
      if ( n >= memory.len ) [[unlikely]]
        throw except::runtime_error("micron::memory [] out of bounds");
    }
    return memory.ptr[n];
  }
  byte *
  begin()
  {
    return &memory.ptr[0];
  }
  byte *
  end()
  {
    return &memory.ptr[memory.len - 1];
  }

  const byte *
  cbegin() const
  {
    return &memory.ptr[0];
  }
  const byte *
  cend() const
  {
    return &memory.ptr[memory.len - 1];
  }
  size_t
  size() const
  {
    return memory.len;
  }
};
using buffer = memory_block<>;
};     // namespace micron
