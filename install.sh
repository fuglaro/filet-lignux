#!/bin/bash -e

# Install base code.
mkdir -p ~/.config
rsync -a --delete filetlignux/ ~/.config/filetlignux

# Install Xorg color scheme.
touch ~/.xprofile
awk -vl=\
'xrdb -merge ~/.config/filetlignux/xorg/solarized.Xresources'\
    '$0=l {o==1} END {if(!o) print l} 1' ~/.xprofile > ~/.xprofile

# Build dwm (forked).
cd ~/.config/filetlignux/dwm/
make clean
make
cd -

# Build st (forked).
cd ~/.config/filetlignux/st/
make clean
make
cd -

