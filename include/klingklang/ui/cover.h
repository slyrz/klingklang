#ifndef KK_UI_COVER_H
#define KK_UI_COVER_H

#include <klingklang/base.h>
#include <klingklang/library.h>
#include <klingklang/ui/image.h>
#include <klingklang/ui/widget.h>

#include <cairo.h>

typedef struct kk_cover kk_cover_t;

struct kk_cover {
  kk_widget_t widget;
  kk_image_t *foreground;
  kk_image_t *background;
  cairo_pattern_t *contour;
  double radius;
  double darkness;
  double blur;
  char *path;
};

int kk_cover_init (kk_cover_t **cover);
int kk_cover_free (kk_cover_t *cover);
int kk_cover_load (kk_cover_t *cover, kk_library_file_t *file);

#endif
