#ifndef DISPLAY_H
#define DISPLAY_H

#include <cairo/cairo.h>

void applyBlur(cairo_surface_t *surface, int radius);
void draw_password_entry(int screen_num);
void draw_clock(int screen_num);
void determine_text_color(cairo_surface_t *img, int width, int height);
void initialize_graphics();
void draw_graphics();
int get_opposite_color(int color);

#endif