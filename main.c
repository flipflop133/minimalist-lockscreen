#include "args.h"
#include "defs.h"
#include "display.h"
#include "pam.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <math.h>
#include <openssl/evp.h>
#include <pwd.h>
#include <shadow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
struct DisplayConfig *display_config;
struct ScreenConfig screen_configs[128]; // TODO : use a dynamic array
struct passwd *pw;
char current_input[128];
int current_input_index = 0;
int password_is_wrong = 0;
int running = 1;
int main(int argc, char *argv[]) {
  parse_arguments(argc, argv);
  pw = getpwnam(getlogin());
  if (pw == NULL) {
    return 1;
  }
  display_config = (struct DisplayConfig *)malloc(sizeof(struct DisplayConfig));
  XEvent event;

  display_config->display = XOpenDisplay(NULL);
  if (display_config->display == NULL) {
    fprintf(stderr, "Cannot open display\n");
    exit(1);
  }
  display_config->screen_info = XineramaQueryScreens(
      display_config->display, &display_config->num_screens);
  unsigned long background_color =
      hex_color_to_pixel("#000000", DefaultScreen(display_config->display));

  Window root_window = RootWindow(display_config->display,
                                  DefaultScreen(display_config->display));

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

    Atom net_wm_window_property =
        XInternAtom(display_config->display, "_NET_WM_STATE", False);
    Atom net_wm_window_type =
        XInternAtom(display_config->display, "_NET_WM_STATE_FULLSCREEN", False);

    XChangeProperty(display_config->display, screen_configs[i].window,
                    net_wm_window_property, XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)&net_wm_window_type, 1);
    XMapWindow(display_config->display, screen_configs[i].window);
  }

  initialize_graphics();
  XFixesHideCursor(display_config->display, root_window);
  XGrabKeyboard(display_config->display,
                DefaultRootWindow(display_config->display), True, GrabModeAsync,
                GrabModeAsync, CurrentTime);

  pthread_t my_thread; // Declare a thread object
  int thread_create_result;

  // Create a thread
  thread_create_result = pthread_create(&my_thread, NULL, date_loop, NULL);

  // Check if the thread creation was successful
  if (thread_create_result != 0) {
    fprintf(stderr, "Failed to create thread.\n");
    return 1;
  }

  while (running) {
    XNextEvent(display_config->display, &event);
    if (event.type == Expose) {
      redraw_graphics();
    } else if (event.type == KeyPress) {
      handle_keypress(event.xkey);
    }
  }

  // Clean up
  for (int i = 0; i < display_config->num_screens; i++) {
    cairo_font_face_t *overlay_font_face =
        cairo_get_font_face(screen_configs[i].overlay_object);
    cairo_font_face_destroy(overlay_font_face);
    cairo_destroy(screen_configs[i].overlay_object);
    cairo_destroy(screen_configs[i].background_object);

    cairo_pattern_destroy(screen_configs[i].pattern);
    cairo_surface_destroy(screen_configs[i].surface);
    XDestroyWindow(display_config->display, screen_configs[i].window);
  }
  cairo_surface_destroy(display_config->image_surface);

  XCloseDisplay(display_config->display);

  pthread_join(my_thread, NULL);
  return 0;
}

void handle_keypress(XKeyEvent keyEvent) {
  password_is_wrong = 0;
  if (keyEvent.keycode == 22) {
    if (current_input_index != 0) {
      current_input_index--;
      current_input[current_input_index] = '\0';
    }
  } else if (keyEvent.keycode == 36) {
    if (auth_pam(current_input, pw->pw_name) == 0) {
      running = 0;
    } else {
      current_input_index = 0;
      current_input[0] = '\0';
      password_is_wrong = 1;
    }
  } else {
    char event_char;
    XLookupString(&keyEvent, &event_char, 1, 0, NULL);
    current_input[current_input_index] = event_char;
    current_input_index++;
  }
  redraw_graphics();
}

/**
 * Converts a hexadecimal color code to a pixel value.
 *
 * @param hex_color The hexadecimal color code to convert.
 * @param screen_num The screen number to convert the color for.
 * @return The pixel value corresponding to the given color code.
 */
unsigned long hex_color_to_pixel(char *hex_color, int screen_num) {
  XColor color;
  Colormap colormap = DefaultColormap(display_config->display, screen_num);
  XParseColor(display_config->display, colormap, hex_color, &color);
  XAllocColor(display_config->display, colormap, &color);
  return color.pixel;
}

void *date_loop(void *arg) {
  while (running) {
    redraw_graphics();
    sleep(1);
  }
  return NULL;
}