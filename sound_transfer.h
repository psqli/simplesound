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

#ifndef SOUND_TRANSFER_H
#define SOUND_TRANSFER_H

#include "sound_global.h"

/*
 * MMAP access
 * ===========
 */

int
snd_mmap_areas_copy(struct snd *snd, unsigned int pcm_offset, char *buf,
                      unsigned int user_offset, unsigned int frames);

int
snd_update_appl_ptr(struct snd *snd, unsigned int frames);

ssize_t
snd_mmap_transfer(struct snd *snd, void *buffer, unsigned int bytes);

/*
 * RW access
 * =========
 */

ssize_t
snd_ioctl_transfer(struct snd *snd, void *buffer, unsigned int bytes);

/*
 * Generic read/write
 * ==================
 */

int
snd_write(struct snd *snd, const void *data, unsigned int bytes);

int
snd_read(struct snd *snd, void *data, unsigned int bytes);

#endif /* SOUND_TRANSFER_H */
