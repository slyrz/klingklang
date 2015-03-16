#include <klingklang/base.h>
#include <klingklang/ui/progressbar.h>

#include <cairo.h>
#include <math.h>               /* fabs */

static inline int
progressbar_is_active (kk_progressbar_t *progressbar)
{
  return fabs (progressbar->progress) > 0.001;
}

static inline int
progressbar_is_progress (kk_progressbar_t *progressbar, double value)
{
  return fabs (progressbar->progress - value) > 0.005;
}

static void
progressbar_draw (kk_widget_t *widget, cairo_t *ctx)
{
  kk_progressbar_t *progressbar = (kk_progressbar_t *) widget;

  cairo_pattern_t *pat;

  pat = cairo_pattern_create_linear (
      (double) 0.0,
      (double) 0.0,
      (double) progressbar->widget.width,
      (double) 0.0);

  if (cairo_pattern_status (pat) != CAIRO_STATUS_SUCCESS)
    return;

  cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.75, 0.00, 0.98, 0.5);
  cairo_pattern_add_color_stop_rgba (pat, 0.5, 0.92, 0.03, 0.41, 0.5);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.18, 0.09, 0.21, 0.5);

  cairo_rectangle (ctx,
      (double) progressbar->widget.x,
      (double) progressbar->widget.y,
      (double) progressbar->widget.width,
      (double) progressbar->widget.height);

  cairo_set_source_rgb (ctx, 0.0, 0.0, 0.0);
  cairo_fill_preserve (ctx);
  cairo_set_source (ctx, pat);
  cairo_fill (ctx);
  cairo_pattern_destroy (pat);
  cairo_set_source_rgb (ctx, 0.0, 0.0, 0.0);
  cairo_rectangle (ctx,
      (double) progressbar->widget.x + ((double) progressbar->widget.width * progressbar->progress),
      (double) progressbar->widget.y,
      (double) progressbar->widget.width * 1.0 - (double) progressbar->progress,
      (double) progressbar->widget.height);
  cairo_fill (ctx);
}

int
kk_progressbar_init (kk_progressbar_t **progressbar)
{
  kk_progressbar_t *result = NULL;

  if (kk_widget_init ((kk_widget_t **) &result, sizeof (kk_progressbar_t)) != 0)
    goto error;

  if (kk_widget_bind_draw ((kk_widget_t *) result, progressbar_draw) != 0)
    goto error;

  *progressbar = result;
  return 0;
error:
  *progressbar = NULL;
  return -1;
}

int
kk_progressbar_free (kk_progressbar_t *progressbar)
{
  if (progressbar == NULL)
    return 0;
  kk_widget_free ((kk_widget_t *) progressbar);
  return 0;
}

int
kk_progressbar_set_value (kk_progressbar_t *progressbar, double value)
{
  if ((value < 0.0) || (value > 1.0))
    return -1;

  /* Check if value changed a bit, otherwise ignore */
  if (progressbar_is_progress (progressbar, value)) {
    progressbar->progress = value;
    kk_widget_invalidate ((kk_widget_t *) progressbar);
  }
  return 0;
}
