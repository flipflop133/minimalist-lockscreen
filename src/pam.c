/**
 * @file pam.c
 * @brief Authentication functionality using PAM (Pluggable Authentication
 * Modules).
 */

#include "pam.h"
#include <security/_pam_types.h>
#include <security/pam_appl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declaration of the conversation function. */
static int converse(int num_msg, const struct pam_message **msg,
                    struct pam_response **resp, void *appdata_ptr);

/**
 * @brief Authenticates a user via PAM.
 *
 * @param password The user's password.
 * @param username The user's username.
 * @return 0 on successful authentication, 1 on failure.
 */
int auth_pam(const char *password, const char *username) {
  const char *service_name = "login"; /* PAM service name. */

  struct pam_conv conv = {.conv = converse, .appdata_ptr = (void *)password};
  pam_handle_t *pamh = NULL;
  int ret;

  /* Start a PAM session. */
  ret = pam_start(service_name, username, &conv, &pamh);
  if (ret != PAM_SUCCESS) {
    fprintf(stderr, "pam_start failed: %s\n", pam_strerror(pamh, ret));
    return 1;
  }

  /* Attempt to authenticate the user. */
  ret = pam_authenticate(pamh, 0);
  if (ret == PAM_SUCCESS) {
    printf("Authentication successful!\n");
  } else {
    fprintf(stderr, "Authentication failed: %s\n", pam_strerror(pamh, ret));
  }

  /* End the PAM session. */
  pam_end(pamh, ret);

  return (ret == PAM_SUCCESS) ? 0 : 1;
}

/**
 * @brief Conducts the conversation between the PAM module and the application.
 *
 * This function responds to the module's prompts with the provided password,
 * and prints or logs any informational or error messages.
 *
 * @param num_msg The number of messages in the conversation.
 * @param msg An array of pointers to PAM message structures.
 * @param resp A pointer to an array of PAM response structures.
 * @param appdata_ptr Custom data provided by the application (the password).
 * @return PAM_SUCCESS on success, or an appropriate error code otherwise.
 */
static int converse(int num_msg, const struct pam_message **msg,
                    struct pam_response **resp, void *appdata_ptr) {
  const char *password = (const char *)appdata_ptr;
  *resp = (struct pam_response *)malloc(num_msg * sizeof(struct pam_response));
  if (*resp == NULL) {
    return PAM_BUF_ERR;
  }

  for (int i = 0; i < num_msg; ++i) {
    const struct pam_message *m = msg[i];
    struct pam_response *r = &(*resp)[i];
    r->resp_retcode = 0;
    r->resp = NULL;

    switch (m->msg_style) {
    case PAM_PROMPT_ECHO_OFF:
      /* Provide the password without echo. */
      printf("%s", m->msg);
      fflush(stdout);
      r->resp = strdup(password);
      break;

    case PAM_TEXT_INFO:
      /* Display informational messages. */
      printf("%s\n", m->msg);
      break;

    case PAM_ERROR_MSG:
      /* Display error messages. */
      fprintf(stderr, "Error: %s\n", m->msg);
      break;

    default:
      free(*resp);
      *resp = NULL;
      return PAM_CONV_ERR;
    }
  }

  return PAM_SUCCESS;
}
