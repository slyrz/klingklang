bin_PROGRAMS = klingklang

klingklang_SOURCES = \
  src/device.c \
  src/event.c \
  src/format.c \
  src/frame.c \
  src/input.c \
  src/library.c \
  src/list.c \
  src/main.c \
  src/player.c \
  src/str.c \
  src/timer.c \
  src/ui/cover.c \
  src/ui/image.c \
  src/ui/keys.c \
  src/ui/progressbar.c \
  src/ui/widget.c \
  src/ui/window/xcb.c \
  src/util.c

klingklang_WARNINGS = \
  -Wall \
  -Wextra

if DEBUGGING
  klingklang_WARNINGS += \
    @VENDOR_FLAGS@ \
    -Wbad-function-cast  \
    -Wcast-qual \
    -Wconversion  \
    -Wfloat-equal \
    -Wformat=2 \
    -Winit-self \
    -Winline  \
    -Wmissing-declarations \
    -Wmissing-include-dirs \
    -Wmissing-prototypes \
    -Wnested-externs \
    -Wno-format-nonliteral \
    -Wold-style-definition \
    -Wpointer-arith \
    -Wshadow  \
    -Wstrict-overflow=2 \
    -Wstrict-prototypes  \
    -Wswitch-enum \
    -Wundef  \
    -Wuninitialized \
    -Wunreachable-code \
    -Wwrite-strings
endif

klingklang_CFLAGS = -pthread -I$(top_builddir)/include
klingklang_LDADD = $(libavcodec_LIBS) $(libavformat_LIBS) $(libavutil_LIBS)

klingklang_CFLAGS += $(xcb_CFLAGS) $(xcb_icccm_CFLAGS)  $(xcb_event_CFLAGS)
klingklang_LDADD += $(xcb_LIBS) $(xcb_icccm_LIBS) $(xcb_event_LIBS)

klingklang_CFLAGS += $(cairo_CFLAGS)  $(xkbcommon_CLFAGS) $(libswscale_CFLAGS)
klingklang_LDADD += $(cairo_LIBS) $(xkbcommon_LIBS) $(libswscale_LIBS)

klingklang_CFLAGS += $(klingklang_WARNINGS)

if BACKEND_ALSA
  klingklang_SOURCES += src/device/alsa.c
  klingklang_CFLAGS += $(libasound_CFLAGS)
  klingklang_LDADD += $(libasound_LIBS)
endif

if BACKEND_AO
  klingklang_SOURCES += src/device/ao.c
  klingklang_CFLAGS += $(libao_CFLAGS)
  klingklang_LDADD += $(libao_LIBS)
endif

if BACKEND_PORTAUDIO
  klingklang_SOURCES += src/device/portaudio.c
  klingklang_CFLAGS += $(portaudio_CFLAGS)
  klingklang_LDADD += $(portaudio_LIBS)
endif

if BACKEND_PULSEAUDIO
  klingklang_SOURCES += src/device/pulseaudio.c
  klingklang_CFLAGS += $(pulseaudio_CFLAGS)
  klingklang_LDADD += $(pulseaudio_LIBS)
endif

if BACKEND_SNDIO
  klingklang_SOURCES += src/device/sndio.c
  klingklang_LDADD += -lsndio
endif

if BACKEND_OSS
  klingklang_SOURCES += src/device/oss.c
endif
