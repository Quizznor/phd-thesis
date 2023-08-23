
" Colorscheme for big monitor
:colorscheme koehler

" Set line numbering to relative
set number relativenumber

" Support utf-8 encoding
set fileencoding=utf-8
set encoding=utf-8

" see vim-latex for details (mm for full build, nn for quick build)
map mm :! $HOME/scripts/_LaTeXBinaries/vim-latex.sh %:p build full<CR><CR>
map nn :! $HOME/scripts/_LaTeXBinaries/vim-latex.sh %:p build <CR><CR>

" automatically change fontsize when writing stuff
autocmd VimEnter *.tex :silent !xdotool key ctrl+9
autocmd VimLeave *.tex :silent !xdotool key ctrl+0

" VimPlug section
call plug#begin('~/.vim/plugged')

" Ultisnips
let g:UltiSnipsEditSplit="vertical"
let g:UltiSnipsExpandTrigger="<tab>"
let g:UltiSnipsJumpForwardTrigger="<tab>"
let g:UltiSnipsJumpBackwardTrigger="<s-tab>"

Plug 'SirVer/ultisnips'
Plug 'Townk/vim-autoclose'

call plug#end()
