#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_aux.h>

#include "calmstatus.h"

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
	procinfo_t *procinfo;
} xinfo_t;

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

static volatile sig_atomic_t running = 1;

static void
signal_term(int sig)
{
	(void)sig;
	running = 0;
}

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

static xinfo_t *
get_xinfo(procinfo_t *info)
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

	xi->procinfo = info;

	return xi;
}

static void
destroy_xinfo(xinfo_t *xi)
{
	xcb_disconnect(xi->conn);
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
	} else {
		((char *)res->value)[res->len] = '\0';
		strlcpy(buf, (char *)res->value, sz);
	}

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

static void
getworkspacewindowcounts(xinfo_t *xi, uint32_t *count)
{
	uint32_t listc, *list;
	uint32_t i;

	listc = getactiveworkspaces(xi, &list);

	memset(count, 0, 10 * sizeof(uint32_t));

	for (i = 0; i < listc; i++) {
		count[list[i]]++;
	}

	free(list);
}

static void
getcurrentwindowtitle(xinfo_t *xi, char *buf)
{
	xcb_window_t win;

	win = getcurrentwindow(xi);

	getwindowtitle(xi, win, buf, MAX_TITLE_LENGTH);
}

void
print_workspaces(uint32_t *counts, uint32_t current)
{
	uint32_t i;

	for (i = 1; i < 10; i++) {
		if (i == current) {
			printf("!");
		} else if (counts[i] > 0) {
			printf("+");
		} else {
			printf("-");
		}
		printf(" ");
	}
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

static void
xstuff_signal_output(procinfo_t *info)
{
	priv_send_cmd(info->output, CMD_OUTPUT_DO);
}

static void
xstuff_goodbye_output(procinfo_t *info)
{
	priv_send_cmd(info->output, CMD_GOODBYE);
}

void *
watch_for_x_changes(void *arg)
{
	xcb_window_t curwin, prevwin;
	xinfo_t *xi = (xinfo_t *)arg;

	watch_win(xi, xi->root);

	curwin = prevwin = getcurrentwindow(xi);
	watch_win(xi, curwin);

	while (xi->conn != NULL && xcb_wait_for_event(xi->conn) != NULL) {
		prevwin = curwin;
		curwin = getcurrentwindow(xi);
		if (curwin != prevwin) {
			watch_win(xi, curwin);
			unwatch_win(xi, prevwin);
		}
		if (running)
			xstuff_signal_output(xi->procinfo);
		else {
			xstuff_goodbye_output(xi->procinfo);
			break;
		}
	}
	return NULL;
}

void
xstuff_windowtitle(struct imsgbuf *ibuf, char *buf, size_t sz)
{
	struct imsg imsg;

	priv_send_cmd(ibuf, CMD_DESKTOP_TITLE);
	priv_get_res(ibuf, &imsg);

	if ((imsg.hdr.len - IMSG_HEADER_SIZE) != sz)
		err(1, "xstuff_windowtitle: msg wrong size");

	memcpy(buf, (char *)imsg.data, sz);
	imsg_free(&imsg);
}

uint32_t
xstuff_currentdesktop(struct imsgbuf *ibuf)
{
	struct imsg imsg;
	uint32_t res;

	priv_send_cmd(ibuf, CMD_DESKTOP_CURRENT);
	priv_get_res(ibuf, &imsg);

	if ((imsg.hdr.len - IMSG_HEADER_SIZE) != sizeof(uint32_t))
		err(1, "xstuff_currentdesktop: wrong msg size");

	res = *(uint32_t *)imsg.data;
	imsg_free(&imsg);

	return res;
}

void
xstuff_activeworkspaces(struct imsgbuf *ibuf, uint32_t *list)
{
	struct imsg imsg;

	priv_send_cmd(ibuf, CMD_DESKTOP_WINDOWS);
	priv_get_res(ibuf, &imsg);

	if ((imsg.hdr.len - IMSG_HEADER_SIZE) != (10 * sizeof(uint32_t)))
		err(1, "xstuff_activeworkspaces: wrong msg size");

	memcpy(list, (uint32_t *)imsg.data, 10 * sizeof(uint32_t));
	imsg_free(&imsg);
}

int
xstuff_main(procinfo_t *info)
{
	xinfo_t *xi;
	enum priv_cmd cmd;
	pthread_t thr;
	uint32_t cur, win[10];
	char title[MAX_TITLE_LENGTH];
	int ret = 1;

	setproctitle("xstuff");

	signal(SIGTERM, signal_term);

	xi = get_xinfo(info);

	pthread_create(&thr, NULL, watch_for_x_changes, xi);

	if (pledge("stdio", NULL) == -1)
		err(1, "pledge");

	while (running) {

		cmd = priv_get_cmd(info->xstuff);

		switch (cmd) {
		case CMD_DESKTOP_CURRENT:
			cur = getcurrentdesktop(xi);
			priv_send_res(info->xstuff, RES_DESKTOP_CURRENT,
			    &cur, sizeof(cur));
			break;
		case CMD_DESKTOP_WINDOWS:
			getworkspacewindowcounts(xi, win);
			priv_send_res(info->xstuff, RES_DESKTOP_WINDOWS,
			    win, 10 * sizeof(uint32_t));
			break;
		case CMD_DESKTOP_TITLE:
			getcurrentwindowtitle(xi, title);
			priv_send_res(info->xstuff, RES_DESKTOP_TITLE,
			    title, sizeof(title));
			break;
		case CMD_STOP_RIGHT_NOW:
			running = 0;
			ret = 0;
			break;
		default:
			warnx("xstuff_main: invalid cmd");
			running = 0;
			ret = 1;
			break;
		}
	}
	xstuff_goodbye_output(info);

	destroy_xinfo(xi);
	pthread_join(thr, NULL);
	destroy_procinfo(info);

	return ret;
}
