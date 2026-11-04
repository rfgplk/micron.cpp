# Freestanding dynamic hosts

`dynamic_open(path, flags, options)` accepts an optional `dynamic_options`:

- `resolve(context, name, version)` supplies host symbols by address before the
  loaded-library search. Return null to continue ordinary resolution.
- `validate(context, module)` runs after mapping and before relocating or running
  constructors. Return null to accept, or a persistent error string to reject.
- `context` and both callbacks must remain valid for the dependency closure's lifetime.

The options propagate to dependencies. Reopening an existing module with different
callbacks or context fails; it does not change the ABI of an already-relocated
module. Callbacks execute under the loader lock and must not re-enter the loader.

`dynamic_finalize(handle)` runs the closure's finalizers once, without unmapping
it. Use it only after all users of that closure have finished, normally during host
shutdown. It also finalizes dependencies, which may be shared with other handles.
A subsequent close does not run the finalizers again. `rtld::nodelete` retains the
mapping through host callbacks and destruction.

Module-local general-dynamic TLS works without a system loader. Its per-thread
mappings are reclaimed after C++ TLS destructors on Micron worker exit, and after
process finalizers under Micron startup. A hosted main thread retains its mappings
until the kernel reclaims them at process exit. Initial-exec TLS, TLSDESC, and
cross-module TLS references are rejected. On x86-64, build compatible libraries
with `-fPIC -mtls-dialect=gnu` and no hosted-runtime dependencies.

Freestanding exceptions now keep their bookkeeping in TLS by default. Targets
without TLS retain the explicit `__micron_eh_single_threaded` fallback. Emergency
exception slots use atomic ownership, including when multiple threads exhaust
address-space allocation simultaneously.

Run `sh tests/support/dl/build.sh x64`, then the `dl_freestanding_host.cpp` test
with duck `-ke -s -i . --start ./start`. It covers host resolution, validation,
module TLS isolation/reclamation, finalization, and concurrent normal/emergency
exceptions. `tests/rigor/gcc_int_syms.cpp` pins the signed 128-bit helpers and
full-width unsigned quotient/remainder boundary cases.
