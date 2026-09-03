//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/attach/mx_entry.hpp"

#include "../src/io/console.hpp"
#include "../src/vector.hpp"

#include "snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::test_case;

namespace
{

int g_ctor_ran = 0;

struct ctor_probe {
  ctor_probe() { g_ctor_ran = 7; }
};

ctor_probe g_probe;

thread_local int g_tls_probe = 11;

}      // namespace

extern "C" int
mx_main(const micron_mx_entry_args *a)
{
  sb::print("=== MICRON MX ENTRY ===");

  test_case("the descriptor validates, and reports the size the crt compiled");
  {
    require(micron::__mx_entry_args_valid(a), true);
    require(a->abi == micron_mx_entry_abi, true);
    require(a->size == static_cast<u32>(sizeof(micron_mx_entry_args)), true);
    require((a->flags & ~micron_mx_entry_f_all) == 0u, true);
  }
  end_test_case();

  test_case("the system v stack reached the guest through the struct");
  {
    require(a->argc >= 1u, true);
    require(a->argv != 0u, true);
    require(a->envp != 0u, true);
    require(a->auxv != 0u, true);
    char **const argv = reinterpret_cast<char **>(static_cast<uintptr_t>(a->argv));
    require(argv[0] != nullptr, true);
    require(argv[0][0] != 0, true);
    require(argv[a->argc] == nullptr, true);
  }
  end_test_case();

  test_case("the page size came out of the auxv rather than a guess");
  {
    require(a->page_size == 4096u || a->page_size == 16384u || a->page_size == 65536u, true);
  }
  end_test_case();

  test_case("the stack region is real, and its flag says so");
  {
    require((a->flags & micron_mx_entry_f_have_stack) != 0u, true);
    require(a->stack_lo < a->stack_hi, true);

    const u64 here = static_cast<u64>(reinterpret_cast<uintptr_t>(&a));
    require(here > a->stack_lo && here <= a->stack_hi, true);
  }
  end_test_case();

  test_case("a field its flag does not claim reads zero, and so does every reserved word");
  {
    require((a->flags & micron_mx_entry_f_have_tls) == 0u, true);
    require(a->tls_base == 0u, true);
    require((a->flags & micron_mx_entry_f_have_user) == 0u, true);
    require(a->user == 0u, true);
    require(a->user_len == 0u, true);
    for ( u32 i = 0; i < 4; ++i ) require(a->reserved[i] == 0u, true);
  }
  end_test_case();

  test_case("__micron_mxc booted the full runtime, not a subset of it");
  {
    require(g_ctor_ran == 7, true);
    require(g_tls_probe == 11, true);
    micron::vector<int> v;
    for ( int i = 0; i < 4; ++i ) v.push_back(i * i);
    int sum = 0;
    for ( usize i = 0; i < v.size(); ++i ) sum += v[i];
    require(sum == 14, true);
  }
  end_test_case();

  test_case("size is what gates a read, so a field the crt was too old to fill is not read");
  {
    require(micron::__mx_entry_has(a, __builtin_offsetof(micron_mx_entry_args, page_size) + 8u), true);
    require(!micron::__mx_entry_has(a, static_cast<u32>(sizeof(micron_mx_entry_args)) + 8u), true);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
