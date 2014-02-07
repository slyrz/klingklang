#include <klingklang/base.h>
#include <klingklang/device.h>
#include <klingklang/util.h>
#include <klingklang/format.h>

#include <ao/ao.h>

typedef struct kk_device_ao_s kk_device_ao_t;

struct kk_device_ao_s {
  kk_device_t base;
  kk_frame_t *buffer;
  ao_device *device;
  int driver;
};

int kk_device_ao_init (kk_device_t *dev_base);
int kk_device_ao_free (kk_device_t *dev_base);
int kk_device_ao_drop (kk_device_t *dev_base);
int kk_device_ao_setup (kk_device_t *dev_base, kk_format_t *format);
int kk_device_ao_write (kk_device_t *dev_base, kk_frame_t *frame);

const kk_device_backend_t kk_device_backend = {
  .size = sizeof (kk_device_ao_t),
  .init = kk_device_ao_init,
  .free = kk_device_ao_free,
  .drop = kk_device_ao_drop,
  .setup = kk_device_ao_setup,
  .write = kk_device_ao_write,
};

int
kk_device_ao_init (kk_device_t *dev_base)
{
  kk_device_ao_t *dev_impl = (kk_device_ao_t *) dev_base;

  ao_initialize ();
  dev_impl->device = NULL;
  dev_impl->driver = ao_default_driver_id ();

  if (kk_frame_init (&dev_impl->buffer) != 0)
    return -1;
  return 0;
}

int
kk_device_ao_free (kk_device_t *dev_base)
{
  kk_device_ao_t *dev_impl = (kk_device_ao_t *) dev_base;

  ao_close (dev_impl->device);
  ao_shutdown ();
  kk_frame_free (dev_impl->buffer);
  dev_impl->device = NULL;
  return 0;
}

int
kk_device_ao_drop (kk_device_t *dev_base)
{
  kk_device_ao_t *dev_impl = (kk_device_ao_t *) dev_base;

  ao_close (dev_impl->device);
  dev_impl->device = NULL;
  return 0;
}

int
kk_device_ao_setup (kk_device_t *dev_base, kk_format_t *format)
{
  ao_sample_format ao_format = {
    .bits = kk_format_get_bits (format),
    .channels = kk_format_get_channels (format),
    .rate = format->sample_rate,
    .byte_format = AO_FMT_NATIVE,
    .matrix = 0
  };

  kk_device_ao_t *dev_impl = (kk_device_ao_t *) dev_base;

  if (dev_impl->device)
    ao_close (dev_impl->device);

  /**
   * libao might say that our sample rate isn't supported by the hardware. But
   * sadly, libao only prints a warning to stderr instead of returning a
   * meaningful error value or atleast changing the value of the erroneous field
   * in the format structure.
   * If libao would indicate an error, we could resample the audio
   * before writing it to the device. But right now there's no way to know
   * whether setting the format failed or succeeded.
   */
  dev_impl->device = ao_open_live (dev_impl->driver, &ao_format, NULL);
  if (dev_impl->device == NULL) {
    kk_log (KK_LOG_WARNING, "ao_open_live failed.");
    return -1;
  }
  return 0;
}

int
kk_device_ao_write (kk_device_t *dev_base, kk_frame_t *frame)
{
  kk_device_ao_t *dev_impl = (kk_device_ao_t *) dev_base;
  int error = 0;

  if (dev_impl->device) {
    uint32_t size;

    if (frame->size > UINT32_MAX)
      error = 1;
    else {
      size = (uint32_t) frame->size;

      /* ao_play returns 0 on error */
      switch (dev_base->format->layout) {
        case KK_LAYOUT_PLANAR:
          if ((error = kk_frame_interleave (dev_impl->buffer, frame, dev_base->format)) == 0)
            error = (ao_play (dev_impl->device, (void *) dev_impl->buffer->data[0], size) == 0);
          break;
        case KK_LAYOUT_INTERLEAVED:
          error = (ao_play (dev_impl->device, (void *) frame->data[0], size) == 0);
          break;
      }
    }
  }

  if (error)
    return -1;

  return 0;
}
