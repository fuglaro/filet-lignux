/* See LICENSE file for copyright and license details. */

/* This is a minimal fork to dwm.
 *
 * Changes to dwm focus on default behaviour being more familiar
 * to users of less-leet window managers, while still supporting all
 * productivity boosting behaviours. Consider this like a gateway
 * drug to the beauty of dwm.
 *
 * - There is only a tiled layout but windows will launch in floating mode.
 * - Fullscreen mode for windows replaces the monocle layout.
 * - Less daunting bar arrangement.
 * - Additional window states:
 *     * raised - visible above all other windows, even if tiled.
 *     * zoomed - at the top of the stack (also applies to floating windows).
 * - Changed bindings to be closer to less-leet window managers.
 *     * Alt+Tab combos raise windows temporarily, and then zoom on release.
*/

/* appearance */
static const unsigned int borderpx  = 2;        /* border pixel of windows */
static const unsigned int snap      = 8;       /* snap pixel */
static const int topbar             = 1;        /* 0 means bottom bar */
static const char *fonts[]          = { "monospace:bold:size=4" };
static const char dmenufont[]       = "monospace:size=8";
static const char col_gray1[]       = "#111111";
static const char col_gray2[]       = "#555555";
static const char col_gray3[]       = "#dddddd";
static const char col_gray4[]       = "#ffffff";
static const char col_cyan[]        = "#335577";
static const char col_red[]         = "#dd4422";
static const char *colors[][3]      = {
	/*               fg         bg         border   */
	[SchemeNorm] = { col_gray3, col_gray1, col_gray2 },
	[SchemeSel]  = { col_gray4, col_cyan,  col_red  },
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

/* layout */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */

/* launcher symbol */
static const char lsymbol[] = ">";

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = { "dmenu_run", "-p", ">", "-m", dmenumon, "-i", "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_cyan, "-sf", col_gray4, NULL };
static const char *termcmd[]  = { "~/.config/filetlignux/st/st", NULL };
static const char *lockcmd[]  = { "slock", NULL };
static const char *helpcmd[]  = { "~/.config/filetlignux/st/st",
"-g80x30", "-t", "FiletLignux Controls", "-e",
"bash", "-c", "printf 'FiletLignux Controls\n\
               Alt+`: launcher\n\
         Shift+Alt+`: open terminal\n\
              Ctrl+`: move window with mouse\n\
             Shift+`: resize window with mouse\n\
        Ctrl+Shift+`: tile window (raised)\n\
           Alt+Enter: fullscreen window\n\
             LButton: raise window (zoom if not tiled)\n\
              Alt+F4: close window\n\
        Shift+Alt+F4: lock\n\
   Shift+Ctrl+Alt+F4: quit\n\
\n\
             Alt+Tab: next window (raise and focus, then zoom on release)\n\
       Shift+Alt+Tab: previous window (raise and focus, then zoom on release)\n\
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

/* key definitions */
#define MODKEY Mod1Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

static const KeySym grabstackrelease = XK_Alt_L;
static Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_grave,  spawn,          {.v = dmenucmd } },
	{ MODKEY|ShiftMask,             XK_grave,  spawn,          {.v = termcmd } },
	{ ControlMask,                  XK_grave,  movemouse,      {0} },
	{ ShiftMask,                    XK_grave,  resizemouse,    {0} },
	{ ControlMask|ShiftMask,        XK_grave,  togglefloating, {0} },
	{ MODKEY,                       XK_Return, togglefullscreen, {0} },
	{ MODKEY,                       XK_F4,     killclient,     {0} },
	{ MODKEY|ShiftMask,             XK_F4,     spawn,          {.v = lockcmd} },
	{ MODKEY|ControlMask|ShiftMask, XK_F4,     quit,           {0} },

	{ MODKEY,                       XK_Tab,    grabstack,      {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_Tab,    grabstack,      {.i = -1 } },

	{ MODKEY|ControlMask,           XK_Down,   focusstack,     {.i = +1 } },
	{ MODKEY|ControlMask,           XK_Up,     focusstack,     {.i = -1 } },
	{ MODKEY|ControlMask,           XK_Return, zoom,           {0} },

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

/* button definitions
 * click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin,
 * or ClkRootWin
 */
static Button buttons[] = {
	/* click         event mask    button      function        argument */
	{ ClkLtSymbol,   0,            Button1,    spawn,          {.v = dmenucmd } },
	{ ClkLtSymbol,   0,            Button3,    spawn,          {.v = termcmd } },
	{ ClkStatusText, 0,            Button1,    spawn,          {.v = helpcmd } },

	{ ClkTagBar,     0,            Button1,    view,           {0} },
	{ ClkTagBar,     0,            Button3,    toggleview,     {0} },
	{ ClkTagBar,     MODKEY,       Button1,    tag,            {0} },
	{ ClkTagBar,     MODKEY,       Button3,    toggletag,      {0} },
};

