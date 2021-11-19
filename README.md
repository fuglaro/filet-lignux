# filet-lignux

A developer's linux configuration - a palatable lean slice.

See https://github.com/fuglaro/filet-wm, a core component of this project.
![](https://raw.githubusercontent.com/fuglaro/filet-wm/main/filetwm-demo.gif)

This project explores minimalism while still providing the full power of a desktop environment. It is not for everyone, and it is the result of strong opinions, such as:
* **Complexity must justify itself**.
* Lightweight is better than heavyweight.
* Select your dependencies wisely: they are complexity, but not using them, or using the wrong ones, can lead to worse complexity.
* Powerful features are good, but simplicity and clarity are essential.
* Adding layers of simplicity, to avoid understanding something useful, only adds complexity, and is a trap for learning trivia instead of knowledge.
* Steep learning curves are dangerous, but don't just push a vertical wall deeper; learning is good, so make the incline gradual for as long as possible.
* Allow other tools to thrive - e.g: terminals don't need tabs or scrollback, that's what tmux is for.
* Fix where fixes belong - don't work around bugs in other applications, contribute to them, or make something better.
* Improvement via reduction is sometimes what a project desperately needs, because we do so tend to just add. (https://www.theregister.com/2021/04/09/people_complicate_things/)

## Installation

Note that this will move aside some configuration files in your home
directory such as .vimrc and .tmux.conf.

```bash
git clone git@github.com:fuglaro/filet-lignux.git ~/filet-lignux
cd ~/filet-lignux
./install
```

To also install the mouse keys, this must be installed to the system via sudo:

```bash
sudo ./sysinstall
```
## Launching

```bash
~/.config/filetlignux/start
```

# Thanks to, grateful forks, and contributions

We stand on the shoulders of giants. They own this, far more than I do.

* https://archlinux.org
* https://github.com
* https://github.com/torvalds/linux
* https://www.x.org/wiki/XorgFoundation
* https://dwm.suckless.org
* https://alacritty.org
* https://github.com/eendroroy/alacritty-theme
* https://tools.suckless.org/slock
* https://github.com/fuglaro/filet-wm
* https://www.vim.org
* https://github.com/tmux/tmux
* https://www.texturex.com/fractal-textures/fractal-design-picture-wallpaper-stock-art-image-definition-free-neuron-chaos-fractal-fracture-broken-synapse-texture
* https://keithp.com/blogs/Cursor_tracking
* https://www.jetbrains.com/lp/mono
* https://github.com/march-linux/mimi
