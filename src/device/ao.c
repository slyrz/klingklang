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

static int device_init (kk_device_t *);
static int device_free (kk_device_t *);
static int device_drop (kk_device_t *);
static int device_setup (kk_device_t *, kk_format_t *);
static int device_write (kk_device_t *, kk_frame_t *);

const kk_device_backend_t kk_device_backend = {
  .size = sizeof (kk_device_ao_t),
  .init = device_init,
  .free = device_free,
  .drop = device_drop,
  .setup = device_setup,
  .write = device_write,
};

static int
device_init (kk_device_t *dev_base)
{
  kk_device_ao_t *dev = (kk_device_ao_t *) dev_base;

  ao_initialize ();
  dev->device = NULL;
  dev->driver = ao_default_driver_id ();
  if (kk_frame_init (&dev->buffer) != 0)
    return -1;
  return 0;
}

static int
device_free (kk_device_t *dev_base)
{
  kk_device_ao_t *dev = (kk_device_ao_t *) dev_base;

  ao_close (dev->device);
  ao_shutdown ();
  kk_frame_free (dev->buffer);
  dev->device = NULL;
  return 0;
}

static int
device_drop (kk_device_t *dev_base)
{
  kk_device_ao_t *dev = (kk_device_ao_t *) dev_base;

  ao_close (dev->device);
  dev->device = NULL;
  return 0;
}

static int
device_setup (kk_device_t *dev_base, kk_format_t *format)
{
  kk_device_ao_t *dev = (kk_device_ao_t *) dev_base;

  ao_sample_format ao_format = {
    .byte_format = AO_FMT_NATIVE,
    .bits = kk_format_get_bits (format),
    .channels = kk_format_get_channels (format),
    .rate = (int) format->sample_rate,
    .matrix = 0,
  };

  if (dev->device)
    ao_close (dev->device);

  /**
   * libao might say that our sample rate isn't supported by the hardware. But
   * sadly, libao only prints a warning to stderr instead of returning a
   * meaningful error value or atleast changing the value of the erroneous field
   * in the format structure.
   * If libao would indicate an error, we could resample the audio
   * before writing it to the device. But right now there's no way to know
   * whether setting the format failed or succeeded.
   */
  dev->device = ao_open_live (dev->driver, &ao_format, NULL);
  if (dev->device == NULL) {
    kk_log (KK_LOG_WARNING, "ao_open_live failed.");
    return -1;
  }
  return 0;
}

static int
device_write (kk_device_t *dev_base, kk_frame_t *frame)
{
  kk_device_ao_t *dev = (kk_device_ao_t *) dev_base;
  int error = 0;

  if (dev->device) {
    if (frame->size > UINT32_MAX)
      error = 1;
    else {
      switch (dev_base->format->layout) {
        case KK_LAYOUT_PLANAR:
          if ((error = kk_frame_interleave (dev->buffer, frame, dev_base->format)) == 0)
            error = (ao_play (dev->device, (void *) dev->buffer->data[0], (uint32_t) frame->size) == 0);
          break;
        case KK_LAYOUT_INTERLEAVED:
          error = (ao_play (dev->device, (void *) frame->data[0], (uint32_t) frame->size) == 0);
          break;
      }
    }
  }
  if (error)
    return -1;
  return 0;
}
