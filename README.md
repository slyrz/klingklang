# klingklang
![alt tag](https://raw.github.com/slyrz/klingklang/master/img/klingklang.png)

A minimal music player for Linux.

## Requirements
You need to have the following libraries/programs installed:

* cairo
* libav or ffmpeg
* xcb
* xcb-event
* xcb-icccm
* xcb-keysyms
* dmenu

## Supported Audio Backends
You need to have one of the following libraries installed:

* alsa
* ao
* portaudio
* pulseaudio

Choosing alsa as audio backend on Linux systems with pulseaudio installed is not recommended.

## Installation

    autoreconf --force --install
    ./configure --with-backend={alsa|ao|portaudio|pulseaudio}
    make
    make install

## Commands

* `CTRL` + `A` - Add
* `CTRL` + `C` - Clear
* `CTRL` + `N` - Next
* `CTRL` + `P` - Pause
