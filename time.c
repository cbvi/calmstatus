#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/audioio.h>
#include <sys/ioctl.h>

int
getvolume(char *buf, size_t sz)
{
	int mfd;
	char mixer[] = "/dev/mixer";
	mixer_devinfo_t dinfo;
	mixer_devinfo_t minfo;
	mixer_ctrl_t cinfo;
	int vol;

	if ((mfd = open(mixer, O_RDWR)) < 0)
		return -1;

	dinfo.index = 0;
	while (ioctl(mfd, AUDIO_MIXER_DEVINFO, &dinfo) >= 0) {
		if (dinfo.type != AUDIO_MIXER_CLASS) {
			dinfo.index++;
			continue;
		}
		if (strncmp(dinfo.label.name, AudioCoutputs, MAX_AUDIO_DEV_LEN) == 0)
			break;

		dinfo.index++;
	}

	minfo.index = 0;
	while (ioctl(mfd, AUDIO_MIXER_DEVINFO, &minfo) >= 0) {
		if (strncmp(minfo.label.name, AudioNmaster, MAX_AUDIO_DEV_LEN) == 0) {
		break;
		}
		minfo.index++;
	}

	cinfo.dev = minfo.index;
	cinfo.type = AUDIO_MIXER_VALUE;

	if (ioctl(mfd, AUDIO_MIXER_READ, &cinfo) == -1) {
		return -1;
	}

	float avgf = ((float)cinfo.un.value.level[AUDIO_MIXER_LEVEL_MONO] / AUDIO_MAX_GAIN) * 100;
        vol = (int)avgf;
	vol = (avgf - vol < 0.5 ? vol : (vol + 1));

	printf("%f\n", avgf);

	return 0;
}

int
getdate(char *buf, size_t sz)
{
	time_t clock;
	struct tm *timeptr;

	if ((clock = time(NULL)) == -1)
		return -1;

	if ((timeptr = localtime(&clock)) == NULL)
		return -1;

	strftime(buf, sz, "%a %d %b %H:%M", timeptr);

	return 0;
}

int
main()
{
	char right[] = "%{r}";
	char left[] = "%{l}";
	char date[32];

	if (getvolume(NULL, 0) == -1) {
		err(1, "getvolume");
	}

	for (;;) {
		if (getdate(date, sizeof(date)) == -1) {
			err(1, "getdate");
		}

		printf("%s%s\n", right, date);
		sleep(1);
	}

	return 0;
}
