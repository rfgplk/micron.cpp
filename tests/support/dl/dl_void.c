/*  Copyright (c) 2026- David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 *
 * A module that EXPORTS NOTHING and imports several symbols from the leaf.
 *
 * That combination is the whole point. GNU hash covers only the defined exported symbols, from
 * .gnu.hash's symoffset onward; a module with no exports has every bucket empty, so deriving the
 * .dynsym length by walking the chains yields symoffset -- 1 -- instead of the real count, and
 * every relocation whose symbol index is >= 1 then looks out of range. Under reloc_mode_t::strict
 * that is a load failure; under best_effort the GOT/PLT slots are quietly left unbound.
 *
 * Not a synthetic shape: 381 ELF files under /bin and /usr/lib64 on a stock Fedora box are built
 * this way, /usr/lib64/libibverbs/librxe-rdmav59.so among them -- an rdma provider plugin whose
 * whole .dynsym (53 entries) is UND imports and which registers itself from a constructor.
 * Exactly what this file imitates.
 *
 * It exports nothing, so everything it proves it proves through the leaf's trace buffer.
 */

extern void mc_dl_trace_add(const char *); /* JUMP_SLOT */
extern int mc_dl_leaf_value(void);         /* JUMP_SLOT */
extern int mc_dl_leaf_global;              /* GLOB_DAT  */
extern int *mc_dl_dtor_flag;               /* GLOB_DAT  */

static int observed;

__attribute__((constructor)) static void
ctor(void)
{
  observed = mc_dl_leaf_value() + mc_dl_leaf_global; /* 42 + 7 */
  /* the token IS the assertion: a wrongly bound slot yields "void?", not a silent pass */
  mc_dl_trace_add(observed == 49 ? "void+" : "void?");
}

__attribute__((destructor)) static void
dtor(void)
{
  mc_dl_trace_add("void-");
  if ( mc_dl_dtor_flag ) *mc_dl_dtor_flag |= 8;
}
