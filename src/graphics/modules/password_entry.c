#include "../../defs.h"
#include "../graphics.h"
#include <cairo/cairo.h>
#include <math.h>
#include <stdlib.h>

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

  double radius = 20.0;
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

  // Draw text
  cairo_set_font_size(screen_configs[screen_num].overlay_buffer, 30);
  int opposite_color = get_opposite_color(screen_configs->text_color);
  cairo_set_source_rgb(screen_configs[screen_num].overlay_buffer,
                       opposite_color, opposite_color, opposite_color);
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
      if (screen_configs->text_color > (255 / 2)) {
        cairo_set_source_rgb(screen_configs[screen_num].overlay_buffer,
                             0.70196078431, 0, 0);
      } else {
        cairo_set_source_rgb(screen_configs[screen_num].overlay_buffer, 1,
                             0.3686274509803922, 0.3686274509803922);
      }
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