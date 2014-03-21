#include <klingklang/ui/window.h>
#include <klingklang/str.h>
#include <klingklang/util.h>

#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>

#include <cairo.h>
#include <cairo-xcb.h>

#ifdef HAVE_XCB_ICCCM_PREFIX
#  define xcb_set_wm_protocols xcb_icccm_set_wm_protocols
#endif

#define KK_WINDOW_MAX_TITLE_LEN         0x80u
#define KK_WINDOW_MAX_ATOM_LEN          0x80u

typedef struct kk_window_xcb_s kk_window_xcb_t;

struct kk_window_xcb_s {
  kk_window_t base;
  xcb_connection_t *connection;
  xcb_screen_t *screen;
  xcb_window_t window;
  struct {
    cairo_surface_t *surface;
    cairo_t *context;
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
  .size = sizeof (kk_window_xcb_t),
  .init = window_init,
  .free = window_free,
  .show = window_show,
  .draw = window_draw,
  .set_title = window_set_title
};

static xcb_atom_t
window_get_atom (kk_window_xcb_t *win, const char *name)
{
  xcb_intern_atom_cookie_t cookie;
  xcb_intern_atom_reply_t *reply;
  xcb_atom_t atom;

  size_t len;

  if (name == NULL)
    return XCB_NONE;

  len = kk_str_len (name, KK_WINDOW_MAX_ATOM_LEN);
  if (len == KK_WINDOW_MAX_ATOM_LEN) {
    kk_log (KK_LOG_WARNING, "Name of atom exceeds maximum atom length.");
    return XCB_NONE;
  }

  cookie = xcb_intern_atom (win->connection, 0, (uint16_t) len, name);
  reply = xcb_intern_atom_reply (win->connection, cookie, NULL);
  if (reply == NULL)
    return XCB_NONE;

  atom = reply->atom;
  free (reply);
  return atom;
}

static xcb_visualtype_t *
window_get_visual_type (kk_window_xcb_t *win)
{
  xcb_depth_iterator_t depth_iter;
  xcb_visualtype_iterator_t visual_iter;
  xcb_visualtype_t *visual_type;

  depth_iter = xcb_screen_allowed_depths_iterator (win->screen);

  visual_type = NULL;
  for (; depth_iter.rem; xcb_depth_next (&depth_iter)) {
    visual_iter = xcb_depth_visuals_iterator (depth_iter.data);
    for (; visual_iter.rem; xcb_visualtype_next (&visual_iter)) {
      if (win->screen->root_visual == visual_iter.data->visual_id) {
        visual_type = visual_iter.data;
        break;
      }
    }
  }
  return visual_type;
}

static void
window_configure_notify (kk_window_xcb_t *win, xcb_configure_notify_event_t *event)
{
  kk_window_event_resize (win->base.events, event->width, event->height);
}

static void
window_expose (kk_window_xcb_t *win, xcb_expose_event_t *event)
{
  (void) event;

  kk_widget_invalidate ((kk_widget_t *) win);
  kk_window_event_expose (win->base.events, win->base.width, win->base.height);
}

static void
window_key_press (kk_window_xcb_t *win, xcb_key_press_event_t *event)
{
  int key = 0;
  int mod = 0;

  key = kk_keys_get_symbol (win->base.keys, event->detail);

  if (event->state & XCB_MOD_MASK_SHIFT)
    mod |= KK_MOD_SHIFT;

  if (event->state & XCB_MOD_MASK_CONTROL)
    mod |= KK_MOD_CONTROL;

  kk_window_event_key_press (win->base.events, mod, key);
}

static void
window_mapping_notify (kk_window_xcb_t *win, xcb_mapping_notify_event_t *event)
{
  /**
   * TODO: find a way to refresh keyboard mapping or completely switch back to
   * xcb_keysyms?
   * xcb_refresh_keyboard_mapping (win->syms, event);
   */
  (void) win;
  (void) event;
}

static void *
window_event_handler (kk_window_xcb_t *win)
{
  xcb_generic_event_t *event;

  win->alive = 1;
  while ((event = xcb_wait_for_event (win->connection))) {
    switch (XCB_EVENT_RESPONSE_TYPE (event)) {
      case XCB_CONFIGURE_NOTIFY:
        window_configure_notify (win, (xcb_configure_notify_event_t *) event);
        break;
      case XCB_EXPOSE:
        window_expose (win, (xcb_expose_event_t *) event);
        break;
      case XCB_KEY_PRESS:
        window_key_press (win, (xcb_key_press_event_t *) event);
        break;
      case XCB_MAPPING_NOTIFY:
        window_mapping_notify (win, (xcb_mapping_notify_event_t *) event);
        break;
      case XCB_CLIENT_MESSAGE:
        win->alive = 0;
        break;
      default:
        break;
    }
    free (event);
    if (!win->alive)
      break;
  }
  kk_window_event_close (win->base.events);
  win->alive = 0;
  return NULL;
}

static int
window_init (kk_window_t *win_base)
{
  kk_window_xcb_t *win = (kk_window_xcb_t *) win_base;

  win->connection = xcb_connect (NULL, NULL);
  if (win->connection == NULL)
    return -1;

  const xcb_setup_t *setup = xcb_get_setup (win->connection);
  if (setup == NULL)
    return -1;

  win->screen = xcb_setup_roots_iterator (setup).data;
  if (win->screen == NULL)
    return -1;

  if (pthread_create (&win->thread, 0, (void *(*)(void *)) window_event_handler, win) != 0)
    return -1;

  return 0;
}

static int
window_free (kk_window_t *win_base)
{
  kk_window_xcb_t *win = (kk_window_xcb_t *) win_base;

  if (win->alive)
    pthread_cancel (win->thread);

  if (win->thread)
    pthread_join (win->thread, NULL);

  if (win->cairo.context)
    cairo_destroy (win->cairo.context);

  if (win->cairo.surface) {
    cairo_surface_flush (win->cairo.surface);
    cairo_surface_finish (win->cairo.surface);
    cairo_surface_destroy (win->cairo.surface);
  }

  if (win->window)
    xcb_destroy_window (win->connection, win->window);

  /**
   * Disconnecting causes a memory leak if the connection had an error,
   * but there's nothing we can do about it. The function xcb_disconnect
   * doesn't do anything if the has_error flag of the xcb_connection_t struct
   * is true. However, we can't set this field to false since xcb.h exports
   * xcb_connection_t as opaque struct.
   * And besides ruining our valgrind reports, it really doesn't affect us.
   */
  if (win->connection)
    xcb_disconnect (win->connection);

  return 0;
}

static int
window_set_title (kk_window_t *win_base, const char *title)
{
  kk_window_xcb_t *win = (kk_window_xcb_t *) win_base;
  size_t len;

  if (win->window == 0u)
    return -1;

  len = kk_str_len (title, KK_WINDOW_MAX_TITLE_LEN);
  if (len == KK_WINDOW_MAX_TITLE_LEN)
    kk_log (KK_LOG_WARNING, "Title exceeds maximum title length.");

  xcb_change_property (win->connection, XCB_PROP_MODE_REPLACE, win->window,
      XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, (uint32_t) len, title);
  xcb_flush (win->connection);
  return 0;
}

static int
window_show (kk_window_t *win_base)
{
  kk_window_xcb_t *win = (kk_window_xcb_t *) win_base;

  xcb_visualtype_t *visual_type;
  uint32_t value_mask;
  uint32_t value_list[3];

  value_mask = XCB_CW_BACK_PIXEL
    | XCB_CW_OVERRIDE_REDIRECT
    | XCB_CW_EVENT_MASK;

  value_list[0] = win->screen->black_pixel;
  value_list[1] = 0;
  value_list[2] = XCB_EVENT_MASK_KEY_PRESS
    | XCB_EVENT_MASK_EXPOSURE
    | XCB_EVENT_MASK_PROPERTY_CHANGE
    | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

  win->window = xcb_generate_id (win->connection);
  xcb_create_window (win->connection,
      win->screen->root_depth,
      win->window,
      win->screen->root,
      0,
      0,
      (uint16_t) win->base.width,
      (uint16_t) win->base.height,
      0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT,
      win->screen->root_visual,
      value_mask,
      value_list);

  /**
   * The following code allows us to receive a XCB_CLIENT_MESSAGE
   * event when the window get's closed. This allows us to shut down
   * the xcb session gracefully.
   */
  xcb_atom_t del = window_get_atom (win, "WM_DELETE_WINDOW");
  xcb_atom_t prt = window_get_atom (win, "WM_PROTOCOLS");

  if ((del != XCB_NONE) && (prt != XCB_NONE))
    xcb_set_wm_protocols (win->connection, win->window, prt, 1, &del);

  xcb_map_window (win->connection, win->window);
  xcb_flush (win->connection);

  visual_type = window_get_visual_type (win);
  if (visual_type == NULL)
    return -1;

  win->cairo.surface = cairo_xcb_surface_create (win->connection, win->window,
      visual_type, win->base.width, win->base.height);
  if (cairo_surface_status (win->cairo.surface) != CAIRO_STATUS_SUCCESS)
    return -1;

  win->cairo.context = cairo_create (win->cairo.surface);
  if (cairo_status (win->cairo.context) != CAIRO_STATUS_SUCCESS)
    return -1;

  return 0;
}

static int
window_draw (kk_window_t *win_base)
{
  kk_window_xcb_t *win = (kk_window_xcb_t *) win_base;

  if (win->base.resized)
    cairo_xcb_surface_set_size (win->cairo.surface, win->base.width, win->base.height);
  kk_widget_draw ((kk_widget_t*) win, win->cairo.context);
  xcb_flush (win->connection);
  return 0;
}
