#ifndef KK_UI_IMAGE_H
#define KK_UI_IMAGE_H

#include <cairo.h>

typedef struct kk_image_s kk_image_t;

struct kk_image_s {
  cairo_surface_t *surface;
  unsigned char *buffer;
  int width;
  int height;
};

int kk_image_init (kk_image_t **img, const char *path);
int kk_image_free (kk_image_t *img);
int kk_image_blur (kk_image_t *img, double intensity);

#endif
