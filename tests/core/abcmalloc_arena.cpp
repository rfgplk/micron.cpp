
#include "../../src/allocation/abcmalloc/arena.hpp"
#include "../../src/allocation/abcmalloc/book.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include <random>
int
main()
{
  // mc::infolog(sizeof(abc::__arena::node<abc::sheet<abc::__class_arena_internal>>));
  // mc::infolog(sizeof(abc::__arena::node<abc::sheet<abc::__class_small>>));
  // mc::infolog(sizeof(abc::__arena::node<abc::sheet<abc::__class_medium>>));
  // mc::infolog(sizeof(abc::__arena::node<abc::sheet<abc::__class_large>>));
  // mc::infolog(sizeof(abc::__arena::node<abc::sheet<abc::__class_huge>>));
  // size_t sz = abc::__class_small;
  // mc::infolog(abc::__calculate_desired_space(sz));
  // sz = abc::__class_medium;
  // mc::infolog(abc::__calculate_desired_space(sz));
  // sz = abc::__class_large;
  // mc::infolog(abc::__calculate_desired_space(sz));
  // sz = abc::__class_huge;
  if constexpr ( false ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1000000);
    abc::__arena arena;
    size_t req_total = 0;
    for ( size_t n = 0; n < 120000; ++n ) {
      std::malloc(dist(gen));
      //arena.push(dist(gen));
    }
  }
  if constexpr ( false ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1000000);
    abc::__arena arena;
    size_t req_total = 0;
    for ( size_t n = 0; n < 120000; ++n ) {
      auto mem = arena.push(dist(gen));
      mc::console("#", n, " - Allocated memory, with size of: ", mem.len, " at address: ", mem.ptr);
      // if ( mem.ptr == (byte *)-1 )
      //   mc::console("Allocation failed");
      //  info
    }
  }
  if constexpr ( true ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1000000);
    abc::__arena arena;
    size_t req_total = 0;
    for ( size_t n = 0; n < (2 << 10); ++n ) {
      auto mem = arena.push(dist(gen));
      mc::console("#", n, " - Allocated memory, with size of: ", mem.len, " at address: ", mem.ptr);
      if ( arena.pop(mem) == false )
        mc::console("Failed to pop memory at: ", mem.ptr);
    }
    abc::stats_t stats = abc::get_stats();
    mc::console("Total number of memory allocations: ", stats.alloc_requests);
    mc::console("Total number of memory deallocations: ", stats.dealloc_requests);
    mc::console("Total amount of memory requested: ", stats.total_memory_req);
    mc::console("Total amount of memory allocated: ", stats.total_memory_throughput);
    mc::console("Total amount of memory freed: ", stats.total_memory_freed);
  }
  if constexpr ( false ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 20000);
    abc::__arena arena;
    size_t req_total = 0;
    for ( size_t n = 0; n < (2 << 15); ++n ) {
      auto tmp = dist(gen);
      req_total += tmp;
      auto mem = arena.push(tmp);
      if ( mem.ptr == (byte *)-1 )
        mc::console("Allocation failed");
      // info
      mc::console("#", n);
      mc::console("Free internal buffer: ", arena.__available_buffer());
      mc::console("Total allocated memory: ", arena.total_usage());
      mc::console("Total requested memory: ", req_total);
      mc::console("Efficiency ratio: ", (float)req_total / (float)arena.total_usage());
    }
    mc::infolog("Success");
  }
  if constexpr ( false ) {
    {
      // testing the dtors
      int j = 0;
      for ( int i = 0;; ++i ) {
        abc::__arena arena;
        for ( size_t n = 0; n < (2 << 12); ++n ) {
          if ( arena.push(256).ptr == (byte *)-1 )
            mc::console("Error");
        }
        mc::infolog("Success");
        mc::console("Attempt #: ", j++);
      }
    }
  }
  if constexpr ( false ) {
    mc::infolog(sizeof(abc::sheet<abc::__class_arena_internal>));
    mc::infolog(sizeof(abc::sheet<abc::__class_small>));
    mc::infolog(sizeof(abc::sheet<abc::__class_medium>));
    mc::infolog(sizeof(abc::sheet<abc::__class_large>));
    mc::infolog(sizeof(abc::sheet<abc::__class_huge>));
    abc::__arena arena;
    arena.push(1000);
    mc::infolog("Success");
  }
  if constexpr ( false ) {
    abc::__arena arena;
    for ( size_t n = 0; n < (2 << 12); ++n ) {
      mc::console("Free internal buffer: ", arena.__available_buffer());
      if ( arena.push(256).ptr == (byte *)-1 )
        mc::console("Error");
    }
    mc::infolog("Success");
  }
  if constexpr ( false ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(30, 4521);
    abc::__arena arena;
    for ( size_t n = 0; n < (2 << 10); ++n ) {
      mc::console("#", n);
      mc::console("Free internal buffer: ", arena.__available_buffer());
      auto mem = arena.push(dist(gen));
      if ( mem.ptr == (byte *)-1 )
        mc::console("Allocation failed");
      for ( int i = 0; i < mem.len; i++ )
        mem.ptr[i] = 0xF3;
      // NOTE: verified manually that this is properly packed
      // in gdb
      // shell cat /proc/self/maps | grep heap
      // set $hs = (void*)...start of region
      // set $he = (void*)... end of region
      // dump memory heap.bin $hs $he
    }
    mc::infolog("Success");
  }
  if constexpr ( false ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(30, 4521);
    abc::__arena arena;
    for ( size_t n = 0; n < (2 << 14); ++n ) {
      mc::console("#", n);
      mc::console("Free internal buffer: ", arena.__available_buffer());
      auto mem = arena.push(dist(gen));
      if ( mem.ptr == (byte *)-1 )
        mc::console("Allocation failed");
    }
    mc::infolog("Success");
  }
  if constexpr ( false ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(30, 4521);
    abc::__arena arena;
    for ( size_t n = 0; n < (2 << 14); ++n ) {
      auto mem = arena.push(dist(gen));
      if ( mem.ptr == (byte *)-1 )
        mc::console("Allocation failed");
    }
    mc::infolog("Success");
  }
  if constexpr ( false ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(30, 4521);
    abc::__arena arena;
    for ( size_t n = 0; n < (2 << 15); ++n ) {
      arena.push(2048);
    }
  }
  if constexpr ( false ) {
    byte *p_ptr = nullptr;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(30, 4521);
    abc::__arena arena;
    for ( size_t n = 0; n < (2 << 18); ++n ) {
      auto tmp = arena.push(dist(gen));
      if ( p_ptr == nullptr ) {
        p_ptr = tmp.ptr;
        *p_ptr = 0x1;
      }
    }
    if ( *p_ptr == 0x1 )
      mc::console("Valid");
  }
  if constexpr ( false ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(30, 4521);
    abc::__arena arena;
    for ( size_t n = 0; n < (2 << 30); ++n ) {
      mc::console("Free internal buffer: ", arena.__available_buffer());
      if ( arena.push(dist(gen)).ptr == (byte *)-1 )
        mc::console("Error");
      mc::console(n);
    }
    mc::infolog("Success");
  }
  return 0;
}
