#pragma once

#include <concepts>
#include <xmmintrin.h>

#include "allocation/allocate_linux.hpp"
#include "allocation/system_rs.hpp"
#include "memory/memory.hpp"
#include "types.hpp"

namespace micron
{

#define ALLOC_FREE 0
#define ALLOC_USED 1

#define POOL_SERIAL 0
#define POOL_LINKED 1
#define POOL_NONE 0xFF
// determine how memory will be allocated, whenever a container requests more
// mem follow this policy
struct serial_allocation_policy {
  static constexpr bool concurrent = false;              // used in concurrent structures
  static constexpr uint32_t on_grow = 3;                 // by how much memory grows on each realloc (& how)
  static constexpr uint32_t pooling = POOL_SERIAL;       // what type of pool
  static constexpr uint32_t granularity = page_size;     // minimum amount of memory alloc'd
  static constexpr uint32_t shareable = 0;               // can be shared between owners
};

// for huge pages
struct huge_allocation_policy {
  static constexpr bool concurrent = false;                    // used in concurrent structures
  static constexpr uint32_t on_grow = 4;                       // by how much memory grows on each realloc (& how)
  static constexpr uint32_t pooling = POOL_SERIAL;             // what type of pool
  static constexpr uint32_t granularity = large_page_size;     // minimum amount of memory alloc'd
  static constexpr uint32_t shareable = 0;                     // can be shared between owners
};

// for allocating nearly all RAM
struct total_allocation_policy {
  static constexpr bool concurrent = false;              // used in concurrent structures
  static constexpr uint32_t on_grow = 0;                 // cannot grow by definition
  static constexpr uint32_t pooling = POOL_NONE;         // what type of pool
  static constexpr uint32_t granularity = page_size;     // minimum amount of memory alloc'd
  static constexpr uint32_t shareable = 1;               // can be shared between owners
};

struct tiny_allocation_policy {
  static constexpr bool concurrent = false;            // used in concurrent structures
  static constexpr uint32_t on_grow = 2;               // by how much memory grows on each realloc (& how)
  static constexpr uint32_t pooling = POOL_SERIAL;     // what type of pool
  static constexpr uint32_t granularity = 256;         // minimum amount of memory alloc'd
  static constexpr uint32_t shareable = 1;             // can be shared between owners
};

struct linked_allocation_policy {
  static constexpr bool concurrent = true;               // used in concurrent structures
  static constexpr uint32_t on_grow = 3;                 // by how much memory grows on each realloc (& how)
  static constexpr uint32_t pooling = POOL_LINKED;       // what type of pool
  static constexpr uint32_t granularity = page_size;     // minimum amount of memory alloc'd
  static constexpr uint32_t shareable = 1;               // can be shared between owners
};

template <typename P>
concept is_policy = requires {
  { P::concurrent } -> std::same_as<const bool &>;
  { P::on_grow } -> std::same_as<const uint32_t &>;
  { P::pooling } -> std::same_as<const uint32_t &>;
  { P::granularity } -> std::same_as<const uint32_t &>;
};
// default allocator, use malloc/free
template <typename T> class stl_allocator
{
  constexpr stl_allocator() = default;
  constexpr stl_allocator(const stl_allocator &) = default;
  constexpr stl_allocator(stl_allocator &&) = default;
  T *
  allocate(size_t cnt)
  {
    const auto ptr = std::malloc(sizeof(T) * cnt);
    if ( !ptr )
      throw except::memory_error();
    return static_cast<T *>(ptr);
  }
  void
  deallocate(T *ptr, size_t cnt)
  {
    std::free(ptr);
  }
  friend bool
  operator==(const stl_allocator<T> &a, const stl_allocator<T> &b)
  {
    return true;
  }
  friend bool
  operator!=(const stl_allocator<T> &a, const stl_allocator<T> &b)
  {
    return false;
  }
};
template <size_t Sz = page_size>
constexpr inline size_t
to_page(size_t n)
{
  if ( n % Sz != 0 )
    n += Sz - (n % Sz);
  return n;
}
template <u32 G>
inline constexpr size_t
to_granularity(size_t n)
{
  if ( n % G != 0 )
    n += G - (n % G);
  return n;
}
// default micron allocator, uses mmap
template <typename T, size_t Sz> class map_allocator : private allocator<byte, Sz>
{
public:
  constexpr map_allocator() = default;
  constexpr map_allocator(const map_allocator &) = default;
  constexpr map_allocator(map_allocator &&) = default;
  T *
  allocate(size_t cnt)
  {
    return allocator<byte, Sz>::alloc(cnt);
  }
  void
  deallocate(T *ptr, size_t cnt)
  {
    return allocator<byte, Sz>::dealloc(ptr, cnt);
  }
};
// serial standard allocator, cannot be mempooled, default doubling policy
template <is_policy P = serial_allocation_policy> class allocator_serial : private map_allocator<byte, page_size>
{     // uses mmap_allocator as baseline allocator

public:
  // anything else untouched
  allocator_serial() {}     // init blank without args
  allocator_serial(const allocator_serial &o) {};
  allocator_serial(allocator_serial &&o) {}
  allocator_serial &
  operator=(const allocator_serial &o)
  {
    return *this;
  }
  allocator_serial &
  operator=(allocator_serial &&o)
  {
    return *this;
  }
  ~allocator_serial() {}
  // functions that should be used for creating
  // repeatedly call allocate to allocate more (desired) mem
  inline __attribute__((always_inline)) static constexpr size_t
  auto_size()
  {
    return alloc_auto_sz;
  }
  chunk<byte>
  create(size_t n)
  {
    n = to_page(n);
    return chunk<byte>{ allocate(n), n };     // create the block, the handler is responsible for calling destroy
  }
  chunk<byte>
  grow(byte *ptr, size_t old, size_t n)
  {
    n = to_page(n);
    n = n / old < P::on_grow ? P::on_grow * old : (n / old) * old;
    chunk<byte> mem = { allocate(n), n };
    micron::memcpy(mem.ptr, ptr, old);
    destroy({ ptr, old });
    return mem;
  }
  void
  destroy(const chunk<byte> &mem)
  {
    deallocate(mem.ptr, mem.len);
  }
  byte *share(void) = delete;
  int16_t
  get_grow() const
  {
    return P::on_grow;
  }
};

// total memory allocator, will allocate all system available memory reduced by offset
template <is_policy P = huge_allocation_policy> class allocator_total : private map_allocator<byte, page_size>
{     // uses mmap_allocator as baseline allocator
  size_t ram_size;

public:
  // anything else untouched
  allocator_total()
  {
    resources sz;
    ram_size = static_cast<size_t>(sz.free_memory * 0.90);     // leave some
  }     // init blank without args
  allocator_total(const allocator_total &o) {};
  allocator_total(allocator_total &&o) {}
  allocator_total &
  operator=(const allocator_total &o)
  {
    return *this;
  }
  allocator_total &
  operator=(allocator_total &&o)
  {
    return *this;
  }
  ~allocator_total() {}
  // functions that should be used for creating
  // repeatedly call allocate to allocate more (desired) mem
  inline __attribute__((always_inline)) size_t
  auto_size()
  {
    return ram_size;
  }
  chunk<byte>
  create(size_t n)
  {
    if ( n < ram_size )
      n = ram_size;
    n = to_page<page_size>(n);
    return chunk<byte>{ allocate(n), n };     // create the block, the handler is responsible for calling destroy
  }
  chunk<byte>
  grow(byte *ptr, size_t old, size_t n)
  {
    throw except::memory_error("The total memory allocator cannot grow");
  }
  void
  destroy(const chunk<byte> &mem)
  {
    deallocate(mem.ptr, mem.len);
  }
  byte *share(void) = delete;
  int16_t
  get_grow() const
  {
    return P::on_grow;
  }
};

// huge standard allocator, cannot be mempooled, quad growth policy
// NOTE: make absolutely sure that a) the kernel has support for huge pages b) the executing user has all the appropriate
// huge pages permissions c) huge pages have been enabled otherwise this will error out with ENOMEM set
// /proc/sys/vm/nr_hugepages && nr_hugepages_policy on a lot of distros this is disabled by default
template <is_policy P = huge_allocation_policy> class allocator_huge : private map_allocator<byte, large_page_size>
{     // uses mmap_allocator as baseline allocator

public:
  // anything else untouched
  allocator_huge() {}     // init blank without args
  allocator_huge(const allocator_huge &o) {};
  allocator_huge(allocator_huge &&o) {}
  allocator_huge &
  operator=(const allocator_huge &o)
  {
    return *this;
  }
  allocator_huge &
  operator=(allocator_huge &&o)
  {
    return *this;
  }
  ~allocator_huge() {}
  // functions that should be used for creating
  // repeatedly call allocate to allocate more (desired) mem
  inline __attribute__((always_inline)) static constexpr size_t
  auto_size()
  {
    return alloc_auto_large_sz;
  }
  chunk<byte>
  create(size_t n)
  {
    n = to_page<large_page_size>(n);
    return chunk<byte>{ allocate(n), n };     // create the block, the handler is responsible for calling destroy
  }
  chunk<byte>
  grow(byte *ptr, size_t old, size_t n)
  {
    n = to_page<large_page_size>(n);
    n = n / old < P::on_grow ? P::on_grow * old : (n / old) * old;
    chunk<byte> mem = { allocate(n), n };
    micron::memcpy(mem.ptr, ptr, old);
    destroy({ ptr, old });
    return mem;
  }
  void
  destroy(const chunk<byte> &mem)
  {
    deallocate(mem.ptr, mem.len);
  }
  byte *share(void) = delete;
  int16_t
  get_grow() const
  {
    return P::on_grow;
  }
};
// allocator for small allocations of 256 bytes, altered version of serial_allocator
template <is_policy P = tiny_allocation_policy> class allocator_tiny : private map_allocator<byte, page_size>
{     // uses mmap_allocator as baseline allocator
  static constexpr size_t tiny_packets = page_size / P::granularity;
  byte **book;
  u32 count;
  u32 total;
  inline void
  __tiny(byte *t, size_t n)
  {
    total = n;
    n = n / P::granularity;     // divide to get nr of packets
    count = n;
    book[0] = t;
    for ( u32 i = 1; i < n; i++ )
      book[i] = book[0] + (P::granularity * i);     // each chunk is gran. aligned
  }

public:
  // anything else untouched
  allocator_tiny() : book(new byte *[tiny_packets]), count(0), total(0)
  {
    micron::cbset<tiny_packets>(book, 0x0);
  }     // init blank without args
  allocator_tiny(const allocator_tiny &o) {};
  allocator_tiny(allocator_tiny &&o) {}
  allocator_tiny &
  operator=(const allocator_tiny &o)
  {
    return *this;
  }
  allocator_tiny &
  operator=(allocator_tiny &&o)
  {
    return *this;
  }
  ~allocator_tiny() {}
  // functions that should be used for creating
  // repeatedly call allocate to allocate more (desired) mem
  inline __attribute__((always_inline)) static constexpr size_t
  auto_size()
  {
    return P::granularity;
  }
  chunk<byte>
  create(size_t n)
  {
    n = to_page(n);
    byte *t = allocate(n);
    __tiny(t, n);
    return chunk<byte>{ t, P::granularity };
  }
  chunk<byte>
  grow(byte *ptr, size_t old, size_t n)
  {
    if ( n <= (total - (count * P::granularity)) ) [[likely]]     // less than remaining memory in book
    {
      n = to_granularity<P::granularity>(n);
      size_t c = n / P::granularity;
      count += c;
      return chunk<byte>{ book[count - c], n };
    }
    // if not, reallocate
    n = to_page(n);     // get page alignment
    n = n / old < P::on_grow ? P::on_grow * old : (n / old) * old;
    chunk<byte> mem = { allocate(n), n };
    micron::memcpy(mem.ptr, ptr, total);
    destroy({ ptr, total });
    // this will implicitly free book
    // now update book
    book = new byte *[n / P::granularity];
    __tiny(mem.ptr, n);
    // memory & count are reset send back first ptr
    return chunk<byte>{ book[0], to_granularity<P::granularity>(n) };
  }
  byte **
  share(void)
  {
    return book;
  }
  void
  destroy(const chunk<byte> &mem)
  {
    deallocate(mem.ptr, total);
    delete[] book;
    count = 0;
    total = 0;
  }
  int16_t
  get_grow() const
  {
    return P::on_grow;
  }
};

// linked allocation policy
// unlike the serial allocator, this one won't keep memory contiguous in
// memory, and is allowed to be more granular no bookkeping means it will
// almost always be faster than regular allocs, but CANNOT be used for
// contiguous structures (need more mem, simply alloc ahead) mem view looks
// like this
// [...](size) -> [...](size) -> [...](size)
// pages are stored in ptrs and wiped on dtor
template <is_policy P = linked_allocation_policy> class allocator_linked : private map_allocator<byte, page_size>
{     // uses mmap_allocator as baseline allocator
public:
  // anything else untouched
  allocator_linked() {}     // init blank without args
  allocator_linked(const allocator_linked &o) {};
  allocator_linked(allocator_linked &&o) {}
  allocator_linked &
  operator=(const allocator_linked &o)
  {
    return *this;
  }
  allocator_linked &
  operator=(allocator_linked &&o)
  {
    return *this;
  }
  ~allocator_linked() {}
  // functions that should be used for creating
  // repeatedly call allocate to allocate more (desired) mem
  chunk<byte>
  create(size_t n)
  {
    n = to_page(n);
    auto x = chunk<byte>{ allocate(n), n };
  }
  chunk<byte>
  grow(byte *ptr, size_t old, size_t n)
  {
    n = to_page(n);
    n = n / old < P::on_grow ? P::on_grow * old : (n / old) * old;
    chunk<byte> mem = { allocate(n), n };
    micron::memcpy(mem.ptr, ptr, old);
    destroy({ ptr, old });
    return mem;
  }
  void
  destroy(const chunk<byte> &mem)
  {
    deallocate(mem.ptr, mem.len);
  }
  int16_t
  get_grow() const
  {
    return P::on_grow;
  }
};

// deprecated
};     // namespace micron
