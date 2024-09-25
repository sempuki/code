#!/bin/bash
ssh $@ -t tmux new 'nvim -u ~/.vimrc --cmd ":cd ~/Work/av/ | :set nonumber | :exe \"term\" | :vsplit | :exe \"term\" | :split | :exe \"term\" | :tabnew | :set number"'
