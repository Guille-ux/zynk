vim.api.nvim_create_autocmd({ "BufRead", "BufNewFile" }, {
  pattern = {"*.z", "*.zn", "*.zk", "*.zynk"},
  callback = function()
    vim.bo.filetype = "zynk" -- WTF I JUST REALIZED -- IS A COMMENT IN LUA
  end, 
})
