#!/bin/bash
tmux new 'nvim -u ~/.vimrc --cmd ":cd ~/Repos/code/ | :set nonumber | :exe \"term\" | :vsplit | :exe \"term\" | :split | :exe \"term\" | :tabnew | :set number"'
