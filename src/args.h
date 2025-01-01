#ifndef ARGS_H
#define ARGS_H

/**
 * @file args.h
 * @brief Provides declarations for parsing and retrieving command-line arguments.
 */

/* ------------------------------------------------------------------------- */
/* Structure Definitions                                                     */
/* ------------------------------------------------------------------------- */

/**
 * @brief Represents a single command-line argument.
 */
struct Argument {
    struct Argument *next; /**< Pointer to the next argument in the list. */
    char *name;            /**< The name of the argument (e.g., "--image"). */
    void *value;           /**< The value associated with the argument. */
};

/* ------------------------------------------------------------------------- */
/* Global Variable Declarations                                              */
/* ------------------------------------------------------------------------- */

/**
 * @brief Head of the linked list storing parsed command-line arguments.
 */
extern struct Argument *argument_head;

/**
 * @brief Total number of command-line arguments.
 */
extern int args_count;

/* ------------------------------------------------------------------------- */
/* Function Declarations                                                     */
/* ------------------------------------------------------------------------- */

void parse_arguments(int argc, char *argv[]);
char *retrieve_command_arg(const char *arg);

#endif /* ARGS_H */
