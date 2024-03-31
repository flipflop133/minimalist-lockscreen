#ifndef PAM_H
#define PAM_H

struct pam_message;
struct pam_response;
int auth_pam(const char *password, const char *username);
int converse(int num_msg, const struct pam_message **msg,
             struct pam_response **resp, void *appdata_ptr);

#endif