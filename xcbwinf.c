#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_aux.h>

typedef struct {
	xcb_connection_t *conn;
	xcb_window_t root;
} xinfo_t;

typedef struct {
	xcb_window_t win;
	char *name;
	xcb_atom_t type;
} propreq_t;

typedef struct {
	void *value;
	xcb_get_property_reply_t *reply;
	uint32_t len;
} propres_t;

xinfo_t *
get_xinfo()
{
	xinfo_t *xi;
	xcb_connection_t *conn;
	xcb_screen_t *screen;

	if ((xi = calloc(1, sizeof(xinfo_t))) == NULL)
		err(1, "calloc");

	if ((conn = xcb_connect(NULL, NULL)) == NULL)
		err(1, "xcb_connect");

	if ((screen = xcb_aux_get_screen(conn, 0)) == NULL)
		err(1, "xcb_aux_get_screen");

	xi->conn = conn;
	xi->root = screen->root;

	return xi;
}

void
destroy_xinfo(xinfo_t *xi)
{
	free(xi->conn);
	xi->conn = NULL;
	free(xi);
}

xcb_atom_t
get_atom(xinfo_t *xi, const char *name)
{
	xcb_atom_t ret;
	xcb_intern_atom_cookie_t cook;
	xcb_intern_atom_reply_t *rep;

	cook = xcb_intern_atom(xi->conn, 1, strlen(name), name);
	if ((rep = xcb_intern_atom_reply(xi->conn, cook, NULL)) == NULL)
		err(1, "xcb_intern_atom_reply");

	ret = rep->atom;

	free(rep);

	return ret;
}

propres_t *
get_property(xinfo_t *xi, propreq_t *req)
{
	xcb_get_property_cookie_t cook;
	xcb_get_property_reply_t *rep;
	xcb_atom_t atom;
	propres_t *res;

	if ((res = calloc(1, sizeof(propres_t))) == NULL)
		err(1, "calloc");

	atom = get_atom(xi, req->name);

	cook = xcb_get_property(xi->conn, 0, req->win, atom, req->type, 0, 4096);
	if ((rep = xcb_get_property_reply(xi->conn, cook, NULL)) != NULL) {
		res->value = (void *)xcb_get_property_value(rep);
		res->len = rep->length;
		res->reply = rep;
	} else {
		res->value = NULL;
		res->len = 0;
		res->reply = NULL;
	}

	return res;
}

void
destroy_property(propres_t *res)
{
	free(res->reply);
	res->reply = NULL;
	res->value = NULL;
	res->len = 0;
	free(res);
}

uint32_t
getcurrentdesktop(xinfo_t *xi)
{
	uint32_t *l;
	uint32_t ret;
	propreq_t req;
	propres_t *res;

	req.win = xi->root;
	req.type = XCB_ATOM_CARDINAL;
	req.name = "_NET_CURRENT_DESKTOP";

	res = get_property(xi, &req);

	if (res->len == 0)
		errx(1, "current desktop returned 0 length");

	l = (uint32_t *)res->value;
	ret = *l;

	destroy_property(res);

	return ret;
}

uint32_t
getwindowlist(xinfo_t *xi, xcb_window_t **wlist)
{
	xcb_window_t *list;
	propreq_t req;
	propres_t *res;
	uint32_t i;
	uint32_t sz;

	req.win = xi->root;
	req.type = XCB_ATOM_WINDOW;
	req.name = "_NET_CLIENT_LIST";

	res = get_property(xi, &req);

	list = (xcb_window_t *)res->value;

	if ((*wlist = calloc(res->len, sizeof(xcb_window_t))) == NULL)
		err(1, "calloc");

	for (i = 0; i < res->len; i++) {
		(*wlist)[i] = list[i];
	}

	sz = res->len;

	destroy_property(res);
	return sz;
}

uint32_t
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
	req.name = "_NET_WM_DESKTOP";

	if ((*ret = calloc(sz, sizeof(uint32_t))) == NULL)
		err(1, "calloc");

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

int
main()
{
	xinfo_t *xi;
	uint32_t sz;
	uint32_t *wl;
	uint32_t i;

	xi = get_xinfo();

	printf("%i\n", getcurrentdesktop(xi));
	sz = getactiveworkspaces(xi, &wl);

	for (i = 0; i < sz; i++) {
		printf("%i\n", wl[i]);
	}

	free(wl);

	destroy_xinfo(xi);
}
