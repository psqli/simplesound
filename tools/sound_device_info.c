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
 *
 * get information about sound device
 *
 * Manual compilation:
 * $ gcc -o sdi hardware_parameters.c sound_parameters.c sound_open_device.c
 *   sound_device_info.c
 */

#include <stdio.h>  /* printf() */
#include <unistd.h> /* close() */

#include "sound_open_device.h" /* SND_OUTPUT and SND_INPUT */
#include "sound_parameters.h"  /* SND_* */

static void
print_sign(struct snd_parameters *p, unsigned int f_unsigned,
           unsigned int f_signed)
{
	f_unsigned = sound_params_test(p, SND_FORMAT, f_unsigned);
	f_signed =   sound_params_test(p, SND_FORMAT, f_signed);

	if (f_unsigned && f_signed)
		printf("Unsigned and Signed\n");
	else if (f_unsigned)
		printf("Unsigned only\n");
	else if (f_signed)
		printf("Signed only\n");
	else
		printf("Not supported\n");
}

static void
print_endian(struct snd_parameters *p,
             unsigned int f_little_u,
             unsigned int f_little_s,
             unsigned int f_big_u,
             unsigned int f_big_s)
{
	printf("  Little Endian: ");
	print_sign(p, f_little_u, f_little_s);
	printf("  Big Endian: ");
	print_sign(p, f_big_u, f_big_s);
}

static void
print_formats(struct snd_parameters *p)
{
	printf("8-bits: ");
	print_sign(p, SND_FORMAT_U8, SND_FORMAT_S8);

	printf("16-bits:\n");
	print_endian(p, SND_FORMAT_U16_LE, SND_FORMAT_S16_LE,
	                SND_FORMAT_U16_BE, SND_FORMAT_S16_BE);

	printf("32-bits:\n");
	print_endian(p, SND_FORMAT_U32_LE, SND_FORMAT_S32_LE,
	                SND_FORMAT_U32_BE, SND_FORMAT_S32_BE);
}

static void
print_range(struct snd_parameters *p, unsigned int parameter)
{
	unsigned int min, max;

	sound_params_get_interval(p, parameter, &min, &max);
	printf("min: %u, max: %u\n", min, max);
}

static void
print_parameters(struct snd_parameters *p)
{
	printf("MMAP access: %s\n",
	       sound_params_test(p, SND_ACCESS, SND_ACCESS_MMAP)
	       ? "Yes" : "No");

	printf("Sample bits: ");
	print_range(p, SND_SAMPLE_BITS);

	print_formats(p);

	printf("Channels: ");
	print_range(p, SND_CHANNELS);

	printf("Rate (frames/s): ");
	print_range(p, SND_RATE);

	printf("Period size (frames): ");
	print_range(p, SND_PERIOD_SIZE);

	printf("Periods: ");
	print_range(p, SND_PERIODS);

	printf("Buffer size (frames): ");
	print_range(p, SND_BUFFER_SIZE);
}

#if 1

#include <sound/asound.h> /* struct snd_pcm_info */
#include <sys/ioctl.h>    /* ioctl() */

static void
print_info(int fd)
{
	struct snd_pcm_info i;

	if (ioctl(fd, SNDRV_PCM_IOCTL_INFO, &i) == -1)
		return;

	printf("Name: %s\n", i.name);
}

#endif

static int
print_device(unsigned int card, unsigned int device, unsigned int type)
{
	int fd;
	struct snd_parameters p;

	if (type == SND_OUTPUT)
		printf("Playback\n");
	else
		printf("Capture\n");

	fd = sound_device_open(card, device, type | SND_NONBLOCK);
	if (fd == -1) {
		perror("Couldn't open device");
		return 1;
	}

	if (sound_params_init(fd, &p) == -1)
		goto _go_close;

	print_info(fd);
	print_parameters(&p);

	close(fd);

	return 0;

_go_close:
	close(fd);
	return -1;
}

int
main(int argc, char **argv)
{
	int card, device;

	if (argc < 3) {
		printf("usage: cmd <card> <device>\n\n"
		       "Check out pcm* files in /dev/snd/\n"
		       "e.g. pcmC<card>D<device> P(layback) or C(apture)\n");
		return 1;
	}

	card =   atoi(argv[1]);
	device = atoi(argv[2]);

	if (print_device(card, device, SND_OUTPUT) == -1)
		return 1;

	printf("\n");

	if (print_device(card, device, SND_INPUT) == -1)
		return 1;

	return 0;
}
