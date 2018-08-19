
# 17/07/2018

# -W  Control display of warnings.
# -I  Add a search path for headers.
# -D  Preprocessor macros. See 'Available preprocessor
#     options' below.
CFLAGS = -Wall \
         -I.. \
         -I./clock_deviation_utility \
         -I./sched_deadline \
         -I./time_helpers \
         #-D DEADLINE_WAKEUP

# Available preprocessor options:
#
# - TIMER_WAKEUP: Application wakeup using timerfd.
#
# - DEADLINE_WAKEUP: Application wakeup using
#   SCHED_DEADLINE.
#
# - Default: Application wakeup using sound device
#   interrupts.

# -L          Add a search path for libraries.
# -Wl,-rpath  Add a search path to runtime linker.
LDFLAGS = -L.. -Wl,-rpath=. -Wl,-rpath=..
LDLIBS = -lsimplesound

# Additional search path for prerequisites.
VPATH = ..:./clock_deviation_utility:./sched_deadline:./time_helpers

all: waveplay sound_device_info

# Play wave (.wav) files

waveplay: mix_utility.o deviation_average.o \
  timer_wakeup.o deadline_wakeup.o waveplay.o

waveplay.o: waveplay.c sound.h mix_utility.h \
  deadline_wakeup.h timer_wakeup.h

# Sound device information

sound_device_info: sound_device_info.o

sound_device_info.o: sound_device_info.c sound_open_device.h \
  sound_parameters.h

# Timer wake up using SCHED_DEADLINE

deadline_wakeup.o: deadline_wakeup.c sched_deadline.h sound.h timer_wakeup.h

# Timer wake up implementation

timer_wakeup.o: timer_wakeup.c timer_wakeup.h sound.h timespec_helpers.h \
  deviation_average.h smooth_correction.h

# Deviation utility

deviation_average.o: deviation_average.c deviation_average.h \
  deviation_history.h

# Mix utility

mix_utility.o: mix_utility.c mix_utility.h
