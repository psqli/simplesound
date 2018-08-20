==============================
Simple Linux sound I/O library
==============================

:Date: 2017-08-27


How to use
==========

Here is a simple setup, for more details read
``Documentation/how_to_use.rst``.

1. Set flags, card, device, format, channels, rate and
   period_size in ``struct snd_config``.

- flags: SND_INPUT or SND_OUTPUT.
- card and device: Usually both are set to zero. See
  ``/dev/snd/`` directory.
- format: Defines the sample resolution. SND_FORMAT_S16_LE
  is the most common. See ``snd_parameters.h`` for more.
- channels: one (mono), two (stereo).
- rate: frames per second. 44100 is the most common.
- period_size: size of a period. This defines the latency.

2. Call ``snd_open((struct snd*), (struct snd_config*))``.

3. Call ``snd_start((struct snd*))``.

4. Poll in ``fd`` file descriptor of ``struct snd``.

5. On wake up, use ``snd_write()`` and write ``period_size``
   frames.

6. When done, call ``snd_close()``.


Documentation
=============

If you think something can be improved or added in the
documentation please let me know by sending an email
to ``pasqualirb at gmail dot com`` or by creating an issue
in GitHub.


Files
=====

- ``sound_setup.c``: open/close device, set
  hardware/software parameters, mmap/munmap areas.

- ``sound_operations.c``: synchronize, start, stop sound
  device.

- ``sound_transfer.c``: transfer helpers.

- ``sound_parameters.c``: helpers to obtain the allowed
  values for hardware parameters. It's actually wrappers
  to a few functions from ``hardware_parameters.c``.

- ``hardware_parameters.c``: manipulate hardware parameters
  (bit masking, conversions, etc).

- ``sound_global.h``: global header file. Contains sound
  structures and macros.
