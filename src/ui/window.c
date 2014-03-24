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
  if (!win->state.shown)
    return 0;

  if (window_backend.draw (win) != 0) {
    kk_log (KK_LOG_WARNING, "Drawing window failed.");
    return -1;
  }
  return 0;
}

static void
window_resize (kk_window_t *win, int width, int height)
{
  kk_widget_set_size ((kk_widget_t *) win->cover,  width, height - 4);
  kk_widget_set_size ((kk_widget_t *) win->progressbar, width, 4);
  kk_widget_set_position ((kk_widget_t *) win->cover, 0, 0);
  kk_widget_set_position ((kk_widget_t *) win->progressbar, 0, height - 4);
}

static void
window_draw_thread_cleanup (kk_window_t *win)
{
  win->state.alive = 0;
}

static void *
window_draw_thread (kk_window_t *win)
{
  struct timespec wakeup;
  int status;

  win->state.alive = 1;

  pthread_cleanup_push ((void (*)(void *)) window_draw_thread_cleanup, win);
  for (;;) {
    pthread_mutex_lock (&win->draw.mutex);

    /**
     * Wakeup in t + 1 seconds to redraw the window. Unless someone calls
     * the kk_window_update function, then we'll unblock and redraw
     * immediately.
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
  pthread_cleanup_pop (0);
  return NULL;
}

int
kk_window_init (kk_window_t **win, int width, int height)
{
  kk_window_t *result;

  if (kk_widget_init ((kk_widget_t **) &result, window_backend.size) != 0)
    goto error;

  if (kk_widget_bind_resize ((kk_widget_t *) result, (kk_widget_resize_f) window_resize) != 0)
    goto error;

  if (kk_event_queue_init (&result->events) != 0)
    goto error;

  if (kk_keys_init (&result->keys) != 0)
    goto error;

  result->widget.width = width;
  result->widget.height = height;

  if (window_backend.init (result) < 0)
    goto error;

  /* Initialize the window content. */
  if (kk_cover_init (&result->cover) != 0)
    goto error;

  if (kk_progressbar_init (&result->progressbar) != 0)
    goto error;

  kk_widget_add_child ((kk_widget_t*) result, (kk_widget_t*) result->cover);
  kk_widget_add_child ((kk_widget_t*) result, (kk_widget_t*) result->progressbar);

  /**
   * Update the size of the children, otherwise their position and size will
   * be uninitialized.
   */
   window_resize (result, width, height);

  /* Fire up the draw thread. */
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

  /**
   * If draw thread is up, cancel it and wait until it terminated before we
   * start freeing memory here.
   */
  if (win->state.alive) {
    if (pthread_cancel (win->draw.thread) == 0)
      pthread_join (win->draw.thread, NULL);
  }

  pthread_cond_destroy (&win->draw.cond);
  pthread_mutex_destroy (&win->draw.mutex);

  window_backend.free (win);

  kk_progressbar_free (win->progressbar);
  kk_cover_free (win->cover);

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
  kk_widget_invalidate ((kk_widget_t *) win);

  if (window_backend.show (win) != 0)
    return -1;

  if (!win->state.title)
    kk_window_set_title (win, PACKAGE_NAME);

  win->state.shown = 1;
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
  win->state.title = 1;
  return 0;
}

static void *
window_input_reader (kk_window_input_state_t *state)
{
  static char buffer[1024] = { 0 };

  size_t max = sizeof (buffer) - 1;
  size_t tot = 0;
  int err = 0;

  while (tot < max) {
    ssize_t ret;

    errno = 0;
    ret = read (state->fd, buffer + tot, max - tot);
    err = errno;

    /* Break on error, but keep going on interrupts. */
    if ((ret <= 0) && (err != EINTR))
      break;

    if (ret > 0)
      tot += (size_t) ret;
  }

  /* Something went wrong. */
  if ((tot == 0) || (err != 0))
    goto cleanup;

  /* Null-terminate string. */
  buffer[tot] = '\0';

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
