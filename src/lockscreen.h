#ifndef LOCKSCREEN_H
#define LOCKSCREEN_H

/**
 * @file lockscreen.h
 * @brief Declarations for the lockscreen system, including windows, buffers,
 *        and lock screen lifecycle management.
 */

#include <X11/extensions/Xinerama.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <cairo/cairo.h>

/* ------------------------------------------------------------------------- */
/* Structure Definitions                                                     */
/* ------------------------------------------------------------------------- */

/**
 * @brief Holds all objects and state relevant to a single screen in the lockscreen.
 */
struct ScreenConfig {
    Window window;                  /**< The X11 window for this screen. */
    Visual *visual;                 /**< Visual for the window. */
    cairo_surface_t *surface;       /**< Main surface for rendering. */
    cairo_t *overlay_buffer;        /**< Overlay buffer for drawing text. */
    cairo_t *background_buffer;     /**< Background buffer for images or colors. */
    cairo_t *screen_buffer;         /**< Combined buffer for final compositing. */
    cairo_surface_t *off_screen_buffer; /**< Off-screen surface for temporary drawing. */
    int text_color;                 /**< Numeric color value (0-255). */
    cairo_pattern_t *pattern;       /**< Cairo pattern for rendering backgrounds. */
};

/**
 * @brief Holds global display information, including multiple screens and a reference image.
 */
struct DisplayConfig {
    Display *display;              /**< Pointer to the opened X display. */
    GC gc;                         /**< Graphics context for low-level X operations. */
    int num_screens;               /**< Number of screens available via Xinerama. */
    int yFontCoordinate;           /**< Y-axis coordinate used for some text rendering. */
    XineramaScreenInfo *screen_info; /**< Xinerama screen info for multi-screen support. */
    cairo_surface_t *image_surface;  /**< Optional image surface for backgrounds. */
};

/* ------------------------------------------------------------------------- */
/* Global Variable Declarations                                              */
/* ------------------------------------------------------------------------- */

/**
 * @brief Array of ScreenConfig objects, one per screen. TODO: Make this dynamic.
 */
extern struct ScreenConfig screen_configs[128];

/**
 * @brief Pointer to global display configuration.
 */
extern struct DisplayConfig *display_config;

/**
 * @brief Current index into the password input buffer.
 */
extern int current_input_index;

/**
 * @brief Indicates whether the last password attempt was wrong (1) or not (0).
 */
extern int password_is_wrong;

/**
 * @brief Flag controlling the lockscreen’s main event loop. 1 = running, 0 = stopped.
 */
extern int lockscreen_running;

/* ------------------------------------------------------------------------- */
/* Function Declarations                                                     */
/* ------------------------------------------------------------------------- */

int lockscreen(void);

#endif /* LOCKSCREEN_H */
