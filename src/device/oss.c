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

static int
device_ctrl (int fd, unsigned long req, int val)
{
  int arg = val;

  if ((ioctl (fd, req, &arg) == -1) || (arg != val))
    return -1;
  return 0;
}

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
  kk_frame_free (dev_impl->buffer);
  return 0;
}

int
kk_device_oss_drop (kk_device_t * dev_base)
{
  kk_device_oss_t *dev_impl = (kk_device_oss_t *) dev_base;

  if (device_ctrl (dev_impl->fd, SNDCTL_DSP_SKIP, 0) != 0)
    return -1;
  return 0;
}

int
kk_device_oss_setup (kk_device_t * dev_base, kk_format_t * format)
{
  kk_device_oss_t *dev_impl = (kk_device_oss_t *) dev_base;
  int req;

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

  req = kk_format_get_channels (format);
  if (device_ctrl (dev_impl->fd, SNDCTL_DSP_CHANNELS, req) != 0) {
    kk_log (KK_LOG_WARNING, "Device doesn't support %d channels.", req);
    goto error;
  }

  req = (int) format->sample_rate;
  if (device_ctrl (dev_impl->fd, SNDCTL_DSP_SPEED, req) != 0) {
    kk_log (KK_LOG_WARNING, "Device doesn't %d Hz sample rate.", req);
    goto error;
  }

  if (format->type == KK_TYPE_FLOAT) {
    kk_log (KK_LOG_WARNING, "Floating point sample format not supported by OSS.");
    goto error;
  }

  switch (format->bits) {
    case KK_BITS_8:
      req = AFMT_U8;
      break;
    case KK_BITS_16:
      req = AFMT_S16_LE;
      break;
    case KK_BITS_24:
      req = AFMT_S24_LE;
      break;
    case KK_BITS_32:
      req = AFMT_S32_LE;
      break;
    case KK_BITS_64:
      kk_log (KK_LOG_WARNING, "64 bit sample format not supported by OSS.");
      goto error;
  }

  if (device_ctrl (dev_impl->fd, SNDCTL_DSP_SETFMT, req) != 0) {
    kk_log (KK_LOG_WARNING, "Device doesn't support sample format.");
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
  void *data;

  switch (dev_base->format->layout) {
    case KK_LAYOUT_PLANAR:
      if (kk_frame_interleave (dev_impl->buffer, frame, dev_base->format) != 0)
        goto error;
      data = (void *) dev_impl->buffer->data[0];
      break;
    case KK_LAYOUT_INTERLEAVED:
      data = (void *) frame->data[0];
      break;
  }

  if (write (dev_impl->fd, data, frame->size) <= 0)
    goto error;

  return 0;
error:
  kk_log (KK_LOG_WARNING, "Writing frame failed.");
  return -1;
}
