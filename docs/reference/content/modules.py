"""Handwritten reference content. Names and behavior are checked against src/."""

GROUPS = [
    ("foundation", "Language support"), ("containers", "Containers and strings"),
    ("algorithms", "Algorithms"), ("memory", "Memory management"),
    ("concurrency", "Concurrency"), ("io", "Input and output"),
    ("numeric", "Numerics and mathematics"), ("system", "System interfaces"),
    ("graphics", "Graphics"), ("simd", "SIMD"), ("internal", "Implementation notes"),
]

# id, title, group, header/path prefixes, introduction, (name, description), example
MODULES = [
("foundation", "Types and language support", "foundation", ["types.hpp", "bits-types.hpp", "defs.hpp", "attributes.hpp", "std.hpp", "endian.hpp", "bytewise.hpp", "bitfield.hpp", "kernel.hpp"],
 "Fundamental types, configuration definitions, byte order, and the foundation header. Include the specific facility needed by a translation unit.", [
 ("usize", "An unsigned integer type used for sizes and indices. Its width follows the target address width."),
 ("u8, u16, u32, u64", "Unsigned integer aliases with the indicated widths. Signed counterparts are named `i8`, `i16`, `i32`, and `i64`."),
 ("byte", "An unsigned byte type used by the memory and binary I/O interfaces."),
 ("<micron/std.hpp>", "Includes common types, traits, allocation, errors, and other foundation facilities. Individual modules may require additional headers.")], None),
("configuration", "Build configuration", "foundation", ["config.hpp", "version.hpp"],
 "Preprocessor definitions and build modes select library facilities at compile time. Define a configuration macro consistently before including the headers that use it.", [
 ("__micron_freestanding", "Selects freestanding runtime paths, including the freestanding thread backend where available."),
 ("MICRON_ABC_MT", "Enables the multithreaded abcmalloc paths. In a freestanding thread-using translation unit, define it before allocator configuration is included."),
 ("MICRON_NO_ZZZ_HASH", "Disables the zzz hash selection used by the default hashing configuration."),
 ("MICRON_MEM_NT_THRESHOLD", "Sets the default threshold, in bytes, used by the memory operation tuning code."),
 ("MICRON_ENABLE_ATTACH", "Enables attachment support in code that provides an attach-capable host."),
 ("__micron_reflection", "Is defined when the selected C++ language mode and compiler expose reflection support. It gates the reflection interfaces.")], None),
("traits", "Type traits and concepts", "foundation", ["type_traits.hpp", "concepts.hpp", "compare.hpp", "tags.hpp", "container_safety.hpp"],
 "Compile-time predicates, transformations, constraints, comparison utilities, and container tags.", [
 ("micron::is_same_v<T, U>", "Evaluates to true when `T` and `U` are the same type."),
 ("micron::remove_cvref_t<T>", "Names the type obtained by removing reference and top-level cv qualifiers from `T`."),
 ("micron::is_container<T>", "A constraint used by interfaces that require the container operations defined by the concept.")], None),
("ranges", "Ranges, iterators, and views", "foundation", ["range.hpp", "slice.hpp", "slice_forward.hpp", "span.hpp", "view.hpp", "iterator.hpp"],
 "Range and iterator utilities and sequence views. Ownership and storage are properties of the particular type; inspect the selected type before retaining a pointer into it.", [
 ("micron::span<T, N>", "Owns up to `N` elements in storage within the span object. Provides element access, iterators, and size operations."),
 ("micron::slice<T>", "A dynamically sized sequence with constructors and accessors for its underlying storage."),
 ("micron::range", "Provides range-related types and operations in `<micron/range.hpp>`.")], None),
("utility", "Functions, tuples, and sum types", "foundation", ["function.hpp", "tuple.hpp", "sum.hpp", "match.hpp", "closures.hpp", "control.hpp"],
 "Callable wrappers, tuples, sum types, matching, and control-flow utilities.", [
 ("micron::pair<A, B>", "Stores two values as members `a` and `b`."),
 ("micron::tuple", "Stores a heterogeneous sequence of values whose element types are template arguments."),
 ("micron::option<T, F>", "Stores one of two alternatives. `is_first()` identifies the first alternative, and `is_second()` identifies the second; `cast<U>()` accesses a selected alternative.")], None),
("errors", "Errors and exceptions", "foundation", ["errno.hpp", "except.hpp", "exit.hpp", "new.hpp", "stdcoro.hpp", "stdthread.hpp"],
 "Error values, exception types, allocation operators, and process termination support. Availability of exception handling depends on build configuration.", [
 ("micron::exc<E>(message)", "Reports a setup or configuration failure using exception type `E` and the supplied message, subject to the exception configuration."),
 ("micron::error", "Contains error constants used by interfaces that return negative error values.")], None),
("reflection", "Reflection", "foundation", ["meta.hpp", "reflect.hpp"],
 "Compile-time reflection helpers. These interfaces require the compiler and build options required by the C++ reflection implementation.", [
 ("micron::reflect::field_count<T>", "The number of reflected fields in `T`."),
 ("micron::reflect::for_each_field(object, function)", "Invokes `function` for each reflected field, supplying its name and value.")], None),
("arrays", "Arrays", "containers", ["array/", "array.hpp"],
 "Fixed-size and specialized array containers, including mutable, immutable, persistent, and multidimensional forms.", [
 ("micron::array<T, N>", "Stores `N` elements of `T` in the array object. `N` must be positive and the total storage must satisfy the class constraint."),
 ("array::array()", "Constructs the elements from `T{}`."),
 ("array::array(initializer_list)", "Constructs from the supplied elements and value-initializes the remainder. An initializer list larger than `N` reports a runtime error."),
 ("array::size(), array::data()", "Return the number of elements and a pointer to the first element, respectively."),
 ("array::begin(), array::end()", "Return iterators delimiting the elements.")], "array.cpp"),
("vectors", "Vectors", "containers", ["vector/", "vector.hpp", "circle_buffer.hpp"],
 "Dynamically sized and fixed-capacity sequence containers. The vector family includes mutable, immutable, persistent, concurrent, and segmented variants.", [
 ("micron::vector<T, Alloc, Sf>", "Owns a contiguous sequence of `T`. `Alloc` selects the allocator and `Sf` controls the container checks; checks are enabled by default."),
 ("vector::vector(n, value)", "Constructs `n` elements initialized from `value`. The one-argument size constructor value-initializes its elements."),
 ("vector::size(), vector::max_size()", "Return the number of constructed elements and the current allocated capacity, respectively."),
 ("vector::reserve(n)", "Ensures space for at least `n` elements. It does not change the number of elements. Reallocation can invalidate pointers and iterators."),
 ("vector::push_back(value), vector::emplace_back(args...)", "Append an element by copying or moving `value`, or by constructing it from `args`."),
 ("vector::insert(position, value), vector::erase(position)", "Insert or erase at an index or iterator accepted by the selected overload; subsequent elements may move."),
 ("vector::clear()", "Destroys the constructed elements and sets the size to zero."),
 ("vector::find(value)", "Returns a pointer to a matching element, or a null pointer when no element matches.")], "vector.cpp"),
("strings", "Strings", "containers", ["string/", "strings.hpp"],
 "Character sequences, string views, formatting, and numeric conversions.", [
 ("micron::string", "A mutable character string. Construct it from a character sequence and use `size()` and `c_str()` to inspect it."),
 ("micron::sstr<N>", "A character string with capacity selected by the template argument `N`."),
 ("micron::string_view<S>", "A view over a string of type `S`. The referenced string must remain valid while its elements are accessed."),
 ("string::find(character)", "Returns the position of a matching character, or `micron::npos` when no match is found."),
 ("micron::int_to_string<I>(value)", "Converts the integer `value` of type `I` to a string.")], "strings.cpp"),
("maps", "Maps", "containers", ["maps/", "maps.hpp", "map.hpp"],
 "Associative containers for keys and values, including Robin Hood, hopscotch, Swiss, persistent, and tree-backed forms.", [
 ("micron::robin_map<K, V>", "Stores keys of type `K` with mapped values of type `V` using open addressing."),
 ("robin_map::insert(key, value)", "Inserts a key and its associated value into the map."),
 ("robin_map::find(key)", "Returns a pointer to the mapped value, or a null pointer if the key is absent."),
 ("robin_map::contains(key), robin_map::erase(key)", "Test for a key and remove a key, respectively."),
 ("micron::stack_swiss_map<K, V, N>", "A fixed-capacity Swiss-style table with its storage in the object.")], "maps.cpp"),
("trees", "Trees", "containers", ["trees/", "trees.hpp"],
 "Tree containers include B-trees, red-black trees, and radix-based structures. The selected type determines key ownership and traversal order.", [
 ("micron::art", "An adaptive radix tree with insertion, lookup, update, and erasure operations."),
 ("art::find(key)", "Looks up `key` in the tree."),
 ("art::insert(key, value)", "Adds a key and associated value to the tree.")], None),
("queues", "Queues", "containers", ["queue/", "queue.hpp"],
 "FIFO containers and concurrent queues, including single-producer/single-consumer and multi-producer/multi-consumer forms.", [
 ("micron::queue<T, N>", "A FIFO container of elements of type `T`."),
 ("micron::spsc_queue<T, N>", "A bounded queue intended for one producer and one consumer."),
 ("spsc_queue::push(value)", "Attempts to enqueue `value`; returns whether the insertion succeeded."),
 ("spsc_queue::pop(out)", "Attempts to remove an element into `out`; returns whether an element was obtained.")], "queue.cpp"),
("stacks", "Stacks", "containers", ["stacks/", "stack.hpp"],
 "Last-in, first-out containers with fixed, dynamic, immutable, and concurrent variants.", [
 ("micron::sstack", "A stack container with `push`, `pop`, `top`, `empty`, and `full` operations."),
 ("stack::top()", "Accesses the element at the top of the selected stack.")], None),
("heaps", "Heaps", "containers", ["heap/"],
 "Priority containers and related heap structures, including binary, Fibonacci, binomial, and quake heaps.", [
 ("micron::fibonacci_heap", "A heap supporting insertion and removal of a priority element."),
 ("fibonacci_heap::insert(value), fibonacci_heap::pop()", "Add a value and remove a priority element, respectively."),
 ("fibonacci_heap::max()", "Accesses the maximum element.")], None),
("sets", "Sets", "containers", ["sets/"],
 "Key-only associative containers, including immutable and Swiss-table variants.", [
 ("micron::immutable_set", "A set whose insertion and erasure operations produce updated set state."),
 ("immutable_set::contains(key)", "Tests whether `key` is present.")], None),
("lists", "Lists", "containers", ["list.hpp", "doublelist.hpp"],
 "Linked sequences provide node-based storage and iterator traversal.", [
 ("micron::list<T>", "A linked sequence of elements of type `T`."),
 ("list::push_back(value)", "Appends `value` to the sequence.")], None),
("algorithms", "Algorithms", "algorithms", ["algorithm/", "algorithm.hpp"],
 "Algorithms operate on iterator ranges or containers, depending on the overload. The lazy algorithms have a separate section.", [
 ("micron::find(first, last, value)", "Searches the half-open range `[first, last)` for `value` and returns a pointer to the first match, or a null pointer when no match is found."),
 ("micron::find_if(first, last, predicate)", "Returns the first element satisfying `predicate`, or a null pointer if none does. Available overloads accept predicates on values or pointers."),
 ("micron::count(first, last, value)", "Returns the number of elements equal to `value` in the range."),
 ("micron::equal(first, last, other)", "Tests whether the elements in the first range equal the corresponding elements beginning at `other`."),
 ("micron::fp", "Contains functional algorithms that produce results from containers and functions.")], "algorithms.cpp"),
("functional", "Functional algorithms", "algorithms", ["algorithm/fp.hpp", "algorithm/fpalgorithm.hpp", "algorithm/fparith.hpp", "algorithm/fpdata.hpp", "algorithm/fperrors.hpp", "algorithm/fpfilter.hpp", "algorithm/fpmap.hpp", "algorithm/fptree.hpp"],
 "Functional operations transform, select, combine, and reduce container contents. The selected operation determines its result container and error behavior.", [
 ("micron::fp::fmap(function, container)", "Applies `function` to the elements of `container` and returns the resulting container."),
 ("micron::fp::take(container, n)", "Returns a container containing up to the first `n` elements.")], None),
("lazy", "Lazy algorithms", "algorithms", ["algorithm/lazy/", "lz.hpp"],
 "Pull-based views in `micron::lz` compose with `operator|`. A view chain is evaluated when a terminal consumes it.", [
 ("lz::filter(predicate)", "Produces a view of elements for which `predicate` is true."),
 ("lz::fmap(function)", "Produces a view that applies `function` to each element requested by the consumer."),
 ("lz::take(n)", "Produces a view containing at most `n` elements from its input."),
 ("lz::collect<C>()", "Consumes a bounded input view and constructs a container of type `C` containing its elements."),
 ("lz::sum(), lz::count()", "Consume the input and return a scalar sum or element count, respectively."),
 ("lz::head()", "Returns an `option` containing the first value or an error alternative when the input is empty.")], "lazy.cpp"),
("sorting", "Sorting", "algorithms", ["sort/"],
 "Sorting and selection algorithms in `micron::sort`. Overloads accept containers or iterator ranges as specified by the algorithm.", [
 ("micron::sort::quick(container)", "Reorders the container using quicksort."),
 ("micron::sort::insertion(container, comparator)", "Reorders the container using insertion sort and the supplied ordering predicate."),
 ("micron::sort::merge(container)", "Reorders the container using merge sort."),
 ("micron::sort::is_sorted(first, last)", "Tests whether the elements in the selected range are ordered.")], "algorithms.cpp"),
("hashing", "Hashing", "algorithms", ["hash/"],
 "Hash functions, checksums, and hash types for byte sequences and values. Select the function and seed required by the data format or container.", [
 ("micron::hashes::murmur", "A member of the Murmur hash family."),
 ("micron::adler32", "Computes an Adler-32 checksum for the supplied data.")], None),
("regex", "Regular expressions", "algorithms", ["regex/", "regex.hpp"],
 "Compiled regular-expression matching and capture results.", [
 ("micron::regex(pattern)", "Constructs an expression from `pattern`. `valid()` reports whether the pattern was accepted."),
 ("regex::match(input), regex::search(input)", "Search `input` for a match and return an `rmatch` describing it."),
 ("regex::has_match(input)", "Returns true if a match is found."),
 ("regex::search_n(data, length)", "Searches `length` characters starting at `data`; embedded null characters are included."),
 ("rmatch::group_start(i), rmatch::group_end(i)", "Return the start and end offsets of capture `i`, or `-1` when the capture is absent.")], "regex.cpp"),
("allocation", "Allocation", "memory", ["memory/allocation/", "alloc.hpp", "allocator.hpp", "cmalloc.hpp"],
 "Allocation interfaces expose byte chunks, allocators, memory resources, and the abcmalloc allocator. A `chunk` describes an address and length; it does not release the allocation on destruction.", [
 ("micron::allocator_serial<>::create(bytes)", "Allocates storage for at least the requested number of bytes and returns a `chunk<byte>` with `ptr` and `len` members."),
 ("micron::allocator_serial<>::grow(chunk, bytes)", "Resizes the allocation described by `chunk` and returns the resulting chunk. The original address can be invalidated."),
 ("micron::allocator_serial<>::destroy(chunk)", "Releases the allocation represented by `chunk` using the same allocator."),
 ("micron::arena_resource", "Manages a region for allocations. Marks, rewinds, resets, and release operate on the resource's storage; object lifetimes remain the caller's responsibility."),
 ("micron::allocator_traits<A>", "Provides a uniform interface to the supported allocation operations of allocator `A`."),
 ("allocator_traits::has_unsized_deallocate<Alignment>", "Evaluates to true when the allocator supplies one of the unsized `deallocate` or `destroy` operations tested by the trait, with or without an explicit alignment template argument.")], "memory.cpp"),
("memory", "Memory operations", "memory", ["memory/cmemory/", "memory/", "mem.hpp", "memory_block.hpp"],
 "Memory copying, movement, filling, comparison, and address utilities. Select the overload according to whether its count is expressed in elements or bytes.", [
 ("micron::memcpy(destination, source, count)", "Copies `count` elements for the typed overload and returns `destination`. The source and destination ranges must not overlap."),
 ("micron::memmove(destination, source, count)", "Moves the selected range and permits overlap."),
 ("micron::memset(destination, value, count)", "Fills `count` bytes at `destination` with the byte value `value` and returns `destination`."),
 ("micron::memcmp(left, right, count)", "Compares the selected memory ranges and returns an ordering result.")], "memory.cpp"),
("pointers", "Pointer utilities", "memory", ["memory/pointers/", "pointer.hpp"],
 "Ownership and observation wrappers for dynamically managed objects.", [
 ("micron::ptr<T>", "A unique ownership pointer. Construction can forward arguments to the managed object; moving transfers ownership."),
 ("micron::sptr<T>", "A shared ownership pointer. Copying a shared pointer retains a reference to its managed object."),
 ("micron::wptr<T>", "A non-owning observer associated with a shared pointer."),
 ("micron::atom_ptr<T>", "An ownership pointer with atomic pointer operations.")], "pointers.cpp"),
("atomics", "Atomic operations", "concurrency", ["atomic/"],
 "Atomic values, operations, and memory-order selections for synchronization between execution contexts.", [
 ("micron::atomic_token<T>", "Stores an atomic value of type `T` and provides operations including loads, stores, and compare-and-swap."),
 ("atomic_token::get(order)", "Loads the value using the selected memory order."),
 ("atomic_token::store(value, order)", "Stores `value` using the selected memory order.")], "atomic.cpp"),
("mutexes", "Mutexes and locks", "concurrency", ["mutex/"],
 "Mutexes, guards, barriers, and other locking primitives.", [
 ("micron::mutex", "A non-copyable spin mutex."),
 ("mutex::lock()", "Waits until the mutex is acquired and returns an unlock member-function handle."),
 ("mutex::try_lock()", "Attempts to acquire the mutex without waiting. Returns true on acquisition."),
 ("mutex::unlock()", "Releases the mutex."),
 ("micron::lock_guard<M>", "Acquires the supplied mutex on construction and releases it when the guard is destroyed.")], "sync.cpp"),
("synchronization", "Synchronization", "concurrency", ["sync/"],
 "Synchronization utilities include semaphores, futures, channels, latches, and yield operations.", [
 ("micron::basic_semaphore(initial)", "Constructs a semaphore with an initial permit count."),
 ("basic_semaphore::wait()", "Waits for a permit and consumes it. An unexpected futex error can also cause it to return; this function does not return an error code."),
 ("basic_semaphore::try_wait()", "Attempts to consume a permit without waiting; returns true if one was obtained."),
 ("basic_semaphore::flag()", "Adds a permit and wakes a waiter when required.")], "sync.cpp"),
("threads", "Threads", "concurrency", ["thread/"],
 "Thread objects, thread groups, pools, scheduling, and CPU affinity. Backend selection depends on the build configuration.", [
 ("micron::auto_thread<>(function, args...)", "Starts a thread that invokes `function` with the supplied arguments."),
 ("auto_thread::result<T>()", "Retrieves the thread function's result as type `T`."),
 ("auto_thread::join()", "Waits for a joinable thread to complete and joins it."),
 ("micron::solo::spawn(function, args...)", "Creates an independent thread object and returns an owning pointer to it."),
 ("micron::solo::join(thread)", "Joins the specified thread. The pointer overload clears its owning pointer after joining.")], "thread.cpp"),
("parallel", "Parallel algorithms", "concurrency", ["parallel/"],
 "Parallel execution helpers and algorithms for applying operations to ranges and work items.", [
 ("micron::parallel::find", "Searches for a value using the parallel algorithm interface."),
 ("micron::parallel::fill", "Fills the selected output using the parallel algorithm interface.")], None),
("tasks", "Tasks and coroutines", "concurrency", ["tasks/", "coroio.hpp"],
 "Coroutine tasks, executors, and asynchronous coordination facilities.", [
 ("micron::async_channel", "A channel for asynchronous producers and consumers, with push, pull, and close operations."),
 ("micron::coro::async_barrier", "A coroutine-aware barrier with an `arrive_and_wait` operation.")], None),
("io", "Input and output", "io", ["io/", "io.hpp"],
 "Descriptors, files, streams, formatted output, serialization, and filesystem operations. Low-level data-path operations report byte counts or negative error values.", [
 ("micron::io::read(fd, buffer, count)", "Reads up to `count` bytes into `buffer` from descriptor `fd`. Returns a nonnegative byte count, or a negative error value."),
 ("micron::io::write(fd, buffer, count)", "Writes up to `count` bytes from `buffer` to descriptor `fd`. Returns a nonnegative byte count, or a negative error value."),
 ("micron::io::print(args...)", "Formats and writes the supplied arguments to standard output without appending a newline."),
 ("micron::io::println(args...)", "Formats and writes one or more arguments to standard output, followed by a newline. An empty argument pack writes nothing."),
 ("micron::io::printn(args...)", "Writes each supplied argument followed by a newline.")], "io.cpp"),
("filesystem", "Files and filesystem", "io", ["io/file.hpp", "io/cached_file.hpp", "io/filesystem.hpp", "io/concurrent_filesystem.hpp", "io/fsys.hpp", "io/paths.hpp", "io/realpath.hpp", "io/ftw.hpp"],
 "File handles, paths, directory traversal, filesystem operations, and whole-file helpers.", [
 ("micron::io::file", "A file handle with read, write, seek, and line-processing operations."),
 ("micron::io::open_file(path, mode)", "Opens `path` with the selected mode and returns a file object. Inspect `valid()` and `raw_fd()` to determine whether opening succeeded."),
 ("micron::io::file_size(path)", "Returns the file size in bytes, or `-1` when the metadata query fails."),
 ("micron::io::read_file<T>(path)", "Reads a file into `T` and returns `option<T, io::error_t>`. The first alternative is the value; the second is an error."),
 ("micron::io::basic_filesystem", "Manages a collection of file handles with open, close, lookup, and synchronization operations.")], "filesystem.cpp"),
("flash", "Asynchronous file I/O", "io", ["io/flash.hpp"],
 "The `micron::io::flash` interfaces use io_uring for file operations. The available operation tier depends on kernel capabilities and successful ring initialization.", [
 ("micron::io::flash::engine", "Owns an io_uring-backed engine and provides initialization, submission, and shutdown operations."),
 ("micron::io::flash::tier", "Identifies the available operation tier: `none`, `plumbing`, `basic`, or `fixed`."),
 ("engine::live()", "Reports whether the engine is live."),
 ("engine::level()", "Returns the selected capability tier.")], None),
("serialization", "Serialization", "io", ["io/serial.hpp", "io/canonical.hpp"],
 "Serialization facilities frame data and reconstruct values from byte sequences or files.", [
 ("micron::io::serialize::frame(file, value)", "Serializes `value` into a frame and writes it at the file's current offset. Returns a byte count or a negative error value."),
 ("micron::io::serialize::unframe(file, out)", "Reads a frame from the file's current offset and reconstructs `out`. Returns a byte count or a negative error value."),
 ("micron::io::serialize::canonical::writer", "Writes canonical serialized values to a byte destination.")], None),
("terminal", "Terminal control", "io", ["io/term/"],
 "Terminal-control sequences and ANSI formatting helpers.", [
 ("micron::ansi", "Contains helpers and constants for terminal control sequences.")], None),
("input", "Input devices", "io", ["io/uxin/"],
 "Input-device abstractions, event decoding, key mapping, polling, virtual devices, and Wayland input sources. Availability depends on the host environment.", [
 ("micron::uxin::device_t", "Represents an input device in the input subsystem."),
 ("micron::uxin::basic_wayland_source", "Provides a Wayland input source with bind, polling, and release operations.")], None),
("io_coro", "Coroutine I/O", "io", ["io/coroutine/"],
 "Coroutine-oriented file, pipe, filesystem, and stream operations. The coroutine and I/O backends must be available in the selected build.", [
 ("<micron/coroio.hpp>", "Provides the coroutine I/O entry header.")], None),
("math", "Mathematical functions", "numeric", ["math/", "math.hpp", "numerics.hpp", "rng.hpp"],
 "Scalar and packed mathematical functions, numeric types, and specialized mathematical facilities. Subsections describe their respective data structures and algorithms.", [
 ("micron::math::fsqrt(x)", "Computes a square root of `x`; overloads accept supported floating-point and integer types."),
 ("micron::math::cbrt(x)", "Computes a cube root for a floating-point argument."),
 ("micron::math::frsqrt(x)", "Computes a reciprocal square root for `x`.")], "math.cpp"),
("numeric_types", "Numeric types", "numeric", ["math/complex.hpp", "math/rational.hpp", "math/decimal.hpp", "math/float16.hpp", "math/dd64.hpp"],
 "Complex, rational, decimal-conversion, and reduced-precision floating-point facilities.", [
 ("micron::math::complex<T>", "Stores a complex value with real and imaginary components of type `T`."),
 ("micron::math::rational", "Represents a rational number with numerator and denominator accessors."),
 ("micron::math::float16", "Represents a 16-bit floating-point value and provides conversion and bit-access operations."),
 ("micron::math::bfloat16", "Represents a bfloat16 value and provides conversion and bit-access operations.")], None),
("symbolic", "Symbolic expressions", "numeric", ["math/symbolic.hpp"],
 "Symbolic expression storage, operations, and evaluation support.", [
 ("micron::math::symbolic::arena", "Owns the storage used for symbolic expression nodes."),
 ("micron::math::symbolic::expression", "Represents a symbolic expression in an arena.")], None),
("packed_math", "Packed mathematical functions", "numeric", ["math/simd/", "math/simd.hpp"],
 "Packed mathematical functions operate on vector values using the available compile-time SIMD backend.", [
 ("micron::math::mk::exp", "Computes exponential values for the supplied packed input."),
 ("micron::math::mk::log", "Computes logarithms for the supplied packed input.")], None),
("arbint", "Arbitrary-precision integers", "numeric", ["math/arbint/", "math/arbint.hpp"],
 "Arbitrary-precision integer arithmetic and low-level multi-precision limb operations.", [
 ("micron::math::to_chars", "Converts an arbitrary-precision value to characters in the selected representation."),
 ("micron::math::from_chars", "Parses characters into an arbitrary-precision value.")], None),
("blas", "BLAS", "numeric", ["math/blas/"],
 "Vector and matrix operations in `micron::math::blas`, including level 1, 2, and 3 operations and supported dispatch variants.", [
 ("micron::math::blas::level1::axpy", "Adds a scaled input vector to an output vector."),
 ("micron::math::blas::level1::scal", "Scales the selected vector."),
 ("micron::math::blas::level3::gemm", "Performs a general matrix multiplication with the selected operands and scaling parameters.")], "blas.cpp"),
("matrix", "Matrices and linear algebra", "numeric", ["math/matrix/", "math/linalg/", "math/matrix.hpp", "math/linalg.hpp"],
 "Matrix types and linear algebra operations, including decompositions and systems of equations.", [
 ("micron::math::dynmat", "A dynamically sized matrix with element access and storage inspection."),
 ("micron::math::linalg::trace(matrix)", "Computes the trace of the supplied matrix."),
 ("micron::math::linalg::det(matrix)", "Computes the determinant of the supplied matrix.")], None),
("graphs", "Graphs", "numeric", ["math/graph/", "math/graph.hpp", "io/graph.hpp"],
 "Graph representations, traversal, paths, connectivity, flows, matching, and graph I/O.", [
 ("micron::math::graph<T>", "A graph whose vertices can carry values of type `T`."),
 ("graph::add_vertex(value)", "Adds a vertex with the supplied value and returns its vertex identifier."),
 ("graph::add_edge(from, to)", "Adds an edge between the selected vertices."),
 ("micron::math::graphs::bfs(graph, start)", "Traverses the graph from `start` in breadth-first order and returns a traversal result."),
 ("micron::math::graphs::shortest_path(graph, from, to)", "Computes a path result between the selected vertices. Inspect its status before using the result.")], "graph.cpp"),
("fft", "Fourier transforms", "numeric", ["math/fft/", "math/fft.hpp"],
 "Fourier transforms, transform plans and workspaces, frequency helpers, and convolution.", [
 ("micron::math::fft::rfft(input, n)", "Transforms `n` real input samples and returns the frequency-domain result."),
 ("micron::math::fft::irfft(input, n)", "Reconstructs `n` real samples from the frequency-domain input."),
 ("micron::math::fft::descriptor<T>", "Describes a transform's dimensions, domain, and layouts."),
 ("micron::math::fft::plan<T>", "Constructs a reusable transform plan from a descriptor."),
 ("micron::math::fft::workspace<T>", "Provides scratch storage for a transform plan.")], "fft.cpp"),
("random", "Random numbers", "numeric", ["math/rng/", "math/rng.hpp"],
 "Random-number engines, distributions, and random sampling facilities.", [
 ("micron::math::rng::splitmix64", "A deterministic 64-bit random-number engine."),
 ("micron::math::rng::xoshiro256ss", "A deterministic xoshiro-family engine."),
 ("micron::math::rng::mt19937", "A Mersenne Twister engine.")], None),
("integrate", "Integration", "numeric", ["math/integrate/", "math/integrate.hpp"],
 "Numerical quadrature, derivatives, cubature, and integration workspaces.", [
 ("micron::math::integrate::cubature", "Integrates a callable over a multidimensional region using the selected options."),
 ("micron::math::integrate::cubature_result", "Contains the result and status of a cubature computation.")], None),
("splines", "Splines and interpolation", "numeric", ["math/splines/", "math/splines.hpp"],
 "Interpolation and spline construction, evaluation, and differentiation.", [
 ("micron::math::splines::bspline", "Represents a B-spline."),
 ("micron::math::splines::evaluate(spline, x)", "Evaluates a spline at parameter `x`."),
 ("micron::math::splines::derivative_spline", "Constructs a spline representing a derivative.")], None),
("geometry", "Geometry", "numeric", ["math/geometry/", "math/geometry.hpp"],
 "Geometric primitives, transformations, projections, and spatial calculations.", [
 ("micron::math::geometry::aligned_box", "Represents an axis-aligned box."),
 ("micron::math::geometry::hyperplane", "Represents a hyperplane."),
 ("micron::math::geometry::look_at", "Constructs a view transformation from the supplied geometry.")], None),
("manifolds", "Manifolds", "numeric", ["math/manifolds/", "math/manifolds.hpp"],
 "Embedded manifolds, Lie groups, tangent-space operations, and metrics.", [
 ("micron::math::manifolds::sphere", "Represents a sphere manifold."),
 ("micron::math::manifolds::euclidean", "Represents a Euclidean manifold."),
 ("micron::math::manifolds::spd", "Represents a manifold of symmetric positive-definite matrices.")], None),
("quaternions", "Quantities and quaternions", "numeric", ["math/quaternions/", "math/quants/", "math/quaternions.hpp", "math/quants.hpp"],
 "Vectors, quantities, and quaternion operations for rotations and related computations.", [
 ("micron::math::dynvec", "A dynamically sized vector of numeric values."),
 ("micron::math::quaternions::quaternion", "Represents a quaternion."),
 ("micron::math::quaternions::conjugate(q)", "Returns the conjugate of quaternion `q`."),
 ("micron::math::quaternions::normalize(q)", "Returns a normalized quaternion.")], "quaternion.cpp"),
("sparse", "Sparse matrices", "numeric", ["math/sparse/", "math/sparse.hpp"],
 "Sparse matrix representations, conversion, and arithmetic.", [
 ("micron::math::sparse::csr", "Stores a sparse matrix in compressed sparse row form."),
 ("micron::math::sparse::csc", "Stores a sparse matrix in compressed sparse column form."),
 ("micron::math::sparse::spmv", "Multiplies a sparse matrix by a vector.")], None),
("compute", "Computation graphs", "numeric", ["math/compute/", "math/compute.hpp"],
 "Static-shape computation graphs, reusable storage, and executors for numeric workloads.", [
 ("micron::math::compute::thread_pool_executor", "Executes computation work using a thread pool.")], None),
("chrono", "Date and time", "system", ["chrono/", "chrono.hpp"],
 "Clocks, durations, calendar calculations, parsing, formatting, and time-zone facilities.", [
 ("micron::chrono::is_leap(year)", "Tests whether `year` is a leap year."),
 ("micron::chrono::days_in_month(year, month)", "Returns the number of days in the selected month."),
 ("micron::chrono::days_from_civil(year, month, day)", "Converts a civil date to its day count.")], "chrono.cpp"),
("wasm", "WebAssembly target", "system", ["bits/wasm/", "memory/allocation/wasmalloc/"],
 "The wasm32 target provides a linear-memory allocator and a host ABI for diagnostics, clocks, randomness, and execution. Threaded builds require the corresponding host and shared-memory support.", [
 ("micron::wasm::write_diag(data, length, stream)", "Writes `length` bytes from `data` to the selected diagnostic stream through the host and returns the host result."),
 ("micron::wasm::panic(code)", "Reports `code` through the host panic operation and does not return."),
 ("micron::wasmalloc", "Provides the wasm32 allocation backend. Freeing an allocation makes its storage reusable; it does not shrink the module's linear memory."),
 ("createLocal(module, memory, options)", "The JavaScript host helper creates a local host around a module and its linear memory.")], None),
("linux", "Linux interfaces", "system", ["linux/", "syscall.hpp", "proc.hpp"],
 "Linux types, system-call interfaces, process operations, and low-level operating-system facilities. Individual interfaces can depend on kernel support and process permissions.", [
 ("micron::posix", "Contains Linux-facing types and syscall wrappers."),
 ("micron::micthread", "Contains the freestanding thread backend and related operations.")], None),
("security", "Security", "system", ["sec/", "sec.hpp"],
 "Security policies and Linux security interfaces, including seccomp, namespaces, Landlock, and SELinux helpers. Kernel support and process privileges determine which operations are available.", [
 ("micron::sec::seccomp::policy", "Describes a seccomp policy."),
 ("micron::sec::seccomp::allow", "Adds an allow rule to a seccomp policy."),
 ("micron::sec::seccomp::deny_all", "Adds a default denial rule to a seccomp policy.")], None),
("dynamic", "Dynamic loading", "system", ["dynamic/", "dynamic.hpp"],
 "Dynamic-module loading, symbol lookup, and module metadata.", [
 ("micron::dynamic_open", "Opens a dynamic module according to the selected path and flags."),
 ("micron::dynamic_sym", "Looks up a symbol in a dynamic module."),
 ("micron::dynamic_close", "Closes a dynamic module handle.")], None),
("elf", "ELF", "system", ["elf/", "elf.hpp"],
 "ELF image loading, execution, and symbol inspection facilities.", [
 ("micron::elf::open", "Opens an ELF image."),
 ("micron::elf::list_symbols", "Enumerates symbols from an ELF image."),
 ("micron::elf::execute", "Executes an ELF image through the selected interface.")], None),
("attach", "Module attachment", "system", ["attach/", "start.hpp"],
 "Entry points and argument structures used for attaching modules and continuations. These interfaces are enabled by their corresponding build definitions.", [
 ("_attach", "The attachment entry point for a configured module."),
 ("micron_mx_entry_args", "The versioned argument structure supplied to an MX entry point.")], None),
("graphics", "Graphics", "graphics", ["gfx/", "gfx.hpp", "gl.hpp"],
 "Graphics abstractions and platform support, with OpenGL and Vulkan interfaces in their respective subsections.", [
 ("micron::gfx::platform::display", "Manages a display connection using an available platform backend.")], None),
("opengl", "OpenGL", "graphics", ["gfx/gl/"],
 "OpenGL-facing types, operations, and platform integration. The corresponding graphics implementation and context must be available at runtime.", [
 ("micron::gfx::gl::context", "Manages an OpenGL context associated with a window and its platform display."),
 ("micron::gfx::gl::buffer", "Manages an OpenGL buffer object."),
 ("micron::gfx::gl::program", "Manages an OpenGL program object.")], None),
("vulkan", "Vulkan", "graphics", ["gfx/vk/", "gfx/vk.hpp"],
 "Vulkan-facing types, functions, allocation callbacks, and loader integration. The corresponding Vulkan implementation must be available at runtime.", [
 ("micron::gfx::vk::instance", "Manages a Vulkan instance and exposes its native handle."),
 ("micron::gfx::vk::device", "Manages a Vulkan device and exposes its native handle."),
 ("micron::gfx::vk::host_allocation_callbacks", "Returns allocation callbacks for the Vulkan host interface.")], None),
("simd", "SIMD", "simd", ["simd/"],
 "Packed vector types, operations, and compile-time architecture selection. Available operations depend on the target and enabled instruction-set features.", [
 ("micron::simd::sse::load_f32", "Loads floating-point elements into a packed vector."),
 ("micron::simd::sse::splat_f32", "Replicates a floating-point value across the lanes of a packed vector.")], None),
("internal", "Implementation notes", "internal", [],
 "Internal headers implement architecture selection, compiler support, low-level dispatch, and runtime machinery. Names beginning with `__` are implementation details. Public interfaces are indexed under their owning modules.", [], None),
]
