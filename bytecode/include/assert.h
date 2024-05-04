#ifndef KN_ASSERT_H
#define KN_ASSERT_H

#include "decls.h"

#ifndef KN_NDEBUG
# define kn_assertm(cond, msg) do { if (!(cond)) abort(); } while(0)
#else
# define kn_assertm(cond, msg) do { (void) 0; } while(0)
#endif

#define kn_assert(cond) kn_assertm(cond, "assertion `" #cond "` failed.")

#endif
