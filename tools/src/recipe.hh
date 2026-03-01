#pragma once

// all build toolchains go in here
// recipes are compiler/language specific, they tell duck what to do with the input data given (ie how to compile)

#include "recipes/gnu/config.hh"

#include "recipes/gnu/batch.hh"

namespace recipes
{
inline constexpr bool __using_gnu = true;
};     // namespace recipes

template <bool B> struct config_enable_if_gnu;

template <> struct config_enable_if_gnu<true> {
  using type = recipes::gnu::config_t;
};

using config_t = typename config_enable_if_gnu<recipes::__using_gnu>::type;
