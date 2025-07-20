//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include <fpu_control.h>
#include <stdlib.h>

#ifndef _GET_ROUNDING_MODE_H
#define _GET_ROUNDING_MODE_H 1
/* Define values for FE_* modes not defined for this architecture.  */
#ifdef FE_DOWNWARD
#define ORIG_FE_DOWNWARD FE_DOWNWARD
#else
#define ORIG_FE_DOWNWARD 0
#endif
#ifdef FE_TONEAREST
#define ORIG_FE_TONEAREST FE_TONEAREST
#else
#define ORIG_FE_TONEAREST 0
#endif
#ifdef FE_TOWARDZERO
#define ORIG_FE_TOWARDZERO FE_TOWARDZERO
#else
#define ORIG_FE_TOWARDZERO 0
#endif
#ifdef FE_UPWARD
#define ORIG_FE_UPWARD FE_UPWARD
#else
#define ORIG_FE_UPWARD 0
#endif
#define FE_CONSTRUCT_DISTINCT_VALUE(X, Y, Z) ((((X) & 1) | ((Y) & 2) | ((Z) & 4)) ^ 7)
#ifndef FE_DOWNWARD
#define FE_DOWNWARD FE_CONSTRUCT_DISTINCT_VALUE(ORIG_FE_TONEAREST, ORIG_FE_TOWARDZERO, ORIG_FE_UPWARD)
#endif
#ifndef FE_TONEAREST
#define FE_TONEAREST FE_CONSTRUCT_DISTINCT_VALUE(FE_DOWNWARD, ORIG_FE_TOWARDZERO, ORIG_FE_UPWARD)
#endif
#ifndef FE_TOWARDZERO
#define FE_TOWARDZERO FE_CONSTRUCT_DISTINCT_VALUE(FE_DOWNWARD, FE_TONEAREST, ORIG_FE_UPWARD)
#endif
#ifndef FE_UPWARD
#define FE_UPWARD FE_CONSTRUCT_DISTINCT_VALUE(FE_DOWNWARD, FE_TONEAREST, FE_TOWARDZERO)
#endif

/* Return the floating-point rounding mode.  */

static inline int
get_rounding_mode(void)
{
#if ( defined _FPU_RC_DOWN || defined _FPU_RC_NEAREST || defined _FPU_RC_ZERO || defined _FPU_RC_UP )
  fpu_control_t fc;
  const fpu_control_t mask = (0
#ifdef _FPU_RC_DOWN
                              | _FPU_RC_DOWN
#endif
#ifdef _FPU_RC_NEAREST
                              | _FPU_RC_NEAREST
#endif
#ifdef _FPU_RC_ZERO
                              | _FPU_RC_ZERO
#endif
#ifdef _FPU_RC_UP
                              | _FPU_RC_UP
#endif
  );

  _FPU_GETCW(fc);
  switch ( fc & mask ) {
#ifdef _FPU_RC_DOWN
  case _FPU_RC_DOWN:
    return FE_DOWNWARD;
#endif

#ifdef _FPU_RC_NEAREST
  case _FPU_RC_NEAREST:
    return FE_TONEAREST;
#endif

#ifdef _FPU_RC_ZERO
  case _FPU_RC_ZERO:
    return FE_TOWARDZERO;
#endif

#ifdef _FPU_RC_UP
  case _FPU_RC_UP:
    return FE_UPWARD;
#endif

  default:
    abort();
  }
#else
  return FE_TONEAREST;
#endif
}

#endif /* get-rounding-mode.h */
