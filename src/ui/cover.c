#include <klingklang/base.h>
#include <klingklang/util.h>
#include <klingklang/ui/cover.h>

#include <math.h>

static void cover_draw (kk_widget_t *widget, cairo_t *ctx);

int
kk_cover_init (kk_cover_t **cover)
{
  kk_cover_t *result;

  if (kk_widget_init ((kk_widget_t **) & result, sizeof (kk_cover_t), cover_draw) != 0)
    goto error;

  result->darkness = 0.333;
  result->blur = 0.08;
  *cover = result;
  return 0;
error:
  *cover = NULL;
  return -1;
}

int
kk_cover_free (kk_cover_t *cover)
{
  if (cover == NULL)
    return 0;

  if (cover->foreground)
    kk_image_free (cover->foreground);

  if (cover->background)
    kk_image_free (cover->background);

  if (cover->contour)
    cairo_pattern_destroy (cover->contour);

  free (cover->path);
  kk_widget_free ((kk_widget_t *) cover);
  return 0;
}

int
kk_cover_load (kk_cover_t *cover, kk_library_file_t *file)
{
  size_t len;
  size_t out;

  char *path = NULL;

  len = 512;
  path = calloc (len, sizeof (char));
  if (path == NULL)
    goto error;

  out = kk_library_file_get_album_cover_path (file, path, len);
  if (out >= len) {
    if (out >= 8192)
      goto error;

    /* Contains truncated stuff - useless */
    free (path);

    /* Try one last time */
    len = out + 1;
    path = calloc (len, sizeof (char));
    if (path == NULL)
      goto error;

    out = kk_library_file_get_album_cover_path (file, path, len);
    if (out >= len)
      goto error;
  }

  /* No album cover found? We can stop */
  if (out == 0)
    goto error;

  if (cover->path) {
    /* Image already loaded. No need to change it. */
    if (strcmp (path, cover->path) == 0)
      goto cleanup;
    if (cover->foreground)
      kk_image_free (cover->foreground);
    if (cover->background)
      kk_image_free (cover->background);
    free (cover->path);
  }

  cover->path = path;

  if (kk_image_init (&cover->foreground, cover->path) != 0)
    goto error;

  if (kk_image_init (&cover->background, cover->path) != 0)
    goto error;

  kk_image_blur (cover->background, cover->blur);
  kk_widget_invalidate ((kk_widget_t *) cover);
  return 0;
error:
  if ((path) && (*path))
    kk_log (KK_LOG_WARNING, "Could not load cover '%s'.", path);
  cover->path = NULL;
  cover->foreground = NULL;
  cover->background = NULL;
cleanup:
  free (path);
  return -1;
}

static void
cairo_rounded_rectangle (cairo_t *cr, double x, double y, double width, double height, double radius)
{
  const double d = M_PI / 180.0;

  cairo_arc (cr, x + width - radius, y + radius, radius, -90.0 * d, 0.0);
  cairo_arc (cr, x + width - radius, y + height - radius, radius, 0.0, 90.0 * d);
  cairo_arc (cr, x + radius, y + height - radius, radius,  90.0 * d, 180.0 * d);
  cairo_arc (cr, x + radius, y + radius, radius, 180.0 * d, 270.0 * d);
  cairo_line_to (cr, x + width - radius, y);
}

static void
cover_resize (kk_cover_t *cover)
{
  const double margin = 40.0;
  const double cx = cover->x + (cover->width / 2);
  const double cy = cover->y + (cover->height / 2);

  if (cover->width < cover->height)
    cover->radius = (cover->width + margin) / 2.0;
  else
    cover->radius = (cover->height + margin) / 2.0;

  if (cover->contour)
    cairo_pattern_destroy (cover->contour);

  cover->contour = cairo_pattern_create_linear (cx, cy - cover->radius, cx, cy + cover->radius);
  cairo_pattern_add_color_stop_rgba (cover->contour, 0.0, 1.0, 1.0, 1.0, 0.4);
  cairo_pattern_add_color_stop_rgba (cover->contour, 0.1, 1.0, 1.0, 1.0, 0.1);
  cairo_pattern_add_color_stop_rgba (cover->contour, 0.5, 0.5, 0.5, 0.5, 0.0);
  cairo_pattern_add_color_stop_rgba (cover->contour, 0.9, 0.0, 0.0, 0.0, 0.1);
  cairo_pattern_add_color_stop_rgba (cover->contour, 1.0, 0.0, 0.0, 0.0, 0.4);
}

static void
cover_draw (kk_widget_t *widget, cairo_t *ctx)
{
  kk_cover_t *cover = (kk_cover_t *) widget;

  if ((cover->foreground == NULL) | (cover->background == NULL))
    return;

  if (widget->resized)
    cover_resize (cover);

  /* Draw blurred background image */
  cairo_save (ctx);
  cairo_scale (ctx,
    (double) cover->width / (double) cover->foreground->width,
    (double) cover->height / (double) cover->foreground->height
  );
  cairo_set_source_surface (ctx, cover->background->surface, 0.0, 0.0);
  cairo_paint (ctx);
  cairo_restore (ctx);

  /* Draw dark gray with darnkess as alpha channel to darken background */
  cairo_save (ctx);
  cairo_set_source_rgba (ctx, 0.070, 0.082, 0.090, cover->darkness);
  cairo_rectangle (ctx,
    (double) cover->x,
    (double) cover->y,
    (double) cover->width,
    (double) cover->height
  );
  cairo_fill (ctx);
  cairo_restore (ctx);

  /* Scale to stretch / shrink foreground image according to widget size */
  const double scale = (2.0 * cover->radius + 8.0) / (double) cover->foreground->width;

  /* Clip center + draw foreground */
  cairo_save (ctx);
  cairo_rounded_rectangle (ctx,
    (double) cover->x + ((double) cover->width / 2.0 - cover->radius),
    (double) cover->y + ((double) cover->height / 2.0 - cover->radius),
    cover->radius * 2.0,
    cover->radius * 2.0,
    cover->radius / 30.0
  );

  cairo_clip (ctx);
  cairo_new_path (ctx);
  cairo_scale (ctx, scale, scale);
  cairo_set_operator (ctx, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface (ctx,
    cover->foreground->surface,
    (((double) cover->width - ((double) cover->foreground->width * scale)) / 2.0) / scale,
    (((double) cover->height - ((double) cover->foreground->height * scale)) / 2.0) / scale
  );
  cairo_paint (ctx);
  cairo_close_path (ctx);
  cairo_reset_clip (ctx);
  cairo_restore (ctx);

  /* Draw unobstrusive border around foreground */
  if (cover->contour) {
    cairo_save (ctx);
    cairo_rounded_rectangle (ctx,
      (double) cover->width / 2.0 - cover->radius,
      (double) cover->height / 2.0 - cover->radius,
      cover->radius * 2.0,
      cover->radius * 2.0,
      cover->radius / 30.0
    );
    cairo_set_line_width (ctx, 1.0);
    cairo_set_source (ctx, cover->contour);
    cairo_stroke (ctx);
    cairo_restore (ctx);
  }
}
