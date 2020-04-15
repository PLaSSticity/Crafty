#ifndef TPCC_TYPES_H
#define TPCC_TYPES_H 1


#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long ulong_t;

enum {
    FALSE = 0,
    TRUE  = 1
};

typedef long bool_t;

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

#ifdef __cplusplus
}
#endif


#endif /* TYPES_H */
