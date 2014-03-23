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

  /**
   * If there's some progress, paint colored progress bar.
   */
  if (progressbar_is_active (progressbar)) {
    cairo_set_source_rgb (ctx, 0.94, 0.85, 0.62);
    cairo_rectangle (ctx,
        (double) progressbar->x,
        (double) progressbar->y,
        (double) progressbar->width * progressbar->progress,
        (double) progressbar->height);
    cairo_fill (ctx);
  }
  else {
    /**
     * If progress is zero or the widget needs a redraw, fill background black
     * to override the previously displayed progress.
     */
    if (widget->state.resized || widget->state.redraw) {
      cairo_set_source_rgb (ctx, 0.0, 0.0, 0.0);
      cairo_rectangle (ctx,
          (double) progressbar->x,
          (double) progressbar->y,
          (double) progressbar->width,
          (double) progressbar->height);
      cairo_fill (ctx);
    }
  }
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
