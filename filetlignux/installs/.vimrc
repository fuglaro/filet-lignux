set encoding=utf-8
set fileencoding=utf-8

" Key combinations "
" --normal mode--
"  space,l : toggle hidden characters
nnoremap <SPACE>l :set list! list?<CR>
"  space,p : toggle auto-formatting
nnoremap <SPACE>p :set paste! paste?<CR>

" Hidden character management "
set tabstop=2
set shiftwidth=2
set list listchars=tab:\|\ ,trail:‹

" Prettier search highlight background color "
set hlsearch
hi Search cterm=NONE ctermbg=black

" Highlight the 80th column "
set colorcolumn=80
hi ColorColumn cterm=NONE ctermbg=black

" Cross instance state "
set viminfo='10,/10,:10,n~/.vim/.viminfo

" Restore cursor position "
function! ResCur()
  if line("'\"") <= line("$")
    normal! g`"
    return 1
  endif
endfunction
augroup resCur
  autocmd!
  autocmd BufWinEnter * call ResCur()
augroup END

" Swap file location etc "
set backupdir=~/.vim
set directory=~/.vim
set undodir=~/.vim

" Disable auto-formatting "
set paste

" Status display "
set ruler
