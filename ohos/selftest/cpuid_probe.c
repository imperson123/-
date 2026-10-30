// CPUID feature probe for the HarmonyOS emulator virtual CPU.
#include <stdio.h>

int main(void) {
    unsigned int eax, ebx, ecx, edx;

    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0), "c"(0));
    printf("max_leaf=%u vendor=%.4s%.4s%.4s\n", eax, (char*)&ebx, (char*)&edx, (char*)&ecx);

    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1), "c"(0));
    printf("SSE3=%d SSSE3=%d SSE4.1=%d SSE4.2=%d AVX=%d FMA=%d F16C=%d\n",
           (ecx >> 0) & 1, (ecx >> 9) & 1, (ecx >> 19) & 1, (ecx >> 20) & 1,
           (ecx >> 28) & 1, (ecx >> 12) & 1, (ecx >> 29) & 1);

    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    printf("AVX2=%d AVX512F=%d AVX512DQ=%d AVX512BW=%d AVX512VL=%d\n",
           (ebx >> 5) & 1, (ebx >> 16) & 1, (ebx >> 17) & 1, (ebx >> 30) & 1, (ebx >> 31) & 1);
    return 0;
}
