#include <klingklang/timer.h>
#include <klingklang/util.h>

#ifdef HAVE_SIGNAL_H
#  include <signal.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

static kk_timer_t *active = NULL;

static void
timer_signal_handler (void)
{
  if (active)
    kk_timer_event_fired (active->events);
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

#ifdef HAVE_POSIX_TIMER_API
  if (timer->id) {
    struct itimerspec its;

    memset (&its, 0, sizeof (struct itimerspec));
    if (timer_settime (timer->id, 0, &its, NULL) != 0)
      kk_log (KK_LOG_WARNING, "Disarming timer failed.");

    if (timer_delete (timer->id) != 0)
      kk_log (KK_LOG_WARNING, "Deleting timer failed.");
  }
#else
  struct itimerval its;

  memset (&its, 0, sizeof (struct itimerval));
  if (setitimer (ITIMER_REAL, &its, NULL) != 0)
    kk_log (KK_LOG_WARNING, "Disarming timer failed.");
#endif
  if (timer == active)
    active = NULL;

  free (timer);
  return 0;
}

int
kk_timer_start (kk_timer_t *timer, int seconds)
{
  struct sigaction sac;

#ifdef HAVE_POSIX_TIMER_API
  struct itimerspec its;
  struct sigevent sev;
#else
  struct itimerval its;
#endif

  /**
   * The timer implementation used SIGEV_THREAD at first. This would have
   * allowed multipled timers. Sadly, some systems don't offer this functionality.
   * So we had to switch to plain simple signal handlers. And to avoid
   * needless complexity, these work best with one timer at a time.
   */
  if ((active) && (timer != active)) {
    kk_log (KK_LOG_WARNING, "Timer %p currently active.", active);
    return -1;
  }

  memset (&sac, 0, sizeof (struct sigaction));
#ifdef HAVE_POSIX_TIMER_API
  memset (&sev, 0, sizeof (struct sigevent));
  memset (&its, 0, sizeof (struct itimerspec));
#else
  memset (&its, 0, sizeof (struct itimerval));
#endif

  sac.sa_handler = (void (*)(int)) timer_signal_handler;
  sigemptyset (&sac.sa_mask);
  if (sigaction (SIGALRM, &sac, NULL) != 0)
    goto error;

  its.it_value.tv_sec = seconds;
  its.it_interval.tv_sec = seconds;

#ifdef HAVE_POSIX_TIMER_API
  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = SIGALRM;
  if (timer_create (CLOCK_MONOTONIC, &sev, &timer->id) != 0)
    goto error;

  if (timer_settime (timer->id, 0, &its, NULL) != 0)
    goto error;
#else
  if (setitimer (ITIMER_REAL, &its, NULL) != 0)
    goto error;
#endif
  active = timer;
  return 0;
error:
#ifdef HAVE_POSIX_TIMER_API
  if (timer->id)
    timer_delete (timer->id);
  timer->id = NULL;
#endif
  return -1;
}

int
kk_timer_get_event_fd (kk_timer_t *timer)
{
  return kk_event_queue_get_read_fd (timer->events);
}
