#ifndef KK_TIMER_H
#define KK_TIMER_H

#include <klingklang/base.h>
#include <klingklang/timer-events.h>

#ifdef HAVE_TIME_H
#  include <time.h>
#endif

typedef struct kk_timer_s kk_timer_t;

struct kk_timer_s {
  timer_t id;
  kk_event_queue_t *events;
};

int kk_timer_init (kk_timer_t ** timer);
int kk_timer_free (kk_timer_t * timer);
int kk_timer_start (kk_timer_t * timer, int seconds);

int kk_timer_get_event_fd (kk_timer_t * timer);

#endif
