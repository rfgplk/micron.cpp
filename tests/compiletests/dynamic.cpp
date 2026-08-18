//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Compile coverage for the dynamic-loading layer: built across the arch x opt x freestanding
// matrix, never run.
//
// The rows that matter most are -k and -ke. dynamic_open and dynamic_call throw, and a freestanding
// build without the trampoline turns exc<> into an abort() -- so those paths must still COMPILE
// there, and the query half (dynamic_sym / dynamic_close / dynamic_error) must remain usable, which
// is exactly why it is noexcept and reports through a buffer instead of throwing.
//
// The --i386 and --arm rows matter for a second reason: the whole layer sits on a loader that until
// 2026-08 did not exist on either.

#include "../../src/dynamic.hpp"

namespace mc = micron;
namespace dl = micron::elf::dl;

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the flag algebra, checked at compile time on every cell

static_assert(static_cast<u32>(mc::rtld::local) == 0, "local is the absence of global, not a bit");
static_assert(mc::has(mc::rtld::now | mc::rtld::global, mc::rtld::global));
static_assert(mc::has(mc::rtld::now | mc::rtld::global, mc::rtld::now));
static_assert(!mc::has(mc::rtld::now | mc::rtld::global, mc::rtld::nodelete));
static_assert(!mc::has(mc::rtld::now | mc::rtld::local, mc::rtld::global));
static_assert(static_cast<u32>((mc::rtld::now | mc::rtld::lazy) & mc::rtld::now) != 0);

// lazy binding has no PLT trampoline behind it; the answer must be a compile-time no, not a
// runtime surprise
static_assert(!mc::dynamic_lazy_supported());

// a default-constructed handle is falsy, and the two pseudo-handles are distinct from it and from
// each other
static_assert(mc::dynamic_t{}.generation == 0);
static_assert(!(mc::dynamic_default == mc::dynamic_next));
static_assert(!(mc::dynamic_default == mc::dynamic_t{}));

// the registry must be able to record every DT_NEEDED the parser will hand it
static_assert(dl::max_deps == mc::elf::dyn_info_t::max_needed);
static_assert(dl::max_modules >= 64);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// force every template in the public surface to be instantiated on every target

int
iterate_cb(dl::dl_phdr_info_t *, usize, void *)
{
  return 0;
}

void
instantiate_public()
{
  mc::dynamic_t d{};

  // open: raw pointer and is_string overloads, every flag shape
  (void)mc::dynamic_open("x", mc::rtld::now);
  (void)mc::dynamic_open("x", mc::rtld::lazy | mc::rtld::global);
  (void)mc::dynamic_open("x", mc::rtld::noload);
  (void)mc::dynamic_open("x", mc::rtld::now | mc::rtld::nodelete | mc::rtld::deepbind);
  micron::sstring<64> name;
  (void)mc::dynamic_open(name);

  // queries -- all noexcept, all usable from a -k build
  (void)mc::dynamic_sym(d, "s");
  (void)mc::dynamic_sym(d, name);
  (void)mc::dynamic_sym(mc::dynamic_default, "s");
  (void)mc::dynamic_sym(mc::dynamic_next, "s");
  (void)mc::dynamic_sym_as<int (*)()>(d, "s");
  (void)mc::dynamic_close(d);
  (void)mc::dynamic_error();
  (void)mc::dynamic_owner(nullptr);
  (void)mc::dynamic_path(d);
  (void)mc::dynamic_soname(d);
  (void)mc::dynamic_refcount(d);

  (void)mc::dl_iterate_phdr(&iterate_cb, nullptr);
}

// dynamic_call over the shapes that actually differ: void vs value return, zero vs several
// arguments, handle vs one-shot, raw pointer vs string
void
instantiate_call()
{
  mc::dynamic_t d{};
  micron::sstring<64> lib, sym;

  mc::dynamic_call(d, "v");                       // R defaults to void
  mc::dynamic_call<void>(d, "v");
  (void)mc::dynamic_call<int>(d, "i");
  (void)mc::dynamic_call<const char *>(d, "s");
  (void)mc::dynamic_call<int>(d, "i", 1, 2.0, static_cast<void *>(nullptr));
  (void)mc::dynamic_call<int>(d, sym);

  (void)mc::dynamic_call<int>("lib", "i");
  (void)mc::dynamic_call<int>("lib", "i", 1);
  (void)mc::dynamic_call<int>(lib, sym);
  mc::dynamic_call<void>("lib", "v");
}

// the machinery below the public surface, so a change there is caught by the matrix too
void
instantiate_internals()
{
  mc::elf::dyn_info_t d{};
  (void)dl::verneed_name(d, 2);
  (void)dl::verdef_name(d, 2);
  (void)dl::version_matches(d, 0, nullptr);
  (void)dl::version_matches(d, 0, "GLIBC_2.34");
  (void)dl::wanted_version(d, 0);
  (void)dl::lookup_versioned(d, "s", nullptr);
  (void)dl::gnu_lookup_versioned(d, "s", "V1");
  (void)dl::sysv_lookup_versioned(d, "s", "V1");

  (void)dl::__find_by_ident(0, 0);
  (void)dl::__find_by_soname("s");
  (void)dl::__find_by_address(nullptr);
  (void)dl::__slot_at(0, 0);
  u64 dev = 0, ino = 0;
  (void)dl::__file_ident("/dev/null", dev, ino);

  (void)dl::__dirname_of("/a/b/c");
  (void)dl::__expand_origin("$ORIGIN/../lib", 14, "/here");
  (void)dl::__try_path_list("/a:/b", "libx.so", "/here");
  (void)dl::resolve_dependency("libx.so", nullptr, nullptr, nullptr);

  (void)dl::__lookup_global("s", nullptr);
  (void)dl::__host_resolve("s");
  (void)dl::__resolve_for_module(nullptr, "s", 0);

  dl::__debug_init_once();
  dl::__debug_rebuild_chain();
  dl::__debug_publish(dl::rt_consistent);

  dl::load_state st;
  dl::__note_created(st, 0);
  dl::__unwind(st);
}

}      // namespace

int
main()
{
  instantiate_public();
  instantiate_call();
  instantiate_internals();
  return 1;
}
