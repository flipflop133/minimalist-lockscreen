/**
 * @file graphics.c
 * @brief Provides functions for loading a background image, initializing
 *        per-screen graphics contexts, drawing overlays, and repainting
 *        backgrounds.
 */

#include "graphics.h"
#include "../args.h"
#include "../defs.h"
#include "../lockscreen.h"
#include <X11/Xlib.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <malloc.h>
#include <pthread.h>
#include <stdio.h>

/* ------------------------------------------------------------------------- */
/* Forward Declarations                                                      */
/* ------------------------------------------------------------------------- */
static cairo_surface_t *load_background_image(void);
static int setup_screen(int screen_num, cairo_surface_t *image_surface);

/* ------------------------------------------------------------------------- */
/* Function Definitions                                                      */
/* ------------------------------------------------------------------------- */

/**
 * @brief Loads a background image from the file path specified by the
 *        "--image" command-line argument.
 *
 * @return A pointer to the created Cairo surface, or NULL if loading failed.
 */
static cairo_surface_t *load_background_image(void) {
  const char *image_path = retrieve_command_arg("--image");
  if (!image_path) {
    fprintf(stderr, "No --image argument provided.\n");
    return NULL;
  }

  /* Create a Cairo surface from the PNG file. */
  cairo_surface_t *surface = cairo_image_surface_create_from_png(image_path);
  if (!surface) {
    fprintf(stderr, "Unable to create surface from file: %s\n", image_path);
    return NULL;
  }

  /* Check if the surface was created successfully. */
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
    fprintf(stderr, "Failed to load image: %s\n", image_path);
    cairo_surface_destroy(surface);
    return NULL;
  }

  return surface;
}

/**
 * @brief Sets up the graphics objects (surfaces, contexts, patterns) for
 *        one screen, based on the given background image surface.
 *
 * @param screen_num Index of the screen to set up.
 * @param image_surface The Cairo surface containing the loaded background
 * image.
 * @return 0 on success, non-zero on failure.
 */
static int setup_screen(int screen_num, cairo_surface_t *image_surface) {
  screen_configs[screen_num].visual = DefaultVisual(
      display_config->display, DefaultScreen(display_config->display));

  /*
   * The Xlib-backed surface for the *on-screen* drawing
   * tied to this screen's window.
   */
  screen_configs[screen_num].surface = cairo_xlib_surface_create(
      display_config->display, screen_configs[screen_num].window,
      screen_configs[screen_num].visual,
      display_config->screen_info[screen_num].width,
      display_config->screen_info[screen_num].height);

  if (!screen_configs[screen_num].surface) {
    fprintf(stderr, "Unable to create cairo_xlib_surface for screen %d\n",
            screen_num);
    return -1;
  }

  /* Main on-screen context. */
  screen_configs[screen_num].screen_buffer =
      cairo_create(screen_configs[screen_num].surface);

  /* Off-screen surface for layering (with alpha). */
  screen_configs[screen_num].off_screen_buffer = cairo_surface_create_similar(
      screen_configs[screen_num].surface, CAIRO_CONTENT_COLOR_ALPHA,
      display_config->screen_info[screen_num].width,
      display_config->screen_info[screen_num].height);

  /* Two contexts on the off-screen: overlay and background. */
  screen_configs[screen_num].overlay_buffer =
      cairo_create(screen_configs[screen_num].off_screen_buffer);
  screen_configs[screen_num].background_buffer =
      cairo_create(screen_configs[screen_num].off_screen_buffer);

  /* Set a font face on overlay context (just an example). */
  cairo_font_face_t *font_face = cairo_toy_font_face_create(
      "JetBrainsMono NF", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_face(screen_configs[screen_num].overlay_buffer, font_face);
  cairo_font_face_destroy(font_face); // the context holds its own reference

  /*
   *  Create a pattern from the loaded image surface, scale to fit,
   *  paint onto background_buffer, then free the pattern.
   */
  cairo_pattern_t *pattern = cairo_pattern_create_for_surface(image_surface);

  /* Calculate scaling so the image fits the screen. */
  double image_width = (double)cairo_image_surface_get_width(image_surface);
  double image_height = (double)cairo_image_surface_get_height(image_surface);
  double screen_width = (double)display_config->screen_info[screen_num].width;
  double screen_height = (double)display_config->screen_info[screen_num].height;

  double x_scale = image_width / screen_width;
  double y_scale = image_height / screen_height;
  double scale_factor = (x_scale < y_scale) ? x_scale : y_scale;

  /* Invert the scale factor so we scale the pattern down to the screen size.
   */
  cairo_matrix_t matrix;
  cairo_matrix_init_scale(&matrix, scale_factor, scale_factor);
  cairo_pattern_set_matrix(pattern, &matrix);

  /* Paint background once onto background_buffer. */
  cairo_set_source(screen_configs[screen_num].background_buffer, pattern);
  cairo_paint(screen_configs[screen_num].background_buffer);

  /* We no longer need the pattern once painted. */
  cairo_pattern_destroy(pattern);

  /*
   * Optionally analyze the loaded background for text color.
   * (If you do it here, it must happen before we destroy 'pattern'
   * or the 'image_surface' in the caller.)
   */
  determine_text_color(image_surface,
                       display_config->screen_info[screen_num].width,
                       display_config->screen_info[screen_num].height);

  return 0; // Success
}

/**
 * @brief Initializes graphics resources for the lockscreen.
 *
 * This includes setting up surfaces, contexts, and loading the background
 * image exactly once. After painting each screen's background, we free
 * the original image surface to reduce memory usage.
 */
void initialize_graphics(void) {
  /* 1) Load the background image from the command-line argument. */
  display_config->image_surface = load_background_image();
  if (!display_config->image_surface) {
    fprintf(stderr, "Failed to load background image.\n");
    return;
  }

  /* 2) Initialize each screen using the loaded image. */
  for (int screen_num = 0; screen_num < display_config->num_screens;
       screen_num++) {
    if (setup_screen(screen_num, display_config->image_surface) != 0) {
      fprintf(stderr, "Failed to initialize screen %d.\n", screen_num);
      return;
    }
  }

  /*
   * 3) Now that each screen’s background_buffer has a copy
   *    of the image, free the original surface to avoid
   *    keeping large image data in memory.
   */
  cairo_surface_destroy(display_config->image_surface);
  display_config->image_surface = NULL;
  malloc_trim(0);
  /* 4) Perform an initial draw to make the lock screen visible. */
  draw_graphics();
}

/**
 * @brief Draw the final screen content by compositing the off-screen buffers
 *        onto the on-screen surfaces.
 */
void draw_graphics(void) {
  /* Ensure thread safety if multiple threads call drawing functions. */
  pthread_mutex_lock(&mutex);

  /* Redraw UI elements on each screen. */
  for (int screen_num = 0; screen_num < display_config->num_screens;
       screen_num++) {
    /* First draw overlay components like password entry and clock. */
    draw_password_entry(screen_num);
    draw_clock(screen_num);

    /* Paint the off-screen content onto the on-screen context. */
    cairo_set_source_surface(screen_configs[screen_num].screen_buffer,
                             screen_configs[screen_num].off_screen_buffer, 0,
                             0);
    cairo_paint(screen_configs[screen_num].screen_buffer);
  }

  pthread_mutex_unlock(&mutex);
}

/**
 * @brief Repaints the background in a specific rectangular region by
 *        copying from the background_buffer.
 *
 * This can be used to "erase" overlays or other elements.
 *
 * @param x The x-coordinate of the top-left corner of the rectangle.
 * @param y The y-coordinate of the top-left corner of the rectangle.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @param screen_num The index of the screen where the operation will occur.
 */
void repaint_background_at(int x, int y, int width, int height, int screen_num)
{
  cairo_t *bg_cr = screen_configs[screen_num].background_buffer;

  /* Save the context state. */
  cairo_save(bg_cr);

  /* Clip to the specified rectangle. */
  cairo_reset_clip(bg_cr);
  cairo_rectangle(bg_cr, x, y, width, height);
  cairo_clip(bg_cr);

  /* Use CAIRO_OPERATOR_SOURCE to overwrite the old content fully. */
  cairo_set_operator(bg_cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint(bg_cr);

  /* Restore context state. */
  cairo_restore(bg_cr);
}
