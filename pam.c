#include "pam.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int auth_pam(const char *password, const char *username) {
  const char *service_name = "login"; // PAM service name

  struct pam_conv conv = {converse, (void *)password};
  pam_handle_t *pamh = NULL;
  int ret;

  // Start PAM session
  ret = pam_start(service_name, username, &conv, &pamh);
  if (ret != PAM_SUCCESS) {
    fprintf(stderr, "pam_start failed: %s\n", pam_strerror(pamh, ret));
    return 1;
  }

  // Authenticate user
  ret = pam_authenticate(pamh, 0);
  if (ret != PAM_SUCCESS) {
    fprintf(stderr, "Authentication failed: %s\n", pam_strerror(pamh, ret));
  } else {
    printf("Authentication successful!\n");
  }

  // End PAM session
  pam_end(pamh, ret);

  return ret == PAM_SUCCESS ? 0 : 1;
}
// PAM conversation function
int converse(int num_msg, const struct pam_message **msg,
             struct pam_response **resp, void *appdata_ptr) {
  const char *password = (const char *)appdata_ptr;
  *resp = (struct pam_response *)malloc(num_msg * sizeof(struct pam_response));
  if (*resp == NULL) {
    return PAM_BUF_ERR;
  }

  for (int i = 0; i < num_msg; ++i) {
    const struct pam_message *m = msg[i];
    struct pam_response *r = &(*resp)[i];

    switch (m->msg_style) {
    case PAM_PROMPT_ECHO_OFF:
      printf("%s", m->msg);
      fflush(stdout);
      r->resp = strdup(password);
      break;
    case PAM_TEXT_INFO:
      printf("%s\n", m->msg);
      break;
    case PAM_ERROR_MSG:
      fprintf(stderr, "Error: %s\n", m->msg);
      break;
    default:
      free(*resp);
      return PAM_CONV_ERR;
    }
  }

  return PAM_SUCCESS;
}