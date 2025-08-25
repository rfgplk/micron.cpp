<div align="center">
  <img src="https://github.com/user-attachments/assets/b66231f3-655b-4106-a111-7f72dc38b2b9" alt="micron_logo_default" width="300"/>
  
# the micron standard library
### a core reimplementation of the C++ Standard Template Library
</div>

> [!WARNING]
> micron is still in active development. The ABI may change at any point, and without notice.

#### *micron intends to bring the ideological power of C and assembly to C++, while imbuing it with the functional power of more modern languages such as Rust*

---

**micron** is a comprehensive collection of algorithms, containers, iterators, functions, and OS interfaces; a header-only C++23 reimplementation of the C++ STL *and* the C standard library targeting the Linux syscall API.
Unlike library collections such as Boost et al., *micron* does not intend to merely *augment* the STL, but entirely replace it.

---

<p align="justify"> 
The Standard Template Library (STL) has long been constrained by its imperative to preserve ABI stability across different versions. This limitation, coupled with the necessity of maintaining binary compatibility, intrinsically impedes the STL's capacity to evolve and assimilate modern development paradigms. The necessity of safeguarding compatibility with legacy systems imposes an architectural limit that resists transformative change, effectively tethering the STL to design philosophies that, while once pioneering, now appear antiquated. Consequently, this has permitted more novel programming languages such as Rust, Go, and even Zig, to develop a markedly more sophisticated set of core libraries. These languages, unburdened by historical bloat, are free to explore novel abstractions and innovative ideas, allowing them to embrace paradigms such as memory safety by design, zero-cost abstractions, and concurrency models that are inherently safer and more efficient.</p>

<p align="justify"> 
Conversely, the *micron standard library*, unencumbered by such legacy constraints, has been reimagined ab initio with an unwavering commitment to rigorous performance and safety, thereby embodying a more powerful design ethos. Its development paradigm is liberated from the shackles of historical compatibility, allowing for an effective evolution that can readily integrate state-of-the-art techniques and designs. This freedom facilitates a design philosophy that prioritizes performance determinism and safety invariants, fostering a more robust and efficient ecosystem. By eschewing the historical baggage that impedes the STL, the *micron* library is able to transcend the limitations of its predecessor, positioning itself at the vanguard of modern software development. In doing so, it exemplifies a fundamental truth of technological software evolution: *that liberation from legacy obligations is often a prerequisite for genuine innovation*.</p>

<p align="justify"> 
Since *micron* is specifically developed with Linux in mind, all library code is meticulously optimized for the nuances of the Linux kernel and its underlying system calls. This singular focus allows for an unparalleled level of integration and efficiency, leveraging Linux-specific features without the overhead of cross-platform abstractions. Consequently, the library achieves a degree of performance and system coherence that is unattainable in more generalized, platform-agnostic designs, solidifying its role as an indispensable tool for high-performance Linux development.</p>

---

**All core library code adheres to the following design principles**:
- in all instances where functional equivalence exists between *micron* and the STL, or any third-party library, *micron* must demonstrate superior performance
- in all conceivable scenarios, this code grants the developer absolute control, both of execution and compilation
- functionality must be preserved with any arbitrary data type
- in all instances, *micron* must maintain seamless interoperability with the STL and any other library offering equivalent functionality
- in all cases, performance always takes precedence over safety, with the developer assuming full responsibility for code validity and security

in short:
- the written code, in its explicit form, stands as the ultimate arbiter of truth, unyielding and devoid of ambiguity, embodying the essence of how code should perform.

***

<p align="justify"> 
While the STL (and most other libraries of it's kind) intend to restrict developer freedom, or much rather limit functionality to an approval subset of permissible behavior, *micron* makes no such restrictions, granting nearly full control not just over library code, but even library internals. As such, containers much as maps, vectors, or strings, freely expose their internal components and make them *directly accessible* via the public library API. As a result, it's perfectly simple to create a *micron*::string<byte> from a *micron*::vector<struct custom_struct>, if the need arises.</p>

<p align="justify"> 
Additionally, micron has taken great inspiration from the implementation of standard libraries of other languages, notably Rust, Golang, and Python (see algorithms, channels, arenas). By studying and adapting their most innovative concepts, micron not only inherits proven design patterns but also refines them to align with its own performance-centric philosophy.</p>

***

Is *micron* entirely independent of the STL yet? 

No. There are still a few specific headers needed to compile *micron* (although they will be removed - soon), these are:
- \<initializer_list>



> [!IMPORTANT]
> Documentation for the *micron* library does not exist for now, although the source is intended to be structured in a legible and understandable enough way to serve as documentation for the time being. *micron* is specifically designed for Linux and x86, as such other operating systems, kernels, or CPU architectures are entirely unsupported for the time being.


## currently, micron provides the following core C++ libraries:
- allocation
- algorithms
- containers
- iterators
- atomics
- hashing
- io
- maps
- math
- parallel
- threading 

## License
Licensed under the Boost Software License
