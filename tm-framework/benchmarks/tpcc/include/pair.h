#ifndef TPCC_PAIR_H
#define TPCC_PAIR_H 1

#ifdef __cplusplus
extern "C" {
#endif


typedef struct pair {
    void* firstPtr;
    void* secondPtr;
} pair_t;


pair_t* pair_alloc (void* firstPtr, void* secondPtr);

void pair_free (pair_t* pairPtr);

void pair_swap (pair_t* pairPtr);

#define PPAIR_ALLOC(f,s)    Ppair_alloc(f, s)
#define PPAIR_FREE(p)       Ppair_free(p)

#define TMPAIR_ALLOC(f,s)   TMpair_alloc(TM_ARG  f, s)
#define TMPAIR_FREE(p)      TMpair_free(TM_ARG  p)


#ifdef __cplusplus
}
#endif


#endif
