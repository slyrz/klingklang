#include <klingklang/device.h>

extern const kk_device_backend_t device_backend;

int
kk_device_init (kk_device_t **dev)
{
  kk_device_t *result;

  result = calloc (1, device_backend.size);
  if (result == NULL)
    goto error;

  if (pthread_mutex_init (&result->mutex, NULL) != 0)
    goto error;

  if (device_backend.init (result) < 0)
    goto error;

  *dev = result;
  return 0;
error:
  kk_device_free (result);
  *dev = NULL;
  return -1;
}

int
kk_device_free (kk_device_t *dev)
{
  if (dev == NULL)
    return 0;

  pthread_mutex_lock (&dev->mutex);
  device_backend.free (dev);
  pthread_mutex_unlock (&dev->mutex);
  pthread_mutex_destroy (&dev->mutex);
  free (dev);
  return 0;
}

int
kk_device_drop (kk_device_t *dev)
{
  int ret;

  pthread_mutex_lock (&dev->mutex);
  ret = device_backend.drop (dev);
  pthread_mutex_unlock (&dev->mutex);
  return ret;
}

int
kk_device_setup (kk_device_t *dev, kk_format_t *format)
{
  int ret;

  pthread_mutex_lock (&dev->mutex);
  ret = device_backend.setup (dev, format);
  if (ret == 0)
    dev->format = format;
  pthread_mutex_unlock (&dev->mutex);
  return ret;
}

int
kk_device_write (kk_device_t *dev, kk_frame_t *frame)
{
  int ret;

  pthread_mutex_lock (&dev->mutex);
  ret = device_backend.write (dev, frame);
  pthread_mutex_unlock (&dev->mutex);
  return ret;
}
