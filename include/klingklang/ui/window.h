#ifndef KK_UI_WINDOW_H
#define KK_UI_WINDOW_H

#include <klingklang/event.h>
#include <klingklang/ui/widget.h>
#include <klingklang/ui/keys.h>
#include <klingklang/ui/window-events.h>
#include <klingklang/ui/window-input.h>

#include <pthread.h>

typedef struct kk_window_s kk_window_t;
typedef struct kk_window_backend_s kk_window_backend_t;

struct kk_window_s {
  kk_widget_fields;
  kk_event_queue_t *events;
  kk_keys_t *keys;

  unsigned has_title:1;
  unsigned is_alive:1;
};

struct kk_window_backend_s {
  size_t size;
  int (*init) (kk_window_t *win);
  int (*free) (kk_window_t *win);
  int (*show) (kk_window_t *win);
  int (*draw) (kk_window_t *win);
  int (*set_title) (kk_window_t *win, const char *title);
};

int kk_window_init (kk_window_t **win, int width, int height);
int kk_window_free (kk_window_t *win);
int kk_window_show (kk_window_t *win);
int kk_window_draw (kk_window_t *win);

int kk_window_get_input (kk_window_t *win);
int kk_window_set_title (kk_window_t *win, const char *title);

int kk_window_get_event_fd (kk_window_t *win);

#endif
