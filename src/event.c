#include <klingklang/event.h>

#include <fcntl.h>
#include <signal.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

int
kk_event_queue_init (kk_event_queue_t **queue)
{
  kk_event_queue_t *result;

  result = calloc (1, sizeof (kk_event_queue_t));
  if (result == NULL)
    goto error;

  if (pipe (result->fd) == -1)
    goto error;

  if (fcntl (result->fd[0], F_SETFL, O_NONBLOCK) == -1)
    goto error;

  if (fcntl (result->fd[1], F_SETFL, O_NONBLOCK) == -1)
    goto error;

  *queue = result;
  return 0;
error:
  kk_event_queue_free (result);
  *queue = NULL;
  return -1;
}

int
kk_event_queue_free (kk_event_queue_t *queue)
{
  if (queue == NULL)
    return 0;

  if (queue->fd[0])
    close (queue->fd[0]);
  if (queue->fd[1])
    close (queue->fd[1]);
  free (queue);
  return 0;
}

int
kk_event_queue_write (kk_event_queue_t *queue, void *ptr, size_t n)
{
  kk_event_t event;

  if (n > sizeof (kk_event_t))
    return -1;

  memset (&event, 0, sizeof (kk_event_t));
  memcpy (&event, ptr, n);
  if (write (queue->fd[1], &event, sizeof (kk_event_t)) <= 0)
    return -1;
  return 0;
}

int
kk_event_queue_get_read_fd (kk_event_queue_t *queue)
{
  return queue->fd[0];
}

int
kk_event_queue_get_write_fd (kk_event_queue_t *queue)
{
  return queue->fd[1];
}

int
kk_event_loop_init (kk_event_loop_t **loop, size_t cap)
{
  kk_event_loop_t *result;

  result = calloc (1, sizeof (kk_event_loop_t) + cap *sizeof (kk_event_handler_t));
  if (result == NULL)
    goto error;

  result->cap = cap;
  result->len = 0;
  *loop = result;
  return 0;
error:
  kk_event_loop_free (result);
  *loop = NULL;
  return -1;
}

int
kk_event_loop_free (kk_event_loop_t *loop)
{
  free (loop);
  return 0;
}

int
kk_event_loop_exit (kk_event_loop_t *loop)
{
  if (loop->running)
    loop->exit = 1;
  return 0;
}

int
kk_event_loop_add (kk_event_loop_t *loop, int fd, kk_event_func_f func, void *arg)
{
  kk_event_handler_t *handler;

  if (loop->len >= loop->cap)
    return -1;

  handler = loop->handler + loop->len;
  handler->fd = fd;
  handler->func = func;
  handler->arg = arg;

  if (fd > loop->mfd)
    loop->mfd = fd;
  loop->len++;
  return 0;
}

/**
 *Note: With some older glibc versions (pre 2.16), the calls FD_SET and 
 *FD_ISSET will cause compiler warnings about conversions from int to 
 *unsigned long int conversions. There's nothing we can do about. This
 *was fixed in glibc 2.16.
 */
static inline int
_kk_event_loop_dispatch (kk_event_loop_t *loop)
{
  kk_event_handler_t *handler;
  struct timeval tv;
  fd_set rfds;
  int r;

  FD_ZERO (&rfds);
  for (handler = loop->handler; handler < loop->handler + loop->len; handler++)
    FD_SET (handler->fd, &rfds);

  tv.tv_sec = 10;
  tv.tv_usec = 0;
  r = select (loop->mfd + 1, &rfds, NULL, NULL, &tv);
  if (r <= 0)
    return r;

  for (handler = loop->handler; handler < loop->handler + loop->len; handler++) {
    if (FD_ISSET (handler->fd, &rfds))
      handler->func (loop, handler->fd, handler->arg);
    if (loop->exit)
      return -1;
  }
  return 0;
}

int
kk_event_loop_run (kk_event_loop_t *loop)
{
  int r;

  loop->running = 1;
  loop->exit = 0;
  for (;;) {
    r = _kk_event_loop_dispatch (loop);
    if (r < 0)
      break;
  }
  loop->running = 0;
  loop->exit = 0;
  return 0;
}
