//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/linux/process/process.hpp"

#include "../../src/linux/process/environ.hpp"
#include "../../src/linux/process/which.hpp"

#include "../../src/linux/process/exec.hpp"
#include "../../src/linux/process/spawn.hpp"
#include "../../src/linux/process/spawn_basic.hpp"

namespace
{

void
env_surface(void)
{
  (void)micron::env_get("PATH");
  (void)micron::env_has("PATH");
  (void)environ;
}

void
which_surface(void)
{
  char out[micron::posix::path_max];
  (void)micron::which_into("sh", out, sizeof(out));
  (void)micron::which_into("sh", out);
}

}      // namespace

int
main(void)
{
  env_surface();
  which_surface();
  return 1;
}
