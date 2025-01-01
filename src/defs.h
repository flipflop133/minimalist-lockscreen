#ifndef DEFS_H
#define DEFS_H

#include <pthread.h>

/**
 * @file defs.h
 * @brief Contains declarations for screensaver-related thread functions,
 *        signal handlers, and utility checks.
 */

/* ------------------------------------------------------------------------- */
/* Global Variables                                                          */
/* ------------------------------------------------------------------------- */

/**
 * @brief Mutex used for thread-safety in certain shared resources.
 */
extern pthread_mutex_t mutex;

#endif /* DEFS_H */
