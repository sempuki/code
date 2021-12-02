" Derived from slate; cterm values are unchanged as a fallback.
:set background=dark
:highlight clear
if version > 580
  hi clear
  if exists("syntax_on")
    syntax reset
  endif
endif
let colors_name = "slate-mine"
" General UI highlighting
:hi Normal          guifg=White       guibg=Grey15
:hi Cursor          guifg=SlateGrey   guibg=Khaki       
:hi VertSplit       guifg=Grey40      guibg=#c2bfa5             cterm=reverse
:hi Folded          guifg=Grey40      guibg=Black               ctermfg=Grey ctermbg=DarkGrey
:hi FoldColumn      guifg=Grey20      guibg=Black               ctermfg=4 ctermbg=7
:hi IncSearch       guifg=Green       guibg=Black               cterm=none ctermfg=Yellow ctermbg=Green
:hi ModeMsg         guifg=GoldenRod                             cterm=none ctermfg=Brown
:hi MoreMsg         guifg=SeaGreen                              ctermfg=DarkGreen
:hi WarningMsg      guifg=Salmon                                ctermfg=1
:hi NonText         guifg=RoyalBlue   guibg=Grey12              cterm=bold ctermfg=Blue
:hi Question        guifg=SpringGreen                           ctermfg=Green
:hi Search          guifg=Wheat       guibg=Peru                cterm=none ctermfg=Grey ctermbg=Blue
:hi SpecialKey      guifg=YellowGreen                           ctermfg=DarkGreen
:hi StatusLine      guifg=Black       guibg=#c2bfa5   gui=bold  cterm=bold,reverse
:hi StatusLineNC    guifg=Grey40      guibg=#c2bfa5             cterm=reverse
:hi Title           guifg=Gold                                  cterm=bold ctermfg=Yellow
:hi Visual          guifg=Khaki       guibg=OliveDrab           cterm=reverse
:hi Directory                                                   ctermfg=DarkCyan
:hi ErrorMsg        guifg=White guibg=Red                       cterm=bold cterm=bold ctermfg=7 ctermbg=1
:hi WildMenu                                                    ctermfg=0 ctermbg=3
:hi DiffAdd                                                     ctermbg=4
:hi DiffChange                                                  ctermbg=5
:hi DiffDelete                                                  cterm=bold ctermfg=4 ctermbg=6
:hi DiffText                                                    cterm=bold ctermbg=1
:hi Underlined                                                  cterm=underline ctermfg=5
:hi Error           guifg=White guibg=Red                       cterm=bold ctermfg=7 ctermbg=1
:hi SpellErrors     guifg=White guibg=Red                       cterm=bold ctermfg=7 ctermbg=1
" Syntax highlighting
:hi Statement       guifg=Green                                 ctermfg=LightBlue
:hi String          guifg=SkyBlue                               ctermfg=DarkCyan
:hi Comment         guifg=Grey40                                term=bold ctermfg=11 
:hi Constant        guifg=#ffa0a0                               ctermfg=Brown
:hi Special         guifg=DarkKhaki                             ctermfg=Brown
:hi Identifier      guifg=Salmon                                ctermfg=Red
:hi Include         guifg=Red                                   ctermfg=Red
:hi PreProc         guifg=Red         guibg=White               ctermfg=Red
:hi Operator        guifg=OrangeRed                             ctermfg=Red
:hi Define          guifg=Gold                      gui=bold    ctermfg=Yellow
:hi Type            guifg=CornflowerBlue                        ctermfg=2
:hi Function        guifg=NavajoWhite                           ctermfg=Brown
:hi Structure       guifg=Green                                 ctermfg=Green
:hi LineNr          guifg=Grey50                                ctermfg=3
:hi Ignore          guifg=Grey40                                cterm=bold ctermfg=7
:hi Todo            guifg=OrangeRed   guibg=Yellow2
