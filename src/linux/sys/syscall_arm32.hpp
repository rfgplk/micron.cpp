#pragma once

// bare minimum va_list code. only included here for syscall compatibility

#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, l) __builtin_va_arg(v, l)
#define va_copy(d, s) __builtin_va_copy(d, s)
#define __va_copy(d, s) __builtin_va_copy(d, s)

typedef __builtin_va_list va_list;

#ifndef va_end
#define va_end(ap) va_end(ap)
#endif

namespace micron
{

    inline long int
    syscall(long int n, ...) noexcept
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

        register long int r0 asm("r0") = a0;
        register long int r1 asm("r1") = a1;
        register long int r2 asm("r2") = a2;
        register long int r3 asm("r3") = a3;
        register long int r4 asm("r4") = a4;
        register long int r5 asm("r5") = a5;
        register long int r7 asm("r7") = n;

        asm volatile(
            "svc 0"
            : "+r"(r0) // return value in r0
            : "r"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5), "r"(r7)
            : "memory"
        );

        if (r0 < 0) [[unlikely]] {
        }

        return r0;
    }
