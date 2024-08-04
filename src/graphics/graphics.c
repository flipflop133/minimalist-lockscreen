#include "graphics.h"
#include "../args.h"
#include "../lockscreen.h"
#include <X11/Xlib.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <pthread.h>
#include <stdio.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Initializes the graphics for the lockscreen.
 */
void initialize_graphics() {
  display_config->image_surface =
      cairo_image_surface_create_from_png(retrieve_command_arg("--image"));

  if (display_config->image_surface == NULL) {
    fprintf(stderr, "Unable to load image\n");
    return;
  }
  if (cairo_surface_status(display_config->image_surface) !=
      CAIRO_STATUS_SUCCESS) {
    fprintf(stderr, "Unable to load image\n");
    return;
  }

  // Initialize graphics for each screen
  for (int screen_num = 0; screen_num < display_config->num_screens;
       screen_num++) {
    // Initialize cairo surface
    screen_configs[screen_num].visual = DefaultVisual(
        display_config->display, DefaultScreen(display_config->display));
    screen_configs[screen_num].surface = cairo_xlib_surface_create(
        display_config->display, screen_configs[screen_num].window,
        screen_configs[screen_num].visual,
        display_config->screen_info[screen_num].width,
        display_config->screen_info[screen_num].height);

    if (screen_configs[screen_num].surface == NULL) {
      fprintf(stderr, "Unable to create surface\n");
      return;
    }

    screen_configs[screen_num].screen_buffer =
        cairo_create(screen_configs[screen_num].surface);
    screen_configs[screen_num].off_screen_buffer = cairo_surface_create_similar(
        screen_configs[screen_num].surface, CAIRO_CONTENT_COLOR_ALPHA,
        display_config->screen_info[screen_num].width,
        display_config->screen_info[screen_num].height);

    screen_configs[screen_num].overlay_buffer =
        cairo_create(screen_configs[screen_num].off_screen_buffer);
    screen_configs[screen_num].background_buffer =
        cairo_create(screen_configs[screen_num].off_screen_buffer);
    cairo_font_face_t *font_face = cairo_toy_font_face_create(
        "JetBrainsMono NF", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_face(screen_configs[screen_num].overlay_buffer, font_face);
    cairo_font_face_destroy(font_face);

    screen_configs[screen_num].pattern =
        cairo_pattern_create_for_surface(display_config->image_surface);

    // Scale the pattern surface
    cairo_matrix_t matrix;
    double scale_factor;
    double x_scale =
        cairo_image_surface_get_width(display_config->image_surface) /
        (double)display_config->screen_info[screen_num].width;
    double y_scale =
        cairo_image_surface_get_height(display_config->image_surface) /
        (double)display_config->screen_info[screen_num].height;

    if (x_scale < y_scale) {
      // Fit the image height to the screen height
      scale_factor = x_scale;
    } else {
      // Fit the image width to the screen width
      scale_factor = y_scale;
    }

    cairo_matrix_init_scale(&matrix, scale_factor, scale_factor);
    cairo_pattern_set_matrix(screen_configs[screen_num].pattern, &matrix);

    cairo_set_source(screen_configs[screen_num].background_buffer,
                     screen_configs[screen_num].pattern);

    determine_text_color(display_config->image_surface,
                         display_config->screen_info[screen_num].width,
                         display_config->screen_info[screen_num].height);
  }
  draw_graphics();
}

/**
 * Draws the graphics on the screens.
 */
void draw_graphics() {
  pthread_mutex_lock(&mutex);
  for (int screen_num = 0; screen_num < display_config->num_screens;
       screen_num++) {
    cairo_paint(
        screen_configs[screen_num].background_buffer); // Draw background image
    draw_password_entry(screen_num);
    draw_clock(screen_num);
    cairo_set_source_surface(screen_configs[screen_num].screen_buffer,
                             screen_configs[screen_num].off_screen_buffer, 0,
                             0);
    cairo_paint(screen_configs[screen_num].screen_buffer);
  }
  pthread_mutex_unlock(&mutex);
}