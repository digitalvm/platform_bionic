/* Generated by gensyscalls.py. Do not edit. */

#include <private/bionic_asm.h>

ENTRY(__unchecked_pwrite64)
    movq    %rcx, %r10
    movl    $__NR_pwrite64, %eax
    syscall
    cmpq    $-MAX_ERRNO, %rax
    jb      1f
    negl    %eax
    movl    %eax, %edi
    call    __set_errno_internal
1:
    ret
END(__unchecked_pwrite64)

ALIAS_SYMBOL(__unchecked_pwrite, __unchecked_pwrite64)
.hidden __unchecked_pwrite64
