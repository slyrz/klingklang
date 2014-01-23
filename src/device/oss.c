#include <klingklang/base.h>
#include <klingklang/device.h>
#include <klingklang/util.h>
#include <klingklang/format.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/stat.h>

typedef struct kk_device_oss_s kk_device_oss_t;

struct kk_device_oss_s {
  kk_device_t base;
  kk_frame_t *buffer;
  int fd;
};

int kk_device_oss_init (kk_device_t * dev_base);
int kk_device_oss_free (kk_device_t * dev_base);
int kk_device_oss_drop (kk_device_t * dev_base);
int kk_device_oss_setup (kk_device_t * dev_base, kk_format_t * format);
int kk_device_oss_write (kk_device_t * dev_base, kk_frame_t * frame);

const kk_device_backend_t kk_device_backend = {
  .size = sizeof (kk_device_oss_t),
  .init = kk_device_oss_init,
  .free = kk_device_oss_free,
  .drop = kk_device_oss_drop,
  .setup = kk_device_oss_setup,
  .write = kk_device_oss_write,
};

static const int sample_format[2][4] = {
  [KK_BYTE_ORDER_LITTLE_ENDIAN] = {
    AFMT_U8,
    AFMT_S16_LE,
    AFMT_S24_LE,
    AFMT_S32_LE
  },
  [KK_BYTE_ORDER_BIG_ENDIAN] = {
    AFMT_U8,
    AFMT_S16_BE,
    AFMT_S24_BE,
    AFMT_S32_BE
  }
};

int
kk_device_oss_init (kk_device_t * dev_base)
{
  kk_device_oss_t *dev_impl = (kk_device_oss_t *) dev_base;

  if (kk_frame_init (&dev_impl->buffer) != 0)
    return -1;
  return 0;
}

int
kk_device_oss_free (kk_device_t * dev_base)
{
  kk_device_oss_t *dev_impl = (kk_device_oss_t *) dev_base;

  close (dev_impl->fd);
  return 0;
}

int
kk_device_oss_drop (kk_device_t * dev_base)
{
  kk_device_oss_t *dev_impl = (kk_device_oss_t *) dev_base;

  if (ioctl (dev_impl->fd, SNDCTL_DSP_SKIP, 0) < 0) {
    kk_log (KK_LOG_WARNING, "Could not stop playback.");
    return -1;
  }

  return 0;
}

int
kk_device_oss_setup (kk_device_t * dev_base, kk_format_t * format)
{
  kk_device_oss_t *dev_impl = (kk_device_oss_t *) dev_base;

  int n;
  int o;

  /**
   * There's no real reset ioctl. It's the recommended way to reopen
   * the device on every setup.
   */
  if (dev_impl->fd > 0) {
    kk_device_oss_drop (dev_base);
    close (dev_impl->fd);
  }

  dev_impl->fd = open ("/dev/dsp", O_WRONLY, 0);
  if (dev_impl->fd == -1) {
    kk_log (KK_LOG_WARNING, "Could not open audio device /dev/dsp.");
    return -1;
  }

  o = kk_format_get_channels (format);
  n = o;
  if ((ioctl (dev_impl->fd, SNDCTL_DSP_CHANNELS, &n) == -1) || (o != n)) {
    kk_log (KK_LOG_WARNING, "Could not stop playback.");
    goto error;
  }

  o = format->sample_rate;
  n = o;
  if ((ioctl (dev_impl->fd, SNDCTL_DSP_SPEED, &n) == -1) || (o != n)) {
    kk_log (KK_LOG_WARNING, "Could not set sample rate.");
    goto error;
  }

  o = 0;
  if (format->type == KK_TYPE_FLOAT) {
    return -1; // TODO
    if (format->bits == KK_BITS_32)
      o = AFMT_FLOAT; // TODO: double precision?
  }
  else {
    switch (format->bits) {
      case KK_BITS_8:
        o = sample_format[format->layout][0];
        break;
      case KK_BITS_16:
        o = sample_format[format->layout][1];
        break;
      case KK_BITS_24:
        o = sample_format[format->layout][2];
        break;
      case KK_BITS_32:
        o = sample_format[format->layout][3];
        break;
      case KK_BITS_64:
        break;
    }
  }

  if (o == 0) {
    kk_log (KK_LOG_WARNING, "Sample format not supported by OSS.");
    goto error;
  }

  n = o;
  if ((ioctl (dev_impl->fd, SNDCTL_DSP_SETFMT, &n) == -1) || (o != n)) {
    kk_log (KK_LOG_WARNING, "Could not set sample format.");
    goto error;
  }

  return 0;
error:
  return -1;
}

int
kk_device_oss_write (kk_device_t * dev_base, kk_frame_t * frame)
{
  kk_device_oss_t *dev_impl = (kk_device_oss_t *) dev_base;
  int error = 0;



  switch (dev_base->format->layout) {
    case KK_LAYOUT_PLANAR:
      if ((error = kk_frame_interleave (dev_impl->buffer, frame, dev_base->format)) == 0) {
        error = (write (dev_impl->fd, (void *) dev_impl->buffer->data[0], frame->size) <= 0);
      }
      break;
    case KK_LAYOUT_INTERLEAVED:
      error = (write (dev_impl->fd, (void *) frame->data[0], frame->size) <= 0);
      break;
  }

  if (error) {
    kk_log (KK_LOG_WARNING, "Writing frame failed.");
    return -1;
  }

  return 0;
}
