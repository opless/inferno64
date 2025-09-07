// wild guess

.text
.p2align 2

// void FPsave(void *buf)
// buf layout (8 bytes):
//   [0..3] = FPCR (uint32)
//   [4..7] = FPSR (uint32)
.globl _FPsave
_FPsave:
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp

    // x0 = buf
    mrs     x1, fpcr               // FP control register (low 32 bits used)
    mrs     x2, fpsr               // FP status register (low 32 bits used)
    str     w1, [x0]               // store FPCR (32-bit)
    str     w2, [x0, #4]           // store FPSR (32-bit)

    ldp     x29, x30, [sp], #16
    ret

// void FPrestore(void *buf)
// expects the same 8-byte layout produced by FPsave
.globl _FPrestore
_FPrestore:
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp

    // x0 = buf
    ldr     w1, [x0]               // load FPCR (32-bit)
    ldr     w2, [x0, #4]           // load FPSR (32-bit)
    msr     fpcr, x1               // restore FPCR (low 32 bits used)
    msr     fpsr, x2               // restore FPSR (low 32 bits used)

    ldp     x29, x30, [sp], #16
    ret