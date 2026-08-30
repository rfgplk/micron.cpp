# Allocation

Include `<micron/allocator.hpp>` for the allocator family, `allocator_traits`, and arena resources. Allocators expose byte chunks;
container boundaries perform checked element-to-byte arithmetic. `allocator_traits<A>` normalizes the canonical
`allocate`/`resize`/`deallocate` surface and the legacy `create`/`grow`/`destroy` surface. An allocator that reports more usable bytes than
requested must expose `allocation_extent(bytes, alignment)`: it reapplies the allocator's monotonic, idempotent sizing rule without
allocating. Owned resources use that rule to recover the exact byte chunk from element capacity without storing another word in every
container.

The ordinary abcmalloc-backed allocators (`allocator_serial`, `allocator_small`, `allocator_constrained`, and `allocator_exact`) use abcmalloc's native block alignment directly through 32 bytes in a normal build. Larger alignments carry one pointer-sized recovery prefix. `MICRON_ALLOCATOR_CHECKS` adds adapter postcondition checks for trusted built-ins; custom allocators are checked without the macro.

## Explicit arenas

A local arena can own upstream blocks:

```cpp
micron::arena_resource<micron::allocator_exact<>> scratch{64 * 1024};
auto checkpoint = scratch.mark();
auto bytes = scratch.allocate<64>(4096);
// construct and use objects in bytes.ptr
scratch.rewind(checkpoint); // object lifetime is the caller's responsibility
scratch.release();
```

A caller-owned span is strict by default and is never freed:

```cpp
alignas(64) byte storage[4096];
micron::arena_resource<> local{{storage, sizeof(storage)}};
auto memory = local.allocate<32>(128);
// exhaustion throws; release() only rewinds storage
```

Pass `arena_overflow::upstream` to let an external-span arena add owned blocks. `reset()` retains every block; `release()` frees owned blocks and rewinds the external span. Resetting or rewinding while live objects refer to discarded storage is caller-owned undefined behavior.

Containers require a resource with static storage duration because their allocator parameter is a type:

```cpp
inline micron::arena_resource<> application_arena{256 * 1024};
using application_allocator = micron::arena_allocator<application_arena>;

micron::vector<u64, application_allocator> values;
```

`arena_sync::thread_confined` is the lock-free default. Use `arena_sync::shared` when several threads allocate from one resource; it serializes bumps with a TTAS lock. `allocator_monotonic` remains a separate, static tagged allocator.

## Explicit mapping and abcmalloc modes

- `fixed_map_allocator::create_at(address, bytes)` uses anonymous read/write `MAP_FIXED_NOREPLACE`. The address must be page aligned. It fails if the range is occupied and never substitutes another address or replaces an existing mapping.
- `allocator_immutable::create` creates a dedicated writable mapping. `allocator_immutable::seal(block)` makes only that mapping read-only; destroy it with the same allocator.
- `allocator_temporal::launder` may return an address that aliases an earlier temporal result. It intentionally has no `create` method and cannot be selected as a container allocator.
- `allocator_retiring` tombstones storage on destruction. `allocator_persistent` is available only when the whole translation unit is built with `MICRON_ABC_PERSISTENT`.
- `abc::freeze_sheet(pointer)` preserves the old sheet-wide freeze operation. Every unrelated allocation on that allocator sheet may become read-only. The compatibility name `abc::freeze` has the same hazard.
- `abc::mark_at` and `abc::unmark_at` register and unregister caller-owned ranges for provenance queries. They never map, free, or unmap the range. Duplicate and overlapping registrations are rejected.

Dedicated fixed, guarded, huge, secure, and immutable mappings participate in external provenance. `abc::within` accepts an interior address; `abc::is_present` and `abc::query_size` require the registered base.

`MICRON_ALLOCATOR_STATS` enables `allocator_stats_snapshot` counters on explicit arenas and instrumented allocator types. `MICRON_ABC_STATS` independently enables abcmalloc's process-wide `abc::stats()` counters. Both are compiled out by default; inspect the `enabled` field before treating an all-zero snapshot as a clean run.
