"""Additional contracts for frequently used interfaces."""

# Keys match the module id and selected interface heading in modules.py.
DETAILS = {
("arrays", "micron::array<T, N>"): {
 "header": "array/array.hpp",
 "declarations": ["template<is_regular_object T, usize N = 64>\n  requires (N > 0 and ((N * sizeof(T)) < (1 << 22)))\nclass array;"],
 "parameters": [("T", "The element type. It must satisfy `is_regular_object`."), ("N", "The number of elements. It must be positive and the object's element storage must be less than 2²² bytes.")],
 "notes": "The elements are stored in the array object. Destruction destroys its elements. The initializer-list constructor fills omitted elements with `T{}` and reports an error if the list is too large."
},
("vectors", "micron::vector<T, Alloc, Sf>"): {
 "header": "vector/vector.hpp",
 "declarations": ["template<typename T, class Alloc = micron::allocator_serial<>, bool Sf = true>\nclass vector;"],
 "parameters": [("T", "The element type."), ("Alloc", "The allocator type used for the element storage."), ("Sf", "Enables the container's checks when true.")],
 "notes": "The vector owns its element storage. Operations that expand storage can invalidate pointers, references, and iterators. Copy and move availability depends on the element type."
},
("vectors", "vector::size(), vector::max_size()"): {
 "header": "vector/vector.hpp", "declarations": ["size_type size() const;", "size_type max_size() const;"],
 "returns": "`size()` returns the number of constructed elements. `max_size()` returns the current allocation capacity in elements."
},
("vectors", "vector::reserve(n)"): {
 "header": "vector/vector.hpp", "declarations": ["void reserve(size_type n);"],
 "parameters": [("n", "Minimum requested capacity, in elements.")],
 "returns": "None.",
 "notes": "If `n` does not exceed the current capacity, no expansion is performed. On expansion, existing pointers, references, and iterators can be invalidated. The number of elements is unchanged."
},
("vectors", "vector::find(value)"): {
 "header": "vector/vector.hpp", "declarations": ["iterator find(const T& value);", "const_iterator find(const T& value) const;"],
 "parameters": [("value", "Value to locate in the vector.")],
 "returns": "A pointer to the first matching element, or a null pointer if no element matches."
},
("maps", "robin_map::find(key)"): {
 "header": "maps/robin.hpp", "declarations": ["V* find(const K& key);", "const V* find(const K& key) const;"],
 "parameters": [("key", "The key to locate.")], "returns": "A pointer to the mapped value, or a null pointer if the key is absent."
},
("maps", "robin_map::insert(key, value)"): {
 "header": "maps/robin.hpp", "parameters": [("key", "The key to insert."), ("value", "The mapped value to insert.")],
 "returns": "A pointer to the mapped value. The available overloads determine whether the arguments are copied or moved.",
 "notes": "Insertion can report a library error when the table cannot accommodate the probe sequence."
},
("algorithms", "micron::find(first, last, value)"): {
 "header": "algorithm/find.hpp",
 "declarations": ["template<typename T, class P>\nconstexpr T* find(T* first, T* last, const P& value) noexcept;", "template<typename T, class P>\nconstexpr const T* find(const T* first, const T* last, const P& value) noexcept;"],
 "parameters": [("first, last", "Pointers delimiting the half-open range to inspect."), ("value", "The value to find.")],
 "returns": "A pointer to the first matching element, or a null pointer when no match is found.",
 "notes": "The pointer overload requires the element type and search value to satisfy its conversion constraint. Container overloads are also available."
},
("algorithms", "micron::find_if(first, last, predicate)"): {
 "header": "algorithm/find.hpp",
 "parameters": [("first, last", "Pointers delimiting the half-open range to inspect."), ("predicate", "A callable tested on each element or element pointer, according to the selected overload.")],
 "returns": "A pointer to the first element for which the predicate is true, or a null pointer when none matches."
},
("lazy", "lz::filter(predicate)"): {
 "header": "algorithm/lazy/filter.hpp",
 "parameters": [("predicate", "A callable determining whether an input element is included.")],
 "returns": "A view adaptor. The predicate is evaluated as elements are requested from the resulting view.",
 "notes": "The input range must remain valid while a view referring to it is consumed."
},
("lazy", "lz::fmap(function)"): {
 "header": "algorithm/lazy/transform.hpp", "parameters": [("function", "A callable applied to an input element.")],
 "returns": "A view adaptor that transforms elements on demand."
},
("lazy", "lz::take(n)"): {
 "header": "algorithm/lazy/take.hpp", "parameters": [("n", "Maximum number of elements to produce.")],
 "returns": "A view adaptor that stops after at most `n` elements."
},
("lazy", "lz::collect<C>()"): {
 "header": "algorithm/lazy/collect.hpp", "parameters": [("C", "The output container type.")],
 "returns": "A container of type `C` populated from the input view.",
 "notes": "The input must be bounded. Use an adaptor such as `take` before collecting an unbounded source."
},
("regex", "regex::match(input), regex::search(input)"): {
 "header": "regex/regex.hpp", "declarations": ["template<class S> rmatch match(const S& input) const noexcept;", "template<class S> rmatch search(const S& input) const noexcept;"],
 "parameters": [("input", "The character sequence to search.")],
 "returns": "An `rmatch`. Check `has_match()` before using its captures.",
 "notes": "Both operations perform an unanchored search. Captured spans refer to the input, so the input must remain valid while those spans are used."
},
("regex", "rmatch::group_start(i), rmatch::group_end(i)"): {
 "header": "regex/regex.hpp", "declarations": ["i64 group_start(usize i) const noexcept;", "i64 group_end(usize i) const noexcept;"],
 "parameters": [("i", "Capture index; zero identifies the whole match.")],
 "returns": "The offset from the start of the input, or `-1` if there is no corresponding capture. The end offset is exclusive."
},
("allocation", "micron::allocator_serial<>::create(bytes)"): {
 "header": "allocator.hpp", "parameters": [("bytes", "Number of bytes requested.")],
 "returns": "A `chunk<byte>` containing a pointer and the allocation's reported length.",
 "notes": "Release the result with the matching allocator's `destroy` operation. A chunk is a descriptor and does not free the allocation itself."
},
("memory", "micron::memcpy(destination, source, count)"): {
 "header": "memory/cmemory/memcpy.hpp", "declarations": ["template<typename F, typename D>\nconstexpr F* memcpy(F* destination, const D* source, u64 count) noexcept;"],
 "parameters": [("destination", "Pointer to the first destination element."), ("source", "Pointer to the first source element."), ("count", "Number of elements to copy for this typed overload.")],
 "returns": "`destination`.", "notes": "The source and destination ranges must not overlap. A separate `void*` overload takes its count in bytes."
},
("memory", "micron::memmove(destination, source, count)"): {
 "header": "memory/cmemory/memmove.hpp", "declarations": ["template<typename F, typename D>\nF* memmove(F* destination, D* source, u64 count) noexcept;"],
 "parameters": [("destination, source", "Pointers to the destination and source ranges."), ("count", "Number of elements for this typed overload.")],
 "returns": "`destination`.", "notes": "The ranges may overlap. A separate `void*` overload takes its count in bytes."
},
("memory", "micron::memset(destination, value, count)"): {
 "header": "memory/cmemory/memset.hpp", "declarations": ["template<typename F>\nF* memset(F* destination, byte value, u64 count) noexcept;"],
 "parameters": [("destination", "Address of the first destination byte."), ("value", "Byte value to write."), ("count", "Number of bytes to write.")],
 "returns": "`destination`."
},
("mutexes", "mutex::lock()"): {
 "header": "mutex/mutex.hpp", "declarations": ["auto lock();"],
 "returns": "A member-function handle used by micron's lock wrappers to release this mutex.",
 "notes": "The call waits until it acquires the mutex. The mutex must be released after the protected work completes."
},
("mutexes", "mutex::try_lock()"): {
 "header": "mutex/mutex.hpp", "declarations": ["bool try_lock() noexcept;"],
 "returns": "True if the mutex was acquired; false if it was already held."
},
("synchronization", "basic_semaphore::try_wait()"): {
 "header": "sync/semaphore.hpp", "declarations": ["bool try_wait() noexcept;"],
 "returns": "True if a permit was available and consumed; false otherwise. The call does not wait."
},
("threads", "micron::auto_thread<>(function, args...)"): {
 "header": "thread/thread_types/auto_thread.hpp", "parameters": [("function", "A callable invoked by the new thread."), ("args", "Arguments supplied to the callable.")],
 "notes": "The object is neither copyable nor movable. Its destructor joins a live thread. The worker stack is allocated separately; `Stack_Size` selects its size."
},
("threads", "auto_thread::result<T>()"): {
 "header": "thread/thread_types/auto_thread.hpp", "declarations": ["template<typename R>\n  requires (micron::is_trivially_copyable_v<R> && sizeof(R) <= sizeof(byte*))\nR result();"],
 "parameters": [("R", "The result type. It must be trivially copyable and no larger than a pointer.")],
 "returns": "The stored literal result after waiting for completion; a value-initialized `R` if the payload is not a literal result.",
 "notes": "Retrieve the result before joining: joining releases the thread payload."
},
("threads", "auto_thread::join()"): {
 "header": "thread/thread_types/auto_thread.hpp", "declarations": ["int join();"],
 "returns": "`0` after joining and releasing a thread, or `1` if it was already joined or released."
},
("io", "micron::io::read(fd, buffer, count)"): {
 "header": "io/io.hpp", "declarations": ["template<typename T>\nmax_t read(i32 fd, T* buffer, usize count);"],
 "parameters": [("fd", "An open descriptor."), ("buffer", "Destination for the bytes read."), ("count", "Maximum number of bytes to read.")],
 "returns": "The number of bytes read, including zero at end of file, or a negative error value.",
 "notes": "The pointer overload checks the buffer and descriptor before issuing the read. Other overloads accept descriptor wrappers, arrays, and containers."
},
("io", "micron::io::write(fd, buffer, count)"): {
 "header": "io/io.hpp", "declarations": ["template<typename T = byte>\nmax_t write(i32 fd, const T* buffer, usize count);"],
 "parameters": [("fd", "An open descriptor."), ("buffer", "The bytes to write."), ("count", "Maximum number of bytes to write.")],
 "returns": "The number of bytes written, or a negative error value. A successful write can be shorter than the requested count."
},
("io", "micron::io::println(args...)"): {
 "header": "io/echo.hpp", "parameters": [("args", "Values to format and write, in order.")],
 "returns": "None.", "notes": "No separator is inserted automatically. With at least one argument, a newline follows the formatted output."
},
("graphs", "micron::math::graphs::bfs(graph, start)"): {
 "header": "math/graph/traversal.hpp", "parameters": [("graph", "The graph to traverse."), ("start", "The starting vertex identifier.")],
 "returns": "A traversal result containing an `order` sequence and associated traversal information."
},
("fft", "micron::math::fft::rfft(input, n)"): {
 "header": "math/fft.hpp", "parameters": [("input", "Pointer to real input samples."), ("n", "Number of input samples.")],
 "returns": "The frequency-domain result. Pass its data and the original length to `irfft` for the inverse transform."
},
("blas", "micron::math::blas::level1::axpy"): {
 "header": "math/blas/level1.hpp", "declarations": ["template<typename T>\nconstexpr void axpy(T alpha, const T* x, const T* x_end, T* y) noexcept;"],
 "parameters": [("alpha", "Scale factor for the input values."), ("x, x_end", "The half-open input range."),
                ("y", "Output range with at least `x_end - x` elements.")],
 "returns": "None. For each element, assigns `y[i] + alpha * x[i]` to `y[i]`.",
 "notes": "The pointer parameters are restricted; use non-overlapping input and output ranges."
},
("quaternions", "micron::math::quaternions::normalize(q)"): {
 "header": "math/quaternions/algebra.hpp", "parameters": [("q", "The quaternion to normalize.")],
 "returns": "A quaternion representing the normalized value of `q`.",
 "notes": "A non-finite input or an input whose squared norm is too small produces a NaN-tagged quaternion."
},
("quaternions", "micron::math::quaternions::conjugate(q)"): {
 "header": "math/quaternions/algebra.hpp", "parameters": [("q", "The quaternion to conjugate.")],
 "returns": "A quaternion with the vector components negated and the scalar component unchanged."
},
("chrono", "micron::chrono::days_in_month(year, month)"): {
 "header": "chrono/calendar.hpp", "declarations": ["constexpr u32 days_in_month(i32 year, u32 month) noexcept;"],
 "parameters": [("year", "The civil year."), ("month", "The month number, from 1 through 12.")],
 "returns": "The number of days in the month, accounting for leap years, or zero for an invalid month."
},
("queues", "spsc_queue::push(value)"): {
 "header": "queue/spsc_queue.hpp", "parameters": [("value", "The element to enqueue.")],
 "returns": "True if the element was enqueued, or false if the queue was full."
},
("queues", "spsc_queue::pop(out)"): {
 "header": "queue/spsc_queue.hpp", "parameters": [("out", "Receives the removed element on success.")],
 "returns": "True if an element was removed, or false if the queue was empty."
},
("filesystem", "micron::io::file_size(path)"): {
 "header": "io/fsys.hpp", "declarations": ["posix::off64_t file_size(const io::path_t& path);"],
 "parameters": [("path", "Path of the file to inspect.")],
 "returns": "The size reported by the filesystem in bytes, or `-1` if the metadata query fails."
},
("filesystem", "micron::io::read_file<T>(path)"): {
 "header": "io/fsys.hpp", "parameters": [("T", "The string or contiguous container type used for the result; its elements must be one byte."),
                                             ("path", "Path of the file to read.")],
 "returns": "An `option<T, io::error_t>`. The first alternative holds the data; the second holds an error.",
 "notes": "Use `is_first()` to test for a value before calling `cast<T>()`."
},
}
