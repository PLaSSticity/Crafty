#ifndef HTM_ARCH_H_GUARD
#define HTM_ARCH_H_GUARD

// Headers
#if defined(__powerpc__)
#include <htmxlintrin.h>
#elif defined(__x86_64__)
#include <immintrin.h>
#include <rtmintrin.h>
#else
#error "Platform not suported"
#endif /* Platform */

// Defining Cache Line Size
#ifndef HTM_CLSIZE
#if defined(__powerpc__)
#define HTM_CLSIZE 128
#elif defined(__x86_64__)
#define HTM_CLSIZE 64
#endif
#endif

#define HTM_CL_ALIGN __attribute__((aligned (HTM_CLSIZE)))

// Defining HTM errors
typedef enum {
HTM_SUCCESS = 0,
HTM_FALLBACK,
HTM_ABORT,
HTM_CONFLICT,
HTM_CAPACITY,
HTM_EXPLICIT,
HTM_ADDRESS,
HTM_INTERRUPT,
HTM_NESTING_DEPTH,
HTM_OTHER,
HTM_PERSISTENT
} HTM_errors_e;
#define HTM_NB_ERRORS 11

// HTM primitives
#if defined(__powerpc__)
	#define HTM_STATUS_TYPE       TM_buff_type
	#define HTM_CODE_SUCCESS      _HTM_TBEGIN_STARTED
	#define HTM_begin(var)        (__TM_begin(var))
	#define HTM_abort()           __TM_abort()
	#define HTM_named_abort(code) __TM_named_abort(code)
	#define HTM_test()            __builtin_tcheck()
	#define HTM_commit()          __TM_end()
	#define HTM_is_named(status)  __TM_is_named_user_abort (status, NULL);
	#define HTM_get_named(status)	({ \
		unsigned char code; \
		__TM_is_named_user_abort (status, &code); \
		code; \
	})
	#define HTM_ERROR_TO_INDEX(tm_buffer) ({ \
	  HTM_errors_e code = HTM_OTHER; \
	  if(*_TEXASRL_PTR (tm_buffer) == 0)             code = HTM_SUCCESS; \
	  else if(__TM_is_conflict(tm_buffer))           code = HTM_CONFLICT; \
	  else if(__TM_is_illegal  (tm_buffer))          code = HTM_ADDRESS; \
	  else if(__TM_is_footprint_exceeded(tm_buffer)) code = HTM_CAPACITY; \
	  else if(__TM_nesting_depth(tm_buffer))         code = HTM_HTM_NESTING_DEPTH; \
	  else if(__TM_is_nested_too_deep(tm_buffer))    code = HTM_NESTING_DEPTH; \
	  else if(__TM_is_user_abort(tm_buffer))         code = HTM_EXPLICIT; \
	  else if(__TM_is_failure_persistent(tm_buffer)) code = HTM_PERSISTENT; \
	  code; \
	})
#elif defined(__x86_64__)
#define HTM_CLSIZE 64
	#define HTM_STATUS_TYPE       register int
	#define HTM_CODE_SUCCESS      _XBEGIN_STARTED
	#define HTM_begin(var)        (var = _xbegin())
	#define HTM_abort()           _xabort(0)
	#define HTM_named_abort(code)	_xabort(code)
	#define HTM_test()            _xtest()
	#define HTM_commit()          _xend()
	#define HTM_is_named(status)  (status & 1)
	#define HTM_get_named(status) (status >> 24)
	#define HTM_ERROR_TO_INDEX(status) ({ \
		HTM_errors_e code = HTM_OTHER; \
	  if (status == _XBEGIN_STARTED) { \
			code == HTM_SUCCESS; \
	  } else { \
			if (status & _XABORT_CONFLICT) { \
				code = HTM_CONFLICT; \
			} else if (status & _XABORT_CAPACITY) { \
				code = HTM_CAPACITY; \
			} else if (status & _XABORT_EXPLICIT) { \
				code = HTM_EXPLICIT; \
			} \
	  } \
		code; \
	})
#endif /* Platform */
#define HTM_ERROR_INC(status, error_array) ({ \
  HTM_errors_e code = ERROR_TO_INDEX(status); \
  if (code != HTM_SUCCESS && code != HTM_FALLBACK)
		error_array[HTM_ABORT] += 1; \
  error_array[code] += 1; \
})

#endif /* HTM_ARCH_H_GUARD */
