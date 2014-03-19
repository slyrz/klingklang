#include <klingklang/ui/window.h>
#include <klingklang/util.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <errno.h>

int
window_get_input (kk_event_queue_t *queue)
{
  static char buffer[1024] = { 0 };

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
    close (pofd[1]);

    /**
     * Upon start, dmenu wants to read the menu entries from stdin.
     * Since we use dmenu as simple text input, we don't pass menu
     * entries to dmenu. Therefore, we close stdin directly.
     */
    close (pifd[1]);

    int err = 0;
    ssize_t ret = 0;
    size_t tot = 0;

    for (;;) {
      if (tot >= sizeof (buffer))
        break;

      errno = 0;
      ret = read (pofd[0], buffer + tot, sizeof (buffer) - tot);
      err = errno;

      /* Break on error, but keep going on interrupts. */
      if ((ret <= 0) && (err != EINTR))
          break;

      if (ret > 0)
        tot += (size_t) ret;
    }

    /* No input read? Must have been canceled by user, so no error. */
    if (tot == 0)
      return 0;

    /* No input read? Must have been canceled by user, so no error. */
    if (tot >= sizeof (buffer))
      return -1;

    /* Remove trailing newline. */
    if (buffer[tot - 1] == '\n')
      buffer[--tot] = '\0';

    if (tot > 0)
      kk_window_event_input (queue, buffer);
  }

  return 0;
}