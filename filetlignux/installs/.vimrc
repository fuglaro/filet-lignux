set encoding=utf-8

" Prettier search highlight background color "
set hlsearch
hi Search cterm=NONE ctermfg=black ctermbg=red

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
set backupdir=~/.local/share/vim
set directory=~/.local/share/vim
set undodir=~/.local/share/vim

" Disable auto-formatting "
set paste

" Status display "
set ruler
