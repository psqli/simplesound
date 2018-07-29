=======================
Sound inside the kernel
=======================

:Date: 2017-03
:Date: 2017-08-26
:Date: 2018-07-13

This document explains how the kernel (a.k.a. operating
system) handles the sound. What happens between sound
device and application. How application communicates.


Overview
========

Sound is broken in pieces of the same size (**periods**) to
allow the computer work in other tasks while the sound
device is working. The periods are constituted by frames.

The application must transfer periods to/from sound device
before it runs out of data (**underrun** in playback) or
before it overwrites what have not been read (**overrun**
in capture).

The term **xrun** refers to both underrun and overrun.


Sound device buffer
===================

The sound device has its own space in memory (the buffer)
where it produces (capture) or consume (playback) data.

The buffer is a ring buffer. It means that when the device
reaches the end it goes back to the beginning.

In some devices, buffer size doesn't need to be a multiple
of period size. However it generally requires at least 2
times period size because the application usually wake up
after every period for transferring data (see
`Sound device interrupts`_ section below).


Sound device interrupts
=======================

For what is an interrupt read ``what_are_interrupts.rst``.

The sound device interrupts the processor after every
period processed. The application is then informed by the
kernel via the sound device's file descriptor::

	|  period 0  |  period 1  |
	            IRQ          IRQ

It means, in playback, that a period was consumed and it's
available to be rewritten by the application; and, in
capture, that a period was produced and it's ready to be
read.

The sound handling code inside kernel, while handling the
interrupt, increments a counter (by period size) of frames
the sound device have processed called hardware pointer
(NOTE: it's not a pointer!).

It's possible to configure the audio device to disable
interrupts, so the application needs to use a timer for
waking up and also keep track of the hardware pointer by
querying the operating system (HWSYNC operation in ALSA).
