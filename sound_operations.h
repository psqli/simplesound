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

#ifndef SOUND_OPERATIONS_H
#define SOUND_OPERATIONS_H

#include "sound_global.h"

/*
 * flags for snd_sync()
 * ====================
 *
 * If SND_SYNC_GET is not specified, ALSA's internal
 * appl_ptr and avail_min will be set to the passed
 * values.
 *
 * appl_ptr is changed by ALSA basically only in prepare
 * operation and ACCESS_RW (ioctl) transfers.
 *
 * avail_min is not changed by ALSA internally.
 */

/* get/set appl_ptr and avail_min */
#define SND_SYNC_GET  SNDRV_PCM_SYNC_PTR_APPL | SNDRV_PCM_SYNC_PTR_AVAIL_MIN
#define SND_SYNC_SET  0
/* request hardware pointer update */
#define SND_SYNC_HW  SNDRV_PCM_SYNC_PTR_HWSYNC

int
snd_sync(struct snd *pcm, int flags);

int
snd_start(struct snd *pcm);

int
snd_stop(struct snd *pcm);

int
snd_trigger_tstamp(struct snd *pcm, struct timespec *tstamp);

#endif /* SOUND_OPERATIONS_H */
