@@echo off
REM set LUA_PATH=./?.lua;;
REM set LUA_INIT=package.path='?;'..package.path
REM set PATH=..\bin\x64\Debug\;%PATH%
set PATH=..\bin\x64\Orig\;%PATH%
luajit all.lua
