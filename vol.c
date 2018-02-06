#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>
#include <string.h>
#include <math.h>
#include <err.h>

int
main()
{
	int fd, output = -1, master = -1, mute = -1;
	int pcnt;
	float raw;
	mixer_devinfo_t dinf1, dinf2;
	mixer_ctrl_t ctr;

	if ((fd = open("/dev/mixer", O_RDONLY)) < 0)
		err(1, "open");

	dinf1.index = 0;
	while (ioctl(fd, AUDIO_MIXER_DEVINFO, &dinf1) >= 0) {
		if (dinf1.type != AUDIO_MIXER_CLASS) {
			dinf1.index++;
			continue;
		}
		if (strncmp(dinf1.label.name,
		    AudioCoutputs, MAX_AUDIO_DEV_LEN) == 0) {
			output = dinf1.index;
		}
		dinf1.index++;
	}

	if (output == -1)
		err(1, "couldn't find output");

	dinf2.index = 0;
	while (ioctl(fd, AUDIO_MIXER_DEVINFO, &dinf2) >= 0) {
		if (dinf2.mixer_class == output) {
			if (dinf2.type == AUDIO_MIXER_VALUE) {
				if (strncmp(dinf2.label.name,
				    AudioNmaster, MAX_AUDIO_DEV_LEN) == 0) {
					master = dinf2.index;
				}
			}
			if (dinf2.type == AUDIO_MIXER_ENUM) {
				if (strncmp(dinf2.label.name,
				    AudioNmute, MAX_AUDIO_DEV_LEN) == 0) {
					mute = dinf2.index;
				}
			}
		}
		dinf2.index++;
	}

	if (master == -1)
		errx(1, "couldn't find master");
	if (mute == -1)
		errx(1, "couldn't find mute");

	dinf1.index = master;
	if (ioctl(fd, AUDIO_MIXER_DEVINFO, &dinf1) == -1)
		err(1, "ioctl");

	ctr.dev = master;
	ctr.type = AUDIO_MIXER_VALUE;

	for (;;) {
	if (ioctl(fd, AUDIO_MIXER_READ, &ctr) == -1)
		err(1, "ioctl");

	raw = ((float)ctr.un.value.level[AUDIO_MIXER_LEVEL_MONO] / AUDIO_MAX_GAIN);
	pcnt = ceil(raw * 100);

	printf("%i\n", pcnt);
	sleep(1);
	}
}
