# LuaJIT-extended

This is a personal fork of LuaJIT 2.1.0-beta3 for particular needs. Development on this branch is exclusively done for Windows/x64 and developed in Visual Studio 2022.

The changes done to the original branch are the following:
* Extended the number/GC constant limit per function from 2^16 to 2^32, by introducing new opcodes that can fetch numbers, strings and tables from the constant table with indices greater than 2^16 (an opcode fills the 16 low bits of the index in a register, and then extended versions of existing opcodes use that register and the 16 high bits to form the final index in the constant table). A function only uses those opcodes when there are more than 2^16 constants in it. Current limitations are:
    * JIT is disabled for those functions.
* constant tables used with TDUP opcode will not have entries for non-constant values anymore. This is because for tables initialized with constant string keys but non-constant values, the existing code always made an empty table in the constant table and called TDUP on it, which meant that if a file contained a lot of such tables, its constant table was quickly full of empty tables.

Future changes in the work:
* Add a TINSV opcode that inserts a value from a register in a table at a position given by another register, and increments that register. This will allow us to stop flooding the number constant table with insertion indices when large tables, and use less instructions to initialize tables, at the expense of an additional temporary register.
* Add an option to allow for TSETM at the middle of a table constructor. This should be easy to implement with the previous change, and it will allow us to dynamically concatenate mulret function results in a new table with eg. { fun1(), fun2(), fun3() }. This would be a breaking change for the language but it has its use, and it would still be possible to revert to the old behaviour by doing { (fun1()), (fun2()), fun3() }.

# README for LuaJIT 2.1.0-beta3

LuaJIT is a Just-In-Time (JIT) compiler for the Lua programming language.

Project Homepage: https://luajit.org/

LuaJIT is Copyright (C) 2005-2022 Mike Pall.
LuaJIT is free software, released under the MIT license.
See full Copyright Notice in the COPYRIGHT file or in luajit.h.

Documentation for LuaJIT is available in HTML format.
Please point your favorite browser to:

 doc/luajit.html

