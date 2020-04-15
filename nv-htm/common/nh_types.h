#ifndef NH_TYPES_H
#define NH_TYPES_H

#include "nh_defines.h"
#include "global_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct tx_counters_
{
	/* TODO */
	ts_s local_counter, global_counter;
} CL_ALIGN tx_counters_s;

#ifdef __cplusplus
}
#endif

// ##########################
// include from the solution 
#include "extra_types.h"
// ##########################

#endif /* NH_TYPES_H */
