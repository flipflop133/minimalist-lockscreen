#include "display.h"
#include "args.h"
#include "defs.h"
#include <X11/Xlib.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Initializes the graphics for the lockscreen.
 */
void initialize_graphics() {
  display_config->image_surface =
      cairo_image_surface_create_from_png(retrieve_command_arg("--image"));

  if (cairo_surface_status(display_config->image_surface) !=
      CAIRO_STATUS_SUCCESS) {
    fprintf(stderr, "Unable to load image\n");
    return;
  }

  // Initialize graphics for each screen
  for (int i = 0; i < display_config->num_screens; i++) {
    // Initialize cairo surface
    screen_configs[i].visual = DefaultVisual(
        display_config->display, DefaultScreen(display_config->display));
    screen_configs[i].surface = cairo_xlib_surface_create(
        display_config->display, screen_configs[i].window,
        screen_configs[i].visual, display_config->screen_info[i].width,
        display_config->screen_info[i].height);

    if (screen_configs[i].surface == NULL) {
      fprintf(stderr, "Unable to create surface\n");
      return;
    }

    screen_configs[i].screen_buffer = cairo_create(screen_configs[i].surface);
    screen_configs[i].off_screen_buffer = cairo_surface_create_similar(
        screen_configs[i].surface, CAIRO_CONTENT_COLOR_ALPHA,
        display_config->screen_info[i].width,
        display_config->screen_info[i].height);

    screen_configs[i].overlay_buffer =
        cairo_create(screen_configs[i].off_screen_buffer);
    screen_configs[i].background_buffer =
        cairo_create(screen_configs[i].off_screen_buffer);
    cairo_font_face_t *font_face = cairo_toy_font_face_create(
        "JetBrainsMono NF", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_face(screen_configs[i].overlay_buffer, font_face);
    cairo_font_face_destroy(font_face);

    screen_configs[i].pattern =
        cairo_pattern_create_for_surface(display_config->image_surface);

    // Scale the pattern surface
    cairo_matrix_t matrix;
    double scale_factor;
    double x_scale =
        cairo_image_surface_get_width(display_config->image_surface) /
        (double)display_config->screen_info[i].width;
    double y_scale =
        cairo_image_surface_get_height(display_config->image_surface) /
        (double)display_config->screen_info[i].height;

    if (x_scale < y_scale) {
      // Fit the image height to the screen height
      scale_factor = x_scale;
    } else {
      // Fit the image width to the screen width
      scale_factor = y_scale;
    }

    cairo_matrix_init_scale(&matrix, scale_factor, scale_factor);
    cairo_pattern_set_matrix(screen_configs[i].pattern, &matrix);

    cairo_set_source(screen_configs[i].background_buffer,
                     screen_configs[i].pattern);

    determine_text_color(display_config->image_surface,
                         display_config->screen_info[i].width,
                         display_config->screen_info[i].height);

    draw_graphics();
  }
}

/**
 * Draws the graphics on the screens.
 */
void draw_graphics() {
  pthread_mutex_lock(&mutex);
  for (int i = 0; i < display_config->num_screens; i++) {
    // Draw graphics
    cairo_paint(screen_configs[i].background_buffer); // Draw background image
    draw_password_entry(i);
    draw_clock(i);
    cairo_set_source_surface(screen_configs[i].screen_buffer,
                             screen_configs[i].off_screen_buffer, 0, 0);
    cairo_paint(screen_configs[i].screen_buffer);
  }
  pthread_mutex_unlock(&mutex);
}

/**
 * Draws the password entry interface on the specified screen.
 *
 * @param screen_num The number of the screen where the password entry interface
 * should be drawn.
 */
void draw_password_entry(int screen_num) {
  // Draw rounded rectangle
  cairo_set_line_width(screen_configs[screen_num].overlay_buffer, 2);
  double width;
  // Move to starting point
  if (display_config->screen_info[screen_num].width >
      display_config->screen_info[screen_num].height) {
    width = (double)display_config->screen_info[screen_num].width / 9;
  } else {
    width = (double)display_config->screen_info[screen_num].height / 9;
  }
  double height = width / 4;
  double x =
      ((double)display_config->screen_info[screen_num].width / 2) - (width / 2);
  double y =
      (double)display_config->screen_info[screen_num].height / 2 - (height / 2);

  double radius = 10.0;
  cairo_move_to(screen_configs[screen_num].overlay_buffer, x, y);

  cairo_line_to(screen_configs[screen_num].overlay_buffer, x + width - radius,
                y);
  cairo_arc(screen_configs[screen_num].overlay_buffer, x + width - radius,
            y + radius, radius, -0.5 * M_PI, 0.0);
  cairo_line_to(screen_configs[screen_num].overlay_buffer, x + width,
                y + height - radius);
  cairo_arc(screen_configs[screen_num].overlay_buffer, x + width - radius,
            y + height - radius, radius, 0.0, 0.5 * M_PI);
  cairo_line_to(screen_configs[screen_num].overlay_buffer, x + radius,
                y + height);
  cairo_arc(screen_configs[screen_num].overlay_buffer, x + radius,
            y + height - radius, radius, 0.5 * M_PI, M_PI);
  cairo_line_to(screen_configs[screen_num].overlay_buffer, x, y + radius);
  cairo_arc(screen_configs[screen_num].overlay_buffer, x + radius, y + radius,
            radius, M_PI, 1.5 * M_PI);
  cairo_close_path(screen_configs[screen_num].overlay_buffer);

  cairo_set_source_rgba(screen_configs[screen_num].overlay_buffer,
                        screen_configs->text_color, screen_configs->text_color,
                        screen_configs->text_color, 0.5);
  cairo_fill_preserve(screen_configs[screen_num].overlay_buffer);

  cairo_set_font_size(screen_configs[screen_num].overlay_buffer, 30);
  cairo_set_source_rgb(screen_configs[screen_num].overlay_buffer,
                       screen_configs->text_color, screen_configs->text_color,
                       screen_configs->text_color);
  // Draw text
  cairo_font_extents_t font_extents;
  cairo_font_extents(screen_configs[screen_num].overlay_buffer, &font_extents);
  int password_text_padding = 10;
  if (current_input_index > 0) {
    cairo_text_extents_t password_extents;
    char *str = (char *)malloc(current_input_index + 1);
    str[0] = '\0';
    for (int i = 0; i < current_input_index; i++) {
      cairo_text_extents(screen_configs[screen_num].overlay_buffer, str,
                         &password_extents);
      if (password_extents.width >= width - (password_text_padding * 2)) {
        break;
      }
      str[i] = '*';
      str[i + 1] = '\0';
    }

    cairo_move_to(
        screen_configs[screen_num].overlay_buffer, x + password_text_padding,
        y + (height / 2) + (font_extents.height / 2) - font_extents.descent);
    cairo_show_text(screen_configs[screen_num].overlay_buffer, str);
    free(str);
  } else {
    char *str = "Enter password";
    if (password_is_wrong) {
      cairo_set_source_rgb(screen_configs[screen_num].overlay_buffer, 1, 0, 0);
      str = "Wrong password!";
    }

    cairo_text_extents_t password_extents;
    cairo_text_extents(screen_configs[screen_num].overlay_buffer, str,
                       &password_extents);
    cairo_move_to(screen_configs[screen_num].overlay_buffer, x + 10,
                  y + (height / 2) + (font_extents.height / 2) -
                      font_extents.descent);
    cairo_show_text(screen_configs[screen_num].overlay_buffer, str);
  }
}

/**
 * Draws a clock on the specified screen.
 *
 * @param screen_num The number of the screen to draw the clock on.
 */
void draw_clock(int screen_num) {
  time_t t = time(&t);
  struct tm *time_props = NULL;
  time_props = localtime(&t);
  if (time_props != NULL) {
    cairo_set_source_rgba(
        screen_configs[screen_num].overlay_buffer, screen_configs->text_color,
        screen_configs->text_color, screen_configs->text_color, 0.8);
    // Display date
    cairo_set_font_size(screen_configs[screen_num].overlay_buffer, 30.0);
    char date[64];
    strftime(date, 64, "%A, %d %B", time_props);
    cairo_text_extents_t date_extents;
    cairo_text_extents(screen_configs[screen_num].overlay_buffer, date,
                       &date_extents);
    cairo_font_extents_t font_extents;
    cairo_font_extents(screen_configs[screen_num].overlay_buffer,
                       &font_extents);
    cairo_move_to(screen_configs[screen_num].overlay_buffer,
                  ((double)display_config->screen_info[screen_num].width / 2) -
                      (date_extents.width / 2) - (date_extents.x_bearing),
                  ((double)display_config->screen_info[screen_num].height / 5));
    cairo_show_text(screen_configs[screen_num].overlay_buffer, date);

    // Display clock
    cairo_set_font_size(screen_configs[screen_num].overlay_buffer, 150.0);
    char clock[32];
    strftime(clock, 32, "%I:%M", time_props);
    cairo_text_extents_t clock_extents;
    cairo_text_extents(screen_configs[screen_num].overlay_buffer, clock,
                       &clock_extents);
    cairo_move_to(screen_configs[screen_num].overlay_buffer,
                  ((double)display_config->screen_info[screen_num].width / 2) -
                      (clock_extents.width / 2) - (clock_extents.x_bearing),
                  ((double)display_config->screen_info[screen_num].height / 5) +
                      date_extents.height + clock_extents.height);
    cairo_show_text(screen_configs[screen_num].overlay_buffer, clock);
  }
}

/**
 * Determines the text color for the given image.
 *
 * @param img The Cairo surface representing the image.
 * @param width The width of the image.
 * @param height The height of the image.
 */
void determine_text_color(cairo_surface_t *img, int width, int height) {
  char color[7] = "a3a3a3";
  int text_color = 0;
  if (img != NULL) {
    long red_tot = 0;
    long green_tot = 0;
    long blue_tot = 0;
    long tot = 0;
    for (int i = 0; i < width; i++) {
      for (int j = 0; j < height; j++) {
        uint32_t *data = (uint32_t *)cairo_image_surface_get_data(img);
        uint32_t pixel_color = data[i + j];
        // Extract individual color components
        uint8_t red = (pixel_color >> 16) & 0xFF;
        red_tot += red;
        uint8_t green = (pixel_color >> 8) & 0xFF;
        green_tot += green;
        uint8_t blue = pixel_color & 0xFF;
        blue_tot += blue;
        tot += 1;
      }
    }
    red_tot /= tot;
    green_tot /= tot;
    blue_tot /= tot;
    if (red_tot < 127.5 && green_tot < 127.5 && blue_tot < 127.5) {
      text_color = 255;
    }
  } else {
    int tot = 0;
    for (size_t c = 0; c < (strlen(color)); c++) {
      tot += color[c] - '0';
    }
    if ((double)tot / 2 < 127.5) {
      text_color = 255;
    }
  }
  screen_configs->text_color = text_color;
}