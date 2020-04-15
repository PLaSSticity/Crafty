#ifndef LOG_BACKWARD_H_GUARD
#define LOG_BACKWARD_H_GUARD

  /**
   * Checks if any log is at the filtering threshold,
   * if so applies from that point.
   */
	int LOG_checkpoint_backward();

  // called internally
  int LOG_checkpoint_backward_apply_one();


#endif /* end of include guard: LOG_BACKWARD_H_GUARD */
