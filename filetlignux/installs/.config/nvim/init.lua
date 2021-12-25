
vim.opt.tabstop = 2
vim.opt.shiftwidth = 2
vim.opt.colorcolumn = "80"
vim.opt.cursorline = true
vim.opt.wildmenu = true
vim.opt.mouse = "nv"
vim.opt.splitbelow = true
vim.opt.syntax = "on"
vim.opt.termguicolors = true
vim.cmd[[colorscheme moonfly]]
vim.opt.statusline = "%{%&modified?'%#fzf3#':'%#TabLineSel#'%}▔▜█▙ %t ▟█"
	.."▙ %w%Y ▟█▛▔%0* %=%<<%f>%= ↕ %=▔▜%#TermCursor#❨%c❩ %l/%L%0*▛▔"


-- nnn - setup "Nav" vim-function for launching window to browse files and dirs
vim.cmd[[exe "func Nav(...)\n lua nnnLaunch()\n endf"]]
nnnBuf, nnnWin = nil
nnnOpen = vim.fn.tempname()
function nnnLaunch()
	local winStyle = {style = "minimal", relative = "editor", row = 1, col = 0,
		width = 48, height = vim.api.nvim_get_option("lines")-2}
	if not nnnWin and not nnnBuf then
		nnnBuf = vim.api.nvim_create_buf(false, true)
		nnnWin = vim.api.nvim_open_win(nnnBuf, true, winStyle)
		vim.fn.termopen("nnn -G -p "..nnnOpen, {
			on_exit = function(id, code)
				nnnWin = vim.api.nvim_win_close(nnnWin, false)
				nnnBuf = vim.api.nvim_buf_delete(nnnBuf, {})
				for file in io.lines(nnnOpen) do
					vim.cmd("edit "..vim.fn.fnameescape(file))
				end
			end})
		vim.cmd[[startinsert | au BufEnter <buffer> startinsert]]
		vim.cmd[[au CursorMoved <buffer> startinsert]]
		vim.cmd[[au BufLeave <buffer> stopinsert | call Nav()]]
		vim.cmd[[au WinClosed <buffer> stopinsert]]
	elseif not nnnWin then
		nnnWin = vim.api.nvim_open_win(nnnBuf, true, winStyle)
	else
		nnnWin = vim.api.nvim_win_close(nnnWin, false)
	end
end


-- term - setup "Term" vim-function for launching a terminal split.
vim.cmd[[exe "func Term(...)\n lua termLaunch()\n endf"]]
tBuf, tWin, tHeight = nil
function termLaunch()
	if not tWin and not tBuf then
		vim.cmd[[split | exe "terminal" | set nobuflisted]]
		tBuf = vim.api.nvim_get_current_buf()
		tWin = vim.api.nvim_get_current_win()
		vim.cmd[[startinsert | au BufEnter <buffer> startinsert]]
		vim.cmd[[au CursorMoved <buffer> startinsert]]
		vim.cmd[[au BufLeave <buffer> stopinsert]]
		vim.cmd[[au WinClosed <buffer> stopinsert]]
	elseif not tWin or vim.fn.win_id2win(tWin)==0 or vim.fn.winnr('$')==1 then
		vim.cmd[[split]]
		vim.api.nvim_set_current_buf(tBuf)
		tWin = vim.api.nvim_get_current_win()
		pcall(vim.api.nvim_win_set_height, tWin, tHeight)
	elseif tBuf ~= vim.fn.winbufnr(tWin) then
		vim.api.nvim_set_current_win(tWin)
		vim.api.nvim_set_current_buf(tBuf)
	else
		tHeight = vim.api.nvim_win_get_height(tWin)
		tWin = vim.api.nvim_win_close(tWin, false)
	end
end


-- TODO XXX
-- preview mode split window management
-- gitgutter or https://github.com/lewis6991/gitsigns.nvim
-- treesitter and all languages
-- more menu options
-- keyboard shortcut options
-- help for custom keyboard shortcuts


-- menu - setup "Menu" vim-function for launching a helper menu
vim.cmd[[
source $VIMRUNTIME/menu.vim
cnoremap <expr> <up> wildmenumode()? "\<left>":"\<up>"
cnoremap <expr> <down> wildmenumode()? "\<right>":"\<down>"
cnoremap <expr> <left> wildmenumode()? "\<up>":"\<left>"
cnoremap <expr> <right> wildmenumode()? "\<down>":"\<right>"
func Menu(...)
	stopinsert
	call feedkeys(":emenu \t", 't')
endf]]


-- tabline - launcher shortcuts and buffer tabs.
function tabline()
	-- line length and truncation logic
	local c = vim.o.columns-1
	function S(s)
		c = math.max(c - vim.api.nvim_strwidth(s), 0)
		return s
	end
	-- launcher shortcuts
	local r = '%#fzf1#'..'%0@Menu@'..S('≣')..'%#fzf3#'..'%0@Term@'..S('❱')
		..'%#InsertToggle#'..'%0@DoInsert@'..S('I')
		..'%#fzf3#'..'%0@TwoPane@'..S('‖')
		..'%#fzf2#'..'%0@Nav@'..S('+')
		..'%<%='
	-- buffer tabs
	local wid = 50
	for buf = 1, vim.fn.bufnr('$') do
		if vim.fn.buflisted(buf) == 1 and wid>0 then
			local bufn = vim.fn.bufnr()
			local mod = vim.api.nvim_buf_get_option(buf, 'modified')
			local name = vim.fn.fnamemodify(vim.api.nvim_buf_get_name(buf), ':t')
			r=r..(buf==bufn and'%#TabLineSel#%'or'%#TabLine#%')..buf..'@BufSel@'
				..S(mod and'┣'or'┃')..'%0.'..wid..'('..S(name)..'%)'..(buf==bufn and
				(mod and'%#fzf3#%0@BufSave@'..S('✎')or'%#fzf1#%0@BufDel@'..S('✖'))or'')

				wid = buf<=bufn and 50 or c-1
		end
	end
	-- end the tabline
	return r
end
vim.cmd[[exe "func DoInsert(...)\n startinsert\n endf"]]
vim.cmd[[exe "func BufSel(id,c,b,m)\n exe 'b'.a:id\n endf"]]
vim.cmd[[exe "func BufSave(...)\n w\n endf"]]
vim.cmd[[func BufDel(...)
	if len(getbufinfo({'buflisted':1}))==1 | qa | else | bn|bd# | endif
endf]]
vim.cmd[[hi! def link InsertToggle TabLine]]
vim.cmd[[au InsertEnter * hi! def link InsertToggle Search | redrawtabline]]
vim.cmd[[au InsertLeave * hi! def link InsertToggle TabLine | redrawtabline]]
vim.opt.tabline = [[%!v:lua.tabline()]]
vim.opt.showtabline = 2

