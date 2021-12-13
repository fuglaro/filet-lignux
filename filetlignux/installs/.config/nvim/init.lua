
vim.opt.tabstop = 2
vim.opt.shiftwidth = 2
vim.opt.colorcolumn = "80"
vim.opt.mouse = "nv"

require('nnn').setup({
	picker = {
		cmd = "nnn -HG",
		style = { width=0.5, height=0.9, xoffset=0, yoffset=1 },
	},
})
