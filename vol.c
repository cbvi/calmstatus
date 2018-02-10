#include <stdio.h>
#include <err.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/audioio.h>

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
get_volume(int mixer, int master)
{
	int pcnt;
	float raw;
	mixer_ctrl_t mc;

	mc.dev = master;
	mc.type = AUDIO_MIXER_VALUE;
	mc.un.value.num_channels = 2;

	if (ioctl(mixer, AUDIO_MIXER_READ, &mc) == -1)
		err(1, "AUDIO_MIXER_READ");

	raw = mc.un.value.level[AUDIO_MIXER_LEVEL_MONO];
	pcnt = ceil((raw / AUDIO_MAX_GAIN) * 100);

	return pcnt;
}

int
get_mute_status(int mixer, int mute)
{
	mixer_ctrl_t mc;

	mc.dev = mute;
	mc.type = AUDIO_MIXER_ENUM;
	mc.un.value.num_channels = 2;

	if (ioctl(mixer, AUDIO_MIXER_READ, &mc) == -1)
		err(1, "AUDIO_MIXER_READ");

	if (mc.un.ord)
		return 1;
	else
		return 0;
}

int
main()
{
	int fd, output, master, mute, volume, ms;

	fd = get_mixer();
	output = get_output(fd);
	master = get_master(fd, output);
	mute = get_mute(fd, output, master);
	volume = get_volume(fd, master);
	ms = get_mute_status(fd, mute);

	printf("%i %i\n", volume, ms);
}
