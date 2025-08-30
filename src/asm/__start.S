    .globl _start

# this is here because we'll eventually want to bypass __libc_start_main entirely
    .text
_start:
    xor    %ebp, %ebp
    pop    %rsi
    mov    %rsp, %rdx
    and    $0xfffffffffffffff0, %rsp
    mov    %rsi, %rdi
    call   __micron_start     # call __micron_start which shall call into main
    mov    %rax, %rdi
    mov    $60, %rax       # exit
    syscall
