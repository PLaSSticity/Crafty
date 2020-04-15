#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <immintrin.h>
#include <rtmintrin.h>

#ifndef NVM_LATENCY_NS
#define NVM_LATENCY_NS 300
#endif

#define rdtscp(void) ({ \
    register unsigned long long res; \
    __asm__ __volatile__ ( \
                "xor %%rax,%%rax \n\t" \
                "rdtscp          \n\t" \
                "shl $32,%%rdx   \n\t" \
                "or  %%rax,%%rdx \n\t" \
                "mov %%rdx,%0" \
                : "=r"(res) \
                : \
                : "rax", "rdx", "rcx"); \
    res; \
})
#define PAUSE() _mm_pause()

#define LOOPS 3000000
#define REPEAT 5
#define CYCLES (NVM_LATENCY_NS * 1e-9 * 3500000ULL / 1e-3)

#define SEC_TO_NSEC(sec) ((sec) * 1000000000)

int main() {
    struct timespec ts_begin;
    struct timespec ts_end;
    int j = 0;
    uint64_t cycles = CYCLES;
    for (; j < REPEAT; j++) {
        int i = 0;
        timespec_get(&ts_begin, TIME_UTC);
        for (; i < LOOPS; i++) {
            uint64_t ts_end = rdtscp() + cycles;
            while (ts_end > rdtscp()) PAUSE();
        }
        timespec_get(&ts_end, TIME_UTC);

        uint64_t nsec_taken = SEC_TO_NSEC(ts_end.tv_sec - ts_begin.tv_sec) + (ts_end.tv_nsec - ts_begin.tv_nsec);
        double nsec_per_loop = (double) nsec_taken / LOOPS;
//        printf("Off by %lf.0\n", nsec_per_loop - NVM_LATENCY_NS);
        cycles = 2 * cycles - (uint64_t) (cycles * (nsec_per_loop / NVM_LATENCY_NS));
    }
    printf("%li\n", cycles);
}