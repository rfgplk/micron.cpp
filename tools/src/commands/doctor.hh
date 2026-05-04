#pragma once

#include "build.hh"

// diagnostic mode, hooks into build but sets diagnostic flags

template <bool Wait = mc::exec_wait>
  requires(recipes::__using_gnu)
int
doctor(recipes::gnu::config_t &conf)
{
  conf.doctor = true;
  return build(conf);
}

template <bool Wait = mc::exec_wait>
  requires(recipes::__using_gnu)
int
doctor_debug(recipes::gnu::config_t &conf)
{
  conf.doctor = true;
  return build_debug(conf);
}
