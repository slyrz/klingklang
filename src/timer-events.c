#include <klingklang/timer-events.h>

void
kk_timer_event_fired (kk_event_queue_t *queue)
{
  kk_timer_event_fired_t event;

  memset (&event, 0, sizeof (kk_timer_event_fired_t));
  event.type = KK_TIMER_FIRED;
  kk_event_queue_write (queue, (void *) &event, sizeof (kk_timer_event_fired_t));
}
