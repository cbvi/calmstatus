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

int
volume_get_mixer()
{
	int fd;

	if ((fd = open("/dev/mixer", O_RDONLY)) == -1)
		err(1, "open");

	return fd;
}

int
volume_get_output_group(int mixer)
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
volume_get_master_device(int mixer, int output)
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
volume_get_mute_device(int mixer, int output, int master)
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
volume_get_level(soundinfo_t *si)
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
volume_get_mute(soundinfo_t *si)
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
volume_get_soundinfo()
{
	soundinfo_t *si;
	int output;

	si = xcalloc(1, sizeof(soundinfo_t));

	si->mixer = volume_get_mixer();
	output = volume_get_output_group(si->mixer);
	si->master = volume_get_master_device(si->mixer, output);
	si->mute = volume_get_mute_device(si->mixer, output, si->master);

	return si;
}

void
volume_destroy_soundinfo(soundinfo_t *si)
{
	close(si->mixer);
	free(si);
}

void
volume_print_volume(soundinfo_t *si)
{
	int volume, ms;

	volume = volume_get_level(si);
	ms = volume_get_mute(si);

	printf("%i", volume);
	printf("%s", ms ? " (muted) " : "");
}

void *
volume_watch_for_changes(void *arg)
{
	info_t *info;
	soundinfo_t *si;
	int pvol = -1, pmute = -1;
	int volume, mute;
	int ischanging = 0;

	info = (info_t *)arg;
	si = info->soundinfo;

	for (;;) {
		volume = volume_get_level(si);
		mute = volume_get_mute(si);
		if (volume != pvol) {
			do_output(info);
			ischanging = 1;
		} else if (mute != pmute) {
			do_output(info);
			ischanging = 1;
		} else
			ischanging = 0;
		pvol = volume;
		pmute = mute;
		if (ischanging)
			sleep(1);
		else
			sleep(3);
	}
}
