#include <klingklang/base.h>
#include <klingklang/input.h>
#include <klingklang/util.h>

/**
 * Currently no plans to use any other decoders besides libav, so this isn't
 * as modular as the device interface. Also you aren't supposed to access
 * the fields of kk_input_t structs outside of this file, that's why the
 * header doesn't contain the definition of struct kk_input_s.
 */
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>

/**
 * libavcodec versions >= 55.28.1:
 * av_frame_alloc(), av_frame_unref() and av_frame_free() now can and should be
 * used instead of avcodec_alloc_frame(), avcodec_get_frame_defaults() and
 * avcodec_free_frame() respectively. The latter three functions are
 * deprecated.
 */
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#  define av_frame_alloc(x) avcodec_alloc_frame(x)
#  define av_frame_unref(x) avcodec_get_frame_defaults(x)
#endif

int libav_initialized = 0;

struct kk_input {
  AVCodec *codec;
  AVCodecContext *cctx;
  AVFormatContext *fctx;
  AVFrame *frame;
  AVStream *stream;
  int32_t sidx;
  struct {
    float end;
    float cur;
    float div;
  } time;
};

static int
input_stream_detect (kk_input_t *inp)
{
  int32_t i;

  for (i = 0; i < (int32_t) inp->fctx->nb_streams; i++) {
    inp->stream = inp->fctx->streams[i];
    if (inp->stream->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
      inp->sidx = i;
      return 0;
    }
  }
  return -1;
}

int
kk_input_init (kk_input_t **inp, const char *filename)
{
  kk_input_t *result;

  result = calloc (1, sizeof (kk_input_t));
  if (result == NULL)
    goto error;

  if (!libav_initialized) {
    av_register_all ();
    av_log_set_level (AV_LOG_ERROR);
    libav_initialized = 1;
  }

  if (avformat_open_input (&result->fctx, filename, NULL, NULL) < 0)
    goto error;

  if (avformat_find_stream_info (result->fctx, NULL) < 0)
    goto error;

  if (input_stream_detect (result) != 0)
    goto error;

  result->cctx = result->stream->codec;
  result->codec = avcodec_find_decoder (result->cctx->codec_id);
  if (avcodec_open2 (result->cctx, result->codec, NULL) < 0)
    goto error;

  /**
   * We want to use these as unsigned values, therefore check if we can
   * convert them without changing signedness
   */
  if ((result->cctx->channels < 0) || (result->cctx->sample_rate < 0))
    goto error;

  result->time.cur = 0.0f;
  result->time.end = (float) result->fctx->duration / AV_TIME_BASE;
  result->time.div = (float) result->stream->time_base.den \
                   * (float) result->stream->time_base.num;

  *inp = result;
  return 0;
error:
  kk_input_free (result);
  *inp = NULL;
  return -1;
}

int
kk_input_free (kk_input_t *inp)
{
  if (inp == NULL)
    return 0;

  if (inp->frame)
    av_free (inp->frame);

  if (inp->cctx)
    avcodec_close (inp->cctx);

  if (inp->fctx)
    avformat_close_input (&inp->fctx);

  free (inp);
  return 0;
}

static int64_t
input_timestamp (kk_input_t *inp, float perc)
{
  return (int64_t) ((perc * inp->time.end) * inp->time.div);
}

int
kk_input_seek (kk_input_t *inp, float perc)
{
  const int64_t ts = input_timestamp (inp, perc);
  int error;

  if (perc >= (inp->time.cur / inp->time.end))
    error = av_seek_frame (inp->fctx, inp->sidx, ts, 0);
  else
    error = av_seek_frame (inp->fctx, inp->sidx, ts, AVSEEK_FLAG_BACKWARD);

  if (error < 0)
    return -1;
  return 0;
}

int
kk_input_get_frame (kk_input_t *inp, kk_frame_t *frame)
{
  AVPacket packet;
  int ret = 0;
  int g;
  int ls;

  memset (frame, 0, sizeof (kk_frame_t));

  /**
   * What happens here? A loop reads packages until either av_read_frame
   * returns a value less than zero or a package from the audio stream was
   * read. Since every av_read_frame() call requires an av_free_packet()
   * call, we have to free all the packages we aren't interested in.
   */
  for (;;) {
    ret = av_read_frame (inp->fctx, &packet);
    if (ret < 0)
      goto cleanup;

    if (packet.stream_index == inp->sidx)
      break;

    av_free_packet (&packet);
  }

  if (inp->frame == NULL) {
    inp->frame = av_frame_alloc ();
    if (inp->frame == NULL)
      goto cleanup;
  }
  else
    av_frame_unref (inp->frame);

  ret = avcodec_decode_audio4 (inp->cctx, inp->frame, &g, &packet);
  if (ret < 0)
    goto cleanup;

  ret = av_samples_get_buffer_size (&ls, inp->cctx->channels,
            inp->frame->nb_samples, inp->cctx->sample_fmt, 1);
  if (ret <= 0)
    goto cleanup;

  if (ls < 0)
    ls = 0;

  if (inp->frame->nb_samples > 0)
    frame->samples = (size_t) inp->frame->nb_samples;
  else
    frame->samples = 0u;

  inp->time.cur = (float) packet.pts / inp->time.div;

  /* We store a percentage value in frame, not the time in seconds */
  frame->prog = inp->time.cur / inp->time.end;

  if (av_sample_fmt_is_planar (inp->cctx->sample_fmt)) {
    frame->planes = (size_t) inp->cctx->channels;
    frame->size = (size_t) ls * frame->planes;
  }
  else {
    frame->planes = 1;
    frame->size = (size_t) ls;
  }

  uint8_t **planes;

  if (inp->frame->extended_data)
    planes = inp->frame->extended_data;
  else
    planes = inp->frame->data;

  if (planes == NULL)
    return -1;

  /* Intendet fallthroughs, not a bug */
  switch (frame->planes) {
    case 2:
      frame->data[1] = planes[1];
    case 1:
      frame->data[0] = planes[0];
    default:
      break;
  }

cleanup:
  av_free_packet (&packet);
  if (ret >= 0)
    return ret;
  if (ret == (int) AVERROR_EOF)
    return 0;
  return -1;
}

int
kk_input_get_format (kk_input_t *inp, kk_format_t *format)
{
  switch (inp->cctx->channels) {
    case 1:
      format->channels = KK_CHANNELS_1;
      break;
    case 2:
      format->channels = KK_CHANNELS_2;
      break;
  }

  switch (inp->cctx->sample_fmt) {
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_U8P:
      format->bits = KK_BITS_8;
      break;
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
      format->bits = KK_BITS_16;
      break;
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
      format->bits = KK_BITS_32;
      break;
    case AV_SAMPLE_FMT_DBL:
    case AV_SAMPLE_FMT_DBLP:
      format->bits = KK_BITS_64;
      break;
    case AV_SAMPLE_FMT_NONE:
    case AV_SAMPLE_FMT_NB:
      return -1;
  }

  if (av_sample_fmt_is_planar (inp->cctx->sample_fmt))
    format->layout = KK_LAYOUT_PLANAR;
  else
    format->layout = KK_LAYOUT_INTERLEAVED;

  switch (inp->cctx->sample_fmt) {
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_U8P:
      format->type = KK_TYPE_UINT;
      break;
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
      format->type = KK_TYPE_SINT;
      break;
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
    case AV_SAMPLE_FMT_DBL:
    case AV_SAMPLE_FMT_DBLP:
      format->type = KK_TYPE_FLOAT;
      break;
    case AV_SAMPLE_FMT_NONE:
    case AV_SAMPLE_FMT_NB:
      return -1;
  }

  format->byte_order = KK_BYTE_ORDER_NATIVE;
  format->sample_rate = (unsigned int) inp->cctx->sample_rate;
  return 0;
}
