#ifndef GRAPHICS_H
#define GRAPHICS_H

/**
 * @file graphics.h
 * @brief Provides declarations for graphics-related operations,
 *        including rendering and color manipulation.
 */

#include <cairo/cairo.h>

/* ------------------------------------------------------------------------- */
/* Function Declarations                                                     */
/* ------------------------------------------------------------------------- */

void draw_password_entry(int screen_num);
void draw_clock(int screen_num);
void determine_text_color(cairo_surface_t *img, int width, int height);
void initialize_graphics(void);
void draw_graphics(void);
int get_opposite_color(int color);
void repaint_background_at(int x, int y, int width, int height, int screen_num);

#endif /* GRAPHICS_H */
