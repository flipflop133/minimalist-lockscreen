/**
 * @file utils.c
 * @brief Utility functions for color manipulation and text-color determination.
 */

#include "utils.h"
#include "graphics/graphics.h"
#include "lockscreen.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Converts a hexadecimal color code to its corresponding X11 pixel
 * value.
 *
 * @param hex_color The hexadecimal color code (e.g., "#FFFFFF").
 * @param screen_num The screen index for which to allocate the color.
 * @return The X11 pixel value corresponding to the color.
 */
unsigned long hex_color_to_pixel(char *hex_color, int screen_num) {
  XColor color;
  Colormap colormap = DefaultColormap(display_config->display, screen_num);

  if (!XParseColor(display_config->display, colormap, hex_color, &color)) {
    fprintf(stderr, "Warning: Failed to parse color %s.\n", hex_color);
  }
  if (!XAllocColor(display_config->display, colormap, &color)) {
    fprintf(stderr, "Warning: Failed to allocate color %s.\n", hex_color);
  }
  return color.pixel;
}

/**
 * @brief Computes the complementary (opposite) color value for a given integer
 * color.
 *
 * @param color The original color component in the 0-255 range.
 * @return The inverted color (255 - color).
 */
int get_opposite_color(int color) {
  const int max_color_value = 255;
  return max_color_value - color;
}

/**
 * @brief Determines and sets the text color in `screen_configs->text_color`
 *        based on average image brightness or a default color.
 *
 * If the average of the image's pixels is below the midpoint, the text color
 * is set to 255; otherwise it remains 0 (unless overridden by logic below).
 *
 * @param img The Cairo surface representing the image.
 * @param width The width of the image.
 * @param height The height of the image.
 */
void determine_text_color(cairo_surface_t *img, int width, int height) {
  /* Default color used if no image is provided. */
  char default_color_hex[] = "a3a3a3";
  int text_color = 0; /* Default to black-ish if no reasons to invert. */

  if (img != NULL) {
    long red_total = 0;
    long green_total = 0;
    long blue_total = 0;
    long total_pixels = 0;

    /* Access pixel data. */
    uint32_t *data = (uint32_t *)cairo_image_surface_get_data(img);
    if (data == NULL) {
      fprintf(stderr, "Warning: Unable to access image data.\n");
    } else {
      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
          /* Retrieve pixel (ARGB in memory, but we only use RGB). */
          uint32_t pixel_color = data[x + (y * width)];
          uint8_t red = (pixel_color >> 16) & 0xFF;
          uint8_t green = (pixel_color >> 8) & 0xFF;
          uint8_t blue = pixel_color & 0xFF;

          red_total += red;
          green_total += green;
          blue_total += blue;
          total_pixels++;
        }
      }
    }

    /* Compute the average color components. */
    if (total_pixels > 0) {
      red_total /= total_pixels;
      green_total /= total_pixels;
      blue_total /= total_pixels;
    }

    /* If the average of each channel is below ~128, switch text color to white.
     */
    if (red_total < 128 && green_total < 128 && blue_total < 128) {
      text_color = 255;
    }
  } else {
    /* If no image is provided, we do a rough check of the default color. */
    int sum = 0;
    for (size_t i = 0; i < strlen(default_color_hex); i++) {
      /* Convert hex char to 0-15 and sum up. */
      char c = (char)tolower((unsigned char)default_color_hex[i]);
      int val = 0;
      if (c >= '0' && c <= '9') {
        val = c - '0';
      } else if (c >= 'a' && c <= 'f') {
        val = 10 + (c - 'a');
      }
      sum += val;
    }
    /* If the average is below roughly half the maximum, set text color to 255.
     */
    if ((double)sum / (2.0) < 127.5) {
      text_color = 255;
    }
  }

  /* Apply to the first screen_config. If multiple screens are used,
     you can adapt logic to apply individually per screen. */
  screen_configs->text_color = text_color;
}

/**
 * @brief Determines and sets the text color (`screen_configs->text_color`)
 *        based on the brightness of a solid color background.
 *
 * If the perceived brightness of the background color is below a threshold,
 * the text color is set to white (`255`). Otherwise, it is set to black (`0`).
 *
 * @param r Red component of the color (0–255).
 * @param g Green component of the color (0–255).
 * @param b Blue component of the color (0–255).
 * @param a Alpha component of the color (0–255, optional, ignored in this
 * case).
 */
void determine_text_color_for_color(double r, double g, double b) {

  /* Calculate perceived brightness using the formula:
     Brightness = 0.2126 * R + 0.7152 * G + 0.0722 * B
  */
  double brightness = 0.2126 * r + 0.7152 * g + 0.0722 * b;

  /* Threshold for switching to white text. */
  const double brightness_threshold =
      0.5; // Adjusted for normalized range [0, 1]

  /* Determine the text color based on brightness. */
  if (brightness < brightness_threshold) {
    screen_configs->text_color = 255; // White
  } else {
    screen_configs->text_color = 0; // Black
  }
}
