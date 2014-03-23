#ifndef KK_UI_WINDOW_H
#define KK_UI_WINDOW_H

#include <klingklang/event.h>
#include <klingklang/ui/cover.h>
#include <klingklang/ui/keys.h>
#include <klingklang/ui/progressbar.h>
#include <klingklang/ui/widget.h>
#include <klingklang/ui/window-events.h>

#include <pthread.h>

typedef struct kk_window_backend_s kk_window_backend_t;
typedef struct kk_window_input_state_s kk_window_input_state_t;
typedef struct kk_window_s kk_window_t;

struct kk_window_input_state_s {
  kk_window_t *window;
  int fd;
};

struct kk_window_s {
  kk_widget_t widget;
  kk_event_queue_t *events;
  kk_keys_t *keys;
  kk_cover_t *cover;
  kk_progressbar_t *progressbar;
  struct {
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    pthread_t thread;
  } draw;
  struct {
    unsigned alive:1;
    unsigned shown:1;
    unsigned title:1;
  } state;
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
int kk_window_update (kk_window_t *win);

int kk_window_get_input (kk_window_t *win);
int kk_window_set_title (kk_window_t *win, const char *title);

int kk_window_get_event_fd (kk_window_t *win);

#endif
