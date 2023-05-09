/*
** Bytecode instruction format.
** Copyright (C) 2005-2022 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef _LJ_BC_H
#define _LJ_BC_H

#include "lj_def.h"
#include "lj_arch.h"

/* Bytecode instruction format, 32 bit wide, fields of 8 or 16 bit:
**
** +----+----+----+----+
** | B  | C  | A  | OP | Format ABC
** +----+----+----+----+
** |    D    | A  | OP | Format AD
** +--------------------
** MSB               LSB
**
** In-memory instructions are always stored in host byte order.
*/

/* Operand ranges and related constants. */
#define BCMAX_A		0xff
#define BCMAX_B		0xff
#define BCMAX_C		0xff
#define BCMAX_D		0xffff
#define BCBIAS_J	0x8000
#define NO_REG		BCMAX_A
#define NO_JMP		(~(BCPos)0)

/* Macros to get instruction fields. */
#define bc_op(i)	((BCOp)((i)&0xff))
#define bc_a(i)		((BCReg)(((i)>>8)&0xff))
#define bc_b(i)		((BCReg)((i)>>24))
#define bc_c(i)		((BCReg)(((i)>>16)&0xff))
#define bc_d(i)		((BCReg)((i)>>16))
#define bc_j(i)		((ptrdiff_t)bc_d(i)-BCBIAS_J)

/* Macros to set instruction fields. */
#define setbc_byte(p, x, ofs) \
  ((uint8_t *)(p))[LJ_ENDIAN_SELECT(ofs, 3-ofs)] = (uint8_t)(x)
#define setbc_op(p, x)	setbc_byte(p, (x), 0)
#define setbc_a(p, x)	setbc_byte(p, (x), 1)
#define setbc_b(p, x)	setbc_byte(p, (x), 3)
#define setbc_c(p, x)	setbc_byte(p, (x), 2)
#define setbc_d(p, x) \
  ((uint16_t *)(p))[LJ_ENDIAN_SELECT(1, 0)] = (uint16_t)(x)
#define setbc_j(p, x)	setbc_d(p, (BCPos)((int32_t)(x)+BCBIAS_J))

/* Macros to compose instructions. */
#define BCINS_ABC(o, a, b, c) \
  (((BCIns)(o))|((BCIns)(a)<<8)|((BCIns)(b)<<24)|((BCIns)(c)<<16))
#define BCINS_AD(o, a, d) \
  (((BCIns)(o))|((BCIns)(a)<<8)|((BCIns)(d)<<16))
#define BCINS_AJ(o, a, j)	BCINS_AD(o, a, (BCPos)((int32_t)(j)+BCBIAS_J))

/* Bytecode instruction definition. Order matters, see below.
**
** (name, filler, Amode, Bmode, Cmode or Dmode, metamethod)
**
** The opcode name suffixes specify the type for RB/RC or RD:
** V = variable slot
** S = string const
** N = number const
** P = primitive type (~itype)
** B = unsigned byte literal
** M = multiple args/results
*/
#define BCDEF(_) \
  /* Comparison ops. ORDER OPR. */ \
  /* 00 */ _(ISLT,	var,	___,	var,	lt) \
  /* 01 */ _(ISGE,	var,	___,	var,	lt) \
  /* 02 */ _(ISLE,	var,	___,	var,	le) \
  /* 03 */ _(ISGT,	var,	___,	var,	le) \
  \
  /* 04 */ _(ISEQV,	var,	___,	var,	eq) \
  /* 05 */ _(ISNEV,	var,	___,	var,	eq) \
  /* 06 */ _(ISEQS,	var,	___,	str,	eq) \
  /* 07 */ _(ISNES,	var,	___,	str,	eq) \
  /* 08 */ _(ISEQN,	var,	___,	num,	eq) \
  /* 09 */ _(ISNEN,	var,	___,	num,	eq) \
  /* 0A */ _(ISEQP,	var,	___,	pri,	eq) \
  /* 0B */ _(ISNEP,	var,	___,	pri,	eq) \
  \
  /* Unary test and copy ops. */ \
  /* 0C */ _(ISTC,	dst,	___,	var,	___) \
  /* 0D */ _(ISFC,	dst,	___,	var,	___) \
  /* 0E */ _(IST,	___,	___,	var,	___) \
  /* 0F */ _(ISF,	___,	___,	var,	___) \
  /* 10 */ _(ISTYPE,var,	___,	lit,	___) \
  /* 11 */ _(ISNUM,	var,	___,	lit,	___) \
  \
  /* Unary ops. */ \
  /* 12 */ _(MOV,	dst,	___,	var,	___) \
  /* 13 */ _(NOT,	dst,	___,	var,	___) \
  /* 14 */ _(UNM,	dst,	___,	var,	unm) \
  /* 15 */ _(LEN,	dst,	___,	var,	len) \
  \
  /* Binary ops. ORDER OPR. VV last, POW must be next. */ \
  /* 16 */ _(ADDVN,	dst,	var,	num,	add) \
  /* 17 */ _(SUBVN,	dst,	var,	num,	sub) \
  /* 18 */ _(MULVN,	dst,	var,	num,	mul) \
  /* 19 */ _(DIVVN,	dst,	var,	num,	div) \
  /* 1A */ _(MODVN,	dst,	var,	num,	mod) \
  \
  /* 1B */ _(ADDNV,	dst,	var,	num,	add) \
  /* 1C */ _(SUBNV,	dst,	var,	num,	sub) \
  /* 1D */ _(MULNV,	dst,	var,	num,	mul) \
  /* 1E */ _(DIVNV,	dst,	var,	num,	div) \
  /* 1F */ _(MODNV,	dst,	var,	num,	mod) \
  \
  /* 20 */ _(ADDVV,	dst,	var,	var,	add) \
  /* 21 */ _(SUBVV,	dst,	var,	var,	sub) \
  /* 22 */ _(MULVV,	dst,	var,	var,	mul) \
  /* 23 */ _(DIVVV,	dst,	var,	var,	div) \
  /* 24 */ _(MODVV,	dst,	var,	var,	mod) \
  \
  /* 25 */ _(POW,	dst,	var,	var,	pow) \
  /* 26 */ _(CAT,	dst,	rbase,	rbase,	concat) \
  \
  /* Constant ops. */ \
  /* 27 */ _(KSTR,	dst,	___,	str,	___) \
  /* 28 */ _(KCDATA,dst,	___,	cdata,	___) \
  /* 29 */ _(KSHORT,dst,	___,	lits,	___) \
  /* 2A */ _(KNUM,	dst,	___,	num,	___) \
  /* 2B */ _(KPRI,	dst,	___,	pri,	___) \
  /* 2C */ _(KNIL,	base,	___,	base,	___) \
  \
  /* Upvalue and function ops. */ \
  /* 2D */ _(UGET,	dst,	___,	uv,	___) \
  /* 2E */ _(USETV,	uv,	___,	var,	___) \
  /* 2F */ _(USETS,	uv,	___,	str,	___) \
  /* 30 */ _(USETN,	uv,	___,	num,	___) \
  /* 31 */ _(USETP,	uv,	___,	pri,	___) \
  /* 32 */ _(UCLO,	rbase,	___,	jump,	___) \
  /* 33 */ _(FNEW,	dst,	___,	func,	gc) \
  \
  /* Table ops. */ \
  /* 34 */ _(TNEW,	dst,	___,	lit,	gc) \
  /* 35 */ _(TDUP,	dst,	___,	tab,	gc) \
  /* 36 */ _(GGET,	dst,	___,	str,	index) \
  /* 37 */ _(GSET,	var,	___,	str,	newindex) \
  /* 38 */ _(TGETV,	dst,	var,	var,	index) \
  /* 39 */ _(TGETS,	dst,	var,	str,	index) \
  /* 3A */ _(TGETB,	dst,	var,	lit,	index) \
  /* 3B */ _(TGETR,	dst,	var,	var,	index) \
  /* 3C */ _(TSETV,	var,	var,	var,	newindex) \
  /* 3D */ _(TSETS,	var,	var,	str,	newindex) \
  /* 3E */ _(TSETB,	var,	var,	lit,	newindex) \
  /* 3F */ _(TSETM,	base,	___,	num,	newindex) \
  /* 40 */ _(TSETR,	var,	var,	var,	newindex) \
  \
  /* Calls and vararg handling. T = tail call. */ \
  /* 41 */ _(CALLM,	base,	lit,	lit,	call) \
  /* 42 */ _(CALL,	base,	lit,	lit,	call) \
  /* 43 */ _(CALLMT,base,	___,	lit,	call) \
  /* 44 */ _(CALLT,	base,	___,	lit,	call) \
  /* 45 */ _(ITERC,	base,	lit,	lit,	call) \
  /* 46 */ _(ITERN,	base,	lit,	lit,	call) \
  /* 47 */ _(VARG,	base,	lit,	lit,	___) \
  /* 48 */ _(ISNEXT,base,	___,	jump,	___) \
  \
  /* Returns. */ \
  /* 49 */ _(RETM,	base,	___,	lit,	___) \
  /* 4A */ _(RET,	rbase,	___,	lit,	___) \
  /* 4B */ _(RET0,	rbase,	___,	lit,	___) \
  /* 4C */ _(RET1,	rbase,	___,	lit,	___) \
  \
  /* Loops and branches. I/J = interp/JIT, I/C/L = init/call/loop. */ \
  /* 4D */ _(FORI,	base,	___,	jump,	___) \
  /* 4E */ _(JFORI,	base,	___,	jump,	___) \
  \
  /* 4F */ _(FORL,	base,	___,	jump,	___) \
  /* 50 */ _(IFORL,	base,	___,	jump,	___) \
  /* 51 */ _(JFORL,	base,	___,	lit,	___) \
  \
  /* 52 */ _(ITERL,	base,	___,	jump,	___) \
  /* 53 */ _(IITERL,base,	___,	jump,	___) \
  /* 54 */ _(JITERL,base,	___,	lit,	___) \
  \
  /* 55 */ _(LOOP,	rbase,	___,	jump,	___) \
  /* 56 */ _(ILOOP,	rbase,	___,	jump,	___) \
  /* 57 */ _(JLOOP,	rbase,	___,	lit,	___) \
  \
  /* 58 */ _(JMP,	rbase,	___,	jump,	___) \
  \
  /* Function headers. I/J = interp/JIT, F/V/C = fixarg/vararg/C func. */ \
  /* 59 */ _(FUNCF,	rbase,	___,	___,	___) \
  /* 5A */ _(IFUNCF,rbase,	___,	___,	___) \
  /* 5B */ _(JFUNCF,rbase,	___,	lit,	___) \
  /* 5C */ _(FUNCV,	rbase,	___,	___,	___) \
  /* 5D */ _(IFUNCV,rbase,	___,	___,	___) \
  /* 5E */ _(JFUNCV,rbase,	___,	lit,	___) \
  /* 5F */ _(FUNCC,	rbase,	___,	___,	___) \
  /* 60 */ _(FUNCCW,rbase,	___,	___,	___) \
  \
  /* >XPK< Additional opcodes. */ \
  /* 61 */ _(TDUPHI, dst,   ___,    tab,    gc) \
  /* 62 */ _(KINTLO, dst,   ___,    lit,    ___) \
  /* 63 */ _(KINTHI, dst,   ___,    lit,    ___) \
  /* 64 */ _(KSTRHI, dst,   ___,    lit,    ___)

/* Bytecode opcode numbers. */
typedef enum {
#define BCENUM(name, ma, mb, mc, mt)	BC_##name,
BCDEF(BCENUM)
#undef BCENUM
  BC__MAX
} BCOp;

LJ_STATIC_ASSERT((int)BC_ISEQV+1 == (int)BC_ISNEV);
LJ_STATIC_ASSERT(((int)BC_ISEQV^1) == (int)BC_ISNEV);
LJ_STATIC_ASSERT(((int)BC_ISEQS^1) == (int)BC_ISNES);
LJ_STATIC_ASSERT(((int)BC_ISEQN^1) == (int)BC_ISNEN);
LJ_STATIC_ASSERT(((int)BC_ISEQP^1) == (int)BC_ISNEP);
LJ_STATIC_ASSERT(((int)BC_ISLT^1) == (int)BC_ISGE);
LJ_STATIC_ASSERT(((int)BC_ISLE^1) == (int)BC_ISGT);
LJ_STATIC_ASSERT(((int)BC_ISLT^3) == (int)BC_ISGT);
LJ_STATIC_ASSERT((int)BC_IST-(int)BC_ISTC == (int)BC_ISF-(int)BC_ISFC);
LJ_STATIC_ASSERT((int)BC_CALLT-(int)BC_CALL == (int)BC_CALLMT-(int)BC_CALLM);
LJ_STATIC_ASSERT((int)BC_CALLMT + 1 == (int)BC_CALLT);
LJ_STATIC_ASSERT((int)BC_RETM + 1 == (int)BC_RET);
LJ_STATIC_ASSERT((int)BC_FORL + 1 == (int)BC_IFORL);
LJ_STATIC_ASSERT((int)BC_FORL + 2 == (int)BC_JFORL);
LJ_STATIC_ASSERT((int)BC_ITERL + 1 == (int)BC_IITERL);
LJ_STATIC_ASSERT((int)BC_ITERL + 2 == (int)BC_JITERL);
LJ_STATIC_ASSERT((int)BC_LOOP + 1 == (int)BC_ILOOP);
LJ_STATIC_ASSERT((int)BC_LOOP + 2 == (int)BC_JLOOP);
LJ_STATIC_ASSERT((int)BC_FUNCF + 1 == (int)BC_IFUNCF);
LJ_STATIC_ASSERT((int)BC_FUNCF + 2 == (int)BC_JFUNCF);
LJ_STATIC_ASSERT((int)BC_FUNCV + 1 == (int)BC_IFUNCV);
LJ_STATIC_ASSERT((int)BC_FUNCV + 2 == (int)BC_JFUNCV);

/* This solves a circular dependency problem, change as needed. */
#define FF_next_N	4

/* Stack slots used by FORI/FORL, relative to operand A. */
enum {
  FORL_IDX, FORL_STOP, FORL_STEP, FORL_EXT
};

/* Bytecode operand modes. ORDER BCMode */
typedef enum {
  BCMnone, BCMdst, BCMbase, BCMvar, BCMrbase, BCMuv,  /* Mode A must be <= 7 */
  BCMlit, BCMlits, BCMpri, BCMnum, BCMstr, BCMtab, BCMfunc, BCMjump, BCMcdata,
  BCM_max
} BCMode;
#define BCM___		BCMnone

#define bcmode_a(op)	((BCMode)(lj_bc_mode[op] & 7))
#define bcmode_b(op)	((BCMode)((lj_bc_mode[op]>>3) & 15))
#define bcmode_c(op)	((BCMode)((lj_bc_mode[op]>>7) & 15))
#define bcmode_d(op)	bcmode_c(op)
#define bcmode_hasd(op)	((lj_bc_mode[op] & (15<<3)) == (BCMnone<<3))
#define bcmode_mm(op)	((MMS)(lj_bc_mode[op]>>11))

#define BCMODE(name, ma, mb, mc, mm) \
  (BCM##ma|(BCM##mb<<3)|(BCM##mc<<7)|(MM_##mm<<11)),
#define BCMODE_FF	0

static LJ_AINLINE int bc_isret(BCOp op)
{
  return (op == BC_RETM || op == BC_RET || op == BC_RET0 || op == BC_RET1);
}

LJ_DATA const uint16_t lj_bc_mode[];
LJ_DATA const uint16_t lj_bc_ofs[];

#endif
