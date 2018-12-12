==============
ALSA internals
==============

:Date: 24/05/2017

This document covers some details of ALSA (Advanced Linux
Sound Architecture) inside the Linux kernel.

In ALSA, PCM (Pulse Code Modulation) refers to the sound
device and general digital audio processing.

The files mentioned here are in ``sound/core/`` in Linux
kernel source tree.


The two sound transfer modes
============================

There are two ways to transfer sound to/from device's
buffer.

- ACCESS_RW (Read/Write): Application calls the kernel
  using ``ioctl()``, the latter then transfer the data
  to/from sound buffer.

- ACCESS_MMAP (Memory Map): Application transfer data
  directly to sound device buffer [1]_ (via a memory
  mapping). The memory address is obtained using ``mmap()``
  system call. This is even more efficient when the sound
  device has Direct Memory Access (DMA).

.. [1] In some cases when the sound device doesn't allow
   mapping its buffer on memory, the driver may sit in the
   middle and copy the data to sound device.


hardware and application pointers
=================================

Both of hardware and application pointers aren't real
pointers, they're counters instead.

Hardware pointer (hw_ptr) is the sum of frames processed
by sound device. It is incremented on every interrupt by
period size. It's also possible to request an update
manually, however the accuracy will depend on device and
driver.

Application pointer (appl_ptr) is the sum of frames read
or written by application in ring buffer. It's incremented
by the application itself or when it calls one of the
transfer routines inside the kernel.

Hardware and application pointers round up to the size of
boundary calculated in ``snd_pcm_hw_params()`` inside
``pcm_native.c``::

	boundary = buffer_size;
	while (boundary * 2 <= LONG_MAX - buffer_size)
		boundary *= 2;

The calculation makes boundary a multiple of buffer_size
and less than LONG_MAX - buffer_size. The factor being a
power of two is meaningless [2_].

.. _2: http://mailman.alsa-project.org/
   pipermail/alsa-devel/2018-July/138183.html

When hardware or application pointer exceeds boundary it
is reset to (0 + amount_exceeded).


Device states
=============

There are nine different sound device states:

- STATE_OPEN: The file descriptor is open.

- STATE_SETUP: The file descriptor is open, and hardware
  parameters were set.

- STATE_PREPARED: The file descriptor is open, hardware
  parameters were set, and the device is ready to start
  capture/playback.

- STATE_RUNNING: Device is playing or capturing audio.

- STATE_XRUN: Device has entered in xrun state and need
  to be prepared (STATE_PREPARED) and started
  (STATE_RUNNING) again in order to run again.

- STATE_DRAINING: Device is draining audio. When it
  finish, the state is going to be STATE_SETUP.

- STATE_PAUSED: Device is paused.

- STATE_SUSPENDED: Device is suspended.

- STATE_DISCONNECTED: Device is disconnected.


Available
=========

Also known as avail.

It is a simple calculation done using hardware and
application pointers. See ``tools/avail.h``.

It results:

- For playback: The number of frames available to be
  written to buffer.

- For capture: The number of frames available to be
  read from buffer.

ALSA keeps a value called ``avail_min`` (minimum available)
that specifies the minimum value for avail in order
to wake up the application. See
`When ALSA wakes up the application`_ section below.


SYNC_PTR operation
==================

Synchronize application and hardware pointers.

The synchronization is done via
``ioctl(fd, SNDRV_PCM_IOCTL_SYNC_PTR, sync_ptr)`` or
via a ``mmap()`` area. Both ways use the same struct
``struct snd_pcm_sync_ptr``.

The ioctl operation is ``snd_pcm_sync_ptr()`` in
``pcm_native.c``.

``struct snd_pcm_sync_ptr`` contains ``flags``,
``struct snd_pcm_mmap_status``,
``struct snd_pcm_mmap_control``.

Flags
-----

- ``SNDRV_PCM_SYNC_PTR_HWSYNC``: Request hardware pointer
  update. See `HWSYNC operation`_ section below.

- ``SNDRV_PCM_SYNC_PTR_APPL``: Returns application poiter
  from kernel. Otherwise, updates application pointer in
  kernel.

- ``SNDRV_PCM_SYNC_PTR_AVAIL_MIN``: Returns minimum
  available from kernel. Otherwise, update minimum
  available in kernel. Usually, this value is not changed by
  application, and the kernel never changes it.

Status
------

The ``struct snd_pcm_mmap_status`` structure is used only
to get information from ALSA.

- ``state``: Sound device state.

- ``hw_ptr``: Where hardware pointer is returned. See
  ``SNDRV_PCM_SYNC_PTR_HWSYNC`` flag.

- ``tstamp`` Timestamp taken in every hardware pointer
  update (manually or in interrupt) and also after a xrun.

- ``suspended_state``: The state just before the sound
  device is suspended. When suspending, ``state`` is set
  to ``STATE_SUSPENDED``.

- ``audio_tstamp``: Time calculated from the total frames
  since the start of sound device.

If interrupts are enabled, hw_ptr and tstamp were (if no
HWSYNC occurred) updated in the last interrupt.

Control
-------

The ``struct snd_pcm_mmap_control`` structure is used to
get and set information.

- ``appl_ptr``: Application pointer. See
  ``SNDRV_PCM_SYNC_PTR_APPL`` flag.

- ``avail_min``: Minimum available (see `Available`_
  section above). See ``SNDRV_PCM_SYNC_PTR_APPL`` flag.


HWSYNC operation
================

Request hardware pointer synchronization. The only way
this is done is by using ``SNDRV_PCM_IOCTL_HWSYNC``
ioctl. Here is what happens inside the kernel when calling
this:

Call trace::

	0 [pcm_native.c] snd_pcm_hwsync()
	1 [pcm_lib.c] snd_pcm_update_hw_ptr
	2 snd_pcm_update_hw_ptr0()
	3 [in driver] pointer()

``snd_pcm_hwsync()`` calls ``snd_pcm_update_hw_ptr()``,
which calls ``snd_pcm_update_hw_ptr0()`` with
``in_interrupt`` set to **zero**.

``snd_pcm_update_hw_ptr0()`` request pointer from device
driver, add the value to hardware pointer (with some
additional calculations), and calls
``snd_pcm_update_state()``.

``snd_pcm_update_state()`` updates sound state, issues
xrun state if ``avail >= stop_threshold``, and
wake up applications waiting for ``avail >= avail_min``.

The ``pointer()`` function inside device driver returns
the position within buffer size. If it returns -1 a xrun
is issued.


F.A.Q.
======

When ALSA wakes up the application
----------------------------------

In ``snd_pcm_update_state()``, ``pcm_lib.c``, when
``avail >= avail_min``.

``snd_pcm_update_state()`` is mainly called after hardware
pointer update (at the end of ``snd_pcm_update_hw_ptr0()``).

When ALSA issues a xrun
-----------------------

- In ``snd_pcm_update_state()``, ``pcm_lib.c``, when
  ``avail >= stop_threshold``.

- If device driver returns -1 as the buffer position.

When ALSA wake up ACCESS_RW transfer routines
---------------------------------------------

ALSA has ``runtime.sleep`` for userspace polling calls,
and ``runtime.tsleep`` for internal ACCESS_RW transfer
routines.

How to get timestamp of the last interrupt
------------------------------------------

::

	sptr.flags = SNDRV_PCM_SYNC_PTR_APPL | SNDRV_PCM_SYNC_PTR_AVAIL_MIN;
	ioctl(fd, SNDRV_PCM_IOCTL_SYNC_PTR, &sptr);
	last_irq_ts = sptr.s.status.tstamp;

It is possible to set flags to zero, but then application
would be sending its appl_ptr and avail_min values to
ALSA, and it's not necessary. SNDRV_PCM_SYNC_PTR_HWSYNC
cannot be used because it updates the hardware pointer
and timestamp (see ``update_audio_tstamp()`` in
``pcm_lib.c``).

How to get the start timestamp
------------------------------

::

	ioctl(fd, SNDRV_PCM_IOCTL_STATUS, &status);
	tstamp = status.trigger_tstamp;

``status`` is ``struct snd_pcm_status``. It's different
from ``struct snd_pcm_mmap_status`` structure used in
SYNC_PTR operation.

What are the thresholds
-----------------------

Thresholds are software parameters. See
``snd_pcm_sw_params()`` function in ``pcm_native.c``.

See the mentions for thresholds in kernel code by
searching "_threshold" in ``pcm_lib.c`` and
``pcm_native.c``.

Start threshold
~~~~~~~~~~~~~~~

Automatically starts sound device after writing (playback)
or reading (capture) ``start_threshold`` or more frames
using ``ACCESS_RW`` (ioctl) transfer. Setting it greater
than buffer size disables the feature.

Stop threshold
~~~~~~~~~~~~~~

Automatically stops sound device if
``avail >= stop_threshold``. See `Available`_ section
above.

This action may take place in ``ACCESS_RW`` transfers
and in hardware pointer updates (manually or after an
interrupt). The sound device goes to xrun state.

Silence threshold
~~~~~~~~~~~~~~~~~

Playback only. Automatically fill with silence
``silence_threshold`` frames ahead of the current
application pointer. It is usable for applications when an
underrun is possible. (from alsa-lib documentation)


Files
=====

Some files it's interesting to take a look. Main files of
ALSA are in ``sound/core/``.

- ``pcm_native.c``: Handling of ioctls.

- ``pcm_lib.c``: Hardware pointer update, interrupt
  handling, ACCESS_RW transfer routines, hardware
  parameters manipulation.

- Main header files (in ``include/sound/``): ``pcm.h``,
  ``pcm_params.h``. It's also interesting to take a look
  at ``timer.h`` (timer interface), ``core.h`` (card
  management, etc).

- Intel hda pcm driver (perhaps the most known one)
  operations are in ``sound/pci/hda/hda_controller.c``.
