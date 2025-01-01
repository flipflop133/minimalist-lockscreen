/**
 * @file main.c
 * @brief Entry point for the minimalist lockscreen application.
 */

#include "args.h"
#include "lockscreen.h"
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/dpmsconst.h>
#include <X11/extensions/scrnsaver.h>
#include <pthread.h>
#include <signal.h>
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
volatile int running = 1;
XScreenSaverInfo *ssi = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Application entry point.
 *
 * @param argc Number of command-line arguments.
 * @param argv List of command-line arguments.
 * @return Zero on success, non-zero otherwise.
 */
int main(int argc, char *argv[]) {
  /* Allocate XScreenSaverInfo struct and parse command-line arguments. */
  ssi = XScreenSaverAllocInfo();
  parse_arguments(argc, argv);

  /* Allocate and initialize DisplayConfig. */
  display_config = (struct DisplayConfig *)malloc(sizeof(struct DisplayConfig));
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

  /* Set up signal handlers. */
  signal(SIGINT, main_cleanup);
  signal(SIGTERM, main_cleanup);
  signal(SIGUSR1, lockscreen_handler);

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

  /* Wait for threads to end before exiting. */
  pthread_join(screensaver_info_thread, NULL);
  pthread_join(screensaver_thread, NULL);
  pthread_join(sleep_timeout_thread, NULL);

  /* Clean up shared resources. */
  pthread_mutex_destroy(&mutex);
  XFree(ssi);
  XCloseDisplay(display_config->display);
  free(display_config);

  return 0;
}

/**
 * @brief Thread function that checks inactivity and triggers the lock screen.
 *
 * @param arg Expected to be a pointer to an integer containing the timeout (in
 * seconds).
 * @return Always returns NULL.
 */
static void *screensaver_loop(void *arg) {
  int timeout = *((int *)arg);

  while (running) {
    BOOL dpms_enabled;
    CARD16 power_level;
    DPMSInfo(display_config->display, &power_level, &dpms_enabled);

    /* Wait until idle time exceeds 'timeout' or DPMS is not 'On'. */
    while (
        (ssi->idle < (CARD16)(timeout * 1000) || dpms_enabled == DPMSModeOn) &&
        running) {
      DPMSInfo(display_config->display, &power_level, &dpms_enabled);
      sleep(1);
    }
    if (!running) {
      break;
    }

    /* If the lockscreen is not active, invoke it. */
    if (!lockscreen_running) {
      lockscreen();
    }

    /* Wait until lockscreen stops running. */
    while (lockscreen_running && running) {
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
  while (running) {
    char *suspend_str = retrieve_command_arg("--suspend");
    if (!suspend_str) {
      /* No suspend argument specified; just pause the thread. */
      sleep(1);
      continue;
    }

    int suspend_sec = atoi(suspend_str);
    while ((int)ssi->idle < (suspend_sec * 1000) && running) {
      sleep(1);
    }
    if (!running) {
      break;
    }

    /* Wait for lockscreen to activate before suspending. */
    BOOL state;
    CARD16 power_level;
    DPMSInfo(display_config->display, &power_level, &state);
    while (!lockscreen_running && running) {
      sleep(1);
    }

    /* If no media player is running, suspend the system. */
    if (!is_player_running() && running) {
      system("systemctl suspend");
    }

    /* Wait until lockscreen is deactivated again. */
    while (lockscreen_running && running) {
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
  while (running) {
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
static void main_cleanup(int signal __attribute__((unused))) { running = 0; }

/**
 * @brief Signal handler to trigger the lockscreen on SIGUSR1.
 *
 * @param signal The signal number (unused).
 */
static void lockscreen_handler(int signal __attribute__((unused))) {
  if (!lockscreen_running) {
    lockscreen();
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
