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
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "calmstatus.h"
#include "priv.h"

procinfo_t *procinfo = NULL;

static procinfo_t **
get_procinfo()
{
	procinfo_t **info;
	int outsock[2], volsock[2], xsock[2];
	int i;

	info = xcalloc(2, sizeof(procinfo_t *));
	priv_socketpair(outsock);
	priv_socketpair(volsock);
	priv_socketpair(xsock);

	for (i = 0; i < 2; i++) {
		info[i] = xcalloc(1, sizeof(procinfo_t));
		info[i]->output = xcalloc(1, sizeof(struct imsgbuf));
		info[i]->volume = xcalloc(1, sizeof(struct imsgbuf));
		info[i]->xstuff = xcalloc(1, sizeof(struct imsgbuf));
		imsg_init(info[i]->output, outsock[i]);
		imsg_init(info[i]->volume, volsock[i]);
		imsg_init(info[i]->xstuff, xsock[i]);
	}

	return info;
}

void
destroy_procinfo(procinfo_t *info)
{
	imsg_clear(info->output);
	imsg_clear(info->volume);
	imsg_clear(info->xstuff);

	free(info);
}

int
main()
{
	procinfo_t **info;

	info = get_procinfo();

	if (fork() == 0) {
		destroy_procinfo(info[0]);
		return volume_main(info[1]);
	}

	if (fork() == 0) {
		destroy_procinfo(info[0]);
		return xstuff_main(info[1]);
	}

	destroy_procinfo(info[1]);

	if (pledge("stdio", NULL) == -1)
		err(1, "pledge");

	init_output();

	procinfo = info[0];

	datetime_main(info[0]);

	return output_main(info[0]);
}
