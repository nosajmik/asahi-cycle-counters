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

// Uses the ldnp - load pair of registers with
// nontemporal hint instruction. Nontemporal hint seems
// to have no effect on M1, as the data gets cached.
inline __attribute__((always_inline)) uint64_t load_nontemporal(void *ptr) {
    uint64_t trash1, trash2;
    asm volatile("ldnp %0, %1, [%2]" : "=r" (trash1), "=r" (trash2) : "r" (ptr) : "memory");
    return trash1 + trash2;
}

// Uses 24 MHz (42.67 ns) unprivileged timer.
inline __attribute__((always_inline)) uint64_t time_load_coarse(void *ptr) {
    uint64_t before, after;
    asm volatile("isb sy; mrs %0, CNTVCT_EL0; isb sy" : "=r" (before));
    *(volatile char *)ptr;
    asm volatile("dsb ish; isb sy; mrs %0, CNTVCT_EL0; isb sy" : "=r" (after));
    return after - before;
}

// Load register from flushed address, then multiply
// register by zero to clear. Then, add it to another
// register and load from the resulting address, which
// is also flushed. Returns the time for the entire routine.
inline __attribute__((always_inline)) uint64_t time_mulbyzero(void *ptr) {
    // Need register keyword to avoid ldr/str from and to stack.
    // Compile with Clang. gcc combines rax and rcx into x1.
    register uint64_t rax = 0, rcx = 0, before, after;
    volatile char c = 'A';
    register uint64_t second_ptr = (uint64_t)&c;
    register char trash;

    // Flush both. If loads execute serially, runtime should
    // be approx. miss latency * 2. If parallelized, runtime
    // is miss latency * 1.
    serialized_flush(ptr);
    serialized_flush((void *)second_ptr);

    asm volatile("isb sy; mrs %0, S3_2_c15_c0_0; isb sy" : "=r" (before));

    // Load from flushed address.
    rax = *(volatile char *)ptr;
    // rax *= 0; emits as str xzr, not mul.

    // Multiply-by-zero to clear.
    asm volatile("mul %0, %0, %1" : "=&r" (rax) : "r" (rcx));

    // Add to another reg, then load.
    rcx += rax;
    trash = *(volatile char *)(second_ptr + rcx);

    asm volatile("dsb ish; isb sy; mrs %0, S3_2_c15_c0_0; isb sy" : "=r" (after));
    
    return after - before;
}

// Load register from flushed address, then xor
// register by itself to clear. Then, add it to another
// register and load from the resulting address, which
// is also flushed. Returns the time for the entire routine.
inline __attribute__((always_inline)) uint64_t time_xor(void *ptr) {
    // Need register keyword to avoid ldr/str from and to stack.
    // Compile with Clang. gcc combines rax and rcx into x1.
    register uint64_t rax = 0, rcx = 0, before, after;
    volatile char c = 'A';
    register uint64_t second_ptr = (uint64_t)&c;
    register char trash;

    // Flush both. If loads execute serially, runtime should
    // be approx. miss latency * 2. If parallelized, runtime
    // is miss latency * 1.
    serialized_flush(ptr);
    serialized_flush((void *)second_ptr);

    asm volatile("isb sy; mrs %0, S3_2_c15_c0_0; isb sy" : "=r" (before));

    // Load from flushed address.
    rax = *(volatile char *)ptr;

    // Xor-with-itself to clear.
    asm volatile("eor %0, %0, %0" : "=&r" (rax));

    // Add to another reg, then load.
    rcx += rax;
    trash = *(volatile char *)(second_ptr + rcx);

    asm volatile("dsb ish; isb sy; mrs %0, S3_2_c15_c0_0; isb sy" : "=r" (after));

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

    // Test if the non-temporal pair load
    // instruction caches the data.
    uint64_t nontemporal[TRIALS];
    for (int i = 0; i < TRIALS; i++) {
        serialized_flush(ptr);
        load_nontemporal(ptr);
        nontemporal[i] = time_load(ptr);
    }

    // Test if register clear with xor vs. with mul 0
    // allows loads to be issued on parallel on the M1.
    // Currently, it looks like no difference.
    uint64_t mulbyzero[TRIALS];
    for (int i = 0; i < TRIALS; i++) {
        mulbyzero[i] = time_mulbyzero(ptr);
    }

    uint64_t xor[TRIALS];
    for (int i = 0; i < TRIALS; i++) {
        xor[i] = time_xor(ptr);
    }

    printf("Hits: ");
    for (int i = 0; i < TRIALS; i++) {
        printf("%lu ", hits[i]);
    }
    printf("\n\nMisses: ");
    for (int i = 0; i < TRIALS; i++) {
        printf("%lu ", misses[i]);
    }
    printf("\n\nNontemporal Loads: ");
    for (int i = 0; i < TRIALS; i++) {
        printf("%lu ", nontemporal[i]);
    }
    printf("\n\nMultiply by zero: ");
    for (int i = 0; i < TRIALS; i++) {
        printf("%lu ", mulbyzero[i]);
    }
    printf("\n\nXor with itself: ");
    for (int i = 0; i < TRIALS; i++) {
        printf("%lu ", xor[i]);
    }
    printf("\n");

    return 0;
}