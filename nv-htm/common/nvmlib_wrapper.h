#ifndef NVMLIB_WRAPPER_H
#define NVMLIB_WRAPPER_H

#include <min_nvm.h>

//#include <libpmem.h>
//#include <libvmem.h>

#ifdef __cplusplus
extern "C"
{
#endif
    
    // TODO: add the minimal library


    // if VOL then the path must be a directory

#ifndef ALLOC_FN
#define ALLOC_FN(ptr, type, size) \
	ptr = (type*) malloc(size * sizeof(type))
#endif
	
#define ALLOC_VMEM(objtype, objref, file_name, sizeobj) ({ \
    size_t mapped_len = sizeobj; \
    int is_pmem; \
    VMEM *vmp; \
    if ((vmp = vmem_create("./", VMEM_MIN_POOL)) == NULL) { /* TODO: */ \
                perror("vmem_create"); \
                exit(EXIT_FAILURE); \
        } \
    if ((objref = (objtype*) \
                vmem_malloc(vmp, sizeobj)) == NULL) { \
                perror("vmem_malloc"); \
                exit(EXIT_FAILURE); \
        } \
    memset(objref, 0, sizeobj); \
    mapped_len; \
})

#define ALLOC_PMEM(objtype, objref, file_name, sizeobj) ({ \
    size_t mapped_len; \
    int is_pmem; \
    if (((objref) = (objtype*) \
                pmem_map_file(file_name, sizeobj, \
                              PMEM_FILE_CREATE, 0666, &mapped_len, \
                              &is_pmem)) == NULL) { \
                perror("pmem_map_file"); \
                exit(EXIT_FAILURE); \
        } \
    mapped_len; \
})

// Paolo idea
// #include <asm/cachectl.h> // cacheflush(char *addr, int nbytes, int cache);
#define NVM_FLUSH_RANGE(start, end) __builtin___clear_cache(start, end)

#ifdef USE_MIN_NVM

#define ALLOC_MEM(file, size)  MN_alloc(file, size)
#define FREE_MEM(ptr, size)    MN_free(ptr)

// in order to simulate PHTM as well
//#ifdef DISABLE_FLUSH
#define NVM_PERSIST(ptr, size) SPIN_PER_WRITE(MAX(size / CACHE_LINE_SIZE, 1))
#define NVM_FLUSH(ptr, size)   SPIN_PER_WRITE(MAX(size / CACHE_LINE_SIZE, 1))
#define NVM_DRAIN()            
//#else /* DISABLE_FLUSH */
//#define NVM_PERSIST(ptr, size) MN_flush(ptr, size); MN_drain()
//#define NVM_FLUSH(ptr, size)   MN_flush(ptr, size)
//#define NVM_DRAIN()            MN_drain()
//#endif /* DISABLE_FLUSH */

#else /* USE_MIN_NVM */
#if USE_VOL == 1
#define ALLOC_MEM(file, size) ({ \
    void *res; \
    ALLOC_VMEM(void, res, file, size); \
    res; \
})
#else /* USE_VOL */
#define ALLOC_MEM(file, size) ({ \
    void *res; \
    ALLOC_PMEM(void, res, file, size); \
    res; \
})
#endif /* USE_VOL */

#define FREE_MEM(ptr, size)    pmem_unmap(ptr, size)

#ifdef DISABLE_FLUSH
#define NVM_PERSIST(ptr, size) SPIN_PER_WRITE(MAX(size / CACHE_LINE_SIZE, 1))
#define NVM_FLUSH(ptr, size)   SPIN_PER_WRITE(MAX(size / CACHE_LINE_SIZE, 1))
#define NVM_DRAIN()            
#else /* DISABLE_FLUSH */
#define NVM_PERSIST(ptr, size) pmem_persist(ptr, size)
#define NVM_FLUSH(ptr, size)   pmem_flush(ptr, size)
#define NVM_DRAIN()            pmem_drain()
#endif /* DISABLE_FLUSH */

#endif /* USE_MIN_NVM */

#ifdef __cplusplus
}
#endif

#endif /* NVMLIB_WRAPPER_H */

