#!/bin/bash
REMOTE_CWD=${1}
shift # Pop off first argument.
ssh ${@} -t tmux new 'nvim -u ~/.vimrc --cmd ":cd '${REMOTE_CWD}' | :set nonumber | :set shell=/bin/bash | :exe \"term\" | :vsplit | :exe \"term\" | :split | :exe \"term\" | :tabnew | :set number"'
