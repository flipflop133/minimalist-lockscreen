#ifndef DEFS_H
#define DEFS_H

#include <X11/Xft/Xft.h>
#include <X11/extensions/Xinerama.h>
#include <cairo/cairo.h>
#include <security/pam_appl.h>
// Function declarations
unsigned long hex_color_to_pixel(char *hex_color, int screen_num);
void handle_keypress(XKeyEvent keyEvent);
void *date_loop(void *arg);

// Macro definitions

// Structure definitions
struct ScreenConfig {
  XftDraw *xftDraw;
  Window window;
  Visual *visual;
  cairo_surface_t *surface;
  cairo_t *overlay_object;
  cairo_t *background_object;
  int text_color;
  cairo_pattern_t *pattern;
};

struct DisplayConfig {
  Display *display;
  GC gc;
  XftFont *font;
  XftColor xftColor;
  int num_screens;
  XftColor workspace_foreground_color;
  int yFontCoordinate;
  XineramaScreenInfo *screen_info;
  cairo_surface_t *image_surface;
};

// Variable declarations
extern struct ScreenConfig screen_configs[128]; // TODO : use a dynamic array
extern struct DisplayConfig *display_config;
extern int current_input_index;
extern int password_is_wrong;
#endif