#include "args.h"
#include "defs.h"
#include "lockscreen.h"
#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TODO -> read these from args or config file
#define SCREEN_TIMEOUT 60
#define SLEEP_TIMEOUT 600

int running = 1;

int main(int argc, char *argv[]) {
  parse_arguments(argc, argv);
  display_config = (struct DisplayConfig *)malloc(sizeof(struct DisplayConfig));
  display_config->display = XOpenDisplay(NULL);

  signal(SIGUSR1, signal_handler);

  pthread_t screen_timeout_thread;
  pthread_t sleep_timeout_thread;
  // Create a thread to handle screen timeout
  pthread_create(&screen_timeout_thread, NULL, screen_timeout_loop, NULL);

  // Create a thread to handle sleep timeout
  pthread_create(&sleep_timeout_thread, NULL, sleep_timeout_loop, NULL);

  pthread_join(screen_timeout_thread, NULL);
  pthread_join(sleep_timeout_thread, NULL);
  XCloseDisplay(display_config->display);
  return 0;
}

void *screen_timeout_loop(void *arg) {
  while (running) {
    while (retrieve_idle_time() < SCREEN_TIMEOUT * 1000) {
      sleep(1);
    }
    system("xset dpms force off");
    if (!lockscreen_running)
      lockscreen();
    while (lockscreen_running) {
      sleep(1);
    }
  }
  return NULL;
}

void *sleep_timeout_loop(void *arg) {
  while (running) {
    while (retrieve_idle_time() < SLEEP_TIMEOUT * 1000) {
      sleep(1);
    }
    if (!is_player_running())
      system("systemctl suspend");
    if (!lockscreen_running)
      lockscreen();
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