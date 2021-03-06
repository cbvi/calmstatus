/*
 * Copyright (c) 2018 Carlin Bingham <cb@viennan.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/audioio.h>

#include "calmstatus.h"
#include "priv.h"

/* XXX
 * This won't work now that userland is shut off from accessing the
 * mixer directly. Needs to be updated to work with the new async API.
 */

typedef struct {
	int mixer;
	int master;
	int mute;
	procinfo_t *procinfo;
} soundinfo_t;

static volatile sig_atomic_t running = 1;

static int
volume_get_mixer()
{
	int fd;

	if ((fd = open("/dev/mixer", O_RDONLY)) == -1)
		err(1, "open");

	return fd;
}

static int
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

static int
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

static int
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

static int
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

static int
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

static soundinfo_t *
volume_get_soundinfo(procinfo_t *procinfo)
{
	soundinfo_t *si;
	int output;

	si = xcalloc(1, sizeof(soundinfo_t));

	si->mixer = volume_get_mixer();
	output = volume_get_output_group(si->mixer);
	si->master = volume_get_master_device(si->mixer, output);
	si->mute = volume_get_mute_device(si->mixer, output, si->master);
	si->procinfo = procinfo;

	return si;
}

static void
volume_destroy_soundinfo(soundinfo_t *si)
{
	close(si->mixer);
	free(si);
}

static void
volume_signal_output(procinfo_t *info)
{
	priv_send_cmd(info->output, CMD_OUTPUT_DO);
}

static void
volume_goodbye_output(procinfo_t *info)
{
	priv_send_cmd(info->output, CMD_GOODBYE);
}

void *
volume_watch_for_changes(void *arg)
{
	soundinfo_t *si;
	int pvol = -1, pmute = -1;
	int volume, mute;
	int ischanging = 0;

	si = (soundinfo_t *)arg;

	while (running) {
		volume = volume_get_level(si);
		mute = volume_get_mute(si);
		if (volume != pvol) {
			volume_signal_output(si->procinfo);
			ischanging = 1;
		} else if (mute != pmute) {
			volume_signal_output(si->procinfo);
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
	volume_goodbye_output(si->procinfo);
	return NULL;
}

int
volume_level(struct imsgbuf *ibuf)
{
	struct imsg imsg;
	int res;

	priv_send_cmd(ibuf, CMD_VOLUME_LEVEL);
	priv_get_res(ibuf, &imsg);

	if ((imsg.hdr.len - IMSG_HEADER_SIZE) != sizeof(res))
			err(1, "volume_level: response wrong size");

	res = *(int *)imsg.data;
	imsg_free(&imsg);

	return res;
}

int
volume_mute(struct imsgbuf *ibuf)
{
	struct imsg imsg;
	int res;

	priv_send_cmd(ibuf, CMD_VOLUME_MUTE);
	priv_get_res(ibuf, &imsg);

	if ((imsg.hdr.len - IMSG_HEADER_SIZE) != sizeof(res))
			err(1, "volume_mute: response wrong size");

	res = *(int *)imsg.data;
	imsg_free(&imsg);

	return res;
}

int
volume_main(procinfo_t *info)
{
	soundinfo_t *si;
	enum priv_cmd cmd;
	pthread_t thr;
	int res;
	int ret = 1;

	setproctitle("volume");

	si = volume_get_soundinfo(info);

	pthread_create(&thr, NULL, volume_watch_for_changes, si);

	while (running) {
		cmd = priv_get_cmd(info->volume);

		switch (cmd) {
		case CMD_VOLUME_LEVEL:
			res = volume_get_level(si);
			priv_send_res(info->volume, RES_VOLUME_LEVEL,
			    &res, sizeof(res));
			break;
		case CMD_VOLUME_MUTE:
			res = volume_get_mute(si);
			priv_send_res(info->volume, RES_VOLUME_MUTE,
			    &res, sizeof(res));
			break;
		case CMD_STOP_RIGHT_NOW:
			running = 0;
			ret = 0;
			break;
		default:
			warnx("volume_main: invalid cmd");
			running = 0;
			ret = 1;
			break;
		}
	}
	volume_goodbye_output(info);

	pthread_cancel(thr);
	volume_destroy_soundinfo(si);
	destroy_procinfo(info);
	return ret;
}
