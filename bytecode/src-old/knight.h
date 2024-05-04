#ifndef KN_KNIGHT_H
#define KN_KNIGHT_H

#include "value.h"
#include "env.h"
#include "shared.h"

/**
 * Begins the Knight interpreter.
 *
 * This function should be called before any other Knight function.
 **/
void KN_COLD kn_startup(void);

/**
 * Frees all memory related to the current running Knight process.
 *
 * This invalidates _all_ pointers that Knight functions returned (including all
 * `kn_value`s).
 *
 * After this function is run, `kn_startup` must be called again before calling
 * any other Knight functions.
 **/
void KN_COLD kn_shutdown(void);

/**
 * Executes the given stream as knight code.
 *
 * Note that any errors that may be caused during the execution of the code will
 * simply abort the program (like all exceptions in Knight do).
 **/
kn_value kn_play(struct kn_env *env, const char *source, size_t length);

#endif /* !KN_KNIGHT_H */
