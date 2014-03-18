#ifndef KK_UI_WINDOW_H
#define KK_UI_WINDOW_H

#include <klingklang/event.h>
#include <klingklang/ui/widget.h>
#include <klingklang/ui/keys.h>
#include <klingklang/ui/window-events.h>

#include <pthread.h>

#define kk_window_fields \
  kk_widget_fields; \
  kk_event_queue_t *events; \
  kk_keys_t *keys; \
  pthread_t thread; \
  unsigned alive:1;

typedef struct kk_window_s kk_window_t;

int kk_window_init (kk_window_t **win, int width, int height);
int kk_window_free (kk_window_t *win);
int kk_window_show (kk_window_t *win);
int kk_window_draw (kk_window_t *win);

int kk_window_get_input (kk_window_t *win);
int kk_window_set_title (kk_window_t *win, const char *title);

int kk_window_get_event_fd (kk_window_t *win);

#endif
