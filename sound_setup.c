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
 * 15/04/2017
 *
 * open/close device, set parameters, mmap/munmap areas
 */

#ifdef BUILDING_FOR_GOOGLE_ANDROID
#include <bits/timespec.h>
#endif

#include <assert.h>    /* assert() */
#include <limits.h>    /* ULONG_MAX */
#include <stdio.h>     /* snprintf() */
#include <string.h>    /* memset() */
#include <sys/ioctl.h> /* ioctl() */
#include <sys/mman.h>  /* mmap() */
#include <unistd.h>    /* close() */

/* ALSA header */
#include <sound/asound.h>

#include "sound_global.h"
#include "hardware_parameters.h" /* hw_param_*() */
#include "sound_open_device.h"   /* sound_device_open() */
#include "sound_parameters.h"    /* sound_frames_to_bytes(), SND_* */
#include "sound_transfer.h"      /* snd_*_transfer() */

#define PAGE_SIZE  sysconf(_SC_PAGE_SIZE)
#define page_align(size) \
	( size + (size % PAGE_SIZE ? PAGE_SIZE - size % PAGE_SIZE : 0) )

static int
set_hardware_parameters(struct snd *pcm, struct snd_config *config)
{
	struct snd_pcm_hw_params hw_params;

	hw_param_fill(&hw_params);

	/* set no_interrupts option if user has requested it */
	if (config->flags & SND_NOIRQ)
		hw_params.flags |= SND_NO_INTERRUPTS;

	/* set the access type (MMAP interleaved or RW interleaved) */
	if (config->flags & SND_MMAP)
		hw_param_set(&hw_params, SND_ACCESS, SND_ACCESS_MMAP);
	else
		hw_param_set(&hw_params, SND_ACCESS, SND_ACCESS_RW);

	/*
	 * we don't need to set subformat because there
	 * is only one option (STANDARD), and it's equal
	 * zero.
	 */

	hw_param_set(&hw_params, SND_FORMAT,      config->format);
	hw_param_set(&hw_params, SND_CHANNELS,    config->channels);
	hw_param_set(&hw_params, SND_RATE,        config->rate);
	hw_param_set(&hw_params, SND_PERIOD_SIZE, config->period_size);

	if (config->period_count == 0)
		hw_param_set(&hw_params, SND_PERIODS, 2);
	else
		hw_param_set(&hw_params, SND_PERIODS, config->period_count);

#if 0
	/*
	 * This must be set because ALSA allows buffer
	 * sizes that are not a multiple of period size
	 */
	hw_param_set(&hw_params, SND_BUFFER_SIZE,
	             config->period_size * config->period_count);
#endif

	/* send hw_params to ALSA in kernel */
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_PARAMS, &hw_params) == -1)
		return -1;

	/* NOTE: we assume parameters have not changed */

	pcm->bytes_per_frame =
	  config->channels * snd_format_to_bytes(config->format);
	pcm->buffer_size = config->period_count * config->period_size;

	return 0;
}

static int
set_software_parameters(struct snd *pcm, struct snd_config *config)
{
	struct snd_pcm_sw_params sw_params;

	memset(&sw_params, 0, sizeof(sw_params));

	sw_params.tstamp_mode = SNDRV_PCM_TSTAMP_ENABLE;
	if (config->flags & SND_MONOTONIC) {
		sw_params.tstamp_type = SNDRV_PCM_TSTAMP_TYPE_MONOTONIC;
#ifdef SNDRV_PCM_IOCTL_TTSTAMP
		if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_TTSTAMP,
		    &sw_params.tstamp_type) == -1)
			return -1;
#endif
	}

	/* when (avail >= avail_min) application is allowed to wake up */
	if (!config->avail_min)
		config->avail_min = config->period_size;

	if (!config->start_threshold)
		config->start_threshold = 1;

	if (!config->stop_threshold)
		/*
		 * we won't have xruns! except if device
		 * returns -1 in internal pointer callback
		 */
		config->stop_threshold = ULONG_MAX;

	sw_params.period_step = 1;
	sw_params.avail_min = config->avail_min;
	sw_params.start_threshold = config->start_threshold;
	sw_params.stop_threshold = config->stop_threshold;
	sw_params.silence_threshold = config->silence_threshold;
	sw_params.silence_size = 0;

	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_SW_PARAMS, &sw_params))
		return -1;

	/* get boundary for hardware and application pointers */
	pcm->boundary = sw_params.boundary;

	return 0;
}

static void
control_and_status_munmap(struct snd *pcm)
{
	munmap(pcm->control, page_align(sizeof(*pcm->control)));
	munmap(pcm->status, page_align(sizeof(*pcm->status)));
}

static int
control_and_status_mmap(struct snd *pcm)
{
	/* mmap status */
	pcm->status = mmap(NULL, page_align(sizeof(*pcm->status)),
	                   PROT_READ,  MAP_FILE | MAP_SHARED,
	                   pcm->fd,  SNDRV_PCM_MMAP_OFFSET_STATUS);
	if (pcm->status == MAP_FAILED)
		return -1;

	/* mmap control */
	pcm->control = mmap(NULL, page_align(sizeof(*pcm->control)),
	                    PROT_READ | PROT_WRITE,  MAP_FILE | MAP_SHARED,
	                    pcm->fd,  SNDRV_PCM_MMAP_OFFSET_CONTROL);
	if (pcm->control == MAP_FAILED)
		goto _go_unmap_status;

	pcm->sync_ptr = NULL;

	return 0;

_go_unmap_status:
	munmap(pcm->status, page_align(sizeof(*pcm->status)));

	return -1;
}

static void
control_and_status_free(struct snd *pcm)
{
	free(pcm->sync_ptr);
}

static int
control_and_status_allocate(struct snd *pcm)
{
	pcm->sync_ptr = calloc(1, sizeof(*pcm->sync_ptr));
	if (!pcm->sync_ptr)
		return -1;

	pcm->status = &pcm->sync_ptr->s.status;
	pcm->control = &pcm->sync_ptr->c.control;

	return 0;
}

/* unmap or free, status and control areas */
static void
cleanup_control_and_status(struct snd *pcm)
{
	if (pcm->sync_ptr)
		control_and_status_free(pcm);
	else
		control_and_status_munmap(pcm);
}

/*
 * map or allocate, status and control areas
 *
 * If the mmap of status or control areas fails, we
 * allocate memory with calloc() and set pcm->status
 * and pcm->control pointers to the allocated memory.
 */
static int
setup_control_and_status(struct snd *pcm)
{
	if (control_and_status_mmap(pcm) < 0 &&
	    control_and_status_allocate(pcm) < 0)
		return -1;

	return 0;
}

static void
cleanup_mmap_buffer(struct snd *pcm)
{
	unsigned int size = snd_frames_to_bytes(pcm, pcm->buffer_size);

	munmap(pcm->mmap_buffer, page_align(size));
}

static int
setup_mmap_buffer(struct snd *pcm)
{
	unsigned int size = snd_frames_to_bytes(pcm, pcm->buffer_size);

	pcm->mmap_buffer = mmap(NULL, page_align(size),
	                        PROT_READ | PROT_WRITE,  MAP_FILE | MAP_SHARED,
	                        pcm->fd,  SNDRV_PCM_MMAP_OFFSET_DATA);

	if (pcm->mmap_buffer == MAP_FAILED)
		return -1;

	return 0;
}

void
snd_close(struct snd *pcm)
{
	cleanup_control_and_status(pcm);

	if (pcm->transfer == snd_mmap_transfer) {
		ioctl(pcm->fd, SNDRV_PCM_IOCTL_DROP);
		cleanup_mmap_buffer(pcm);
	}

	close(pcm->fd);
}

int
snd_open(struct snd *pcm, struct snd_config *config)
{
	/* clear pcm structure */
	memset(pcm, 0, sizeof(*pcm));

	pcm->fd = snd_device_open(config->card, config->device,
	                            config->flags);
	if (pcm->fd == -1)
		return -1;

	pcm->type = config->flags & SND_INPUT;

	if (set_hardware_parameters(pcm, config) == -1)
		goto _go_close_device;
	if (set_software_parameters(pcm, config) == -1)
		goto _go_close_device;

	/* if mmap flag is set, mmap the buffer where we'll transfer audio */
	if (config->flags & SND_MMAP) {
		if (setup_mmap_buffer(pcm) == -1)
			goto _go_close_device;
		pcm->transfer = snd_mmap_transfer;
	} else {
		pcm->transfer = snd_ioctl_transfer;
	}

	/* mmap or allocate status and control areas */
	if (setup_control_and_status(pcm) < 0)
		goto _go_unmap_buffer;

	/* reset control variables */
	pcm->control->appl_ptr = 0;
	pcm->control->avail_min = config->avail_min;

	/*
	 * NOTE: xruns are not being handled! User must
	 * set stop_threshold to 0, then it's set to
	 * ULONG_MAX in set_software_parameters()
	 */

	return 0;

_go_unmap_buffer:
	if (config->flags & SND_MMAP)
		cleanup_mmap_buffer(pcm);
_go_close_device:
	close(pcm->fd);
	return -1;
}
