#include <stdio.h>
#include <stdint.h>

/*
    Prerequisites
    Sources: Stephan and https://github.com/AsahiLinux/docs/wiki/HW%3AARM-System-Registers

    From kernelspace:

    SYS_APL_HID4_EL1 bit 11 must be unset.
    ([11] Disable DC MVA Ops)
    Use m1-tools/kmod and m1-tools/tools/chickenctl.

    SYS_APL_PMCR0_EL1 bit 30 must be set.
    ([30] User-mode access to registers enable)
    Use m1-tools/kmod and m1-tools/tools/pmuctl.
*/

inline __attribute__((always_inline)) void serialized_flush(void *ptr) {
    asm volatile("dc civac, %0; isb sy; dsb ish" :: "r" (ptr));
}

inline __attribute__((always_inline)) void unserialized_flush(void *ptr) {
    asm volatile("dc civac, %0" :: "r" (ptr));
}

inline __attribute__((always_inline)) uint64_t rdtsc() {
    uint64_t timestamp;
    asm volatile("isb sy; mrs %0, S3_2_c15_c0_0" : "=r" (timestamp));
    return timestamp;
}

// Uses 24 MHz (42.67 ns) unprivileged timer.
inline __attribute__((always_inline)) uint64_t rdtsc_coarse() {
    uint64_t timestamp;
    asm volatile("isb sy; mrs %0, CNTVCT_EL0" : "=r" (timestamp));
    return timestamp;
}

inline __attribute__((always_inline)) uint64_t time_load(void *ptr) {
    uint64_t before, after;
    asm volatile("isb sy; mrs %0, S3_2_c15_c0_0; isb sy" : "=r" (before));
    *(volatile char *)ptr;
    asm volatile("dsb ish; isb sy; mrs %0, S3_2_c15_c0_0; isb sy" : "=r" (after));
    return after - before;
}

// Uses 24 MHz (42.67 ns) unprivileged timer.
inline __attribute__((always_inline)) uint64_t time_load_coarse(void *ptr) {
    uint64_t before, after;
    asm volatile("isb sy; mrs %0, CNTVCT_EL0; isb sy" : "=r" (before));
    *(volatile char *)ptr;
    asm volatile("dsb ish; isb sy; mrs %0, CNTVCT_EL0; isb sy" : "=r" (after));
    return after - before;
}

int main() {
    /*
        From userspace:
        Enable PMU #0 (FIXED_CYCLES).
        SYS_APL_PMCR0_EL1 bit 0 nust be set.
        ([7:0] Counter enable for PMC #7-0)
    */
    uint64_t PMCR0_EL1;
    asm volatile("mrs %0, S3_1_c15_c0_0" : "=r" (PMCR0_EL1));
    PMCR0_EL1 |= 1ULL << 0;
    asm volatile("msr S3_1_c15_c0_0, %0; isb sy" :: "r" (PMCR0_EL1));

    /*
        SYS_APL_PMCR1_EL1: bit 8 must be set
        to enable EL0 A64 counts on PMU #0.
        ([15:8] EL0 A64 enable PMC #0-7)
    */
    uint64_t PMCR1_EL1 = 1ULL << 8;
    asm volatile("msr S3_1_c15_c1_0, %0; isb sy" :: "r" (PMCR1_EL1));

    char dummy = 'A';
    char *ptr = &dummy;
    
    const int TRIALS = 100;

    uint64_t hits[TRIALS];
    for (int i = 0; i < TRIALS; i++) {
        *(volatile char *)ptr;
        hits[i] = time_load(ptr);
    }

    uint64_t misses[TRIALS];
    for (int i = 0; i < TRIALS; i++) {
        *(volatile char *)ptr;
        serialized_flush(ptr);
        misses[i] = time_load(ptr);
    }

    printf("Hits: ");
    for (int i = 0; i < TRIALS; i++) {
        printf("%llu ", hits[i]);
    }
    printf("\n\nMisses: ");
    for (int i = 0; i < TRIALS; i++) {
        printf("%llu ", misses[i]);
    }
    printf("\n");

    return 0;
}