#include <stdio.h>
#include <stdint.h>

int main() {
    char c = 'A';
    char *ptr = &c;

    // Test dc civac
    asm volatile("dc civac, %0; isb sy; dsb ish" :: "r" (ptr));

    // Test cycle counters
    uint64_t value;
    asm volatile("isb; mrs %0, S3_2_c15_c2_0" : "=r" (value));
    printf("Cycles: %lu\n", value);

    asm volatile("isb; mrs %0, S3_2_c15_c1_0" : "=r" (value));
    printf("Cycles: %lu\n", value);

    asm volatile("isb; mrs %0, S3_2_c15_c0_0" : "=r" (value));
    printf("Cycles: %lu\n", value);

    asm volatile("isb; mrs %0, S3_2_c15_c3_0" : "=r" (value));
    printf("Cycles: %lu\n", value);
}
