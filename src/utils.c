#include "utils.h"
#include "defs.h"
#include "graphics/graphics.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * Converts a hexadecimal color code to a pixel value.
 *
 * @param hex_color The hexadecimal color code to convert.
 * @param screen_num The screen number to convert the color for.
 * @return The pixel value corresponding to the given color code.
 */
unsigned long hex_color_to_pixel(char *hex_color, int screen_num) {
  XColor color;
  Colormap colormap = DefaultColormap(display_config->display, screen_num);
  XParseColor(display_config->display, colormap, hex_color, &color);
  XAllocColor(display_config->display, colormap, &color);
  return color.pixel;
}

/**
 * Calculates the opposite color of the given color.
 *
 * @param color The color value to calculate the opposite color for.
 * @return The opposite color value.
 */
int get_opposite_color(int color) {
  int max_color_value = 255;
  return max_color_value - color;
}

/**
 * Determines the text color for the given image.
 *
 * @param img The Cairo surface representing the image.
 * @param width The width of the image.
 * @param height The height of the image.
 */
void determine_text_color(cairo_surface_t *img, int width, int height) {
  char color[7] = "a3a3a3";
  int text_color = 0;
  if (img != NULL) {
    long red_tot = 0;
    long green_tot = 0;
    long blue_tot = 0;
    long tot = 0;
    for (int i = 0; i < width; i++) {
      for (int j = 0; j < height; j++) {
        uint32_t *data = (uint32_t *)cairo_image_surface_get_data(img);
        uint32_t pixel_color = data[i + j];
        // Extract individual color components
        uint8_t red = (pixel_color >> 16) & 0xFF;
        red_tot += red;
        uint8_t green = (pixel_color >> 8) & 0xFF;
        green_tot += green;
        uint8_t blue = pixel_color & 0xFF;
        blue_tot += blue;
        tot += 1;
      }
    }
    red_tot /= tot;
    green_tot /= tot;
    blue_tot /= tot;
    if (red_tot < 127.5 && green_tot < 127.5 && blue_tot < 127.5) {
      text_color = 255;
    }
  } else {
    int tot = 0;
    for (size_t c = 0; c < (strlen(color)); c++) {
      tot += color[c] - '0';
    }
    if ((double)tot / 2 < 127.5) {
      text_color = 255;
    }
  }
  screen_configs->text_color = text_color;
}