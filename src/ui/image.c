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
#include <klingklang/ui/image.h>

#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#define rgb_iadd(r,g,b,col) \
  do { \
    (r) += ((col) >> 16) & 0xff; \
    (g) += ((col) >>  8) & 0xff; \
    (b) += ((col)      ) & 0xff; \
  } while (0)

#define rgb_isub(r,g,b,col) \
  do { \
    (r) -= ((col) >> 16) & 0xff; \
    (g) -= ((col) >>  8) & 0xff; \
    (b) -= ((col)      ) & 0xff; \
  } while (0)

#define rgb_build(r,g,b) \
  ((((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff))

/**
 * libavutil versions >= 51.42.0:
 * Renamed PixelFormat to AVPixelFormat and all PIX_FMT_*to AV_PIX_FMT_*.
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

extern int libav_initialized;

static int
kk_image_surface_load_nativ (kk_image_t *img, const char *path)
{
  img->surface = cairo_image_surface_create_from_png (path);
  if (cairo_surface_status (img->surface) != CAIRO_STATUS_SUCCESS)
    return -1;
  return 0;
}

static int
kk_image_surface_decode (kk_image_t *img, AVFormatContext *fctx, AVCodecContext *cctx, int stream_index)
{
  const int w = cctx->width;
  const int h = cctx->height;
  const int s = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, w);

  const enum AVPixelFormat pixfmt = AV_PIX_FMT_RGB24;

  const size_t buffer_size = (size_t) avpicture_get_size (pixfmt, w, h);
  const size_t image_size = (size_t)w *(size_t)h;

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

  iframe = avcodec_alloc_frame ();
  oframe = avcodec_alloc_frame ();
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

  avpicture_fill ((AVPicture *) oframe, buffer, pixfmt, w, h);

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
  sctx = sws_getContext (w, h, cctx->pix_fmt, w, h, pixfmt, SWS_FAST_BILINEAR, NULL, NULL, NULL);
  if (sctx == NULL)
    goto error;

  sws_scale (sctx, (const uint8_t *const *) iframe->data, iframe->linesize, 0, h, oframe->data, oframe->linesize);

  cdata = calloc (image_size, sizeof (uint32_t));
  fdata = oframe->data[0];

  /**
   * Okay, so libavcodec / libswscale uses 3 bytes to store a pixel of 
   * rgb24 format, but cairo uses 4. That's why we have to create another 
   * buffer for our cairo surface and transform the RGBRGBRGB... data
   * into RGB_RGB_RGB_...
   */
  for (i = j = 0; i < (cctx->width *cctx->height); i++, j += 3)
    cdata[i] = ((fdata[j] & 0xffu) << 16) | ((fdata[j + 1] & 0xffu) << 8) | (fdata[j + 2] & 0xffu);

  img->surface = cairo_image_surface_create_for_data ((unsigned char*)cdata, CAIRO_FORMAT_RGB24, w, h, s);
  img->buffer = (unsigned char*)cdata;

  sws_freeContext (sctx);
  avcodec_free_frame (&iframe);
  avcodec_free_frame (&oframe);
  av_free_packet (&packet);
  free (buffer);
  return 0;
error:
  if (sctx)
    sws_freeContext (sctx);
  if (iframe)
    avcodec_free_frame (&iframe);
  if (oframe)
    avcodec_free_frame (&oframe);
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

  if (sidx < fctx->nb_streams)
    cctx = fctx->streams[sidx]->codec;
  else
    goto error;

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
  const int hr = (int)(intensity *(double) img->width) / 2;
  const int opo = -(hr + 1) *img->width;
  const int npo = hr *img->width;

  int x;
  int y;
  int i;

  int *colors;
  int *pixels;

  pixels = (int *) cairo_image_surface_get_data (img->surface);

  if (img->width > img->height)
    colors = calloc (sizeof (int), (size_t) img->width);
  else
    colors = calloc (sizeof (int), (size_t) img->height);

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

      if ((x >= 0) && (hits != 0))
        colors[x] = rgb_build ((r / hits), (g / hits), (b / hits));
    }

    for (x = 0; x < img->width; x++)
      pixels[i + x] = colors[x];

    i += img->width;
  }

  for (x = 0; x < img->height; x++) {
    int hits = 0;
    int r = 0;
    int g = 0;
    int b = 0;

    i = -hr *img->width + x;

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

      if ((y >= 0) && (hits != 0))
        colors[y] = rgb_build ((r / hits), (g / hits), (b / hits));

      i += img->width;
    }

    for (y = 0; y < img->height; y++)
      pixels[y *img->width + x] = colors[y];
  }

  free (colors);
  return 0;
}

