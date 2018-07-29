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
 * 08/06/2018
 *
 * SNDRV_* constants are from sound/asound.h
 */

#ifndef SOUND_PARAMETERS_H
#define SOUND_PARAMETERS_H

/* ALSA header */
#include <sound/asound.h>

struct snd_parameters {
	struct snd_pcm_hw_params hw_params;
};

/*
 * masks
 * =====
 *
 * - ACCESS: MMAP or RW. At the moment only interleaved is
 *   supported.
 *
 * - FORMAT: 8-bit, 16-bit and 32-bit. Little or Big
 *   endian. Unsigned or Signed.
 */

/* ACCESS */
#define SND_ACCESS        SNDRV_PCM_HW_PARAM_ACCESS
#define SND_ACCESS_MMAP   SNDRV_PCM_ACCESS_MMAP_INTERLEAVED
#define SND_ACCESS_RW     SNDRV_PCM_ACCESS_RW_INTERLEAVED

/* FORMAT */
#define SND_FORMAT         SNDRV_PCM_HW_PARAM_FORMAT
#define SND_FORMAT_S8      SNDRV_PCM_FORMAT_S8
#define SND_FORMAT_U8      SNDRV_PCM_FORMAT_U8
#define SND_FORMAT_S16_LE  SNDRV_PCM_FORMAT_S16_LE
#define SND_FORMAT_S16_BE  SNDRV_PCM_FORMAT_S16_BE
#define SND_FORMAT_U16_LE  SNDRV_PCM_FORMAT_U16_LE
#define SND_FORMAT_U16_BE  SNDRV_PCM_FORMAT_U16_BE
#define SND_FORMAT_S32_LE  SNDRV_PCM_FORMAT_S32_LE
#define SND_FORMAT_S32_BE  SNDRV_PCM_FORMAT_S32_BE
#define SND_FORMAT_U32_LE  SNDRV_PCM_FORMAT_U32_LE
#define SND_FORMAT_U32_BE  SNDRV_PCM_FORMAT_U32_BE

/*
 * intervals
 * =========
 *
 * Main ones (the only ones it's necessary to set):
 * - CHANNELS
 * - RATE (frames/s)
 * - PERIOD_SIZE (frames)
 * - PERIODS (periods in buffer)
 *
 * BUFFER_SIZE: Generally, PERIOD_SIZE * PERIODS define
 * the BUFFER_SIZE. However, in some devices buffer size
 * is not required to be a multiple of period size, so
 * this can be set.
 *
 * Secondary ones (interesting to get them after set the
 * main ones):
 * - SAMPLE_BITS
 * - PERIOD_TIME (us)
 * - TICK_TIME (us)
 */

/* Main */
#define SND_CHANNELS     SNDRV_PCM_HW_PARAM_CHANNELS
#define SND_RATE         SNDRV_PCM_HW_PARAM_RATE
#define SND_PERIOD_SIZE  SNDRV_PCM_HW_PARAM_PERIOD_SIZE
#define SND_PERIODS      SNDRV_PCM_HW_PARAM_PERIODS

#define SND_BUFFER_SIZE  SNDRV_PCM_HW_PARAM_BUFFER_SIZE

/* Secondary */
#define SND_SAMPLE_BITS  SNDRV_PCM_HW_PARAM_SAMPLE_BITS
#define SND_PERIOD_TIME  SNDRV_PCM_HW_PARAM_PERIOD_TIME
#define SND_TICK_TIME    SNDRV_PCM_HW_PARAM_TICK_TIME

/*
 * Flags
 * =====
 */

#define SND_NO_INTERRUPTS  SNDRV_PCM_HW_PARAMS_NO_PERIOD_WAKEUP

int
sound_params_test(struct snd_parameters *p, unsigned int parameter,
                  unsigned int value);

void
sound_params_get_interval(struct snd_parameters *p, unsigned int parameter,
                          unsigned int *min, unsigned int *max);

int
sound_params_init(int fd, struct snd_parameters *params);

static inline unsigned int
sound_params_get_min(struct snd_parameters *p, unsigned int parameter)
{
	unsigned int min, max;

	sound_params_get_interval(p, parameter, &min, &max);

	return min;
}

static inline unsigned int
sound_params_get_max(struct snd_parameters *p, unsigned int parameter)
{
	unsigned int min, max;

	sound_params_get_interval(p, parameter, &min, &max);

	return max;
}

static inline unsigned int
sound_format_to_bytes(unsigned int format)
{
	switch (format) {
	case SND_FORMAT_S8:
	case SND_FORMAT_U8:
		return 1;
	case SND_FORMAT_S16_LE:
	case SND_FORMAT_S16_BE:
	case SND_FORMAT_U16_LE:
	case SND_FORMAT_U16_BE:
		return 2;
	case SND_FORMAT_S32_LE:
	case SND_FORMAT_S32_BE:
	case SND_FORMAT_U32_LE:
	case SND_FORMAT_U32_BE:
		return 4;
	}

	return 0;
}

#endif /* SOUND_PARAMETERS_H */
