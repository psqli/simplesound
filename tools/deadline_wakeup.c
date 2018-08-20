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
 * 12/06/2018
 *
 * Sound scheduling using the deadline scheduler
 *
 * See this presentation by Alessio Balsini:
 * http://retis.sssup.it/ospm-summit/Downloads/OSPM_deadline_audio.pdf
 */

#include <stdio.h> /* printf(), perror() */

#include "sched_deadline.h"
#include "sound.h"
#include "timer_wakeup.h"

int
snd_deadline_start(struct snd_timer *snd_timer, struct snd *snd)
{
	struct sched_attr attr;
	int runtime_ms, deadline_ms;

	int tmp;

	/*
	 * define scheduler parameters
	 *
	 * TODO: allow user set it based on worst case
	 * execution time.
	 */
	runtime_ms = 2;
	deadline_ms = 2;

	/* set up sched_attr structure */
	attr.size = sizeof(attr);
	attr.sched_policy = SCHED_DEADLINE;
	attr.sched_flags = 0;
	attr.sched_nice = 0;
	attr.sched_priority = 0;
	attr.sched_runtime =  runtime_ms * 1000000;
	attr.sched_deadline = deadline_ms * 1000000;
	attr.sched_period =   snd_timer->period_ns;

	/* start sound device */
	snd_start(snd);

	/* set sched deadline */
	tmp = sched_setattr(0, &attr, 0);
	if (tmp < 0) {
		perror("sched_setattr");
		return -1;
	}

	/*
	 * NOTE: At the moment, we don't know if there
	 * is any guarantee about the time between the
	 * call to sched_setattr() and the first run
	 */

	/*
	 * Synchronize here because user will yield when
	 * entering in the loop.
	 *
	 * NOTE: Do we need to yield before synchronize?
	 * This question arises because we don't know if
	 * we're in the beginning of a period after the
	 * sched_setattr() call.
	 */

	snd_sync(snd, SND_SYNC_GET | SND_SYNC_HW);
	snd->control->appl_ptr = snd->status->hw_ptr +
	  snd_timer->period_size + snd_timer->period_size / 2;
	snd_sync(snd, SND_SYNC_SET);

	return 0;
}

#if 0

#include <time.h>    /* clock_gettime() */
#include <signal.h>  /* signal() */
#include <string.h>  /* memset() */

int
main(void)
{
	/* sound stuff */
	struct snd_timer snd_timer;
	struct snd snd;
	struct snd_config config;
	int period_size = 4410;
	/* buffer */
	char *buffer;

	signal(SIGINT, on_signal);

	printf("Ctrl C to stop\n");
	printf("thread id: %ld\n", gettid());

	memset(&config, 0, sizeof(config));

	config.card = 0;
	config.device = 0;
	config.flags = SND_OUTPUT /* | SND_MMAP */;
	config.format = SND_FORMAT_S16_LE;
	config.channels = 2;
	config.rate = 44100;

	if (snd_timer_open(&snd_timer, &snd, &config, period_size) == -1) {
		printf("Unable to open sound device\n");
		return 1;
	}

	buffer = calloc(1, period_size * snd.bytes_per_frame * 2);

	snd_deadline_start(&snd_timer, &snd);

	/* here we assume we're synchronized */

	while (_keep_running) {

		/* sleep until the next run */
		sched_yield();

		snd_sync(&snd, SND_SYNC_GET | SND_SYNC_HW);

		snd_timer_write(&snd_timer, &snd, buffer);
	}

	free(buffer);
	snd_timer_close(&snd_timer, &snd);

	return 0;
}
#endif
