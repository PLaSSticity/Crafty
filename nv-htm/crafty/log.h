#ifndef CRAFTY_H
#define CRAFTY_H

#include "nh.h"

#ifdef __cplusplus
extern "C"
{
#endif

// VSCode uses Clang/LLVM and _xtest() isn't defined for some reason, but it works fine when building with GCC via the Makefiles
#ifdef __clang__
#define _xtest() (assert(0), 0)
#endif

//Crafty_log_s *crafty_create_undo_log();  // DO NOT CALL
void crafty_thr_init(int tid);

void crafty_after_begin_any_transaction();


#ifdef __cplusplus
}
#endif

#endif /* CRAFTY_H */
