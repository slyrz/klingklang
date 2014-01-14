/**
 * This file incorporates modified versions of work covered by the 
 * following copyright and permission notice:
 *
 * kk_image_blur
 * -------------
 * Copyright (c) 2010 Mattias Fagerlund
 * http://lotsacode.wordpress.com/2010/12/08/fast-blur-box-blur-with-accumulator/
 */
#include <klingklang/base.h>
#include <klingklang/util.h>
#include <klingklang/ui/image.h>

#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#define rgb_iadd(r,g,b,col) \
  do { \
    (r) += (uint8_t) (((col) >> 16) & 0xffu); \
    (g) += (uint8_t) (((col) >>  8) & 0xffu); \
    (b) += (uint8_t) (((col)      ) & 0xffu); \
  } while (0)

#define rgb_isub(r,g,b,col) \
  do { \
    (r) -= (uint8_t) (((col) >> 16) & 0xffu); \
    (g) -= (uint8_t) (((col) >>  8) & 0xffu); \
    (b) -= (uint8_t) (((col)      ) & 0xffu); \
  } while (0)

#define rgb_build(r,g,b) \
  ((((uint32_t)(r) & 0xffu) << 16) | (((uint32_t)(g) & 0xffu) << 8) | ((uint32_t)(b) & 0xffu))

/**
 * libavutil versions >= 51.42.0:
 * Renamed PixelFormat to AVPixelFormat and all PIX_FMT_* to AV_PIX_FMT_*.
 */
#if !((LIBAVUTIL_VERSION_MAJOR >= 51) && (LIBAVUTIL_VERSION_MINOR >= 42))
#  define AVPixelFormat PixelFormat
#  define AV_PIX_FMT_RGB24 PIX_FMT_RGB24
#endif

/**
 * libavcodec versions >= 54.28.0:
 * Introduced avcodec_free_frame() function which must now be used for freeing an
 * AVFrame.
 */
#if !((LIBAVCODEC_VERSION_MAJOR >= 54) && (LIBAVCODEC_VERSION_MINOR >= 28))
#  define avcodec_free_frame(x) av_free(*(x))
#endif

/**
 * libavcodec versions >= 55.28.1:
 * av_frame_alloc(), av_frame_unref() and av_frame_free() now can and should be
 * used instead of avcodec_alloc_frame(), avcodec_get_frame_defaults() and
 * avcodec_free_frame() respectively. The latter three functions are deprecated.
 */
#if !((LIBAVCODEC_VERSION_MAJOR >= 55) && (LIBAVCODEC_VERSION_MINOR >= 28))
#  define av_frame_free(x) avcodec_free_frame(x)
#  define av_frame_alloc(x) avcodec_alloc_frame(x)
#endif

extern int libav_initialized;

static int
kk_image_surface_load_nativ (kk_image_t *img, const char *path)
{
  cairo_status_t status;

  img->surface = cairo_image_surface_create_from_png (path);

  status = cairo_surface_status (img->surface);
  if (status != CAIRO_STATUS_SUCCESS) {
    kk_log (KK_LOG_WARNING, "Failed to load file '%s'. Cairo returned '%s'.", path, cairo_status_to_string (status));
    return -1;
  }
  return 0;
}

static int
kk_image_surface_decode (kk_image_t *img, AVFormatContext *fctx, AVCodecContext *cctx, int stream_index)
{
  const int iw = cctx->width;
  const int ih = cctx->height;

  /**
   * Now this is kind of a long story. We use swsscale to transform any
   * pixelformat to RGB24, not to scale images. That's why the output
   * dimension equaled the input dimension at first. However, this lead
   * to a strange bug. The sws_scale'd output buffer contained some black
   * pixels at the end of each row. These pixels caused a visible, vertical
   * black strip on the right side of the output image. To this day I have
   * no idea how this function should be responsible for this. Anyway, after
   * a long debugging session I noticed that the following combination
   *
   *    output width = input width - 1, output height = input height
   *
   * prevents this from happening. I tested it quite a bit and it seems to
   * work. Again, I have no idea why. Btw. this is definitly not cairo
   * related. The sws_scale() just returns a really strange buffer.
   */
  const int ow = iw - 1;
  const int oh = ih;

  const int os = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, ow);

  const size_t buffer_size = (size_t) avpicture_get_size (AV_PIX_FMT_RGB24, ow, oh);

  struct SwsContext *sctx = NULL;
  AVFrame *iframe = NULL;
  AVFrame *oframe = NULL;
  AVPacket packet;

  int i = 0;
  int j = 0;
  int r = 0;
  int got_frame = 0;

  uint32_t *cdata = NULL;
  uint8_t *fdata = NULL;
  uint8_t *buffer = NULL;

  iframe = av_frame_alloc ();
  oframe = av_frame_alloc ();
  if ((iframe == NULL) || (oframe == NULL))
    goto error;

  /**
   * Somehow the function swscale() performs an invalid write of size 1 at the
   * end of this buffer. I'm not sure why but adding additional 4 Bytes makes
   * sure this does no harm.
   */
  buffer = calloc (buffer_size + 4, sizeof (uint8_t));
  if (buffer == NULL)
    goto error;

  avpicture_fill ((AVPicture *) oframe, buffer, AV_PIX_FMT_RGB24, ow, oh);

  for (;;) {
    r = av_read_frame (fctx, &packet);
    if (r < 0)
      goto error;
    if (packet.stream_index == (int) stream_index)
      break;
    av_free_packet (&packet);
  }

  if ((avcodec_decode_video2 (cctx, iframe, &got_frame, &packet) <= 0) || (!got_frame))
    goto error;

  /**
   * We read a frame. Now we use libswscale to convert the pixel format
   * to rgb24, because cairo won't like our frame's pixel format.
   */
  sctx = sws_getCachedContext (NULL, iw, ih, cctx->pix_fmt, ow, oh, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR , NULL, NULL, NULL);
  if (sctx == NULL)
    goto error;

  sws_scale (sctx, (const uint8_t *const *) iframe->data, iframe->linesize, 0, oh, oframe->data, oframe->linesize);

  fdata = oframe->data[0];
  cdata = calloc ((size_t) ow * (size_t) oh, sizeof (uint32_t));
  if (cdata == NULL)
    goto error;

  /**
   * Okay, so libavcodec / libswscale uses 3 bytes to store a pixel of
   * rgb24 format, but cairo uses 4. That's why we have to create another
   * buffer for our cairo surface and transform the RGBRGBRGB... data
   * into RGB_RGB_RGB_...
   */
  for (i = j = 0; i < (ow * oh); i++, j += 3)
    cdata[i] = rgb_build (fdata[j], fdata[j + 1], fdata[j + 2]);

  img->surface = cairo_image_surface_create_for_data ((unsigned char*) cdata, CAIRO_FORMAT_RGB24, ow, oh, os);
  img->buffer = (unsigned char*) cdata;

  sws_freeContext (sctx);
  av_frame_free (&iframe);
  av_frame_free (&oframe);
  av_free_packet (&packet);
  free (buffer);
  return 0;
error:
  if (sctx)
    sws_freeContext (sctx);
  if (iframe)
    av_frame_free (&iframe);
  if (oframe)
    av_frame_free (&oframe);
  av_free_packet (&packet);
  free (buffer);
  free (cdata);
  return -1;
}

static int
kk_image_surface_load_other (kk_image_t *img, const char *path)
{
  AVCodecContext *cctx = NULL;
  AVFormatContext *fctx = NULL;
  AVInputFormat *ifmt = NULL;
  AVOutputFormat *ofmt = NULL;

  unsigned int sidx;
  unsigned char *data = NULL;

  if (!libav_initialized) {
    av_register_all ();
    libav_initialized = 1;
  }

  ofmt = av_guess_format (NULL, path, NULL);
  if (ofmt == NULL)
    goto error;

  ifmt = av_find_input_format (ofmt->name);
  if (ifmt == NULL) {
    /* One last chance: we can open gif with image2. Let's try that... */
    if (strncmp (ofmt->name, "gif", 3) == 0)
      ifmt = av_find_input_format ("image2");
    if (ifmt == NULL)
      goto error;
  }

  fctx = NULL;
  if (avformat_open_input (&fctx, path, ifmt, NULL) != 0)
    goto error;

  if (avformat_find_stream_info (fctx, NULL) < 0)
    goto error;

  for (sidx = 0; sidx < fctx->nb_streams; sidx++) {
    if (fctx->streams[sidx]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
      break;
  }

  if (sidx >= fctx->nb_streams)
    goto error;

  cctx = fctx->streams[sidx]->codec;
  if (avcodec_open2 (cctx, avcodec_find_decoder (cctx->codec_id), NULL) < 0)
    goto error;

  if (kk_image_surface_decode (img, fctx, cctx, (int) sidx) != 0)
    goto error;

  avcodec_close (cctx);
  avformat_close_input (&fctx);
  return 0;
error:
  if (cctx)
    avcodec_close (cctx);
  if (fctx)
    avformat_close_input (&fctx);
  free (data);
  return -1;
}

static int
kk_image_surface_load (kk_image_t *img, const char *path)
{
  char *ext = strrchr (path, '.');      /* No need to error check */

  if (strcmp (ext, ".png") == 0)
    return kk_image_surface_load_nativ (img, path);
  else
    return kk_image_surface_load_other (img, path);
}

int
kk_image_init (kk_image_t **img, const char *path)
{
  kk_image_t *result;

  result = calloc (1, sizeof (kk_image_t));
  if (result == NULL)
    goto error;

  if (kk_image_surface_load (result, path) != 0)
    goto error;

  result->width = cairo_image_surface_get_width (result->surface);
  result->height = cairo_image_surface_get_height (result->surface);
  *img = result;
  return 0;
error:
  if (result)
    kk_image_free (result);
  *img = NULL;
  return -1;
}

int
kk_image_free (kk_image_t *img)
{
  if (img == NULL)
    return 0;

  if (img->surface) {
    cairo_surface_finish (img->surface);
    cairo_surface_destroy (img->surface);
  }
  free (img->buffer);
  free (img);
  return 0;
}

int
kk_image_blur (kk_image_t *img, double intensity)
{
  const int hr = (int)(intensity * (double) img->width) >> 1;
  const int opo = -(hr + 1) * img->width;
  const int npo = hr * img->width;

  uint32_t *colors = NULL;
  uint32_t *pixels = NULL;

  int x;
  int y;
  int i;

  pixels = (uint32_t *) cairo_image_surface_get_data (img->surface);

  if (img->width > img->height)
    colors = calloc ((size_t) img->width, sizeof (uint32_t));
  else
    colors = calloc ((size_t) img->height, sizeof (uint32_t));

  if ((pixels == NULL) | (colors == NULL))
    goto error;

  i = 0;
  for (y = 0; y < img->height; y++) {
    int hits = 0;
    int r = 0;
    int g = 0;
    int b = 0;

    for (x = -hr; x < img->width; x++) {
      int op = x - hr - 1;
      int np = x + hr;

      if (op >= 0) {
        rgb_isub (r, g, b, pixels[i + op]);
        hits--;
      }

      if (np < img->width) {
        rgb_iadd (r, g, b, pixels[i + np]);
        hits++;
      }

      if ((x >= 0) && (hits != 0)) {
        colors[x] = rgb_build ((r / hits), (g / hits), (b / hits));
      }
    }

    for (x = 0; x < img->width; x++)
      pixels[i + x] = colors[x];

    i += img->width;
  }

  for (x = 0; x < img->width; x++) {
    int hits = 0;
    int r = 0;
    int g = 0;
    int b = 0;

    i = -hr * img->width + x;

    for (y = -hr; y < img->height; y++) {
      int op = y - hr - 1;
      int np = y + hr;

      if (op >= 0) {
        rgb_isub (r, g, b, pixels[i + opo]);
        hits--;
      }

      if (np < img->height) {
        rgb_iadd (r, g, b, pixels[i + npo]);
        hits++;
      }

      if ((y >= 0) && (hits != 0)) {
        colors[y] = rgb_build ((r / hits), (g / hits), (b / hits));
      }

      i += img->width;
    }

    for (y = 0; y < img->height; y++)
      pixels[y * img->width + x] = colors[y];
  }

  free (colors);
  return 0;
error:
  free (colors);
  return -1;
}

