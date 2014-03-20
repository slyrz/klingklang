#include <klingklang/ui/window.h>
#include <klingklang/util.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <errno.h>
#include <pthread.h>

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
    kk_window_event_input (state->queue, buffer);

cleanup:
  close (state->fd);
  free (state);
  return NULL;
}

static int
window_input_detach_read (kk_event_queue_t *queue, int fd)
{
  kk_window_input_state_t *state = NULL;
  pthread_attr_t attr;
  pthread_t thread;

  state = calloc (1, sizeof (kk_window_input_state_t));
  if (state == NULL)
    return -1;

  state->queue = queue;
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
kk_window_input_request (kk_event_queue_t *queue)
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

    if (window_input_detach_read (queue, pofd[0]) != 0) {
      close (pofd[0]);
      return -1;
    }
  }
  return 0;
}

