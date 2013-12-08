#include <klingklang/base.h>
#include <klingklang/device.h>
#include <klingklang/format.h>
#include <klingklang/frame.h>
#include <klingklang/util.h>

#include <pulse/simple.h>
#include <pulse/error.h>

typedef struct kk_device_pulseaudio_s kk_device_pulseaudio_t;

struct kk_device_pulseaudio_s {
  kk_device_t base;
  kk_frame_t *buffer;
  pa_simple *handle;
};

int kk_device_pulseaudio_init (kk_device_t *dev_base);
int kk_device_pulseaudio_free (kk_device_t *dev_base);
int kk_device_pulseaudio_drop (kk_device_t *dev_base);
int kk_device_pulseaudio_setup (kk_device_t *dev_base, kk_format_t *format);
int kk_device_pulseaudio_write (kk_device_t *dev_base, kk_frame_t *frame);

const kk_device_backend_t kk_device_backend = {
  .size = sizeof (kk_device_pulseaudio_t),
  .init = kk_device_pulseaudio_init,
  .free = kk_device_pulseaudio_free,
  .drop = kk_device_pulseaudio_drop,
  .setup = kk_device_pulseaudio_setup,
  .write = kk_device_pulseaudio_write
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

int
kk_device_pulseaudio_init (kk_device_t *dev_base)
{
  kk_device_pulseaudio_t *dev_impl = (kk_device_pulseaudio_t *) dev_base;

  if (kk_frame_init (&dev_impl->buffer) != 0)
    return -1;

  return 0;
}

int
kk_device_pulseaudio_free (kk_device_t *dev_base)
{
  kk_device_pulseaudio_t *dev_impl = (kk_device_pulseaudio_t *) dev_base;

  if (dev_impl->handle)
    pa_simple_free(dev_impl->handle);

  kk_frame_free (dev_impl->buffer);
  return 0;
}

int
kk_device_pulseaudio_drop (kk_device_t *dev_base)
{
  kk_device_pulseaudio_t *dev_impl = (kk_device_pulseaudio_t *) dev_base;

  if (dev_impl->handle)
    pa_simple_drain (dev_impl->handle, NULL);

  return 0;
}

int
kk_device_pulseaudio_setup (kk_device_t *dev_base, kk_format_t *format)
{
  kk_device_pulseaudio_t *dev_impl = (kk_device_pulseaudio_t *) dev_base;

  const char *client = PACKAGE_STRING;
  const char *stream = "playback";
  const char *server = NULL;
  const char *resource = NULL;

  int status = 0;
  int channels;

  pa_sample_spec spec = {
    .format = PA_SAMPLE_INVALID,
    .channels = 0,
    .rate = 0
  };

  if (dev_impl->handle) {
    pa_simple_free(dev_impl->handle);
    dev_impl->handle = NULL;
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

  channels = kk_format_get_channels (format);
  if ((channels <= 0) | (channels >= (int) UINT8_MAX)) {
    kk_log (KK_LOG_WARNING, "Invalid number of channels encountered.");
    goto error;
  }
  spec.channels = (uint8_t) channels;
  spec.rate = format->sample_rate;

  dev_impl->handle = pa_simple_new (server, client, PA_STREAM_PLAYBACK, resource, stream, &spec, NULL, NULL, &status);
  if ((dev_impl->handle == NULL) || (status != 0))
    goto error;

  return 0;
error:
  dev_impl->handle = NULL;
  return -1;
}

int
kk_device_pulseaudio_write (kk_device_t *dev_base, kk_frame_t *frame)
{
  kk_device_pulseaudio_t *dev_impl = (kk_device_pulseaudio_t *) dev_base;

  int error = 0;

  switch (dev_base->format->layout) {
    case KK_LAYOUT_PLANAR:
      if ((error = kk_frame_interleave (dev_impl->buffer, frame, dev_base->format)) == 0)
        error = pa_simple_write (dev_impl->handle, (void *) dev_impl->buffer->data[0], frame->size, NULL);
      break;
    case KK_LAYOUT_INTERLEAVED:
      error = pa_simple_write (dev_impl->handle, (void *) frame->data[0], frame->size, NULL);
      break;
  }

  if (error)
    return -1;
  return 0;
}
