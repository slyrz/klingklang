#ifndef KK_TIMER_H
#define KK_TIMER_H

#include <klingklang/base.h>
#include <klingklang/timer-events.h>

#ifdef HAVE_POSIX_TIMER_API
#  include <time.h>
#else
#  include <sys/time.h>
#endif

typedef struct kk_timer kk_timer_t;

struct kk_timer {
#ifdef HAVE_POSIX_TIMER_API
  timer_t id;
#endif
  kk_event_queue_t *events;
};

int kk_timer_init (kk_timer_t **timer);
int kk_timer_free (kk_timer_t *timer);
int kk_timer_start (kk_timer_t *timer, int seconds);

int kk_timer_get_event_fd (kk_timer_t *timer);

#endif
