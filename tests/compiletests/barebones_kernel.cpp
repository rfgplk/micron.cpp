//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// THE KEEP-SET AS A KERNEL MODULE SEES IT.
//
// Built, never run. This is barebones_core.cpp's sibling with one axis added: MICRON_PORT_KERNEL,
// which swaps every port facet from the linux backend to the kernel one and swaps abcmalloc for
// micron::bb. Both of those are compile-time selections with no runtime tell, so without a cell
// that actually builds them they are dead text -- which is the mistake Phase 4 caught twice.
//
// WARNING: INERT WITHOUT -DMICRON_PORT_KERNEL, and that is not optional. Both
// verify_compile_{gcc,clang}.duck sweep tests/compiletests/ BY DIRECTORY, so a file here that
// needs a flag they do not pass reddens ~338 cells at once. tests/compiletests/eh_runtime.cpp is
// written the same way for the same reason -- everything lives inside the #if, and `nm` on a
// non-kernel build shows only main.
//
// WARNING: INTEGER-ONLY, for the same reason barebones_core.cpp is. Under -mno-sse -mno-mmx
// -mno-80387 there is no FP register class, so a function that merely RETURNS a double is a hard
// error before its body is considered. tests/compiletests/{math,chrono,strings,graph,lazy}.cpp
// fail these cells by design and always will.
//
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// WHAT THIS FILE IS THE GATE FOR, beyond "it compiles"
//
// The object it produces is checked for TWO things the compiler will not tell you about:
//
//   1. ZERO syscall instructions. That is the whole point of the port layer, and it is not
//      self-evident: before Phase 5 this exact source under MICRON_PORT_KERNEL built cleanly and
//      emitted 41 of them, every one inside abc:: -- __va_reserve_once, __get_kernel_memory,
//      __vmap_freeze_at, the sheet release paths. It compiled, it linked, and it would have
//      executed `syscall` in ring 0. Compiling is not the assertion; objdump is.
//   2. ZERO vector and x87 registers, against a live negative control.
//
// Both checks live in verify_compile_barebones.duck beside the cells that build this.
//
// The undefined symbols of this object are also part of the contract: they must be exactly the
// mc_kport_* shim ABI plus the libgcc integer helpers. Anything else means micron reached for
// something a module cannot give it.

#if defined(MICRON_PORT_KERNEL)

#include "../../src/algorithm/algorithm.hpp"
#include "../../src/array.hpp"
#include "../../src/hash/hash.hpp"
#include "../../src/heap/binary_heap.hpp"
#include "../../src/lz.hpp"
#include "../../src/maps.hpp"
#include "../../src/memory/allocation/barebones/bb_alloc.hpp"
#include "../../src/memory/cmemory.hpp"
#include "../../src/memory/cstring.hpp"
#include "../../src/port/init.hpp"
#include "../../src/port/port.hpp"
#include "../../src/print.hpp"
#include "../../src/queue.hpp"
#include "../../src/sets/sets.hpp"
#include "../../src/sort/sort.hpp"
#include "../../src/stack.hpp"
#include "../../src/strings.hpp"
#include "../../src/trees.hpp"
#include "../../src/tuple.hpp"
#include "../../src/vector.hpp"

// MICRON_PORT_KERNEL must select the barebones allocator (defs.hpp): abcmalloc IS a 256 GiB
// PROT_NONE reservation committed with MAP_FIXED, and a module has no sparse address space to
// reserve into. If this ever stops holding, the build below would quietly go back to emitting
// syscalls from abc::, so it is asserted rather than assumed.
#if !defined(__micron_bb_alloc)
#error "MICRON_PORT_KERNEL must select the barebones allocator -- see defs.hpp. abcmalloc cannot run in a module."
#endif

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// containers over the stub allocator

[[gnu::used]] u64
containers()
{
  micron::vector<u64> v;
  for ( u64 i = 0; i < 64; ++i ) v.push_back(i * 3);
  u64 acc = 0;
  for ( auto x : v ) acc += x;

  micron::hopscotch_map<u64, u64> hm;
  for ( u64 i = 0; i < 32; ++i ) hm.insert(i, i ^ 0x5Aull);
  acc += hm.size();

  micron::hopscotch_set<u64> st;
  st.insert(u64{ 7 });
  acc += st.size();

  micron::string s = "barebones-kernel";
  s += "-tail";
  acc += s.size();

  micron::vector<u64> q;
  q.push_back(1);
  micron::sort::quick(q);
  acc += q.size();

  acc += micron::hash64(reinterpret_cast<const byte *>("kernel"), 6);
  return acc;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the allocator seam itself, named directly so the shim's hot names are instantiated rather than
// merely declared

[[gnu::used]] usize
allocator_seam()
{
  usize acc = 0;
  auto c = micron::bb::balloc(4096);
  if ( c.ptr != nullptr ) {
    acc += c.len;
    acc += micron::bb::query_size(c.ptr);
    acc += micron::bb::is_present(c.ptr) ? 1u : 0u;
    acc += micron::bb::within(c.ptr) ? 2u : 0u;
    auto g = micron::bb::resize(c, 8192, 4096, 32);
    micron::bb::dealloc(g.ptr);
  }
  auto a = micron::bb::aligned_balloc(64, 256);
  micron::bb::dealloc(a.ptr);

  // and through the abc:: alias surface, which is what every container above actually calls
  auto b = abc::balloc(128);
  acc += abc::query_size(b.ptr);
  abc::dealloc(b.ptr);
  acc += abc::native_block_alignment + abc::__hdr_offset;
  return acc;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the port surface, through the kernel backend

[[gnu::used]] i64
port_surface()
{
  micron::port::write_diag("barebones kernel gate\n");
  micron::port::cpu_relax();
  micron::port::yield();

  i64 acc = micron::port::mono_ticks() ^ micron::port::real_ticks();
  acc += micron::port::ticks_per_sec();
  acc += micron::port::exec_id() + micron::port::process_id() + micron::port::cpu_id();
  acc += micron::port::thread_alive(1) ? 1 : 0;

  const auto he = micron::port::heap_extent();
  acc += static_cast<i64>(he.total ^ he.free);

  auto s = micron::port::page_alloc(micron::port::page_size);
  if ( !s.failed() ) {
    acc += micron::port::addr_readable(s.ptr) ? 1 : 0;
    micron::port::page_discard(s.ptr, s.len);
    micron::port::page_free(s);
  }

  void *raw = micron::port::raw_map(4096);
  if ( raw != nullptr ) micron::port::raw_unmap(raw, 4096);

  alignas(4) u32 word = 0;
  acc += micron::port::wait(&word, 1, 0);
  acc += micron::port::wake(&word, 1);
  return acc;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printing, which is the only io a module gets

[[gnu::used]] void
printing()
{
  micron::println("kernel gate: v=", u64{ 42 }, " ok=", true);
  micron::print("no newline");
  micron::printn(u64{ 7 });
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the ctor walk a module_init would drive

[[gnu::used]] void
init_walk(micron::port::init_fn *first, micron::port::init_fn *last)
{
  micron::port::run_init_array(first, last);
  micron::port::run_fini_array(first, last);
}

};      // namespace

#endif      // MICRON_PORT_KERNEL

int
main()
{
  return 1;
}
