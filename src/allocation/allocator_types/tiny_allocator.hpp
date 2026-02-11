#pragma once

namespace micron
{

// TODO: update

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
  i16
  get_grow() const
  {
    return P::on_grow;
  }
};

};
