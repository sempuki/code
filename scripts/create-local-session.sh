#!/bin/bash
tmux new 'nvim -u ~/.vimrc --cmd ":cd '${1}' | :set nonumber | :set shell=/bin/bash | :exe \"term\" | :vsplit | :exe \"term\" | :split | :exe \"term\" | :tabnew | :set number"'
