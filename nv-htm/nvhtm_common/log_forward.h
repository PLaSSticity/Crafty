#ifndef LOG_FORWARD_H_GUARD
#define LOG_FORWARD_H_GUARD

#ifdef __cplusplus
extern "C"
{
  #endif

  /**
  * Applies one transactions into the checkpoint,
  * but does not move the log start pointers.
  *
  * When the log capacity is log, the buffered
  * cache lines are flushed.
  */
  int LOG_checkpoint_forward();

  // do not call this
  int LOG_checkpoint_forward_apply_one(int, int, void*);

  #ifdef __cplusplus
}
#endif

#endif /* end of include guard: LOG_FORWARD_H_GUARD */
