/*  Copyright (c) 2026- David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 *
 * A dependency-free module, for the tests that want a load with no closure behind it.
 */

int
mc_dl_solo_value(void)
{
  return 99;
}

const char *
mc_dl_solo_name(void)
{
  return "solo";
}
