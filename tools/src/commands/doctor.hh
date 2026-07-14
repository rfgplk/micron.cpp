#pragma once

#include "build.hh"

// diagnostic mode, hooks into build but sets diagnostic flags

inline void
recipe_doctor(recipes::gnu::config_t &conf)
{
  conf.doctor = true;
}

template <bool Wait = mc::exec_wait>
  requires(recipes::__using_gnu)
int
doctor(recipes::gnu::config_t &conf)
{
  recipe_doctor(conf);
  return mc::wexitstatus(build(conf));
}

template <bool Wait = mc::exec_wait>
  requires(recipes::__using_gnu)
int
doctor_debug(recipes::gnu::config_t &conf)
{
  recipe_doctor(conf);
  return build_debug(conf);
}
