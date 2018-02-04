#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

typedef struct {
	Display 	*dpy;
	Window  	 root;
} displayinfo;

void
getdisplayinfo(displayinfo **dinfo)
{
	Display 	*dpy;
	Window		 win;
	int		 scr;

	if ((*dinfo = calloc(1, sizeof(displayinfo))) == NULL) {
		err(1, "calloc");
	}

	if ((dpy = XOpenDisplay(NULL)) == NULL) {
		err(1, "XOpenDisplay");
	}

	scr = XDefaultScreen(dpy);
	win = RootWindow(dpy, scr);

	(*dinfo)->dpy = dpy;
	(*dinfo)->root = win;
}

void
destroydisplayinfo(displayinfo *dinfo)
{
	XCloseDisplay(dinfo->dpy);
	dinfo->dpy = NULL;
	free(dinfo);
}

Atom
getatom(displayinfo *dinfo, char *atomname)
{
	Display *dpy = dinfo->dpy;
	Atom a;

	if ((a = XInternAtom(dpy, atomname, True)) == None) {
		err(1, "XInternAtom %s", atomname);
	}

	return a;
}

int
getcurrentworkspace(displayinfo *dinfo)
{
	Display *dpy = dinfo->dpy;
	Window w = dinfo->root;

	Atom a;
	Atom typeret;
	int fmtret;
	unsigned long bytesafter, numret;
	unsigned long *d;

	a = getatom(dinfo, "_NET_CURRENT_DESKTOP");

	XGetWindowProperty(dpy, w, a, 0, 0x7fffffff, False,
			XA_CARDINAL, &typeret, &fmtret, &numret,
			&bytesafter, (unsigned char **)&d);

	if (numret == 0 || typeret != XA_CARDINAL) {
		err(1, "XGetWindowProperty _NET_CURRENT_DESKTOP");
	}

	printf("%lu\n", *d);

	XFree(d);

	return 0;
}

Window *
getwindowlist(displayinfo *dinfo, unsigned long *szp)
{
	Display *dpy = dinfo->dpy;
	Window w = dinfo->root;

	Atom a;
	Atom typeret;
	int fmtret;
	unsigned long bytesafter;
	Window *d;

	a = getatom(dinfo, "_NET_CLIENT_LIST");

	XGetWindowProperty(dpy, w, a, 0, 0x7fffffff, False,
			XA_WINDOW, &typeret, &fmtret, szp, 
			&bytesafter, (unsigned char **)&d);

	if (*szp == 0) {
		err(1, "XGetWindowProperty _NET_CLIENT_LIST");
	}

	return d;
}


unsigned long *
getworkspaces(displayinfo *dinfo, Window *list, unsigned long sz)
{
	Display *dpy = dinfo->dpy;

	unsigned long l;
	unsigned long *active;

	Atom a;
	Atom typeret;
	int fmtret;
	unsigned long bytesafter, numret;
	unsigned long *id;

	a = getatom(dinfo, "_NET_WM_DESKTOP");

	if ((active = calloc(sz, sizeof(unsigned long))) == NULL) {
		err(1, "calloc");
	}

	for (l = 0; l < sz; l++) {
		XGetWindowProperty(dpy, list[l], a, 0, 0x7fffffff, False,
				XA_CARDINAL, &typeret, &fmtret, &numret,
				&bytesafter, (unsigned char **)&id);
		active[l] = *id;
		XFree(id);
	}

	return active;
}

int
main()
{
	Window *list;
	displayinfo *dinfo;
	unsigned long *spaces;
	unsigned long count;
	unsigned long l;

	getdisplayinfo(&dinfo);
	getcurrentworkspace(dinfo);

	list = getwindowlist(dinfo, &count);
	spaces = getworkspaces(dinfo, list, count);

	for (l = 0; l < count; l++) {
		printf("%lu\n", spaces[l]);
	}

	destroydisplayinfo(dinfo);
	free(spaces);
	XFree(list);

	return 0;
}
