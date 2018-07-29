==============================
Simple Linux sound I/O library
==============================

:Date: 2017-08-27


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
