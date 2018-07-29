================================
System timer application wake up
================================

:Date: 2018-06-23


Overview
========

There are two methods for application wake up:

- Sound device interrupt (sound IRQ)
- System timer interrupt (timer IRQ)

The conventional is sound IRQ. Application waits on the
sound device file descriptor and the kernel wake it up
after a sound IRQ.

This document covers timer IRQ. The clock source is from
system instead of sound device.


System timer wake up
====================

System timer (T) and sound device (S)::

	T | period 0 | period 1 | period 2 | period 3 | period 4 |
	
	S |         period 0         |         period 1         |
	                            IRQ                        IRQ

The application sees the timer period, not the hardware's
one.

There are two actual code implementations:

- timerfd: A file descriptor that delivers timer events.

- Deadline scheduler: Scheduler that runs periodic tasks
  within a deadline.

Both implementations use a timer internally.

There are two synchronization techniques:

- Corrections applied in the timer itself. Only possible
  with timerfd implementation because it can be reliably
  set.

- Corrections applied in application pointer.

If the sound device does not allow writing anywhere and
anytime in its buffer, system timer wake up will work
**only** if corrections are applied in the timer, and the
timer period size is a multiple of the sound device's one
(i.e. behaving like an interrupt).

As the system and sound device clock diverges, the user
must obtain the fill level of sound device buffer in order
to synchronize. This information can be obtained in two
ways:

- Requesting the hardware pointer in every wake up.
  Interrupts can be disabled.

- With interrupts enabled, use hardware pointer from the
  last interrupt and the time since that to predict where
  the hardware pointer is.

The latter is only used if the sound device doesn't return
an accurate value when requesting hardware pointer::

	Some hardware doesn't realiably tell the current
	position in the buffer.
	
	Clemens Ladisch, alsa-dev mailing list, 20/01/2009


Further information
===================

Why timerfd
-----------

The timerfd API seems to be more elegant because the user
might not even notice that it is polling on a timer file
descriptor instead of sound device's one. Other timer APIs
such as POSIX timers have the same functionality, however
they don't notify via a file descriptor.

Deadline scheduler
------------------

The Linux deadline scheduler (SCHED_DEADLINE) is the same
as timer with corrections applied in application pointer.
Instead of using a timer object to wake the process up,
the kernel's scheduler do the job.

After a call to set up the scheduler with the interval
(period) value defined, the scheduler make the application
runs every interval, inside 'deadline', for at most
'runtime' nanoseconds::

	    |--runtime--|
	|-----deadline-------|
	|--------------------period---------------------|

See sched.7 manual.

After wake up and write the sound to device, the
application must yield. i.e. call the kernel to say it
doesn't want cpu anymore.
