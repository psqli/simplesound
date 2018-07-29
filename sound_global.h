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

#ifndef SOUND_GLOBAL_H
#define SOUND_GLOBAL_H

/* ALSA header */
#include <sound/asound.h>

#include "sound_open_device.h"

/*
 * configuration flags
 * ===================
 *
 * The first four bits are reserved to open flags
 */

/* request MMAP access to sound buffer */
#define SND_MMAP       0x00000010
/* disable device interrupts */
#define SND_NOIRQ      0x00000020
/* use CLOCK_MONOTONIC for timestamps */
#define SND_MONOTONIC  0x00000040

/*
 * snd states
 * ==========
 */

#define SND_STATE_OPEN          SNDRV_PCM_STATE_OPEN
#define SND_STATE_SETUP         SNDRV_PCM_STATE_SETUP
#define SND_STATE_PREPARED      SNDRV_PCM_STATE_PREPARED
#define SND_STATE_RUNNING       SNDRV_PCM_STATE_RUNNING
#define SND_STATE_PAUSED        SNDRV_PCM_STATE_PAUSED
#define SND_STATE_XRUN          SNDRV_PCM_STATE_XRUN /* (over/under)run */
#define SND_STATE_DRAINING      SNDRV_PCM_STATE_DRAINING
#define SND_STATE_SUSPENDED     SNDRV_PCM_STATE_SUSPENDED
#define SND_STATE_DISCONNECTED  SNDRV_PCM_STATE_DISCONNECTED

/*
 * snd configuration structure for snd_open()
 * ==========================================
 */

struct snd_config {

	/*
	 * General flags
	 * =============
	 *
	 * Capture or playback? NONBLOCK? mmap? Interrupts?
	 */

	unsigned int flags;

	/*
	 * Opening configuration
	 * =====================
	 */

	unsigned int card;
	unsigned int device;

	/*
	 * Hardware parameters
	 * ===================
	 */

	unsigned int format;
	unsigned int channels;
	unsigned int rate;
	unsigned int period_size;
	unsigned int period_count;

	/*
	 * Software parameters
	 * ===================
	 */

	/* minimum available for application wake up */
	unsigned long avail_min;

	/*
	 * number of frames that, in playback, application
	 * needs to write, in capture, application must
	 * try to read, in order to start sound device.
	 */
	unsigned long start_threshold;

	/*
	 * stop_threshold: number of available frames to
	 * sound device enter in xrun state.
	 *
	 * NOTE: As xruns are not being handled, it's set
	 * to boundary. However, xruns can still occur when
	 * device returns -1 as its buffer position.
	 */
	unsigned long stop_threshold;

	/*
	 * Playback only. Frames to fill with silence
	 * ahead of application pointer.
	 */
	unsigned long silence_threshold;
};

/*
 * The sound structure
 * ===================
 *
 * This is the main structure of this sound library. It
 * contains the file descriptor, parameters, and
 * memory mapping info.
 */

struct snd {
	/* file descriptor for the PCM */
	int fd;

	/* OUTPUT or INPUT */
	unsigned int type;

	unsigned int  bytes_per_frame;
	unsigned int  buffer_size; /* frames */
	unsigned long boundary;    /* frames */

	ssize_t (*transfer) (struct snd*, void*, unsigned int);

	/*
	 * Synchronization structures
	 * ==========================
	 */

	/*
	 * When mmap of status or control areas fails,
	 * sync_ptr points to the memory allocated to
	 * synchronize pointers via ioctl. If mmap was
	 * successful, sync_ptr is set to NULL.
	 *
	 * status and control point to an offset of mmaped
	 * or allocated memory.
	 */
	struct snd_pcm_mmap_status *status;
	struct snd_pcm_mmap_control *control;
	struct snd_pcm_sync_ptr *sync_ptr;

	/*
	 * MMAP data buffer
	 * ================
	 *
	 * Only when MMAP transfer
	 */

	/* sound device buffer to transfer audio */
	void *mmap_buffer;
};

static inline int
snd_is_running(struct snd *pcm)
{
	return (pcm->status->state == SND_STATE_RUNNING ||
		(pcm->status->state == SND_STATE_DRAINING &&
		 pcm->type == SND_OUTPUT));
}

#if 0
static inline int
snd_is_empty(struct snd *pcm)
{
	if (pcm->type & SND_INPUT)
		return pcm->avail == 0;
	else
		return pcm->avail >= pcm->buffer_size;
}
#endif

static inline unsigned int
sound_bytes_to_frames(const struct snd *pcm, unsigned int bytes)
{
	return bytes / pcm->bytes_per_frame;
}

static inline unsigned int
sound_frames_to_bytes(const struct snd *pcm, unsigned int frames)
{
	return frames * pcm->bytes_per_frame;
}

#endif /* SOUND_GLOBAL_H */
