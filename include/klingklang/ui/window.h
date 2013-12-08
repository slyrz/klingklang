#ifndef KK_UI_WINDOW_H
#define KK_UI_WINDOW_H

#include <klingklang/ui/widget.h>
#include <klingklang/ui/keys.h>
#include <klingklang/ui/window-events.h>

#include <pthread.h>

#include <cairo.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

typedef struct kk_window_s kk_window_t;

struct kk_window_s {
  kk_widget_fields;
  xcb_connection_t *conn;
  xcb_screen_t *scrn;
  xcb_window_t win;
  xcb_key_symbols_t *syms;
  xcb_atom_t input;
  cairo_surface_t *srf;
  cairo_t *ctx;
  kk_event_queue_t *events;
  pthread_t thread;
  unsigned alive:1;
};

int kk_window_init (kk_window_t ** win, int width, int height);
int kk_window_free (kk_window_t * win);
int kk_window_show (kk_window_t * win);
int kk_window_draw (kk_window_t * win);

int kk_window_get_input (kk_window_t * win);
int kk_window_set_title (kk_window_t * win, const char *title);

int kk_window_get_event_fd (kk_window_t * win);

#endif
