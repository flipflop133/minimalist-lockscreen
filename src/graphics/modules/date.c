#include "../../defs.h"
#include "../graphics.h"
#include <cairo/cairo.h>
#include <stdio.h>
#include <time.h>

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