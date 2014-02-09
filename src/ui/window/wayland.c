#include <klingklang/ui/window.h>
#include <klingklang/base.h>
#include <klingklang/str.h>
#include <klingklang/util.h>

#include <err.h>                // TODO: remove

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <pthread.h>

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
};

static void keyboard_handle_enter (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys);
static void keyboard_handle_key (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
static void keyboard_handle_keymap (void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size);
static void keyboard_handle_leave (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface);
static void keyboard_handle_modifiers (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);

static void registry_handle_global (void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
static void registry_handle_global_remove (void *data, struct wl_registry *registry, uint32_t name);

static void seat_handle_capabilities (void *data, struct wl_seat *seat, enum wl_seat_capability caps);
static void seat_handle_name (void *data, struct wl_seat *wl_seat, const char *name);

static void shell_surface_handle_configure (void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height);
static void shell_surface_handle_ping (void *data, struct wl_shell_surface *shell_surface, uint32_t serial);
static void shell_surface_handle_popup_done (void *data, struct wl_shell_surface *shell_surface);

static const struct wl_keyboard_listener keyboard_listener = {
  .keymap = keyboard_handle_keymap,
  .enter = keyboard_handle_enter,
  .leave = keyboard_handle_leave,
  .key = keyboard_handle_key,
  .modifiers = keyboard_handle_modifiers,
};

static const struct wl_registry_listener registry_listener = {
.global = registry_handle_global,.global_remove = registry_handle_global_remove};

static const struct wl_seat_listener seat_listener = {
  .capabilities = seat_handle_capabilities,
  .name = seat_handle_name
};

static const struct wl_shell_surface_listener shell_surface_listener = {
  .ping = shell_surface_handle_ping,
  .configure = shell_surface_handle_configure,
  .popup_done = shell_surface_handle_popup_done
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
keyboard_handle_enter (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
  (void) data;
  (void) keyboard;
  (void) serial;
  (void) surface;
  (void) keys;
  return;
}

static void
keyboard_handle_key (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
  (void) data;
  (void) keyboard;
  (void) serial;
  (void) time;
  (void) key;
  (void) state;
  return;
}

static void
keyboard_handle_keymap (void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size)
{
  (void) data;
  (void) keyboard;
  (void) format;
  (void) fd;
  (void) size;
  return;
}

static void
keyboard_handle_leave (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface)
{
  (void) data;
  (void) keyboard;
  (void) serial;
  (void) surface;
  return;
}

static void
keyboard_handle_modifiers (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
  (void) data;
  (void) keyboard;
  (void) serial;
  (void) mods_depressed;
  (void) mods_latched;
  (void) mods_locked;
  (void) group;
  return;
}

static void
registry_handle_global (void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
  (void) registry;
  (void) version;

  const char *interfaces[] = {
    "wl_compositor",
    "wl_shell",
    "wl_seat"
  };

  kk_window_t *window = (kk_window_t *) data;

  int i;
  int n;

  i = 0;
  n = sizeof (interfaces) / sizeof (char *);

  for (; i < n; i++) {
    if (strcmp (interface, interfaces[i]) == 0)
      break;
  }

  switch (i) {
    case 0:
      window->compositor = wl_registry_bind (window->registry, name, &wl_compositor_interface, 1);
      break;
    case 1:
      window->shell = wl_registry_bind (window->registry, name, &wl_shell_interface, 1);
      break;
    case 2:
      window->seat = wl_registry_bind (window->registry, name, &wl_seat_interface, 1);
      wl_seat_add_listener (window->seat, &seat_listener, window);
      break;
    default:
      return;
  }
}

static void
registry_handle_global_remove (void *data, struct wl_registry *registry, uint32_t name)
{
  (void) data;
  (void) registry;
  (void) name;
  return;
}

static void
seat_handle_capabilities (void *data, struct wl_seat *seat, enum wl_seat_capability caps)
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

/**
 * Ping a client to check if it is receiving events and sending requests.
 * A client is expected to reply with a pong request.
 */
static void
shell_surface_handle_ping (void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
  (void) data;

  wl_shell_surface_pong (shell_surface, serial);
}

/**
 * The configure event asks the client to resize its surface.
 */
static void
shell_surface_handle_configure (void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height)
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

  _kk_window_event_expose (window, width, height);
}

/**
 * The popup_done event is sent out when a popup grab is broken, that is,
 * when the user clicks a surface that doesn't belong to the client owning the
 * popup surface.
 */
static void
shell_surface_handle_popup_done (void *data, struct wl_shell_surface *shell_surface)
{
  (void) data;
  (void) shell_surface;
  return;
}

static void *
_kk_window_event_handler (kk_window_t * win)
{

  /**
   * Calling this function makes the calling thread the main thread.
   */
  wl_display_dispatch (win->display);
  wl_display_roundtrip (win->display);

  /**
   * Run main loop
   */
  for (;;) {
    puts ("LOOPING");
    if (wl_display_dispatch (win->display) == -1)
      err (EXIT_FAILURE, "wl_display_dispatch");
  }
}


static void
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

  if (eglInitialize (win->egl.dpy, &major, &minor) == EGL_FALSE)
    err (EXIT_FAILURE, "eglInitialize");

  if (eglBindAPI (EGL_OPENGL_API) == EGL_FALSE)
    err (EXIT_FAILURE, "eglBindAPI");

  if (eglChooseConfig (win->egl.dpy, conf_attr, &win->egl.conf, 1, &count) == EGL_FALSE)
    err (EXIT_FAILURE, "eglChooseConfig");

  if (count != 1)
    err (EXIT_FAILURE, "(count != 1)");

  win->egl.ctx = eglCreateContext (win->egl.dpy, win->egl.conf, EGL_NO_CONTEXT, NULL);
  if (win->egl.ctx == EGL_NO_CONTEXT)
    err (EXIT_FAILURE, "eglCreateContext");

  win->egl.surface = eglCreateWindowSurface (win->egl.dpy, win->egl.conf, (EGLNativeWindowType) win->window, NULL);
  if (win->egl.surface == EGL_NO_SURFACE)
    err (EXIT_FAILURE, "eglCreateWindowSurface");

  if (eglMakeCurrent (win->egl.dpy, win->egl.surface, win->egl.surface, win->egl.ctx) == EGL_FALSE)
    err (EXIT_FAILURE, "eglMakeCurrent");
}

static void
_kk_window_init_cairo (kk_window_t * win)
{
  win->cairo.device = cairo_egl_device_create (win->egl.dpy, win->egl.ctx);
  if (cairo_device_status (win->cairo.device) != CAIRO_STATUS_SUCCESS)
    err (EXIT_FAILURE, "cairo_device");

  win->cairo.surface = cairo_gl_surface_create_for_egl (win->cairo.device, win->egl.surface, win->width, win->height);
  if (cairo_surface_status (win->cairo.surface) != CAIRO_STATUS_SUCCESS)
    err (EXIT_FAILURE, "cairo_gl_surface_create_for_egl");
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
  if (result->display == NULL)
    err (EXIT_FAILURE, "wl_display_connect");

  /**
   * Registry allows the client to list and bind the global objects
   * available from the compositor.
   */
  result->registry = wl_display_get_registry (result->display);
  if (result->registry == NULL)
    err (EXIT_FAILURE, "wl_display_get_registry");

  /**
   * Listener contains callbacks that tell us about global objects
   * becoming available and global objects getting removed.
   */
  if (wl_registry_add_listener (result->registry, &registry_listener, result) != 0)
    err (EXIT_FAILURE, "wl_registry_add_listener");

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

  if (win->events)
    kk_event_queue_free (win->events);

  if (win->keys)
    kk_keys_free (win->keys);

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
    usleep(10000);
  }

  if (win->compositor == NULL)
    return -1;

  win->surface = wl_compositor_create_surface (win->compositor);
  if (win->surface == NULL)
    err (EXIT_FAILURE, "wl_compositor_create_surface");

  win->shell_surface = wl_shell_get_shell_surface (win->shell, win->surface);
  if (win->shell_surface == NULL)
    err (EXIT_FAILURE, "wl_shell_get_shell_surface");

  win->window = wl_egl_window_create (win->surface, win->width, win->height);
  if (win->window == NULL)
    err (EXIT_FAILURE, "wl_egl_window_create");

  wl_shell_surface_add_listener (win->shell_surface, &shell_surface_listener, win);
  wl_shell_surface_set_toplevel (win->shell_surface);

  _kk_window_init_egl (win);
  _kk_window_init_cairo (win);

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

  return -1;
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
