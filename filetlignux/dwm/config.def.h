/* See LICENSE file for copyright and license details. */

/* This is a minimal fork to dwm, aiming to be smaller, simpler
 * and friendlier.
 *
 * Changes to dwm focus on default behaviour being more familiar
 * to users of less-leet window managers, while still supporting
 * productivity boosting behaviours. Consider this like a gateway
 * drug to the beauty of dwm - friendly to noob and leet alike.
 *
 * - There is only a tiled layout but windows will launch in floating mode.
 * - Monitor association of windows based on floating mode position.
 * - Fullscreen mode for windows replaces the monocle layout.
 * - Less daunting bar arrangement.
 * - Changed bindings to be closer to less-leet window managers.
 *     * Alt+Tab combos raise windows temporarily, and then zoom on release.
 * - Mouse resize movements triggered by keys not buttons.
 * - Mouse resize controls at the edge of windows. */

#include <X11/XF86keysym.h>

#define SELBLUE "#335577"
#define SELRED "#dd4422"
#define FOREGREY "#dddddd"
#define MIDGREY "#555555"
#define BACKGREY "#111111"

/* appearance */
static const unsigned int borderpx  = 1; /* border pixel of windows */
static const unsigned int snap      = 8; /* snap pixel */
static const int topbar             = 1; /* 0 means bottom bar */
static const Time zenmode           = 3; /* if set, delays showing rapid
                                            sequences of client triggered
                                            window title changes until the
                                            next natural refresh. */
static const char font[]            = "monospace:bold:size=4";
static const char dmenufont[]       = "monospace:size=5";
                             /* fg        bg     highlight border sel-border */
static const char *colors[] = { FOREGREY, BACKGREY, SELBLUE, MIDGREY, SELRED };

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

/* layout
 * Set mons to the number of monitors you want supported.
 * Initialise with {0} for autodetection of monitors,
 * otherwise set the position and size ({x,y,w,h}).
 * The first monitor will be the primary monitor and have the bar.
 * e.g:
static Monitor mons[] = {
	{2420, 0, 1020, 1080},
	{1920, 0, 500, 1080},
	{3440, 0, 400,  1080}
};
 * or to autodetect up to 3 monitors:
static Monitor mons[] = {{0}, {0}, {0}};
*/
static Monitor mons[] = {{0}};
/* factor of main area size [0.05..0.95] (for each monitor) */
static float mfact[] = {0.6};
/* number of clients in main area (for each monitor) */
static int nmain[] = {1};
static const int resizehints = 1;
                           /* 1 means respect size hints in tiled resizals */

/* launcher symbol */
static const char lsymbol[] = ">";

/* commands */
static const char *dmenucmd[] = {
	"launcher", dmenufont, FOREGREY, BACKGREY, SELBLUE, NULL };
static const char *termcmd[] = { "st", NULL };
static const char *upvol[]   = { "volumeup", NULL };
static const char *downvol[] = { "volumedown", NULL };
static const char *mutevol[] = { "volumemute", NULL };
static const char *ssleep[]  = { "sussleep", NULL };
static const char *dimup[]   = { "dimup", NULL };
static const char *dimdown[] = { "dimdown", NULL };
static const char *helpcmd[] = { "st", "-g68x24", "-t", "FiletLignux Controls",
"-e", "bash", "-c", "printf 'FiletLignux Controls\n\
                   Win+Tab: launcher\n\
             Win+Shift+Tab: terminal\n\
                 Win+Space: move window\n\
             Win+Alt+Space: resize window\n\
            Win+Ctrl+Space: tile window\n\
                 Alt+Enter: fullscreen window\n\
                 Win+Enter: pin window\n\
             Win+Alt+Enter: raise window\n\
           (Shift+)Alt+Tab: switch window, and raise\n\
               Win+Up/Down: switch window\n\
            Win+Left/Right: switch workspace\n\
      Win+Shift+Left/Right: switch workspace with window\n\
                 Win+[1-9]: switch workspace\n\
           Win+Shift+[1-9]: move window to workspace\n\
                 Alt+[1-9]: add window to workspace\n\
                     Alt+0: add window to all workspaces\n\
                    Alt+F4: close window\n\
                    Win+F4: sleep\n\
         Shift+Ctrl+Alt+F4: quit\n'; read -s -n 1", NULL };

/* key definitions */
#define AltMask Mod1Mask
#define WinMask Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{                  WinMask, KEY, view, {.ui = 1 << TAG} }, \
	{        WinMask|ShiftMask, KEY, tag, {.ui = 1 << TAG} }, \
	{                  AltMask, KEY, toggletag, {.ui = 1 << TAG} },

static const KeySym stackrelease = XK_Alt_L;
static Key keys[] = {
	/*               modifier / key, function / argument */
	{               WinMask, XK_Tab, spawn, {.v = dmenucmd } },
	{     WinMask|ShiftMask, XK_Tab, spawn, {.v = termcmd } },
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
	TAGKEYS( XK_1, 0)
	TAGKEYS( XK_2, 1)
	TAGKEYS( XK_3, 2)
	TAGKEYS( XK_4, 3)
	TAGKEYS( XK_5, 4)
	TAGKEYS( XK_6, 5)
	TAGKEYS( XK_7, 6)
	TAGKEYS( XK_8, 7)
	TAGKEYS( XK_9, 8)
	{                 AltMask, XK_0, tag, {.ui = ~0 } },
	{                AltMask, XK_F4, killclient, {0} },
	{                WinMask, XK_F4, spawn, {.v = ssleep } },
	{ AltMask|ControlMask|ShiftMask, XK_F4, quit, {0} },
	{    0, XF86XK_AudioLowerVolume, spawn, {.v = downvol } },
	{           0, XF86XK_AudioMute, spawn, {.v = mutevol } },
	{    0, XF86XK_AudioRaiseVolume, spawn, {.v = upvol } },
	{               0, XF86XK_Sleep, spawn, {.v = ssleep } },
	{     0, XF86XK_MonBrightnessUp, spawn, {.v = dimup } },
	{   0, XF86XK_MonBrightnessDown, spawn, {.v = dimdown } },
};

/* bar actions */
static Button buttons[] = {
	/* click,      button, function / argument */
	{ ClkLauncher, Button1, spawn, {.v = dmenucmd } },
	{ ClkWinTitle, Button1, focusstack, {.i = +1 } },
	{ ClkWinTitle, Button3, focusstack, {.i = -1 } },
	{ ClkStatus,   Button1, spawn, {.v = helpcmd } },
	{ ClkTagBar,   Button1, view, {0} },
	{ ClkTagBar,   Button3, tag, {0} },
};

