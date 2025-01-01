#ifndef PAM_H
#define PAM_H

/**
 * @file pam.h
 * @brief Declarations for PAM authentication functions.
 */

/* ------------------------------------------------------------------------- */
/* Function Declarations                                                     */
/* ------------------------------------------------------------------------- */

int auth_pam(const char *password, const char *username);

#endif /* PAM_H */
