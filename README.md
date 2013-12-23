# klingklang
![alt tag](https://raw.github.com/slyrz/klingklang/master/img/klingklang.png)

A minimal music player. Metadata free. Manage your music library with mv/cp and friends.

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
Files created by autotools are not kept under version control.
It's necessary to create the missing Makefile and configure script by running

    autoreconf --force --install

Before you can run make, you have to execute the configure script.

    ./configure --with-backend={alsa|ao|portaudio|pulseaudio|sndio}

If you want to test and debug *klingklang*, run

    ./configure --with-backend={alsa|ao|portaudio|pulseaudio|sndio} --enable-debugging

instead. This will disable compiler optimization, but turn on additional warning flags and debug symbols.
Now you're able to compile *klingklang* by running

    make
    make install

The second command is optional. Execute it if you want to install the *klingklang* binary on your system.

## Commands

* `CTRL` + `A` - Add
* `CTRL` + `C` - Clear
* `CTRL` + `N` - Next
* `CTRL` + `P` - Pause
* `CTRL` + `R` - Rewind
