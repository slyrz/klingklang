#include <klingklang/base.h>
#include <klingklang/device.h>
#include <klingklang/format.h>
#include <klingklang/frame.h>
#include <klingklang/util.h>

#include <pulse/simple.h>
#include <pulse/error.h>

typedef struct kk_device_pulseaudio kk_device_pulseaudio_t;

struct kk_device_pulseaudio {
  kk_device_t base;
  kk_frame_t *buffer;
  pa_simple *handle;
};

static int device_init (kk_device_t *);
static int device_free (kk_device_t *);
static int device_drop (kk_device_t *);
static int device_setup (kk_device_t *, kk_format_t *);
static int device_write (kk_device_t *, kk_frame_t *);

const kk_device_backend_t device_backend = {
  .size = sizeof (kk_device_pulseaudio_t),
  .init = device_init,
  .free = device_free,
  .drop = device_drop,
  .setup = device_setup,
  .write = device_write
};

static const enum pa_sample_format sample_format[2][5] = {
  [KK_BYTE_ORDER_LITTLE_ENDIAN] = {
    PA_SAMPLE_U8,
    PA_SAMPLE_S16NE,
    PA_SAMPLE_S24NE,
    PA_SAMPLE_S32NE,
    PA_SAMPLE_FLOAT32NE,
  },
  [KK_BYTE_ORDER_BIG_ENDIAN] = {
    PA_SAMPLE_U8,
    PA_SAMPLE_S16NE,
    PA_SAMPLE_S24NE,
    PA_SAMPLE_S32NE,
    PA_SAMPLE_FLOAT32NE,
  }
};

static int
device_init (kk_device_t *dev_base)
{
  kk_device_pulseaudio_t *dev = (kk_device_pulseaudio_t *) dev_base;

  if (kk_frame_init (&dev->buffer) != 0)
    return -1;
  return 0;
}

static int
device_free (kk_device_t *dev_base)
{
  kk_device_pulseaudio_t *dev = (kk_device_pulseaudio_t *) dev_base;

  if (dev->handle)
    pa_simple_free (dev->handle);
  kk_frame_free (dev->buffer);
  return 0;
}

static int
device_drop (kk_device_t *dev_base)
{
  kk_device_pulseaudio_t *dev = (kk_device_pulseaudio_t *) dev_base;

  if (dev->handle)
    pa_simple_drain (dev->handle, NULL);
  return 0;
}

static int
device_setup (kk_device_t *dev_base, kk_format_t *format)
{
  kk_device_pulseaudio_t *dev = (kk_device_pulseaudio_t *) dev_base;

  const char *client = PACKAGE_STRING;
  const char *stream = "playback";
  const char *server = NULL;
  const char *resource = NULL;

  int status = 0;

  pa_sample_spec spec = {
    .format = PA_SAMPLE_INVALID,
    .channels = 0,
    .rate = format->sample_rate,
  };

  if (dev->handle) {
    pa_simple_free (dev->handle);
    dev->handle = NULL;
  }

  if (format->type == KK_TYPE_FLOAT) {
    if (format->bits == KK_BITS_32)
      spec.format = sample_format[format->layout][4];
  }
  else {
    switch (format->bits) {
      case KK_BITS_8:
        spec.format = sample_format[format->layout][0];
        break;
      case KK_BITS_16:
        spec.format = sample_format[format->layout][1];
        break;
      case KK_BITS_24:
        spec.format = sample_format[format->layout][2];
        break;
      case KK_BITS_32:
        spec.format = sample_format[format->layout][3];
        break;
      case KK_BITS_64:
        break;
    }
  }

  if (spec.format == PA_SAMPLE_INVALID) {
    kk_log (KK_LOG_WARNING, "Sample format not supported by PulseAudio.");
    goto error;
  }

  switch (format->channels) {
    case KK_CHANNELS_1:
      spec.channels = 1;
      break;
    case KK_CHANNELS_2:
      spec.channels = 2;
      break;
  }

  dev->handle =
      pa_simple_new (server, client, PA_STREAM_PLAYBACK, resource, stream,
          &spec, NULL, NULL, &status);

  if ((dev->handle == NULL) || (status != 0))
    goto error;
  return 0;
error:
  dev->handle = NULL;
  return -1;
}

static int
device_write (kk_device_t *dev_base, kk_frame_t *frame)
{
  kk_device_pulseaudio_t *dev = (kk_device_pulseaudio_t *) dev_base;

  void *data = NULL;

  switch (dev_base->format->layout) {
    case KK_LAYOUT_PLANAR:
      if (kk_frame_interleave (dev->buffer, frame, dev_base->format) != 0)
        return -1;
      data = (void *) dev->buffer->data[0];
      break;
    case KK_LAYOUT_INTERLEAVED:
      data = (void *) frame->data[0];
      break;
  }

  if (pa_simple_write (dev->handle, data, frame->size, NULL) != 0)
    return -1;
  return 0;
}
