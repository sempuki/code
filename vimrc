" Plugins:
" - a.vim                   "automatically switch header/impl files
" - surround.vim            "surround text quotes
" - ragtag.vim              "navigating XML-type script
" - OmniCppComplete         "C++ omnifunc

"use VIM settings
set nocompatible
colorscheme slate

filetype plugin indent on   "autoguess by file extension
set tabstop=4               "number of spaces in a tab, used in expandtab
set softtabstop=4           "number of spaces in a tab, used in smarttab
set shiftwidth=4            "used in shifting or C indenting
set expandtab               "insert spaces for tabs
set smarttab                "somewhat smarter tab control
set cindent                 "C indenting
"set cinwords                "indent after words
"set cinkeys                 "indent after keys
set number                  "print line numbers
set incsearch               "incremental search
set hlsearch                "highlight search matches
set ruler                   "show the cursor
set wildmenu                "print menu completions
set autowriteall            "write buffer to file when switching
set scrolloff=5             "keep lines of context when scrolling
set foldmethod=syntax       "fold according to syntax hl rules
set foldlevel=99            "default to all open
set matchpairs+=<:>         "match angle brackets
set splitright              "split in empty space to the right
set diffopt+=context:100    "turn off folding context in diffs
set t_vb=                   "turn off annoying bells

"syntax highlighting always on
if !exists ("syntax_on")
    syntax on
endif

"multi-platform support
if has("unix")
    let FIND="find . -name "
    let TAGS="ctags"
else
    let FIND="dir /b /s "
    let TAGS="ctags.exe"
endif

"generate local C++ tags files
nnoremap <silent> <C-\>+ :exe "!".TAGS." -R --languages=C++ --c++-kinds=+p --fields=+iaS --extra=+q -f ".getcwd()."/cpp.tags"<CR>
set tags+=./cpp.tags,cpp.tags

"generate local C# tags files
nnoremap <silent> <C-\># :exe "!".TAGS.' -R --languages=C\# --c\#-kinds=cimnp --fields=+ianmzS --extra=+fq -f '.getcwd()."/cs.tags"<CR>
set tags+=./cs.tags,cs.tags

"look for global tags files
if filereadable ("C:/Code/")
    set tags+="C:/Code/"
endif
if filereadable ($HOME."/Code/")
    let tags+=$HOME."/Code/"
endif

" OmniCppComplete
set completeopt-=preview    "disable annoying window
let OmniCpp_ShowPrototypeInAbbr = 1 " show function parameters
let OmniCpp_MayCompleteScope = 1 " autocomplete after ::
let OmniCpp_DefaultNamespaces   = ["std", "_GLIBCXX_STD"]

"toggle .{c|cpp}/.{h|hpp}
nnoremap <C-A> :A<CR>

"next/prev buffer
nnoremap <silent> <C-N> :bnext<CR>
nnoremap <silent> <C-P> :bprevious<CR>

"save buffer
noremap <silent> <F1> <Esc>:w<CR>
noremap! <silent> <F1> <Esc>:w<CR>

"cancel highlighting
nnoremap <silent> <C-C> <C-C>:nohl<CR>

"insert newline
nnoremap <C-J> i<CR><Esc>==

"search+replace word under cursor
nnoremap <C-S> :,$s/\<<C-R><C-W>\>/

"quickfix window
nnoremap <C-Q><C-W> :copen<CR>
nnoremap <C-Q><C-Q> :cclose<CR>

"vimgrep word under cursor
nnoremap <C-G> :vimgrep /\<<C-R><C-W>\>/gj %:h

"find file with quickfix integration
nnoremap <C-F> :cgetexpr system(FIND."")

" Windows GUI tweaks
if has("gui_win32")
    autocmd GUIEnter * :simalt ~x
    set guifont=DejaVu_Sans_Mono:h8
    set guioptions-=tT
endif

"p4 integration
" nnoremap <C-E> :!p4 edit %<CR>

"text editing

"set ignorecase              "only for smartcase below
"set smartcase               "if no caps, case insensitive
"set autochdir               "change CWD to file in the buffer
"set spell                   "enable spell checking

"add dictionary to ^N completion
"set dictionary+=/usr/share/dict/words
"set complete+=k

"list of file encodings to try
"set fileencodings=iso-2022-jp,ucs-bom,utf8,sjis,euc-jp,latin1
