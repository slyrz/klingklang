#include <klingklang/ui/window.h>
#include <klingklang/util.h>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>

#include <cairo.h>
#include <cairo-gl.h>

#include <unistd.h> /* usleep */

typedef struct kk_window_wayland_s kk_window_wayland_t;

struct kk_window_wayland_s {
  kk_window_t base;
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
  pthread_t thread;
  unsigned alive:1;
};

static int window_init (kk_window_t *);
static int window_free (kk_window_t *);
static int window_show (kk_window_t *);
static int window_draw (kk_window_t *);
static int window_set_title (kk_window_t *, const char *);

const kk_window_backend_t kk_window_backend = {
  .size = sizeof (kk_window_wayland_t),
  .init = window_init,
  .free = window_free,
  .show = window_show,
  .draw = window_draw,
  .set_title = window_set_title
};

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

  kk_window_wayland_t *window = (kk_window_wayland_t *) data;

  if (state != WL_KEYBOARD_KEY_STATE_RELEASED)
    return;

  int sym = kk_keys_get_symbol (window->base.keys, key + 8);
  int mod = kk_keys_get_modifiers (window->base.keys);

  kk_window_event_key_press (window->base.events, mod, sym);
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

  kk_window_wayland_t *window = (kk_window_wayland_t *) data;

  kk_keys_set_modifiers (window->base.keys, mods_depressed, mods_latched,
      mods_locked, group);
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

  kk_window_wayland_t *window = (kk_window_wayland_t *) data;

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

  kk_window_wayland_t *window = (kk_window_wayland_t *) data;

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

  kk_window_wayland_t *window = (kk_window_wayland_t *) data;

  window->base.width = width;
  window->base.height = height;
  window->base.resized = 1;
  window->base.redraw = 1;

  wl_egl_window_resize (window->window, width, height, 0, 0);
  cairo_gl_surface_set_size (window->cairo.surface, width, height);
  kk_window_event_resize (window->base.events, width, height);
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
window_event_handler (kk_window_wayland_t *win)
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
window_init_egl (kk_window_wayland_t *win)
{
  EGLint conf_attr[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  EGLBoolean status;

  int major;
  int minor;
  int count;

  win->egl.dpy = eglGetDisplay ((EGLNativeDisplayType) win->display);

  if (eglInitialize (win->egl.dpy, &major, &minor) == EGL_FALSE) {
    kk_log (KK_LOG_WARNING, "eglInitialize failed.");
    return -1;
  }

  status = eglBindAPI (EGL_OPENGL_API);
  if (status == EGL_FALSE) {
    kk_log (KK_LOG_WARNING, "eglBindAPI failed.");
    return -1;
  }

  status =
      eglChooseConfig (win->egl.dpy, conf_attr, &win->egl.conf, 1, &count);
  if (status == EGL_FALSE) {
    kk_log (KK_LOG_WARNING, "eglChooseConfig failed.");
    return -1;
  }

  if (count != 1) {
    kk_log (KK_LOG_WARNING, "Expected 1 config, got %d.", count);
    return -1;
  }

  win->egl.ctx =
      eglCreateContext (win->egl.dpy, win->egl.conf, EGL_NO_CONTEXT, NULL);
  if (win->egl.ctx == EGL_NO_CONTEXT) {
    kk_log (KK_LOG_WARNING, "eglCreateContext failed.");
    return -1;
  }

  win->egl.surface =
      eglCreateWindowSurface (win->egl.dpy, win->egl.conf,
          (EGLNativeWindowType) win->window, NULL);
  if (win->egl.surface == EGL_NO_SURFACE) {
    kk_log (KK_LOG_WARNING, "eglCreateWindowSurface failed.");
    return -1;
  }

  status =
      eglMakeCurrent (win->egl.dpy, win->egl.surface, win->egl.surface,
          win->egl.ctx);
  if (status == EGL_FALSE) {
    kk_log (KK_LOG_WARNING, "eglMakeCurrent failed.");
    return -1;
  }

  return 0;
}

static int
window_init_cairo (kk_window_wayland_t *win)
{
  win->cairo.device = cairo_egl_device_create (win->egl.dpy, win->egl.ctx);
  if (cairo_device_status (win->cairo.device) != CAIRO_STATUS_SUCCESS) {
    kk_log (KK_LOG_WARNING, "Creating cairo egl device failed.");
    return -1;
  }

  win->cairo.surface = cairo_gl_surface_create_for_egl (win->cairo.device,
      win->egl.surface, win->base.width, win->base.height);
  if (cairo_surface_status (win->cairo.surface) != CAIRO_STATUS_SUCCESS) {
    kk_log (KK_LOG_WARNING, "Creating cairo surface from egl device failed.");
    return -1;
  }
  return 0;
}

static int
window_init (kk_window_t *win_base)
{
  kk_window_wayland_t *win = (kk_window_wayland_t *) win_base;

  /**
   * Connect to Wayland
   */
  win->display = wl_display_connect (NULL);
  if (win->display == NULL) {
    kk_log (KK_LOG_WARNING, "wl_display_connect failed.");
    return -1;
  }

  /**
   * Registry allows the client to list and bind the global objects
   * available from the compositor.
   */
  win->registry = wl_display_get_registry (win->display);
  if (win->registry == NULL) {
    kk_log (KK_LOG_WARNING, "wl_display_get_registry failed.");
    return -1;
  }

  /**
   * Listener contains callbacks that tell us about global objects
   * becoming available and global objects getting removed.
   */
  if (wl_registry_add_listener (win->registry, &registry_listener, win) != 0) {
    kk_log (KK_LOG_WARNING, "wl_registry_add_listener failed.");
    return -1;
  }

  if (pthread_create (&win->thread, 0,
          (void *(*)(void *)) window_event_handler, win) != 0)
    return -1;

  return 0;
}

static int
window_free (kk_window_t *win_base)
{
  kk_window_wayland_t *win = (kk_window_wayland_t *) win_base;

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
    eglMakeCurrent (win->egl.dpy,
        EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
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

  return 0;
}

static int
window_show (kk_window_t *win_base)
{
  kk_window_wayland_t *win = (kk_window_wayland_t *) win_base;

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

  win->window = wl_egl_window_create (win->surface, win->base.width, win->base.height);
  if (win->window == NULL) {
    kk_log (KK_LOG_WARNING, "wl_egl_window_create");
    return -1;
  }

  wl_shell_surface_add_listener (win->shell_surface,
      &shell_surface_listener, win);
  wl_shell_surface_set_toplevel (win->shell_surface);

  if (window_init_egl (win) != 0) {
    kk_log (KK_LOG_WARNING, "Initializing EGL failed.");
    return -1;
  }

  if (window_init_cairo (win) != 0 ) {
    kk_log (KK_LOG_WARNING, "Initializing cairo failed.");
    return -1;
  }

  shell_surface_handle_configure (win, win->shell_surface, 0,
      win->base.width, win->base.height);

  wl_display_flush (win->display);
  return 0;
}

static int
window_draw (kk_window_t *win_base)
{
  kk_window_wayland_t *win = (kk_window_wayland_t *) win_base;

  cairo_t *ctx;

  ctx = cairo_create (win->cairo.surface);
  if (cairo_status (ctx) != CAIRO_STATUS_SUCCESS)
    return -1;

  cairo_set_source_rgb (ctx, 0.0, 0.0, 0.0);
  cairo_rectangle (ctx, 0.0, 0.0, win->base.width, win->base.height);
  cairo_fill (ctx);

  kk_widget_invalidate ((kk_widget_t *) win);
  kk_widget_draw ((kk_widget_t *) win, ctx);
  cairo_destroy (ctx);

  cairo_gl_surface_swapbuffers (win->cairo.surface);
  return 0;
}

static int
window_set_title (kk_window_t *win_base, const char *title)
{
  kk_window_wayland_t *win = (kk_window_wayland_t *) win_base;

  if (win->shell_surface == NULL)
    return -1;

  wl_shell_surface_set_title (win->shell_surface, title);
  return 0;
}
