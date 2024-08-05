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
#include <pthread.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

struct DisplayConfig *display_config;
struct ScreenConfig screen_configs[128]; // TODO : use a dynamic array
struct passwd *pw;
char current_input[128];
int current_input_index = 0;
int password_is_wrong = 0;
int lockscreen_running = 0;
pthread_t date_thread;
Window root_window;

int lockscreen(void)
{
  lockscreen_running = 1;

  // Retrieve user database
  pw = getpwnam(getlogin());
  if (pw == NULL)
  {
    return 1;
  }

  initialize_windows();

  initialize_graphics();

  XFixesHideCursor(display_config->display, root_window);
  XGrabKeyboard(display_config->display,
                DefaultRootWindow(display_config->display), True, GrabModeAsync,
                GrabModeAsync, CurrentTime);

  // Create a thread to update the date and time
  int thread_create_result;
  thread_create_result = pthread_create(&date_thread, NULL, date_loop, NULL);
  if (thread_create_result != 0)
  {
    fprintf(stderr, "Failed to create thread.\n");
    return 1;
  }

  // Main event loop
  XEvent event;
  while (lockscreen_running)
  {
    XNextEvent(display_config->display, &event);
    if (event.type == Expose)
    {
      draw_graphics();
    }
    else if (event.type == KeyPress)
    {
      handle_keypress(event.xkey);
    }
  }

  cleanUp();

  return 0;
}

/**
 * Initializes the windows for the application.
 */
void initialize_windows()
{
  if (display_config->display == NULL)
  {
    fprintf(stderr, "Cannot open display\n");
    exit(1);
  }
  display_config->screen_info = XineramaQueryScreens(
      display_config->display, &display_config->num_screens);
  unsigned long background_color =
      hex_color_to_pixel("#000000", DefaultScreen(display_config->display));

  root_window = RootWindow(display_config->display,
                           DefaultScreen(display_config->display));

  for (int i = 0; i < display_config->num_screens; i++)
  {
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
}

/**
 * Performs clean-up tasks.
 * This function is responsible for cleaning up any resources or performing any
 * necessary clean-up tasks before the program terminates.
 */
void cleanUp()
{

  // Clean up
  XFixesShowCursor(display_config->display, root_window);
  XUngrabKeyboard(display_config->display, CurrentTime);
  for (int screen_num = 0; screen_num < display_config->num_screens;
       screen_num++)
  {
    cairo_font_face_t *overlay_font_face =
        cairo_get_font_face(screen_configs[screen_num].overlay_buffer);
    if (overlay_font_face != NULL)
      cairo_font_face_destroy(overlay_font_face);
    cairo_destroy(screen_configs[screen_num].overlay_buffer);
    cairo_destroy(screen_configs[screen_num].background_buffer);
    cairo_destroy(screen_configs[screen_num].screen_buffer);
    cairo_pattern_destroy(screen_configs[screen_num].pattern);
    cairo_surface_destroy(screen_configs[screen_num].surface);
    cairo_surface_destroy(screen_configs[screen_num].off_screen_buffer);

    XDestroyWindow(display_config->display, screen_configs[screen_num].window);
  }
  cairo_surface_destroy(display_config->image_surface);

  pthread_join(date_thread, NULL);
}

/**
 * Handles a key press event.
 *
 * @param keyEvent The XKeyEvent representing the key press event.
 */
void handle_keypress(XKeyEvent keyEvent)
{
  password_is_wrong = 0;
  if (keyEvent.keycode == 22)
  {
    if (current_input_index != 0)
    {
      current_input_index--;
      current_input[current_input_index] = '\0';
    }
  }
  else if (keyEvent.keycode == 36)
  {
    if (auth_pam(current_input, pw->pw_name) == 0)
    {
      lockscreen_running = 0;
    }
    else
    {
      password_is_wrong = 1;
    }
    current_input_index = 0;
    current_input[0] = '\0';
  }
  else
  {
    char event_char;
    KeySym keySym;
    XLookupString(&keyEvent, &event_char, 1, &keySym, NULL);
    if (isprint(event_char))
    {
      current_input[current_input_index] = event_char;
      current_input_index++;
    }
  }
  draw_graphics();
}