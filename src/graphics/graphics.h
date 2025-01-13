#ifndef GRAPHICS_H
#define GRAPHICS_H

/**
 * @file graphics.h
 * @brief Provides declarations for graphics-related operations,
 *        including rendering and color manipulation.
 */

#include <X11/Xlib.h>
#include <cairo/cairo.h>

/* ------------------------------------------------------------------------- */
/* Function Declarations                                                     */
/* ------------------------------------------------------------------------- */

void draw_password_entry(int screen_num);
void draw_clock(int screen_num);
void initialize_graphics(void);
void draw_graphics(void);
int get_opposite_color(int color);
void repaint_background_at(int x, int y, int width, int height, int screen_num);
void exit_cleanup(void);
void request_redraw(Display *display);
#endif /* GRAPHICS_H */
