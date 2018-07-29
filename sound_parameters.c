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
 * get hardware parameters
 */

#include <sys/ioctl.h> /* ioctl() */

/* ALSA header */
#include <sound/asound.h>

#include "sound_parameters.h"    /* struct snd_parameters */
#include "hardware_parameters.h" /* hw_param_*() */

int
snd_params_test(struct snd_parameters *p, unsigned int parameter,
                  unsigned int value)
{
	return hw_param_get_mask(&p->hw_params, parameter, value);
}

/* get minimum and maximum value of a given parameter */
void
snd_params_get_interval(struct snd_parameters *p, unsigned int parameter,
                          unsigned int *min, unsigned int *max)
{
	hw_param_get_interval(&p->hw_params, parameter, min, max);
}

int
snd_params_init(int fd, struct snd_parameters *p)
{
	hw_param_fill(&p->hw_params);
	if (ioctl(fd, SNDRV_PCM_IOCTL_HW_REFINE, &p->hw_params) == -1)
		return -1;

	return 0;
}
