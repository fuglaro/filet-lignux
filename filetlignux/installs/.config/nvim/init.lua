
vim.opt.compatible = false
vim.opt.tabstop = 2
vim.opt.shiftwidth = 2
vim.opt.colorcolumn = "80"
vim.opt.number = true
vim.opt.wildmenu = true
vim.opt.mouse = "nv"
vim.opt.splitbelow = true
vim.opt.syntax = "on"
vim.opt.termguicolors = true
vim.cmd[[colorscheme moonfly]]
vim.opt.statusline = "%{%&modified?'%#TabLineSel#':''%}█▙ %t ▟█"
	.."%#TermCursor# %w%Y %0* %<<%f>%= ↕ %#TermCursor# ❨%c❩ %l/%L "
require('gitsigns').setup({signcolumn=false, numhl=true})


-- nnn - setup "Nav" vim-function for launching window to browse files and dirs
vim.cmd[[exe "func Nav(...)\n lua nnnLaunch()\n endf"]]
nnnBuf, nnnWin = nil
nnnOpen = vim.fn.tempname()
function nnnLaunch()
	local winStyle = {style = "minimal", relative = "editor", row = 1, col = 0,
		width = 64, height = vim.api.nvim_get_option("lines")-2}
	if not nnnBuf then
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
		vim.cmd[[startinsert | au BufEnter,CursorMoved <buffer> startinsert]]
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
	if not tBuf then
		vim.cmd[[split | exe "terminal" | set nobuflisted]]
		tBuf = vim.api.nvim_get_current_buf()
		tWin = vim.api.nvim_get_current_win()
		vim.cmd[[startinsert | au BufEnter,CursorMoved <buffer> startinsert]]
		vim.cmd[[au BufLeave,WinClosed <buffer> stopinsert]]
		vim.cmd[[au BufUnload <buffer> lua tBuf = nil]]
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


-- two-pane - helper to manage split windows
tpWin, tpWidth = nil
vim.cmd[[exe "func TwoPane(...)\n lua twoPane()\n endf"]]
function twoPane()
	if not tpWin or vim.fn.win_id2win(tpWin)==0 then
		vim.cmd[[bo vsplit]]
		tpWin = vim.api.nvim_get_current_win()
		pcall(vim.api.nvim_win_set_width, tpWin, tpWidth)
	else
		tpWidth = vim.api.nvim_win_get_width(tpWin)
		tpWin = vim.api.nvim_win_close(tpWin, false)
	end
end


-- TODO XXX 
-- telescope and menu/launcher/shortcuts/help
-- treesitter and languages


-- menu - setup "Menu" vim-function for launching a helper menu
vim.cmd([[source $VIMRUNTIME/menu.vim
cnoremap <expr> <up> wildmenumode()? "\<left>":"\<up>"
cnoremap <expr> <down> wildmenumode()? "\<right>":"\<down>"
cnoremap <expr> <left> wildmenumode()? "\<up>":"\<left>"
cnoremap <expr> <right> wildmenumode()? "\<down>":"\<right>"
exe "func Menu(...)\n stopinsert | call feedkeys(':emenu \t', 't')\n endf"
menu .5 File.Project :call feedkeys(":cd \t", "t")<CR>
menu File.Save\ As :call feedkeys(":w \t", "t")<CR>
menu File.Open :call Nav()<CR>
menu Edit.Insert\ Mode :startinsert<CR>
menu File.Save\ As :call feedkeys(":w \t", "t")<CR>
menu Window.Toggle\ Other\ Pane :call TwoPane()<CR>
menu Window.Toggle\ Terminal :call Term()<CR>
menu Help.NNN\ File\ Browser :call Nav()<CR>?
menu Help.GitSigns :e ]]
	..[[~/.local/share/nvim/site/pack/*/start/gitsigns.nvim/doc/gitsigns.txt<CR>
]])


-- tabline - launcher shortcuts and buffer tabs.
function tabline()
	-- line length and truncation logic
	local c = vim.o.columns-1
	function S(s)
		c = math.max(c - vim.api.nvim_strwidth(s), 0)
		return s
	end
	-- launcher shortcuts
	local r = '%#TabLineSel#%0@Menu@'..S('≣')..'%0@DoInsert@'..S('I')
		..'%0@MGit@'..S('G')..'%0@Term@'..S('❱')..'%0@TwoPane@'..S('/')
		..'%0@Nav@%#TabLine#%=%#TabLineSel#'..S('+') ..'%<'
	-- buffer tabs
	local wid = 50
	for buf = 1, vim.fn.bufnr('$') do
		if vim.fn.buflisted(buf) == 1 and wid>0 then
			local bufn = vim.fn.bufnr()
			local mod = vim.api.nvim_buf_get_option(buf, 'modified')
			local name = vim.fn.fnamemodify(vim.api.nvim_buf_get_name(buf), ':t')
			r=r..'%'..buf..'@BufSel@'..(buf==bufn and'%#TabLineSel#'or'%#TabLine#')
				..S(mod and'┣'or'┃')..'%0.'..wid..'('..S(name)..'%)'..(buf==bufn and
				(mod and'%0@BufSave@'..S('✎')or'%0@BufDel@'..S('✖'))or'')
				wid = buf<=bufn and 50 or c-1
		end
	end
	-- end the tabline
	return r
end
vim.cmd[[exe "func DoInsert(...)\n startinsert\n endf"
exe "func MGit(...)\n stopinsert | call feedkeys(':Gitsigns \t', 't')\n endf"
exe "func BufSel(id,c,b,m)\n exe 'b'.a:id\n endf"
exe "func BufSave(...)\n w\n endf"
func BufDel(...)
	if len(getbufinfo({'buflisted':1}))==1 | qa | else | bn|bd# | endif
endf]]
vim.opt.tabline = [[%!v:lua.tabline()]]
vim.opt.showtabline = 2

