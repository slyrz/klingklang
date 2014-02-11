#include <klingklang/ui/window.h>
#include <klingklang/base.h>
#include <klingklang/str.h>
#include <klingklang/util.h>

#include <errno.h>
#include <pthread.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>

#include <cairo.h>
#include <cairo-gl.h>

struct kk_window_s {
  kk_widget_fields;
  struct wl_callback *callback;
  struct wl_compositor *compositor;
  struct wl_display *display;
  struct wl_egl_window *window;
  struct wl_keyboard *keyboard;
  struct wl_registry *registry;
  struct wl_seat *seat;
  struct wl_shell *shell;
  struct wl_shell_surface *shell_surface;
  struct wl_surface *surface;
  struct {
    EGLDisplay dpy;
    EGLContext ctx;
    EGLConfig conf;
    EGLSurface surface;
  } egl;
  struct {
    cairo_device_t *device;
    cairo_surface_t *surface;
  } cairo;
  kk_event_queue_t *events;
  kk_keys_t *keys;
  pthread_t thread;
  unsigned alive:1;
};

static void
_kk_window_event_expose (kk_window_t *window, int width, int height)
{
  kk_window_event_expose_t event;

  memset (&event, 0, sizeof (kk_window_event_expose_t));
  event.type = KK_WINDOW_EXPOSE;
  event.width = width;
  event.height = height;
  kk_event_queue_write (window->events, (void *) &event, sizeof (kk_window_event_expose_t));
}

static void
_kk_window_event_input (kk_window_t *window, char *text)
{
  kk_window_event_input_t event;

  memset (&event, 0, sizeof (kk_window_event_input_t));
  event.type = KK_WINDOW_INPUT;
  event.text = strdup (text);
  kk_event_queue_write (window->events, (void *) &event, sizeof (kk_window_event_input_t));
}

static void
_kk_window_event_key_press (kk_window_t *window, int modifier, int key)
{
  kk_window_event_key_press_t event;

  memset (&event, 0, sizeof (kk_window_event_key_press_t));
  event.type = KK_WINDOW_KEY_PRESS;
  event.key = key;
  event.mod = modifier;
  kk_event_queue_write (window->events, (void *) &event, sizeof (kk_window_event_key_press_t));
}

static void
_kk_window_event_resize (kk_window_t *window, int width, int height)
{
  kk_window_event_resize_t event;

  memset (&event, 0, sizeof (kk_window_event_resize_t));
  event.type = KK_WINDOW_RESIZE;
  event.width = width;
  event.height = height;
  kk_event_queue_write (window->events, (void *) &event, sizeof (kk_window_event_resize_t));
}

static void
keyboard_handle_enter (void *data, struct wl_keyboard *keyboard,
    uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
  (void) data;
  (void) keyboard;
  (void) serial;
  (void) surface;
  (void) keys;
  return;
}

static void
keyboard_handle_key (void *data, struct wl_keyboard *keyboard, uint32_t serial,
    uint32_t time, uint32_t key, uint32_t state)
{
  (void) keyboard;
  (void) serial;
  (void) time;

  kk_window_t *window = (kk_window_t *) data;

  if (state != WL_KEYBOARD_KEY_STATE_RELEASED)
    return;

  int sym = kk_keys_get_symbol (window->keys, key + 8);
  int mod = kk_keys_get_modifiers (window->keys);

  _kk_window_event_key_press (window, mod, sym);
}

static void
keyboard_handle_keymap (void *data, struct wl_keyboard *keyboard,
    uint32_t format, int fd, uint32_t size)
{
  (void) data;
  (void) keyboard;
  (void) format;
  (void) fd;
  (void) size;
  return;
}

static void
keyboard_handle_leave (void *data, struct wl_keyboard *keyboard,
    uint32_t serial, struct wl_surface *surface)
{
  (void) data;
  (void) keyboard;
  (void) serial;
  (void) surface;
  return;
}

static void
keyboard_handle_modifiers (void *data, struct wl_keyboard *keyboard,
    uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
    uint32_t mods_locked, uint32_t group)
{
  (void) keyboard;
  (void) serial;

  kk_window_t *window = (kk_window_t *) data;

  kk_keys_set_modifiers (window->keys, mods_depressed, mods_latched, mods_locked, group);
  return;
}

static const struct wl_keyboard_listener keyboard_listener = {
  .keymap = keyboard_handle_keymap,
  .enter = keyboard_handle_enter,
  .leave = keyboard_handle_leave,
  .key = keyboard_handle_key,
  .modifiers = keyboard_handle_modifiers,
};

static void
seat_handle_capabilities (void *data, struct wl_seat *seat,
    enum wl_seat_capability caps)
{
  (void) seat;

  kk_window_t *window = (kk_window_t *) data;

  if (window->keyboard)
    return;

  if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
    window->keyboard = wl_seat_get_keyboard (window->seat);
    wl_keyboard_add_listener (window->keyboard, &keyboard_listener, window);
  }
}

static void
seat_handle_name (void *data, struct wl_seat *seat, const char *name)
{
  (void) data;
  (void) seat;
  (void) name;
  return;
}

static const struct wl_seat_listener seat_listener = {
  .capabilities = seat_handle_capabilities,
  .name = seat_handle_name
};

static void
registry_handle_global (void *data, struct wl_registry *registry,
    uint32_t name, const char *interface, uint32_t version)
{
  (void) registry;
  (void) version;

  const char *interfaces[] = {
    "wl_compositor",
    "wl_shell",
    "wl_seat"
  };

  kk_window_t *window = (kk_window_t *) data;

  int i = 0;
  int n = sizeof (interfaces) / sizeof (char *);
  for (; i < n; i++) {
    if (strcmp (interface, interfaces[i]) == 0)
      break;
  }

  switch (i) {
    case 0:
      window->compositor =
        wl_registry_bind (window->registry, name, &wl_compositor_interface, 1);
      break;
    case 1:
      window->shell =
        wl_registry_bind (window->registry, name, &wl_shell_interface, 1);
      break;
    case 2:
      window->seat =
        wl_registry_bind (window->registry, name, &wl_seat_interface, 1);
      wl_seat_add_listener (window->seat, &seat_listener, window);
      break;
    default:
      return;
  }
}

static void
registry_handle_global_remove (void *data, struct wl_registry *registry,
    uint32_t name)
{
  (void) data;
  (void) registry;
  (void) name;
  return;
}

static const struct wl_registry_listener registry_listener = {
  .global = registry_handle_global,
  .global_remove = registry_handle_global_remove
};

/**
 * Ping a client to check if it is receiving events and sending requests.
 * A client is expected to reply with a pong request.
 */
static void
shell_surface_handle_ping (void *data, struct wl_shell_surface *shell_surface,
    uint32_t serial)
{
  (void) data;

  wl_shell_surface_pong (shell_surface, serial);
}

/**
 * The configure event asks the client to resize its surface.
 */
static void
shell_surface_handle_configure (void *data,
    struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width,
    int32_t height)
{
  (void) shell_surface;
  (void) edges;

  kk_window_t *window = (kk_window_t *) data;

  window->width = width;
  window->height = height;
  window->resized = 1;
  window->redraw = 1;

  wl_egl_window_resize (window->window, width, height, 0, 0);
  cairo_gl_surface_set_size (window->cairo.surface, width, height);
  _kk_window_event_resize (window, width, height);
}

/**
 * The popup_done event is sent out when a popup grab is broken, that is,
 * when the user clicks a surface that doesn't belong to the client owning the
 * popup surface.
 */
static void
shell_surface_handle_popup_done (void *data,
    struct wl_shell_surface *shell_surface)
{
  (void) data;
  (void) shell_surface;
  return;
}

static const struct wl_shell_surface_listener shell_surface_listener = {
  .ping = shell_surface_handle_ping,
  .configure = shell_surface_handle_configure,
  .popup_done = shell_surface_handle_popup_done
};

static void *
_kk_window_event_handler (kk_window_t * win)
{
  win->alive = 1;

  /* Make calling thread the main thread. */
  wl_display_dispatch (win->display);
  wl_display_roundtrip (win->display);

  /* Run main loop */
  while (win->alive) {
    kk_log (KK_LOG_DEBUG, "Event loop.");
    if (wl_display_dispatch (win->display) == -1)
      break;
  }
  win->alive = 0;
  return NULL;
}

static int
_kk_window_init_egl (kk_window_t * win)
{
  EGLint conf_attr[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  int major;
  int minor;
  int count;

  win->egl.dpy = eglGetDisplay ((EGLNativeDisplayType) win->display);

  if (eglInitialize (win->egl.dpy, &major, &minor) == EGL_FALSE) {
    kk_log (KK_LOG_WARNING, "eglInitialize failed.");
    return -1;
  }

  if (eglBindAPI (EGL_OPENGL_API) == EGL_FALSE) {
    kk_log (KK_LOG_WARNING, "eglBindAPI failed.");
    return -1;
  }

  if (eglChooseConfig (win->egl.dpy, conf_attr, &win->egl.conf, 1, &count) == EGL_FALSE) {
    kk_log (KK_LOG_WARNING, "eglChooseConfig failed.");
    return -1;
  }

  if (count != 1) {
    kk_log (KK_LOG_WARNING, "Expected 1 config, got %d.", count);
    return -1;
  }

  win->egl.ctx = eglCreateContext (win->egl.dpy, win->egl.conf, EGL_NO_CONTEXT, NULL);
  if (win->egl.ctx == EGL_NO_CONTEXT) {
    kk_log (KK_LOG_WARNING, "eglCreateContext failed.");
    return -1;
  }

  win->egl.surface = eglCreateWindowSurface (win->egl.dpy, win->egl.conf, (EGLNativeWindowType) win->window, NULL);
  if (win->egl.surface == EGL_NO_SURFACE) {
    kk_log (KK_LOG_WARNING, "eglCreateWindowSurface failed.");
    return -1;
  }

  if (eglMakeCurrent (win->egl.dpy, win->egl.surface, win->egl.surface, win->egl.ctx) == EGL_FALSE) {
    kk_log (KK_LOG_WARNING, "eglMakeCurrent failed.");
    return -1;
  }

  return 0;
}

static int
_kk_window_init_cairo (kk_window_t * win)
{
  win->cairo.device = cairo_egl_device_create (win->egl.dpy, win->egl.ctx);
  if (cairo_device_status (win->cairo.device) != CAIRO_STATUS_SUCCESS) {
    kk_log (KK_LOG_WARNING, "Creating cairo egl device failed.");
    return -1;
  }

  win->cairo.surface = cairo_gl_surface_create_for_egl (win->cairo.device,
      win->egl.surface, win->width, win->height);
  if (cairo_surface_status (win->cairo.surface) != CAIRO_STATUS_SUCCESS) {
    kk_log (KK_LOG_WARNING, "Creating cairo gl surface from egl device failed.");
    return -1;
  }
  return 0;
}

int
kk_window_init (kk_window_t ** win, int width, int height)
{
  kk_window_t *result;

  if (kk_widget_init ((kk_widget_t **) & result, sizeof (kk_window_t), NULL) != 0)
    goto error;

  if (kk_event_queue_init (&result->events) != 0)
    goto error;

  if (kk_keys_init (&result->keys) != 0)
    goto error;

  /**
   * Connect to Wayland
   */
  result->display = wl_display_connect (NULL);
  if (result->display == NULL) {
    kk_log (KK_LOG_WARNING, "wl_display_connect failed.");
    goto error;
  }

  /**
   * Registry allows the client to list and bind the global objects
   * available from the compositor.
   */
  result->registry = wl_display_get_registry (result->display);
  if (result->registry == NULL) {
    kk_log (KK_LOG_WARNING, "wl_display_get_registry failed.");
    goto error;
  }

  /**
   * Listener contains callbacks that tell us about global objects
   * becoming available and global objects getting removed.
   */
  if (wl_registry_add_listener (result->registry, &registry_listener, result) != 0) {
    kk_log (KK_LOG_WARNING, "wl_registry_add_listener failed.");
    goto error;
  }

  if (pthread_create (&result->thread, 0, (void *(*)(void *)) _kk_window_event_handler, result) != 0)
    goto error;

  result->width = width;
  result->height = height;

  *win = result;
  return 0;
error:
  *win = NULL;
  return -1;
}

int
kk_window_free (kk_window_t * win)
{
  if (win == NULL)
    return 0;

  if (win->alive) {
    pthread_cancel (win->thread);
    pthread_join (win->thread, NULL);
  }

  if (win->cairo.surface)
    cairo_surface_destroy (win->cairo.surface);

  if (win->cairo.device) {
    cairo_device_finish (win->cairo.device);
    cairo_device_destroy (win->cairo.device);
  }

  if (win->egl.surface)
    eglDestroySurface (win->egl.dpy, win->egl.surface);

  if (win->window)
    wl_egl_window_destroy (win->window);

  if (win->surface)
    wl_surface_destroy (win->surface);

  if (win->egl.ctx)
    eglDestroyContext (win->egl.dpy, win->egl.ctx);

  if (win->egl.dpy) {
    eglMakeCurrent (win->egl.dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglReleaseThread ();
    eglTerminate (win->egl.dpy);
  }

  if (win->compositor)
    wl_compositor_destroy (win->compositor);

  if (win->registry)
    wl_registry_destroy (win->registry);

  if (win->display) {
    wl_display_flush (win->display);
    wl_display_disconnect (win->display);
  }

  if (win->keys)
    kk_keys_free (win->keys);

  if (win->events)
    kk_event_queue_free (win->events);

  kk_widget_free ((kk_widget_t *) win);
  return 0;
}

int
kk_window_show (kk_window_t * win)
{
  /**
   * Block and wait till we receive global compositor from registry. If we have
   * to wait longer than 1 second, something is fishy and we return with an
   * error.
   */
  int i;
  for (i = 0; i < 100; i++) {
    if (win->compositor != NULL)
      break;
    usleep (10000);
  }

  /**
   * Compositor still NULL? Exit.
   */
  if (win->compositor == NULL)
    return -1;

  win->surface = wl_compositor_create_surface (win->compositor);
  if (win->surface == NULL) {
    kk_log (KK_LOG_WARNING, "wl_compositor_create_surface");
    return -1;
  }

  win->shell_surface = wl_shell_get_shell_surface (win->shell, win->surface);
  if (win->shell_surface == NULL) {
    kk_log (KK_LOG_WARNING, "wl_shell_get_shell_surface");
    return -1;
  }

  win->window = wl_egl_window_create (win->surface, win->width, win->height);
  if (win->window == NULL) {
    kk_log (KK_LOG_WARNING, "wl_egl_window_create");
    return -1;
  }

  wl_shell_surface_add_listener (win->shell_surface, &shell_surface_listener, win);
  wl_shell_surface_set_toplevel (win->shell_surface);

  if (_kk_window_init_egl (win) != 0) {
    kk_log (KK_LOG_WARNING, "Initializing EGL failed.");
    return -1;
  }

  if (_kk_window_init_cairo (win)  != 0 ) {
    kk_log (KK_LOG_WARNING, "Initializing cairo failed.");
    return -1;
  }

  shell_surface_handle_configure (win, win->shell_surface, 0, win->width, win->height);

  kk_widget_invalidate ((kk_widget_t *) win);
  kk_window_draw (win);
  wl_display_flush (win->display);
  return 0;
}

int
kk_window_draw (kk_window_t * win)
{
  cairo_t *ctx;

  ctx = cairo_create (win->cairo.surface);
  if (cairo_status (ctx) != CAIRO_STATUS_SUCCESS)
    return -1;

  cairo_set_source_rgb (ctx, 0.0, 0.0, 0.0);
  cairo_rectangle (ctx, 0.0, 0.0, win->width, win->height);
  cairo_fill (ctx);

  kk_widget_invalidate ((kk_widget_t *) win);
  kk_widget_draw ((kk_widget_t *) win, ctx);
  cairo_destroy (ctx);

  cairo_gl_surface_swapbuffers (win->cairo.surface);
  return 0;
}

int
kk_window_get_input (kk_window_t * win)
{
  static char buffer[1024] = { 0 };

  int pofd[2];                  /* stdout of child */
  int pifd[2];                  /* stdin of child */

  if (pipe (pifd) != 0)
    return -1;

  if (pipe (pofd) != 0)
    return -1;

  if (fork () == 0) {
    dup2 (pifd[0], STDIN_FILENO);
    dup2 (pofd[1], STDOUT_FILENO);

    close (pofd[0]);
    close (pifd[1]);

    setsid ();
    if (execlp ("dmenu", "dmenu", NULL) != 0)
      kk_err (EXIT_FAILURE, "execlp() failed.");
  }
  else {
    close (pifd[0]);
    close (pofd[1]);

    /**
     * Upon start, dmenu wants to read the menu entries from stdin.
     * Since we use dmenu as simple text input, we don't pass menu
     * entries to dmenu. Therefore, we close stdin directly.
     */
    close (pifd[1]);

    int err = 0;
    ssize_t ret = 0;
    size_t tot = 0;

    for (;;) {
      if (tot >= sizeof (buffer))
        break;

      errno = 0;
      ret = read (pofd[0], buffer + tot, sizeof (buffer) - tot);
      err = errno;

      /* Break on error, but keep going on interrupts. */
      if ((ret <= 0) && (err != EINTR))
          break;

      if (ret > 0)
        tot += (size_t) ret;
    }

    /* No input read? Must have been canceled by user, so no error. */
    if (tot == 0)
      return 0;

    /* No input read? Must have been canceled by user, so no error. */
    if (tot >= sizeof (buffer))
      return -1;

    /* Remove trailing newline. */
    if (buffer[tot - 1] == '\n')
      buffer[--tot] = '\0';

    if (tot > 0)
      _kk_window_event_input (win, buffer);
  }

  return 0;
}

int
kk_window_set_title (kk_window_t * win, const char *title)
{
  if (win->shell_surface == NULL)
    return -1;

  wl_shell_surface_set_title (win->shell_surface, title);
  return 0;
}

int
kk_window_get_event_fd (kk_window_t * win)
{
  return kk_event_queue_get_read_fd (win->events);
}
