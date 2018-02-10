#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/audioio.h>

#include "calmstatus.h"

void *calloc(size_t, size_t);

int
get_mixer()
{
	int fd;

	if ((fd = open("/dev/mixer", O_RDONLY)) == -1)
		err(1, "open");

	return fd;
}

int
get_output(int mixer)
{
	int output = -1;
	mixer_devinfo_t dinf;

	dinf.index = 0;
	while (ioctl(mixer, AUDIO_MIXER_DEVINFO, &dinf) >= 0) {
		if (dinf.type == AUDIO_MIXER_CLASS)
			if (strncmp(dinf.label.name,
			    AudioCoutputs, MAX_AUDIO_DEV_LEN) == 0)
				output = dinf.index;
		dinf.index++;
	}

	if (output == -1)
		errx(1, "couldn't find output group");

	return output;
}

int
get_master(int mixer, int output)
{
	int master = -1;
	mixer_devinfo_t dinf;

	dinf.index = 0;
	while (ioctl(mixer, AUDIO_MIXER_DEVINFO, &dinf) >= 0) {
		if (dinf.mixer_class == output)
			if (strncmp(dinf.label.name,
			    AudioNmaster, MAX_AUDIO_DEV_LEN) == 0)
				master = dinf.index;
		dinf.index++;
	}

	if (master == -1)
		errx(1, "couldn't find master output device");

	return master;
}

int
get_mute(int mixer, int output, int master)
{
	int mute = -1, next = AUDIO_MIXER_LAST;
	mixer_devinfo_t dinf;

	dinf.index = master;
	if (ioctl(mixer, AUDIO_MIXER_DEVINFO, &dinf) < 0)
		err(1, "couldn't get master output info");

	next = dinf.next;
	while (ioctl(mixer, AUDIO_MIXER_DEVINFO, &dinf) >= 0) {
		if (dinf.mixer_class == output)
			if (strncmp(dinf.label.name,
			    AudioNmute, MAX_AUDIO_DEV_LEN) == 0)
				if (dinf.index == next)
					mute = dinf.index;
		if (next != AUDIO_MIXER_LAST)
			next = dinf.next;
		dinf.index++;
	}

	if (mute == -1)
		errx(1, "couldn't find master output mute");

	return mute;
}

int
get_volume_level(soundinfo_t *si)
{
	int pcnt;
	float raw;
	mixer_ctrl_t mc;

	mc.dev = si->master;
	mc.type = AUDIO_MIXER_VALUE;
	mc.un.value.num_channels = 2;

	if (ioctl(si->mixer, AUDIO_MIXER_READ, &mc) == -1)
		err(1, "AUDIO_MIXER_READ");

	raw = mc.un.value.level[AUDIO_MIXER_LEVEL_MONO];
	pcnt = ceil((raw / AUDIO_MAX_GAIN) * 100);

	return pcnt;
}

int
get_mute_status(soundinfo_t *si)
{
	mixer_ctrl_t mc;

	mc.dev = si->mute;
	mc.type = AUDIO_MIXER_ENUM;
	mc.un.value.num_channels = 2;

	if (ioctl(si->mixer, AUDIO_MIXER_READ, &mc) == -1)
		err(1, "AUDIO_MIXER_READ");

	if (mc.un.ord)
		return 1;
	else
		return 0;
}

soundinfo_t *
get_soundinfo()
{
	soundinfo_t *si;
	int output;

	si = calloc(1, sizeof(soundinfo_t));

	si->mixer = get_mixer();
	output = get_output(si->mixer);
	si->master = get_master(si->mixer, output);
	si->mute = get_mute(si->mixer, output, si->master);

	return si;
}

void
destroy_soundinfo(soundinfo_t *si)
{
	close(si->mixer);
	free(si);
}

void
print_volume(soundinfo_t *si)
{
	int volume, ms;

	volume = get_volume_level(si);
	ms = get_mute_status(si);

	printf("%i", volume);
	printf("%s", ms ? " (muted) " : "");
}

int
main()
{
	soundinfo_t *si = get_soundinfo();
	print_volume(si);
}
