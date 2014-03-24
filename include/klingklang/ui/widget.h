#ifndef KK_UI_WIDGET_H
#define KK_UI_WIDGET_H

#include <klingklang/base.h>
#include <klingklang/list.h>

#include <cairo.h>

typedef struct kk_widget kk_widget_t;

typedef void (*kk_widget_draw_f) (kk_widget_t *, cairo_t *);
typedef void (*kk_widget_resize_f) (kk_widget_t *, int, int);

struct kk_widget {
  kk_list_t *children;
  int x;
  int y;
  int width;
  int height;
  struct {
    kk_widget_draw_f draw;
    kk_widget_resize_f resize;
  } callback;
  struct {
    unsigned redraw:1;
    unsigned resized:1;
  } state;
};

int kk_widget_init (kk_widget_t **widget, size_t size);
int kk_widget_free (kk_widget_t *widget);
int kk_widget_draw (kk_widget_t *widget, cairo_t *ctx);

int kk_widget_bind_draw (kk_widget_t *widget, kk_widget_draw_f func);
int kk_widget_bind_resize (kk_widget_t *widget, kk_widget_resize_f func);

int kk_widget_invalidate (kk_widget_t *widget);
int kk_widget_set_position (kk_widget_t *widget, int x, int y);
int kk_widget_set_size (kk_widget_t *widget, int width, int height);
int kk_widget_add_child (kk_widget_t *widget, kk_widget_t *child);

#endif
