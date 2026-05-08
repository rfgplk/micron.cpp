//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"

// undef first if included from elsewhere
#undef _MM_FROUND_TO_NEAREST_INT
#undef _MM_FROUND_TO_NEG_INF
#undef _MM_FROUND_TO_POS_INF
#undef _MM_FROUND_TO_ZERO
#undef _MM_FROUND_CUR_DIRECTION
#undef _MM_FROUND_RAISE_EXC
#undef _MM_FROUND_NO_EXC
#undef _MM_FROUND_NINT
#undef _MM_FROUND_FLOOR
#undef _MM_FROUND_CEIL
#undef _MM_FROUND_TRUNC
#undef _MM_FROUND_RINT
#undef _MM_FROUND_NEARBYINT

#undef _MM_EXCEPT_MASK
#undef _MM_EXCEPT_INVALID
#undef _MM_EXCEPT_DENORM
#undef _MM_EXCEPT_DIV_ZERO
#undef _MM_EXCEPT_OVERFLOW
#undef _MM_EXCEPT_UNDERFLOW
#undef _MM_EXCEPT_INEXACT

#undef _MM_MASK_MASK
#undef _MM_MASK_INVALID
#undef _MM_MASK_DENORM
#undef _MM_MASK_DIV_ZERO
#undef _MM_MASK_OVERFLOW
#undef _MM_MASK_UNDERFLOW
#undef _MM_MASK_INEXACT

#undef _MM_ROUND_MASK
#undef _MM_ROUND_NEAREST
#undef _MM_ROUND_DOWN
#undef _MM_ROUND_UP
#undef _MM_ROUND_TOWARD_ZERO

#undef _MM_FLUSH_ZERO_MASK
#undef _MM_FLUSH_ZERO_ON
#undef _MM_FLUSH_ZERO_OFF

#undef _CMP_EQ_OQ
#undef _CMP_LT_OS
#undef _CMP_LE_OS
#undef _CMP_UNORD_Q
#undef _CMP_NEQ_UQ
#undef _CMP_NLT_US
#undef _CMP_NLE_US
#undef _CMP_ORD_Q
#undef _CMP_EQ_UQ
#undef _CMP_NGE_US
#undef _CMP_NGT_US
#undef _CMP_FALSE_OQ
#undef _CMP_NEQ_OQ
#undef _CMP_GE_OS
#undef _CMP_GT_OS
#undef _CMP_TRUE_UQ
#undef _CMP_EQ_OS
#undef _CMP_LT_OQ
#undef _CMP_LE_OQ
#undef _CMP_UNORD_S
#undef _CMP_NEQ_US
#undef _CMP_NLT_UQ
#undef _CMP_NLE_UQ
#undef _CMP_ORD_S
#undef _CMP_EQ_US
#undef _CMP_NGE_UQ
#undef _CMP_NGT_UQ
#undef _CMP_FALSE_OS
#undef _CMP_NEQ_OS
#undef _CMP_GE_OQ
#undef _CMP_GT_OQ
#undef _CMP_TRUE_US

#undef _MM_SHUFFLE
#undef _MM_SHUFFLE2
#undef _MM_MK_INSERTPS_NDX

namespace micron
{
namespace simd
{
namespace __bits
{

inline constexpr int _MM_FROUND_TO_NEAREST_INT = 0x00;
inline constexpr int _MM_FROUND_TO_NEG_INF = 0x01;
inline constexpr int _MM_FROUND_TO_POS_INF = 0x02;
inline constexpr int _MM_FROUND_TO_ZERO = 0x03;
inline constexpr int _MM_FROUND_CUR_DIRECTION = 0x04;

inline constexpr int _MM_FROUND_RAISE_EXC = 0x00;
inline constexpr int _MM_FROUND_NO_EXC = 0x08;

inline constexpr int _MM_FROUND_NINT = _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_RAISE_EXC;
inline constexpr int _MM_FROUND_FLOOR = _MM_FROUND_TO_NEG_INF | _MM_FROUND_RAISE_EXC;
inline constexpr int _MM_FROUND_CEIL = _MM_FROUND_TO_POS_INF | _MM_FROUND_RAISE_EXC;
inline constexpr int _MM_FROUND_TRUNC = _MM_FROUND_TO_ZERO | _MM_FROUND_RAISE_EXC;
inline constexpr int _MM_FROUND_RINT = _MM_FROUND_CUR_DIRECTION | _MM_FROUND_RAISE_EXC;
inline constexpr int _MM_FROUND_NEARBYINT = _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC;

inline constexpr int _MM_HINT_IT0 = 19;
inline constexpr int _MM_HINT_IT1 = 18;
inline constexpr int _MM_HINT_RST2 = 9;
inline constexpr int _MM_HINT_ET0 = 7;
inline constexpr int _MM_HINT_T0 = 3;
inline constexpr int _MM_HINT_T1 = 2;
inline constexpr int _MM_HINT_T2 = 1;
inline constexpr int _MM_HINT_NTA = 0;

inline constexpr int _MM_EXCEPT_MASK = 0x003f;
inline constexpr int _MM_EXCEPT_INVALID = 0x0001;
inline constexpr int _MM_EXCEPT_DENORM = 0x0002;
inline constexpr int _MM_EXCEPT_DIV_ZERO = 0x0004;
inline constexpr int _MM_EXCEPT_OVERFLOW = 0x0008;
inline constexpr int _MM_EXCEPT_UNDERFLOW = 0x0010;
inline constexpr int _MM_EXCEPT_INEXACT = 0x0020;

inline constexpr int _MM_MASK_MASK = 0x1f80;
inline constexpr int _MM_MASK_INVALID = 0x0080;
inline constexpr int _MM_MASK_DENORM = 0x0100;
inline constexpr int _MM_MASK_DIV_ZERO = 0x0200;
inline constexpr int _MM_MASK_OVERFLOW = 0x0400;
inline constexpr int _MM_MASK_UNDERFLOW = 0x0800;
inline constexpr int _MM_MASK_INEXACT = 0x1000;

inline constexpr int _MM_ROUND_MASK = 0x6000;
inline constexpr int _MM_ROUND_NEAREST = 0x0000;
inline constexpr int _MM_ROUND_DOWN = 0x2000;
inline constexpr int _MM_ROUND_UP = 0x4000;
inline constexpr int _MM_ROUND_TOWARD_ZERO = 0x6000;

inline constexpr int _MM_FLUSH_ZERO_MASK = 0x8000;
inline constexpr int _MM_FLUSH_ZERO_ON = 0x8000;
inline constexpr int _MM_FLUSH_ZERO_OFF = 0x0000;

inline constexpr int _CMP_EQ_OQ = 0x00;
inline constexpr int _CMP_LT_OS = 0x01;
inline constexpr int _CMP_LE_OS = 0x02;
inline constexpr int _CMP_UNORD_Q = 0x03;
inline constexpr int _CMP_NEQ_UQ = 0x04;
inline constexpr int _CMP_NLT_US = 0x05;
inline constexpr int _CMP_NLE_US = 0x06;
inline constexpr int _CMP_ORD_Q = 0x07;
inline constexpr int _CMP_EQ_UQ = 0x08;
inline constexpr int _CMP_NGE_US = 0x09;
inline constexpr int _CMP_NGT_US = 0x0a;
inline constexpr int _CMP_FALSE_OQ = 0x0b;
inline constexpr int _CMP_NEQ_OQ = 0x0c;
inline constexpr int _CMP_GE_OS = 0x0d;
inline constexpr int _CMP_GT_OS = 0x0e;
inline constexpr int _CMP_TRUE_UQ = 0x0f;
inline constexpr int _CMP_EQ_OS = 0x10;
inline constexpr int _CMP_LT_OQ = 0x11;
inline constexpr int _CMP_LE_OQ = 0x12;
inline constexpr int _CMP_UNORD_S = 0x13;
inline constexpr int _CMP_NEQ_US = 0x14;
inline constexpr int _CMP_NLT_UQ = 0x15;
inline constexpr int _CMP_NLE_UQ = 0x16;
inline constexpr int _CMP_ORD_S = 0x17;
inline constexpr int _CMP_EQ_US = 0x18;
inline constexpr int _CMP_NGE_UQ = 0x19;
inline constexpr int _CMP_NGT_UQ = 0x1a;
inline constexpr int _CMP_FALSE_OS = 0x1b;
inline constexpr int _CMP_NEQ_OS = 0x1c;
inline constexpr int _CMP_GE_OQ = 0x1d;
inline constexpr int _CMP_GT_OQ = 0x1e;
inline constexpr int _CMP_TRUE_US = 0x1f;

[[nodiscard]] consteval int
__mm_shuffle_imm(int fp3, int fp2, int fp1, int fp0) noexcept
{
  return (fp3 << 6) | (fp2 << 4) | (fp1 << 2) | fp0;
}

[[nodiscard]] consteval int
__mm_shuffle2_imm(int fp1, int fp0) noexcept
{
  return (fp1 << 1) | fp0;
}

[[nodiscard]] consteval int
__mm_mk_insertps_ndx_imm(int s, int d, int m) noexcept
{
  return (s << 6) | (d << 4) | m;
}

};     // namespace __bits
};     // namespace simd
};     // namespace micron

#if defined(MICRON_SIMD_INJECT_INTRIN_TYPES)

using ::micron::simd::__bits::_MM_FROUND_CEIL;
using ::micron::simd::__bits::_MM_FROUND_CUR_DIRECTION;
using ::micron::simd::__bits::_MM_FROUND_FLOOR;
using ::micron::simd::__bits::_MM_FROUND_NEARBYINT;
using ::micron::simd::__bits::_MM_FROUND_NINT;
using ::micron::simd::__bits::_MM_FROUND_NO_EXC;
using ::micron::simd::__bits::_MM_FROUND_RAISE_EXC;
using ::micron::simd::__bits::_MM_FROUND_RINT;
using ::micron::simd::__bits::_MM_FROUND_TO_NEAREST_INT;
using ::micron::simd::__bits::_MM_FROUND_TO_NEG_INF;
using ::micron::simd::__bits::_MM_FROUND_TO_POS_INF;
using ::micron::simd::__bits::_MM_FROUND_TO_ZERO;
using ::micron::simd::__bits::_MM_FROUND_TRUNC;

using ::micron::simd::__bits::_MM_HINT_ET0;
using ::micron::simd::__bits::_MM_HINT_IT0;
using ::micron::simd::__bits::_MM_HINT_IT1;
using ::micron::simd::__bits::_MM_HINT_NTA;
using ::micron::simd::__bits::_MM_HINT_RST2;
using ::micron::simd::__bits::_MM_HINT_T0;
using ::micron::simd::__bits::_MM_HINT_T1;
using ::micron::simd::__bits::_MM_HINT_T2;

using ::micron::simd::__bits::_MM_EXCEPT_DENORM;
using ::micron::simd::__bits::_MM_EXCEPT_DIV_ZERO;
using ::micron::simd::__bits::_MM_EXCEPT_INEXACT;
using ::micron::simd::__bits::_MM_EXCEPT_INVALID;
using ::micron::simd::__bits::_MM_EXCEPT_MASK;
using ::micron::simd::__bits::_MM_EXCEPT_OVERFLOW;
using ::micron::simd::__bits::_MM_EXCEPT_UNDERFLOW;

using ::micron::simd::__bits::_MM_MASK_DENORM;
using ::micron::simd::__bits::_MM_MASK_DIV_ZERO;
using ::micron::simd::__bits::_MM_MASK_INEXACT;
using ::micron::simd::__bits::_MM_MASK_INVALID;
using ::micron::simd::__bits::_MM_MASK_MASK;
using ::micron::simd::__bits::_MM_MASK_OVERFLOW;
using ::micron::simd::__bits::_MM_MASK_UNDERFLOW;

using ::micron::simd::__bits::_MM_ROUND_DOWN;
using ::micron::simd::__bits::_MM_ROUND_MASK;
using ::micron::simd::__bits::_MM_ROUND_NEAREST;
using ::micron::simd::__bits::_MM_ROUND_TOWARD_ZERO;
using ::micron::simd::__bits::_MM_ROUND_UP;

using ::micron::simd::__bits::_MM_FLUSH_ZERO_MASK;
using ::micron::simd::__bits::_MM_FLUSH_ZERO_OFF;
using ::micron::simd::__bits::_MM_FLUSH_ZERO_ON;

using ::micron::simd::__bits::_CMP_EQ_OQ;
using ::micron::simd::__bits::_CMP_EQ_OS;
using ::micron::simd::__bits::_CMP_EQ_UQ;
using ::micron::simd::__bits::_CMP_EQ_US;
using ::micron::simd::__bits::_CMP_FALSE_OQ;
using ::micron::simd::__bits::_CMP_FALSE_OS;
using ::micron::simd::__bits::_CMP_GE_OQ;
using ::micron::simd::__bits::_CMP_GE_OS;
using ::micron::simd::__bits::_CMP_GT_OQ;
using ::micron::simd::__bits::_CMP_GT_OS;
using ::micron::simd::__bits::_CMP_LE_OQ;
using ::micron::simd::__bits::_CMP_LE_OS;
using ::micron::simd::__bits::_CMP_LT_OQ;
using ::micron::simd::__bits::_CMP_LT_OS;
using ::micron::simd::__bits::_CMP_NEQ_OQ;
using ::micron::simd::__bits::_CMP_NEQ_OS;
using ::micron::simd::__bits::_CMP_NEQ_UQ;
using ::micron::simd::__bits::_CMP_NEQ_US;
using ::micron::simd::__bits::_CMP_NGE_UQ;
using ::micron::simd::__bits::_CMP_NGE_US;
using ::micron::simd::__bits::_CMP_NGT_UQ;
using ::micron::simd::__bits::_CMP_NGT_US;
using ::micron::simd::__bits::_CMP_NLE_UQ;
using ::micron::simd::__bits::_CMP_NLE_US;
using ::micron::simd::__bits::_CMP_NLT_UQ;
using ::micron::simd::__bits::_CMP_NLT_US;
using ::micron::simd::__bits::_CMP_ORD_Q;
using ::micron::simd::__bits::_CMP_ORD_S;
using ::micron::simd::__bits::_CMP_TRUE_UQ;
using ::micron::simd::__bits::_CMP_TRUE_US;
using ::micron::simd::__bits::_CMP_UNORD_Q;
using ::micron::simd::__bits::_CMP_UNORD_S;

#define _MM_SHUFFLE(fp3, fp2, fp1, fp0) (::micron::simd::__bits::__mm_shuffle_imm((fp3), (fp2), (fp1), (fp0)))
#define _MM_SHUFFLE2(fp1, fp0) (::micron::simd::__bits::__mm_shuffle2_imm((fp1), (fp0)))
#define _MM_MK_INSERTPS_NDX(S, D, M) (::micron::simd::__bits::__mm_mk_insertps_ndx_imm((S), (D), (M)))

#endif
