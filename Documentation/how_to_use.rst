=========================================
How to use the simple Linux sound library
=========================================

:Date: 2018-07-29


Parameters
==========

First of all, ``struct snd_config`` must be filled. Below
are its members and their meaning.

Flags
-----

Flags to be ORed (e.g. SND_OUTPUT | SND_MMAP).

- (required) SND_OUTPUT for playback or SND_INPUT for
  capture. Do not use both.

- SND_NONBLOCK do not block I/O operations. At the moment
  this only works in ACCESS_RW mode (i.e. if SND_MMAP is
  not set).

- SND_MMAP for mmap access to sound device buffer.

- SND_MONOTONIC for timestamps from CLOCK_MONOTONIC.
  Otherwise it's CLOCK_REALTIME.

- SND_NOIRQ for disabling interrupts.

Card and device
---------------

These parameters define the hardware to open. See
``pcmC*D*`` files in ``/dev/snd/``. **C** stands for Card
and **D** for Device.

The ``pcm`` file in ``/dev/snd/`` is the way the sound
library communicates to ALSA.

Format
------

Defines the resolution of the samples. The higher the
resolution, the more amplitude values can be represented.

Examples:

- SND_FORMAT_U8 (8 bits)

- SND_FORMAT_S16_LE (16 bits)

**U** stands for Unsigned, **S** for Signed, **LE** for
Little-Endian, **BE** for Big-Endian. The number is the
resolution in bits. A single byte (8 bits) doesn't have
endianness.

Channels
--------

The number of samples to be played or captured together,
making a **frame**.

Example: Mono (1 channel), Stereo (2 channels).

This is generally used to each sample carry audio for
a different speaker.

Rate
----

Defines the number of frames per second. In a CD standard
it's 44100. Some argue that more than this is not
necessary because human can't hear so [1_, 2_].

.. _1: https://xiph.org/~xiphmont/demo/neil-young.html

.. _2: https://www.soundonsound.com/sound-advice/
       q-it-worth-recording-higher-sample-rate

Period size
-----------

Defines the interval (in frames) to wake up the application
for filling a period.

After every period processed the sound device generates an
interrupt.


Functions
=========

All functions of the library and what they do.

- snd_open(): Open sound device and set parameters.

- snd_close(): Close sound device.

- snd_sync(): Set/get application pointer and avail_min
  (minimum available frames for application wakeup).
  Request hardware pointer. Note: avail_min doesn't need
  to be touched by application, and the kernel never
  touches it.

- snd_start(): Start sound device.

- snd_stop(): Stop sound device.

- snd_trigger_ts(): Get timestamp of the last state
  change. Usually used to get start timestamp.

- snd_read(): Read frames from sound device buffer.

- snd_write(): Write frames to sound device buffer.


Example of use
==============

A simple application that plays ``your_buffer``::

	#include <poll.h>   /* poll() */
	#include <string.h> /* memset() */
	
	#include "sound.h"
	
	int
	main(void)
	{
		struct snd        snd;
		struct snd_config cfg;
	
		struct pollfd pfd;
	
		memset(&cfg, 0, sizeof(cfg));
		cfg.flags       = SND_OUTPUT;
		cfg.card        = 0;
		cfg.device      = 0;
		cfg.format      = SND_FORMAT_S16_LE;
		cfg.channels    = 2;
		cfg.rate        = 44100;
		cfg.period_size = 4410; /* 10 milliseconds */
	
		if (snd_open(&snd, &cfg) == -1)
			return 1;
	
		snd_start(&snd);
	
		while (1) {
			pfd.fd      = snd.fd;
			pfd.events  = POLLOUT;
			pfd.revents = 0;
	
			if (poll(&pfd, 1, -1) == -1) {
				snd_close(&snd);
				return 1;
			}
	
			/* fill your_buffer */
	
			snd_write(&snd, your_buffer, cfg.period_size);
		}
	
		snd_close(&snd);
	
		return 0;
	}
