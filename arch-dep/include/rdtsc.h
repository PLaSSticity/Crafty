#ifndef RDTSC_H_GUARD
#define RDTSC_H_GUARD

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(__i386__)

static __inline__ unsigned long long rdtsc(void) {
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}
#elif defined(__x86_64__)

static __inline__ uint64_t rdtsc(void) {
    register unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (uint64_t) lo) | (((uint64_t) hi) << 32);
}

// Another option for rdtscp:
/*static __inline__ uint64_t rdtscp(void) {
        register unsigned hi, lo, dummy;
    __asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi), "=c"(dummy));
    return ( (uint64_t) lo) | (((uint64_t) hi) << 32);
}*/

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


#elif defined(__powerpc__)

static __inline__ unsigned long long rdtsc(void) {
    unsigned long long int result = 0;
    unsigned long int upper, lower, tmp;
    __asm__ volatile(
            "0:                  \n"
            "\tmftbu   %0           \n"
            "\tmftb    %1           \n"
            "\tmftbu   %2           \n"
            "\tcmpw    %2,%0        \n"
            "\tbne     0b         \n"
            : "=r"(upper), "=r"(lower), "=r"(tmp)
            );
    result = upper;
    result = result << 32;
    result = result | lower;

    return (result);
}

#else

#error "No tick counter is available!"

#endif

/*  $RCSfile:  $   $Author: kazutomo $
 *  $Revision: 1.6 $  $Date: 2005/04/13 18:49:58 $
 */

#ifdef __cplusplus
}
#endif

#endif /* RDTSC_H_GUARD */

