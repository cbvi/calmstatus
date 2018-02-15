#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_aux.h>

#include "calmstatus.h"

#define MAX_TITLE_LENGTH 128

typedef struct {
	xcb_window_t win;
	atoms_available_t name;
	xcb_atom_t type;
} propreq_t;

typedef struct {
	void *value;
	xcb_get_property_reply_t *reply;
	uint32_t len;
} propres_t;

static xcb_atom_t
get_atom(xinfo_t *xi, const char *name)
{
	xcb_atom_t ret;
	xcb_intern_atom_cookie_t cook;
	xcb_intern_atom_reply_t *rep;

	cook = xcb_intern_atom(xi->conn, 1, strlen(name), name);
	if ((rep = xcb_intern_atom_reply(xi->conn, cook, NULL)) == NULL)
		errx(1, "xcb_intern_atom_reply");

	ret = rep->atom;

	free(rep);

	return ret;
}

xinfo_t *
get_xinfo()
{
	xinfo_t *xi;
	xcb_connection_t *conn;
	xcb_screen_t *screen;

	xi = xcalloc(1, sizeof(xinfo_t));

	if ((conn = xcb_connect(NULL, NULL)) == NULL)
		err(1, "xcb_connect");

	if ((screen = xcb_aux_get_screen(conn, 0)) == NULL)
		errx(1, "xcb_aux_get_screen");

	xi->conn = conn;
	xi->root = screen->root;

	xi->atoms[CURRENT_DESKTOP] = get_atom(xi, "_NET_CURRENT_DESKTOP");
	xi->atoms[WINDOW_LIST] = get_atom(xi, "_NET_CLIENT_LIST");
	xi->atoms[WINDOW_DESKTOP] = get_atom(xi, "_NET_WM_DESKTOP");
	xi->atoms[CURRENT_WINDOW] = get_atom(xi, "_NET_ACTIVE_WINDOW");
	xi->atoms[WINDOW_NAME] = get_atom(xi, "WM_NAME");

	return xi;
}

static void
destroy_xinfo(xinfo_t *xi)
{
	free(xi->conn);
	xi->conn = NULL;
	free(xi);
}

static propres_t *
get_property(xinfo_t *xi, propreq_t *req)
{
	xcb_get_property_cookie_t cook;
	xcb_get_property_reply_t *rep;
	xcb_atom_t atom;
	propres_t *res;

	res = xcalloc(1, sizeof(propres_t));

	atom = xi->atoms[req->name];

	cook = xcb_get_property(xi->conn, 0, req->win, atom, req->type, 0, 4096);
	if ((rep = xcb_get_property_reply(xi->conn, cook, NULL)) != NULL) {
		res->value = (void *)xcb_get_property_value(rep);
		res->len = rep->value_len;
		res->reply = rep;
	} else {
		res->value = NULL;
		res->len = 0;
		res->reply = NULL;
	}

	return res;
}

static void
destroy_property(propres_t *res)
{
	free(res->reply);
	res->reply = NULL;
	res->value = NULL;
	res->len = 0;
	free(res);
}

static xcb_window_t
getcurrentwindow(xinfo_t *xi)
{
	propreq_t req;
	propres_t *res;
	xcb_window_t ret;

	req.win = xi->root;
	req.type = XCB_ATOM_WINDOW;
	req.name = CURRENT_WINDOW;

	res = get_property(xi, &req);

	if (res->len == 0)
		errx(1, "couldn't get current window");

	ret = *(xcb_window_t *)res->value;

	destroy_property(res);

	return ret;
}

static uint32_t
getwindowtitle(xinfo_t *xi, xcb_window_t win, char *buf, uint32_t sz)
{
	uint32_t ret;
	propreq_t req;
	propres_t *res;

	req.win = win;
	req.type = XCB_ATOM_STRING;
	req.name = WINDOW_NAME;

	res = get_property(xi, &req);

	if (res->len == 0) {
		strlcpy(buf, "", sz);
		goto end;
	}

	((char *)res->value)[res->len] = '\0';
	strlcpy(buf, (char *)res->value, sz);

end:
	ret = res->len;
	destroy_property(res);
	return ret;
}

static uint32_t
getcurrentdesktop(xinfo_t *xi)
{
	uint32_t ret;
	propreq_t req;
	propres_t *res;

	req.win = xi->root;
	req.type = XCB_ATOM_CARDINAL;
	req.name = CURRENT_DESKTOP;

	res = get_property(xi, &req);

	if (res->len == 0)
		errx(1, "current desktop returned 0 length");

	ret = *(uint32_t *)res->value;

	destroy_property(res);

	return ret;
}

static uint32_t
getwindowlist(xinfo_t *xi, xcb_window_t **wlist)
{
	xcb_window_t *list;
	propreq_t req;
	propres_t *res;
	uint32_t i;
	uint32_t sz;

	req.win = xi->root;
	req.type = XCB_ATOM_WINDOW;
	req.name = WINDOW_LIST;

	res = get_property(xi, &req);

	list = (xcb_window_t *)res->value;

	*wlist = xcalloc(res->len, sizeof(xcb_window_t));

	for (i = 0; i < res->len; i++) {
		(*wlist)[i] = list[i];
	}

	sz = res->len;

	destroy_property(res);
	return sz;
}

static uint32_t
getactiveworkspaces(xinfo_t *xi, uint32_t **ret)
{
	xcb_window_t *list;
	propreq_t req;
	propres_t *res;
	uint32_t sz;
	uint32_t i;
	uint32_t *ws;
	uint32_t realcount;

	sz = getwindowlist(xi, &list);

	req.type = XCB_ATOM_CARDINAL;
	req.name = WINDOW_DESKTOP;

	*ret = xcalloc(sz, sizeof(uint32_t));

	realcount = 0;

	for (i = 0; i < sz; i++) {
		req.win = list[i];
		res = get_property(xi, &req);

		if (res->len > 0) {
			ws = (uint32_t *)res->value;
			(*ret)[i] = *ws;
			realcount++;
		}
		destroy_property(res);
	}

	free(list);

	return realcount;
}

static int
does_workspace_have_window(uint32_t id, uint32_t *list, uint32_t sz)
{
	uint32_t i;

	for (i = 0; i < sz; i++) {
		if (list[i] == id)
			return 1;
	}
	return 0;
}

void
print_workspaces(xinfo_t *xi)
{
	uint32_t i, cur, sz, *list;

	cur = getcurrentdesktop(xi);
	sz = getactiveworkspaces(xi, &list);

	for (i = 1; i <= 9; i++) {
		if (i == cur) {
			printf("!");
		} else if (does_workspace_have_window(i, list, sz)) {
			printf("+");
		} else {
			printf("-");
		}
		printf(" ");
	}

	free(list);
}

void
print_title(xinfo_t *xi)
{
	char title[MAX_TITLE_LENGTH];
	xcb_window_t win;

	win = getcurrentwindow(xi);

	getwindowtitle(xi, win, title, sizeof(title));

	printf("%s", title);
}

static void
watch_win(xinfo_t *xi, xcb_window_t win)
{
	uint32_t values;

	values = XCB_EVENT_MASK_PROPERTY_CHANGE;
	xcb_change_window_attributes(xi->conn, win,
	    XCB_CW_EVENT_MASK, &values);
}

static void
unwatch_win(xinfo_t *xi, xcb_window_t win)
{
	uint32_t values;

	values = XCB_EVENT_MASK_NO_EVENT;
	xcb_change_window_attributes(xi->conn, win,
	    XCB_CW_EVENT_MASK, &values);
}

void *
watch_for_x_changes(void *arg)
{
	xcb_generic_event_t *ev;
	xcb_window_t curwin, prevwin;
	info_t *info = (info_t *)arg;
	xinfo_t *xi = info->xinfo;

	watch_win(xi, xi->root);

	curwin = prevwin = getcurrentwindow(xi);
	watch_win(xi, curwin);

	while ((ev = xcb_wait_for_event(xi->conn)) != NULL) {
		prevwin = curwin;
		curwin = getcurrentwindow(xi);
		if (curwin != prevwin) {
			watch_win(xi, curwin);
			unwatch_win(xi, prevwin);
		}
		do_output(info);
	}
	return NULL;
}

void
destroy_info(info_t *info)
{
	destroy_xinfo(info->xinfo);
	volume_destroy_soundinfo(info->soundinfo);
	free(info);
}
