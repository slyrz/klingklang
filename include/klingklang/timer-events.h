#ifndef KK_TIMER_EVENTS_H
#define KK_TIMER_EVENTS_H

#include <klingklang/event.h>

enum {
  KK_TIMER_FIRED
};

typedef struct kk_timer_event_fired_s kk_timer_event_fired_t;

struct kk_timer_event_fired_s {
  kk_event_fields;
};

void kk_timer_event_fired (kk_event_queue_t *queue);

#endif
