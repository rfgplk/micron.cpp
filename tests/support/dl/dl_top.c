/*  Copyright (c) 2026- David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 *
 * The top of the chain. It names NOTHING in the leaf -- only mid. Resolving mc_dl_top_call, and
 * seeing "leaf+" in the trace before "top+", is what proves the loader followed DT_NEEDED
 * transitively and initialised depth-first rather than one level deep.
 */

extern void mc_dl_mid_trace(const char *);
extern void mc_dl_mid_set_dtor_bit(int);
extern int mc_dl_mid_call(void);

int
mc_dl_top_call(void)
{
  return mc_dl_mid_call() + 1;
}

__attribute__((constructor)) static void
ctor(void)
{
  mc_dl_mid_trace("top+");
}

__attribute__((destructor)) static void
dtor(void)
{
  mc_dl_mid_trace("top-");
  mc_dl_mid_set_dtor_bit(4);
}
