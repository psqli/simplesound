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
 * 15/07/2017, 11/06/2018
 *
 * play multiple wave (audio) files
 *
 * Based on tinyplay from tinyalsa library
 */

#include <errno.h>
#include <poll.h>   /* poll() */
#include <stdio.h>  /* printf() */
#include <stdlib.h> /* labs() */
#include <stdint.h> /* int*_t */
#include <string.h> /* memset(), strerror() */
#include <signal.h>
#include <unistd.h> /* getopt() */

#if defined(DEADLINE_WAKEUP)
#include <sched.h>  /* sched_yield() */
#endif

#include "sound.h"       /* snd_*() */
#include "mix_utility.h" /* sndmix_*() */

#if defined(DEADLINE_WAKEUP) || defined(TIMER_WAKEUP)
#include "deadline_wakeup.h"
#include "timer_wakeup.h"
#endif

#define RIFF_MAGIC 0x46464952

#define RIFF_TYPE_WAVE 0x45564157

#define CHUNK_INFO  0x20746d66
#define CHUNK_DATA  0x61746164

/*
 * Wave files (.wav) are RIFF (Resource Interchange File
 * Format) files with information about the audio and the
 * audio itself.
 *
 * The riff header has a magic number (to know that the
 * file is a RIFF one), the size of the file, and the type
 * of content it has (wave in this case).
 *
 * In order to know the configuration of the audio, there
 * is an information chunk (see struct sound_info).
 *
 * Example of a file:
 *
 * +-------------+
 * |riff_header  |  File header
 * +-------------+
 * |chunk_header | \
 * +-------------+  Information chunk
 * |sound_info   | /
 * +-------------+
 * |chunk_header | \
 * +-------------+  Data chunk
 * |sound data   | /
 * +-------------+
 */

/* file header */
struct riff_header {
	uint32_t magic;
	uint32_t size;
	uint32_t type;
};

/* chunk header */
struct chunk_header {
	uint32_t id;   /* what is the chunk about */
	uint32_t size; /* size of the chunk */
};

/* info chunk */
struct sound_info {
	uint16_t format;
	uint16_t channels;
	uint32_t rate;
	uint32_t bytes_per_second;
	uint16_t bytes_per_sample;
	uint16_t bits_per_sample;
};

/*
 * Structure representing a file. Multiple files can be
 * opened and mixed together.
 */
struct file {
	FILE *file;
	struct sound_info info;
};

static volatile sig_atomic_t _keep_running = 1;

static void
on_signal(int sig)
{
	signal(sig, SIG_IGN);
	_keep_running = 0;
}

static int
open_file(struct file *f, char *filename)
{
	struct riff_header header;
	struct chunk_header chunk_header;
	struct sound_info *info;
	int more_chunks = 1;

	info = &f->info;

	f->file = fopen(filename, "r");
	if (f->file == NULL) {
		fprintf(stderr, "Unable to open file '%s'\n", filename);
		return -1;
	}

	fread(&header, sizeof(header), 1, f->file);
	if (header.magic != RIFF_MAGIC || header.type != RIFF_TYPE_WAVE) {
		fprintf(stderr, "Error: '%s' is not a riff/wave file\n",
		        filename);
		fclose(f->file);
		return -1;
	}

	/* find INFO chunk */

	do {
		fread(&chunk_header, sizeof(chunk_header), 1, f->file);

		switch (chunk_header.id) {
		case CHUNK_INFO:
			fread(info, sizeof(*info), 1, f->file);
			/* If the format header is larger, skip the rest */
			if (chunk_header.size > sizeof(*info))
				fseek(f->file,
				      chunk_header.size - sizeof(*info),
				      SEEK_CUR);
			break;
		case CHUNK_DATA:
			/* Stop looking for chunks */
			more_chunks = 0;
			break;
		default:
			/* Unknown chunk, skip bytes */
			fseek(f->file, chunk_header.size, SEEK_CUR);
		}
	} while (more_chunks);

	return 0;
}

static void
run(struct file *files,        unsigned int files_count, unsigned int card,
    unsigned int device,       unsigned int channels,    unsigned int rate,
    unsigned int bits,         unsigned int period_size,
    unsigned int period_count, unsigned int mmap)
{
#if defined(TIMER_WAKEUP) || defined(DEADLINE_WAKEUP)
	struct snd_timer snd_timer;
#endif
	struct snd pcm;
	struct snd_config config;
	char *buffer;
	int size;
	unsigned int frames;

	/* mix stuff */
	int i;
	void *mix_sum;
	void *mix_dst;

	int tmp;

	/*
	 * Setup
	 * =====
	 */

	memset(&config, 0, sizeof(config));
	config.card =         card;
	config.device =       device;
	config.flags =        SND_OUTPUT | SND_NONBLOCK |
	                      (mmap ? SND_MMAP : 0);
	config.channels =     channels;
	config.rate =         rate;
	config.period_size =  period_size;
	config.period_count = period_count;

	if (bits == 32)
		config.format = SND_FORMAT_S32_LE;
	else if (bits == 16)
		config.format = SND_FORMAT_S16_LE;

#if defined(TIMER_WAKEUP) || defined(DEADLINE_WAKEUP)
	if (snd_timer_open(&snd_timer, &pcm, &config, period_size) == -1) {
#else
	if (snd_open(&pcm, &config) == -1) {
#endif
		fprintf(stderr, "Unable to open sound device\n");
		return;
	}

	size = snd_frames_to_bytes(&pcm, period_size);
	buffer = malloc(size);
	mix_sum = malloc(size * 2);
#if defined(TIMER_WAKEUP) || defined(DEADLINE_WAKEUP)
	/* allocate a bigger buffer in order to handle deviations */
	mix_dst = malloc(size * 2);
#else
	mix_dst = malloc(size);
#endif

	printf("Channels: %u, %u Hz, %u-bits, Access %s\n",
	       channels, rate, bits,
	       mmap ? "MMAP" : "RW");

	/*
	 * Run
	 * ===
	 */

#if   defined(DEADLINE_WAKEUP)
	if (snd_deadline_start(&snd_timer, &pcm) == -1)
		goto _cleanup;
#elif defined(TIMER_WAKEUP)
	snd_timer_start(&snd_timer, &pcm);
#else
	snd_start(&pcm);
#endif

	do {
#if   defined(DEADLINE_WAKEUP)
		/* nothing */
#elif defined(TIMER_WAKEUP)
		struct pollfd pfd = {snd_timer.fd, POLLIN, 0};
#else
		struct pollfd pfd = {pcm.fd, POLLOUT, 0};
#endif

#if defined(DEADLINE_WAKEUP)
		sched_yield();
#else
		if (poll(&pfd, 1, -1) == -1)
			goto _cleanup;
		if (pfd.revents & POLLERR)
			continue;
	#if defined(TIMER_WAKEUP)
		uint64_t ticks;
		if (read(snd_timer.fd, &ticks, sizeof(ticks)) == -1)
			goto _cleanup;
		if (ticks != 1)
			goto _cleanup;
	#endif

#endif /* DEADLINE_WAKEUP */

		/* prepare to mix */
		memset(mix_sum, 0, size * 2);
		memset(mix_dst, 0, size);

		i = files_count;
		while (i--) {
			tmp = fread(buffer, 1, size, files[i].file);
			if (tmp == -1) {
				printf("fread error\n");
				break;
			}

			frames = snd_bytes_to_frames(&pcm, tmp);

			if (frames > 0)
				sndmix(mix_dst, buffer, mix_sum,
				       frames * channels, bits);
		}

#if defined(TIMER_WAKEUP) || defined(DEADLINE_WAKEUP)
		snd_sync(&pcm, SND_SYNC_GET | SND_SYNC_HW);
#else
		snd_sync(&pcm, SND_SYNC_GET);
#endif

		//printf("%ld, %ld\n", pcm.status->hw_ptr, pcm.control->appl_ptr);
		//printf("%ld\n", pcm.control->avail_min);

#if defined(TIMER_WAKEUP) || defined(DEADLINE_WAKEUP)
		tmp = snd_timer_write(&snd_timer, &pcm, mix_dst);
#else
		tmp = snd_write(&pcm, mix_dst, frames);
#endif
		if (tmp < 0) {
			fprintf(stderr, "Error playing sample: %s\n",
				strerror(errno));
			break;
		}
	} while (_keep_running && frames);

_cleanup:
	free(mix_dst);
	free(mix_sum);
	free(buffer);
#if defined(TIMER_WAKEUP) || defined(DEADLINE_WAKEUP)
	snd_timer_close(&snd_timer, &pcm);
#else
	snd_close(&pcm);
#endif
}

int
main(int argc, char **argv)
{
	int opt;
	unsigned int device = 0;
	unsigned int card = 0;
	unsigned int period_size = 1024;
	unsigned int period_count = 4;
	unsigned int mmap = 0;
	char *filename;

	int i;
	int files_count;
	struct file *files;
	struct sound_info *tmp;

	signal(SIGINT, on_signal);

	if (argc < 2) {
		printf("usage: cmd [-c card] [-d device] "
		       "[-p period_size] [-n n_periods] "
		       "[-m mmap access] <files>\n");
		return 1;
	}

	/* parse command line arguments */
	while ((opt = getopt(argc, argv, "+c:d:p:n:m")) != -1) {
		switch (opt) {
		case 'c':
			card = atoi(optarg);
			break;
		case 'd':
			device = atoi(optarg);
			break;
		case 'p':
			period_size = atoi(optarg);
			break;
		case 'n':
			period_count = atoi(optarg);
			break;
		case 'm':
			mmap = 1;
			break;
		}
	}

	files_count = argc - optind;
	if (!files_count) {
		printf("At least one file must be specified\n");
		return 1;
	}

	files = calloc(files_count, sizeof(*files));

	for (i = 0; i < files_count; i++) {
		filename = argv[optind + i];
		if (open_file(&files[i], filename) < 0) {
			goto _go_close_files;
		}
	}

	tmp = &files[0].info;

	run(files, files_count, card, device, tmp->channels,
	    tmp->rate, tmp->bits_per_sample, period_size,
	    period_count, mmap);

	/* clean up */
	i = files_count; /* all files were opened */
_go_close_files:
	/* here if (i < files_count) at least one open has failed */
	while (i--)
		fclose(files[i].file);
	free(files);

	return 0;
}
