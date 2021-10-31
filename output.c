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
#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "calmstatus.h"

pthread_mutex_t mut;

static volatile sig_atomic_t running = 1;

void
init_output()
{
	pthread_mutex_init(&mut, NULL);
}

void *
xcalloc(size_t n, size_t sz)
{
	void *p;

	if ((p = calloc(n, sz)) == NULL) {
		warnx("calloc failed allocating %lu * %lu", n, sz);
		abort();
	}

	return p;
}

static void
signal_hdlr(int sig)
{
	(void)sig;
	running = 0;
}

static void
terminate_processes(procinfo_t *info)
{
	priv_send_cmd(info->xstuff, CMD_STOP_RIGHT_NOW);
	//priv_send_cmd(info->volume, CMD_STOP_RIGHT_NOW);
}

static void left() { printf("%s", "%{l}"); }
static void right() { printf("%s", "%{r}"); }
static void pad(int i) { printf("%%{O%i}", i); }

void
do_output(procinfo_t *info)
{
	//int level, mute;
	uint32_t curdesktop, actlist[10];
	char title[MAX_TITLE_LENGTH];

	if (pthread_mutex_trylock(&mut) != 0)
		return;

	left();

	curdesktop = xstuff_currentdesktop(info->xstuff);
	xstuff_activeworkspaces(info->xstuff, actlist);
	xstuff_windowtitle(info->xstuff, title, sizeof(title));

	print_workspaces(actlist, curdesktop);

	printf("%s ", title);

	right();
	printf(" ");

	/* volume_print_volume(info->soundinfo); */
	/*level = volume_level(info->volume);
	mute = volume_mute(info->volume);
	printf("%i%%%s", level, mute ? " (muted)" : "");*/

	pad(100);

	printf(" ");
	print_datetime();
	printf("%s", "  ");

	printf("\n");
	fflush(stdout);

	pthread_mutex_unlock(&mut);
}

int
output_main(procinfo_t *info)
{
	enum priv_cmd cmd;

	signal(SIGTERM, signal_hdlr);
	signal(SIGCHLD, signal_hdlr);

	while (running) {
		cmd = priv_wait_cmd(info->output, 1000 * 10);

		switch (cmd) {
		case CMD_OUTPUT_DO:
			do_output(info);
			break;
		case CMD_GOODBYE:
			running = 0;
			break;
		case CMD_TRYAGAIN:
			break;
		default:
			warnx("output_main: invalid cmd");
			running = 0;
			break;
		}
	}

	terminate_processes(info);
	destroy_procinfo(info);

	return 1;
}
