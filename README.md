# filet-lignux

A developer's linux configuration - a palatable lean slice.

Inspired by dwm, and largely forked from it. This project explores minimalism while still providing the full power of a desktop environment. It is not for everyone, and it is the result of strong opinions, such as:
* **Complexity must justify itself**.
* Lightweight is better than heavyweight.
* Select your dependencies wisely: they are complexity, but not using them, or using the wrong ones, can lead to worse complexity.
* Powerful features are good, but simplicity and clarity are essential.
* Adding layers of simplicity, to avoid understanding something useful, only adds complexity, and is a trap for learning trivia instead of knowledge.
* Steep learning curves are dangerous, but don't just push a vertical wall deeper; learning is good, so make the incline gradual for as long as possible.
* Allow other tools to thrive - e.g: terminals don't need tabs or scrollback, that's what tmux is for.

## X11 vs Wayland

This is built on X11, not Wayland, for no other reason than timing. Shortly after this project was started, NVIDIA support for Wayland was announced. This project will not include Wayland support due to the inevitable complexities of concurrently supporting multiple interfaces. When the timing is right, this will fork into a new project which can move in the direction of Wayland.

## DWM

The heart of this project is a fork of dwm. Dwm is breathtakingly beautiful. This started as a programming exercise to see if I could take something already very well placed in terms of simplicity and elegance, and make it even simpler, inside and out, without significantly moving away from powerful features. It ended up steering in a significantly different direction largely from the opinions stated above. I would best describe it now as dwm with a clean and simple user interface that is welcoming to users familiar with tradition window managers, whilst still holding on to powerful features.

Significant changes:
* More familiar and standard window manager bar.
* Mouse 

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

# Thanks to, grateful forks, and contributions

We stand on the shoulders of giants. They own this, far more than I do.

* https://archlinux.org/
* https://github.com
* https://github.com/torvalds/linux
* https://dwm.suckless.org
* https://st.suckless.org
* https://st.suckless.org/patches/nordtheme
* https://st.suckless.org/patches/delkey
* https://tools.suckless.org/slock
* https://tools.suckless.org/dmenu/
* https://tools.suckless.org/dmenu/patches/mouse-suppor/
* https://www.vim.org
* https://github.com/tmux/tmux
* https://www.texturex.com/fractal-textures/fractal-design-picture-wallpaper-stock-art-image-definition-free-neuron-chaos-fractal-fracture-broken-synapse-texture/
* https://gist.github.com/palopezv/efd34059af6126ad970940bcc6a90f2e
* https://keithp.com/blogs/Cursor_tracking/
* https://www.jetbrains.com/lp/mono/
* https://github.com/akinozgen/dmenu_applications
* https://github.com/march-linux/mimi
