@@echo off
REM set LUA_PATH=./?.lua;;
REM set LUA_INIT=package.path='?;'..package.path
set PATH=..\bin\x64\Debug\;%PATH%
luajit all.lua
