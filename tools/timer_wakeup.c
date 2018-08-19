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
 * 15/06/2018
 *
 * timer based sound period wakeup
 */

#include <limits.h>       /* ULONG_MAX */
#include <stdint.h>       /* uint64_t */
#include <stdio.h>        /* printf() */
#include <stdlib.h>       /* calloc() labs() */
#include <sys/timerfd.h>  /* timerfd */
#include <unistd.h>       /* close() */
#include <time.h>         /* clock_gettime() */

#include <sys/ioctl.h>  /* ioctl() */

/* ALSA header */
#include <sound/asound.h>

#include "deviation_average.h"
#include "smooth_correction.h"
#include "sound.h"
#include "timespec_helpers.h"

#include "timer_wakeup.h"

/*
 * Frames in buffer waiting to be played by sound device
 * (playback) or waiting to be read by application
 * (capture).
 */
static inline unsigned long
filled(struct snd *snd, unsigned long hw_ptr, unsigned long appl_ptr)
{
	long filled;

	if (snd->type == SND_OUTPUT)
		filled = appl_ptr - hw_ptr;
	else
		filled = hw_ptr - appl_ptr;

	if (filled < 0)
		filled += snd->boundary;

	return filled;
}

/*
 * timer based sound interrupt
 * ===========================
 */

/*
 * get hardware pointer from last interrupt and add to it
 * an estimate of how many frames have elapsed since that
 * based on the time elapsed.
 *
 * In order to keep the prediction synchronized,
 * interrupts must be enabled and HWSYNC operation should
 * never be done.
 *
 * NOTE: If using mmaped status, hardware may interrupt
 * while status->hw_ptr and status->tstamp are being read.
 * Therefore, status is requested using ioctl() hoping
 * hw_ptr and tstamp will be get atomically.
 */
static unsigned long
predict_hardware_pointer(struct snd *snd, struct snd_timer *snd_timer)
{
	struct snd_pcm_sync_ptr sync_ptr;
	struct timespec now;
	struct timespec diff;
	int64_t diff_ns;
	unsigned long estimate;

	/* get information from kernel */
	sync_ptr.flags = SNDRV_PCM_SYNC_PTR_APPL |
	                 SNDRV_PCM_SYNC_PTR_AVAIL_MIN;
	/* TODO: do error check */
	ioctl(snd->fd, SNDRV_PCM_IOCTL_SYNC_PTR, &sync_ptr);

	/* get now */
	clock_gettime(CLOCK_MONOTONIC, &now);
	/* TODO: support CLOCK_REALTIME? */

	/* time difference between now and last interrupt */
	diff = timespec_sub(&now, &sync_ptr.s.status.tstamp);
	diff_ns = timespec_to_ns(&diff);

	/* estimate of frames since last interrupt */
	estimate = diff_ns / snd_timer->frame_ns;

	return sync_ptr.s.status.hw_ptr + estimate;
}

/*
 * this must be called after every timer wake up
 *
 * The deviation is accounted and, if necessary,
 * a sync is issued.
 */
int
snd_timer_write(struct snd_timer *t, struct snd *snd, void *buffer)
{
	long diff;

	t->n_wakeups++;

	/*
	 * 'diff' is going to be set three times,
	 * each one corresponding to a different
	 * 'difference'
	 */

	/*
	 * difference between the expected and the
	 * actual number of frames waiting to be
	 * played (playback) or to be read (capture)
	 */
	diff = t->expected -
	       filled(snd, snd->status->hw_ptr, snd->control->appl_ptr);

	/* TODO: remove debug */
	printf("diff: %ld\n", diff);

	/*
	 * deviation_average keeps a history of the
	 * last N deviations (the 'diff' above)
	 * and returns a value only when more than
	 * half of the history is out of the
	 * expected deviation E.
	 *
	 * N (history_size) and E (allowed_deviation) are
	 * defined in deviation_average_reset().
	 *
	 * WARNING: we're accounting deviations even
	 * if a synchronization is already in progress.
	 * However another synchronization cannot take
	 * place.
	 */
	diff = deviation_average_calculate(&t->avg, diff);
	if (diff && !t->smooth.count) {
		/* do sync */

		deviation_average_reset(&t->avg, t->deviation_history,
		                        t->history_size, t->allowed_deviation);

		/* TODO: remove debug */
		printf("sync: %ld\n", diff);
		printf("wakeups: %ld\n", t->n_wakeups);

		/*
		 * n_wakeups is the number of wakeups
		 * since the last sync
		 */

		/*
		 * Instead of dropping or adding diff
		 * frames at once, do it smoothly.
		 *
		 * smooth correction distributes
		 * deviation among n_wakeups
		 */
		smooth_correction_start(&t->smooth, diff, t->n_wakeups);

		t->n_wakeups = 0;
	}

	/*
	 * smooth_correction_get() takes care if there is
	 * a synchronization happening
	 *
	 * It will make it sounds smoother :-)
	 */
	diff = smooth_correction_get(&t->smooth);

	/*
	 * if diff is greater than zero, we should
	 * copy the last frame 'diff' times in
	 * order to not have an audible pop
	 */

	/* we transfer period_size + diff */
	return snd_write(snd, buffer, t->period_size + diff);
}

int
snd_timer_start(struct snd_timer *snd_timer, struct snd *snd)
{
	/* timerspec */
	struct itimerspec t;

	/* start sound device and get start timestamp */
	if (snd_start(snd) == -1)
		return -1;
	snd_trigger_tstamp(snd, &t.it_value);

	/* start timer */
	timespec_add_ns(&t.it_value, snd_timer->period_ns / 2);
	t.it_interval = timespec_from_ns(snd_timer->period_ns);
	if (timerfd_settime(snd_timer->fd, TFD_TIMER_ABSTIME, &t, NULL) == -1)
		goto _go_snd_stop;

	/*
	 * say that one period has already been written
	 *
	 * As the first period is skipped, increment
	 * application pointer by 'period_size'.
	 */
	snd->control->appl_ptr += snd_timer->period_size;
	snd_sync(snd, SND_SYNC_SET);

	return 0;

_go_snd_stop:
	snd_stop(snd);
	return -1;
}

static int
setup_sound(struct snd *snd, struct snd_config *cfg)
{
	int fd;
	struct snd_parameters p;

	/* get allowed parameters */
	fd = snd_device_open(0, 0, cfg->flags & SND_INPUT | SND_NONBLOCK);
	if (fd == -1)
		return -1;
	snd_params_init(fd, &p);
	close(fd);

	cfg->flags = cfg->flags | SND_NOIRQ | SND_MONOTONIC;

	/*
	 * Period size is choosen in order to minimize
	 * the number of interrupts. Thus, the maximum
	 * possible value.
	 */
	cfg->period_size = snd_params_get_max(&p, SND_PERIOD_SIZE);
	cfg->period_count = snd_params_get_max(&p, SND_BUFFER_SIZE) /
	  cfg->period_size;

	/* software parameters */
	cfg->avail_min = ULONG_MAX;
	cfg->start_threshold = 0;
	cfg->stop_threshold = 0;
	cfg->silence_threshold = 0;

	if (snd_open(snd, cfg) == -1)
		return -1;

	return 0;
}

void
snd_timer_close(struct snd_timer *snd_timer, struct snd *snd)
{
	free(snd_timer->deviation_history);
	close(snd_timer->fd);
	snd_close(snd);
}

int
snd_timer_open(struct snd_timer *snd_timer, struct snd *snd,
               struct snd_config *cfg, int period_size)
{
	int periods_per_sec;

	if (setup_sound(snd, cfg) == -1)
		return -1;

	snd_timer->fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	if (snd_timer->fd == -1)
		goto _go_sound_close;

	/* calculate nanoseconds of a frame and a period */
	snd_timer->frame_ns = NSEC_PER_SEC / cfg->rate;
	snd_timer->period_ns = snd_timer->frame_ns * period_size;

	periods_per_sec = cfg->rate / period_size;

	/* TODO: it's a debug */
	printf("sound timer: periods per second: %d\n", periods_per_sec);

	snd_timer->history_size = periods_per_sec + 1;
	snd_timer->allowed_deviation = 16;
	snd_timer->period_size = period_size;
	snd_timer->expected = period_size / 2;
	/* periods since the last sync */
	snd_timer->n_wakeups = 0;

	snd_timer->deviation_history =
	  calloc(1, snd_timer->history_size * sizeof(long));

	deviation_average_reset(&snd_timer->avg, snd_timer->deviation_history,
	                        snd_timer->history_size,
	                        snd_timer->allowed_deviation);

	smooth_correction_reset(&snd_timer->smooth);

	return 0;

_go_sound_close:
	snd_close(snd);
	return -1;
}

#if 0

#include <poll.h>         /* poll() */
#include <signal.h>       /* signal() */
#include <unistd.h>       /* read() */

static volatile sig_atomic_t _keep_running = 1;

static void
on_signal(int n)
{
	_keep_running = 0;
}

static int
run(struct snd_timer *snd_timer, struct snd *snd, void *buffer)
{
	struct pollfd pfd;

	snd_timer_start(snd_timer, snd);

	while (_keep_running) {
		uint64_t ticks;

		pfd = (struct pollfd) {snd_timer->fd, POLLIN, 0};
		if (poll(&pfd, 1, -1) == -1)
			break;
		if (read(snd_timer->fd, &ticks, sizeof(ticks)) == -1)
			break;
		if (ticks != 1)
			break;

		snd_sync(snd, SND_SYNC_HW | SND_SYNC_GET);

		//printf("hw: %ld, appl: %ld\n",
		//       snd->status->hw_ptr, snd->control->appl_ptr);

		/* TODO: check for underruns. */

		snd_timer_write(snd_timer, snd, buffer);
	}

	snd_stop(snd);

	return 0;
}

int
main(void)
{
	struct snd_timer snd_timer;
	struct snd_config cfg;
	struct snd snd;
	int period_size;
	void *buffer;

	signal(SIGINT, on_signal);

	/*
	 * In timer based sound scheduling, some hardware
	 * and software parameters are set internally:
	 *
	 * Flags
	 * - interrupt disablement
	 * - flag for CLOCK_MONOTONIC timestamps
	 * Hardware parameters
	 * - period size
	 * - period count
	 * Software parameters
	 * - avail_min
	 * - (start/stop/silence)_threshold
	 */

	cfg.card = 0;
	cfg.device = 0;
	cfg.flags = SND_OUTPUT;

	/* hardware parameters */
	cfg.format = SND_FORMAT_S16_LE;
	cfg.channels = 2;
	cfg.rate = 44100;

	/* period size for timer sound scheduling */
	period_size = 441;

	if (snd_timer_open(&snd_timer, &snd, &cfg, period_size) == -1)
		return 1;

	buffer = calloc(1, 2 * period_size * snd.bytes_per_frame);

	run(&snd_timer, &snd, buffer);

	free(buffer);
	snd_timer_close(&snd_timer, &snd);

	return 0;
}

#endif
