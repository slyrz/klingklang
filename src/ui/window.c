#include <klingklang/ui/window.h>

extern const kk_window_backend_t kk_window_backend;

int
kk_window_init (kk_window_t **win, int width, int height)
{
  kk_window_t *result;

  if (kk_widget_init ((kk_widget_t **) &result, kk_window_backend.size, NULL) != 0)
    goto error;

  if (kk_event_queue_init (&result->events) != 0)
    goto error;

  if (kk_keys_init (&result->keys) != 0)
    goto error;

  result->width = width;
  result->height = height;

  if (kk_window_backend.init (result) < 0)
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

  kk_window_backend.free (win);

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
  if (kk_window_backend.show (win) != 0)
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
  if (kk_window_backend.draw (win) != 0)
    return -1;
  return 0;
}

int
kk_window_set_title (kk_window_t *win, const char *title)
{
  if (kk_window_backend.set_title (win, title) != 0)
    return -1;
  win->has_title = 1;
  return 0;
}

extern int window_get_input (kk_event_queue_t * queue);

int
kk_window_get_input (kk_window_t *win)
{
  return kk_window_input_request (win->events);
}

int
kk_window_get_event_fd (kk_window_t *win)
{
  return kk_event_queue_get_read_fd (win->events);
}
