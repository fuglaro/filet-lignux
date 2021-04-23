/* See LICENSE file for copyright and license details.
 *
 * This is a minimal fork to dwm, aiming to be smaller, simpler
 * and friendlier.
 *
 * Filet-Lignux's dynamic window manager is designed like any other X client.
 * It is driven through handling X events. In contrast to other X clients,
 * a window manager selects for SubstructureRedirectMask on the root window,
 * to receive events about window (dis-)appearance. Only one X connection at a
 * time is allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list. Each client contains a bit array to indicate the tags (workspaces)
 * of a client.
 *
 * Keyboard shortcuts are organized as arrays.
 *
 * Mouse motion tracking governs window focus, along with
 * a click-to-raise behavior. Mouse motion is stateful and supports different
 * drag-modes for moving and resizing windows.
 *
 * To understand everything else, start reading main().
 */

#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

/* basic macros */
#define LOADCONF(P,C) ((*(void **)(&C)) = dlsym(dlopen(P, RTLD_LAZY), "config"))
                     /* leave the loaded lib in memory until process cleanup */
#define KEYMASK(mask) (mask & (ShiftMask|ControlMask|Mod1Mask|Mod4Mask))
#define KCODE(keysym) ((KeyCode)(XKeysymToKeycode(dpy, keysym)))
#define ISVISIBLE(C) ((C->tags & tagset))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MOUSEINF(W,X,Y,M) (XQueryPointer(dpy,root,&dwin,&W,&X,&Y,&di,&di,&M))
#define PROPEDIT(P, C, A) {XChangeProperty(dpy, root, netatom[A], XA_WINDOW,\
	32, P, (unsigned char *) &(C->win), 1);}
#define TEXTPAD (xfont->ascent + xfont->descent) /* side padding of text */
#define TEXTW(X) (drawgettextwidth(X) + TEXTPAD)

/* edge dragging and region macros*/
#define BARZONE(X, Y) (topbar ? Y <= mons->my : Y >= mons->my + mons->mh - 1\
	&& (X >= mons->mx) && (X <= mons->mx + mons->mw))
#define INZONE(C, X, Y) (X >= C->x - C->bw && Y >= C->y - C->bw\
	&& X <= C->x + WIDTH(C) + C->bw && Y <= C->y + HEIGHT(C) + C->bw)
#define MOVEZONE(C, X, Y) (INZONE(C, X, Y)\
	&& (abs(C->x - X) <= C->bw || abs(C->y - Y) <= C->bw))
#define RESIZEZONE(C, X, Y) (INZONE(C, X, Y)\
	&& (abs(C->x + WIDTH(C) - X) <= C->bw || abs(C->y + HEIGHT(C) - Y) <= C->bw))

/* monitor macros */
#define BARH (TEXTPAD + 2)
#define BARY (topbar ? mons->my : mons->my + WINH(mons[0]))
#define INMON(X, Y, M)\
	(X >= M.mx && X < M.mx + M.mw && Y >= M.my && Y < M.my + M.mh)
#define MONNULL(M) (M.mx == 0 && M.my == 0 && M.mw == 0 && M.mh == 0)
#define SETMON(M, R) {M.mx = R.x; M.my = R.y; M.mw = R.width; M.mh = R.height;}
#define WINH(M) (&M == mons ? M.mh - BARH : M.mh)
#define WINY(M) (&M == mons && topbar ? M.my + BARH : M.my)
#define ONMON(C, M) INMON(C->x + WIDTH(C)/2, C->y + HEIGHT(C)/2, M)

/* window macros */
#define HEIGHT(X) ((X)->h + 2 * (X)->bw)
#define WIDTH(X) ((X)->w + 2 * (X)->bw)

/* virtual desktop macros */
#define TAGMASK ((1 << tagslen) - 1)
#define TAGSHIFT(TAGS, I) (I < 0 ? (TAGS >> -I) | (TAGS << (tagslen + I))\
	: (TAGS << I) | (TAGS >> (tagslen - I)))

/* enums */
enum { fg, bg, mark, bdr, selbdr, colslen }; /* colors */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck, /* EWMH atoms */
       NetWMFullscreen, NetActiveWindow, NetWMWindowType,
       NetWMWindowTypeDialog, NetClientList, NetCliStack, NetLast };
/* default atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast };
/* bar click regions */
enum { ClkLauncher, ClkWinTitle, ClkStatus, ClkTagBar, ClkLast };
/* mouse motion modes */
enum { DragMove, DragSize, DragTile, DragCheck, DragNone };
/* window stack actions */
enum { CliPin, CliRaise, CliZoom, CliRemove, CliRefresh };

/* argument template for keyboard shortcut and bar click actions */
typedef union {
	int i;
	unsigned int ui;
	const void *v;
} Arg;

/* bar click action */
typedef struct {
	unsigned int click;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

/* clients wrap managed windows */
typedef struct Client Client;
struct Client {
	char name[256], zenname[256];
	float mina, maxa;
	int x, y, w, h;
	int fx, fy, fw, fh; /*remember during tiled and fullscreen states */
	int basew, baseh, maxw, maxh, minw, minh;
	int bw, fbw, oldbw;
	unsigned int tags;
	int isfloating, isurgent, fstate, isfullscreen;
	Client *next;
	Window win;
	Time zenping;
};

/* keyboard shortcut action */
typedef struct {
	unsigned int mod;
	KeySym key;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

/* A monitor could be a connected display.
 * You can also have multiple monitors across displays if you
 * want custom windowing regions. */
typedef struct { int mx, my, mw, mh; } Monitor; /* windowing region size */

/* function declarations */
static void arrange(void);
static void attach(Client *c);
static void buttonpress(XEvent *e);
static void buttonrelease(XEvent *e);
static void cleanup(void);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurerequest(XEvent *e);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void drawbar(int zen);
static int drawgettextwidth(const char *text);
static void drawtext(int x, int y, int w, int h, const char *text,
	const XftColor *fg, const XftColor *bg);
static void die(const char *msg);
static void expose(XEvent *e);
static void exthandler(XEvent *ev);
static void focus(Client *c);
static Atom getatomprop(Client *c, Atom prop);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabkeys(void);
static void grabresizeabort();
static void keypress(XEvent *e);
static void keyrelease(XEvent *e);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static Client *nexttiled(Client *c);
static void propertynotify(XEvent *e);
static void rawmotion();
static void resize(Client *c, int x, int y, int w, int h);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void restack(Client *c, int mode);
static void run(void);
static void scan(void);
static int sendevent(Client *c, Atom proto);
static void setclientstate(Client *c, long state);
static void setfullscreen(Client *c, int fullscreen);
static void setup(void);
static void seturgent(Client *c, int urg);
static void showhide(Client *c);
static void sigchld(int unused);
static void tile(void);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatemonitors(XEvent *e);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static Client *wintoclient(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);

/* function declarations callable from config plugins*/
void focusstack(const Arg *arg);
void grabresize(const Arg *arg);
void grabstack(const Arg *arg);
void killclient(const Arg *arg);
void pin(const Arg *arg);
void quit(const Arg *arg);
void spawn(const Arg *arg);
void tag(const Arg *arg);
void togglefloating(const Arg *arg);
void togglefullscreen(const Arg *arg);
void toggletag(const Arg *arg);
void view(const Arg *arg);
void viewshift(const Arg *arg);
void viewtagshift(const Arg *arg);
void zoom(const Arg *arg);

/* variables */
static char stext[256];
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static Window barwin;
static int barfocus;
static int dragmode = DragNone;
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int tagset = 1;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ButtonRelease] = buttonrelease,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[DestroyNotify] = destroynotify,
	[Expose] = expose,
	[GenericEvent] = exthandler,
	[KeyPress] = keypress,
	[KeyRelease] = keyrelease,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[PropertyNotify] = propertynotify,
	[UnmapNotify] = unmapnotify,
};
static int randroutputchange;
static Atom wmatom[WMLast], netatom[NetLast];
static int end;
static Display *dpy;
static Drawable drawable;
static XftDraw *drawablexft;
static GC gc;
static Client *clients;
static Client *sel;
static Window root, wmcheckwin;
static Cursor curpoint, cursize;
static XftColor cols[colslen];
static XftFont *xfont;
/* dummy variables */
static int di;
static unsigned long dl;
static unsigned int dui;
static Window dwin;

/***********************
* Configuration Section
* Allows config plugins to change config variables.
* The defaultconfig method has plugin compatible code but check the README
* for the macros you need to work in the plugin.
************************/

#define S(T, N, V) N = V /* compatibily for plugin-flavor macro */
#define V(T, N, L, ...) do {static T _##N[] = __VA_ARGS__; N = _##N; L} while(0)
#define P(T, N, ...) V(T,N,,__VA_ARGS__;)
#define A(T, N, ...) V(T,N,N##len = (sizeof _##N/sizeof _##N[0]);,__VA_ARGS__;)

/* configurable values (see defaultconfig) */
Monitor *mons;
char *lsymbol, *font, **colors, **tags, **launcher, **terminal, **upvol,
	**downvol, **mutevol, **suspend, **dimup, **dimdown, **help;
int borderpx, snap, topbar, zenmode, tagslen, monslen, *nmain, keyslen,
	buttonslen;
float *mfact;
KeySym stackrelease;
Key *keys;
Button *buttons;

void
defaultconfig(void)
{
	/* appearance */
	S(int, borderpx, 1); /* border pixel width of windows */
	S(int, snap, 8); /* edge snap pixel distance */
	S(int, topbar, 1); /* 0 means bottom bar */
	S(int, zenmode, 3); /* ignores showing rapid client name changes (seconds) */
	S(char*, lsymbol, ">"); /* launcher symbol */
	S(char*, font, "monospace:size=8");
	/* colors (must be five colors: fg, bg, highlight, border, sel-border) */
	P(char*, colors, { "#dddddd", "#111111", "#335577", "#555555", "#dd4422" });

	/* virtual workspaces (must be 32 or less, *usually*) */
	A(char*, tags, { "1", "2", "3", "4", "5", "6", "7", "8", "9" });

	/* monitor layout
	   Set mons to the number of monitors you want supported.
	   Initialise with {0} for autodetection of monitors,
	   otherwise set the position and size ({x,y,w,h}).
	   The first monitor will be the primary monitor and have the bar.
	   !!!Warning!!! maximum of 32 monitors supported.
	   e.g:
	A(Monitor, mons, {
		{2420, 0, 1020, 1080},
		{1920, 0, 500, 1080},
		{3440, 0, 400,  1080}
	});
	   or to autodetect up to 3 monitors:
	A(Monitor, mons, {{0}, {0}, {0}});
	*/
	A(Monitor, mons, {{0}});
	/* factor of main area size [0.05..0.95] (for each monitor) */
	P(float, mfact, {0.6});
	/* number of clients in main area (for each monitor) */
	P(int, nmain, {1});

	/* commands */
	P(char*, launcher, { "dmenu_run", "-p", ">", "-m", "0", "-i", "-fn",
		"monospace:size=8", "-nf", "#dddddd", "-sf", "#dddddd", "-nb", "#111111",
		"-sb", "#335577", NULL });
	P(char*, terminal, { "st", NULL });
	#define VOLCMD(A) ("amixer -q set Master "#A"; xsetroot -name \"Volume: "\
		"$(amixer sget Master | awk -F'[][]' '/dB/ { print $2, $6 }')\"")
	P(char*, upvol, { "bash", "-c", VOLCMD("5%+"), NULL });
	P(char*, downvol, { "bash", "-c", VOLCMD("5%-"), NULL });
	P(char*, mutevol, { "bash", "-c", VOLCMD("toggle"), NULL });
	P(char*, suspend, {
		"bash", "-c", "killall slock; slock systemctl suspend -i", NULL });
	#define DIMCMD(A) ("xbacklight "#A" 5; xsetroot -name \"Brightness: "\
		"$(xbacklight | cut -d. -f1)%\"")
	P(char*, dimup, { "bash", "-c", DIMCMD("-inc"), NULL });
	P(char*, dimdown, { "bash", "-c", DIMCMD("-dec"), NULL });
	P(char*, help, { "st", "-t", "Help", "-e", "bash", "-c",
		"man filetwm || man -l ~/.config/filetwmconf.1", NULL });

	/* keyboard shortcut definitions */
	#define AltMask Mod1Mask
	#define WinMask Mod4Mask
	#define TK(KEY) { WinMask, XK_##KEY, view, {.ui = 1 << (KEY - 1)} }, \
	{       WinMask|ShiftMask, XK_##KEY, tag, {.ui = 1 << (KEY - 1)} }, \
	{                 AltMask, XK_##KEY, toggletag, {.ui = 1 << (KEY - 1)} },
	/* Alt+Tab style behaviour key release */
	S(KeySym, stackrelease, XK_Alt_L);
	A(Key, keys, {
		/*               modifier / key, function / argument */
		{               WinMask, XK_Tab, spawn, {.v = &launcher } },
		{     WinMask|ShiftMask, XK_Tab, spawn, {.v = &terminal } },
		{             WinMask, XK_space, grabresize, {.i = DragMove } },
		{     WinMask|AltMask, XK_space, grabresize, {.i = DragSize } },
		{ WinMask|ControlMask, XK_space, togglefloating, {0} },
		{            AltMask, XK_Return, togglefullscreen, {0} },
		{            WinMask, XK_Return, pin, {0} },
		{    WinMask|AltMask, XK_Return, zoom, {0} },
		{               AltMask, XK_Tab, grabstack, {.i = +1 } },
		{     AltMask|ShiftMask, XK_Tab, grabstack, {.i = -1 } },
		{                WinMask, XK_Up, focusstack, {.i = -1 } },
		{              WinMask, XK_Down, focusstack, {.i = +1 } },
		{              WinMask, XK_Left, viewshift, {.i = -1 } },
		{             WinMask, XK_Right, viewshift, {.i = +1 } },
		{    WinMask|ShiftMask, XK_Left, viewtagshift, {.i = -1 } },
		{   WinMask|ShiftMask, XK_Right, viewtagshift, {.i = +1 } },
		{                 AltMask, XK_0, tag, {.ui = ~0 } },
		{                AltMask, XK_F4, killclient, {0} },
		{                WinMask, XK_F4, spawn, {.v = &suspend } },
		{ AltMask|ControlMask|ShiftMask, XK_F4, quit, {0} },
		{    0, XF86XK_AudioLowerVolume, spawn, {.v = &downvol } },
		{           0, XF86XK_AudioMute, spawn, {.v = &mutevol } },
		{    0, XF86XK_AudioRaiseVolume, spawn, {.v = &upvol } },
		{               0, XF86XK_Sleep, spawn, {.v = &suspend } },
		{     0, XF86XK_MonBrightnessUp, spawn, {.v = &dimup } },
		{   0, XF86XK_MonBrightnessDown, spawn, {.v = &dimdown } },
		TK(1) TK(2) TK(3) TK(4) TK(5) TK(6) TK(7) TK(8) TK(9)
	});

	/* bar actions */
	A(Button, buttons, {
		/* click,      button, function / argument */
		{ ClkLauncher, Button1, spawn, {.v = &launcher } },
		{ ClkWinTitle, Button1, focusstack, {.i = +1 } },
		{ ClkWinTitle, Button3, focusstack, {.i = -1 } },
		{ ClkStatus,   Button1, spawn, {.v = &help } },
		{ ClkTagBar,   Button1, view, {0} },
		{ ClkTagBar,   Button3, tag, {0} },
	});
}

/* End Configuration Section
****************************/

void
arrange(void)
{
	focus(NULL);
	showhide(clients);
	tile();
	restack(sel, CliRaise);
}

void
attach(Client *c)
{
	c->next = clients;
	clients = c;
}

void
buttonpress(XEvent *e)
{
	int i, x = mons->mw, click = ClkWinTitle;
	Client *c;
	Arg arg = {0};
	XButtonPressedEvent *ev = &e->xbutton;

	if (ev->window == barwin) {
		for (i = tagslen - 1; i >= 0 && ev->x < (x -= TEXTW(tags[i])); i--);
		if (i >= 0) {
			click = ClkTagBar;
			arg.ui = 1 << i;
		} else if (ev->x > x - TEXTW(stext))
			click = ClkStatus;
		else if (ev->x < TEXTW(lsymbol))
			click = ClkLauncher;
		for (i = 0; i < buttonslen; i++)
			if (click == buttons[i].click && buttons[i].button == ev->button)
				buttons[i].func(arg.ui ? &arg : &buttons[i].arg);
	}

	else if (sel && dragmode == DragCheck)
		grabresize(&(Arg){.i = MOVEZONE(sel, ev->x, ev->y) ? DragMove : DragSize});

	else if ((c = wintoclient(ev->window))) {
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		focus(c);
		restack(c, c->isfloating ? CliZoom : CliRaise);
	}
}

void
buttonrelease(XEvent *e)
{
	grabresizeabort();
}

void
cleanup(void)
{
	view(&(Arg){.ui = ~0});
	while (clients)
		unmanage(clients, 0);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	XUngrabKeyboard(dpy, CurrentTime);
	XUnmapWindow(dpy, barwin);
	XDestroyWindow(dpy, barwin);
	XFreeCursor(dpy, curpoint);
	XFreeCursor(dpy, cursize);
	XDestroyWindow(dpy, wmcheckwin);
	XftFontClose(dpy, xfont);
	XFreePixmap(dpy, drawable);
	XFreeGC(dpy, gc);
	XftDrawDestroy(drawablexft);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void
clientmessage(XEvent *e)
{
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen])
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */
				&& !c->isfullscreen)));
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		if (c != sel && !c->isurgent)
			seturgent(c, 1);
	}
}

void
configure(Client *c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurerequest(XEvent *e)
{
	Client *c;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		if (c->isfloating) {
			if (ev->value_mask & CWX)
				c->x = c->fx = ev->x;
			if (ev->value_mask & CWY)
				c->y = c->fy = ev->y;
			if (ev->value_mask & CWWidth)
				c->w = c->fw = ev->width;
			if (ev->value_mask & CWHeight)
				c->h = c->fw = ev->height;
			if ((ev->value_mask & (CWX|CWY))
			&& !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
			configure(c);
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		if (ev->value_mask & CWSibling)
			wc.sibling = ev->above;
		if (ev->value_mask & CWStackMode)
			wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
}

void
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window)))
		unmanage(c, 1);
}

void
detach(Client *c)
{
	Client **tc;

	for (tc = &clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void
die(const char *msg) {
	fputs(msg, stderr);
	exit(1);
}

void
drawbar(int zen)
{
	int i, x, w;
	int boxs = TEXTPAD / 9;
	int boxw = TEXTPAD / 6 + 2;
	unsigned int occ = 0, urg = 0;
	Client *c;

	/* get urgency and occupied status for each tag */
	for (c = clients; c; c = c->next) {
		occ |= c->tags;
		urg |= c->tags * c->isurgent;
	}

	/* draw launcher button (left align) */
	drawtext(0, 0, (x = TEXTW(lsymbol)), BARH, lsymbol, &cols[fg], &cols[mark]);

	/* draw window title (left align) */
	drawtext(x, 0, mons->mw - x, BARH,
		sel ? (zen ? sel->zenname : sel->name) : "", &cols[fg], &cols[bg]);

	/* draw tags (right align) */
	x = mons->mw;
	for (i = tagslen - 1; i >= 0; i--) {
		x -= (w = TEXTW(tags[i]));
		drawtext(x, 0, w, BARH, tags[i], &cols[urg & 1 << i ? bg : fg],
			&cols[urg & 1 << i ? fg : (tagset & 1 << i ? mark : bg)]);
		if (occ & 1 << i) {
			XSetForeground(dpy, gc, cols[fg].pixel);
			XFillRectangle(dpy, drawable, gc, x + boxs, boxs, boxw, boxw);
		}
	}

	/* draw status (right align) */
	w = TEXTW(stext);
	drawtext(x - w, 0, w, BARH, stext, &cols[fg], &cols[bg]);

	/* display composited bar */
	XCopyArea(dpy, drawable, barwin, gc, 0, 0, mons->mw, BARH, 0, 0);
}

int
drawgettextwidth(const char *text)
{
	XGlyphInfo ext;
	XftTextExtentsUtf8(dpy, xfont, (XftChar8*)text, strlen(text), &ext);
	return ext.xOff;
}

void
drawtext(int x, int y, int w, int h, const char *text, const XftColor *fg,
	const XftColor *bg)
{
	int ty = y + (h - (xfont->ascent + xfont->descent)) / 2 + xfont->ascent;

	XSetForeground(dpy, gc, bg->pixel);
	XFillRectangle(dpy, drawable, gc, x, y, w, h);
	XftDrawStringUtf8(drawablexft, fg, xfont, x + (TEXTPAD / 2), ty,
		(XftChar8 *)text, strlen(text));
}

void
exthandler(XEvent *ev)
{
	if (ev->xcookie.evtype == XI_RawMotion)
		rawmotion();
	else if (ev->xcookie.evtype == randroutputchange)
		updatemonitors(ev);
}

void
expose(XEvent *e)
{
	XExposeEvent *ev = &e->xexpose;

	if (ev->count == 0)
		drawbar(0);
}

void
focus(Client *c)
{
	if ((!c || !ISVISIBLE(c)) && (!(c = sel) || !ISVISIBLE(sel)))
		for (c = clients; c && !ISVISIBLE(c); c = c->next);
	if (sel && sel != c) {
		/* catch the Click-to-Raise that could be coming */
		XGrabButton(dpy, AnyButton, AnyModifier, sel->win, False,
			ButtonPressMask, GrabModeSync, GrabModeSync, None, None);
		/* unfocus */
		XSetWindowBorder(dpy, sel->win, cols[bdr].pixel);
	}
	if (c) {
		if (c->isurgent)
			seturgent(c, 0);
		XSetWindowBorder(dpy, c->win, cols[selbdr].pixel);
		if (!barfocus) {
			XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
			PROPEDIT(PropModeReplace, c, NetActiveWindow)
			sendevent(c, wmatom[WMTakeFocus]);
		}
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	sel = c;
	drawbar(0);
}

void
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;

	if (!sel)
		return;
	if (arg->i > 0) {
		for (c = sel->next; c && !ISVISIBLE(c); c = c->next);
		if (!c)
			for (c = clients; c && !ISVISIBLE(c); c = c->next);
	} else {
		for (i = clients; i != sel; i = i->next)
			if (ISVISIBLE(i))
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i))
					c = i;
	}
	if (c) {
		focus(c);
		restack(c, CliRaise);
	}
}

Atom
getatomprop(Client *c, Atom prop)
{
	unsigned char *p = NULL;
	Atom da, atom = None;

	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		XFree(p);
	}
	return atom;
}

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False,
		wmatom[WMState], &real, &format, &n, &extra,
		(unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;
	if (name.encoding == XA_STRING)
		strncpy(text, (char *)name.value, size - 1);
	else {
		if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success
		&& n > 0 && *list) {
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

void
grabkeys(void)
{
	unsigned int i, j;
	/* NumLock assumed to be Mod2Mask */
	unsigned int mods[] = { 0, LockMask, Mod2Mask, Mod2Mask|LockMask };

	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	for (i = 0; i < keyslen; i++)
		for (j = 0; j < (sizeof mods / sizeof mods[0]) && KCODE(keys[i].key); j++)
			XGrabKey(dpy, KCODE(keys[i].key), keys[i].mod | mods[j], root,
				True, GrabModeAsync, GrabModeAsync);
}

void
grabresize(const Arg *arg) {
	/* abort if already in the desired mode */
	if (dragmode == arg->i)
		return;
	/* only grab if there is a selected window,
	   no support moving fullscreen or repositioning tiled windows. */
	if (!sel || sel->isfullscreen || (arg->i == DragMove && !sel->isfloating))
		return;

	/* set the drag mode so future motion applies to the action */
	dragmode = arg->i;
	/* detect if we should be dragging the tiled layout */
	if (dragmode == DragSize && !sel->isfloating)
		dragmode = DragTile;
	/* capture input */
	XGrabPointer(dpy, root, True, ButtonPressMask|ButtonReleaseMask
		|PointerMotionMask, GrabModeAsync, GrabModeAsync,None,cursize,CurrentTime);
	if (dragmode != DragCheck) {
		XGrabKeyboard(dpy, root, True, GrabModeAsync, GrabModeAsync, CurrentTime);
		/* bring the window to the top */
		restack(sel, CliRaise);
	}
}

void
grabresizeabort() {
	int i, m;
	char keystatemap[32];

	if (dragmode == DragNone)
		return;
	/* only release grab when all keys are up (or key repeat would interfere) */
	XQueryKeymap(dpy, keystatemap);
	for (i = 0; i < 32; i++)
		if (keystatemap[i] && dragmode != DragCheck)
			return;

	/* update the monitor layout to match any tiling changes */
	if (sel && dragmode == DragTile) {
		for (m = 0; m < monslen-1 && !INMON(sel->x, sel->y, mons[m]); m++);
		mfact[m] = MIN(0.95, MAX(0.05, (float)WIDTH(sel) / mons[m].mw));
		nmain[m] = MAX(1, mons[m].mh / HEIGHT(sel));
		tile();
	}

	/* release the drag */
	XUngrabPointer(dpy, CurrentTime);
	XUngrabKeyboard(dpy, CurrentTime);
	dragmode = DragNone;
}

void
grabstack(const Arg *arg)
{
	XGrabKeyboard(dpy, root, True, GrabModeAsync, GrabModeAsync, CurrentTime);
	focusstack(arg);
}

void
keypress(XEvent *e)
{
	unsigned int i;

	grabresizeabort();
	for (i = 0; i < keyslen; i++)
		if (e->xkey.keycode == KCODE(keys[i].key)
		&& KEYMASK(keys[i].mod) == KEYMASK(e->xkey.state))
			keys[i].func(&(keys[i].arg));
}

void
keyrelease(XEvent *e)
{
	grabresizeabort();
	/* zoom after cycling windows if releasing the modifier key, this gives
	   Alt+Tab+Tab... behavior like with common window managers */
	if (dragmode == DragNone && KCODE(stackrelease) == e->xkey.keycode) {
		restack(sel, CliZoom);
		XUngrabKeyboard(dpy, CurrentTime);
	}
}

void
killclient(const Arg *arg)
{
	if (!sel)
		return;
	if (!sendevent(sel, wmatom[WMDelete])) {
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, sel->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void
manage(Window w, XWindowAttributes *wa)
{
	int x, y, m;
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;

	if (!(c = calloc(1, sizeof(Client))))
		die("calloc failed.\n");
	attach(c);
	c->win = w;
	c->zenping = 0;
	/* geometry */
	c->x = c->fx = wa->x;
	c->y = c->fy = wa->y;
	c->w = c->fw = wa->width;
	c->h = c->fh = wa->height;
	c->oldbw = wa->border_width;

	updatetitle(c);
	strcpy(c->zenname, c->name);
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans)))
		c->tags = t->tags;
	else
		c->tags = tagset;
	c->isfloating = 1;

	/* find current monitor */
	if (MOUSEINF(dwin, x, y, dui))
		for (m = monslen-1; m > 0 && !INMON(x, y, mons[m]); m--);
	else m = 0;
	/* adjust to current monitor */
	if (c->x + WIDTH(c) > mons[m].mx + mons[m].mw)
		c->x = c->fx = mons[m].mx + mons[m].mw - WIDTH(c);
	if (c->y + HEIGHT(c) > mons[m].my + mons[m].mh)
		c->y = c->fy = mons[m].my + mons[m].mh - HEIGHT(c);
	c->x = c->fx = MAX(c->x, mons[m].mx);
	/* only fix client y-offset, if the client center might cover the bar */
	c->y = c->fy =
		MAX(c->y, ((BARY == mons->my) && (c->x + (c->w / 2) >= mons->mx)
			&& (c->x + (c->w / 2) < mons->mx + mons->mw)) ? BARH : mons->my);
	c->bw = borderpx;

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	XSelectInput(dpy, w, PropertyChangeMask|StructureNotifyMask);
	PROPEDIT(PropModeAppend, c, NetClientList)
	/* some windows require this */
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h);
	setclientstate(c, NormalState);
	showhide(clients);
	restack(c, CliRaise);
	XMapWindow(dpy, c->win);
	focus(c);
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

	if (!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
	if (wa.override_redirect)
		return;
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

Client *
nexttiled(Client *c)
{
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}

void
pin(const Arg *arg)
{
	restack(sel, CliPin);
}

void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if ((ev->window == root) && (ev->atom == XA_WM_NAME))
		updatestatus();
	else if (ev->state == PropertyDelete)
		return; /* ignore */
	else if ((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
				(c->isfloating = (wintoclient(trans)) != NULL))
				arrange();
			break;
		case XA_WM_NORMAL_HINTS:
			updatesizehints(c);
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbar(0);
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == sel)
				if (!zenmode || (ev->time - c->zenping) > (zenmode * 1000)) {
					strcpy(c->zenname, c->name);
					drawbar(0);
				}
			c->zenping = ev->time;

		}
		if (ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

void
quit(const Arg *arg)
{
	end = 1;
}

void
rawmotion()
{
	int rx, ry, x, y;
	static int lx = 0, ly = 0;
	unsigned int mask;
	Window cw;
	static Window lastcw = {0};
	static Client *c = NULL;

	/* capture pointer and motion details */
	if (!MOUSEINF(cw, rx, ry, mask))
		return;
	x = rx - lx; lx = rx;
	y = ry - ly; ly = ry;

	/* handle any drag modes */
	if (sel && dragmode == DragMove)
		resize(sel, sel->fx + x, sel->fy + y, sel->fw, sel->fh);
	if (sel && dragmode == DragSize)
		resize(sel, sel->fx, sel->fy, MAX(sel->fw + x, 1), MAX(sel->fh + y, 1));
	if (sel && dragmode == DragTile)
		resize(sel, sel->x, sel->y, MAX(sel->w + x, 1), MAX(sel->h + y, 1));
	if (dragmode == DragCheck && (!sel || BARZONE(rx, ry)
		|| (!MOVEZONE(sel, rx, ry) && !RESIZEZONE(sel, rx, ry))))
		grabresizeabort();
	if (dragmode != DragNone)
		return;

	/* top bar raise when mouse hits the screen edge.
	   especially useful for apps that capture the keyboard. */
	if (BARZONE(rx, ry) && !barfocus) {
		barfocus = 1;
		XRaiseWindow(dpy, barwin);
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	} else if (!BARZONE(rx, ry) && barfocus) {
		barfocus = 0;
		if (sel)
			focus(sel);
		restack(NULL, CliRefresh);
	}

	c = cw != lastcw ? wintoclient(cw) : c;
	lastcw = cw;
	/* focus follows mouse */
	if (c && c != sel)
		focus(c);
	/* watch for border edge locations for resizing */
	if (c && !mask && (MOVEZONE(c, rx, ry) || RESIZEZONE(c, rx, ry)))
		grabresize(&(Arg){.i = DragCheck});
}

void
resize(Client *c, int x, int y, int w, int h)
{
	int m1, m2;

	if (c->isfloating && !c->isfullscreen) {
		c->fx = x;
		c->fy = y;
		c->fw = w;
		c->fh = h;
		/* snap position to edges */
		for (m1 = 0; m1 < monslen-1 && !INMON(x+snap, y+snap, mons[m1]); m1++);
		for (m2 = 0; m2 < monslen-1 && !INMON(x+w-snap, y+h-snap, mons[m2]); m2++);
		/* snap position */
		x = (abs(mons[m1].mx - x) < snap) ? mons[m1].mx : x;
		y = (abs(WINY(mons[m1]) - y) < snap) ? WINY(mons[m1]) : y;
		/* snap size */
		if (abs((mons[m2].mx + mons[m2].mw) - (x + w + 2*c->bw)) < snap)
			w = mons[m2].mx + mons[m2].mw - x - 2*c->bw;
		if (abs((WINY(mons[m2]) + WINH(mons[m2])) - (y + h + 2*c->bw)) < snap)
			h = WINY(mons[m2]) + WINH(mons[m2]) - y - 2*c->bw;
	}

	/* set minimum possible size */
	w = MAX(1, w);
	h = MAX(1, h);
	/* return to visible area */
	x = (x > sw) ? sw - WIDTH(c) : x;
	y = (y > sh) ? sh - HEIGHT(c) : y;
	x = (x + w + 2 * c->bw < 0) ? 0 : x;
	y = (y + h + 2 * c->bw < 0) ? 0 : y;

	/* adjust for aspect limits */
	/* see last two sentences in ICCCM 4.1.2.3 */
	w -= c->basew;
	h -= c->baseh;
	if (c->mina > 0 && c->maxa > 0) {
		if (c->maxa < (float)w / h)
			w = h * c->maxa + 0.5;
		else if (c->mina < (float)h / w)
			h = w * c->mina + 0.5;
	}

	/* restore base dimensions and apply max and min dimensions */
	w = MAX(w + c->basew, c->minw);
	h = MAX(h + c->baseh, c->minh);
	w = (c->maxw) ? MIN(w, c->maxw) : w;
	h = (c->maxh) ? MIN(h, c->maxh) : h;

	/* apply the resize if anything ended up changing */
	if (x != c->x || y != c->y || w != c->w || h != c->h)
		resizeclient(c, x, y, w, h);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
	XWindowChanges wc;

	c->x = wc.x = x;
	c->y = wc.y = y;
	c->w = wc.width = w;
	c->h = wc.height = h;
	/* fullscreen changes update the border width */
	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, False);
}

void
restack(Client *c, int mode)
{
	int i = 0;
	static Client *pinned = NULL;
	static Client *raised = NULL;
	Window up[3];
	XWindowChanges wc;

	switch (mode) {
	case CliPin:
		/* toggle pinned state */
		pinned = pinned != c ? c : NULL;
		break;
	case CliRemove:
		detach(c);
		pinned = pinned != c ? pinned : NULL;
		raised = raised != c ? raised : NULL;
		break;
	case CliZoom:
		if (c) {
			detach(c);
			attach(c);
			if (!c->isfloating)
				arrange();
		}
		/* fall through to CliRaise */
	case CliRaise:
		raised = c;
	}
	/* always lift up anything pinned */
	if (pinned && pinned->isfloating) {
		detach(pinned);
		attach(pinned);
	}

	XDeleteProperty(dpy, root, netatom[NetCliStack]);
	if (barfocus) up[i++] = barwin;
	if (pinned) {
		up[i++] = pinned->win;
		PROPEDIT(PropModePrepend, pinned, NetCliStack)
	}
	if (raised) {
		up[i++] = raised->win;
		PROPEDIT(PropModePrepend, raised, NetCliStack)
	}
	if (!barfocus) up[i++] = barwin;
	XRaiseWindow(dpy, up[0]);
	XRestackWindows(dpy, up, i);
	wc.stack_mode = Below;
	wc.sibling = up[i - 1];

	/* order floating/fullscreen layer */
	for (c = clients; c; c = c->next)
		if (c != pinned && c != raised && c->isfloating) {
			XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
			wc.sibling = c->win;
			PROPEDIT(PropModePrepend, c, NetCliStack)
		}
	/* order tiled layer */
	for (c = clients; c; c = c->next)
		if (c != pinned && c != raised && !c->isfloating) {
			XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
			wc.sibling = c->win;
			PROPEDIT(PropModePrepend, c, NetCliStack)
		}
}

void
run(void)
{
	XEvent ev;
	/* main event loop */
	XSync(dpy, False);
	while (!end && !XNextEvent(dpy, &ev))
		if (handler[ev.type])
			handler[ev.type](&ev); /* call handler */
}

void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if (wins)
			XFree(wins);
	}
}

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
}

int
sendevent(Client *c, Atom proto)
{
	int n;
	Atom *protocols;
	int exists = 0;
	XEvent ev;

	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		while (!exists && n--)
			exists = protocols[n] == proto;
		XFree(protocols);
	}
	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = c->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = proto;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
	}
	return exists;
}

void
setfullscreen(Client *c, int fullscreen)
{
	int w, h, m1, m2;

	if (fullscreen && !c->isfullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = 1;
		c->fstate = c->isfloating;
		c->fbw = c->bw;
		c->bw = 0;
		c->isfloating = 1;
		/* find the full screen spread across the monitors */
		for (m1 = monslen-1; m1 > 0 && !INMON(c->x, c->y, mons[m1]); m1--);
		for (m2 = 0; m2 < monslen
		&& !INMON(c->x + WIDTH(c), c->y + HEIGHT(c), mons[m2]); m2++);
		if (m2 == monslen || mons[m2].mx + mons[m2].mw <= mons[m1].mx
		|| mons[m2].my + mons[m2].mh <= mons[m1].my)
			m2 = m1;
		/* apply fullscreen window parameters */
		w = mons[m2].mx - mons[m1].mx + mons[m2].mw;
		h = mons[m2].my - mons[m1].my + mons[m2].mh;
		resizeclient(c, mons[m1].mx, mons[m1].my, w, h);
		restack(c, CliZoom);

	} else if (!fullscreen && c->isfullscreen){
		/* change back to original floating parameters */
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;
		c->isfloating = c->fstate;
		c->bw = c->fbw;
		resize(c, c->fx, c->fy, c->fw, c->fh);
	}
	arrange();
}

void
setup(void)
{
	int i, xre;
	unsigned char xi[XIMaskLen(XI_RawMotion)] = { 0 };
	XIEventMask evm;
	Atom utf8string;
	void (*conf)(void);

	/* register handler to clean up any zombies immediately */
	sigchld(0);

	/* Load configs.
	   First load the default included config.
	   Then load the distribution's config plugin, if one exists.
	   Then load the user's config plugin, if they have one.
	   Leave the current working directory in the user's home dir. */
	defaultconfig();
	if (LOADCONF("/etc/config/filetwmconf.so", conf))
		conf();
	if (!chdir(getenv("HOME")) && LOADCONF(".config/filetwmconf.so", conf))
		conf();
	else if (access(".config/filetwmconf.so", F_OK) == 0)
		die(dlerror());

	/* init screen and display */
	XSetErrorHandler(xerror);
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
	drawable = XCreatePixmap(dpy, root, sw, sh, DefaultDepth(dpy, screen));
	drawablexft = XftDrawCreate(dpy, drawable, DefaultVisual(dpy, screen),
		DefaultColormap(dpy, screen));
	gc = XCreateGC(dpy, root, 0, NULL);
	XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);
	if (!(xfont = XftFontOpenName(dpy, screen, font)))
		die("font couldn't be loaded.\n");
	/* init monitor layout */
	if (MONNULL(mons[0])) {
		updatemonitors(NULL);
		/* select xrandr events (if monitor layout isn't hard configured) */
		if (XRRQueryExtension(dpy, &xre, &di)) {
			randroutputchange = xre + RRNotify_OutputChange;
			XRRSelectInput(dpy, root, RROutputChangeNotifyMask);
		}
	}
	/* init atoms */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen] = XInternAtom(
		dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(
		dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	netatom[NetCliStack] = XInternAtom(dpy, "_NET_CLIENT_LIST_STACKING", False);
	/* init cursors */
	curpoint = XCreateFontCursor(dpy, XC_left_ptr);
	cursize = XCreateFontCursor(dpy, XC_sizing);
	XDefineCursor(dpy, root, curpoint);
	/* init colors */
	for (i = 0; i < colslen; i++)
		if (!XftColorAllocName(dpy, DefaultVisual(dpy, screen),
				DefaultColormap(dpy, screen), colors[i], &cols[i]))
			die("error, cannot allocate colors.\n");
	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "filetwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* init bars */
	barwin = XCreateWindow(dpy, root, mons->mx, BARY, mons->mw, BARH, 0,
		DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
		CWOverrideRedirect|CWBackPixmap|CWEventMask, &(XSetWindowAttributes){
			.override_redirect = True,
			.background_pixmap = ParentRelative,
			.event_mask = ButtonPressMask|ExposureMask});
	XMapRaised(dpy, barwin);
	XSetClassHint(dpy, barwin, &(XClassHint){"filetwm", "filetwm"});
	updatestatus();
	/* select events */
	XSelectInput(dpy, root, SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|ButtonReleaseMask|StructureNotifyMask|PropertyChangeMask);
	/* prepare motion capture */
	rawmotion();
	/* select xinput events */
	if (XQueryExtension(dpy, "XInputExtension", &di, &di, &di)
	&& XIQueryVersion(dpy, &(int){2}, &(int){0}) == Success) {
		XISetMask(xi, XI_RawMotion);
		evm.deviceid = XIAllMasterDevices;
		evm.mask_len = sizeof(xi);
		evm.mask = xi;
		XISelectEvents(dpy, root, &evm, 1);
	}

	grabkeys();
	focus(NULL);
}


void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	c->isurgent = urg;
	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags = urg ? (wmh->flags|XUrgencyHint) : (wmh->flags&~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
showhide(Client *c)
{
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if (c->isfloating && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h);
		showhide(c->next);
	} else {
		/* hide clients bottom up */
		showhide(c->next);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}


void
sigchld(int unused)
{
	/* self-register this method as the SIGCHLD handler (if haven't already) */
	if (signal(SIGCHLD, sigchld) == SIG_ERR) {
		die("can't install SIGCHLD handler.\n");
	}

	/* immediately release resources associated with any zombie child */
	while (0 < waitpid(-1, NULL, WNOHANG));
}

void
spawn(const Arg *arg)
{
	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp((*(char***)arg->v)[0], *(char ***)arg->v);
		fprintf(stderr, "filetwm: execvp %s", (*(char ***)arg->v)[0]);
		perror(" failed");
		exit(EXIT_SUCCESS);
	}
}

void
tag(const Arg *arg)
{
	if (sel && arg->ui & TAGMASK) {
		sel->tags = arg->ui & TAGMASK;
		arrange();
	}
}

void
tile(void)
{
	unsigned int m, h, mw;
	/* maximum of 32 monitors supported */
	int nm[32] = {0}, i[32] = {0}, my[32] = {0}, ty[32] = {0};
	Client *c;

	/* find the number of clients in each monitor */
	for (c = nexttiled(clients); c; c = nexttiled(c->next)) {
		for (m = monslen-1; m > 0 && !ONMON(c, mons[m]); m--);
		nm[m]++;
	}

	/* tile windows into the relevant monitors. */
	for (c = nexttiled(clients); c; c = nexttiled(c->next), i[m]++) {
		/* find the monitor placement again */
		for (m = monslen-1; m > 0 && !ONMON(c, mons[m]); m--);
		/* tile the client within the relevant monitor */
		mw = nm[m] > nmain[m] ? mons[m].mw * mfact[m] : mons[m].mw;
		if (i[m] < nmain[m]) {
			h = (WINH(mons[m]) - my[m]) / (MIN(nm[m], nmain[m]) - i[m]);
			resize(c, mons[m].mx, WINY(mons[m]) + my[m], mw-(2*c->bw), h-(2*c->bw));
			if (my[m] + HEIGHT(c) < WINH(mons[m]))
				my[m] += HEIGHT(c);
		} else {
			h = (WINH(mons[m]) - ty[m]) / (nm[m] - i[m]);
			resize(c, mons[m].mx + mw, WINY(mons[m]) + ty[m],
				mons[m].mw - mw - (2*c->bw), h - (2*c->bw));
			if (ty[m] + HEIGHT(c) < WINH(mons[m]))
				ty[m] += HEIGHT(c);
		}
	}
}

void
togglefloating(const Arg *arg)
{
	if (!sel)
		return;
	if (sel->isfullscreen)
		setfullscreen(sel, 0);
	if ((sel->isfloating = !sel->isfloating))
		resize(sel, sel->fx, sel->fy, sel->fw, sel->fh);
	arrange();
}

void
togglefullscreen(const Arg *arg)
{
	if (sel)
		setfullscreen(sel, !sel->isfullscreen);
}

void
toggletag(const Arg *arg)
{
	unsigned int newtags;

	if (!sel)
		return;
	newtags = sel->tags ^ (arg->ui & TAGMASK);
	if (newtags) {
		sel->tags = newtags;
		arrange();
	}
}

void
unmanage(Client *c, int destroyed)
{
	XWindowChanges wc;

	restack(c, CliRemove);
	if (!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy); /* avoid race conditions */
		XSetErrorHandler(xerrordummy);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	sel = sel != c ? sel : NULL;
	free(c);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (c = clients; c; c = c->next)
		PROPEDIT(PropModeAppend, c, NetClientList)
	arrange();
}

void
unmapnotify(XEvent *e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, 0);
	}
}

void
updatemonitors(XEvent *e)
{
	int i, pri = 0, n;
	Monitor m;
	XRRMonitorInfo *inf;

	inf = XRRGetMonitors(dpy, root, 1, &n);
	for (i = 0; i < n && i < monslen; i++) {
		SETMON(mons[i], inf[i])
		if (inf[i].primary)
			pri = i;
	}
	/* push the primary monitor to the top */
	m = mons[pri];
	mons[pri] = mons[0];
	mons[0] = m;
}

void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	c->basew = c->baseh = c->maxw = c->maxh = c->minw = c->minh = 0;
	c->maxa = c->mina = 0.0;
	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		return;

	if (size.flags & PBaseSize) {
		c->basew = c->minw = size.base_width;
		c->baseh = c->minh = size.base_height;
	}
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	}
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	}
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	}
}

void
updatestatus(void)
{
	if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
		strcpy(stext, "  FiletLignux  ");
	drawbar(1);
}

void
updatetitle(Client *c)
{
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
}

void
updatewindowtype(Client *c)
{
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	if (wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = 1;
}

void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
		XFree(wmh);
	}
}

void
view(const Arg *arg)
{
	tagset = arg->ui & TAGMASK;
	arrange();
}

void
viewshift(const Arg *arg)
{
	tagset = TAGSHIFT(tagset, arg->i);
	arrange();
}

void
viewtagshift(const Arg *arg)
{
	if (sel) sel->tags = TAGSHIFT(sel->tags, arg->i);
	viewshift(arg);
}

Client *
wintoclient(Window w)
{
	Client *c;

	for (c = clients; c; c = c->next)
		if (c->win == w)
			return c;
	return NULL;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	else if (ee->request_code == X_ChangeWindowAttributes
	&& ee->error_code == BadAccess)
		die("filetwm: another window manager may already be running.\n");
	fprintf(stderr, "filetwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

void
zoom(const Arg *arg)
{
	restack(sel, CliZoom);
}

int
main(int argc, char *argv[])
{
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("filetwm-"VERSION"\n");
	else if (argc != 1)
		die("usage: filetwm [-v]\n");
	if (!(dpy = XOpenDisplay(NULL)))
		die("filetwm: cannot open display.\n");
	setup();
	scan();
	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
