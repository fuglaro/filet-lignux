/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include "drw.h"
#include "util.h"

/* macros */
#define BUTTONMASK           (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)      (mask & ~(numlockmask|LockMask)\
                              & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask\
                                 |Mod3Mask|Mod4Mask|Mod5Mask))
#define ISVISIBLE(C)            ((C->tags & tagset))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define MOUSEINF(W,X,Y,M) (XQueryPointer(dpy,root,&dwin,&W,&X,&Y,&di,&di,&M))
#define INZONE(C, X, Y)         (X >= C->x - C->bw && Y >= C->y - C->bw &&\
               (X <= C->x + WIDTH(C) + C->bw && Y <= C->y + HEIGHT(C) + C->bw)
#define MOVEZONE(C, X, Y)       (INZONE(C, X, Y) && abs(C->x - X) <= C->bw ||\
                                abs(C->y - Y) <= C->bw)
#define RESIZEZONE(C, X, Y)     (INZONE(C, X, Y) &&\
     abs(C->x + WIDTH(C) - X) <= C->bw || abs(C->y + HEIGHT(C) - Y) <= C->bw)
#define WINY(M)                 (&M == mons && topbar ? M.my + bh : M.my)
#define WINH(M)                 (&M == mons ? M.mh - bh : M.mh)
#define MONLEN                  (sizeof mons / sizeof mons[0])
#define MONNULL(M)          (M.mx == 0 && M.my == 0 && M.mw == 0 && M.mh == 0)
#define INMON(X, Y, M)          (X >= M.mx && X < M.mx + M.mw &&\
                                 Y >= M.my && Y < M.my + M.mh)
#define SETMON(M, R)            {M.mx = R.x; M.my = R.y;\
                                 M.mw = R.width; M.mh = R.height;}
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define WINMON(C, M)           INMON(C->x + WIDTH(C)/2, C->y + HEIGHT(C)/2, M)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define TAGSHIFT(TAGS, I) (I < 0 ? (TAGS >> -I) | (TAGS << (LENGTH(tags) + I))\
                                 : (TAGS << I) | (TAGS >> (LENGTH(tags) - I)))
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)

#define PROPEDIT(P, C, A) {XChangeProperty(dpy, root, netatom[A], XA_WINDOW,\
                         32, P, (unsigned char *) &(C->win), 1);}
/* enums */
enum { SchemeNorm, SchemeSel }; /* color schemes */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck, /* EWMH atoms */
       NetWMFullscreen, NetActiveWindow, NetWMWindowType,
       NetWMWindowTypeDialog, NetClientList, NetCliStack, NetLast };
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast };
                                                           /* default atoms */
enum { ClkLauncher, ClkWinTitle, ClkStatus, ClkTagBar, ClkLast };
enum { DragMove, DragSize, DragTile };
enum { CliPin, CliRaise, CliZoom, CliRemove, CliRefresh };

typedef union {
	int i;
	unsigned int ui;
	const void *v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Client Client;
struct Client {
	char name[256], zenname[256];
	float mina, maxa;
	int x, y, w, h;
	int fx, fy, fw, fh; /*remember during tiled and fullscreen states */
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int bw, fbw, oldbw;
	unsigned int tags;
	int isfixed, isfloating, isurgent, fstate, isfullscreen;
	Client *next;
	Window win;
	Time zenping;
};

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct Monitor Monitor;
struct Monitor {
	int mx, my, mw, mh;   /* screen size */
};

/* function declarations */
static int applysizehints(Client *c, int *x, int *y, int *w, int *h);
static void arrange(void);
static void attach(Client *c);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurerequest(XEvent *e);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void drawbar(int zen);
static void expose(XEvent *e);
static void exthandler(XEvent *ev);
static void focus(Client *c);
static void focusstack(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabkeys(void);
static void grabresize(const Arg *arg);
static void grabresizecheck(Client *c);
static void grabstack(const Arg *arg);
static void keypress(XEvent *e);
static void keyrelease(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static Client *nexttiled(Client *c);
static void pin(const Arg *arg);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static void rawmotion(XEvent *e);
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
static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static void tile(void);
static void togglefloating(const Arg *arg);
static void togglefullscreen(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatemonitors(XEvent *e);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static void viewshift(const Arg *arg);
static void viewtagshift(const Arg *arg);
static Client *wintoclient(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);

/* variables */
static char stext[256];
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static Window barwin;
static int barfocus;
static int bh, blw, by, vw;  /* bar geometry */
static int lrpad;            /* sum of left and right padding for text */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask;
static unsigned int tagset = 1;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
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
	[XI_RawMotion] = rawmotion,
};
static Atom wmatom[WMLast], netatom[NetLast];
static int end;
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static Client *clients;
static Client *sel;
static Window root, wmcheckwin;
static Cursor curpoint, cursize;
/* dummy variables */
static int di;
static unsigned long dl;
static unsigned int dui;
static Window dwin;

/* configuration, allows nested code to access above variables */
#include "config.h"

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };

int
applysizehints(Client *c, int *x, int *y, int *w, int *h)
{
	int baseismin;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	{
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2 * c->bw < 0)
			*x = 0;
		if (*y + *h + 2 * c->bw < 0)
			*y = 0;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (resizehints || c->isfloating) {
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(void)
{
	focus(NULL);
	showhide(clients);
	tile();
	restack(NULL, CliRefresh);
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
	unsigned int i = 0, x = vw, click = ClkWinTitle;
	Client *c;
	Arg arg = {0};
	XButtonPressedEvent *ev = &e->xbutton;

	if (ev->window == barwin) {
		for (;ev->x >= vw && ev->x >= (x+=TEXTW(tags[i])) && ++i < LENGTH(tags););
		if (ev->x >= vw && i < LENGTH(tags)) {
			click = ClkTagBar;
			arg.ui = 1 << i;
		} else if (ev->x > vw - (int)TEXTW(stext))
			click = ClkStatus;
		else if (ev->x < blw)
			click = ClkLauncher;
		for (i = 0; i < LENGTH(buttons); i++)
			if (click == buttons[i].click && buttons[i].button == ev->button)
				buttons[i].func(arg.ui ? &arg : &buttons[i].arg);
	} else if ((c = wintoclient(ev->window))) {
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		focus(c);
		restack(c, c->isfloating ? CliZoom : CliRaise);
	}
}

void
checkotherwm(void)
{
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(void)
{
	size_t i;

	view(&(Arg){.ui = ~0});
	while (clients)
		unmanage(clients, 0);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	XUngrabKeyboard(dpy, CurrentTime);
	XUnmapWindow(dpy, barwin);
	XDestroyWindow(dpy, barwin);
	XFreeCursor(dpy, curpoint);
	XFreeCursor(dpy, cursize);
	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
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
drawbar(int zen)
{
	int x, w, tw = 0;
	int boxs = drw->fonts->h / 9;
	int boxw = drw->fonts->h / 6 + 2;
	unsigned int i, occ = 0, urg = 0;
	Client *c;

	vw = mons->mw;
	for (i = 0; i < LENGTH(tags); i++) vw -= TEXTW(tags[i]);

	/* draw status first so it can be overdrawn by tags later */
	drw_setscheme(drw, scheme[SchemeNorm]);
	tw = TEXTW(stext) - lrpad + 2; /* 2px right padding */
	drw_text(drw, vw - tw, 0, tw, bh, 0, stext, 0);

	for (c = clients; c; c = c->next) {
		occ |= c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}
	x = vw;
	for (i = 0; i < LENGTH(tags); i++) {
		w = TEXTW(tags[i]);
		drw_setscheme(drw, scheme[tagset & 1 << i ? SchemeSel : SchemeNorm]);
		drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);
		if (occ & 1 << i)
			drw_rect(drw, x + boxs, boxs, boxw, boxw,
				sel && sel->tags & 1 << i,
				urg & 1 << i);
		x += w;
	}

	x = 0;
	w = blw = TEXTW(lsymbol);
	drw_setscheme(drw, scheme[SchemeSel]);
	x = drw_text(drw, x, 0, w, bh, lrpad / 2, lsymbol, 0);

	if ((w = vw - x - tw) > bh) {
		if (sel) {
			drw_setscheme(drw, scheme[SchemeNorm]);
			drw_text(drw, x, 0, w, bh, lrpad / 2, zen ? sel->zenname : sel->name, 0);
			if (sel->isfloating)
				drw_rect(drw, x + boxs, boxs, boxw, boxw, sel->isfixed, 0);
		} else {
			drw_setscheme(drw, scheme[SchemeNorm]);
			drw_rect(drw, x, 0, w, bh, 1, 1);
		}
	}
	drw_map(drw, barwin, 0, 0, mons->mw, bh);
}

void
exthandler(XEvent *ev)
{
	if (handler[ev->xcookie.evtype])
		handler[ev->xcookie.evtype](ev); /* call handler */
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
	if (!c || !ISVISIBLE(c)){
		if (sel)
			for (c = sel; c && !ISVISIBLE(c); c = c->next);
		if (!c) for (c = clients; c && !ISVISIBLE(c); c = c->next);
	}
	if (sel && sel != c) {
		/* catch the Click-to-Raise that could be coming */
		XGrabButton(dpy, AnyButton, AnyModifier, sel->win, False,
			BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
		/* unfocus */
		XSetWindowBorder(dpy, sel->win, scheme[SchemeNorm][ColBorder].pixel);
	}
	if (c) {
		if (c->isurgent)
			seturgent(c, 0);
		XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
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
	KeyCode code;
	XModifierKeymap *modmap;

	/* update numlock mask */
	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);

	{
		unsigned int modifiers[] = {
			0, LockMask, numlockmask, numlockmask|LockMask };

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		for (i = 0; i < LENGTH(keys); i++)
			if ((code = XKeysymToKeycode(dpy, keys[i].keysym)))
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
						True, GrabModeAsync, GrabModeAsync);
	}
}

void
grabresize(const Arg *arg) {
	static int grabguard = 0;
	int x, y, i, m1, m2, type = arg->i;
	char keydown = 'x';
	char keystatemap[32];
	Client *c;
	Client oc, nc;
	XEvent ev;
	Time lasttime = 0;

	/* prevent keyrepeats interfering with grabs,
	   only grab if there is a selected window,
	   no support moving fullscreen windows by mouse */
	if (grabguard || !(c = sel) || c->isfullscreen || !MOUSEINF(dwin, x, y, dui))
		return;
	restack(c, CliRaise);
	nc = oc = *c;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursize, CurrentTime) != GrabSuccess)
		return;
	XGrabKeyboard(dpy, root, True, GrabModeAsync, GrabModeAsync, CurrentTime);

	grabguard = 1;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask|
			KeyPressMask|KeyReleaseMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case KeyRelease:
			XQueryKeymap(dpy, keystatemap);
			keydown = keystatemap[0];
			for (i = 1; i < 32; i++)
				keydown |= keystatemap[i];
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;
			/* release from tile mode if needed */
			if (type != DragTile && !c->isfloating
			&& MAX(abs(ev.xmotion.x - x), abs(ev.xmotion.y - y)) > snap)
				togglefloating(NULL);

			/* calculate window movement */
			if (type == DragMove) {
				nc.x = oc.x + (ev.xmotion.x - x);
				nc.y = oc.y + (ev.xmotion.y - y);
				for (m1 = 0; m1 < MONLEN-1
				&& !INMON(nc.x + snap, nc.y + snap, mons[m1]); m1++);
				for (m2 = 0; m2 < MONLEN-1
				&& !INMON(nc.x + nc.w - snap, nc.y + nc.h - snap, mons[m2]); m2++);
				/* snap to edges */
				if (abs(mons[m1].mx - nc.x) < snap)
					nc.x = mons[m1].mx;
				else if (abs((mons[m2].mx + mons[m2].mw) - (nc.x + WIDTH(c))) < snap)
					nc.x = mons[m2].mx + mons[m2].mw - WIDTH(c);
				if (abs(WINY(mons[m1]) - nc.y) < snap)
					nc.y = WINY(mons[m1]);
				else if (abs((WINY(mons[m2]) + WINH(mons[m2]))
				             - (nc.y + HEIGHT(c))) < snap)
					nc.y = WINY(mons[m2]) + WINH(mons[m2]) - HEIGHT(c);
			}
			else if (type == DragSize || type == DragTile) {
				nc.w = MAX(oc.w + (ev.xmotion.x - x), 1);
				nc.h = MAX(oc.h + (ev.xmotion.y - y), 1);
				for (m2 = 0; m2 < MONLEN-1
				&& !INMON(nc.x + nc.w - snap, nc.y + nc.h - snap, mons[m2]); m2++);
				/* snap to edges */
				if (abs((mons[m2].mx + mons[m2].mw) - (c->x + nc.w + 2*c->bw)) < snap)
					nc.w = mons[m2].mx + mons[m2].mw - c->x - 2*c->bw;
				if (abs((WINY(mons[m2]) + WINH(mons[m2]))
				        - (c->y + nc.h + 2*c->bw)) < snap)
					nc.h = WINY(mons[m2]) + WINH(mons[m2]) - c->y - 2*c->bw;
			}
			if (c->isfloating || type == DragTile)
				resize(c, nc.x, nc.y, nc.w, nc.h);
			break;
		}
	} while (ev.type != ButtonRelease && keydown);
	XUngrabPointer(dpy, CurrentTime);
	XUngrabKeyboard(dpy, CurrentTime);
	grabguard = 0;

	/* update the monitor layout to match any tiling changes */
	if (type == DragTile) {
		for (m1 = 0; m1 < MONLEN-1 && !INMON(c->x, c->y, mons[m1]); m1++);
		mfact[m1] = MIN(0.95, MAX(0.5, (float)WIDTH(c) / mons[m1].mw));
		nmain[m1] = MAX(1, mons[m1].mh / HEIGHT(c));
		tile();
	}
}

void
grabresizecheck(Client *c) {
	int m, x, y;
	unsigned int mask;
	XEvent ev;

	if (!c || c != sel || !MOUSEINF(dwin, x, y, mask)
	|| (mask & (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask))
	|| (!MOVEZONE(c, x, y) && !RESIZEZONE(c, x, y))
	|| (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursize, CurrentTime) != GrabSuccess))
		return;

	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask
			|KeyPressMask, &ev);
		switch(ev.type) {
		case KeyPress:
			XUngrabPointer(dpy, CurrentTime);
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			x = ev.xmotion.x_root;
			y = ev.xmotion.y_root;
			break;
		case ButtonPress:
			if (ev.xbutton.button == Button1) {
				if (MOVEZONE(c, x, y))
					grabresize(&(Arg){.i = DragMove});
				else if (RESIZEZONE(c, x, y)) {
					for (m = 0; m < MONLEN && !INMON(x, y, mons[m]); m++);
					if (!c->isfloating && m < MONLEN
					&& x - c->bw <= mons[m].mx + mfact[m]* mons[m].mw)
						grabresize(&(Arg){.i = DragTile});
					else
						grabresize(&(Arg){.i = DragSize});
				}
			}
		}
	} while (ev.type != ButtonPress && ev.type != ButtonRelease
		&& ev.type != KeyPress && (MOVEZONE(c, x, y) || RESIZEZONE(c, x, y)));
	XUngrabPointer(dpy, CurrentTime);
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
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	for (i = 0; i < LENGTH(keys); i++)
		if (keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state))
			keys[i].func(&(keys[i].arg));
}

void
keyrelease(XEvent *e)
{
	if (stackrelease == XKeycodeToKeysym(dpy, (KeyCode)e->xkey.keycode, 0)) {
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

	c = ecalloc(1, sizeof(Client));
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
		for (m = MONLEN-1; m > 0 && !INMON(x, y, mons[m]); m--);
	else m = 0;
	/* adjust to current monitor */
	if (c->x + WIDTH(c) > mons[m].mx + mons[m].mw)
		c->x = c->fx = mons[m].mx + mons[m].mw - WIDTH(c);
	if (c->y + HEIGHT(c) > mons[m].my + mons[m].mh)
		c->y = c->fy = mons[m].my + mons[m].mh - HEIGHT(c);
	c->x = c->fx = MAX(c->x, mons[m].mx);
	/* only fix client y-offset, if the client center might cover the bar */
	c->y = c->fy =
		MAX(c->y, ((by == mons->my) && (c->x + (c->w / 2) >= mons->mx)
			&& (c->x + (c->w / 2) < mons->mx + mons->mw)) ? bh : mons->my);
	c->bw = borderpx;

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	XSelectInput(dpy, w, PropertyChangeMask|StructureNotifyMask);
	attach(c);
	restack(c, CliRaise);
	PROPEDIT(PropModeAppend, c, NetClientList)
	/* some windows require this */
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h);
	setclientstate(c, NormalState);
	showhide(clients);
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
rawmotion(XEvent *e)
{
	int rx, ry;
	Window cw;
	static Window lastcw = {0};
	static Client *c = NULL;

	if (!MOUSEINF(cw, rx, ry, dui)) return;

	/* top bar raise when mouse hits the screen edge.
	   especially useful for apps that capture the keyboard. */
	if (topbar ? ry <= mons->my : ry >= mons->my + mons->mh - 1
		&& (rx >= mons->mx) && (rx <= mons->mx + mons->mw) && !barfocus) {
		barfocus = 1;
		XRaiseWindow(dpy, barwin);
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	} else if (barfocus) {
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
	if (cw != barwin)
		grabresizecheck(sel);
}

void
resize(Client *c, int x, int y, int w, int h)
{
	if (applysizehints(c, &x, &y, &w, &h))
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
	if (c->isfloating && !c->isfullscreen) {
		c->fx = c->x;
		c->fy = c->y;
		c->fw = c->w;
		c->fh = c->h;
	}
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

	/* order floating layer */
	for (c = clients; c; c = c->next)
		if (c != pinned && c != raised && c->isfloating && !c->isfullscreen) {
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
	/* order fullscreen layer */
	for (c = clients; c; c = c->next)
		if (c != pinned && c != raised && c->isfullscreen) {
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
		for (m1 = MONLEN-1; m1 > 0 && !INMON(c->x, c->y, mons[m1]); m1--);
		for (m2 = 0; m2 < MONLEN
		&& !INMON(c->x + WIDTH(c), c->y + HEIGHT(c), mons[m2]); m2++);
		if (m2 == MONLEN || mons[m2].mx + mons[m2].mw <= mons[m1].mx
		|| mons[m2].my + mons[m2].mh <= mons[m1].my)
			m2 = m1;
		/* apply fullscreen window parameters */
		w = mons[m2].mx - mons[m1].mx + mons[m2].mw;
		h = mons[m2].my - mons[m1].my + mons[m2].mh;
		resizeclient(c, mons[m1].mx, mons[m1].my, w, h);
		restack(c, CliRaise);

	} else if (!fullscreen && c->isfullscreen){
		/* change back to original floating parameters */
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;
		c->isfloating = c->fstate;
		c->bw = c->fbw;
		resizeclient(c, c->fx, c->fy, c->fw, c->fh);
	}
	arrange();
}

void
setup(void)
{
	int i, xre;
	unsigned char xi[XIMaskLen(XI_RawMotion)] = { 0 };
	XIEventMask evm;
	XSetWindowAttributes wa;
	Atom utf8string;

	/* clean up any zombies immediately */
	sigchld(0);

	/* init screen */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
	drw = drw_create(dpy, screen, root, sw, sh);
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.\n");
	lrpad = drw->fonts->h;
	/* init monitor layout */
	if (MONNULL(mons[0])) {
		updatemonitors(NULL);
		/* select xrandr events (if monitor layout isn't hard configured) */
		if (XRRQueryExtension(dpy, &xre, &di)) {
			handler[xre + RRNotify_OutputChange] = updatemonitors,
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
	/* init appearance */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);
	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "dwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* init bars */ {
	bh = drw->fonts->h + 2;
	by = topbar ? mons->my : mons->my + WINH(mons[0]);
		barwin = XCreateWindow(dpy, root, mons->mx, by, mons->mw, bh, 0,
			DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
			CWOverrideRedirect|CWBackPixmap|CWEventMask, &(XSetWindowAttributes){
				.override_redirect = True,
				.background_pixmap = ParentRelative,
				.event_mask = ButtonPressMask|ExposureMask});
		XDefineCursor(dpy, barwin, curpoint);
		XMapRaised(dpy, barwin);
		XSetClassHint(dpy, barwin, &(XClassHint){"dwm", "dwm"});
		updatestatus();
	}
	/* select events */
	wa.cursor = curpoint;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
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
	if (signal(SIGCHLD, sigchld) == SIG_ERR) {
		die("can't install SIGCHLD handler.\n");
	}
	while (0 < waitpid(-1, NULL, WNOHANG));
}

void
spawn(const Arg *arg)
{
	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]);
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
	int nm[MONLEN] = {0}, i[MONLEN] = {0}, my[MONLEN] = {0}, ty[MONLEN] = {0};
	Client *c;

	/* find the number of clients in each monitor */
	for (c = nexttiled(clients); c; c = nexttiled(c->next)) {
		for (m = LENGTH(mons)-1; m > 0 && !WINMON(c, mons[m]); m--);
		nm[m]++;
	}

	/* tile windows into the relevant monitors. */
	for (c = nexttiled(clients); c; c = nexttiled(c->next), i[m]++) {
		/* find the monitor placement again */
		for (m = LENGTH(mons)-1; m > 0 && !WINMON(c, mons[m]); m--);
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
	sel->isfloating = !sel->isfloating || sel->isfixed;
	if (sel->isfloating) {
		resize(sel, sel->fx, sel->fy,
			sel->fw, sel->fh);
	}
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
toggleview(const Arg *arg)
{
	tagset ^= (arg->ui & TAGMASK);
	arrange();
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
	for (i = 0; i < n && i < MONLEN; i++) {
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

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
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
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
	die("dwm: another window manager is already running.\n");
	return -1;
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
		die("dwm-"VERSION"\n");
	else if (argc != 1)
		die("usage: dwm [-v]\n");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display.\n");
	checkotherwm();
	setup();
	scan();
	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
