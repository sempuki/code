" Plugins:
" - a.vim                   "automatically switch header/impl files
" - gtags.vim               "GNU Global tags (with CTAGSFORCECPP)
" - matchit.vim             "extend vim's matching operator
" - OmniCppComplete         "C++ omnifunc

"use VIM settings
set nocompatible
colorscheme slate

filetype plugin indent on   "autoguess by file extension
set undofile                "persistent undo
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
set diffopt+=context:999    "turn off folding context in diffs
set noerrorbells            "turn off annoying bells
set t_vb=                   "turn off annoying blinking
let mapleader=","           "comma is more convenient

"syntax highlighting always on
if !exists ("syntax_on")
    syntax on
endif

"multi-platform support
if has("unix")
    let find="find . -name "
else
    let find="dir /b /s "
endif

" Windows GUI tweaks
if has("gui_win32")
    autocmd GUIEnter * :simalt ~x
    set guifont=DejaVu_Sans_Mono:h8
    set guioptions-=tT
endif

"quit without saving
"noremap <silent> <C-Q><C-Q> <Esc>:qa!<CR>
"noremap! <silent> <C-Q><C-Q> <Esc>:qa!<CR>

"save buffer
noremap <silent> <Leader>w <Esc><C-C>:w<CR>
noremap <silent> <F1> <Esc><C-C>:w!<CR>
noremap! <silent> <F1> <Esc><C-C>:w!<CR>

"!cancel highlighting
nnoremap <silent> <C-C> <C-C>:nohl<CR>

"next/prev/delete buffer
nnoremap <silent> <C-N> :bnext<CR>
nnoremap <silent> <C-P> :bprevious<CR>
nnoremap <silent> <C-X> :bdelete!<CR>

"ctag searching
nnoremap <Leader>t :tjump 
nnoremap t] :tnext 
nnoremap t[ :tprev 

"gtag searching
nnoremap <Leader>g :Gtags 
nnoremap <Leader>p :Gtags -P 
nnoremap <Leader>r :Gtags -r <C-R><C-W><CR>
nnoremap <Leader>s :Gtags -s <C-R><C-W><CR>
nnoremap <Leader>f :Gtags -f %<CR>

"quickfix window
nnoremap <Leader>q :copen<CR>
nnoremap <Leader>Q :cclose<CR>
nnoremap ]q :cnext<CR>
nnoremap [q :cprevious<CR>

"window resizing
nnoremap <C-u> :resize +10<CR>
nnoremap <C-d> :resize -10<CR>

"toggle .{c|cpp}/.{h|hpp}
nnoremap <silent> <C-A> :A<CR>

"insert newline
nnoremap <C-J> i<CR><Esc>==

"search+replace word under cursor
nnoremap <C-s> :,$s/\<<C-R><C-W>\>/

"vimgrep word under cursor
nnoremap <C-g> :vimgrep /\<<C-R><C-W>\>/gj %:h

"find file with quickfix integration
nnoremap <C-f> :cgetexpr system(find."")

"generate local C++ tags files
nnoremap <silent> <Leader>+ :!ctags -R --languages=C++ --c++-kinds=+p --fields=+iamzS --extra=+fq -f cpp.tags<CR>
set tags+=./cpp.tags,cpp.tags

"generate local C# tags files
nnoremap <silent> <Leader># :!ctags -R --languages=C\# --c\#-kinds=cimnp --fields=+iamzS --extra=+fq -f cs.tags<CR>
set tags+=./cs.tags,cs.tags

"look for global tags files
if filereadable ("C:/Code/")
    set tags+="C:/Code/"
endif
if filereadable ($HOME."/Code/")
    let tags+=$HOME."/Code/"
endif

"C++ OmniCppComplete
set completeopt-=preview            "disable annoying window
let OmniCpp_ShowPrototypeInAbbr = 1 "show function parameters
let OmniCpp_MayCompleteScope = 1    "autocomplete after ::
let OmniCpp_DefaultNamespaces = ["std", "_GLIBCXX_STD"]

"text editing

"list of file encodings to try
set fileencodings=iso-2022-jp,ucs-bom,utf8,sjis,euc-jp,latin1

nnoremap <Leader>k :setlocal spell<CR>
nnoremap <Leader>K :setlocal nospell<CR>

"set ignorecase              "only for smartcase below
"set smartcase               "if no caps, case insensitive
"set autochdir               "change CWD to file in the buffer

"add dictionary to ^N completion
"set dictionary+=/usr/share/dict/words
"set complete+=k
