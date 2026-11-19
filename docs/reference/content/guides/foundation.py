"""Fundamental types and their target-dependent representations."""

GUIDES = {
    "foundation": {
        "summary": "Fundamental integer, character, floating-point, and Linux ABI types; byte order, fixed bit storage, and the foundation header.",
        "sections": [
            {
                "title": "Type names and representation",
                "paragraphs": [
                    "The short arithmetic aliases in `types.hpp` are declared in the global namespace. They name existing arithmetic types, except for the emulated 128-bit integers on 32-bit targets. `micron::max_align_t` and the Linux aliases in `micron::posix` are declared separately.",
                    "The supported data models use 8-bit bytes, 32-bit `int`, and 64-bit `long long`. amd64 and aarch64 use 64-bit pointers and `long`; i386, armv7-a, and wasm32 use 32-bit pointers and `long`. The compile-time checks in `bits/__ctasserts.hpp` enforce these assumptions.",
                ],
                "table": {
                    "columns": ["Requirement", "Types", "Use"],
                    "rows": [
                        ["Exact integer width", "`i8`–`i64`, `u8`–`u64`", "Binary fields and arithmetic with a specified width. Byte order remains a separate property."],
                        ["Object sizes and pointer differences", "`usize`, `size_t`, `ssize_t`, `ptrdiff_t`", "Container lengths, indices, and differences. On 32-bit targets these have a smaller range than `max_t`."],
                        ["Signed byte count or error", "`max_t`", "Interfaces that return a nonnegative count or a negative error value."],
                        ["Wide integer arithmetic", "`i128`, `u128`", "Fixed 128-bit arithmetic; representation and division cost depend on the target."],
                        ["Floating-point arithmetic", "`f16`, `f32`, `f64`, `f128`, `flong`, `ff`, `df`", "Select using the representation table below; the numeric suffix alone does not specify the precision on every compiler."],
                        ["Linux ABI fields", "`micron::posix::*`, `kernel_*`", "Use the type required by the particular ABI structure or system call."],
                    ],
                },
            },
            {
                "title": "Headers and constants",
                "paragraphs": [
                    "`std.hpp` includes common type, allocation, matching, range, control, error, and version facilities. Linux builds also include the standard descriptor and signal definitions and system-call wrapper. It defines the namespace alias `mc = micron`; it does not include every container or algorithm.",
                    "`endian.hpp` defines the global constants `little_endian` (1234), `big_endian` (4321), `pdp_endian` (3412), `byte_order`, and `float_word_order`. `is_big_endian` and `is_little_endian` test the selected order. `high_half` and `low_half` are 0/1 indices for the corresponding order. Including `types.hpp` also applies the little-endian target check.",
                    "`byte_width` is 8. The helpers `B`, `KB`, `MB`, and `GB` in `std.hpp` return `byte`; the shifted result is converted to an unsigned byte. They are not general-purpose size calculations for values larger than 255.",
                    "`attributes.hpp` supplies compiler attribute macros such as `restrict`, `chot`, `ccold`, `cinline`, `cpure`, and `cconst`. The constructor/destructor attribute macros expand to nothing in a freestanding build. `bytewise.hpp` forwards to `bits.hpp`.",
                ],
            },
        ],
        "types": [
            {
                "name": "sig_t", "header": "std.hpp",
                "summary": "Global alias of `void (*)(int)`, a pointer to a function taking one int and returning void.",
                "sections": [{"title": "Use", "paragraphs": ["The alias describes a function-pointer signature. It does not install a signal handler, own a callable object, or extend the lifetime of code in a loaded module."]}],
            },
            {
                "name": "usize", "header": "types.hpp",
                "covers": ["size_t", "ssize_t", "ptrdiff_t", "intptr_t", "uintptr_t", "ptr_t", "addr_t", "p64"],
                "summary": "Unsigned type of `sizeof`, used for object sizes and container indices. The aliases in this group follow the target pointer width.",
                "sections": [{
                    "title": "Related types",
                    "table": {"columns": ["Type", "Definition and use"], "rows": [
                        ["`size_t`, `usize`", "`decltype(sizeof(0))`; unsigned, 32 or 64 bits according to the target."],
                        ["`ptrdiff_t`", "Compiler pointer-subtraction type, `__PTRDIFF_TYPE__`; signed."],
                        ["`intptr_t`, `ssize_t`", "`__INTPTR_TYPE__`; signed pointer-width integer, checked to be the same type as `ptrdiff_t` on supported targets."],
                        ["`uintptr_t`", "`decltype(sizeof(0))`, rather than the potentially redefined `__UINTPTR_TYPE__`."],
                        ["`ptr_t`, `addr_t`", "Aliases of `uintptr_t` for numeric pointer and address values; neither owns storage."],
                        ["`p64`", "Alias of `uintptr_t`, including on 32-bit targets; the name does not guarantee 64 bits."],
                    ]}}, {
                    "title": "Notes",
                    "paragraphs": ["Pointer-width aliases are suitable for native addresses and lengths. Use an explicitly sized integer for a stored field whose width must remain unchanged across targets."],
                }],
            },
            {
                "name": "max_t", "header": "types.hpp",
                "covers": ["intmax_t", "uintmax_t", "umax_t", "word"],
                "summary": "Signed maximum-width integer alias `__INTMAX_TYPE__`. It is 64 bits on the supported targets, including those with emulated 128-bit integers.",
                "sections": [{"title": "Related types", "table": {"columns": ["Type", "Definition and use"], "rows": [
                    ["`intmax_t`, `max_t`", "Signed `__INTMAX_TYPE__`. A negative data-path result commonly represents `-errno`; inspect it before converting to an unsigned size."],
                    ["`uintmax_t`, `umax_t`", "Unsigned `__UINTMAX_TYPE__`, checked to have the same size as `uint64_t`."],
                    ["`word`", "`umax_t` with the compiler `may_alias` attribute. Used for alias-permitting memory accesses; it is not a pointer-width alias."],
                ]}}],
            },
            {
                "name": "i8", "header": "types.hpp",
                "covers": ["i16", "i32", "i64", "u8", "u16", "u32", "u64", "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "int_least8_t", "int_least16_t", "int_least32_t", "int_least64_t", "uint_least8_t", "uint_least16_t", "uint_least32_t", "uint_least64_t"],
                "summary": "Signed 8-bit integer alias. The fixed-width and least-width families provide the following integer representations.",
                "sections": [{"title": "Fixed-width integer types", "table": {"columns": ["Width", "Signed aliases", "Unsigned aliases"], "rows": [
                    ["8 bits", "`int8_t`, `i8`, `int_least8_t` (`signed char`)", "`uint8_t`, `u8`, `uint_least8_t` (`unsigned char`)"],
                    ["16 bits", "`int16_t`, `i16`, `int_least16_t`", "`uint16_t`, `u16`, `uint_least16_t`"],
                    ["32 bits", "`int32_t`, `i32`, `int_least32_t` (`int`)", "`uint32_t`, `u32`, `uint_least32_t` (`unsigned int`)"],
                    ["64 bits", "`int64_t`, `i64`, `int_least64_t`", "`uint64_t`, `u64`, `uint_least64_t`"],
                ]}, "paragraphs": ["The fixed-width aliases use the compiler's `__INTn_TYPE__` and `__UINTn_TYPE__` definitions. Each least-width alias is the corresponding fixed-width type, without a separate target-specific selection."]}],
            },
            {
                "name": "p8", "header": "types.hpp",
                "covers": ["p16", "p32", "fint8_t", "fint16_t", "fint32_t", "fint64_t"],
                "summary": "Alias of the compiler's unsigned fast 8-bit type. Fast-width aliases can occupy more bits than their suffix indicates.",
                "sections": [{"title": "Fast-width integer types", "table": {"columns": ["Aliases", "Definition"], "rows": [
                    ["`fint8_t`, `p8`", "`__UINT_FAST8_TYPE__`"],
                    ["`fint16_t`, `p16`", "`__UINT_FAST16_TYPE__`"],
                    ["`fint32_t`, `p32`", "`__UINT_FAST32_TYPE__`"],
                    ["`fint64_t`", "`__UINT_FAST64_TYPE__`; `p64` instead names `uintptr_t`."],
                ]}, "paragraphs": ["Every `fint*` alias in this header is unsigned. Use the exact-width family when the size of an array element or external representation matters."]}],
            },
            {
                "name": "byte", "header": "types.hpp", "covers": ["pnt_t"],
                "summary": "Alias of `uint8_t`, an unsigned character type with 8 bits. `pnt_t` is an additional alias of `byte`.",
                "sections": [{"title": "Notes", "paragraphs": ["`byte` supports ordinary integer arithmetic and conversions. It is used for raw buffers and byte counts; it is not a scoped enumeration. A `pnt_t` value is a byte, despite the pointer-like spelling."]}],
            },
            {
                "name": "c8", "header": "types.hpp", "covers": ["c16", "c32", "nullptr_t"],
                "summary": "Alias of `char8_t`. The character aliases name C++ code-unit types rather than string containers.",
                "sections": [{"title": "Related types", "table": {"columns": ["Alias", "Definition"], "rows": [
                    ["`c8`", "`char8_t`, one byte."], ["`c16`", "`char16_t`, two bytes."], ["`c32`", "`char32_t`, four bytes."],
                    ["`nullptr_t`", "`decltype(nullptr)`, the type of the null pointer literal; independent of the character aliases."],
                ]}}],
            },
            {
                "name": "f16", "header": "types.hpp", "covers": ["f32", "f64", "f128", "flong", "ff", "df"],
                "summary": "Compiler-dependent floating-point alias. The `f16` and `f128` names do not specify the same precision on every supported compiler and target.",
                "sections": [{"title": "Representation", "table": {"columns": ["Compiler and target", "`f16`", "`f32`", "`f64`", "`f128`"], "rows": [
                    ["GCC, C++23 or later, amd64", "`_Float16`", "`_Float32`", "`_Float64`", "`_Float128`"],
                    ["GCC, C++23 or later, aarch64", "`float`", "`float`", "`double`", "`long double`"],
                    ["GCC, C++23 or later, other targets", "`float`", "`float`", "`double`", "`double`"],
                    ["Clang", "`float`", "`float`", "`double`", "`long double`"],
                ]}, "paragraphs": ["`ff` always names `float`; `df` always names `double`; `flong` always names `long double`. The representation of `long double` follows the target ABI: it need not be the same as GCC amd64 `_Float128`. Serializing a native floating object therefore requires an explicit format choice."]}],
            },
            {
                "name": "uint128_t", "header": "bits/__int128.hpp", "covers": ["u128"],
                "summary": "Unsigned fixed 128-bit integer. `u128` is its short alias, declared by `types.hpp`.",
                "sections": [{"title": "Representation and operations", "paragraphs": [
                    "On 64-bit targets, `uint128_t` aliases `unsigned __int128`. On 32-bit targets it is a class containing public `u64 lo` and `u64 hi` members. Its two-word constructor takes the high word first and low word second. Include `types.hpp` to provide its prerequisite integer aliases.",
                    "The emulated class provides comparisons, explicit conversions to 32-bit and 64-bit integers and `bool`, bitwise operations, shifts, addition, subtraction, multiplication, division, remainder, and increment/decrement. Arithmetic retains the low 128 bits. Narrowing conversions retain the low word.",
                ]}, {"title": "Complexity and notes", "paragraphs": [
                    "Emulated addition, bitwise operations, and shifts use a fixed number of word operations. Multiplication splits 64-bit operands into 32-bit products. Division and remainder use shifted subtraction with up to 128 iterations; they allocate no storage. This fixed-width type does not grow like an arbitrary-precision integer.",
                    "In the emulated implementation, unsigned division by zero returns an all-one quotient and zero remainder; a shift of 128 or more returns zero. Native `__int128` follows the compiler's built-in arithmetic rules. Do not depend on the emulated edge cases for code intended to run on both representations. Negative shift counts are unchecked.",
                ]}],
            },
            {
                "name": "int128_t", "header": "bits/__int128.hpp", "covers": ["i128"],
                "summary": "Signed fixed 128-bit integer. `i128` is its short alias, declared by `types.hpp`.",
                "sections": [{"title": "Representation and operations", "paragraphs": [
                    "On 64-bit targets this is native `__int128`. The 32-bit class stores a public `uint128_t v` with a two's-complement bit pattern. Signed input constructors sign-extend, comparisons examine the sign bit, and right shift propagates it. Explicit integer conversions retain the low word.",
                    "Emulated signed division divides unsigned magnitudes and applies the quotient sign; remainder follows the numerator's sign. Unary negation and signed arithmetic operate on the underlying unsigned bit pattern. Their overflow behavior is therefore not a portable replacement for the rules of native signed `__int128`.",
                ]}, {"title": "Complexity", "paragraphs": ["Operations have the fixed-word costs of `uint128_t`; signed division and remainder add magnitude and sign handling. No operation allocates memory."]}],
            },
            {
                "name": "micron::max_align_t", "header": "types.hpp",
                "summary": "Union whose alignment accommodates its members: the ordinary integer and floating-point types and `void*`.",
                "sections": [{"title": "Notes", "paragraphs": ["The union includes `char`, `short`, `int`, `long`, `long long`, `float`, `double`, `long double`, and `void*`. Its alignment does not establish support for a user-defined over-aligned type. It carries no allocator or storage-ownership behavior."]}],
            },
            {
                "name": "kernel_long_t", "header": "types.hpp",
                "covers": ["kernel_ulong_t", "kernel_ino_t", "kernel_mode_t", "kernel_pid_t", "kernel_ipc_pid_t", "kernel_uid32_t", "kernel_gid32_t", "kernel_uid_t", "kernel_gid_t", "kernel_old_uid_t", "kernel_old_gid_t", "kernel_suseconds_t", "kernel_daddr_t", "kernel_old_dev_t", "kernel_size_t", "kernel_ssize_t", "kernel_ptrdiff_t", "kernel_off_t", "kernel_loff_t", "kernel_old_time_t", "kernel_time_t", "kernel_time64_t", "kernel_clock_t", "kernel_timer_t", "kernel_clockid_t", "kernel_caddr_t", "kernel_uid16_t", "kernel_gid16_t", "time64_t", "slong_t", "ulong_t", "quad_t", "uquad_t"],
                "summary": "Alias of `long`, used by the global `kernel_*` ABI alias family. These names must be distinguished from the time64 and large-file aliases in `micron::posix`.",
                "sections": [{"title": "ABI aliases", "table": {"columns": ["Aliases", "Definition"], "rows": [
                    ["`kernel_long_t`, `kernel_ulong_t`", "`long`, `unsigned long`; native word width."],
                    ["`kernel_ino_t`, `kernel_size_t`", "`kernel_ulong_t`."],
                    ["`kernel_ssize_t`, `kernel_ptrdiff_t`, `kernel_suseconds_t`", "`kernel_long_t`."],
                    ["`kernel_off_t`, `kernel_old_time_t`, `kernel_time_t`, `kernel_clock_t`", "Signed native-word ABI fields; 32 bits on i386/armv7-a/wasm32."],
                    ["`kernel_loff_t`, `kernel_time64_t`", "Signed 64-bit offset and time aliases from `bits-types.hpp`."],
                    ["`kernel_pid_t`, `kernel_ipc_pid_t`, `kernel_daddr_t`, `kernel_timer_t`, `kernel_clockid_t`", "`int`."],
                    ["`kernel_mode_t`, `kernel_old_dev_t`", "`unsigned int`."],
                    ["`kernel_uid32_t`, `kernel_gid32_t`, `kernel_uid_t`, `kernel_gid_t`, `kernel_old_uid_t`, `kernel_old_gid_t`", "`unsigned int`."],
                    ["`kernel_uid16_t`, `kernel_gid16_t`", "Aliases through `__uid_t_type`/`__gid_t_type`, both currently `unsigned int` (32 bits), despite the suffix."],
                    ["`kernel_caddr_t`", "`char*`."],
                    ["`time64_t`, `quad_t`, `uquad_t`", "`int64_t`, `int64_t`, and `uint64_t`, respectively."],
                    ["`slong_t`, `ulong_t`", "Signed and unsigned system-call word aliases from `bits-types.hpp`."],
                ]}}],
            },
            {
                "name": "micron::posix::time_t", "header": "bits/__posix_types.hpp",
                "covers": ["micron::posix::idtype_t", "micron::posix::id_t", "micron::posix::pid_t", "micron::posix::key_t", "micron::posix::uid_t", "micron::posix::gid_t", "micron::posix::suseconds_t", "micron::posix::suseconds64_t", "micron::posix::rlim_t", "micron::posix::mode_t", "micron::posix::off_t", "micron::posix::off64_t", "micron::posix::syscall_long_t", "micron::posix::syscall_ulong_t", "micron::posix::blksize_t", "micron::posix::blkcnt_t", "micron::posix::nlink_t", "micron::posix::clock_t", "micron::posix::clockid_t", "micron::posix::timer_t", "micron::posix::dev_t", "micron::posix::ino_t", "micron::posix::ino64_t", "micron::posix::daddr_t"],
                "summary": "Signed 64-bit time alias. The namespace contains Linux-facing time64, large-file, identifier, and system-call types.",
                "sections": [{"title": "Related types", "table": {"columns": ["Aliases in `micron::posix`", "Representation and purpose"], "rows": [
                    ["`time_t`, `suseconds_t`, `suseconds64_t`", "Signed 64-bit time and subsecond fields on all supported targets."],
                    ["`off_t`, `off64_t`, `blkcnt_t`", "Signed 64-bit offsets and block counts."],
                    ["`ino_t`, `ino64_t`, `dev_t`, `rlim_t`", "Unsigned 64-bit inode, device, and resource-limit values."],
                    ["`idtype_t`, `id_t`, `uid_t`, `gid_t`, `mode_t`", "Unsigned 32-bit identifier, user/group, and mode values."],
                    ["`pid_t`, `key_t`, `clockid_t`, `daddr_t`", "Signed 32-bit process, IPC, clock, and address fields."],
                    ["`syscall_long_t`, `syscall_ulong_t`", "Signed and unsigned system-call words."],
                    ["`clock_t`, `blksize_t`", "Signed system-call word types, including on time64 targets."],
                    ["`nlink_t`", "Unsigned system-call word on amd64; unsigned 32-bit on the other targets."],
                    ["`timer_t`", "`void*`, rather than the `int` used by `kernel_timer_t`."],
                ]}, "paragraphs": ["The header also re-exports most of these names into `micron`. `idtype_t`, `syscall_long_t`, `syscall_ulong_t`, and `daddr_t` remain in `micron::posix`. These aliases do not determine the layout of a complete ABI structure; use the structure definition supplied for the operation."]}],
            },
            {
                "name": "micron::bitfield", "header": "bitfield.hpp",
                "summary": "Fixed bit storage aligned to 64 bytes. `bitfield<N, R>` stores `N/8` bytes and returns bit masks converted to `R`.",
                "sections": [{"title": "Template parameters and member types", "table": {"columns": ["Parameter or member", "Meaning"], "rows": [
                    ["`N`", "Number of bits; constrained to a multiple of 8. Use a positive size."],
                    ["`R`", "Fundamental return type for element access; defaults to `bool`. For an integer `R`, a set bit returns its byte-local mask, not necessarily 1."],
                    ["`category_type`, `mutability_type`, `memory_type`", "`bitfield_tag`, `mutable_tag`, and `stack_tag`; storage is a member array."],
                ]}}, {"title": "Operations and complexity", "paragraphs": [
                    "The Boolean constructor fills the complete storage with zero or one bits. Copy transfers every byte; move also clears the source. `operator[]` reads an unchecked bit; `at` diagnoses an index of `N` or greater. Indexed `set`, `clear`, and `flip` modify one bit in constant time.",
                    "`size()` returns the number of bytes, `N/8`. `all`, `any`, `none`, compound bitwise operations, and whole-field initialization scan up to `N/8` bytes. No operation allocates storage, and moving transfers values rather than the address of the member array.",
                ]}, {"title": "Notes", "paragraphs": [
                    "Whole-field `set()` and `flip()` process four bytes per iteration without a remainder branch. Their current implementation requires `N` to be a multiple of 32 to avoid writing beyond the byte array, although the template constraint permits other multiples of 8.",
                    "The text constructors do not initialize the byte array and examine only `text_length/8` characters; they do not establish a fully initialized parsed bit sequence. `swap` contains array assignment expressions that are not valid for the member array. Use the Boolean constructor and indexed operations for initialized bit storage.",
                ]}],
            },
            {
                "name": "micron::kernel::probe_gate", "header": "kernel.hpp",
                "summary": "Version gate with an atomic demotion flag for a Linux facility. It carries no system-call implementation of its own.",
                "sections": [{"title": "Operations", "paragraphs": [
                    "`open(feature_ver)` tests `kernel::since(feature_ver)` and the relaxed atomic demotion flag. `demote()` closes the gate; `restore()` clears the flag. The caller must demote after an `ENOSYS` result when implementing a fallback. Permission failures do not establish that a system call is absent.",
                    "`kernel::ver` packs saturated 8-bit major, minor, and patch components. `kernel::version()` obtains and caches a `uname` release, clamped to the configured floor (5.0 by default). `since` returns true without querying the kernel when the requested version is no higher than the floor. Feature constants are version thresholds, not runtime probes of permissions or kernel configuration.",
                ]}, {"title": "Complexity", "paragraphs": ["A cached query uses fixed-cost comparisons and relaxed atomic accesses. The first query above the floor may invoke `uname`; concurrent first callers may each perform detection. This facility is unavailable on wasm32."]}],
            },
        ],
        "sources": ["types.hpp", "bits-types.hpp", "std.hpp", "defs.hpp", "attributes.hpp", "endian.hpp", "bytewise.hpp", "bitfield.hpp", "kernel.hpp", "bits/__int128.hpp", "bits/__posix_types.hpp", "bits/__ctasserts.hpp", "bits/__arch.hpp", "bits/__pause.hpp", "bits/__compilers.hpp"],
    },
}
