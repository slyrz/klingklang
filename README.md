# klingklang
![alt tag](https://raw.github.com/slyrz/klingklang/master/img/klingklang.png)

A minimal music player for Linux.

## Requirements
You need to have the following libraries/programs installed:

* [cairo](http://cairographics.org/)
* [dmenu](http://tools.suckless.org/dmenu/)
* [libav](http://libav.org/) or [ffmpeg](http://www.ffmpeg.org/)
* [xcb](http://xcb.freedesktop.org/)
* [xcb-event](http://xcb.freedesktop.org/)
* [xcb-icccm](http://xcb.freedesktop.org/)
* [xcb-keysyms](http://xcb.freedesktop.org/)
* [xcb-util](http://xcb.freedesktop.org/)

Please make sure your *cairo* library was build with *xcb* support.

## Supported Audio Backends
You need to have one of the following libraries installed:

* [alsa](http://www.alsa-project.org/)
* [ao](http://www.xiph.org/ao/)
* [portaudio](http://www.portaudio.com/)
* [pulseaudio](http://www.freedesktop.org/wiki/Software/PulseAudio/)
* [sndio](http://www.sndio.org/)

Choosing *alsa* as audio backend on Linux systems with *pulseaudio* installed is not recommended.

## Installation
The following command generates the missing Makefile and configure script:

    autoreconf --force --install

To compile *klingklang*, run:

    ./configure --with-backend={alsa|ao|portaudio|pulseaudio|sndio}
    make

To install *klingklang*, run:

    make install

## Commands

* `CTRL` + `A` - Add
* `CTRL` + `C` - Clear
* `CTRL` + `N` - Next
* `CTRL` + `P` - Pause
