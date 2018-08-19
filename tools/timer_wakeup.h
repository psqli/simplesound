/*
 * simple Linux sound library
 * Copyright (C) 2018  Ricardo Biehl Pasquali
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
 * 27/06/2018
 *
 * Timer scheduling header
 *
 * Read Documentation/timer_wakeup.rst
 */

#ifndef TIMER_WAKEUP_H
#define TIMER_WAKEUP_H

#include "deviation_average.h"
#include "smooth_correction.h"

struct snd_timer {

	/* timer fd */
	int fd;

	/* period size for timer interrupt */
	int period_size;

	/* frame and period nanoseconds */
	uint64_t frame_ns;
	uint64_t period_ns;

	/* expected filled after every wake up */
	unsigned long expected;

	/* wakeups since last sync */
	unsigned long n_wakeups;

	/*
	 * deviation_average extracts the average from
	 * deviation_history, which stores a history of
	 * deviations from 'expected' value
	 */
	struct deviation_average avg;
	long *deviation_history;
	unsigned int history_size;
	unsigned int allowed_deviation;

	/*
	 * Smooth correction utility
	 *
	 * Only used in timer scheduling with corrections
	 * applied in application pointer.
	 */
	struct smooth_correction smooth;
};

int
snd_timer_write(struct snd_timer *t, struct snd *snd, void *buffer);

int
snd_timer_start(struct snd_timer *snd_timer, struct snd *snd);

void
snd_timer_close(struct snd_timer *snd_timer, struct snd *snd);

int
snd_timer_open(struct snd_timer *snd_timer, struct snd *snd,
               struct snd_config *cfg, int period_size);

#endif /* TIMER_WAKEUP_H */
