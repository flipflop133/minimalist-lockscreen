#include "args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Argument *argument_head;
int args_count;

char *retrieve_command_arg(char *arg) {
  struct Argument *current = argument_head;
  while (current != NULL) {
    if (strcmp(current->name, arg) == 0) {
      return current->value;
    }
    current = current->next;
  }
  return NULL;
}

void parse_arguments(int argc, char *argv[]) {
  args_count = argc;
  struct Argument *current = NULL;
  struct Argument *previous = NULL;
  int first = 1;
  for (int i = 0; i < argc; i++) {
    current = (struct Argument *)malloc(sizeof(struct Argument));
    current->name = NULL;
    current->value = NULL;
    current->next = NULL;

    current->name = malloc(strlen(argv[i]) + 1);
    strcpy(current->name, argv[i]);
    if (strcmp(argv[i], "--image") == 0) {
      current->value = malloc(strlen(argv[++i]) + 1);
      strcpy(current->value, argv[i]);
    } else if (strcmp(argv[i], "--suspend") == 0) {
      current->value = malloc(strlen(argv[++i]) + 1);
      strcpy(current->value, argv[i]);
    }
    if (first) {
      first = 0;
      argument_head = current;
    } else {
      previous->next = current;
    }
    previous = current;
  }
}