// arm64_cpu_probe.c — ARM64 CPU feature probe for the OHOS port (run this FIRST on a new device).
//
// Answers the only question that matters before shipping the T-MAC binary to a board:
//   does this CPU have NEON + fp16 SIMD (ASIMDHP)?  (i8mm/bf16/SVE only matter for other builds)
//
// Build (baseline+fp16, same as the shipped llama-cli):
//   clang --target=aarch64-linux-ohos -O2 -static -march=armv8.2a+fp16 arm64_cpu_probe.c -o arm64_cpu_probe
// The fp16 test runs in a forked child, so a CPU without fp16 reports "SIGILL" instead of killing the probe.
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/auxv.h>
#include <sys/wait.h>
#include <signal.h>
#include <arm_neon.h>

#ifndef AT_HWCAP2
#define AT_HWCAP2 26
#endif

// The musl sysroot already ships bits/hwcap.h with the HWCAP_* defines; guard ours so the
// system ones win where present (HWCAP2_* is usually missing there).
#ifndef HWCAP_FP
#define HWCAP_FP        (1ul << 0)
#endif
#ifndef HWCAP_ASIMD
#define HWCAP_ASIMD     (1ul << 1)
#endif
#ifndef HWCAP_ATOMICS
#define HWCAP_ATOMICS   (1ul << 8)
#endif
#ifndef HWCAP_FPHP
#define HWCAP_FPHP      (1ul << 9)
#endif
#ifndef HWCAP_ASIMDHP
#define HWCAP_ASIMDHP   (1ul << 10)
#endif
#ifndef HWCAP_ASIMDDP
#define HWCAP_ASIMDDP   (1ul << 20)
#endif
#ifndef HWCAP_SVE
#define HWCAP_SVE       (1ul << 22)
#endif
#ifndef HWCAP_ASIMDFHM
#define HWCAP_ASIMDFHM  (1ul << 23)
#endif
#ifndef HWCAP2_SVE2
#define HWCAP2_SVE2     (1ul << 1)
#endif
#ifndef HWCAP2_I8MM
#define HWCAP2_I8MM     (1ul << 13)
#endif
#ifndef HWCAP2_BF16
#define HWCAP2_BF16     (1ul << 14)
#endif

struct feat { const char *name; unsigned long bit; int cap2; };

static const struct feat feats[] = {
    { "NEON / ASIMD            (all armv8-a)",        HWCAP_ASIMD,   0 },
    { "FP scalar               (all armv8-a)",        HWCAP_FP,      0 },
    { "FP16 scalar    (FPHP)",                        HWCAP_FPHP,    0 },
    { "FP16 SIMD      (ASIMDHP)  <-- T-MAC needs",    HWCAP_ASIMDHP, 0 },
    { "FP16 mul-add   (FHM)",                         HWCAP_ASIMDFHM,0 },
    { "dotprod        (ASIMDDP)",                     HWCAP_ASIMDDP, 0 },
    { "i8mm                       <-- needs v8.6+ build", HWCAP2_I8MM, 1 },
    { "bf16",                                         HWCAP2_BF16,   1 },
    { "LSE atomics",                                  HWCAP_ATOMICS, 0 },
    { "SVE",                                          HWCAP_SVE,     0 },
    { "SVE2",                                         HWCAP2_SVE2,   1 },
};

static int fp16_smoke(void) {
    float16x8_t a = vdupq_n_f16((float16_t) 1.5f);
    float16x8_t b = vdupq_n_f16((float16_t) 2.0f);
    float16x8_t c = vaddq_f16(a, b);
    float r = (float) vgetq_lane_f16(c, 0);
    if (r < 3.4f || r > 3.6f) { printf("  fp16 result wrong: %.3f\n", r); return 2; }
    return 0;
}

int main(void) {
    struct utsname u;
    unsigned long h1 = getauxval(AT_HWCAP);
    unsigned long h2 = getauxval(AT_HWCAP2);

    printf("=== arm64 CPU feature probe (OHOS port) ===\n");
    if (!uname(&u)) printf("os:    %s %s %s\n", u.sysname, u.release, u.machine);
    else            printf("os:    (uname failed)\n");

    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f) {
        char line[512];
        int shown = 0;
        while (fgets(line, sizeof line, f) && shown < 4) {
            if (!strncmp(line, "model name", 10) || !strncmp(line, "Processor", 9) ||
                !strncmp(line, "Hardware", 8) || !strncmp(line, "CPU implementer", 15) ||
                !strncmp(line, "CPU part", 8)) {
                printf("cpuinfo: %s", line);
                shown++;
            }
        }
        fclose(f);
        if (!shown) printf("cpuinfo: (no model lines)\n");
    } else {
        printf("cpuinfo: (not readable)\n");
    }

    printf("HWCAP  = 0x%016lx\nHWCAP2 = 0x%016lx\n\n", h1, h2);
    for (size_t i = 0; i < sizeof(feats) / sizeof(feats[0]); i++) {
        unsigned long v = feats[i].cap2 ? h2 : h1;
        printf("  %-48s %s\n", feats[i].name, (v & feats[i].bit) ? "YES" : "no");
    }

    int has_neon = (h1 & HWCAP_ASIMD) != 0;
    int has_fp16 = (h1 & HWCAP_ASIMDHP) != 0;

    printf("\n=== fp16 NEON smoke test (forked child) ===\n");
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) { _exit(fp16_smoke()); }
    int st = 0;
    if (pid < 0 || waitpid(pid, &st, 0) < 0) {
        printf("  fork failed -> SKIP\n");
    } else if (WIFSIGNALED(st)) {
        printf("  child killed by signal %d (%s) -> fp16 NOT usable\n",
               WTERMSIG(st), WTERMSIG(st) == SIGILL ? "SIGILL" : "other");
        has_fp16 = 0;
    } else if (WIFEXITED(st)) {
        printf("  child exit=%d -> %s\n", WEXITSTATUS(st),
               WEXITSTATUS(st) == 0 ? "PASS (1.5 + 2.0 == 3.5)" : "FAIL");
        if (WEXITSTATUS(st) != 0) has_fp16 = 0;
    }

    printf("\n=== verdict ===\n");
    printf("shipped T-MAC arm64 binary requires: NEON=%s  fp16-SIMD=%s\n",
           has_neon ? "OK" : "MISSING", has_fp16 ? "OK" : "MISSING");
    if (has_neon && has_fp16) {
        printf("=> this device can run the armv8.2a+fp16 build.\n");
        return 0;
    }
    printf("=> CANNOT run the armv8.2a+fp16 build on this device.\n");
    return 1;
}
