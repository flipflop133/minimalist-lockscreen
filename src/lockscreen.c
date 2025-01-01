/**
 * @file lockscreen.c
 * @brief Implements the main lockscreen logic, including initialization,
 *        keyboard capture, and resource cleanup.
 */

#include "lockscreen.h"
#include "graphics/graphics.h"
#include "graphics/modules/date.h"
#include "pam.h"
#include "utils.h"
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xinerama.h>
#include <cairo/cairo.h>
#include <ctype.h>
#include <pthread.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Global (extern) variables declared in lockscreen.h or elsewhere. */
struct DisplayConfig *display_config = NULL;
struct ScreenConfig
    screen_configs[128]; /* TODO: Use dynamic array if needed. */
struct passwd *pw = NULL;
char current_input[128] = {0};
int current_input_index = 0;
int password_is_wrong = 0;
int lockscreen_running = 0;
pthread_t date_thread;
Window root_window;

/**
 * @brief Initializes the X11 windows for the lockscreen.
 */
static void initialize_windows(void) {
  if (display_config->display == NULL) {
    fprintf(stderr, "Cannot open display.\n");
    exit(EXIT_FAILURE);
  }

  /* Query the screen info for multi-monitor support (Xinerama). */
  display_config->screen_info = XineramaQueryScreens(
      display_config->display, &display_config->num_screens);

  if (display_config->screen_info == NULL) {
    fprintf(stderr, "Failed to query Xinerama screens.\n");
    exit(EXIT_FAILURE);
  }

  /* Convert the chosen background color to a pixel value. */
  unsigned long background_color =
      hex_color_to_pixel("#000000", DefaultScreen(display_config->display));

  /* Get the root window. */
  root_window = RootWindow(display_config->display,
                           DefaultScreen(display_config->display));

  /* Create a fullscreen window on each screen. */
  for (int i = 0; i < display_config->num_screens; i++) {
    screen_configs[i].window = XCreateSimpleWindow(
        display_config->display, root_window,
        display_config->screen_info[i].x_org, 0,
        display_config->screen_info[i].width,
        display_config->screen_info[i].height, 0, 0, background_color);

    XStoreName(display_config->display, screen_configs[i].window,
               "minimalist_lockscreen");
    XSelectInput(display_config->display, screen_configs[i].window,
                 ExposureMask | KeyPressMask);

    /* Make the window appear fullscreen. */
    Atom net_wm_state =
        XInternAtom(display_config->display, "_NET_WM_STATE", False);
    Atom net_wm_fullscreen =
        XInternAtom(display_config->display, "_NET_WM_STATE_FULLSCREEN", False);

    XChangeProperty(display_config->display, screen_configs[i].window,
                    net_wm_state, XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)&net_wm_fullscreen, 1);

    /* Map the window (show it). */
    XMapWindow(display_config->display, screen_configs[i].window);
  }
}

/**
 * @brief Cleans up resources before the application exits.
 */
static void cleanUp(void) {
  /* Show the cursor again and ungrab the keyboard. */
  XFixesShowCursor(display_config->display, root_window);
  XUngrabKeyboard(display_config->display, CurrentTime);

  /* Destroy Cairo objects and X11 windows. */
  for (int screen_num = 0; screen_num < display_config->num_screens;
       screen_num++) {
    if (screen_configs[screen_num].pattern) {
      cairo_pattern_destroy(screen_configs[screen_num].pattern);
      screen_configs[screen_num].pattern = NULL;
    }

    if (screen_configs[screen_num].overlay_buffer) {
      cairo_destroy(screen_configs[screen_num].overlay_buffer);
      screen_configs[screen_num].overlay_buffer = NULL;
    }

    if (screen_configs[screen_num].background_buffer) {
      cairo_destroy(screen_configs[screen_num].background_buffer);
      screen_configs[screen_num].background_buffer = NULL;
    }

    if (screen_configs[screen_num].screen_buffer) {
      cairo_destroy(screen_configs[screen_num].screen_buffer);
      screen_configs[screen_num].screen_buffer = NULL;
    }

    if (screen_configs[screen_num].off_screen_buffer) {
      cairo_surface_destroy(screen_configs[screen_num].off_screen_buffer);
      screen_configs[screen_num].off_screen_buffer = NULL;
    }

    if (screen_configs[screen_num].surface) {
      cairo_surface_destroy(screen_configs[screen_num].surface);
      screen_configs[screen_num].surface = NULL;
    }

    XDestroyWindow(display_config->display, screen_configs[screen_num].window);
  }

  if (display_config->image_surface) {
    cairo_surface_destroy(display_config->image_surface);
    display_config->image_surface = NULL;
  }

  /* Wait for the date update thread to finish. */
  pthread_join(date_thread, NULL);
}

/**
 * @brief Handles a key press event within the lock screen.
 *
 * @param key_event The XKeyEvent representing the key press.
 */
static void handle_keypress(XKeyEvent key_event) {
  password_is_wrong = 0;

  /* 22 is the Backspace keycode in many X configurations, 36 is Return. */
  if (key_event.keycode == 22) { /* Backspace */
    if (current_input_index > 0) {
      current_input_index--;
      current_input[current_input_index] = '\0';
    }
  } else if (key_event.keycode == 36) { /* Enter */
    /* Attempt authentication. If successful, exit the lock screen. */
    if (auth_pam(current_input, pw->pw_name) == 0) {
      lockscreen_running = 0;
    } else {
      password_is_wrong = 1;
    }
    current_input_index = 0;
    current_input[0] = '\0';
  } else {
    /* Capture visible characters into current_input. */
    char event_char;
    KeySym key_sym;
    XLookupString(&key_event, &event_char, 1, &key_sym, NULL);

    if (isprint((unsigned char)event_char)) {
      /* Avoid writing past the buffer. */
      if (current_input_index < (int)(sizeof(current_input) - 1)) {
        current_input[current_input_index] = event_char;
        current_input_index++;
        current_input[current_input_index] = '\0';
      }
    }
  }

  /* Update the screen with any changes. */
  draw_graphics();
}

/**
 * @brief Main function to initiate the lock screen.
 *
 * @return 0 on success, nonzero on failure.
 */
int lockscreen(void) {
  lockscreen_running = 1;

  /* Retrieve the user database entry for the current user. */
  pw = getpwnam(getlogin());
  if (pw == NULL) {
    fprintf(stderr, "Failed to get user information.\n");
    return 1;
  }

  /* Set up windows and graphics resources. */
  initialize_windows();
  initialize_graphics();

  /* Hide the cursor and grab the keyboard. */
  XFixesHideCursor(display_config->display, root_window);
  XGrabKeyboard(display_config->display,
                DefaultRootWindow(display_config->display), True, GrabModeAsync,
                GrabModeAsync, CurrentTime);

  /* Create a thread to continuously update date/time. */
  int thread_create_result =
      pthread_create(&date_thread, NULL, date_loop, NULL);
  if (thread_create_result != 0) {
    fprintf(stderr, "Failed to create date update thread.\n");
    return 1;
  }

  /* Event loop for the lock screen. */
  XEvent event;
  while (lockscreen_running) {
    XNextEvent(display_config->display, &event);

    switch (event.type) {
    case Expose:
      draw_graphics();
      break;
    case KeyPress:
      handle_keypress(event.xkey);
      break;
    default:
      break;
    }
  }

  /* Cleanup resources before exiting. */
  cleanUp();
  return 0;
}
