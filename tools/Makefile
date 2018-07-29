
# 17/07/2018

# -W  Control display of warnings.
# -I  Add a search path for headers.
CFLAGS = -Wall -I..

# -L          Add a search path for libraries.
# -Wl,-rpath  Add a search path to runtime linker.
LDFLAGS = -L.. -Wl,-rpath=. -Wl,-rpath=..
LDLIBS = -lsimplesound

# Additional search path for prerequisites.
VPATH = ..

all: sound_device_info

# Sound device information

sound_device_info: sound_device_info.o

sound_device_info.o: sound_device_info.c sound_open_device.h \
  sound_parameters.h

# Mix utility

mix_utility.o: mix_utility.c mix_utility.h
