#ifndef ARGS_H
#define ARGS_H
extern void parse_arguments(int argc, char *argv[]);
extern char *retrieve_command_arg(char *arg);
// Command line arguments are stored here
extern struct Argument *argument_head;
extern int args_count;
struct Argument {
  struct Argument *next;
  char *name;
  void *value;
};
#endif