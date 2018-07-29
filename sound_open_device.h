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
 * 09/06/2018
 */

#ifndef SOUND_OPEN_DEVICE_H
#define SOUND_OPEN_DEVICE_H

/* flags */
#define SND_OUTPUT     0x00000000
#define SND_INPUT      0x00000001
#define SND_NONBLOCK   0x00000002

int
snd_device_open(unsigned int card, unsigned int device, int flags);

#endif /* SOUND_OPEN_DEVICE_H */
