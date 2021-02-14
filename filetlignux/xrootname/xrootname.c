/* See LICENSE file for copyright and license details.
 *
 * Reads lines from stdin and, for each line, updates the root window name.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#define MAX_LINE 256

static Display *dpy;
static Window root;
static char line[MAX_LINE];
static int screen;

void
run()
{
	int len = 0;
	while (fgets((char *)line, MAX_LINE, stdin)) {
		len = strlen(line);
		if (len > 0 && line[len - 1] == '\n')
			line[len - 1] = '\0';
		XStoreName(dpy, root, (char *)line);
		XSync(dpy, False);
	}
}

int
main(int argc, char *argv[])
{
	if (argc != 1) {
		fprintf(stderr, "usage: xrootname\n\t(pipe lines to stdin)");
		exit(1);
	}

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwm: cannot open display");
		exit(1);
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);

	run();

	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
