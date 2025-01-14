/**
 * @file main.c
 * @brief Entry point for the minimalist lockscreen application.
 */

#include "args.h"
#include "graphics/graphics.h"
#include "lockscreen.h"
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/dpmsconst.h>
#include <X11/extensions/scrnsaver.h>
#include <cairo/cairo.h>
#include <fcntl.h>
#include <fontconfig/fontconfig.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/* ------------------------------------------------------------------------- */
/* Forward Declarations                                                      */
/* ------------------------------------------------------------------------- */
static void *screensaver_loop(void *arg);
static void *sleep_timeout_loop(void *arg);
static void *update_xscreensaver_info_loop(void *arg);
static void main_cleanup(int signal);
static void lockscreen_handler(int signal);
static int is_player_running(void);

/* Global or shared variables. */
int lock_screen = 0;
atomic_int running = 1;
XScreenSaverInfo *ssi = NULL;
struct DisplayConfig *display_config = NULL;
int lockscreen_pipe_fd[2];
/**
 * @brief Application entry point.
 *
 * @param argc Number of command-line arguments.
 * @param argv List of command-line arguments.
 * @return Zero on success, non-zero otherwise.
 */
int main(int argc, char *argv[]) {
  /*  Daemonize the process. */
#ifndef DEBUG
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(EXIT_FAILURE);
  }
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  if (setsid() < 0) {
    perror("setsid");
    exit(EXIT_FAILURE);
  }
#endif

  /* Allocate XScreenSaverInfo struct and parse command-line arguments. */
  ssi = XScreenSaverAllocInfo();
  parse_arguments(argc, argv);

  /* Allocate and initialize DisplayConfig. */
  display_config =
      (struct DisplayConfig *)calloc(1, sizeof(struct DisplayConfig));
  if (!display_config) {
    fprintf(stderr, "Failed to allocate display_config.\n");
    return EXIT_FAILURE;
  }

  display_config->display = XOpenDisplay(NULL);
  if (display_config->display == NULL) {
    fprintf(stderr, "Failed to open X display.\n");
    free(display_config);
    return EXIT_FAILURE;
  }

  initialize_windows();
  initialize_graphics();

  /* Set up signal handler for SIGUSR1 */
  struct sigaction sa;
  sa.sa_handler = lockscreen_handler;
  sa.sa_flags = SA_RESTART; // Ensure interrupted system calls restart
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGUSR1, &sa, NULL) == -1) {
    perror("sigaction SIGUSR1");
    exit(EXIT_FAILURE);
  }

  /* Set up signal handler for SIGINT, SIGTERM, SIGABRT */
  sa.sa_handler = main_cleanup;
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction SIGINT");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGTERM, &sa, NULL) == -1) {
    perror("sigaction SIGTERM");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGABRT, &sa, NULL) == -1) {
    perror("sigaction SIGABRT");
    exit(EXIT_FAILURE);
  }

  /* Query initial screensaver info. */
  XScreenSaverQueryInfo(display_config->display,
                        DefaultRootWindow(display_config->display), ssi);

  /* Get current screen saver parameters. */
  int timeout, interval, prefer_blanking, allow_exposures;
  XGetScreenSaver(display_config->display, &timeout, &interval,
                  &prefer_blanking, &allow_exposures);

  /* Create threads for screensaver logic. */
  pthread_t screensaver_info_thread;
  pthread_t screensaver_thread;
  pthread_t sleep_timeout_thread;

  pthread_create(&screensaver_info_thread, NULL, update_xscreensaver_info_loop,
                 NULL);
  pthread_create(&screensaver_thread, NULL, screensaver_loop, &timeout);
  pthread_create(&sleep_timeout_thread, NULL, sleep_timeout_loop, NULL);

  if (pipe(lockscreen_pipe_fd) == -1) {
    perror("Error creating pipe");
    exit(EXIT_FAILURE);
  }
  char buffer;
  while (atomic_load(&running)) {
    ssize_t bytes_read = read(lockscreen_pipe_fd[0], &buffer, 1);
    if (bytes_read > 0) {
      lockscreen();
    }
  }
  close(lockscreen_pipe_fd[0]);
  close(lockscreen_pipe_fd[1]);
  /* Wait for threads to end before exiting. */
  pthread_join(screensaver_info_thread, NULL);
  pthread_join(screensaver_thread, NULL);
  pthread_join(sleep_timeout_thread, NULL);

  /* Clean up shared resources. */
  exit_cleanup();

  if (display_config->screen_info) {
    XFree(display_config->screen_info);
    display_config->screen_info = NULL;
  }

  if (ssi) {
    XFree(ssi);
    ssi = NULL;
  }

  XSync(display_config->display, False);

  /* Force X to destroy all client resources associated with this process: */
  XSetCloseDownMode(display_config->display, DestroyAll);

  XCloseDisplay(display_config->display);

  /* Finalize Fontconfig to release leftover patterns and caches. */
  FcFini();

  /* Force Cairo to drop any static data it might be holding. */
  cairo_debug_reset_static_data();

  free(display_config);

  return 0;
}

/**
 * @brief Triggers the lockscreen by writing a character to the pipe.
 *
 * This function writes the character 'x' to the write end of the pipe
 * specified by the global variable `pipe_fd`. This action is intended
 * to signal the lockscreen mechanism to activate.
 */
static void trigger_lockscreen() { write(lockscreen_pipe_fd[1], "x", 1); }

/**
 * @brief Thread function that checks inactivity and triggers the lock screen.
 *
 * @param arg Expected to be a pointer to an integer containing the timeout (in
 * seconds).
 * @return Always returns NULL.
 */
static void *screensaver_loop(void *arg) {
  int timeout = *((int *)arg);

  while (atomic_load(&running)) {
    BOOL dpms_enabled;
    CARD16 power_level;
    DPMSInfo(display_config->display, &power_level, &dpms_enabled);

    /* Wait until idle time exceeds 'timeout' or DPMS is not 'On'. */
    while (((int)ssi->idle < (timeout * 1000) || dpms_enabled == DPMSModeOn) &&
           atomic_load(&running)) {
      DPMSInfo(display_config->display, &power_level, &dpms_enabled);
      sleep(1);
    }
    if (!atomic_load(&running)) {
      break;
    }

    /* If the lockscreen is not active, invoke it. */
    if (!atomic_load(&lockscreen_running)) {
      trigger_lockscreen();
    }

    /* Wait until lockscreen stops running. */
    while (atomic_load(&lockscreen_running) && atomic_load(&running)) {
      sleep(1);
    }
    sleep(1);
  }

  return NULL;
}

/**
 * @brief Thread function to manage a user-defined suspend timeout.
 *
 * @param arg Unused parameter.
 * @return Always returns NULL.
 */
static void *sleep_timeout_loop(void *arg __attribute__((unused))) {
  while (atomic_load(&running)) {
    char *suspend_str = retrieve_command_arg("--suspend");
    if (!suspend_str) {
      sleep(1);
      continue;
    }

    int suspend_sec = atoi(suspend_str);
    BOOL dpms_enabled;
    CARD16 power_level;
    DPMSInfo(display_config->display, &power_level, &dpms_enabled);
    // Wait until idle time exceeds 'suspend_sec' or DPMS is not 'On' or a media
    // player is running.
    while (((int)ssi->idle < (suspend_sec * 1000) ||
            dpms_enabled == DPMSModeOn || is_player_running()) &&
           atomic_load(&running)) {
      sleep(1);
    }
    if (!atomic_load(&running)) {
      break;
    }

    /* Wait for lockscreen to activate before suspending. */
    while (!atomic_load(&lockscreen_running) && atomic_load(&running)) {
      sleep(1);
    }

    if (atomic_load(&running)) {
      system("systemctl suspend");
    }

    while (atomic_load(&lockscreen_running) && atomic_load(&running)) {
      sleep(1);
    }
  }
  return NULL;
}

/**
 * @brief Thread function to update XScreenSaver info periodically.
 *
 * @param arg Unused parameter.
 * @return Always returns NULL.
 */
static void *update_xscreensaver_info_loop(void *arg __attribute__((unused))) {
  while (atomic_load(&running)) {
    XScreenSaverQueryInfo(display_config->display,
                          DefaultRootWindow(display_config->display), ssi);
    sleep(1);
  }
  return NULL;
}

/**
 * @brief Cleans up when a termination signal is received.
 *
 * @param signal The signal number (unused).
 */
static void main_cleanup(int signal __attribute__((unused))) {
  atomic_store(&running, 0);
}

/**
 * @brief Signal handler to trigger the lockscreen on SIGUSR1.
 *
 * @param signal The signal number (unused).
 */
static void lockscreen_handler(int signal __attribute__((unused))) {
  if (!atomic_load(&lockscreen_running)) {
    trigger_lockscreen();
  }
}

/**
 * @brief Checks if a media player (via playerctl) is currently in 'Playing'
 * state.
 *
 * @return 1 if a player is running and playing, 0 if not, -1 on error.
 */
static int is_player_running(void) {
  const char *command = "playerctl status";
  char buffer[128] = {0};
  int status = 0; /* 0 for not playing, 1 if playing */

  FILE *pipe = popen(command, "r");
  if (!pipe) {
    fprintf(stderr, "Error executing playerctl command.\n");
    return -1;
  }

  /* Read the output and look for "Playing". */
  while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
    if (strstr(buffer, "Playing") != NULL) {
      status = 1;
      break;
    }
  }
  pclose(pipe);

  return status;
}
