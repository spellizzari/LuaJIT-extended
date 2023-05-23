local assert, io_open, io_lines, io_write, load, type, xpcall =
      assert, io.open, io.lines, io.write, load, type, xpcall
local debug_traceback, math_random, tonumber, loadstring =
      debug.traceback, math.random, tonumber, loadstring or load

local dirsep = package.config:match"^(.-)\n"
local own_file = debug.getinfo(1, "S").source:match"^@(.*)" or arg[0]
local own_dir = own_file:match("^.*[/".. dirsep .."]")

local function default_tags()
  local tags = {}
  
  -- Lua version and features
  tags.lua = tonumber(_VERSION:match"%d+%.%d+")
  if table.pack then
    tags["compat5.2"] = true
  end
  if loadstring"return 0xep+9" then
    tags.hexfloat = true
  end
  if loadstring"goto x ::x::" then
    tags["goto"] = true
  end

  -- Libraries
  for _, lib in ipairs{"bit", "ffi", "jit.profile", "table.new"} do
    if pcall(require, lib) then
      tags[lib] = true
    end
  end

  -- LuaJIT-specific
  if jit then
    tags.luajit = tonumber(jit.version:match"%d+%.%d+")
    tags[jit.arch:lower()] = true
    if jit.os ~= "Other" then
      tags[jit.os:lower()] = true
    end
    if jit.status() then
      tags.jit = true
    end
    for _, flag in ipairs{select(2, jit.status())} do
      tags[flag:lower()] = true
    end
  end
  
  -- Environment
  if dirsep == "\\" then
    tags.windows = true
  end
  if tags.ffi then
    local abi = require"ffi".abi
    for _, param in ipairs{"le", "be", "fpu", "softfp", "hardfp", "eabi"} do
      if abi(param) then
        tags[param] = true
      end
    end
    if abi"win" then tags.winabi = true end
    if abi"32bit" then tags.abi32 = true end
    if abi"64bit" then tags.abi64 = true end
  else
    local bytecode = string.dump(function()end)
    if bytecode:find"^\27Lua[\80-\89]" then
      tags[bytecode:byte(7, 7) == 0 and "be" or "le"] = true
      tags["abi".. (bytecode:byte(9, 9) * 8)] = true
    end
  end
  
  return tags
end

do
    local tags = default_tags()
    for k,v in pairs(tags) do
        print(k .. "=" .. tostring(v))
    end
end