#include <klingklang/ui/window-events.h>

void
kk_window_event_close (kk_event_queue_t *queue)
{
  kk_window_event_close_t event;

  memset (&event, 0, sizeof (kk_window_event_close_t));
  event.type = KK_WINDOW_CLOSE;
  kk_event_queue_write (queue, (void *) &event, sizeof (kk_window_event_close_t));
}

void
kk_window_event_expose (kk_event_queue_t *queue, int width, int height)
{
  kk_window_event_expose_t event;

  memset (&event, 0, sizeof (kk_window_event_expose_t));
  event.type = KK_WINDOW_EXPOSE;
  event.width = width;
  event.height = height;
  kk_event_queue_write (queue, (void *) &event, sizeof (kk_window_event_expose_t));
}

void
kk_window_event_input (kk_event_queue_t *queue, char *text)
{
  kk_window_event_input_t event;

  memset (&event, 0, sizeof (kk_window_event_input_t));
  event.type = KK_WINDOW_INPUT;
  event.text = text;
  kk_event_queue_write (queue, (void *) &event, sizeof (kk_window_event_input_t));
}

void
kk_window_event_key_press (kk_event_queue_t *queue, int modifier, int key)
{
  kk_window_event_key_press_t event;

  memset (&event, 0, sizeof (kk_window_event_key_press_t));
  event.type = KK_WINDOW_KEY_PRESS;
  event.key = key;
  event.mod = modifier;
  kk_event_queue_write (queue, (void *) &event, sizeof (kk_window_event_key_press_t));
}

void
kk_window_event_resize (kk_event_queue_t *queue, int width, int height)
{
  kk_window_event_resize_t event;

  memset (&event, 0, sizeof (kk_window_event_resize_t));
  event.type = KK_WINDOW_RESIZE;
  event.width = width;
  event.height = height;
  kk_event_queue_write (queue, (void *) &event, sizeof (kk_window_event_resize_t));
}
