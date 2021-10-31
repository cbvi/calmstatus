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
#ifndef _CALMSTATUS_H
#define _CALMSTATUS_H

#include "priv.h"

#define MAX_TITLE_LENGTH 128

typedef struct {
	struct imsgbuf *output;
	//struct imsgbuf *volume;
	struct imsgbuf *xstuff;
} procinfo_t;

void *xcalloc(size_t, size_t);

void print_workspaces(uint32_t *, uint32_t);
void *watch_for_x_changes(void *);
uint32_t xstuff_currentdesktop(struct imsgbuf *);
void xstuff_windowtitle(struct imsgbuf *, char *, size_t);
void xstuff_activeworkspaces(struct imsgbuf *, uint32_t *);
int xstuff_main(procinfo_t *);

void print_datetime(void);
int datetime_main(procinfo_t *);

/*void *volume_watch_for_changes(void *);
int volume_level(struct imsgbuf *);
int volume_mute(struct imsgbuf *);
int volume_main(procinfo_t *);*/

void init_output(void);
void do_output(procinfo_t *);
int output_main(procinfo_t *);

void destroy_procinfo(procinfo_t *);

#endif
