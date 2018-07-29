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
 * 15/04/2017, 17/07/2018
 *
 * Transfer sound to/from sound device buffer
 */

#ifdef BUILDING_FOR_GOOGLE_ANDROID
#include <bits/timespec.h>
#endif
//#include <time.h>

#include <errno.h>     /* EPIPE */
#include <string.h>    /* memcpy() */
#include <sys/ioctl.h> /* ioctl() */

/* ALSA header */
#include <sound/asound.h>

#include "sound_global.h"     /* struct snd */
#include "sound_operations.h" /* snd_sync() */

/*
 * MMAP access
 * ===========
 */

/* copy data from/to mmap buffer */
int
snd_mmap_areas_copy(struct snd *snd, unsigned int pcm_offset, char *buf,
                      unsigned int user_offset, unsigned int frames)
{
	int size_bytes        = snd_frames_to_bytes(snd, frames);
	int pcm_offset_bytes  = snd_frames_to_bytes(snd, pcm_offset);
	int user_offset_bytes = snd_frames_to_bytes(snd, user_offset);

	if (snd->type & SND_INPUT)
		memcpy(buf + user_offset_bytes,
		       (char*)snd->mmap_buffer + pcm_offset_bytes, size_bytes);
	else
		memcpy((char*)snd->mmap_buffer + pcm_offset_bytes,
		       buf + user_offset_bytes, size_bytes);

	return 0;
}

/* update application pointer */
int
snd_update_appl_ptr(struct snd *snd, unsigned int frames)
{
	unsigned long appl_ptr = snd->control->appl_ptr;

	appl_ptr += frames;
	/* check for boundary wrap */
	if (appl_ptr > snd->boundary)
		appl_ptr -= snd->boundary;

	snd->control->appl_ptr = appl_ptr;

	if (snd_sync(snd, SND_SYNC_SET) < 0)
		return -1;

	return frames;
}

ssize_t
snd_mmap_transfer(struct snd *snd, void *buffer, unsigned int frames)
{
	/* offset in sound device buffer */
	unsigned int snd_offset;
	/* maximum continuous memory that can be copied */
	unsigned int continuous;
	/* size to be copied */
	unsigned int copy;

	unsigned int user_offset = 0;

	if (frames > snd->buffer_size)
		frames = snd->buffer_size;

	while (frames) {
		copy = frames;

		/* get the transfer offset in ring buffer */
		snd_offset = snd->control->appl_ptr % snd->buffer_size;

		/* we can only copy frames if they are continuous */
		continuous = snd->buffer_size - snd_offset;
		if (copy > continuous)
			copy = continuous;

		snd_mmap_areas_copy(snd, snd_offset, (char*) buffer,
		                    user_offset, copy);

		snd_update_appl_ptr(snd, copy);

		user_offset += copy;
		frames -= copy;
	}

	return frames;
}

/*
 * RW access
 * =========
 */

ssize_t
snd_ioctl_transfer(struct snd *snd, void *buffer, unsigned int frames)
{
	/* interleaved transfer structure */
	struct snd_xferi x;

	int tmp;

	x.buf = buffer;
	x.frames = frames;
	x.result = 0;

	if (snd->type & SND_INPUT)
		tmp = ioctl(snd->fd, SNDRV_PCM_IOCTL_READI_FRAMES, &x);
	else
		tmp = ioctl(snd->fd, SNDRV_PCM_IOCTL_WRITEI_FRAMES, &x);
	if (tmp == -1) {
		/* error handling */
		if (errno == EPIPE) {
			/* xrun */
			// snd->xruns++;
		}
		return -1;
	}

	return x.result;
}

/*
 * generic read/write
 * ==================
 */

int
snd_write(struct snd *snd, const void *data, unsigned int frames)
{
	return snd->transfer(snd, (void*) data, frames);
}

int
snd_read(struct snd *snd, void *data, unsigned int frames)
{
	return snd->transfer(snd, data, frames);
}
