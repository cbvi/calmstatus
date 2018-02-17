#ifndef _CALMSTATUS_H
#define _CALMSTATUS_H

#include "priv.h"

#define MAX_TITLE_LENGTH 128

typedef struct {
	struct imsgbuf *output;
	struct imsgbuf *volume;
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
void *watch_for_datetime_changes(void *);

void *volume_watch_for_changes(void *);
int volume_level(struct imsgbuf *);
int volume_mute(struct imsgbuf *);
int volume_main(procinfo_t *);

void init_output(void);
void do_output(procinfo_t *);
int output_main(procinfo_t *);

void destroy_procinfo(procinfo_t *);

#endif
