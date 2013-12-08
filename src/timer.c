#include <klingklang/timer.h>
#include <klingklang/util.h>

#ifdef HAVE_SIGNAL_H
#  include <signal.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

static void
_kk_timer_fired (sigval_t val)
{
  kk_timer_event_fired_t event;
  kk_timer_t *timer = (kk_timer_t *) val.sival_ptr;

  memset (&event, 0, sizeof (kk_timer_event_fired_t));
  event.type = KK_TIMER_FIRED;
  kk_event_queue_write (timer->events, (void *) &event, sizeof (kk_timer_event_fired_t));
}

int
kk_timer_init (kk_timer_t **timer)
{
  kk_timer_t *result;

  result = calloc (1, sizeof (kk_timer_t));
  if (result == NULL)
    goto error;

  if (kk_event_queue_init (&result->events) != 0)
    goto error;

  *timer = result;
  return 0;
error:
  kk_timer_free (result);
  *timer = NULL;
  return -1;
}

int
kk_timer_free (kk_timer_t *timer)
{
  if (timer == NULL)
    return 0;

  if (timer->events)
    kk_event_queue_free (timer->events);

  if (timer->id) {
    struct itimerspec its;
    
    memset (&its, 0, sizeof (struct itimerspec));
    if (timer_settime (timer->id, 0, &its, NULL) != 0)
      kk_log (KK_LOG_WARNING, "Disarming timer failed.");

    if (timer_delete (timer->id) != 0)
      kk_log (KK_LOG_WARNING, "Deleting timer failed.");
  }

  free (timer);
  return 0;
}

int
kk_timer_start (kk_timer_t *timer, int seconds)
{
  struct sigevent sev;
  struct itimerspec its;

  /* Muy  importante! */
  memset (&sev, 0, sizeof (struct sigevent));
  memset (&its, 0, sizeof (struct itimerspec));

  sev.sigev_notify = SIGEV_THREAD;
  sev.sigev_notify_function = _kk_timer_fired;
  sev.sigev_value.sival_ptr = (void *) timer;

  if (timer_create (CLOCK_MONOTONIC, &sev, &timer->id) != 0)
    goto error;

  its.it_value.tv_sec = seconds;
  its.it_interval.tv_sec = seconds;

  if (timer_settime (timer->id, 0, &its, NULL) != 0)
    goto error;

  return 0;
error:
  if (timer->id)
    timer_delete (timer->id);
  timer->id = NULL;
  return -1;
}

int
kk_timer_get_event_fd (kk_timer_t *timer)
{
  return kk_event_queue_get_read_fd (timer->events);
}
