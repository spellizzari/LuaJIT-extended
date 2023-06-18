/*
** Bytecode instruction modes.
** Copyright (C) 2005-2022 Mike Pall. See Copyright Notice in luajit.h
*/

#define lj_bc_c
#define LUA_CORE

#include "lj_obj.h"
#include "lj_bc.h"

/* Bytecode offsets and bytecode instruction modes. */
#include "lj_bcdef.h"

#include "lj_gc.h"
#include "lj_tab.h"
#include "lj_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_operand(GCproto* pt, BCMode mode, BCReg operand)
{
  switch (mode) {
  case BCMuv: fprintf(stdout, "uv:%d", operand); break;
  case BCMdst: fprintf(stdout, "dst:%d", operand); break;
  case BCMvar: fprintf(stdout, "var:%d", operand); break;
  case BCMlit: fprintf(stdout, "lit:%d", operand); break;
  case BCMpri: fprintf(stdout, "pri:%d", operand); break;
  case BCMnum: fprintf(stdout, "num:%d", operand); break;
  case BCMstr: fprintf(stdout, "str:%d", operand); break;
  case BCMtab: fprintf(stdout, "tab:%d", operand); break;
  case BCMbase: fprintf(stdout, "base:%d", operand); break;
  case BCMfunc: fprintf(stdout, "func:%d", operand); break;
  case BCMjump: fprintf(stdout, "jump:%d", operand); break;
  case BCMlits: fprintf(stdout, "lits:%d", operand); break;
  case BCMcdata: fprintf(stdout, "cdata:%d", operand); break;
  case BCMrbase: fprintf(stdout, "rbase:%d", operand); break;
  }
}

static void print_table(GCtab* t);

static void print_value(cTValue* v) {
  switch (itype(v)) {
  case LJ_TNIL: fputs("nil", stdout); break;
  case LJ_TFALSE: fputs("false", stdout); break;
  case LJ_TTRUE: fputs("true", stdout); break;
  case LJ_TLIGHTUD: fputs("[lightuserdata]", stdout); break;
  case LJ_TSTR: fprintf(stdout, "\"%s\"", strVdata(v)); break;
  case LJ_TUPVAL: fputs("[upval]", stdout); break;
  case LJ_TTHREAD: fputs("[thread]", stdout); break;
  case LJ_TPROTO: fputs("[proto]", stdout); break;
  case LJ_TFUNC: fputs("[func]", stdout); break;
  case LJ_TTRACE: fputs("[trace]", stdout); break;
  case LJ_TCDATA: fputs("[cdata]", stdout); break;
  case LJ_TTAB: print_table(tabV(v)); break;
  case LJ_TUDATA: fputs("[userdata]", stdout); break;
  case LJ_TISNUM:
  default: // assume double.
    if (v->n == (int)v->n)
      fprintf(stdout, "%d", (int)v->n);
    else
      fprintf(stdout, "%f", v->n);
    break;
  }
}

static void print_table(GCtab* t)
{
  fputs("{", stdout);
  TValue k[2];
  setnilV(k);
  int first = 1;
  while (lj_tab_next(t, k, k) > 0) {
    if (first) {
      first = 0;
    } else {
      fputs(", ", stdout);
    }
    fputs("[", stdout);
    print_value(&k[0]);
    fputs("]=", stdout);
    print_value(&k[1]);
  }
  fputs("}", stdout);
}

static void lua_print_proto_bc(GCproto* pt)
{
  fprintf(stdout, "FUNCTION DUMP %s:%d\n", proto_chunknamestr(pt), lj_debug_line(pt, 0));

  // Recursively print children of prototype.
  if ((pt->flags & PROTO_CHILD)) {
    ptrdiff_t i, n = pt->sizekgc;
    GCRef* kr = mref(pt->k, GCRef) - 1;
    for (i = 0; i < n; i++, kr--) {
      GCobj* o = gcref(*kr);
      if (o->gch.gct == ~LJ_TPROTO)
        lua_print_proto_bc(gco2pt(o));
    }
  }

  // Print bytecode.
  for (unsigned int pc = 0; pc < pt->sizebc; ++pc) {
    BCIns ins = proto_bc(pt)[ pc ];
    BCOp op = bc_op(ins);
    BCReg a = bc_a(ins);
    BCReg b = bc_b(ins);
    BCReg c = bc_c(ins);
    BCReg d = bc_d(ins);

    BCMode ma = bcmode_a(op);
    BCMode mb = bcmode_b(op);
    BCMode mc = bcmode_c(op);
    BCMode md = bcmode_d(op);

    // Print line number.
    fprintf(stdout, "L%04d\t", lj_debug_line(pt, pc));

    // Print instruction bytes.
    fprintf(stdout, "%02X ", op);
    fprintf(stdout, "%02X ", a);
    fprintf(stdout, "%02X ", b);
    fprintf(stdout, "%02X ", c);
    fputs("\t", stdout);

    // Print opcode name.
    switch (op) {
#define BCENUM(name, ma, mb, mc, mt) case BC_##name: fputs(#name, stdout); break;
      BCDEF(BCENUM)
#undef BCENUM
    default: fputs("UNKNOWN OPCODE", stdout); break;
    }

    // Print operands.
    if (ma != BCMnone) {
      fputs("\t", stdout);
      print_operand(pt, ma, a);
    }
    if (bcmode_hasd(op)) {
      if (md != BCMnone) {
        fputs("\t", stdout);
        print_operand(pt, md, d);
      }
    }
    else {
      if (mb != BCMnone) {
        fputs("\t", stdout);
        print_operand(pt, mb, b);
      }
      if (mc != BCMnone) {
        fputs("\t", stdout);
        print_operand(pt, mc, c);
      }
    }

    fputs("\n", stdout);
  }

  // Print number constants.
  if (pt->sizekn) {
    fprintf(stdout, "NUMBER CONSTANTS (%d)\n", pt->sizekn);
    for (unsigned int i = 0; i < pt->sizekn; ++i) {
      cTValue* tv = proto_knumtv(pt, i);
      fprintf(stdout, "%8d\t%f\n", i, tv->n);
    }
  }

  // Print GC constants.
  if (pt->sizekgc) {
    fprintf(stdout, "GC CONSTANTS (%d)\n", pt->sizekgc);
    GCRef* kr = mref(pt->k, GCRef) - ( ptrdiff_t ) pt->sizekgc;
    for (unsigned int i = 0; i < pt->sizekgc; ++i, ++kr) {
      GCobj* o = gcref(*kr);
      switch (o->gch.gct) {
      case ~LJ_TSTR:
        fprintf(stdout, "%8d\t\"%s\"\n", pt->sizekgc - 1 - i, strdata(gco2str(o)));
        break;
      case ~LJ_TTAB:
        fprintf(stdout, "%8d\ttable ", pt->sizekgc - 1 - i);
        print_table(gco2tab(o));
        fputs("\n", stdout);
        break;
      }
    }
  }

  fputs("DUMP END\n", stdout);
}

LUA_API void lua_print_func_bc(lua_State* L)
{
  cTValue* o = L->top - 1;
  lj_checkapi(L->top > L->base, "top slot empty");
  if (tvisfunc(o) && isluafunc(funcV(o))) {
    lua_print_proto_bc(funcproto(funcV(o)));
  }
}
