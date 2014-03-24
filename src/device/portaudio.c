#include <klingklang/base.h>
#include <klingklang/device.h>
#include <klingklang/format.h>
#include <klingklang/util.h>

#include <portaudio.h>

typedef struct kk_device_portaudio kk_device_portaudio_t;

struct kk_device_portaudio {
  kk_device_t base;
  PaStream *handle;
};

static int device_init (kk_device_t *);
static int device_free (kk_device_t *);
static int device_drop (kk_device_t *);
static int device_setup (kk_device_t *, kk_format_t *);
static int device_write (kk_device_t *, kk_frame_t *);

const kk_device_backend_t device_backend = {
  .size = sizeof (kk_device_portaudio_t),
  .init = device_init,
  .free = device_free,
  .drop = device_drop,
  .setup = device_setup,
  .write = device_write
};

static int
device_init (kk_device_t *dev_base)
{
  (void) dev_base;

  if (Pa_Initialize () != paNoError)
    return -1;
  return 0;
}

static int
device_free (kk_device_t *dev_base)
{
  kk_device_portaudio_t *dev = (kk_device_portaudio_t *) dev_base;
  PaError status;

  if (dev->handle) {
    status = Pa_CloseStream (dev->handle);
    if (status != paNoError)
      kk_log (KK_LOG_WARNING, "Closing stream failed.");
    dev->handle = NULL;
  }

  status = Pa_Terminate ();
  if (status != paNoError)
    kk_log (KK_LOG_WARNING, "Terminating PortAudio failed.");
  return 0;
}

static int
device_drop (kk_device_t *dev_base)
{
  kk_device_portaudio_t *dev = (kk_device_portaudio_t *) dev_base;

  if ((dev->handle) && (Pa_IsStreamActive (dev->handle) > 0)) {
    if (Pa_StopStream (dev->handle) != paNoError) {
      kk_log (KK_LOG_WARNING, "Could not stop stream.");
      return -1;
    }
  }
  return 0;
}

static int
device_setup (kk_device_t *dev_base, kk_format_t *format)
{
  kk_device_portaudio_t *dev = (kk_device_portaudio_t *) dev_base;

  PaError status;
  PaSampleFormat sample_format = (PaSampleFormat) 0;
  PaStreamParameters params;

  if (dev->handle) {
    if (Pa_IsStreamActive (dev->handle) == 1) {
      status = Pa_StopStream (dev->handle);
      if (status != paNoError) {
        kk_log (KK_LOG_WARNING, "Could not stop previous stream.");
      }
    }

    if (Pa_IsStreamStopped (dev->handle) == 1) {
      status = Pa_CloseStream (dev->handle);
      if (status != paNoError) {
        kk_log (KK_LOG_WARNING, "Could not close previous stream.");
      }
      dev->handle = NULL;
    }
  }

  if (format->type == KK_TYPE_FLOAT) {
    if (format->bits != KK_BITS_32) {
      kk_log (KK_LOG_WARNING, "Only 32 bit float sample format supported.");
      return -1;
    }
    sample_format = paFloat32;
  }
  else {
    /* It's an integer type */
    switch (format->bits) {
      case KK_BITS_8:
        sample_format = (format->type == KK_TYPE_SINT) ? paInt8 : paUInt8;
        break;
      case KK_BITS_16:
        sample_format = paInt16;
        break;
      case KK_BITS_24:
        sample_format = paInt24;
        break;
      case KK_BITS_32:
        sample_format = paInt32;
        break;
      case KK_BITS_64:
        kk_log (KK_LOG_WARNING, "64 bits sample format not supported.");
        return -1;
    }
  }

  if (format->layout == KK_LAYOUT_PLANAR)
    sample_format |= paNonInterleaved;

  params.device = Pa_GetDefaultOutputDevice ();
  if (params.device == paNoDevice) {
    kk_log (KK_LOG_ERROR, "No output device found.");
    return -1;
  }

  params.channelCount = kk_format_get_channels (format);
  params.sampleFormat = sample_format;
  params.suggestedLatency = Pa_GetDeviceInfo (params.device)->defaultLowOutputLatency;
  params.hostApiSpecificStreamInfo = NULL;

  status =
      Pa_OpenStream (&dev->handle, NULL, &params, (double) format->sample_rate,
          paFramesPerBufferUnspecified, paClipOff, NULL, NULL);
  if (status != paNoError) {
    kk_log (KK_LOG_ERROR, "Pa_OpenStream failed. Unsupported PCM format?");
    return -1;
  }

  status = Pa_StartStream (dev->handle);
  if (status != paNoError) {
    kk_log (KK_LOG_WARNING, "Starting stream failed.");
    status = Pa_CloseStream (dev->handle);
    if (status != paNoError) {
      kk_log (KK_LOG_WARNING, "Emergency close failed too.");
    }
    dev->handle = NULL;
    return -1;
  }
  return 0;
}

static int
device_write (kk_device_t *dev_base, kk_frame_t *frame)
{
  kk_device_portaudio_t *dev = (kk_device_portaudio_t *) dev_base;

  const void *data = NULL;

  if (dev->handle == NULL)
    return -1;

  switch (dev_base->format->layout) {
    case KK_LAYOUT_PLANAR:
      data = (const void *) frame->data;
      break;
    case KK_LAYOUT_INTERLEAVED:
      data = (const void *) frame->data[0];
      break;
  }

  if (Pa_WriteStream (dev->handle, data, frame->samples) != paNoError)
    return -1;
  return 0;
}
