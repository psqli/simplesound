
# 12/06/2018

# Generate Position Independent Code
CFLAGS = -Wall -fPIC

# Link as a shared library
LDFLAGS = -shared

objects = \
  hardware_parameters.o \
  sound_parameters.o \
  sound_open_device.o \
  sound_setup.o \
  sound_transfer.o \
  sound_operations.o

all: library

# Sound library

library: $(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -o libsimplesound.so $(objects)

hardware_parameters.o: hardware_parameters.c

sound_parameters.o: sound_parameters.c hardware_parameters.h \
  sound_parameters.h

sound_open_device.o: sound_open_device.c sound_open_device.h

sound_setup.o: sound_setup.c sound_global.h hardware_parameters.h \
  sound_open_device.h sound_parameters.h sound_transfer.h

sound_transfer.o: sound_transfer.c sound_global.h sound_operations.h

sound_operations.o: sound_operations.c sound_global.h sound_operations.h

# Clean

.PHONY: clean
clean:
	-rm $(objects)
