#include <klingklang/frame.h>
#include <klingklang/util.h>

static size_t
frame_get_planes (kk_frame_t *frame)
{
  if (frame->planes < KK_FRAME_MAX_PLANES)
    return frame->planes;
  else
    return KK_FRAME_MAX_PLANES;
}

static void
frame_free_planes (kk_frame_t *frame)
{
  size_t i;

  if (frame == NULL)
    return;

  for (i = 0; i < frame_get_planes (frame); i++)
    free (frame->data[i]);

  memset (frame->data, 0, KK_FRAME_MAX_PLANES * sizeof (uint8_t *));
  frame->planes = 0;
  frame->size = 0;
}

int
kk_frame_init (kk_frame_t **frame)
{
  kk_frame_t *result;

  result = calloc (1, sizeof (kk_frame_t));
  if (result == NULL)
    goto error;
  *frame = result;
  return 0;
error:
  *frame = NULL;
  return -1;
}

int
kk_frame_free (kk_frame_t *frame)
{
  frame_free_planes (frame);
  free (frame);
  return 0;
}

static int
frame_realloc (kk_frame_t *frame, size_t planes, size_t size)
{
  size_t i;

  if ((size % planes) != 0) {
    kk_log (KK_LOG_WARNING,
        "Frame size of %zu bytes not divisible into %zu planes.",
        size, planes);
    return -1;
  }

  if (planes > KK_FRAME_MAX_PLANES) {
    kk_log (KK_LOG_WARNING,
        "%zu planes requested, but only %d planes supported.",
        planes, KK_FRAME_MAX_PLANES);
    return -1;
  }

  if (frame->planes > 0)
    frame_free_planes (frame);

  for (i = 0; i < planes; i++) {
    frame->data[i] = calloc (size / planes, sizeof (uint8_t));
    if (frame->data[i] == NULL)
      goto error;
  }
  frame->planes = planes;
  frame->size = size;
  return 0;
error:
  frame_free_planes (frame);
  return -1;
}

static inline void
frame_interleave (kk_frame_t *restrict dst, kk_frame_t *restrict src,
    size_t byte)
{
  register uint8_t *restrict ilp = dst->data[0];
  register uint8_t *restrict pla = src->data[0];
  register uint8_t *restrict plb = src->data[1];

  size_t i;
  size_t j;

  for (i = 0; i < (src->size / (2 * byte)); i++) {
    for (j = 0; j < byte; j++)
      *ilp++ = *pla++;
    for (j = 0; j < byte; j++)
      *ilp++ = *plb++;
  }
}

int
kk_frame_interleave (kk_frame_t *restrict dst, kk_frame_t *restrict src,
    kk_format_t *fmt)
{
  if ((src->size > dst->size) || (dst->planes != 1)) {
    if (frame_realloc (dst, 1, src->size) != 0)
      return -1;
  }

  if (fmt->channels == KK_CHANNELS_1) {
    memcpy (dst->data[0], src->data[0], src->size);
  }
  else {
    switch (fmt->bits) {
      case KK_BITS_8:
        frame_interleave (dst, src, 1);
        break;
      case KK_BITS_16:
        frame_interleave (dst, src, 2);
        break;
      case KK_BITS_24:
        frame_interleave (dst, src, 3);
        break;
      case KK_BITS_32:
        frame_interleave (dst, src, 4);
        break;
      case KK_BITS_64:
        frame_interleave (dst, src, 8);
        break;
    }
  }
  return 0;
}
