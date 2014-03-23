#ifndef KK_WINDOW_EVENTS_H
#define KK_WINDOW_EVENTS_H

#include <klingklang/event.h>

enum {
  KK_WINDOW_CLOSE,
  KK_WINDOW_INPUT,
  KK_WINDOW_KEY_PRESS,
};

typedef struct kk_window_event_close_s kk_window_event_close_t;
typedef struct kk_window_event_input_s kk_window_event_input_t;
typedef struct kk_window_event_key_press_s kk_window_event_key_press_t;

struct kk_window_event_close_s {
  kk_event_fields;
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

void kk_window_event_close (kk_event_queue_t *queue);
void kk_window_event_input (kk_event_queue_t *queue, char *text);
void kk_window_event_key_press (kk_event_queue_t *queue, int modifier, int key);

#endif
