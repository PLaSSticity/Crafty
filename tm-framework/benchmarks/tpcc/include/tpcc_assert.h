#ifndef ASSERT_123_H_GUARD
#define ASSERT_123_H_GUARD

// #ifdef __cplusplus
// #include <cassert>
// #else
#include <assert.h>
// #endif /* __cplusplus */

// Wraps the standard assert macro to avoids "unused variable" warnings when compiled away.
// Inspired by: http://powerof2games.com/node/10
// This is not the "default" because it does not conform to the requirements of the C standard,
// which requires that the NDEBUG version be ((void) 0).
#ifdef NDEBUG
#define ASSERT(x) do { (void)sizeof(x); } while(0)
#else
// #define ASSERT(x) if (!(x)) printf(__FILE__ ":" __LINE__ " error %s\n", #x);
#define ASSERT(x) assert(x)
#endif

#endif /* ASSERT_H_GUARD */
