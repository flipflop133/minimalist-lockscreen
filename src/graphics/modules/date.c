#include "../../defs.h"
#include "../graphics.h"
#include <cairo/cairo.h>
#include <time.h>
#include <unistd.h>

struct date_data {
  char date[64];
  char clock[32];
} date_data;

void *date_loop(void *arg) {
  time_t t;
  struct tm *time_props;
  while (running) {
    t = time(&t);
    time_props = localtime(&t);
    strftime(date_data.date, 64, "%A, %d %B", time_props);
    strftime(date_data.clock, 32, "%I:%M", time_props);
    draw_graphics();
    sleep(1);
  }
  return NULL;
}

/**
 * Draws a clock on the specified screen.
 *
 * @param screen_num The number of the screen to draw the clock on.
 */
void draw_clock(int screen_num) {
  cairo_set_source_rgba(screen_configs[screen_num].overlay_buffer,
                        screen_configs->text_color, screen_configs->text_color,
                        screen_configs->text_color, 0.8);
  // Display date
  cairo_set_font_size(screen_configs[screen_num].overlay_buffer, 30.0);

  cairo_text_extents_t date_extents;
  cairo_text_extents(screen_configs[screen_num].overlay_buffer, date_data.date,
                     &date_extents);
  cairo_font_extents_t font_extents;
  cairo_font_extents(screen_configs[screen_num].overlay_buffer, &font_extents);
  cairo_move_to(screen_configs[screen_num].overlay_buffer,
                ((double)display_config->screen_info[screen_num].width / 2) -
                    (date_extents.width / 2) - (date_extents.x_bearing),
                ((double)display_config->screen_info[screen_num].height / 5));
  cairo_show_text(screen_configs[screen_num].overlay_buffer, date_data.date);

  // Display clock
  cairo_set_font_size(screen_configs[screen_num].overlay_buffer, 150.0);

  cairo_text_extents_t clock_extents;
  cairo_text_extents(screen_configs[screen_num].overlay_buffer, date_data.clock,
                     &clock_extents);
  cairo_move_to(screen_configs[screen_num].overlay_buffer,
                ((double)display_config->screen_info[screen_num].width / 2) -
                    (clock_extents.width / 2) - (clock_extents.x_bearing),
                ((double)display_config->screen_info[screen_num].height / 5) +
                    date_extents.height + clock_extents.height);
  cairo_show_text(screen_configs[screen_num].overlay_buffer, date_data.clock);
}