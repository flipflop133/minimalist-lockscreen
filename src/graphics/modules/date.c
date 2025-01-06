/**
 * @file clock.c
 * @brief Handles displaying the date and time on a lockscreen.
 */

#include "../../lockscreen.h"
#include "../graphics.h"
#include <cairo/cairo.h>
#include <time.h>
#include <unistd.h>

/* Global structure to hold date/time strings. */
static struct DateData {
  char date[64];
  char clock[32];
} g_date_data;

/**
 * @brief Thread function to continuously update the date/time strings.
 *
 * @param unused Unused argument (required for pthread signature).
 * @return Always returns NULL.
 */
void *date_loop(void *unused __attribute__((unused))) {
  while (lockscreen_running) {
    time_t current_time = time(NULL);
    struct tm *local_tm = localtime(&current_time);

    if (local_tm == NULL) {
      /* If localtime() fails, just sleep briefly and continue. */
      sleep(1);
      continue;
    }

    /* Format: e.g., "Monday, 01 January" */
    strftime(g_date_data.date, sizeof(g_date_data.date), "%A, %d %B", local_tm);
    /* Format: e.g., "08:05" in 12-hour format */
    strftime(g_date_data.clock, sizeof(g_date_data.clock), "%I:%M", local_tm);

    /* Redraw graphics to reflect the updated date/time. */
    draw_graphics();

    /* Update once per second. */
    sleep(1);
  }

  return NULL;
}

/**
 * @brief Draws the current date/time on the specified screen.
 *
 * This function displays two lines of text:
 *   1. The formatted date (smaller text).
 *   2. The clock (larger text).
 *
 * @param screen_num Index of the screen where the date/time will be drawn.
 */
void draw_clock(int screen_num) {
  /* Set up text color with some transparency. */
  cairo_set_source_rgba(screen_configs[screen_num].overlay_buffer,
                        screen_configs->text_color, screen_configs->text_color,
                        screen_configs->text_color, 0.8);

  /* --- Draw Date --- */
  const double small_font_size = 30.0;
  cairo_set_font_size(screen_configs[screen_num].overlay_buffer,
                      small_font_size);

  cairo_text_extents_t date_extents;
  cairo_text_extents(screen_configs[screen_num].overlay_buffer,
                     g_date_data.date, &date_extents);

  /* Retrieve font extents, though not strictly used here except as an example.
   */
  cairo_font_extents_t font_extents;
  cairo_font_extents(screen_configs[screen_num].overlay_buffer, &font_extents);

  /* Center the text horizontally and place it at 1/5 of the screen height. */
  double date_x = (display_config->screen_info[screen_num].width / 2.0) -
                  (date_extents.width / 2.0) - date_extents.x_bearing;
  double date_y = (display_config->screen_info[screen_num].height / 5.0);

  /* Repaint background to ensure old text is cleared. */
  int repaint_x = (int)(date_x + date_extents.x_bearing);
  int repaint_y = (int)(date_y + date_extents.y_bearing);
  int repaint_width = (int)date_extents.width + font_extents.max_x_advance;
  int repaint_height = (int)date_extents.height + font_extents.max_y_advance;
  repaint_background_at(repaint_x, repaint_y, repaint_width, repaint_height,
                        screen_num);

  cairo_move_to(screen_configs[screen_num].overlay_buffer, date_x, date_y);
  cairo_show_text(screen_configs[screen_num].overlay_buffer,
                    g_date_data.date);

  /* --- Draw Clock --- */
  const double large_font_size = 150.0;
  cairo_set_font_size(screen_configs[screen_num].overlay_buffer,
                      large_font_size);

  cairo_text_extents_t clock_extents;
  cairo_text_extents(screen_configs[screen_num].overlay_buffer,
                     g_date_data.clock, &clock_extents);

  double clock_x = (display_config->screen_info[screen_num].width / 2.0) -
                   (clock_extents.width / 2.0) - clock_extents.x_bearing;
  /* Position clock below the date; add date_extents.height and
   * clock_extents.height. */
  double clock_y = (display_config->screen_info[screen_num].height / 5.0) +
                   date_extents.height + clock_extents.height;

  repaint_x = (int)(clock_x + clock_extents.x_bearing);
  repaint_y = (int)(clock_y + clock_extents.y_bearing);
  repaint_width = (int)clock_extents.width + font_extents.max_x_advance;
  repaint_height = (int)clock_extents.height + font_extents.max_y_advance;
  repaint_background_at(repaint_x, repaint_y, repaint_width, repaint_height,
                        screen_num);

  cairo_move_to(screen_configs[screen_num].overlay_buffer, clock_x, clock_y);
  cairo_show_text(screen_configs[screen_num].overlay_buffer,
                    g_date_data.clock);
}
