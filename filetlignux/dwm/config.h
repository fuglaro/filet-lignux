/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static const unsigned int snap      = 8;       /* snap pixel */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const char *fonts[]          = { "monospace:size=4" };
static const char dmenufont[]       = "monospace:size=8";
static const char col_gray1[]       = "#222222";
static const char col_gray2[]       = "#444444";
static const char col_gray3[]       = "#bbbbbb";
static const char col_gray4[]       = "#eeeeee";
static const char col_cyan[]        = "#005577";
static const char col_red[]         = "#990000";
static const char *colors[][3]      = {
	/*               fg         bg         border   */
	[SchemeNorm] = { col_gray3, col_gray1, col_gray2 },
	[SchemeSel]  = { col_gray4, col_gray2,  col_red  },
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

static const Rule rules[] = {
	/* xprop(1):
	 *      WM_CLASS(STRING) = instance, class
	 *      WM_NAME(STRING) = title
	 */
	/* class instance title                   tags mask isfloating  monitor */
	{  NULL, NULL,    NULL,                   0,        1,          -1 },
	{  NULL, NULL,    "FiletLignux Controls", 0,        0,          -1 },
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ ">",      tile },    /* first entry is default */
};

/* key definitions */
#define MODKEY Mod1Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-i", "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_cyan, "-sf", col_gray4, NULL };
static const char *termcmd[]  = { "st", NULL };
static const char *lockcmd[]  = { "slock", NULL };
static const char *helpcmd[]  = { "st", "-t", "FiletLignux Controls", "-e",
"bash", "-c", "printf 'FiletLignux Controls\n\
               Alt+`: launcher\n\
         Shift+Alt+`: open terminal\n\
        Shift+Alt+F4: lock\n\
   Shift+Ctrl+Alt+F4: quit\n\
         Alt+LButton: move window\n\
         Alt+MButton: tile window (raise and zoom)\n\
         Alt+RButton: resize window\n\
             LButton: raise window (zoom if not tiled)\n\
\n\
             Alt+Tab: next window (raise and focus, then zoom on release)\n\
       Shift+Alt+Tab: previous window (raise and focus, then zoom on release)\n\
           Alt+Enter: fullscreen window\n\
              Alt+F4: close window\n\
\n\
       Ctrl+Alt+Down: next window (raise and focus)\n\
         Ctrl+Alt+Up: previous window (raise and focus)\n\
      Ctrl+Alt+Enter: raise and zoom window\n\
\n\
           Alt+[1-9]: switch workspace\n\
      Ctrl+Alt+[1-9]: combine workspace\n\
     Shift+Alt+[1-9]: move window to workspace\n\
Shift+Ctrl+Alt+[1-9]: add window to workspace\n\
         Shift+Alt+0: add window to all workspaces\n\
       Ctrl+Alt+Left: switch to previous workspace\n\
      Ctrl+Alt+Right: switch to next workspace\n\
 Shift+Ctrl+Alt+Left: move window and switch to previous workspace\n\
Shift+Ctrl+Alt+Right: move window and switch to next workspace\n\
'; read -s -n 1", NULL };

/* This is a minimal fork to dwm.
 *
 * Changes to dwm focus on default behaviour being more familiar
 * to users of less-leet window managers, while still supporting all
 * productivity boosting behaviours. Consider this like a gateway
 * drug to the beauty of dwm.
 *
 * - There is only a tiled layout but windows will launch in floating mode.
 * - Fullscreen mode for windows replaces the monocle layout. TODO
 * - The topbar auto-hides when raised window is in fullscreen mode. TODO
 * - Less daunting bar arrangement.
 * - Additional window states:
 *     * raised - visible above all other windows, even if tiled. TODO
 *     * zoomed - at the top of the stack (also applies to floating windows). TODO
 * - Changed bindings to be closer to less-leet window managers. TODO
*/

static Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_grave,  spawn,          {.v = dmenucmd } },
	{ MODKEY|ShiftMask,             XK_grave,  spawn,          {.v = termcmd } },
	{ MODKEY|ShiftMask,             XK_F4,     spawn,          {.v = lockcmd} },
	{ MODKEY|ControlMask|ShiftMask, XK_F4,     quit,           {0} },

	{ MODKEY,                       XK_Tab,    focusstack,     {.i = +1 } }, //TODO update
	{ MODKEY|ShiftMask,             XK_Tab,    focusstack,     {.i = -1 } }, //TODO update
	/* TODO{ MODKEY,                       XK_Return, togglefullscreen, {0} }, */
	{ MODKEY,                       XK_F4,     killclient,     {0} },

	{ MODKEY|ControlMask,           XK_Down,   focusstack,     {.i = +1 } }, //TODO update
	{ MODKEY|ControlMask,           XK_Up,     focusstack,     {.i = -1 } }, //TODO update
	{ MODKEY|ControlMask,           XK_Return, zoom,           {0} }, //TODO update

	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	{ MODKEY|ControlMask,           XK_Left,   focusview,      {.i = -1 } },
	{ MODKEY|ControlMask,           XK_Right,  focusview,      {.i = +1 } },
	{ MODKEY|ControlMask|ShiftMask, XK_Left,   moveview,       {.i = -1 } },
	{ MODKEY|ControlMask|ShiftMask, XK_Right,  moveview,       {.i = +1 } },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },

	{ ClkLtSymbol,          0,              Button1,        spawn,          {.v = dmenucmd } },
	{ ClkLtSymbol,          0,              Button3,        spawn,          {.v = termcmd } },
	{ ClkStatusText,        0,              Button1,        spawn,          {.v = helpcmd } },

	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

