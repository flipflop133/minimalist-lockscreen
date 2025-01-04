/**
 * @file args.c
 * @brief Parses command-line arguments and provides a way to retrieve them.
 */

#include "args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global variables to store the head of the argument list and argument count.
 */
struct Argument *g_argument_head = NULL;
int g_args_count = 0;

/**
 * @brief Retrieves the value for a given argument name.
 *
 * @param arg The argument name to look for (e.g., "--image").
 * @return A pointer to the value of the argument, or NULL if not found.
 */
char *retrieve_command_arg(const char *arg) {
  struct Argument *current = g_argument_head;

  while (current != NULL) {
    if (strcmp(current->name, arg) == 0) {
      return current->value;
    }
    current = current->next;
  }
  return NULL;
}

/**
 * @brief Parses the command-line arguments and stores them in a linked list.
 *
 * This function looks for specific arguments such as "--image" or "--suspend"
 * and expects a following value for each of these flags. If a matching flag is
 * found, the next argument is assigned as its value.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line argument strings.
 */
void parse_arguments(int argc, char *argv[]) {
  g_args_count = argc;

  struct Argument *current_arg = NULL;
  struct Argument *previous_arg = NULL;
  int is_first = 1;

  for (int i = 1; i < argc; i++) {
    /* Allocate and initialize a new Argument node. */
    current_arg = (struct Argument *)malloc(sizeof(struct Argument));
    if (current_arg == NULL) {
      perror("Failed to allocate memory for Argument");
      exit(EXIT_FAILURE);
    }
    current_arg->name = NULL;
    current_arg->value = NULL;
    current_arg->next = NULL;

    /* Copy the flag (e.g., "--image"). */
    current_arg->name = malloc(strlen(argv[i]) + 1);
    if (current_arg->name == NULL) {
      perror("Failed to allocate memory for argument name");
      free(current_arg);
      exit(EXIT_FAILURE);
    }
    strcpy(current_arg->name, argv[i]);

    /* Check for flags that require a value. */
    if ((strcmp(argv[i], "--image") == 0) ||
        (strcmp(argv[i], "--suspend") == 0) ||
        (strcmp(argv[i], "--color") == 0)) {
      if (i + 1 < argc) {
        /* Allocate and copy the next argument as the value. */
        current_arg->value = malloc(strlen(argv[i + 1]) + 1);
        if (current_arg->value == NULL) {
          perror("Failed to allocate memory for argument value");
          free(current_arg->name);
          free(current_arg);
          exit(EXIT_FAILURE);
        }
        strcpy(current_arg->value, argv[++i]);
      } else {
        fprintf(stderr, "Warning: Expected a value after '%s' flag.\n",
                argv[i]);
      }
    }

    /* Link the new node into the list. */
    if (is_first) {
      g_argument_head = current_arg;
      is_first = 0;
    } else {
      previous_arg->next = current_arg;
    }
    previous_arg = current_arg;
  }
}
