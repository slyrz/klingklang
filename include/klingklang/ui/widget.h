#ifndef KK_WIDGET_H
#define KK_WIDGET_H

#include <klingklang/base.h>
#include <klingklang/list.h>

#include <cairo.h>

#define kk_widget_fields \
  kk_list_t *children; \
  kk_widget_draw_f _draw; \
  int x; \
  int y; \
  int width; \
  int height; \
  unsigned redraw:1; \
  unsigned resized:1;

typedef struct kk_widget_s kk_widget_t;

typedef void (*kk_widget_draw_f) (kk_widget_t *, cairo_t *);

struct kk_widget_s {
  kk_widget_fields;
};

int kk_widget_init (kk_widget_t **widget, size_t size, kk_widget_draw_f draw);
int kk_widget_free (kk_widget_t *widget);
int kk_widget_draw (kk_widget_t *widget, cairo_t *ctx);

int kk_widget_invalidate (kk_widget_t *widget);
int kk_widget_set_position (kk_widget_t *widget, int x, int y);
int kk_widget_set_size (kk_widget_t *widget, int width, int height);
int kk_widget_add_child (kk_widget_t *widget, kk_widget_t *child);

#endif
