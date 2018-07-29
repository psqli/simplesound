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
 * Rewritten in 08/06/2018
 *
 * manipulate hardware parameters
 */

#ifdef BUILDING_FOR_GOOGLE_ANDROID
#include <bits/timespec.h>
#endif

#include <assert.h>  /* assert() */
#include <limits.h>  /* UINT_MAX */
#include <stdint.h>  /* int*_t */
#include <string.h>  /* memset() */

/* ALSA header */
#include <sound/asound.h>

#define is_interval(parameter) \
	(parameter >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL && \
	 parameter <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL)

#define is_mask(parameter) \
	(parameter >= SNDRV_PCM_HW_PARAM_FIRST_MASK && \
	 parameter <= SNDRV_PCM_HW_PARAM_LAST_MASK)

/*
 * Transform the index of an array of bits to an index
 * of an array of 'uint32_t'.
 *
 * E.g. If x is between 0 and 31, the result is 0. If it's
 * between 32 and 63, the result is 1. And so on.
 *
 * (x >> 5) is the same as (x / 32), or how many times 32
 * fits in x.
 */
#define mask_index(x)  ((x) >> 5)

/*
 * (1 << #): do a left bit shift of # positions.
 *
 * (x & 31) is the same as (x & 00011111), and is used to
 * limit the bit shift to 32.
 */
#define mask_bit(x)  (1 << ((x) & 31))

static inline struct snd_interval*
get_interval_struct(struct snd_pcm_hw_params *p, unsigned int parameter)
{
	return &p->intervals[parameter - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL];
}

static inline struct snd_mask*
get_mask_struct(struct snd_pcm_hw_params *p, unsigned int parameter)
{
	return &p->masks[parameter - SNDRV_PCM_HW_PARAM_FIRST_MASK];
}

void
hw_param_set_interval(struct snd_pcm_hw_params *p, int parameter,
                      int minimum, int maximum)
{
	struct snd_interval *i = get_interval_struct(p, parameter);

	i->min = minimum;
	i->max = maximum;

	/*
	 * openmin and openmax excludes min and max values
	 *
	 * E.g. Suppose max=5. If openmax, the maximum
	 * value is 4, thus value must be less than max,
	 * not less or equal.
	 */
	i->openmin = 0;
	i->openmax = 0;
	i->integer = 1;
	i->empty = 0;
}

void
hw_param_set_mask(struct snd_pcm_hw_params *p, int parameter,
                  unsigned int value)
{
	struct snd_mask *m = get_mask_struct(p, parameter);

	/*
	 * clear the parameter before set it. See
	 * hw_param_fill()
	 */
	memset(m, 0x00, sizeof(*m));

	/* mask->bits[X] is a 32-bit integer */
	m->bits[mask_index(value)] |= mask_bit(value);
}

void
hw_param_set(struct snd_pcm_hw_params *p, int parameter,
             unsigned int value)
{
	if (is_interval(parameter))
		hw_param_set_interval(p, parameter, value, value);
	else if (is_mask(parameter))
		hw_param_set_mask(p, parameter, value);
}

/* After HW_PARAMS ioctl, max becomes equal min */
void
hw_param_get_interval(struct snd_pcm_hw_params *p, int parameter,
                      unsigned int *min, unsigned int *max)
{
	struct snd_interval *interval = get_interval_struct(p, parameter);

	/*
	 * NOTE: According to the ALSA kernel code, if
	 * integer is specified, openmin and openmax
	 * should be always zero.
	 * See snd_interval_refine() in pcm_lib.c
	 */
	assert(interval->openmin == 0);
	assert(interval->openmax == 0);

	*min = interval->min;
	*max = interval->max;
}

/*
 * Return 1 if value is set, otherwise 0
 *
 * Example of parameter and value:
 * parameter = format, value = 16-bit
 */
unsigned int
hw_param_get_mask(struct snd_pcm_hw_params *p, int parameter,
                  unsigned int value)
{
	struct snd_mask *mask = get_mask_struct(p, parameter);

	/* mask->bits[X] is a 32-bit integer */
	return mask->bits[mask_index(value)] & mask_bit(value);
}

unsigned int
hw_param_get(struct snd_pcm_hw_params *p, int parameter,
             unsigned int value)
{
	unsigned int ret, tmp;

	if (is_interval(parameter))
		hw_param_get_interval(p, parameter, &ret, &tmp);
	else if (is_mask(parameter))
		ret = hw_param_get_mask(p, parameter, value);

	return ret;
}

/* define the absolute LAST_MASK and LAST_INTERVAL */

#define LAST_MASK \
	(SNDRV_PCM_HW_PARAM_LAST_MASK - SNDRV_PCM_HW_PARAM_FIRST_MASK)

#define LAST_INTERVAL \
	(SNDRV_PCM_HW_PARAM_LAST_INTERVAL - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL)

/*
 * fill the hardware parameters structure with all
 * possible configurations
 *
 * In HW_REFINE ioctl, ALSA filters the structure and
 * lefts only the configuration allowed by the hardware.
 *
 * For HW_PARAMS ioctl, all the other configuration that
 * is not being set must be passed filled. This is because
 * ALSA assumes the configuration is invalid if it's not
 * in the range allowed by the hardware or if it doesn't
 * respect the defined rules (see
 * snd_pcm_hw_constraints_init() in pcm_native.c).
 */
void
hw_param_fill(struct snd_pcm_hw_params *p)
{
	int i;

	memset(p, 0, sizeof(*p));

	/*
	 * Fill all masks with ones, so ALSA will return
	 * the allowed values back to us.
	 *
	 * Internally, ALSA does a bitwise AND (&) with
	 * the allowed values.
	 *
	 * 0xff is equal to 11111111
	 */
	memset(p->masks, 0xff, sizeof(p->masks));

	/*
	 * Fill intervals with lowest possible value in
	 * min and maximum possible value in max.
	 *
	 * ALSA checks whether the passed minimum value is
	 * less than the minimum allowed one. If yes, it
	 * returns the minimum allowed value. Otherwise
	 * not. The same is valid for maximum.
	 */
	for (i = 0; i <= LAST_INTERVAL; i++) {
		p->intervals[i].min = 0;
		p->intervals[i].max = UINT_MAX;
	}

	/*
	 * Request mask: ALSA will only refine the
	 * parameters (intervals and masks) that are in
	 * this mask. As we want to refine all parameters,
	 * here it's completely filled in.
	 */
	p->rmask = UINT_MAX;

	/*
	 * Changed mask: after HW_REFINE ioctl(), ALSA
	 * returns the parameters it have changed in this
	 * mask.
	 */
	p->cmask = 0;

	/*
	 * Some additional information flags returned by
	 * ALSA.
	 *
	 * See SNDRV_PCM_INFO_* flags in sound/asound.h
	 */
	p->info = UINT_MAX;

	/*
	 * The following variables are not relevant
	 * because there are parameters that return
	 * the same information:
	 * - SNDRV_PCM_HW_PARAM_SAMPLE_BITS (msbits)
	 * - SNDRV_PCM_HW_PARAM_RATE (rate_num, rate_den)
	 *
	 * NOTE: rate denominator seems to be always 1,
	 * thus rate = rate_num
	 */
	p->msbits = 0;
	p->rate_num = 0;
	p->rate_den = 0;
}
