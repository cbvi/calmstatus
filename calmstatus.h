#ifndef _CALMSTATUS_H
#define _CALMSTATUS_H

#include <xcb/xcb.h>

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
} info_t;

void *xcalloc(size_t, size_t);

xinfo_t *get_xinfo(void);
void print_workspaces(xinfo_t *);
void print_title(xinfo_t *);
void *watch_for_x_changes(void *);

void print_datetime(void);
void *watch_for_datetime_changes(void *);

soundinfo_t *volume_get_soundinfo(void);
void volume_print_volume(soundinfo_t *);
void *volume_watch_for_changes(void *);
void volume_destroy_soundinfo(soundinfo_t *);

void init_output(void);
void do_output(info_t *);

#endif
