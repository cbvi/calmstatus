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

int
getcurrentdesktop(xinfo_t *xi)
{
	xcb_atom_t atom;
	xcb_get_property_cookie_t procook;
	xcb_get_property_reply_t *prorep;

	int *l;

	atom = get_atom(xi, "_NET_CURRENT_DESKTOP");

	procook = xcb_get_property(xi->conn, 0, xi->root,
			atom, XCB_ATOM_CARDINAL, 0, 4096);
	if ((prorep = xcb_get_property_reply(xi->conn, procook, NULL)) == NULL)
		err(1, "xcb_get_property_reply");

	if (prorep->length == 0)
		err(1, "xcb_get_property_reply length = 0");

	l = (int *)xcb_get_property_value(prorep);

	printf("%i\n", *l);

	return 0;
}

xcb_window_t *
getwindowlist(xinfo_t *xi, uint32_t *sz)
{
	xcb_atom_t atom;
	xcb_get_property_cookie_t procook;
	xcb_get_property_reply_t *prorep;

	xcb_window_t *list;
	uint32_t i;

	atom = get_atom(xi, "_NET_CLIENT_LIST");

	procook = xcb_get_property(xi->conn, 0, xi->root,
			atom, XCB_ATOM_WINDOW, 0, 4096);
	if ((prorep = xcb_get_property_reply(xi->conn, procook, NULL)) == NULL)
		err(1, "xcb_get_property_reply");

	list = (xcb_window_t *)xcb_get_property_value(prorep);

	for (i = 0; i < prorep->length; i++) {
		printf("%i\n", list[i]);
	}

	*sz = prorep->length;

	return list;
}

int
getactiveworkspaces(xinfo_t *xi)
{
	xcb_atom_t atom;
	xcb_get_property_cookie_t procook;
	xcb_get_property_reply_t *prorep;

	xcb_window_t *list;
	uint32_t sz;
	uint32_t i;
	int *ws;

	list = getwindowlist(xi, &sz);

	atom = get_atom(xi, "_NET_WM_DESKTOP");

	for (i = 0; i < sz; i++) {
		procook = xcb_get_property(xi->conn, 0, list[i],
				atom, XCB_ATOM_CARDINAL, 0, 4096);
		if ((prorep = xcb_get_property_reply(xi->conn, procook, NULL)) == NULL)
		err(1, "xcb_get_property_reply");

		if (prorep->length == 0)
			err(1, "xcb_get_property_reply length = 0");

		ws = (int *)xcb_get_property_value(prorep);

		printf("%i\n", *ws);
	}

	return 0;
}

int
main()
{
	xinfo_t *xi;

	xi = get_xinfo();

	getcurrentdesktop(xi);
	getactiveworkspaces(xi);

	destroy_xinfo(xi);
}
