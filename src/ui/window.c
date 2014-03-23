#include <klingklang/ui/window.h>
#include <klingklang/util.h>

#include <errno.h>
#include <pthread.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

extern const kk_window_backend_t window_backend;

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

  kk_widget_invalidate ((kk_widget_t *) win);
  kk_window_draw (win);
  return 0;
}

int
kk_window_draw (kk_window_t *win)
{
  if (window_backend.draw (win) != 0)
    return -1;
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
