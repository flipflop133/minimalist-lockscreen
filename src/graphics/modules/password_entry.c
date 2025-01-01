#include "../../lockscreen.h"
#include "../graphics.h"
#include <cairo/cairo.h>
#include <math.h>
#include <stdlib.h>

/*
 * Helper: reduce the font size in steps until the text fits
 * or until we reach min_font_size.
 */
static void clamp_text_to_width(cairo_t *cr, const char *text, double max_width,
                                double *font_size, double min_font_size) {
  cairo_text_extents_t ext;
  cairo_set_font_size(cr, *font_size);

  cairo_text_extents(cr, text, &ext);

  // While text exceeds max_width and font_size above min
  while (ext.width > max_width && *font_size > min_font_size) {
    *font_size -= 1.0; // decrement the font size by 1.0
    cairo_set_font_size(cr, *font_size);
    cairo_text_extents(cr, text, &ext);
  }
}

/**
 * Draws the password entry interface on the specified screen.
 *
 * @param screen_num The number of the screen where the password entry interface
 * should be drawn.
 */
void draw_password_entry(int screen_num) {
  // For convenience
  cairo_t *cr = screen_configs[screen_num].overlay_buffer;

  // Decide rectangle size
  double width;
  if (display_config->screen_info[screen_num].width >
      display_config->screen_info[screen_num].height) {
    width = (double)display_config->screen_info[screen_num].width / 9.0;
  } else {
    width = (double)display_config->screen_info[screen_num].height / 9.0;
  }
  double height = width / 4.0;

  double x = ((double)display_config->screen_info[screen_num].width / 2.0) -
             (width / 2.0);
  double y = ((double)display_config->screen_info[screen_num].height / 2.0) -
             (height / 2.0);

  // ----------------------------------------------------------
  // (1) First, repaint (restore) the background region so that
  // we have a fresh background under our rectangle.
  // ----------------------------------------------------------
  repaint_background_at((int)x, (int)y, (int)width, (int)height, screen_num);

  // ----------------------------------------------------------
  // (2) Now draw the semi-transparent rectangle *once*,
  // to avoid alpha stacking on repeated draws.
  // ----------------------------------------------------------
  double radius = 20.0;
  cairo_save(cr);
  {
    // Use OPERATOR_OVER to see the background through alpha
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    // Semi-transparent fill
    cairo_set_source_rgba(cr, screen_configs->text_color,
                          screen_configs->text_color,
                          screen_configs->text_color,
                          0.5); // 50% alpha

    // Build the path for the rounded rectangle
    cairo_move_to(cr, x, y);
    cairo_line_to(cr, x + width - radius, y);
    cairo_arc(cr, x + width - radius, y + radius, radius, -0.5 * M_PI, 0.0);
    cairo_line_to(cr, x + width, y + height - radius);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0.0,
              0.5 * M_PI);
    cairo_line_to(cr, x + radius, y + height);
    cairo_arc(cr, x + radius, y + height - radius, radius, 0.5 * M_PI, M_PI);
    cairo_line_to(cr, x, y + radius);
    cairo_arc(cr, x + radius, y + radius, radius, M_PI, 1.5 * M_PI);
    cairo_close_path(cr);

    cairo_fill_preserve(cr);
  }
  cairo_restore(cr);

  // ----------------------------------------------------------
  // (3) Now draw text on top of that rectangle.
  // ----------------------------------------------------------
  // Setup text color and initial font size
  double font_size = 30.0;
  cairo_set_font_size(cr, font_size);

  int opposite_color = get_opposite_color(screen_configs->text_color);
  cairo_set_source_rgb(cr, opposite_color, opposite_color, opposite_color);

  // Get font extents for positioning
  cairo_font_extents_t font_extents;
  cairo_font_extents(cr, &font_extents);

  double password_text_padding = 10.0;
  double text_area_width = width - (2.0 * password_text_padding);

  // If user typed something, show asterisks
  if (current_input_index > 0) {
    cairo_text_extents_t password_extents;
    char *str = (char *)malloc(current_input_index + 1);
    if (!str)
      return;
    str[0] = '\0';

    int i;
    for (i = 0; i < current_input_index; i++) {
      str[i] = '*';
      str[i + 1] = '\0';
      cairo_text_extents(cr, str, &password_extents);
      if (password_extents.width > text_area_width) {
        // If we exceeded the area, revert the last addition
        str[i] = '\0';
        break;
      }
    }

    double tx = x + password_text_padding;
    double ty =
        y + (height / 2.0) + (font_extents.height / 2.0) - font_extents.descent;
    cairo_move_to(cr, tx, ty);
    cairo_show_text(cr, str);

    free(str);
  } else {
    // Show placeholder or wrong password message
    char *str = "Enter password";
    if (password_is_wrong) {
      // If text_color is bright, use darker red
      // else a bright red
      if (screen_configs->text_color > (255 / 2)) {
        cairo_set_source_rgb(cr, 0.70196078431, 0, 0); // ~#B30000
      } else {
        cairo_set_source_rgb(cr, 1.0, 0.36862745098,
                             0.36862745098); // ~#FF5E5E
      }
      str = "Wrong password!";
    }

    // Clamp text so it never overflows
    clamp_text_to_width(cr, str, text_area_width, &font_size, 8.0);
    // Re-fetch font extents now that we might have changed font_size
    cairo_font_extents(cr, &font_extents);

    double tx = x + 10.0;
    double ty =
        y + (height / 2.0) + (font_extents.height / 2.0) - font_extents.descent;

    cairo_move_to(cr, tx, ty);
    cairo_show_text(cr, str);
  }
}