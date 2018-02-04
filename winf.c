#include <stdio.h>
#include <err.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

typedef struct {
	Display 	*dpy;
	Window  	 root;
} displayinfo;

void
getdisplayinfo(displayinfo *dinfo)
{
	Display 	*dpy;
	Window		 win;
	int		 scr;

	if ((dpy = XOpenDisplay(NULL)) == NULL) {
		err(1, "XOpenDisplay");
	}

	scr = XDefaultScreen(dpy);
	win = RootWindow(dpy, scr);

	dinfo->dpy = dpy;
	dinfo->root = win;
}

void
destroydisplayinfo(displayinfo *dinfo)
{
	XCloseDisplay(dinfo->dpy);
	dinfo->dpy = NULL;
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

	if ((a = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", True)) == None) {
		err(1, "XInternAtom");
	}

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

int
windowlist(displayinfo *dinfo)
{
	Display *dpy = dinfo->dpy;
	Window w = dinfo->root;

	unsigned long i;

	Atom a;
	Atom typeret;
	int fmtret;
	unsigned long bytesafter, numret;
	Window *d;

	if ((a = XInternAtom(dpy, "_NET_CLIENT_LIST", True)) == None) {
		err(1, "XInternAtom");
	}

	XGetWindowProperty(dpy, w, a, 0, 0x7fffffff, False,
			XA_WINDOW, &typeret, &fmtret, &numret,
			&bytesafter, (unsigned char **)&d);

	if (numret == 0) {
		err(1, "XGetWindowProperty _NET_CLIENT_LIST");
	}

	if ((a = XInternAtom(dpy, "_NET_WM_DESKTOP", True)) == None) {
		err(1, "XInternAtom");
	}

	for (i = 0; i < numret; i++) {
		unsigned long *l;
		Atom wtyperet;
		int wfmtret;
		unsigned long wbytesafter, wnumret;

		XGetWindowProperty(dpy, d[i], a, 0, 0x7fffffff, False,
				XA_CARDINAL, &wtyperet, &wfmtret, &wnumret,
				&wbytesafter, (unsigned char **)&l);
		printf("%lu\n", *l);
		XFree(l);
	}

	return 0;
}

int
main()
{
	displayinfo dinfo;

	getdisplayinfo(&dinfo);
	getcurrentworkspace(&dinfo);

	windowlist(&dinfo);

	destroydisplayinfo(&dinfo);

	return 0;
}
