/*
 * Optimised matrix multiplication by blocks.
 * It is written in x86-64 assembly with AVX2 instructions.
 *
 * Author: Sydney Hauke
 */

.equiv BLOCK_SIZE, 32
.equiv UNROLL, 8

.text
.global do_block

# Arguments:
# double *A, double *B, double *C, u32 n, u32 x, u32 y
do_block:

# Prologue
#    push %rbp
#    movq %rsp, %rbp

    push %r12
    push %r13
    push %r14
    push %r15

# Arguments are passed through up to 6 registers.
# In order : %rdi, %rsi, %rdx, %rcx, %r8, %r9

# We treat packs of 4 double precision floating points

    shrq $2, %rcx

    imulq $BLOCK_SIZE, %r9 # line loop indexer
    movq %r9, %r10
    addq $BLOCK_SIZE, %r10 # line loop upper bound index

__line_loop:

    movq %rcx, %r11   # element loop indexer
    subq $4, %r11

__element_loop:

    movq %r8, %rax
    imulq $BLOCK_SIZE, %rax # unroll loop indexer
    movq %rax, %r12
    addq $BLOCK_SIZE, %r12 # unroll loop upper bound index

__unroll_loop:

    movq %r9, %r13
    imulq %rcx, %r13
    addq %rax, %r13  # C array indexer
    imulq $4, %r13

    movq %r9, %r14
    imulq %rcx, %r14
    addq %r11, %r14  # A array indexer
    imulq $4, %r14

    movq %r11, %r15
    imulq %rcx, %r15
    addq %rax, %r15  # B array indexer
    imulq $4, %r15

# actual computation

    vmovapd (%rdi, %r14, 8), %ymm0
    vmovapd (%rsi, %r15, 8), %ymm1
    vmovapd 32(%rdi, %r14, 8), %ymm2
    vmovapd 32(%rsi, %r15, 8), %ymm3

    # vfmadd231pd %ymm0, %ymm1, (%rdx, %r13, 8)    
    # vfmadd231pd %ymm2, %ymm3, 32(%rdx, %r13, 8)

    addq $UNROLL, %rax
    cmpq %rax, %r12
    jne __unroll_loop

    decq %r11
    cmpq $0, %r11
    jne __element_loop

    incq %r9
    cmpq %r9, %r10
    jne __line_loop

# Epilogue
#   leave

    pop %r15
    pop %r14
    pop %r13
    pop %r12

    ret
