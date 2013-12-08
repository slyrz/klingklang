#ifndef KK_WINDOW_EVENTS_H
#define KK_WINDOW_EVENTS_H

#include <klingklang/event.h>

enum {
  KK_WINDOW_CLOSE,
  KK_WINDOW_EXPOSE,
  KK_WINDOW_INPUT,
  KK_WINDOW_KEY_PRESS,
  KK_WINDOW_RESIZE
};

typedef struct kk_window_event_close_s kk_window_event_close_t;
typedef struct kk_window_event_expose_s kk_window_event_expose_t;
typedef struct kk_window_event_input_s kk_window_event_input_t;
typedef struct kk_window_event_key_press_s kk_window_event_key_press_t;
typedef struct kk_window_event_resize_s kk_window_event_resize_t;

struct kk_window_event_close_s {
  kk_event_fields;
};

struct kk_window_event_expose_s {
  kk_event_fields;
  int width;
  int height;
};

struct kk_window_event_input_s {
  kk_event_fields;
  char *text;
};

struct kk_window_event_key_press_s {
  kk_event_fields;
  int mod;
  int key;
};

struct kk_window_event_resize_s {
  kk_event_fields;
  int width;
  int height;
};

#endif
