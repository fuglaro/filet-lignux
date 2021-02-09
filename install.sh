#!/bin/bash

mkdir -p ~/.config
rsync -a filetlignux/ ~/.config/filetlignux

touch ~/.xprofile
awk -vl=\
'xrdb -merge ~/.config/filetlignux/xorg/solarized.Xresources'\
    '$0=l {o==1} END {if(!o) print l} 1' ~/.xprofile > ~/.xprofile

