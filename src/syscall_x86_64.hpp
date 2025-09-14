//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// TODO: refactor to C++

#pragma once


// bare minimum va_list code. only included here for syscall compatibility

#define va_start(v,l)	__builtin_va_start(v,l)
#define va_end(v)	__builtin_va_end(v)
#define va_arg(v,l)	__builtin_va_arg(v,l)
#define va_copy(d,s)	__builtin_va_copy(d,s)
#define __va_copy(d,s)	__builtin_va_copy(d,s)

typedef __builtin_va_list va_list;

#ifndef va_end
#define va_end(ap) va_end(ap)
#endif

// sourced from glibc.

#ifdef MICRON_USE_GLIBC_SYSCALL
/*permitted*/#include <unistd.h>
namespace micron
{
template <typename... Args>
long int
syscall(Args... args)
{
  return ::syscall(args...);
};
};
#else
#ifdef __ASSEMBLER__

/* Linux uses a negative return value to indicate syscall errors,
   unlike most Unices, which use the condition codes' carry flag.

   Since version 2.1 the return value of a system call might be
   negative even if the call succeeded.	 E.g., the `lseek' system call
   might return a large offset.	 Therefore we must not anymore test
   for < 0, but test for a real error by making sure the value in %eax
   is a real error number.  Linus said he will make sure the no syscall
   returns a value in -1 .. -4095 as a valid result so we can safely
   test with -4095.  */

/* We don't want the label for the error handle to be global when we define
   it here.  */
#undef SYSCALL_ERROR_LABEL
#ifdef PIC
#undef SYSCALL_ERROR_LABEL
#define SYSCALL_ERROR_LABEL 0f
#else
#undef SYSCALL_ERROR_LABEL
#define SYSCALL_ERROR_LABEL syscall_error
#endif

/* PSEUDO and T_PSEUDO macros have 2 extra arguments for unsigned long
   int arguments.  */
#define PSEUDOS_HAVE_ULONG_INDICES 1

#ifndef SYSCALL_ULONG_ARG_1
#define SYSCALL_ULONG_ARG_1 0
#define SYSCALL_ULONG_ARG_2 0
#endif

#undef PSEUDO
#if SYSCALL_ULONG_ARG_1
#define PSEUDO(name, syscall_name, args, ulong_arg_1, ulong_arg_2)                                                      \
  .text;                                                                                                                \
  ENTRY(name)                                                                                                           \
  DO_CALL(syscall_name, args, ulong_arg_1, ulong_arg_2);                                                                \
  cmpq $ - 4095, % rax;                                                                                                 \
  jae SYSCALL_ERROR_LABEL
#else
#define PSEUDO(name, syscall_name, args)                                                                                \
  .text;                                                                                                                \
  ENTRY(name)                                                                                                           \
  DO_CALL(syscall_name, args, 0, 0);                                                                                    \
  cmpq $ - 4095, % rax;                                                                                                 \
  jae SYSCALL_ERROR_LABEL
#endif

#undef PSEUDO_END
#define PSEUDO_END(name)                                                                                                \
  SYSCALL_ERROR_HANDLER                                                                                                 \
  END(name)

#undef PSEUDO_NOERRNO
#if SYSCALL_ULONG_ARG_1
#define PSEUDO_NOERRNO(name, syscall_name, args, ulong_arg_1, ulong_arg_2)                                              \
  .text;                                                                                                                \
  ENTRY(name)                                                                                                           \
  DO_CALL(syscall_name, args, ulong_arg_1, ulong_arg_2)
#else
#define PSEUDO_NOERRNO(name, syscall_name, args)                                                                        \
  .text;                                                                                                                \
  ENTRY(name)                                                                                                           \
  DO_CALL(syscall_name, args, 0, 0)
#endif

#undef PSEUDO_END_NOERRNO
#define PSEUDO_END_NOERRNO(name) END(name)

#define ret_NOERRNO ret

#undef PSEUDO_ERRVAL
#if SYSCALL_ULONG_ARG_1
#define PSEUDO_ERRVAL(name, syscall_name, args, ulong_arg_1, ulong_arg_2)                                               \
  .text;                                                                                                                \
  ENTRY(name)                                                                                                           \
  DO_CALL(syscall_name, args, ulong_arg_1, ulong_arg_2);                                                                \
  negq % rax
#else
#define PSEUDO_ERRVAL(name, syscall_name, args)                                                                         \
  .text;                                                                                                                \
  ENTRY(name)                                                                                                           \
  DO_CALL(syscall_name, args, 0, 0);                                                                                    \
  negq % rax
#endif

#undef PSEUDO_END_ERRVAL
#define PSEUDO_END_ERRVAL(name) END(name)

#define ret_ERRVAL ret

#if defined PIC && RTLD_PRIVATE_ERRNO
#define SYSCALL_SET_ERRNO                                                                                               \
  lea rtld_errno(% rip), % RCX_LP;                                                                                      \
  neg % eax;                                                                                                            \
  movl % eax, (% rcx)
#else
#if IS_IN(libc)
#define SYSCALL_ERROR_ERRNO __libc_errno
#else
#define SYSCALL_ERROR_ERRNO errno
#endif
#define SYSCALL_SET_ERRNO                                                                                               \
  movq SYSCALL_ERROR_ERRNO @GOTTPOFF(% rip), % rcx;                                                                     \
  neg % eax;                                                                                                            \
  movl % eax, % fs : (% rcx);
#endif

#ifndef PIC
#define SYSCALL_ERROR_HANDLER /* Nothing here; code in sysdep.S is used.  */
#else
#define SYSCALL_ERROR_HANDLER                                                                                           \
  0 : SYSCALL_SET_ERRNO;                                                                                                \
  or $ - 1, % RAX_LP;                                                                                                   \
  ret;
#endif /* PIC */

/* The Linux/x86-64 kernel expects the system call parameters in
   registers according to the following table:

    syscall number	rax
    arg 1		rdi
    arg 2		rsi
    arg 3		rdx
    arg 4		r10
    arg 5		r8
    arg 6		r9

    The Linux kernel uses and destroys internally these registers:
    return address from
    syscall		rcx
    eflags from syscall	r11

    Normal function call, including calls to the system call stub
    functions in the libc, get the first six parameters passed in
    registers and the seventh parameter and later on the stack.  The
    register use is as follows:

     system call number	in the DO_CALL macro
     arg 1		rdi
     arg 2		rsi
     arg 3		rdx
     arg 4		rcx
     arg 5		r8
     arg 6		r9

    We have to take care that the stack is aligned to 16 bytes.  When
    called the stack is not aligned since the return address has just
    been pushed.


    Syscalls of more than 6 arguments are not supported.  */

#undef DO_CALL
#define DO_CALL(syscall_name, args, ulong_arg_1, ulong_arg_2)                                                           \
  DOARGS_##args ZERO_EXTEND_##ulong_arg_1 ZERO_EXTEND_##ulong_arg_2 movl $SYS_ify(syscall_name), % eax;                 \
  syscall;

#define DOARGS_0 /* nothing */
#define DOARGS_1 /* nothing */
#define DOARGS_2 /* nothing */
#define DOARGS_3 /* nothing */
#define DOARGS_4 movq % rcx, % r10;
#define DOARGS_5 DOARGS_4
#define DOARGS_6 DOARGS_5

#define ZERO_EXTEND_0 /* nothing */
#define ZERO_EXTEND_1 /* nothing */
#define ZERO_EXTEND_2 /* nothing */
#define ZERO_EXTEND_3 /* nothing */
#define ZERO_EXTEND_4 /* nothing */
#define ZERO_EXTEND_5 /* nothing */
#define ZERO_EXTEND_6 /* nothing */

#else /* !__ASSEMBLER__ */

/* Registers clobbered by syscall.  */
#define REGISTERS_CLOBBERED_BY_SYSCALL "cc", "r11", "cx"

/* NB: This also works when X is an array.  For an array X,  type of
   (X) - (X) is ptrdiff_t, which is signed, since size of ptrdiff_t
   == size of pointer, cast is a NOP.   */
#define TYPEFY1(X) __typeof__((X) - (X))
/* Explicit cast the argument.  */
#define ARGIFY(X) ((TYPEFY1(X))(X))
/* Create a variable 'name' based on type of variable 'X' to avoid
   explicit types.  */
#define TYPEFY(X, name) __typeof__(ARGIFY(X)) name

#undef INTERNAL_SYSCALL
#define INTERNAL_SYSCALL(name, nr, args...) internal_syscall##nr(SYS_ify(name), args)

#undef INTERNAL_SYSCALL_NCS
#define INTERNAL_SYSCALL_NCS(number, nr, args...) internal_syscall##nr(number, args)

#undef internal_syscall0
#define internal_syscall0(number, dummy...)                                                                             \
  ({                                                                                                                    \
    unsigned long int resultvar;                                                                                        \
    asm volatile("syscall\n\t" : "=a"(resultvar) : "0"(number) : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);             \
    (long int)resultvar;                                                                                                \
  })

#undef internal_syscall1
#define internal_syscall1(number, arg1)                                                                                 \
  ({                                                                                                                    \
    unsigned long int resultvar;                                                                                        \
    TYPEFY(arg1, __arg1) = ARGIFY(arg1);                                                                                \
    register TYPEFY(arg1, _a1) asm("rdi") = __arg1;                                                                     \
    asm volatile("syscall\n\t" : "=a"(resultvar) : "0"(number), "r"(_a1) : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);   \
    (long int)resultvar;                                                                                                \
  })

#undef internal_syscall2
#define internal_syscall2(number, arg1, arg2)                                                                           \
  ({                                                                                                                    \
    unsigned long int resultvar;                                                                                        \
    TYPEFY(arg2, __arg2) = ARGIFY(arg2);                                                                                \
    TYPEFY(arg1, __arg1) = ARGIFY(arg1);                                                                                \
    register TYPEFY(arg2, _a2) asm("rsi") = __arg2;                                                                     \
    register TYPEFY(arg1, _a1) asm("rdi") = __arg1;                                                                     \
    asm volatile("syscall\n\t"                                                                                          \
                 : "=a"(resultvar)                                                                                      \
                 : "0"(number), "r"(_a1), "r"(_a2)                                                                      \
                 : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);                                                           \
    (long int)resultvar;                                                                                                \
  })

#undef internal_syscall3
#define internal_syscall3(number, arg1, arg2, arg3)                                                                     \
  ({                                                                                                                    \
    unsigned long int resultvar;                                                                                        \
    TYPEFY(arg3, __arg3) = ARGIFY(arg3);                                                                                \
    TYPEFY(arg2, __arg2) = ARGIFY(arg2);                                                                                \
    TYPEFY(arg1, __arg1) = ARGIFY(arg1);                                                                                \
    register TYPEFY(arg3, _a3) asm("rdx") = __arg3;                                                                     \
    register TYPEFY(arg2, _a2) asm("rsi") = __arg2;                                                                     \
    register TYPEFY(arg1, _a1) asm("rdi") = __arg1;                                                                     \
    asm volatile("syscall\n\t"                                                                                          \
                 : "=a"(resultvar)                                                                                      \
                 : "0"(number), "r"(_a1), "r"(_a2), "r"(_a3)                                                            \
                 : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);                                                           \
    (long int)resultvar;                                                                                                \
  })

#undef internal_syscall4
#define internal_syscall4(number, arg1, arg2, arg3, arg4)                                                               \
  ({                                                                                                                    \
    unsigned long int resultvar;                                                                                        \
    TYPEFY(arg4, __arg4) = ARGIFY(arg4);                                                                                \
    TYPEFY(arg3, __arg3) = ARGIFY(arg3);                                                                                \
    TYPEFY(arg2, __arg2) = ARGIFY(arg2);                                                                                \
    TYPEFY(arg1, __arg1) = ARGIFY(arg1);                                                                                \
    register TYPEFY(arg4, _a4) asm("r10") = __arg4;                                                                     \
    register TYPEFY(arg3, _a3) asm("rdx") = __arg3;                                                                     \
    register TYPEFY(arg2, _a2) asm("rsi") = __arg2;                                                                     \
    register TYPEFY(arg1, _a1) asm("rdi") = __arg1;                                                                     \
    asm volatile("syscall\n\t"                                                                                          \
                 : "=a"(resultvar)                                                                                      \
                 : "0"(number), "r"(_a1), "r"(_a2), "r"(_a3), "r"(_a4)                                                  \
                 : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);                                                           \
    (long int)resultvar;                                                                                                \
  })

#undef internal_syscall5
#define internal_syscall5(number, arg1, arg2, arg3, arg4, arg5)                                                         \
  ({                                                                                                                    \
    unsigned long int resultvar;                                                                                        \
    TYPEFY(arg5, __arg5) = ARGIFY(arg5);                                                                                \
    TYPEFY(arg4, __arg4) = ARGIFY(arg4);                                                                                \
    TYPEFY(arg3, __arg3) = ARGIFY(arg3);                                                                                \
    TYPEFY(arg2, __arg2) = ARGIFY(arg2);                                                                                \
    TYPEFY(arg1, __arg1) = ARGIFY(arg1);                                                                                \
    register TYPEFY(arg5, _a5) asm("r8") = __arg5;                                                                      \
    register TYPEFY(arg4, _a4) asm("r10") = __arg4;                                                                     \
    register TYPEFY(arg3, _a3) asm("rdx") = __arg3;                                                                     \
    register TYPEFY(arg2, _a2) asm("rsi") = __arg2;                                                                     \
    register TYPEFY(arg1, _a1) asm("rdi") = __arg1;                                                                     \
    asm volatile("syscall\n\t"                                                                                          \
                 : "=a"(resultvar)                                                                                      \
                 : "0"(number), "r"(_a1), "r"(_a2), "r"(_a3), "r"(_a4), "r"(_a5)                                        \
                 : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);                                                           \
    (long int)resultvar;                                                                                                \
  })

#undef internal_syscall6
#define internal_syscall6(number, arg1, arg2, arg3, arg4, arg5, arg6)                                                   \
  ({                                                                                                                    \
    unsigned long int resultvar;                                                                                        \
    TYPEFY(arg6, __arg6) = ARGIFY(arg6);                                                                                \
    TYPEFY(arg5, __arg5) = ARGIFY(arg5);                                                                                \
    TYPEFY(arg4, __arg4) = ARGIFY(arg4);                                                                                \
    TYPEFY(arg3, __arg3) = ARGIFY(arg3);                                                                                \
    TYPEFY(arg2, __arg2) = ARGIFY(arg2);                                                                                \
    TYPEFY(arg1, __arg1) = ARGIFY(arg1);                                                                                \
    register TYPEFY(arg6, _a6) asm("r9") = __arg6;                                                                      \
    register TYPEFY(arg5, _a5) asm("r8") = __arg5;                                                                      \
    register TYPEFY(arg4, _a4) asm("r10") = __arg4;                                                                     \
    register TYPEFY(arg3, _a3) asm("rdx") = __arg3;                                                                     \
    register TYPEFY(arg2, _a2) asm("rsi") = __arg2;                                                                     \
    register TYPEFY(arg1, _a1) asm("rdi") = __arg1;                                                                     \
    asm volatile("syscall\n\t"                                                                                          \
                 : "=a"(resultvar)                                                                                      \
                 : "0"(number), "r"(_a1), "r"(_a2), "r"(_a3), "r"(_a4), "r"(_a5), "r"(_a6)                              \
                 : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);                                                           \
    (long int)resultvar;                                                                                                \
  })

#define VDSO_NAME "LINUX_2.6"
#define VDSO_HASH 61765110

/* List of system calls which are supported as vsyscalls.  */
#define HAVE_CLOCK_GETTIME64_VSYSCALL "__vdso_clock_gettime"
#define HAVE_GETTIMEOFDAY_VSYSCALL "__vdso_gettimeofday"
#define HAVE_TIME_VSYSCALL "__vdso_time"
#define HAVE_GETCPU_VSYSCALL "__vdso_getcpu"
#define HAVE_CLOCK_GETRES64_VSYSCALL "__vdso_clock_getres"
#define HAVE_GETRANDOM_VSYSCALL "__vdso_getrandom"

#define HAVE_CLONE3_WRAPPER 1

#endif /* __ASSEMBLER__ */

#define __SYSCALL_CONCAT_X(a, b) a##b
#define __SYSCALL_CONCAT(a, b) __SYSCALL_CONCAT_X(a, b)

#define __INTERNAL_SYSCALL0(name) INTERNAL_SYSCALL(name, 0)
#define __INTERNAL_SYSCALL1(name, a1) INTERNAL_SYSCALL(name, 1, a1)
#define __INTERNAL_SYSCALL2(name, a1, a2) INTERNAL_SYSCALL(name, 2, a1, a2)
#define __INTERNAL_SYSCALL3(name, a1, a2, a3) INTERNAL_SYSCALL(name, 3, a1, a2, a3)
#define __INTERNAL_SYSCALL4(name, a1, a2, a3, a4) INTERNAL_SYSCALL(name, 4, a1, a2, a3, a4)
#define __INTERNAL_SYSCALL5(name, a1, a2, a3, a4, a5) INTERNAL_SYSCALL(name, 5, a1, a2, a3, a4, a5)
#define __INTERNAL_SYSCALL6(name, a1, a2, a3, a4, a5, a6) INTERNAL_SYSCALL(name, 6, a1, a2, a3, a4, a5, a6)
#define __INTERNAL_SYSCALL7(name, a1, a2, a3, a4, a5, a6, a7) INTERNAL_SYSCALL(name, 7, a1, a2, a3, a4, a5, a6, a7)

#define __INTERNAL_SYSCALL_NARGS_X(a, b, c, d, e, f, g, h, n, ...) n
#define __INTERNAL_SYSCALL_NARGS(...) __INTERNAL_SYSCALL_NARGS_X(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0, )
#define __INTERNAL_SYSCALL_DISP(b, ...) __SYSCALL_CONCAT(b, __INTERNAL_SYSCALL_NARGS(__VA_ARGS__))(__VA_ARGS__)

/* Issue a syscall defined by syscall number plus any other argument required.
   It is similar to INTERNAL_SYSCALL macro, but without the need to pass the
   expected argument number as second parameter.  */
#define INTERNAL_SYSCALL_CALL(...) __INTERNAL_SYSCALL_DISP(__INTERNAL_SYSCALL, __VA_ARGS__)

#define __INTERNAL_SYSCALL_NCS0(name) INTERNAL_SYSCALL_NCS(name, 0)
#define __INTERNAL_SYSCALL_NCS1(name, a1) INTERNAL_SYSCALL_NCS(name, 1, a1)
#define __INTERNAL_SYSCALL_NCS2(name, a1, a2) INTERNAL_SYSCALL_NCS(name, 2, a1, a2)
#define __INTERNAL_SYSCALL_NCS3(name, a1, a2, a3) INTERNAL_SYSCALL_NCS(name, 3, a1, a2, a3)
#define __INTERNAL_SYSCALL_NCS4(name, a1, a2, a3, a4) INTERNAL_SYSCALL_NCS(name, 4, a1, a2, a3, a4)
#define __INTERNAL_SYSCALL_NCS5(name, a1, a2, a3, a4, a5) INTERNAL_SYSCALL_NCS(name, 5, a1, a2, a3, a4, a5)
#define __INTERNAL_SYSCALL_NCS6(name, a1, a2, a3, a4, a5, a6) INTERNAL_SYSCALL_NCS(name, 6, a1, a2, a3, a4, a5, a6)
#define __INTERNAL_SYSCALL_NCS7(name, a1, a2, a3, a4, a5, a6, a7)                                                       \
  INTERNAL_SYSCALL_NCS(name, 7, a1, a2, a3, a4, a5, a6, a7)

#define INTERNAL_SYSCALL_NCS_CALL(...) __INTERNAL_SYSCALL_DISP(__INTERNAL_SYSCALL_NCS, __VA_ARGS__)

#define __INLINE_SYSCALL0(name) INLINE_SYSCALL(name, 0)
#define __INLINE_SYSCALL1(name, a1) INLINE_SYSCALL(name, 1, a1)
#define __INLINE_SYSCALL2(name, a1, a2) INLINE_SYSCALL(name, 2, a1, a2)
#define __INLINE_SYSCALL3(name, a1, a2, a3) INLINE_SYSCALL(name, 3, a1, a2, a3)
#define __INLINE_SYSCALL4(name, a1, a2, a3, a4) INLINE_SYSCALL(name, 4, a1, a2, a3, a4)
#define __INLINE_SYSCALL5(name, a1, a2, a3, a4, a5) INLINE_SYSCALL(name, 5, a1, a2, a3, a4, a5)
#define __INLINE_SYSCALL6(name, a1, a2, a3, a4, a5, a6) INLINE_SYSCALL(name, 6, a1, a2, a3, a4, a5, a6)
#define __INLINE_SYSCALL7(name, a1, a2, a3, a4, a5, a6, a7) INLINE_SYSCALL(name, 7, a1, a2, a3, a4, a5, a6, a7)

#define __INLINE_SYSCALL_NARGS_X(a, b, c, d, e, f, g, h, n, ...) n
#define __INLINE_SYSCALL_NARGS(...) __INLINE_SYSCALL_NARGS_X(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0, )
#define __INLINE_SYSCALL_DISP(b, ...) __SYSCALL_CONCAT(b, __INLINE_SYSCALL_NARGS(__VA_ARGS__))(__VA_ARGS__)

/* Issue a syscall defined by syscall number plus any other argument
   required.  Any error will be handled using arch defined macros and errno
   will be set accordingly.
   It is similar to INLINE_SYSCALL macro, but without the need to pass the
   expected argument number as second parameter.  */
#define INLINE_SYSCALL_CALL(...) __INLINE_SYSCALL_DISP(__INLINE_SYSCALL, __VA_ARGS__)

#define __INTERNAL_SYSCALL_NCS0(name) INTERNAL_SYSCALL_NCS(name, 0)
#define __INTERNAL_SYSCALL_NCS1(name, a1) INTERNAL_SYSCALL_NCS(name, 1, a1)
#define __INTERNAL_SYSCALL_NCS2(name, a1, a2) INTERNAL_SYSCALL_NCS(name, 2, a1, a2)
#define __INTERNAL_SYSCALL_NCS3(name, a1, a2, a3) INTERNAL_SYSCALL_NCS(name, 3, a1, a2, a3)
#define __INTERNAL_SYSCALL_NCS4(name, a1, a2, a3, a4) INTERNAL_SYSCALL_NCS(name, 4, a1, a2, a3, a4)
#define __INTERNAL_SYSCALL_NCS5(name, a1, a2, a3, a4, a5) INTERNAL_SYSCALL_NCS(name, 5, a1, a2, a3, a4, a5)
#define __INTERNAL_SYSCALL_NCS6(name, a1, a2, a3, a4, a5, a6) INTERNAL_SYSCALL_NCS(name, 6, a1, a2, a3, a4, a5, a6)
#define __INTERNAL_SYSCALL_NCS7(name, a1, a2, a3, a4, a5, a6, a7)                                                       \
  INTERNAL_SYSCALL_NCS(name, 7, a1, a2, a3, a4, a5, a6, a7)

/* Issue a syscall defined by syscall number plus any other argument required.
   It is similar to INTERNAL_SYSCALL_NCS macro, but without the need to pass
   the expected argument number as third parameter.  */
#define INTERNAL_SYSCALL_NCS_CALL(...) __INTERNAL_SYSCALL_DISP(__INTERNAL_SYSCALL_NCS, __VA_ARGS__)

namespace micron
{

long int
syscall(long int n, ...)
{
  va_list args;
  va_start(args, n);
  long int a0 = va_arg(args, long int);
  long int a1 = va_arg(args, long int);
  long int a2 = va_arg(args, long int);
  long int a3 = va_arg(args, long int);
  long int a4 = va_arg(args, long int);
  long int a5 = va_arg(args, long int);
  va_end(args);

  long int r = INTERNAL_SYSCALL_NCS_CALL(n, a0, a1, a2, a3, a4, a5);
  if ( r == -1 ) [[unlikely]] {
  }
  return r;
}
};

#endif
