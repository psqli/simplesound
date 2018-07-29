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
 * 19/07/2017
 *
 * Synchronize, start, stop.
 */

#ifdef BUILDING_FOR_GOOGLE_ANDROID
#include <bits/timespec.h>
#endif

/* ALSA header */
#include <sound/asound.h>

#include <sys/ioctl.h>  /* ioctl() */
#include <time.h>       /* struct timespec */

#include "sound_global.h"
#include "sound_operations.h" /* SND_SYNC_* */

/*
 * synchronize hw_ptr, appl_ptr and avail_min
 *
 * If status and control are mmaped, ioctl() is called
 * only when HWSYNC operation is requested.
 *
 * Flags (defined in sound_operations.h):
 *
 * SND_SYNC_GET: SNDRV_PCM_SYNC_PTR_APPL | SNDRV_PCM_SYNC_PTR_AVAIL_MIN
 * SND_SYNC_SET: 0
 * SND_SYNC_HW:  SNDRV_PCM_SYNC_PTR_HWSYNC
 */
int
snd_sync(struct snd *snd, int flags)
{
	if (snd->sync_ptr == NULL) {
		/* status and control are mmaped */

		if (flags & SND_SYNC_HW) {
			if (ioctl(snd->fd, SNDRV_PCM_IOCTL_HWSYNC) < 0)
				return -1;
		}
	} else {
		snd->sync_ptr->flags = flags;
		if (ioctl(snd->fd, SNDRV_PCM_IOCTL_SYNC_PTR, snd->sync_ptr) < 0)
			return -1;
	}

	return 0;
}

/*
 * Start and drop actions
 * ======================
 */

int
snd_start(struct snd *pcm)
{
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_PREPARE) == -1 ||
	    ioctl(pcm->fd, SNDRV_PCM_IOCTL_START) == -1)
		return -1;

	return 0;
}

/* DROP is internally the stop action */
int
snd_stop(struct snd *pcm)
{
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_DROP) == -1)
		return -1;

	return 0;
}

/*
 * About SNDRV_PCM_IOCTL_STATUS ioctl
 * ==================================
 *
 * It's not interesting to store snd_pcm_status structure
 * after STATUS ioctl because much of the information
 * returned is the same as the SYNC_PTR or HWSYNC
 * ioctl.
 *
 * It's being used only to get trigger timestamp.
 *
 * Trigger timestamp is set in START, STOP/DROP, PAUSE,
 * SUSPEND and RESUME actions.
 */
int
snd_trigger_tstamp(struct snd *pcm, struct timespec *tstamp)
{
	struct snd_pcm_status status;

	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_STATUS, &status) < 0)
		return -1;

	*tstamp = status.trigger_tstamp;

	return 0;
}
