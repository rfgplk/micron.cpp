/*  Copyright (c) 2026- David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 *
 * A diamond: dia -> {leaf, mid}, and mid -> leaf. Its DT_NEEDED lists leaf FIRST.
 *
 * That ordering is the point. A teardown walk that visits the graph parent-first reaches
 * dia, then leaf, then mid -- so the shared child comes up for unmapping BEFORE a sibling
 * whose destructor still calls into it. A loader that unmaps each module as its own
 * destructor finishes passes every chain-shaped test and dies here.
 */

extern void mc_dl_trace_add(const char *); /* leaf */
extern int mc_dl_mid_call(void);           /* mid  */

int
mc_dl_dia_call(void)
{
  return mc_dl_mid_call() + 10;
}

__attribute__((constructor)) static void
ctor(void)
{
  mc_dl_trace_add("dia+");
}

__attribute__((destructor)) static void
dtor(void)
{
  /* reaches the leaf directly, and runs AFTER the leaf has already been queued for unmapping */
  mc_dl_trace_add("dia-");
}
