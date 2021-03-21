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
static const char *lockcmd[] = { "slock", NULL };
static const char *upvol[]   = { "volumeup", NULL };
static const char *downvol[] = { "volumedown", NULL };
static const char *mutevol[] = { "volumemute", NULL };
static const char *ssleep[]  = { "sussleep", NULL };
static const char *dimup[]   = { "dimup", NULL };
static const char *dimdown[] = { "dimdown", NULL };
static const char *helpcmd[] = { "st", "-g68x24", "-t", "FiletLignux Controls",
"-e", "bash", "-c", "printf 'FiletLignux Controls\n\
                     Alt+`: launcher\n\
               Shift+Alt+`: terminal\n\
                 LeftClick: raise window\n\
                   Shift+`: move window\n\
                    Ctrl+`: resize window\n\
              Shift+Ctrl+`: tile window\n\
                 Alt+Enter: fullscreen window\n\
            Ctrl+Alt+Enter: raise window\n\
      Shift+Ctrl+Alt+Enter: pin window\n\
           (Shift+)Alt+Tab: (prev/)next window, and raise\n\
          Ctrl+Alt+Up/Down: prev/next window\n\
       Ctrl+Alt+Left/Right: switch to prev/next workspace\n\
 Shift+Ctrl+Alt+Left/Right: move with window to prev/next workspace\n\
                 Alt+[1-9]: switch workspace\n\
            Ctrl+Alt+[1-9]: combine workspace\n\
           Shift+Alt+[1-9]: move window to workspace\n\
      Shift+Ctrl+Alt+[1-9]: add window to workspace\n\
               Shift+Alt+0: add window to all workspaces\n\
                    Alt+F4: close window\n\
              Shift+Alt+F4: lock\n\
             Shift+Ctrl+F4: sleep\n\
         Shift+Ctrl+Alt+F4: quit\n'; read -s -n 1", NULL };

/* key definitions */
#define MODKEY Mod1Mask
#define TAGKEYS(KEY,TAG) \
	{                            MODKEY, KEY, view, {.ui = 1 << TAG} }, \
	{                MODKEY|ControlMask, KEY, toggleview, {.ui = 1 << TAG} }, \
	{                  MODKEY|ShiftMask, KEY, tag, {.ui = 1 << TAG} }, \
	{      MODKEY|ControlMask|ShiftMask, KEY, toggletag, {.ui = 1 << TAG} },

static const KeySym stackrelease = XK_Alt_L;
static Key keys[] = {
	/*                        modifier / key, function / argument */
	{                        MODKEY, XK_grave, spawn, {.v = dmenucmd } },
	{              MODKEY|ShiftMask, XK_grave, spawn, {.v = termcmd } },
	{                     ShiftMask, XK_grave, grabresize, {.i = DragMove } },
	{                   ControlMask, XK_grave, grabresize, {.i = DragSize } },
	{         ControlMask|ShiftMask, XK_grave, togglefloating, {0} },
	{                       MODKEY, XK_Return, togglefullscreen, {0} },
	{           MODKEY|ControlMask, XK_Return, zoom, {0} },
	{ MODKEY|ControlMask|ShiftMask, XK_Return, pin, {0} },
	{                          MODKEY, XK_Tab, grabstack, {.i = +1 } },
	{                MODKEY|ShiftMask, XK_Tab, grabstack, {.i = -1 } },
	{             MODKEY|ControlMask, XK_Down, focusstack, {.i = +1 } },
	{               MODKEY|ControlMask, XK_Up, focusstack, {.i = -1 } },
	{             MODKEY|ControlMask, XK_Left, viewshift, {.i = -1 } },
	{            MODKEY|ControlMask, XK_Right, viewshift, {.i = +1 } },
	{   MODKEY|ControlMask|ShiftMask, XK_Left, viewtagshift, {.i = -1 } },
	{  MODKEY|ControlMask|ShiftMask, XK_Right, viewtagshift, {.i = +1 } },
	TAGKEYS( XK_1, 0)
	TAGKEYS( XK_2, 1)
	TAGKEYS( XK_3, 2)
	TAGKEYS( XK_4, 3)
	TAGKEYS( XK_5, 4)
	TAGKEYS( XK_6, 5)
	TAGKEYS( XK_7, 6)
	TAGKEYS( XK_8, 7)
	TAGKEYS( XK_9, 8)
	{                  MODKEY|ShiftMask, XK_0, tag, {.ui = ~0 } },
	{                           MODKEY, XK_F4, killclient, {0} },
	{                 MODKEY|ShiftMask, XK_F4, spawn, {.v = lockcmd } },
	{            ShiftMask|ControlMask, XK_F4, spawn, {.v = ssleep } },
	{     MODKEY|ControlMask|ShiftMask, XK_F4, quit, {0} },
	{              0, XF86XK_AudioLowerVolume, spawn, {.v = downvol } },
	{                     0, XF86XK_AudioMute, spawn, {.v = mutevol } },
	{              0, XF86XK_AudioRaiseVolume, spawn, {.v = upvol } },
	{                         0, XF86XK_Sleep, spawn, {.v = ssleep } },
	{               0, XF86XK_MonBrightnessUp, spawn, {.v = dimup } },
	{             0, XF86XK_MonBrightnessDown, spawn, {.v = dimdown } },
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

