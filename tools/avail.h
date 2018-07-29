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
 * 16/07/2018
 *
 * Available calculation
 */

#include "sound_global.h"

/*
 * remember hw/appl pointers are actually counters that
 * indicates how many frames were processed by hardware
 * and application. They round up to the size of boundary.
 *
 * A playback example:
 *
 *
 * --hw----------------[  buffer size  ]
 *                            |  avail |
 * --appl---------------------
 * --boundary-----------------------------------------
 * [  buffer size  ][  buffer size  ][  buffer size  ]
 */

/* capture_avail: frames to be read */
static inline unsigned long
capture_avail(struct snd *snd)
{
	long avail;

	avail = snd->status->hw_ptr - snd->control->appl_ptr;

	if (avail < 0)
		/*
		 * True when:
		 * - hw_ptr has been resetted after crossed
		 *   the boundary and appl_ptr hasn't yet.
		 * - appl_ptr was over incremented -- over
		 *   incrementing appl_ptr is an error!
		 */
		avail += snd->boundary;

	return avail;
}

/* playback_avail: frames to be written */
static inline unsigned long
playback_avail(struct snd *snd)
{
	long avail;

	avail = snd->status->hw_ptr + snd->buffer_size - snd->control->appl_ptr;

	if (avail < 0)
		/*
		 * True when:
		 * - hw_ptr has been resetted after crossed
		 *   the boundary and appl_ptr hasn't yet.
		 * - appl_ptr was over incremented -- over
		 *   incrementing appl_ptr is an error!
		 */
		avail += snd->boundary;
	else if (avail >= (unsigned long) snd->boundary)
		/*
		 * True when appl_ptr has been resetted
		 * after crossed the boundary and hw_ptr
		 * hasn't yet.
		 */
		avail -= snd->boundary;

	return avail;
}

static inline unsigned long
snd_avail(struct snd *snd)
{
	if (snd->type & SND_INPUT)
		return capture_avail(snd);
	else
		return playback_avail(snd);
}
