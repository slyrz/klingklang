#include <klingklang/base.h>
#include <klingklang/device.h>
#include <klingklang/util.h>
#include <klingklang/format.h>

#include <sndio.h>

typedef struct kk_device_sndio_s kk_device_sndio_t;

struct kk_device_sndio_s {
  kk_device_t base;
  kk_frame_t *buffer;
  struct sio_hdl *device;
};

int kk_device_sndio_init (kk_device_t *dev_base);
int kk_device_sndio_free (kk_device_t *dev_base);
int kk_device_sndio_drop (kk_device_t *dev_base);
int kk_device_sndio_setup (kk_device_t *dev_base, kk_format_t *format);
int kk_device_sndio_write (kk_device_t *dev_base, kk_frame_t *frame);

const kk_device_backend_t kk_device_backend = {
  .size = sizeof (kk_device_sndio_t),
  .init = kk_device_sndio_init,
  .free = kk_device_sndio_free,
  .drop = kk_device_sndio_drop,
  .setup = kk_device_sndio_setup,
  .write = kk_device_sndio_write,
};

int
kk_device_sndio_init (kk_device_t *dev_base)
{
  kk_device_sndio_t *dev_impl = (kk_device_sndio_t *) dev_base;

  dev_impl->device = sio_open (SIO_DEVANY, SIO_PLAY, 0);
  if (dev_impl->device == NULL)
    return -1;
  if (kk_frame_init (&dev_impl->buffer) != 0)
    return -1;
  return 0;
}

int
kk_device_sndio_free (kk_device_t *dev_base)
{
  kk_device_sndio_t *dev_impl = (kk_device_sndio_t *) dev_base;

  sio_close (dev_impl->device);
  kk_frame_free (dev_impl->buffer);
  return 0;
}

int
kk_device_sndio_drop (kk_device_t *dev_base)
{
  kk_device_sndio_t *dev_impl = (kk_device_sndio_t *) dev_base;

  if (sio_stop (dev_impl->device) == 0)
    return -1;
  return 0;
}

int
kk_device_sndio_setup (kk_device_t *dev_base, kk_format_t *format)
{
  kk_device_sndio_t *dev_impl = (kk_device_sndio_t *) dev_base;

  struct sio_par param;

  if (format->type == KK_TYPE_FLOAT) {
    kk_log (KK_LOG_WARNING, "Floating point sample format not supported.");
    return -1;
  }

  sio_initpar (&param);
  param.pchan = kk_format_get_channels (format);
  param.bits = kk_format_get_bits (format);
  param.bps = param.bits >> 3;
  param.rate = format->sample_rate;
  param.le = (format->byte_order == KK_BYTE_ORDER_LITTLE_ENDIAN);
  param.sig = (format->type == KK_TYPE_SINT);

  if (sio_setpar (dev_impl->device, &param) == 0)
    return -1;

  if (sio_start (dev_impl->device) == 0)
    return -1;

  return 0;
}

int
kk_device_sndio_write (kk_device_t *dev_base, kk_frame_t *frame)
{
  kk_device_sndio_t *dev_impl = (kk_device_sndio_t *) dev_base;

  void *data = NULL;

  switch (dev_base->format->layout) {
    case KK_LAYOUT_PLANAR:
      if (kk_frame_interleave (dev_impl->buffer, frame, dev_base->format) != 0)
        return -1;
      data = (void *) dev_impl->buffer->data[0];
      break;
    case KK_LAYOUT_INTERLEAVED:
      data = (void *) frame->data[0];
      break;
  }
  if (sio_write (dev_impl->device, data, frame->size) == 0)
    return -1;
  return 0;
}
