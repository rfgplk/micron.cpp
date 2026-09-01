// Copyright (c) 2025 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#pragma once

#include "../../../types.hpp"
#include "../../../version.hpp"

#include "arena.hpp"
#include "external.hpp"
// malloc.hpp includes us ahead of doctor.hpp, so name the dependency ourselves rather than rely on order
#if defined(ABCMALLOC_DOCTOR_HELP)
#include "doctor.hpp"
#endif
#include "sheet_header.hpp"
#include "tapi.hpp"
#include "va_reserve.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// abcmalloc inspection abi
//
// this is a (cheap) retained descriptor naming where the allocator's pages live and how its records are laid out;
// used by an external debugger or memory auditor
//
// WARNING: SYMBOLS CANNOT BE USED FOR DISCOVERY && THE LAYOUT OF THIS STRUCT NEVER VARIES WITH A MACRO

namespace abc
{

struct __ins_probe {
  static constexpr usize off_precise = __builtin_offsetof(__arena, _precise);
  static constexpr usize off_small = __builtin_offsetof(__arena, _small);
  static constexpr usize off_medium = __builtin_offsetof(__arena, _medium);
  static constexpr usize off_large = __builtin_offsetof(__arena, _large);
  static constexpr usize off_huge = __builtin_offsetof(__arena, _huge);
  static constexpr usize off_internal = __builtin_offsetof(__arena, _arena_tier);
  static constexpr usize off_remote_free = __builtin_offsetof(__arena, __remote_free);

  using precise_sheet = tlsf_sheet<__class_precise>;
  using medium_sheet = sheet<__class_medium>;

  static constexpr usize off_sheet_kmem = __builtin_offsetof(medium_sheet, __kernel_memory);
  static constexpr usize off_sheet_book = __builtin_offsetof(medium_sheet, __book);
  static constexpr usize off_sheet_guard = __builtin_offsetof(medium_sheet, __guard_offset);
  static constexpr usize sizeof_sheet = sizeof(medium_sheet);

  static constexpr usize off_tsheet_kmem = __builtin_offsetof(precise_sheet, __kernel_memory);
  static constexpr usize off_tsheet_book = __builtin_offsetof(precise_sheet, __book);
  static constexpr usize off_tsheet_guard = __builtin_offsetof(precise_sheet, __guard_offset);
  static constexpr usize sizeof_tsheet = sizeof(precise_sheet);
};

inline constexpr u32 abc_inspect_magic = 0x49434241u;      // 'ABCI'
inline constexpr u16 abc_inspect_version = 1u;

// feature flags
inline constexpr u32 abc_f_multithread = 1u << 0;
inline constexpr u32 abc_f_doctor = 1u << 1;
inline constexpr u32 abc_f_stats = 1u << 2;
inline constexpr u32 abc_f_redzone = 1u << 3;
inline constexpr u32 abc_f_guard_pages = 1u << 4;
inline constexpr u32 abc_f_persistent = 1u << 5;
inline constexpr u32 abc_f_provenance = 1u << 6;
inline constexpr u32 abc_f_poison_free = 1u << 7;

// tier's kind, which decides how its sheets are walked
inline constexpr u8 abc_tier_tlsf = 1;
inline constexpr u8 abc_tier_buddy = 2;

inline constexpr u32 abc_n_tiers = 6;      // five user tiers, then the arena's own metadata tier

struct abc_tier_desc {
  u64 off;             // byte offset of the tier within __arena
  u64 class_size;      // size class it serves
  u32 max_sheets;      // how many __idx entries it has
  u32 cache_slots;
  u8 kind;      // abc_tier_tlsf or abc_tier_buddy
  u8 user;      // 0 for the allocator's own metadata tier
  // per tier
  u8 min_shift;
  u8 pad[5];
  u32 off_idx;
  u32 off_count;
  u32 off_mask;
  u32 off_cache;
  u32 cache_off_size;
  u32 cache_off_ptr;
  u32 cache_off_count;
  u32 pad2;
};

// WARNING: this struct's layout is fixed, only aeppend
struct abc_inspect_desc {
  u32 magic;         // abc_inspect_magic
  u16 version;       // abc_inspect_version
  u16 abi;           // MICRON_ABI
  u32 size;          // sizeof(abc_inspect_desc) as this build compiled it
  u32 flags;         // abc_f_*
  u8 ptr_width;      // 8 or 4
  u8 endian;         // 1 little, 2 big
  u8 arch;           // 1 amd64, 2 arm64, 3 arm32, 4 i386, 0 other
  u8 n_tiers;
  u32 pad0;
  u64 config_fp;      // an fnv over every layout affecting constant below

  // abc roots
  const void *va_base;      // atomic_token<addr_t *>: the reservation base
  const void *va_offset;
  const void *block_owner_table;      // __arena *[num_blocks], indexed by (p - va_base) >> align
  const void *oor_head;               // sheets that fell outside the reservation
  const void *oor_live;
  const void *arena_pool;      // __arena *[max_arenas]
  const void *arena_pool_next;
  const void *arena_owner;         // atomic_token<i32>[max_arenas]: a kernel tid per slot
  const void *overflow_head;       // __arena_node * LIFO, past max_arenas
  const void *external_pages;      // caller registered ranges
  const void *external_count;
  const void *doctor_state;      // null unless ABCMALLOC_DOCTOR_HELP

  // geometry
  u64 va_reservation_size;
  u64 num_blocks;
  u32 max_arenas;
  u32 sheet_align_log2;
  u32 hdr_offset;
  u32 redzone_size;      // 0 unless abc_f_redzone
  u64 sizeof_arena;
  u64 sizeof_arena_node;

  // arena -> tier -> range -> node -> sheet -> book
  u32 off_tier_idx;
  u32 off_tier_count;
  u32 off_tier_mask;
  u32 sizeof_range;
  u32 off_range_lo;
  u32 off_range_hi;
  u32 off_range_nd;
  u32 sizeof_node;
  u32 off_node_nd;

  u32 off_sheet_kmem;      // micron::__chunk<byte>{ptr, len}
  u32 off_sheet_book;
  u32 off_sheet_guard;
  u32 sizeof_sheet;
  u32 off_tsheet_kmem;
  u32 off_tsheet_book;
  u32 off_tsheet_guard;
  u32 sizeof_tsheet;

  // books themselves
  u32 off_buddy_base;
  u32 off_buddy_total;
  u32 off_buddy_max_order;
  u32 off_buddy_allocated;
  u32 off_buddy_tombstoned;
  u32 off_buddy_tags;      // u8 *
  u32 off_buddy_tag_count;
  u32 off_buddy_tcache;            // free_block *[max_order]
  u32 off_buddy_tcache_count;      // i32[max_order]
  u32 off_buddy_cold;
  u32 off_buddy_cold_count;
  u32 buddy_orders;      // how many entries those four arrays have
  u32 buddy_tag_free;
  u32 buddy_tag_none;
  u32 buddy_min_shift;

  u32 off_tlsf_base;
  u32 off_tlsf_total;
  u32 off_tlsf_allocated;
  u32 off_tlsf_tombstoned;
  u32 sizeof_tlsf_hdr;      // and the header sits at user_ptr - hdr_offset
  u32 off_tlsf_hdr_bsize;
  u32 off_tlsf_hdr_flags;
  u32 pad1;

  u64 off_remote_free;
  u64 remote_capacity;

  abc_tier_desc tiers[abc_n_tiers];
};

namespace __ins
{

constexpr u64
fnv(u64 h, u64 v)
{
  h ^= v;
  return h * 1099511628211ull;
}

constexpr u8
arch_id(void)
{
#if defined(__micron_arch_amd64)
  return 1;
#elif defined(__micron_arch_arm64)
  return 2;
#elif defined(__micron_arch_arm32)
  return 3;
#elif defined(__micron_arch_x86)
  return 4;
#else
  return 0;
#endif
}

constexpr u32
flags(void)
{
  u32 f = 0;
  if constexpr ( __default_multithread_safe ) f |= abc_f_multithread;
#if defined(ABCMALLOC_DOCTOR_HELP)
  f |= abc_f_doctor;
#endif
#if defined(MICRON_ABC_STATS)
  f |= abc_f_stats;
#endif
  if constexpr ( __default_redzone ) f |= abc_f_redzone;
  if constexpr ( __default_insert_guard_pages ) f |= abc_f_guard_pages;
  if constexpr ( __default_persistent_mode ) f |= abc_f_persistent;
  if constexpr ( __default_enforce_provenance ) f |= abc_f_provenance;
  if constexpr ( __default_poison_on_free ) f |= abc_f_poison_free;
  return f;
}

using buddy_book = __buddy_list<micron::__chunk<byte>, __class_medium, 64>;
using tlsf_book = __tlsf_list<micron::__chunk<byte>, __class_precise, 64>;

// every quantity that decides where a byte is. two builds agreeing here agree on the layout
constexpr u64
config_fp(void)
{
  u64 h = 14695981039346656037ull;
  h = fnv(h, sizeof(__arena));
  h = fnv(h, __max_arenas);
  h = fnv(h, __va_reservation_size);
  h = fnv(h, __sheet_align_log2);
  h = fnv(h, __hdr_offset);
  h = fnv(h, flags());
  h = fnv(h, __ins_probe::off_precise);
  h = fnv(h, __ins_probe::off_huge);
  h = fnv(h, sizeof(buddy_book));
  h = fnv(h, sizeof(tlsf_book));
  h = fnv(h, __arena::__ins_sizeof_range);
  h = fnv(h, arch_id());
  h = fnv(h, sizeof(void *));
  return h;
}

constexpr u8
shift_of(u64 v)
{
  u8 r = 0;
  while ( v > 1 ) {
    v >>= 1;
    r++;
  }
  return r;
}

template<typename T>
constexpr abc_tier_desc
tier(u64 off, u64 cls, u32 sheets, u32 slots, u8 kind, u8 user)
{
  abc_tier_desc t{};
  t.off_idx = static_cast<u32>(__arena::__ins_tier_off_idx<T>);
  t.off_count = static_cast<u32>(__arena::__ins_tier_off_count<T>);
  t.off_mask = static_cast<u32>(__arena::__ins_tier_off_mask<T>);
  t.off_cache = static_cast<u32>(__arena::__ins_tier_off_cache<T>);
  t.cache_off_size = static_cast<u32>(__arena::__ins_cache_off_size<typename T::__ins_cache_t>());
  t.cache_off_ptr = static_cast<u32>(__arena::__ins_cache_off_ptr<typename T::__ins_cache_t>());
  t.cache_off_count = static_cast<u32>(__arena::__ins_cache_off_count<typename T::__ins_cache_t>());
  t.off = off;
  t.class_size = cls;
  t.max_sheets = sheets;
  t.cache_slots = slots;
  t.kind = kind;
  t.user = user;
  t.min_shift = shift_of(cls);
  return t;
}

constexpr abc_inspect_desc
build(void)
{
  using A = __arena;
  using P = __ins_probe;
  abc_inspect_desc d{};
  d.magic = abc_inspect_magic;
  d.version = abc_inspect_version;
  d.abi = static_cast<u16>(MICRON_ABI);
  d.size = static_cast<u32>(sizeof(abc_inspect_desc));
  d.flags = flags();
  d.ptr_width = static_cast<u8>(sizeof(void *));
  d.endian = 1;
  d.arch = arch_id();
  d.n_tiers = static_cast<u8>(abc_n_tiers);
  d.config_fp = config_fp();

  d.va_base = &__va_base;
  d.va_offset = &__va_offset;
  d.block_owner_table = &__block_owner_table[0];
  d.oor_head = &__oor_head;
  d.oor_live = &__oor_live;
  d.arena_pool = &__arena_pool[0];
  d.arena_pool_next = &__arena_pool_next;
  d.arena_owner = &__arena_owner[0];
  d.overflow_head = &__overflow_head;
  d.external_pages = &__external_pages;
  d.external_count = &__external_count;
#if defined(ABCMALLOC_DOCTOR_HELP)
  d.doctor_state = &doctor::__dr;      // flags() advertises abc_f_doctor; a null here would be a lie
#else
  d.doctor_state = nullptr;
#endif

  d.va_reservation_size = __va_reservation_size;
  d.num_blocks = __num_blocks;
  d.max_arenas = __max_arenas;
  d.sheet_align_log2 = static_cast<u32>(__sheet_align_log2);
  d.hdr_offset = static_cast<u32>(__hdr_offset);
  d.redzone_size = __default_redzone ? static_cast<u32>(__default_redzone_size) : 0u;
  d.sizeof_arena = sizeof(__arena);
  d.sizeof_arena_node = sizeof(__arena_node);

  d.off_tier_idx = static_cast<u32>(A::__ins_tier_off_idx<A::__ins_precise_t>);
  d.off_tier_count = static_cast<u32>(A::__ins_tier_off_count<A::__ins_precise_t>);
  d.off_tier_mask = static_cast<u32>(A::__ins_tier_off_mask<A::__ins_precise_t>);
  d.sizeof_range = static_cast<u32>(A::__ins_sizeof_range);
  d.off_range_lo = static_cast<u32>(A::__ins_off_range_lo);
  d.off_range_hi = static_cast<u32>(A::__ins_off_range_hi);
  d.off_range_nd = static_cast<u32>(A::__ins_off_range_nd);
  d.sizeof_node = static_cast<u32>(A::__ins_sizeof_node);
  d.off_node_nd = static_cast<u32>(A::__ins_off_node_nd);

  d.off_sheet_kmem = static_cast<u32>(P::off_sheet_kmem);
  d.off_sheet_book = static_cast<u32>(P::off_sheet_book);
  d.off_sheet_guard = static_cast<u32>(P::off_sheet_guard);
  d.sizeof_sheet = static_cast<u32>(P::sizeof_sheet);
  d.off_tsheet_kmem = static_cast<u32>(P::off_tsheet_kmem);
  d.off_tsheet_book = static_cast<u32>(P::off_tsheet_book);
  d.off_tsheet_guard = static_cast<u32>(P::off_tsheet_guard);
  d.sizeof_tsheet = static_cast<u32>(P::sizeof_tsheet);

  d.off_buddy_base = static_cast<u32>(__builtin_offsetof(buddy_book, base));
  d.off_buddy_total = static_cast<u32>(__builtin_offsetof(buddy_book, total));
  d.off_buddy_max_order = static_cast<u32>(__builtin_offsetof(buddy_book, max_order));
  d.off_buddy_allocated = static_cast<u32>(__builtin_offsetof(buddy_book, allocated_bytes));
  d.off_buddy_tombstoned = static_cast<u32>(__builtin_offsetof(buddy_book, tombstoned_bytes));
  d.off_buddy_tags = static_cast<u32>(__builtin_offsetof(buddy_book, block_tags));
  d.off_buddy_tag_count = static_cast<u32>(__builtin_offsetof(buddy_book, tag_count));
  d.buddy_tag_free = buddy_book::__tag_free;
  d.off_buddy_tcache = static_cast<u32>(__builtin_offsetof(buddy_book, tcache));
  d.off_buddy_tcache_count = static_cast<u32>(__builtin_offsetof(buddy_book, tcache_count));
  d.off_buddy_cold = static_cast<u32>(__builtin_offsetof(buddy_book, cold_cache));
  d.off_buddy_cold_count = static_cast<u32>(__builtin_offsetof(buddy_book, cold_count));
  d.buddy_orders = 64;
  d.buddy_tag_none = buddy_book::__tag_none;
  // unlike every other off_buddy_* above, __log2_min varies with the book's Min parameter rather than
  // its Mx, so this one value covers the medium tier alone -- per tier, read tiers[i].min_shift
  d.buddy_min_shift = static_cast<u32>(buddy_book::__log2_min);

  d.off_tlsf_base = static_cast<u32>(__builtin_offsetof(tlsf_book, base));
  d.off_tlsf_total = static_cast<u32>(__builtin_offsetof(tlsf_book, total));
  d.off_tlsf_allocated = static_cast<u32>(__builtin_offsetof(tlsf_book, allocated_bytes));
  d.off_tlsf_tombstoned = static_cast<u32>(__builtin_offsetof(tlsf_book, tombstoned_bytes));
  d.sizeof_tlsf_hdr = static_cast<u32>(sizeof(typename tlsf_book::tlsf_hdr));
  d.off_tlsf_hdr_bsize = static_cast<u32>(__builtin_offsetof(typename tlsf_book::tlsf_hdr, bsize));
  d.off_tlsf_hdr_flags = static_cast<u32>(__builtin_offsetof(typename tlsf_book::tlsf_hdr, flags));

  d.off_remote_free = P::off_remote_free;
  d.remote_capacity = 64;

  using A = __arena;
  d.tiers[0] = tier<A::__ins_precise_t>(P::off_precise, __class_precise, __max_sheets_precise, __cache_slots_precise, abc_tier_tlsf, 1);
  d.tiers[1] = tier<A::__ins_small_t>(P::off_small, __class_small, __max_sheets_small, __cache_slots_small, abc_tier_tlsf, 1);
  d.tiers[2] = tier<A::__ins_medium_t>(P::off_medium, __class_medium, __max_sheets_medium, __cache_slots_medium, abc_tier_buddy, 1);
  d.tiers[3] = tier<A::__ins_large_t>(P::off_large, __class_large, __max_sheets_large, __cache_slots_large, abc_tier_buddy, 1);
  d.tiers[4] = tier<A::__ins_huge_t>(P::off_huge, __class_huge, __max_sheets_huge, 0, abc_tier_buddy, 1);
  d.tiers[5] = tier<A::__ins_internal_t>(P::off_internal, __class_arena_internal, __max_sheets_arena_internal, 0, abc_tier_buddy, 0);
  return d;
}

};      // namespace __ins

// used + retain are both required so flto / optimizer doesn't eat it
[[gnu::used, gnu::retain, gnu::section(".micron.abcmalloc")]] inline constinit const abc_inspect_desc __abc_inspect = __ins::build();

constexpr bool
abc_inspect_valid(const abc_inspect_desc *d)
{
  if ( d == nullptr ) return false;
  if ( d->magic != abc_inspect_magic ) return false;
  if ( d->version != abc_inspect_version ) return false;
  if ( d->abi != static_cast<u16>(MICRON_ABI) ) return false;
  if ( d->size < sizeof(abc_inspect_desc) ) return false;
  if ( d->ptr_width != sizeof(void *) ) return false;
  if ( d->n_tiers != abc_n_tiers ) return false;
  return true;
}

namespace __ins
{
inline constexpr abc_inspect_desc __self = build();
};      // namespace __ins

static_assert(sizeof(abc_inspect_desc) % 8 == 0, "the descriptor is read as words by the inspector");
static_assert(abc_inspect_valid(&__ins::__self), "this build's own descriptor must satisfy the handshake");
static_assert(__ins::__self.n_tiers == abc_n_tiers);
static_assert(__ins::__self.tiers[0].kind == abc_tier_tlsf && __ins::__self.tiers[1].kind == abc_tier_tlsf);
static_assert(__ins::__self.tiers[2].kind == abc_tier_buddy && __ins::__self.tiers[5].kind == abc_tier_buddy);
static_assert(__ins::__self.tiers[5].user == 0, "the metadata tier is not a user tier");
static_assert(__ins::__self.tiers[0].class_size < __ins::__self.tiers[1].class_size);
static_assert(__ins::__self.tiers[1].class_size < __ins::__self.tiers[2].class_size);
static_assert(__ins::__self.tiers[2].class_size < __ins::__self.tiers[3].class_size);
static_assert(__ins::__self.tiers[3].class_size < __ins::__self.tiers[4].class_size);
static_assert((1ull << __ins::__self.tiers[2].min_shift) == __ins::__self.tiers[2].class_size);
static_assert((1ull << __ins::__self.tiers[4].min_shift) == __ins::__self.tiers[4].class_size);
static_assert(__ins::__self.tiers[2].min_shift != __ins::__self.tiers[4].min_shift);

// the reader walks tier_base + off_idx as __range[max_sheets] and reads __count immediately after it;
// that is the one relation it cannot get wrong. checking it per tier also proves the single published
// sizeof_range describes all six, and that no offset truncated into its u32 field
//
// NOTE: deliberately NOT an inequality between two tiers. config_embed gives every tier 64 sheets, so
// tiers[0].off_count == tiers[3].off_count there is correct, not a defect
constexpr bool
__ins_tiers_consistent(void)
{
  const auto &d = __ins::__self;
  for ( u32 i = 0; i < abc_n_tiers; ++i ) {
    if ( d.tiers[i].off_idx != d.tiers[0].off_idx ) return false;
    if ( d.tiers[i].off_count
         != static_cast<u64>(d.tiers[i].off_idx) + static_cast<u64>(d.tiers[i].max_sheets) * d.sizeof_range )
      return false;
  }
  return true;
}

static_assert(__ins_tiers_consistent(), "the tier table's offsets must agree with its geometry");
// buddy_min_shift describes the medium instantiation only; a per tier reader wants tiers[i].min_shift
static_assert(__ins::__self.buddy_min_shift == __ins::__self.tiers[2].min_shift);

};      // namespace abc
