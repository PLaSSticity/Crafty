#include "extra_globals.h"

// ####################################################
// ### LOG VARIABLES ##################################
// ####################################################
// global
CL_ALIGN NVLog_s **NH_global_logs;
void* LOG_global_ptr;
int is_sigsegv = 0;
// thread local
__thread CL_ALIGN NVLog_s *nvm_htm_local_log;
__thread CL_ALIGN int LOG_nb_wraps;
__thread CL_ALIGN NVLogLocal_s LOG_local_state;
// ####################################################
