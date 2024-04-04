#include "args.h"
#include "defs.h"
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

int running = 1;

int main(int argc, char *argv[]) {
  parse_arguments(argc, argv);
  display_config = (struct DisplayConfig *)malloc(sizeof(struct DisplayConfig));
  display_config->display = XOpenDisplay(NULL);

  signal(SIGUSR1, signal_handler);

  CARD16 standby, suspend, off;
  DPMSGetTimeouts(display_config->display, &standby, &suspend, &off);

  int timeout, interval, prefer_blanking, allow_exposures;
  XGetScreenSaver(display_config->display, &timeout, &interval,
                  &prefer_blanking, &allow_exposures);

  pthread_t screen_standby_thread;
  pthread_t screen_suspend_thread;
  pthread_t screen_off_thread;
  pthread_t screensaver_thread;
  pthread_t sleep_timeout_thread;

  pthread_create(&screen_standby_thread, NULL, screen_standby_loop, &standby);
  pthread_create(&screen_suspend_thread, NULL, screen_suspend_loop, &suspend);
  pthread_create(&screen_off_thread, NULL, screen_off_loop, &off);
  pthread_create(&screensaver_thread, NULL, screensaver_loop, &timeout);
  pthread_create(&sleep_timeout_thread, NULL, sleep_timeout_loop, NULL);

  pthread_join(screen_standby_thread, NULL);
  pthread_join(screen_suspend_thread, NULL);
  pthread_join(screen_off_thread, NULL);
  pthread_join(screensaver_thread, NULL);
  pthread_join(sleep_timeout_thread, NULL);
  XCloseDisplay(display_config->display);
  return 0;
}

void *screen_standby_loop(void *arg) {
  while (running) {
    while (retrieve_idle_time() < *((CARD16 *)arg) * 1000) {
      sleep(1);
    }
    CARD16 power_level;
    Bool state;
    DPMSInfo(display_config->display, &power_level, &state);
    if (!(state == DPMSModeOn))
      system("xset dpms force standby");
    sleep(1);
  }
  return NULL;
}

void *screen_suspend_loop(void *arg) {
  while (running) {
    while (retrieve_idle_time() < *((CARD16 *)arg) * 1000) {
      sleep(1);
    }
    CARD16 power_level;
    Bool state;
    DPMSInfo(display_config->display, &power_level, &state);
    if (!(state == DPMSModeOn))
      system("xset dpms force suspend");
    sleep(1);
  }
  return NULL;
}

void *screen_off_loop(void *arg) {
  while (running) {
    while (retrieve_idle_time() < *((CARD16 *)arg) * 1000) {
      sleep(1);
    }
    CARD16 power_level;
    Bool state;
    DPMSInfo(display_config->display, &power_level, &state);
    if (!(state == DPMSModeOn))
      system("xset dpms force off");
    sleep(1);
  }
  return NULL;
}

void *screensaver_loop(void *arg) {
  while (running) {
    while (retrieve_idle_time() < *((CARD16 *)arg) * 1000) {
      sleep(1);
    }
    int timeout, interval, prefer_blanking, allow_exposures;
    XGetScreenSaver(display_config->display, &timeout, &interval,
                    &prefer_blanking, &allow_exposures);
    if (!lockscreen_running && timeout != 0)
      lockscreen();
    while (lockscreen_running) {
      sleep(1);
    }
  }
  return NULL;
}

#define SUSPEND_TIMEOUT 600
void *sleep_timeout_loop(void *arg) {
  while (running) {
    while (retrieve_idle_time() < SUSPEND_TIMEOUT * 1000) {
      sleep(1);
    }
    CARD16 power_level;
    Bool state;
    DPMSInfo(display_config->display, &power_level, &state);
    while (!lockscreen_running)
      sleep(1); // Wait for lockscreen to start before suspending
    if (!is_player_running() && !(state == DPMSModeOn))
      system("systemctl suspend");
    while (lockscreen_running) {
      sleep(1);
    }
  }
  return NULL;
}

long retrieve_idle_time() {
  XScreenSaverInfo *ssi;
  ssi = XScreenSaverAllocInfo();
  XScreenSaverQueryInfo(display_config->display,
                        DefaultRootWindow(display_config->display), ssi);
  return ssi->idle;
}

// Signal handler function
void signal_handler(int signal) {
  if (!lockscreen_running)
    lockscreen();
}

int is_player_running() {
  char command[50] = "playerctl status";
  char buffer[128];
  int status = 0; // 0 for not running, 1 for running

  // Open a pipe to the command and read its output
  FILE *pipe = popen(command, "r");
  if (pipe == NULL) {
    fprintf(stderr, "Error executing command\n");
    return -1;
  }

  // Read the output of the command
  while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
    // Check if the status contains "Playing"
    if (strstr(buffer, "Playing") != NULL) {
      status = 1; // Player is running
      break;
    }
  }

  // Close the pipe
  pclose(pipe);

  return status;
}