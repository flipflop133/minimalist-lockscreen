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
int cafeine = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int main(int argc, char *argv[]) {
  parse_arguments(argc, argv);
  display_config = (struct DisplayConfig *)malloc(sizeof(struct DisplayConfig));
  display_config->display = XOpenDisplay(NULL);

  signal(SIGINT, main_cleanup);
  signal(SIGUSR1, signal_handler);
  signal(SIGUSR2, cafeine_handler);

  int timeout, interval, prefer_blanking, allow_exposures;
  XGetScreenSaver(display_config->display, &timeout, &interval,
                  &prefer_blanking, &allow_exposures);

  pthread_t screensaver_thread;
  pthread_t sleep_timeout_thread;

  pthread_create(&screensaver_thread, NULL, screensaver_loop, &timeout);
  pthread_create(&sleep_timeout_thread, NULL, sleep_timeout_loop, NULL);

  pthread_join(screensaver_thread, NULL);
  pthread_join(sleep_timeout_thread, NULL);
  pthread_mutex_destroy(&mutex);
  XCloseDisplay(display_config->display);
  return 0;
}

void *screensaver_loop(void *arg) {
  while (running) {
    while (retrieve_idle_time() < *((CARD16 *)arg) * 1000) {
      if (!running)
      {
        break;
      }
      sleep(1);
    }
    if (!running)
    {
      break;
    }
    int timeout, interval, prefer_blanking, allow_exposures;
    XGetScreenSaver(display_config->display, &timeout, &interval,
                    &prefer_blanking, &allow_exposures);
    if (!lockscreen_running && timeout != 0)
      lockscreen();
    while (lockscreen_running) {
      sleep(1);
    }
    sleep(1);
  }
  return NULL;
}

#define SUSPEND_TIMEOUT 600
void *sleep_timeout_loop(void *arg) {
  while (running) {
    while ((retrieve_idle_time() < SUSPEND_TIMEOUT * 1000) && !cafeine)
    {
      if (!running)
      {
        break;
      }
      sleep(1);
    }
    if (!running)
    {
      break;
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
  pthread_mutex_lock(&mutex);
  unsigned long idle = 0;
  XScreenSaverInfo *ssi;
  ssi = XScreenSaverAllocInfo();
  XScreenSaverQueryInfo(display_config->display,
                        DefaultRootWindow(display_config->display), ssi);
  idle = ssi->idle;
  XFree(ssi);
  pthread_mutex_unlock(&mutex);
  return idle;
}

void main_cleanup(int signal)
{
  running = 0;
}

// Signal handler function
void signal_handler(int signal) {
  if (!lockscreen_running)
    lockscreen();
}

void cafeine_handler(int signal)
{
  if (cafeine == 0)
  {
    cafeine = 1;
  }
  else
  {
    cafeine = 0;
  }
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