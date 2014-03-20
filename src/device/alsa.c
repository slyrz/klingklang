#include <klingklang/base.h>
#include <klingklang/device.h>
#include <klingklang/format.h>
#include <klingklang/util.h>

#include <alsa/asoundlib.h>

typedef struct kk_device_alsa_s kk_device_alsa_t;

struct kk_device_alsa_s {
  kk_device_t base;
  snd_pcm_t *handle;
};

int kk_device_alsa_init (kk_device_t *dev_base);
int kk_device_alsa_free (kk_device_t *dev_base);
int kk_device_alsa_drop (kk_device_t *dev_base);
int kk_device_alsa_setup (kk_device_t *dev_base, kk_format_t *format);
int kk_device_alsa_write (kk_device_t *dev_base, kk_frame_t *frame);

const kk_device_backend_t kk_device_backend = {
  .size = sizeof (kk_device_alsa_t),
  .init = kk_device_alsa_init,
  .free = kk_device_alsa_free,
  .drop = kk_device_alsa_drop,
  .setup = kk_device_alsa_setup,
  .write = kk_device_alsa_write
};

/**
 * This array allows snd_pcm_format_t lookup via indexes.
 * The index describes a sample format and consists of the following
 * bit-fields:
 *
 *                 | [unused] [BBB] [TT] [E] |
 *                 31         5              0
 *
 * E, 1 bit to indicate endianness:
 *   0 = little endian
 *   1 = big endian
 *
 * T, 2 bits to indicate data type:
 *   00 = unsigned integer
 *   01 = signed integer
 *   10 = float
 *
 * B, 3 bits to indicate sample format:
 *   000 =  8 sample bits
 *   001 = 16 sample bits
 *   010 = 24 sample bits
 *   011 = 32 sample bits
 *   100 = 64 sample bits
 *
 */
 /**INDENT-OFF**/
static const snd_pcm_format_t sample_format[42] = {
  [ 0] = SND_PCM_FORMAT_U8,          /* index = 000|00|0 */
  [ 1] = SND_PCM_FORMAT_U8,          /* index = 000|00|1 */
  [ 2] = SND_PCM_FORMAT_S8,          /* index = 000|01|0 */
  [ 3] = SND_PCM_FORMAT_S8,          /* index = 000|01|1 */
  [ 8] = SND_PCM_FORMAT_U16_LE,      /* index = 001|00|0 */
  [ 9] = SND_PCM_FORMAT_U16_BE,      /* index = 001|00|1 */
  [10] = SND_PCM_FORMAT_S16_LE,      /* index = 001|01|0 */
  [11] = SND_PCM_FORMAT_S16_BE,      /* index = 001|01|1 */
  [16] = SND_PCM_FORMAT_U24_LE,      /* index = 010|00|0 */
  [17] = SND_PCM_FORMAT_U24_BE,      /* index = 010|00|1 */
  [18] = SND_PCM_FORMAT_S24_LE,      /* index = 010|01|0 */
  [19] = SND_PCM_FORMAT_S24_BE,      /* index = 010|01|1 */
  [24] = SND_PCM_FORMAT_U32_LE,      /* index = 011|00|0 */
  [25] = SND_PCM_FORMAT_U32_BE,      /* index = 011|00|1 */
  [26] = SND_PCM_FORMAT_S32_LE,      /* index = 011|01|0 */
  [27] = SND_PCM_FORMAT_S32_BE,      /* index = 011|01|1 */
  [28] = SND_PCM_FORMAT_FLOAT_LE,    /* index = 011|10|0 */
  [29] = SND_PCM_FORMAT_FLOAT_BE,    /* index = 011|10|1 */
  [36] = SND_PCM_FORMAT_FLOAT64_LE,  /* index = 100|10|0 */
  [37] = SND_PCM_FORMAT_FLOAT64_BE,  /* index = 100|10|1 */
};
/**INDENT-ON**/

static snd_pcm_format_t
get_pcm_format (kk_format_t *fmt)
{
  size_t idx = 0;

  idx = fmt->bits;
  idx = (idx << 2) | fmt->type;
  idx = (idx << 1) | fmt->byte_order;
  return sample_format[idx];
}

static snd_pcm_access_t
get_pcm_access (kk_format_t *fmt)
{
  if (fmt->layout == KK_LAYOUT_INTERLEAVED)
    return SND_PCM_ACCESS_RW_INTERLEAVED;
  else
    return SND_PCM_ACCESS_RW_NONINTERLEAVED;
}

int
kk_device_alsa_init (kk_device_t *dev_base)
{
  kk_device_alsa_t *dev_impl = (kk_device_alsa_t *) dev_base;

  const char *name = "default";
  const int mode = 0;
  const snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;

  if (snd_pcm_open (&dev_impl->handle, name, stream, mode) < 0)
    return -1;
  return 0;
}

int
kk_device_alsa_free (kk_device_t *dev_base)
{
  kk_device_alsa_t *dev_impl = (kk_device_alsa_t *) dev_base;

  snd_pcm_drop (dev_impl->handle);
  snd_pcm_close (dev_impl->handle);
  dev_impl->handle = NULL;

  /* ... because valgrind errors */
  snd_config_update_free_global ();
  return 0;
}

int
kk_device_alsa_drop (kk_device_t *dev_base)
{
  kk_device_alsa_t *dev_impl = (kk_device_alsa_t *) dev_base;

  if (snd_pcm_drop (dev_impl->handle) < 0)
    return -1;
  return 0;
}

int
kk_device_alsa_setup (kk_device_t *dev_base, kk_format_t *format)
{
  kk_device_alsa_t *dev_impl = (kk_device_alsa_t *) dev_base;

  const snd_pcm_format_t pcm_format = get_pcm_format (format);
  const snd_pcm_access_t pcm_access = get_pcm_access (format);

  const unsigned int channels = (unsigned int) kk_format_get_channels (format);
  const unsigned int rate = format->sample_rate;
  const unsigned int latency = 500000u;
  const int soft_resample = 1;

  if (snd_pcm_set_params (dev_impl->handle, pcm_format, pcm_access, channels,
          rate, soft_resample, latency) < 0)
    return -1;
  return 0;
}

int
kk_device_alsa_write (kk_device_t *dev_base, kk_frame_t *frame)
{
  kk_device_alsa_t *dev_impl = (kk_device_alsa_t *) dev_base;

  snd_pcm_sframes_t nframes = 0;
  snd_pcm_uframes_t uframes = 0;
  snd_pcm_sframes_t sframes = 0;

  if (frame->size > SSIZE_MAX)
    return -1;

  sframes = snd_pcm_bytes_to_frames (dev_impl->handle, (ssize_t) frame->size);
  uframes = (snd_pcm_uframes_t) sframes;
  if (sframes <= 0)
    return -1;

  switch (dev_base->format->layout) {
    case KK_LAYOUT_PLANAR:
      nframes =
          snd_pcm_writen (dev_impl->handle, (void **) frame->data, uframes);
      break;
    case KK_LAYOUT_INTERLEAVED:
      nframes =
          snd_pcm_writei (dev_impl->handle, (void *) frame->data[0], uframes);
      break;
  }

  if (nframes < 0) {
    if (snd_pcm_recover (dev_impl->handle, (int) nframes, 1) < 0)
      return -1;
  }
  return 0;
}
