#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

void * xcalloc(size_t, size_t);

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
		errx(1, "xcb_intern_atom_reply");

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

	res = xcalloc(1, sizeof(propres_t));

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

void *
xcalloc(size_t n, size_t sz)
{
	void *p;

	if ((p = calloc(n, sz)) == NULL) {
		warnx("calloc failed allocating %lu * %lu", n, sz);
		abort();
	}
	return p;
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

	*wlist = xcalloc(res->len, sizeof(xcb_window_t));

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

int
does_workspace_have_window(uint32_t id, uint32_t *list, uint32_t sz)
{
	uint32_t i;

	for (i = 0; i < sz; i++) {
		if (list[i] == id)
			return 1;
	}
	return 0;
}

void left() 	{ printf("%s", "%{l}"); }
void right()	{ printf("%s", "%{r}"); }

void
print_datetime()
{
	time_t clock;
	struct tm *tp;
	char buf[32];

	if ((clock = time(NULL)) == -1)
		err(1, "time");

	if ((tp = localtime(&clock)) == NULL)
		err(1, "localtime");

	if (strftime(buf, sizeof(buf), "%a %d %b %H:%M", tp) == 0)
		errx(1, "strftime");

	printf("%s", buf);
}

void
print_workspaces(uint32_t cur, uint32_t *list, uint32_t sz)
{
	uint32_t i;

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
}

int
main()
{
	xinfo_t *xi;
	uint32_t cur;
	uint32_t sz;
	uint32_t *wl;

	xi = get_xinfo();

	for (;;) {
		cur = getcurrentdesktop(xi);
		sz = getactiveworkspaces(xi, &wl);

		left();
		print_workspaces(cur, wl, sz);

		right();
		print_datetime();
		printf("%s", "  ");

		printf("\n");
		fflush(stdout);

		free(wl);
		sleep(1);
	}

	destroy_xinfo(xi);
}
