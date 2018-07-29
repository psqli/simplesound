/*
 * simple Linux sound library
 * Copyright (C) 2017, 2018  Ricardo Biehl Pasquali
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * 09/06/2018
 *
 * open sound device file descriptor
 */

#include <stdio.h>     /* snprintf() */
#include <sys/types.h> /* open() */
#include <sys/stat.h>  /* open() */
#include <fcntl.h>     /* open() */

#include "sound_open_device.h"

#define SOUND_DEV_PATH  "/dev/snd/"

/* open sound character device and return file descriptor */
int
snd_device_open(unsigned int card, unsigned int device, int flags)
{
	char path[256];

	snprintf(path, sizeof(path),
		 SOUND_DEV_PATH "pcmC%uD%u%c",
	         card, device, flags & SND_INPUT ? 'c' : 'p');

	return open(path, O_RDWR | (flags & SND_NONBLOCK ? O_NONBLOCK : 0));
}
