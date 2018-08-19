
# 17/07/2018

# -W  Control display of warnings.
# -I  Add a search path for headers.
CFLAGS = -Wall \
         -I.. \
         -I./clock_deviation_utility \
         -I./sched_deadline \
         -I./time_helpers

# -L          Add a search path for libraries.
# -Wl,-rpath  Add a search path to runtime linker.
LDFLAGS = -L.. -Wl,-rpath=. -Wl,-rpath=..
LDLIBS = -lsimplesound

# Additional search path for prerequisites.
VPATH = ..:./clock_deviation_utility:./sched_deadline:./time_helpers

all: sound_device_info

# Sound device information

sound_device_info: sound_device_info.o

sound_device_info.o: sound_device_info.c sound_open_device.h \
  sound_parameters.h

# Timer wake up using SCHED_DEADLINE

deadline_wakeup.o: deadline_wakeup.c sched_deadline.h sound.h timer_wakeup.h

# Timer wake up implementation

timer_wakeup.o: timer_wakeup.c timer_wakeup.h sound.h timespec_helpers.h \
  deviation_average.h smooth_correction.h

# Mix utility

mix_utility.o: mix_utility.c mix_utility.h
