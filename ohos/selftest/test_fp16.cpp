// Minimal probe: does _Float16 conversion work under the OHOS clang toolchain?
// If this prints zeros, the compiler-rt fp16 helpers are broken -> kernel outputs 0.
#include <cstdio>
#include <cstdint>
#include <cstring>

int main() {
    // 1. fp16 -> float
    uint16_t raw = 0x2400;  // fp16 0.015625
    _Float16 h;
    memcpy(&h, &raw, 2);
    float f = (float)h;
    printf("fp16 0x2400 -> float %.10f (expect 0.015625)\n", f);

    // 2. float -> fp16
    float x = 0.015625f;
    _Float16 g = (_Float16)x;
    uint16_t out;
    memcpy(&out, &g, 2);
    printf("float 0.015625 -> fp16 0x%04x (expect 2400)\n", out);

    // 3. another float -> fp16
    float y = 123.5f;
    _Float16 g2 = (_Float16)y;
    memcpy(&out, &g2, 2);
    printf("float 123.5 -> fp16 0x%04x (expect 57b0)\n", out);

    // 4. fp16 arithmetic: sum of 16 values then scale (mirrors kernel pattern)
    _Float16 acc = (_Float16)0.0f;
    for (int i = 0; i < 16; i++) acc = (_Float16)((float)acc + 1.0f);
    printf("fp16 16x+1.0 = %.1f (expect 16.0)\n", (float)acc);
    _Float16 scaled = (_Float16)((float)acc * 0.015625f);
    printf("fp16 16*0.015625 = %.6f (expect 0.25)\n", (float)scaled);

    // 5. vector of 8 fp16 via array
    _Float16 arr[8];
    for (int i = 0; i < 8; i++) arr[i] = (_Float16)(i * 0.5f);
    float s = 0;
    for (int i = 0; i < 8; i++) s += (float)arr[i];
    printf("sum arr = %.1f (expect 14.0)\n", s);

    printf("FP16_PROBE_DONE\n");
    return 0;
}
