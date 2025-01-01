/**
 * @file password_entry.c
 * @brief Renders the password-entry widget, including a rounded rectangle with
 *        text for password input or error messages.
 */

#include "../../lockscreen.h"
#include "../graphics.h"
#include <cairo/cairo.h>
#include <math.h>
#include <stdlib.h>

/* ------------------------------------------------------------------------- */
/* Constants                                                                 */
/* ------------------------------------------------------------------------- */

static const double DEFAULT_FONT_SIZE = 30.0;
static const double MIN_FONT_SIZE = 8.0;
static const double RECTANGLE_RADIUS = 20.0;
static const double SEMI_TRANSPARENCY_ALPHA = 0.5;
static const double PASSWORD_TEXT_PADDING = 10.0;
static const int FONT_DECREMENT_STEP = 1; /* Decrement in px when clamping. */

/* ------------------------------------------------------------------------- */
/* Static Helper Functions                                                   */
/* ------------------------------------------------------------------------- */

/**
 * @brief Draw a rounded rectangle on the given Cairo context.
 *
 * @param cr      The Cairo context to draw on.
 * @param x       Top-left corner X-coordinate.
 * @param y       Top-left corner Y-coordinate.
 * @param width   Width of the rectangle.
 * @param height  Height of the rectangle.
 * @param radius  Radius for the rounded corners.
 */
static void draw_rounded_rectangle_path(cairo_t *cr, double x, double y,
                                        double width, double height,
                                        double radius) {
  cairo_new_path(cr);

  /* Top edge */
  cairo_move_to(cr, x + radius, y);
  cairo_line_to(cr, x + width - radius, y);
  cairo_arc(cr, x + width - radius, y + radius, radius, -0.5 * M_PI, 0.0);

  /* Right edge */
  cairo_line_to(cr, x + width, y + height - radius);
  cairo_arc(cr, x + width - radius, y + height - radius, radius, 0.0,
            0.5 * M_PI);

  /* Bottom edge */
  cairo_line_to(cr, x + radius, y + height);
  cairo_arc(cr, x + radius, y + height - radius, radius, 0.5 * M_PI, M_PI);

  /* Left edge */
  cairo_line_to(cr, x, y + radius);
  cairo_arc(cr, x + radius, y + radius, radius, M_PI, 1.5 * M_PI);

  cairo_close_path(cr);
}

/**
 * @brief Adjusts the font size in small decrements until the text fits within a
 *        specified width or the font size reaches a minimum threshold.
 *
 * @param cr            The Cairo context.
 * @param text          The text to measure.
 * @param max_width     The maximum allowed width.
 * @param font_size     Pointer to the current font size; may be reduced.
 * @param min_font_size The minimum allowable font size.
 */
static void clamp_text_to_width(cairo_t *cr, const char *text, double max_width,
                                double *font_size, double min_font_size) {
  cairo_text_extents_t ext;
  cairo_set_font_size(cr, *font_size);
  cairo_text_extents(cr, text, &ext);

  while ((ext.width > max_width) && (*font_size > min_font_size)) {
    *font_size -= FONT_DECREMENT_STEP;
    cairo_set_font_size(cr, *font_size);
    cairo_text_extents(cr, text, &ext);
  }
}

/* ------------------------------------------------------------------------- */
/* Primary Function                                                          */
/* ------------------------------------------------------------------------- */

/**
 * @brief Draws the password entry UI on the given screen. This includes a
 *        semi-transparent rounded rectangle with either placeholder text,
 *        error text, or asterisks representing the current password input.
 *
 * @param screen_num Index of the screen where the widget should be drawn.
 */
void draw_password_entry(int screen_num) {
  cairo_t *cr = screen_configs[screen_num].overlay_buffer;

  /* --- Screen dimensions --- */
  double screen_width = (double)display_config->screen_info[screen_num].width;
  double screen_height = (double)display_config->screen_info[screen_num].height;

  /* Determine raw rectangle width based on the dominant screen dimension. */
  double raw_width =
      (screen_width > screen_height) ? screen_width / 9.0 : screen_height / 9.0;

  int rect_width = (int)ceil(raw_width);
  int rect_height = (int)ceil(rect_width / 4.0);

  /* Position the rectangle in the center of the screen. */
  int rect_x = (int)round((screen_width / 2.0) - (rect_width / 2.0));
  int rect_y = (int)round((screen_height / 2.0) - (rect_height / 2.0));

  /* ---------------------------------------------------------------------
   * (1) Clear the rectangle area.
   * --------------------------------------------------------------------- */
  cairo_save(cr);

  draw_rounded_rectangle_path(cr, rect_x, rect_y, rect_width, rect_height,
                              RECTANGLE_RADIUS);

  cairo_clip(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_restore(cr);

  /* ---------------------------------------------------------------------
   * (2) Repaint the background in this cleared region.
   * --------------------------------------------------------------------- */
  repaint_background_at(rect_x, rect_y, rect_width, rect_height, screen_num);

  /* ---------------------------------------------------------------------
   * (3) Draw a semi-transparent rounded rectangle on top.
   * --------------------------------------------------------------------- */
  cairo_save(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  /* Use screen_configs->text_color as the grayscale channel. */
  cairo_set_source_rgba(cr, screen_configs->text_color,
                        screen_configs->text_color, screen_configs->text_color,
                        SEMI_TRANSPARENCY_ALPHA);

  cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
  draw_rounded_rectangle_path(cr, rect_x, rect_y, rect_width, rect_height,
                              RECTANGLE_RADIUS);
  cairo_fill(cr);
  cairo_restore(cr);

  /* ---------------------------------------------------------------------
   * (4) Draw the text (password asterisks or placeholder/wrong password).
   * --------------------------------------------------------------------- */
  double font_size = DEFAULT_FONT_SIZE;
  cairo_set_font_size(cr, font_size);

  /* Compute a contrasting color for text (opposite of text_color). */
  int opposite_color = get_opposite_color(screen_configs->text_color);
  cairo_set_source_rgb(cr, opposite_color, opposite_color, opposite_color);

  /* Retrieve the baseline info. */
  cairo_font_extents_t font_extents;
  cairo_font_extents(cr, &font_extents);

  double text_area_width = rect_width - 2.0 * PASSWORD_TEXT_PADDING;

  if (current_input_index > 0) {
    /* --- User typed something: show asterisks. --- */

    cairo_text_extents_t password_extents;
    char *password_buffer =
        (char *)calloc(current_input_index + 1, sizeof(char));
    if (!password_buffer) {
      return; /* Bail out if allocation fails. */
    }

    /* Build an asterisk string without exceeding text_area_width. */
    for (int i = 0; i < current_input_index; i++) {
      password_buffer[i] = '*';
      password_buffer[i + 1] = '\0';

      cairo_text_extents(cr, password_buffer, &password_extents);
      if (password_extents.width > text_area_width) {
        /* Revert the last asterisk if it doesn't fit. */
        password_buffer[i] = '\0';
        break;
      }
    }

    double text_x = rect_x + PASSWORD_TEXT_PADDING;
    double text_y = rect_y + (rect_height / 2.0) + (font_extents.height / 2.0) -
                    font_extents.descent;

    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, password_buffer);

    free(password_buffer);

  } else {
    /* --- No input yet: show a placeholder or "Wrong password!" --- */
    const char *placeholder_str = "Enter password";
    const char *wrong_str = "Wrong password!";
    const char *display_str = placeholder_str;

    if (password_is_wrong) {
      display_str = wrong_str;

      /* Adjust text color for a "red" message. */
      if (screen_configs->text_color > 127) {
        /* If text_color is bright, use darker red (#B30000). */
        cairo_set_source_rgb(cr, 0.70196, 0.0, 0.0);
      } else {
        /* Otherwise, use a brighter red (#FF5E5E). */
        cairo_set_source_rgb(cr, 1.0, 0.36863, 0.36863);
      }
    }

    /* Clamp text so it never overflows the rectangle. */
    clamp_text_to_width(cr, display_str, text_area_width, &font_size,
                        MIN_FONT_SIZE);

    /* Re-fetch font metrics in case font_size changed. */
    cairo_font_extents(cr, &font_extents);

    double text_x = rect_x + PASSWORD_TEXT_PADDING;
    double text_y = rect_y + (rect_height / 2.0) + (font_extents.height / 2.0) -
                    font_extents.descent;

    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, display_str);
  }
}
