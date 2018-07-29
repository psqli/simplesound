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

#ifndef SOUND_MIX_UTILITY_H
#define SOUND_MIX_UTILITY_H

void
sndmix_16(int16_t *dst, int16_t *src, int32_t *sum, unsigned int samples);

void
sndmix_32(int32_t *dst, int32_t *src, int64_t *sum, unsigned int samples);

static inline void
sndmix(void *dst, void *src, void *sum, unsigned int samples, unsigned int bits)
{
	if (bits == 16)
		sndmix_16(dst, src, sum, samples);
	else if (bits == 32)
		sndmix_32(dst, src, sum, samples);
}

#endif /* SOUND_MIX_UTILITY_H */
