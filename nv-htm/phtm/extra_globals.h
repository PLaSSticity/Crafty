#ifndef EXTRA_GLOBALS_H
#define EXTRA_GLOBALS_H

#include "extra_types.h"

// ####################################################
// ### LOG VARIABLES ##################################
// ####################################################
// global
extern CL_ALIGN PHTM_marker_pool_s *phtm_markers;
// thread local
extern __thread CL_ALIGN PHTM_log_s *phtm_log;
// ####################################################

#endif /* EXTRA_GLOBALS_H */
