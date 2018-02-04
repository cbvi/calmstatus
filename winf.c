#include <stdio.h>
#include <err.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>


int
main()
{
	Display *dpy;
	Atom a;
	Window w;
	int screen;
	int status;

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

	status = XGetWindowProperty(dpy, w, a, 0, 0x7fffffff, False,
			XA_CARDINAL, &typeret, &fmtret, &numret,
			&bytesafter, &d);

	if (numret == 0 || typeret != XA_CARDINAL) {
		err(1, "XGetWindowProperty");
	}

	printf("%ld\n", (long)*d);

	XFree(d);
	XCloseDisplay(dpy);
}
