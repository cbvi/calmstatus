#ifndef _CALMSTATUS_H
#define _CALMSTATUS_H

#include <xcb/xcb.h>

#include "priv.h"

#define MAX_TITLE_LENGTH 128

typedef struct {
	struct imsgbuf *volume;
	struct imsgbuf *xstuff;
} procinfo_t;

typedef enum {
	CURRENT_DESKTOP,
	WINDOW_LIST,
	WINDOW_DESKTOP,
	CURRENT_WINDOW,
	WINDOW_NAME,
	ATOMS_AVAILABLE_MAX
} atoms_available_t;

typedef struct {
	xcb_connection_t *conn;
	xcb_window_t root;
	xcb_atom_t atoms[ATOMS_AVAILABLE_MAX];
} xinfo_t;

typedef struct {
	int mixer;
	int master;
	int mute;
} soundinfo_t;

typedef struct {
	xinfo_t *xinfo;
	soundinfo_t *soundinfo;
	procinfo_t *procinfo;
} info_t;

void *xcalloc(size_t, size_t);

xinfo_t *get_xinfo(void);
void destroy_xinfo(xinfo_t *);
void print_workspaces(uint32_t *, uint32_t);
void print_title(xinfo_t *);
void *watch_for_x_changes(void *);
uint32_t xstuff_currentdesktop(struct imsgbuf *);
void xstuff_windowtitle(struct imsgbuf *, char *, size_t);
void xstuff_activeworkspaces(struct imsgbuf *, uint32_t *);
int xstuff_main(procinfo_t *);

void print_datetime(void);
void *watch_for_datetime_changes(void *);

soundinfo_t *volume_get_soundinfo(void);
void volume_print_volume(soundinfo_t *);
void *volume_watch_for_changes(void *);
void volume_destroy_soundinfo(soundinfo_t *);
int volume_level(struct imsgbuf *);
int volume_mute(struct imsgbuf *);
int volume_main(procinfo_t *);

void init_output(void);
void do_output(info_t *);

void destroy_procinfo(procinfo_t *);

#endif
