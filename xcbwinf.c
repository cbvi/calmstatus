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

int
getcurrentdesktop()
{
	xinfo_t *xi;
	xcb_intern_atom_cookie_t atcook;
	xcb_intern_atom_reply_t *rep;

	xcb_get_property_cookie_t procook;
	xcb_get_property_reply_t *prorep;

	int *l;

	const char ncd_name[] = "_NET_CURRENT_DESKTOP";

	xi = get_xinfo();

	atcook = xcb_intern_atom(xi->conn, 1, strlen(ncd_name), ncd_name);
	if ((rep = xcb_intern_atom_reply(xi->conn, atcook, NULL)) == 0)
		err(1, "xcb_intern_atom_reply");

	procook = xcb_get_property(xi->conn, 0, xi->root,
			rep->atom, XCB_ATOM_CARDINAL, 0, 4096);
	if ((prorep = xcb_get_property_reply(xi->conn, procook, NULL)) == NULL)
		err(1, "xcb_get_property_reply");

	if (prorep->length == 0)
		err(1, "xcb_get_property_reply length = 0");

	l = (int *)xcb_get_property_value(prorep);

	printf("%i\n", *l);

	destroy_xinfo(xi);

	return 0;
}

xcb_window_t *
getwindowlist(uint32_t *sz)
{
	xinfo_t *xi;
	xcb_intern_atom_cookie_t atcook;
	xcb_intern_atom_reply_t *rep;

	xcb_get_property_cookie_t procook;
	xcb_get_property_reply_t *prorep;

	xcb_window_t *list;
	uint32_t i;

	const char ncd_name[] = "_NET_CLIENT_LIST";

	xi = get_xinfo();

	atcook = xcb_intern_atom(xi->conn, 1, strlen(ncd_name), ncd_name);
	if ((rep = xcb_intern_atom_reply(xi->conn, atcook, NULL)) == 0)
		err(1, "xcb_intern_atom_reply");

	procook = xcb_get_property(xi->conn, 0, xi->root,
			rep->atom, XCB_ATOM_WINDOW, 0, 4096);
	if ((prorep = xcb_get_property_reply(xi->conn, procook, NULL)) == NULL)
		err(1, "xcb_get_property_reply");

	list = (xcb_window_t *)xcb_get_property_value(prorep);

	for (i = 0; i < prorep->length; i++) {
		printf("%i\n", list[i]);
	}

	*sz = prorep->length;

	destroy_xinfo(xi);

	return list;
}

int
getactiveworkspaces()
{
	xinfo_t *xi;
	xcb_intern_atom_cookie_t atcook;
	xcb_intern_atom_reply_t *rep;

	xcb_get_property_cookie_t procook;
	xcb_get_property_reply_t *prorep;

	xcb_window_t *list;
	uint32_t sz;
	uint32_t i;
	int *ws;

	const char ncd_name[] = "_NET_WM_DESKTOP";

	xi = get_xinfo();

	list = getwindowlist(&sz);

	atcook = xcb_intern_atom(xi->conn, 1, strlen(ncd_name), ncd_name);
	if ((rep = xcb_intern_atom_reply(xi->conn, atcook, NULL)) == 0)
		err(1, "xcb_intern_atom_reply");

	for (i = 0; i < sz; i++) {
		procook = xcb_get_property(xi->conn, 0, list[i],
				rep->atom, XCB_ATOM_CARDINAL, 0, 4096);
		if ((prorep = xcb_get_property_reply(xi->conn, procook, NULL)) == NULL)
		err(1, "xcb_get_property_reply");

		if (prorep->length == 0)
			err(1, "xcb_get_property_reply length = 0");

		ws = (int *)xcb_get_property_value(prorep);

		printf("%i\n", *ws);
	}

	destroy_xinfo(xi);

	return 0;
}

int
main()
{
	getcurrentdesktop();
	getactiveworkspaces();
}
