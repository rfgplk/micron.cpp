"""Compile-time configuration and version vocabulary."""

GUIDES = {"configuration": {
    "summary": "Compile-time build predicates, allocator/exception defaults, feature gates, and encoded library version information. Configuration must be consistent before dependent headers are included.",
    "sections": [{"title": "Configuration values", "table": {"columns": ["Value in `micron::config`", "Definition"], "rows": [
        ["`freestanding`", "True when the architecture/compiler configuration derived `__micron_freestanding` from a non-hosted compilation."],
        ["`concurrency_at_startup`", "The negation of freestanding; a configuration value, not itself a thread-creation call."],
        ["`fast_math_x86`", "True only when fast math, an x86-family target, and SSE are all enabled."],
    ]}, "paragraphs": ["`bits/__arch.hpp` derives architecture, pointer width, endian, compiler-language, and ISA feature predicates from compiler definitions. It does not detect a runtime CPU. Code selected by an enabled instruction-set feature is intended for a compatible target. The reflection gate additionally requires the compiler reflection feature macro."]}, {"title": "Defaults and inclusion order", "paragraphs": [
        "`defs.hpp` selects the wasm allocator on wasm32; otherwise it enables the abcmalloc standard allocation backend unless `MICRON_ABCMALLOC_DISABLE_STD` or an existing selection says otherwise. A sanitizer that owns the heap disables the abcmalloc C override. The internal standard-allocation predicate requires the matching allocator configuration so allocation and deallocation use the same family.",
        "The hosted default defines `TWORKERS` and the corresponding startup-concurrency marker. `except::__use_exceptions` is false only for a freestanding compilation without the micron exception trampoline; other modes select throwing exception reporting. This predicate does not add compiler or linker flags on its own.",
        "`MICRON_PROFILE_TINY` supplies defaults guarded by `#ifndef`: no eager allocator hot tiers, a 64-page arena buffer request, a cache-size factor of 512, a smaller virtual reservation and arena count, `MICRON_ATEXIT_CAP = 64`, and no memory-tuning probe. The virtual reservation default is 4 GiB for a 64-bit pointer target and 1 GiB for a 32-bit pointer target. These are requested policy values; actual allocator granules and grants remain allocator properties.",
        "A macro read by an already included header cannot retroactively change that header's definition. Threading and coroutine umbrellas arrange multithreaded allocator configuration before allocator inclusion. Apply feature selections consistently across translation units that share objects or runtime state.",
    ]}, {"title": "Version information", "paragraphs": [
        "`version.hpp` defines `MICRON_ABI` and four integer version components. The current values are ABI 9, major 0x1000, minor 0x0a00, patch 0x0020, and hotfix 0. They are encoded components, not decimal semantic-version numbers.",
        "`is_version<major,minor,patch>()` tests exact component equality; `is_hotfix<value>()` tests the hotfix. `get_version()` combines all four components with bitwise OR. These functions are constexpr and perform constant work. Their output does not probe compatibility of a particular application or loaded object.",
    ]}],
    "types": [{"name": "kernel_support", "header": "version.hpp", "summary": "Global integer enumeration naming kernel targets in version metadata.", "sections": [{"title": "Enumerators", "paragraphs": ["The enumerators are `kernel_linux = 0`, `kernel_darwin = 1`, `kernel_obsd`, `kernel_netbsd`, `kernel_freebsd`, and `kernel_none`. `MICRON_KERNEL_TYPE` currently selects `kernel_linux`. Enumerating a target does not provide that target's operating-system implementation."]}]}],
    "sources": ["config.hpp", "version.hpp", "defs.hpp", "bits/__profile.hpp", "bits/__arch.hpp", "bits/__compilers.hpp"],
}}
