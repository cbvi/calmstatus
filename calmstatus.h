#ifndef _TYPES
#define _TYPES

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

void print_workspaces(xinfo_t *);
void print_title(xinfo_t *);

void print_datetime(void);
void *watch_for_datetime_changes(void *);

void print_volume(soundinfo_t *);
void *watch_for_volume_changes(void *);
soundinfo_t *get_soundinfo(void);
void destroy_soundinfo(soundinfo_t *);

void init_output(void);
void do_output(info_t *);

#endif
