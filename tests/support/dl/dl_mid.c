/*  Copyright (c) 2026- David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 *
 * The middle of the chain. Everything it does crosses a module boundary on purpose, and it
 * re-exports the leaf's trace so `top` can reach it WITHOUT naming the leaf -- that is what makes
 * the transitive-DT_NEEDED property testable rather than assumed.
 */

extern void mc_dl_trace_add(const char *);
extern int mc_dl_leaf_value(void);
extern int mc_dl_leaf_global;
extern int *mc_dl_dtor_flag;

/* a cross-module CALL (R_X86_64_JUMP_SLOT / R_386_JMP_SLOT / R_ARM_JUMP_SLOT) */
int
mc_dl_mid_call(void)
{
  return mc_dl_leaf_value() + 1;
}

/* a cross-module DATA reference (R_*_GLOB_DAT) */
int
mc_dl_mid_read_global(void)
{
  return mc_dl_leaf_global;
}

/* the leaf's trace, reachable one hop up */
void
mc_dl_mid_trace(const char *s)
{
  mc_dl_trace_add(s);
}

void
mc_dl_mid_set_dtor_bit(int bit)
{
  if ( mc_dl_dtor_flag ) *mc_dl_dtor_flag |= bit;
}

__attribute__((constructor)) static void
ctor(void)
{
  /* reaches into the leaf: this call is unbound unless the leaf relocated AND ran first */
  mc_dl_trace_add("mid+");
}

__attribute__((destructor)) static void
dtor(void)
{
  mc_dl_trace_add("mid-");
  if ( mc_dl_dtor_flag ) *mc_dl_dtor_flag |= 2;
}
