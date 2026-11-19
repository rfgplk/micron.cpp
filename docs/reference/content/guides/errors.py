"""Error-value, exception, allocation-failure, and termination contracts."""

_exceptions = [
    ("domain_error", "domain", "domain_errno", "Domain errors reported by an operation."),
    ("invalid_argument", "invalid", "invalid_errno", "Invalid operation arguments."),
    ("length_error", "length", "length_errno", "Invalid or unsupported lengths."),
    ("logic_error", "logic", "logic_errno", "Logical precondition failures."),
    ("out_of_range_error", "oor", "oor_errno", "Out-of-range access or index."),
    ("overflow_error", "overflow", "overflow_errno", "Overflow reported by an operation."),
    ("range_error", "range_error", "range_errno", "Range errors reported by an operation."),
    ("runtime_error", "runtime", "runtime_errno", "Runtime failures without a more specific category."),
    ("underflow_error", "underflow", "underflow_errno", "Underflow reported by an operation."),
    ("future_error", "future_err", "future_errno", "Future-related failures."),
    ("system_error", "system", "system_errno", "System-operation failures."),
    ("io_error", "io_err", "io_errno", "Input/output failures."),
    ("format_error", "format_err", "format_errno", "Formatting failures."),
    ("standard_error", "standard", "standard_errno", "General library standard-interface failures."),
    ("library_error", "library", "library_errno", "Library-specific precondition or capacity failures."),
    ("hardware_error", "hardware", "hardware_errno", "Hardware-related failures reported by an operation."),
    ("memory_error", "memory_err", "memory_errno", "Allocation and memory-operation failures."),
    ("thread_error", "thread_err", "thread_errno", "Thread-operation failures."),
    ("filesystem_error", "fs_error", "filesystem_errno", "Filesystem failures."),
    ("network_error", "net_error", "network_errno", "Network-operation failures."),
    ("critical_error", "critical_err", "critical_errno", "Critical library failures."),
]

_memory_variants = [
    ("memory_error_new_scalar", "Failure of scalar `operator new`."),
    ("memory_error_new_array", "Failure of array `operator new[]`."),
    ("memory_error_new_scalar_aligned", "Failure of aligned scalar allocation."),
    ("memory_error_new_array_aligned", "Failure of aligned array allocation."),
    ("memory_error_abc_mark", "Allocator block-marking failure."),
    ("memory_error_abc_unmark_untracked", "Unmarking an untracked allocator region."),
    ("memory_error_abc_unmark_failed", "Allocator unmark operation failure."),
    ("memory_error_abc_fetch_oom", "Allocator fetch exhaustion."),
    ("memory_error_abc_retire", "Allocator retirement failure."),
    ("memory_error_abc_salloc_oom", "Small allocator allocation exhaustion."),
    ("memory_error_abc_dealloc_unknown", "Deallocation of an unrecognized allocation."),
    ("memory_error_abc_dealloc_zero", "Allocator zero-size deallocation diagnostic."),
    ("memory_error_abc_dealloc_size", "Allocator deallocation-size diagnostic."),
    ("memory_error_abc_freeze", "Allocator freeze operation failure."),
    ("memory_error_abc_launder", "Allocator laundering operation failure."),
    ("memory_error_abc_realloc_unknown", "Reallocation of an unrecognized allocation."),
    ("memory_error_abc_aligned_free_bad", "Invalid aligned-free diagnostic."),
    ("memory_error_core_unaligned", "Core allocator alignment diagnostic."),
]

GUIDES = {"errors": {
    "summary": "Signed error results, thread-local error state, exception categories, allocation failures, and thread/process termination. The selected runtime mode determines whether exception reporting throws or terminates.",
    "sections": [{"title": "Error reporting", "paragraphs": [
        "A signed data-path result commonly uses nonnegative values for a count or success and negative values for an error number. `errno` is a separate thread-local state. A raw error result does not by itself establish that errno has been updated; use the particular wrapper's contract and inspect the signed result before converting it to a size.",
        "`set_errno` assigns the error slot from an i32, a convertible arithmetic value, or a constant template argument. `error::get_errno(e)` maps a positive error number to a static message; `const_get_errno<e>()` supplies the compile-time selection, and `what_errno()` reads the current errno. Unknown values, including unconverted negative results, select the fallback message. The returned strings are static and require no release.",
        "`micron::error` contains positive Linux error constants, the corresponding message pointers, and compatibility E* macros. `try_again` and `would_block` share 11; `device_busy` and `busy` share 16. Error constants are separate from exception category codes.",
    ]}, {"title": "Exception reporting", "paragraphs": [
        "`exc<E>(args...)` and `exc_silent<E>(args...)` construct E. Hosted builds and builds with the micron exception trampoline throw it. Freestanding builds without that trampoline instead terminate using `which()`. The ordinary terminating path writes `what()` and errno diagnostics; the silent path omits them. In a throwing build, both report through an exception.",
        "`exc_e<E>(negative_error,args...)` throws E in a throwing build; in the terminating mode it prints the negated supplied error as an unsigned number. Exception objects store the supplied message pointer without copying its text. A caught object does not extend the lifetime of a transient message buffer.",
        "The exception classes share a virtual base for `what()`, but each ordinary category is its own derived type. The supplied base's sized delete is a no-op because exception storage is managed through the C++ exception ABI. These types are diagnostic/control-flow objects rather than data-path result wrappers.",
    ]}, {"title": "Allocation failure", "paragraphs": [
        "`new.hpp` includes the memory action and allocation-operator definitions. The supplied scalar, array, and aligned replaceable new/delete families route to the selected allocator. Zero-size allocation requests are changed to a nonzero request. An allocation returning null reports the corresponding memory-error variant. Aligned requests round storage to the requested alignment.",
        "Delete routes to the matching ordinary or aligned free operation; the sized delete forms ignore the supplied size. Sanitizer-owned heaps suppress these replacements. Constructor/destructor work and allocator cost are separate from the pointer-level operation. Ordinary C++ new/delete lifetime rules still apply.",
    ]}, {"title": "Termination functions", "table": {"columns": ["Function", "Effect on Linux"], "rows": [
        ["`sys_exit`, `_Exit`, `quick_exit`", "Terminate the calling thread through SYS_exit, without the micron exit-callback sequence. No separate quick-exit callback registry is implemented here."],
        ["`sys_group_exit`, `proc_exit`", "Terminate the thread group through SYS_exit_group, with the attach fatal hook when configured."],
        ["`exit`", "Run the micron soft-exit sequence once, then terminate the calling thread."],
        ["`group_exit`", "Run the soft-exit sequence once, then terminate the thread group."],
        ["`abort()`", "Drain available I/O buffers and terminate the thread group with status 6; it does not raise a SIGABRT signal."],
        ["`abort(status)`", "The same direct termination path with the supplied status."],
        ["`atexit(fn)`", "Register a non-null callback; return 0 on success and -1 on null input or capacity exhaustion."],
    ]}, "paragraphs": [
        "The soft-exit sequence drains thread-local destructors, registered callbacks in reverse registration order, the fini array, and further callbacks registered during those phases. An atomic flag selects the first exit sequence; a subsequent exit call goes directly to its termination operation. No return from main or thread-joining behavior is implied by these functions themselves.",
        "The callback table has `MICRON_ATEXIT_CAP` slots, default 4096, with a minimum of 8. Reservations use a monotonic cursor and are not reused. Callbacks are claimed before invocation; new registrations are considered before remaining older callbacks. Execution can take arbitrary time because it calls user code. Unpublished reservations are given a bounded number of drain rounds before being abandoned. A callback must not throw through the noexcept drain routines.",
        "Attach builds and wasm builds select different host/runtime paths. An attached guest avoids directly terminating its host thread group through the configured fatal hook. `stdthread.hpp` and `stdcoro.hpp` are threading/coroutine umbrella headers and configure the multithreaded allocator before those facilities are included; they are not exception types.",
    ]}],
    "types": [{
        "name": "micron::except::runtime_error", "header": "except.hpp",
        "covers": ["micron::except::" + name for name, _, _, _ in _exceptions if name != "runtime_error"] + ["micron::" + alias for _, alias, _, _ in _exceptions],
        "summary": "Exception category storing a borrowed message pointer. The other ordinary exception categories have the same message and category-code interface.",
        "sections": [{"title": "Construction and members", "paragraphs": ["Explicit construction takes `const char*`. `what()` returns that exact pointer and is noexcept; `which()` returns the category's u32 code. Copying the exception copies the pointer, without allocating or copying message text. The categories do not validate a domain-specific property when constructed; the reporting operation chooses the category." ]}, {"title": "Exception categories", "table": {"columns": ["Type in `micron::except`", "Alias in `micron`", "Code constant", "Category"], "rows": [["`" + name + "`", "`" + alias + "`", "`" + code + "`", meaning] for name, alias, code, meaning in _exceptions]}, "paragraphs": ["Codes from domain through network occupy 0x7f01 through 0x7f14 in the declared sequence; critical uses 0x7fff. A process exit status retains only the low status bits supplied by the environment, so the full category code is not an independently preserved OS exit-status value."]}],
    }, {
        "name": "micron::except::memory_error_new_scalar", "header": "except.hpp",
        "covers": ["micron::except::" + name for name, _ in _memory_variants if name != "memory_error_new_scalar"],
        "summary": "Distinct memory-error subtype for scalar allocation failure. The allocator variants inherit the message and code behavior of `memory_error`.",
        "sections": [{"title": "Memory-error variants", "table": {"columns": ["Derived type", "Diagnostic category"], "rows": [["`" + name + "`", description] for name, description in _memory_variants]}, "paragraphs": ["Each variant's constructor forwards the message to `memory_error`. All return `memory_errno` from `which()`; their distinct C++ types allow more specific catches. Availability of a type does not imply that every corresponding allocator path throws in every configuration."]}],
    }],
    "sources": ["errno.hpp", "except.hpp", "exit.hpp", "new.hpp", "stdcoro.hpp", "stdthread.hpp", "defs.hpp", "memory/new.hpp", "bits/__exceptions.hpp", "bits/__attach_hook.hpp", "bits/__thread_exit_hook.hpp", "bits/__profile.hpp"],
}}
