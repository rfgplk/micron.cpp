<img align="left" style="width:300px" src="https://github.com/user-attachments/assets/0a2fc934-93dc-448e-860f-56f9334a0946" alt="micron_logo_default" width="300"/> 

<div align="left">

### the micron core library 🦅 <img src="https://img.shields.io/badge/indev-green">

#### a core (re)design of the C++ Standard Template Library (and libc)

**micron** is a comprehensive core library; a collection of algorithms, containers, iterators, functions, and OS interfaces; a header-only core system library written in c++23 targeting the Linux syscall API.
Unlike library collections such as Boost et al., *micron* does not intend to merely *augment* the STL, but entirely replace it.

</div>

[![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)](#)
![Version](https://img.shields.io/badge/version-0.5.10.0-red)
[![License](https://img.shields.io/badge/License-Boost_1.0-lightblue.svg)](https://www.boost.org/LICENSE_1_0.txt)
[![C++23](https://img.shields.io/badge/C++-23-blue.svg)](https://en.cppreference.com/w/cpp/23)


------

<br/>

<br/>


> [!WARNING]
> micron is still in active development, and as of v0.5 has reached relative stability. Regardless, the ABI may change at any point, and without notice.

Features
--------
  - a *fully functional*, templated C++ standard library implementation, designed from the **ground up** with modern principles in mind
  - completely self-contained, self-hosted, and freestanding; with *no dependencies on external code whatsoever*; not even the traditional C standard library
  - all functions are guaranteed to be side effect free, ensuring deterministic and predictable behavior across the codebase
  - written entirely in c++23
  - a high performance, cache-aware algorithmic base architecture
  - provides an innovative foundation for systems-level development, reimagining conventional approaches to low-level programming
  - uses a modular design allowing for reduced compiled binary sizes and easier vetting of include dependencies

Who is this Library for?
-------------------------

The idea behind this library is that you shouldn't need anything more than a compliant C++ compiler to actually build useful applications. If you are in need of a truly portable (meaning any Linux system), high-performance freestanding library that offers all utilities & functions needed to program and control your system in a single repository with a relatively small footprint, this is for you.


Using the Library
-------------------

All necessary code is self-contained within the `src/` directory. Since *micron* is freestanding, it relies on no external sources; no other files or libraries are necessary (*with the sole exception of `pthread.h`, more on that below*). Simply include any header file you want into your project and compile. For examples, check out the `examples/` directory (currently being added).


#### Installation

First, clone the repository via ssh:

`git clone --depth=1 git@github.com:rfgplk/micron.cpp.git`

or https:

`git clone --depth=1 https://github.com/rfgplk/micron.cpp.git`


Then, install to your desired location either via `cp`, `rsync`, or `scripts/install_local.py`, a simple script which copies all header files to `/usr/local/micron`

Philosophy
----------

**All core library code adheres to the following design principles**:
- in all instances where functional equivalence exists between *micron* and the STL, or any third-party library, *micron* must demonstrate superior performance
- in all conceivable scenarios, this code grants the developer absolute control, both of execution and compilation
- functionality must be preserved with any arbitrary data type
- in all instances, *micron* must maintain seamless interoperability with the STL and any other library offering equivalent functionality
- in all cases, performance always takes precedence over safety, with the developer assuming full responsibility for code validity and security
- all functions follow a strict side effect free formulation (pure functions)

in short:
- the written code, in its explicit form, stands as the ultimate arbiter of truth, unyielding and devoid of ambiguity, embodying the essence of how code should perform.


Motivation
----------

<p align="justify"> 

  The Standard Template Library (STL) has long been constrained by its imperative to preserve ABI stability across different versions, notably maintaining legacy support for the earliest version, all the while keeping backward compatibility with C. This rudimentary limitation intrinsically impedes the STL's capacity to evolve and assimilate modern development paradigms. The necessity of safeguarding compatibility with legacy systems imposes an architectural limit that resists transformative change, effectively tethering the STL to design philosophies that, while once pioneering, now appear antiquated. Consequently, this has permitted more novel programming languages such as Rust, Go, Swifrt or even Zig (mentioned), to develop a markedly more sophisticated and feature complete set of core libraries. These languages, unburdened by historical bloat, are free to explore novel abstractions and innovative ideas, allowing them to embrace paradigms such as memory safety by design, zero-cost abstractions, and concurrency models that are inherently safer and more efficient.</p>

<p align="justify"> 
  
  Conversely, the *micron standard library*, unencumbered by such legacy constraints, has been reimagined ab initio with an unwavering commitment to rigorous performance and safety, thereby embodying a more powerful design ethos. Its development paradigm is liberated from the shackles of historical compatibility, allowing for an effective evolution that can readily integrate state-of-the-art techniques and designs. This freedom facilitates a design philosophy that prioritizes performance determinism and safety invariants, fostering a more robust and efficient ecosystem. By eschewing the historical baggage that impedes the STL, the *micron* library is able to transcend the limitations of its predecessor, positioning itself at the vanguard of modern software development. In doing so, it exemplifies a fundamental truth of technological software evolution: *that liberation from legacy obligations is often a prerequisite for genuine innovation*.</p>

<p align="justify"> 

  Since *micron* is specifically developed with Linux in mind, all library code is meticulously optimized for the nuances of the Linux kernel and its underlying system calls. This singular focus allows for an unparalleled level of integration and efficiency, leveraging Linux-specific features without the overhead of cross-platform abstractions. Consequently, the library achieves a degree of performance and system coherence that is unattainable in more generalized, platform-agnostic designs, solidifying its role as an indispensable tool for high-performance Linux development.</p>

Is micron entirely self-sufficient?
--------------------------------------

Yes, *micron* relies on no external code other than what is included in this repository. Meaning as long as you have a working `g++` compiler, you can compile and run it anywhere. The sole exception being, if you wish to use multithreading (or any thread related code), you **must** link against pthread.

Architecture Support
----------------------

*micron* is primarily developed for the x86_64 arch. As of v0.5, it hosts experimental ARM32/ARM64 support.

Conformance with the STL
-------------------------

*micron* currently provides numerous containers and functions which have existing implementations in the C++ Standard Template Library. Although *most* of these functions do generallyhave the same interfaces and functionality, there are minute core differences (in certain cases, significant ones) which you must be aware of. Do not assume all containers are functionally identical to the STL, because they are not.

> [!IMPORTANT]
> Documentation for the *micron* library does not currently exist, although the source is intended to be structured in a legible and understandable enough way to serve as documentation for the time being. *micron* is specifically designed for Linux and x86_64 (with limited ARM support - still experimental), as such other operating systems, kernels, or CPU architectures are entirely unsupported.

 
Libraries
-----------

### currently, micron provides the following core C++ libraries:
- [algorithm]
- [allocation]
- [array]
- [hash]
- [maps]
- [math]
- [matrix]
- [memory]
- [parallel]
- [quants]
- [queue]
- [simd]
- [sort]
- [string]
- [sync]
- [thread]
- [trees]
- [vector]


## License
Licensed under the Boost Software License, except the 'abcmalloc' memory allocator, which is licensed under the MIT License
