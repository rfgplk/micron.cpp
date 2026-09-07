//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../cmalloc.hpp"
#include "../../types.hpp"
#include "__seam.hpp"
#if defined(__micron_bb_alloc)
#include "barebones/bb_alloc.hpp"
#endif

// the system-allocator fallback is a HOSTED-only path. It used to be selected purely on the absence
// of MICRON_ABCMALLOC_STD, so a freestanding build that set ABCMALLOC_DISABLE -- which defs.hpp does
// by itself under a heap-owning sanitizer -- silently pulled <cstdlib> and called ::malloc, in a
// library whose first hard rule is that there is no libc.
#if !defined(__micron_abcmalloc_std_backend) && !defined(__micron_bb_alloc)
#if defined(__micron_freestanding)
#error "micron: no allocator. A freestanding build has no ::malloc to fall back on -- leave MICRON_ABCMALLOC_STD set, or build with -DMICRON_BAREBONES_ALLOC (duck: --kernel)."
#endif
/*permitted*/ #include<cstdlib>
#endif
namespace micron
{
#if defined(__micron_abcmalloc_std_backend)
inline __attribute__((always_inline)) byte *
__alloc(usize sz)
{
  return abc::alloc(sz);
}

template<typename T>
inline __attribute__((always_inline)) void
__free(T *ptr)
{
  abc::dealloc(reinterpret_cast<byte *>(ptr));
}

inline constexpr usize __native_alignment = abc::native_block_alignment;

inline __attribute__((always_inline)) void *
__alloc_aligned(usize alignment, usize bytes)
{
  return abc::aligned_alloc(alignment, bytes);
}

inline __attribute__((always_inline)) void
__free_aligned(void *ptr, usize alignment)
{
  if ( alignment <= __native_alignment )
    abc::dealloc(reinterpret_cast<byte *>(ptr));
  else
    abc::aligned_free(ptr);
}

// __seam.hpp declares this [[gnu::weak]] and calls it through micron::__heap_owns, which supplies
// the conservative answer when no allocator is linked. This is the definition that binds when one
// is; keep the signature IDENTICAL to the declaration there.
inline bool
__heap_owns_provider(const void *__ptr) noexcept
{
  return abc::within(reinterpret_cast<const addr_t *>(__ptr));
}
#elif defined(__micron_bb_alloc)
// the barebones tier. Same shape as the abc arm above -- abc:: IS bb:: here (abc_shim.hpp) -- but
// spelled through bb:: directly, because this header is included by cmalloc.hpp itself and must
// not depend on the alias surface having landed first.
inline __attribute__((always_inline)) byte *
__alloc(usize sz)
{
  return micron::bb::alloc(sz);
}

template<typename T>
inline __attribute__((always_inline)) void
__free(T *ptr)
{
  micron::bb::dealloc(reinterpret_cast<byte *>(ptr));
}

inline constexpr usize __native_alignment = micron::bb::native_alignment;

// bb::, not abc:: -- the arm's own rule, four lines up, and this was the one name that broke it.
// It worked only because cmalloc.hpp:18 happens to land abc_shim.hpp before this header's namespace
// block, which is precisely the ordering dependency the comment says must not exist. It also meant
// this arm inherited abc_shim's C11 `size % alignment != 0 -> nullptr` rule, which bb::aligned_balloc
// does not itself impose.
inline __attribute__((always_inline)) void *
__alloc_aligned(usize alignment, usize bytes)
{
  auto __c = micron::bb::aligned_balloc(alignment, bytes);
  return static_cast<void *>(__c.ptr);
}

inline __attribute__((always_inline)) void
__free_aligned(void *ptr, usize alignment)
{
  if ( alignment <= __native_alignment )
    micron::bb::dealloc(ptr);
  else
    micron::bb::aligned_free(ptr);
}

// see the abc arm above, and __seam.hpp's banner
inline bool
__heap_owns_provider(const void *__ptr) noexcept
{
  return micron::bb::within(__ptr);
}
#else
// THE ZERO CASE IS NORMALISED TO MATCH THE OTHER TWO ARMS. abc::alloc(0) and bb::alloc(0) both
// answer nullptr; ::malloc(0) answers a valid unique pointer. Callers that do not special-case zero
// -- array/parray.hpp, vector/pvector.hpp, string/rope.hpp and maps/itable.hpp all call __alloc
// directly, and only memory/new.hpp:50,62,134,143 bumps a zero to one -- would otherwise get a
// different answer per allocator for the same request. nullptr is the one the tree already treats
// as "nothing was allocated".
inline __attribute__((always_inline)) void *
__alloc(usize sz)
{
  if ( sz == 0 ) return nullptr;
  return ::malloc(sz);
}

template<typename T>
inline __attribute__((always_inline)) void
__free(T *ptr)
{
  ::free(ptr);
}

inline constexpr usize __native_alignment = sizeof(void *) * 2;

inline __attribute__((always_inline)) void *
__alloc_aligned(usize alignment, usize bytes)
{
  return ::aligned_alloc(alignment, bytes);
}

inline __attribute__((always_inline)) void
__free_aligned(void *ptr, usize)
{
  ::free(ptr);
}

// the libc arm has NO ownership oracle -- ::malloc will not say whether it handed a pointer out.
// true is the honest answer for a hint whose only caller (cmemory's __is_valid_address) treats
// false as "reject this call": answering false here would make every mem* over heap memory refuse.
inline bool
__heap_owns_provider(const void *) noexcept
{
  return true;
}
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// COUNT x SIZE, CHECKED. Arm-independent: it is arithmetic, and every arm needs it.
//
// __alloc takes a byte count, so every caller that wants an array multiplies first -- and none of
// them checked. On wrap the multiply produces a SMALL number, the allocation succeeds, and the
// caller then writes `count` elements into it: a heap overflow, not an allocation failure. Live
// sites were vector/pvector.hpp:1266 and string/rope.hpp:111,187,418,642,722.
//
// bbmalloc is already careful about this one level down (check_mul_overflow, bbmalloc/malloc.hpp:365)
// and abcmalloc's C shim does it for calloc; the gap was in micron's own layer above both.
//
// nullptr on overflow, because that is what these call sites already do with a failed allocation --
// the same reason __alloc's zero case answers nullptr rather than throwing.
inline __attribute__((always_inline)) void *
__alloc_n(usize __count, usize __size)
{
  usize __bytes = 0;
  if ( __builtin_mul_overflow(__count, __size, &__bytes) ) return nullptr;
  return static_cast<void *>(micron::__alloc(__bytes));
}

};      // namespace micron
