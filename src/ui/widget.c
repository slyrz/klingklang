#include <klingklang/ui/widget.h>

int
kk_widget_init (kk_widget_t **widget, size_t size, kk_widget_draw_f draw)
{
  kk_widget_t *result = NULL;

  /**
   * We could set size to sizeof (kk_widget_t) and continue like nothing
   * happened, but this is not a good idea: if the caller forgot to allocate
   * large enough memory, he probably declared the struct without our
   * kk_widget_t fields.
   */
  if (size < sizeof (kk_widget_t))
    goto error;

  result = calloc (1, size);
  if (result == NULL)
    goto error;

  if (kk_list_init (&result->children) != 0)
    goto error;

  result->draw = draw;
  result->resized = 1;
  result->redraw = 1;

  *widget = result;
  return 0;
error:
  kk_widget_free (result);
  *widget = NULL;
  return -1;
}

int
kk_widget_free (kk_widget_t *widget)
{
  if (widget == NULL)
    return 0;

  kk_list_free (widget->children);
  free (widget);
  return 0;
}

int
kk_widget_draw (kk_widget_t *widget, cairo_t *ctx)
{
  size_t i;

  if (((widget->resized) | (widget->redraw)) && (widget->draw))
    widget->draw (widget, ctx);

  for (i = 0; i < widget->children->len; i++)
    kk_widget_draw ((kk_widget_t *) widget->children->items[i], ctx);

  widget->redraw = 0;
  widget->resized = 0;
  return 0;
}

int
kk_widget_invalidate (kk_widget_t *widget)
{
  size_t i;

  for (i = 0; i < widget->children->len; i++)
    kk_widget_invalidate ((kk_widget_t *) widget->children->items[i]);
  widget->redraw = 1;
  widget->resized = 1;
  return 0;
}

int
kk_widget_set_position (kk_widget_t *widget, int x, int y)
{
  if ((widget->x != x) | (widget->y != y))
    widget->resized = 1;

  widget->x = x;
  widget->y = y;
  return 0;
}

int
kk_widget_set_size (kk_widget_t *widget, int width, int height)
{
  if ((widget->width != width) | (widget->height != height))
    widget->resized = 1;

  widget->width = width;
  widget->height = height;
  return 0;
}

int
kk_widget_add_child (kk_widget_t *widget, kk_widget_t *child)
{
  return kk_list_append (widget->children, child);
}
