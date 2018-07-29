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
 * 15/07/2017
 *
 * sound mix utility
 *
 * Based on alsa-lib's dmix plugin
 *
 * sum is used to store the sum of all samples mixed into
 * dst. sum is necessary because dst can't store a value
 * greater than its resolution:
 *
 *  ============ sum max
 *       .-.
 *  ============ dst max
 *   .´       `.
 *  ´           `.
 *  dst min ============
 *                  `-´
 *  sum min ============
 */

#include <stdint.h> /* int*_t */

void
sndmix_16(int16_t *dst, int16_t *src, int32_t *sum, unsigned int samples)
{
	int32_t sample;

	while (samples--) {
		sample = *src;

		if (*dst == 0) {
			*sum = *dst = sample;
		} else {
			*sum += sample;
			sample = *sum;
			if (sample > INT16_MAX)
				sample = INT16_MAX;
			else if (sample < INT16_MIN)
				sample = INT16_MIN;
			*dst = sample;
		}

		src++;
		dst++;
		sum++;
	}
}

void
sndmix_32(int32_t *dst, int32_t *src, int64_t *sum, unsigned int samples)
{
	int64_t sample;

	while (samples--) {
		sample = *src;

		if (*dst == 0) {
			*sum = *dst = sample;
		} else {
			*sum += sample;
			sample = *sum;
			if (sample > INT32_MAX)
				sample = INT32_MAX;
			else if (sample < INT32_MIN)
				sample = INT32_MIN;
			*dst = sample;
		}

		src++;
		dst++;
		sum++;
	}
}
