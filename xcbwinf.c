#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_aux.h>

int
main()
{
	xcb_connection_t *conn;
	xcb_screen_t *scr;
	xcb_intern_atom_cookie_t atcook;
	xcb_intern_atom_reply_t *rep;

	xcb_get_property_cookie_t procook;
	xcb_get_property_reply_t *prorep;

	int *l;

	/* const char ncd_name[] = "_NET_CURRENT_DESKTOP"; */
	const char ncd_name[] = "_NET_CURRENT_DESKTOP";

	if ((conn = xcb_connect(NULL, NULL)) == NULL)
		err(1, "xcb_connect");
	scr = xcb_aux_get_screen(conn, 0);

	atcook = xcb_intern_atom(conn, 1, strlen(ncd_name), ncd_name);
	if ((rep = xcb_intern_atom_reply(conn, atcook, NULL)) == 0)
		err(1, "xcb_intern_atom_reply");

	procook = xcb_get_property(conn, 0, scr->root,
			rep->atom, XCB_ATOM_CARDINAL, 0, 4096);
	if ((prorep = xcb_get_property_reply(conn, procook, NULL)) == NULL)
		err(1, "xcb_get_property_reply");

	l = (int *)xcb_get_property_value(prorep);

	printf("%i\n", *l);

	xcb_disconnect(conn);
}
