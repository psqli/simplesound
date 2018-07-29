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

#ifndef HARDWARE_PARAMETERS_H
#define HARDWARE_PARAMETERS_H

/* ALSA header */
#include <sound/asound.h>

/*
 * set parameters
 * ==============
 */

void
hw_param_set_interval(struct snd_pcm_hw_params *p, int parameter,
                      int minimum, int maximum);

void
hw_param_set_mask(struct snd_pcm_hw_params *p, int parameter,
                  unsigned int value);

void
hw_param_set(struct snd_pcm_hw_params *p, int parameter,
             unsigned int value);

/*
 * get parameters
 * ==============
 */

void
hw_param_get_interval(struct snd_pcm_hw_params *p, int parameter,
                      unsigned int *min, unsigned int *max);

unsigned int
hw_param_get_mask(struct snd_pcm_hw_params *p, int parameter,
                  unsigned int value);

unsigned int
hw_param_get(struct snd_pcm_hw_params *p, int parameter,
             unsigned int value);

/*
 * initialize parameters structure
 * ===============================
 */

void
hw_param_fill(struct snd_pcm_hw_params *p);

#endif /* HARDWARE_PARAMETERS_H */
