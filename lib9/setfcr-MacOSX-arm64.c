// fpuctl_mac_arm64.c — clang inline asm version
#include <stdint.h>

uint32_t getfcr(void) {
    unsigned long v;
    __asm__ volatile("mrs %0, fpcr" : "=r"(v));
    return (uint32_t)v;
}

void setfcr(uint32_t fcr) {
    unsigned long v = fcr;
    __asm__ volatile("msr fpcr, %0" :: "r"(v));
}

uint32_t getfsr(void) {
    unsigned long v;
    __asm__ volatile("mrs %0, fpsr" : "=r"(v));
    return (uint32_t)v;
}

void setfsr(uint32_t fsr) {
    unsigned long v = fsr;
    __asm__ volatile("msr fpsr, %0" :: "r"(v));
}