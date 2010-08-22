" Plugins:
" - a.vim                   "automatically switch header/impl files
" - surround.vim            "surround text quotes
" - ragtag.vim              "navigating XML-type script
" - OmniCppComplete         "C++ omnifunc
" $ ctags -R --sort=yes --c++-kinds=+p --fields=+iaS --extra=+q --language-force=C++ -f tags .

"use VIM settings
set nocompatible
colorscheme slate

"list of file encodings to try
"set fileencodings=iso-2022-jp,ucs-bom,utf8,sjis,euc-jp,latin1

"change CWD to the dir of the file in the buffer
":autocmd BufEnter * cd %:p:h

filetype plugin indent on   "autoguess by file extension
set tabstop=4               "number of spaces in a tab, used in expandtab
set softtabstop=4           "number of spaces in a tab, used in smarttab
set shiftwidth=4            "used in shifting or C indenting    
set expandtab               "insert spaces for tabs
set smarttab                "somewhat smarter tab control
set cindent                 "C indenting
set number                  "print line numbers
set incsearch               "incremental search
set hlsearch                "highlight search matches
"set ignorecase              "only for smartcase below
"set smartcase               "if no caps, case insensitive
set ruler                   "show the cursor
set wildmenu                "print menu completions
set autowrite               "write buffer to file when switching
set scrolloff=5             "keep lines of context when scrolling
set foldmethod=syntax       "fold according to syntax hl rules
set foldlevel=99            "default to all open
set matchpairs+=<:>         "match angle brackets
set novisualbell            "no annoying bell

"list of places to look for tags
set tags+=$HOME/Code/tags

" OmniCppComplete
set completeopt-=preview    "disable annoying window
let OmniCpp_ShowPrototypeInAbbr = 1 " show function parameters
let OmniCpp_MayCompleteScope = 1 " autocomplete after ::
let OmniCpp_DefaultNamespaces   = ["std", "_GLIBCXX_STD"]

"set cinwords
"set cinkeys

"add dictionary to ^N completion
"set dictionary+=/usr/share/dict/words
"set complete+=k        

"next/prev buffer
nnoremap <silent> <C-N> :bnext<CR>
nnoremap <silent> <C-P> :bprevious<CR>

"save buffer
noremap <F1> <Esc>:w<CR>
noremap! <F1> <Esc>:w<CR>

"cancel highlighting
nnoremap <C-C> :nohl<CR>

"insert newline
nnoremap <C-J> i<CR><Esc>==

"search+replace word under cursor
nnoremap <C-S> :,$s/\<<C-R><C-W>\>/

" Switch on syntax highlighting if it wasn't on yet.
if !exists("syntax_on")
    syntax on
endif
