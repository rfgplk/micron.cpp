/*  Copyright (c) 2026- David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 *
 * The bottom of the fixture dependency chain: top -> mid -> leaf.
 *
 * The trace buffer lives HERE and nowhere else, so mid's and top's constructors have to reach it
 * through a cross-module PLT call. That is the property under test: if the loader initialises a
 * parent before its child is relocated, the call lands on an unbound PLT slot and dies. Ordering
 * bugs in a dynamic loader are otherwise invisible -- everything looks loaded.
 */

static char trace_buf[512];
static unsigned trace_len;

void
mc_dl_trace_add(const char *s)
{
  while ( *s && trace_len + 1 < sizeof(trace_buf) ) trace_buf[trace_len++] = *s++;
  trace_buf[trace_len] = 0;
}

const char *
mc_dl_trace_get(void)
{
  return trace_buf;
}

void
mc_dl_trace_reset(void)
{
  trace_len = 0;
  trace_buf[0] = 0;
}

/* set by the test before dlclose so a destructor can be observed after the mapping is gone */
int *mc_dl_dtor_flag;

int mc_dl_leaf_global = 7;

int
mc_dl_leaf_value(void)
{
  return 42;
}

/* returns the address of a module-local static: two independent mappings return two addresses,
   one refcounted mapping returns one. this is how the dedup test tells them apart. */
void *
mc_dl_leaf_identity(void)
{
  return (void *)&trace_buf[0];
}

__attribute__((constructor)) static void
ctor(void)
{
  mc_dl_trace_add("leaf+");
}

__attribute__((destructor)) static void
dtor(void)
{
  mc_dl_trace_add("leaf-");
  if ( mc_dl_dtor_flag ) *mc_dl_dtor_flag |= 1;
}
