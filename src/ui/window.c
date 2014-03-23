#include <klingklang/ui/window.h>
#include <klingklang/util.h>

#include <errno.h>
#include <pthread.h>
#include <time.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif


extern const kk_window_backend_t window_backend;

static int
window_draw (kk_window_t *win)
{
  kk_log (KK_LOG_DEBUG, "Drawing window.");

  if (window_backend.draw (win) != 0)
    return -1;
  return 0;
}



static void *
window_draw_thread (kk_window_t *win) {
  struct timespec wakeup;
  int status;

  for (;;) {
    pthread_mutex_lock (&win->draw.mutex);

    /**
     * Wakeup in t + 1 seconds to redraw the window. Unless someone calls
     * the kk_window_update function, then we'll unblock and redraw immediately.
     */
    clock_gettime(CLOCK_REALTIME, &wakeup);
    wakeup.tv_sec++;

    status = pthread_cond_timedwait (&win->draw.cond, &win->draw.mutex, &wakeup);
    switch (status) {
      case 0:
        kk_log (KK_LOG_DEBUG, "User triggered redraw.");
        break;
      case ETIMEDOUT:
        kk_log (KK_LOG_DEBUG, "Timeout triggered redraw.");
        break;
    }

    window_draw (win);
    pthread_mutex_unlock (&win->draw.mutex);
  }
  return NULL;
}

int
kk_window_init (kk_window_t **win, int width, int height)
{
  kk_window_t *result;

  if (kk_widget_init ((kk_widget_t **) &result, window_backend.size) != 0)
    goto error;

  if (kk_event_queue_init (&result->events) != 0)
    goto error;

  if (kk_keys_init (&result->keys) != 0)
    goto error;

  result->width = width;
  result->height = height;

  if (window_backend.init (result) < 0)
    goto error;

  if (pthread_cond_init (&result->draw.cond, NULL) != 0)
    goto error;

  if (pthread_mutex_init (&result->draw.mutex, NULL) != 0)
    goto error;

  if (pthread_create (&result->draw.thread, NULL, (void *(*)(void *)) window_draw_thread, result) != 0)
    goto error;

  *win = result;
  return 0;
error:
  kk_window_free (result);
  *win = NULL;
  return -1;
}

int
kk_window_free (kk_window_t *win)
{
  if (win == NULL)
    return 0;

  window_backend.free (win);

  if (win->events)
    kk_event_queue_free (win->events);

  if (win->keys)
    kk_keys_free (win->keys);

  kk_widget_free ((kk_widget_t *) win);
  return 0;
}

int
kk_window_show (kk_window_t *win)
{
  if (window_backend.show (win) != 0)
    return -1;

  if (!win->has_title)
    kk_window_set_title (win, PACKAGE_NAME);

  kk_window_update (win);
  return 0;
}

int
kk_window_update (kk_window_t *win)
{
  kk_widget_invalidate ((kk_widget_t *) win);
  pthread_mutex_lock (&win->draw.mutex);
  pthread_cond_signal (&win->draw.cond);
  pthread_mutex_unlock (&win->draw.mutex);
  return 0;
}

int
kk_window_set_title (kk_window_t *win, const char *title)
{
  if (window_backend.set_title (win, title) != 0)
    return -1;
  win->has_title = 1;
  return 0;
}

static void *
window_input_reader (kk_window_input_state_t *state)
{
  static char buffer[1024] = { 0 };

  int err = 0;
  size_t tot = 0;
  ssize_t ret = 0;

  for (;;) {
    if (tot >= sizeof (buffer))
      break;

    errno = 0;
    ret = read (state->fd, buffer + tot, sizeof (buffer) - tot);
    err = errno;

    /* Break on error, but keep going on interrupts. */
    if ((ret <= 0) && (err != EINTR))
      break;

    if (ret > 0)
      tot += (size_t) ret;
  }

  if ((tot == 0) || (tot >= sizeof (buffer)))
    goto cleanup;

  /* Remove trailing newline. */
  if (buffer[tot - 1] == '\n')
    buffer[--tot] = '\0';

  if (tot > 0)
    kk_window_event_input (state->window->events, buffer);

cleanup:
  close (state->fd);
  free (state);
  return NULL;
}

static int
window_input_reader_detach (kk_window_t *win, int fd)
{
  kk_window_input_state_t *state = NULL;
  pthread_attr_t attr;
  pthread_t thread;

  state = calloc (1, sizeof (kk_window_input_state_t));
  if (state == NULL)
    return -1;

  state->window = win;
  state->fd = fd;

  if (pthread_attr_init (&attr) != 0)
    goto error;

  if (pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED) != 0)
    goto error;

  if (pthread_create (&thread, &attr, (void *(*)(void *)) window_input_reader, state) != 0)
    goto error;

  pthread_attr_destroy (&attr);
  return 0;
error:
  pthread_attr_destroy (&attr);
  free (state);
  return -1;
}

int
kk_window_get_input (kk_window_t *win)
{
  int pofd[2];                  /* stdout of child */
  int pifd[2];                  /* stdin of child */

  if (pipe (pifd) != 0)
    return -1;

  if (pipe (pofd) != 0)
    return -1;

  if (fork () == 0) {
    dup2 (pifd[0], STDIN_FILENO);
    dup2 (pofd[1], STDOUT_FILENO);

    close (pofd[0]);
    close (pifd[1]);

    setsid ();
    if (execlp ("dmenu", "dmenu", NULL) != 0)
      kk_err (EXIT_FAILURE, "execlp() failed.");
  }
  else {
    close (pifd[0]);
    close (pifd[1]);
    close (pofd[1]);

    if (window_input_reader_detach (win, pofd[0]) != 0) {
      close (pofd[0]);
      return -1;
    }
  }
  return 0;
}

int
kk_window_get_event_fd (kk_window_t *win)
{
  return kk_event_queue_get_read_fd (win->events);
}
