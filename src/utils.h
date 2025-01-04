#ifndef UTILS_H
#define UTILS_H

/**
 * @file utils.h
 * @brief Utility functions for color handling and text color determination.
 */

#include <cairo/cairo.h>
#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* Function Declarations                                                     */
/* ------------------------------------------------------------------------- */

unsigned long hex_color_to_pixel(char *hex_color, int screen_num);
int get_opposite_color(int color);
void determine_text_color(cairo_surface_t *img, int width, int height);
void determine_text_color_for_color(double r, double g, double b);

#endif /* UTILS_H */
