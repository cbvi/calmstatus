#include <stdio.h>
#include <err.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>


int
getcurrentworkspace()
{
	Display *dpy;
	Atom a;
	Window w;
	int screen;

	Atom typeret;
	int fmtret;
	unsigned long bytesafter, numret;
	unsigned char *d;

	if ((dpy = XOpenDisplay(":0")) == NULL) {
		err(1, "XOpenDisplay");
	}
	
	screen = XDefaultScreen(dpy);

	if ((a = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", True)) == None) {
		err(1, "XInternAtom");
	}

	w = RootWindow(dpy, screen);

	XGetWindowProperty(dpy, w, a, 0, 0x7fffffff, False,
			XA_CARDINAL, &typeret, &fmtret, &numret,
			&bytesafter, &d);

	if (numret == 0 || typeret != XA_CARDINAL) {
		err(1, "XGetWindowProperty");
	}

	printf("%ld\n", (long)*d);

	XFree(d);
	XCloseDisplay(dpy);
}

int
main()
{
	Display *dpy;
	Atom a;
	Window w;
	int screen;
	unsigned long i;

	Atom typeret;
	int fmtret;
	unsigned long bytesafter, numret;
	Window *d;

	if ((dpy = XOpenDisplay(":0")) == NULL) {
		err(1, "XOpenDisplay");
	}

	screen = XDefaultScreen(dpy);

	if ((a = XInternAtom(dpy, "_NET_CLIENT_LIST", True)) == None) {
		err(1, "XInternAtom");
	}

	w = RootWindow(dpy, screen);

	XGetWindowProperty(dpy, w, a, 0, 0x7fffffff, False,
			XA_WINDOW, &typeret, &fmtret, &numret,
			&bytesafter, (unsigned char **)&d);

	if (numret == 0) {
		err(1, "blah");
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
