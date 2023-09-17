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
#include <share.h>
#include <string.h>

static FILE* g_printOut = NULL;
static void g_printOutInit()
{
  if (!g_printOut) {
    g_printOut = _fsopen("bcdump.txt", "w", _SH_DENYWR);
    setvbuf(g_printOut, NULL, _IOFBF, 64 * 1024 + 8);
  }
}

static void print_operand(GCproto* pt, BCMode mode, BCReg operand)
{
  switch (mode) {
  case BCMuv: fprintf(g_printOut, "uv:%d", operand); break;
  case BCMdst: fprintf(g_printOut, "dst:%d", operand); break;
  case BCMvar: fprintf(g_printOut, "var:%d", operand); break;
  case BCMlit: fprintf(g_printOut, "lit:%d", operand); break;
  case BCMpri: fprintf(g_printOut, "pri:%d", operand); break;
  case BCMnum: fprintf(g_printOut, "num:%d", operand); break;
  case BCMstr: fprintf(g_printOut, "str:%d", operand); break;
  case BCMtab: fprintf(g_printOut, "tab:%d", operand); break;
  case BCMbase: fprintf(g_printOut, "base:%d", operand); break;
  case BCMfunc: fprintf(g_printOut, "func:%d", operand); break;
  case BCMjump: fprintf(g_printOut, "jump:%d", operand); break;
  case BCMlits: fprintf(g_printOut, "lits:%d", operand); break;
  case BCMcdata: fprintf(g_printOut, "cdata:%d", operand); break;
  case BCMrbase: fprintf(g_printOut, "rbase:%d", operand); break;
  }
}

static void print_table(GCtab* t);

static void print_value(cTValue* v) {
  switch (itype(v)) {
  case LJ_TNIL: fputs("nil", g_printOut); break;
  case LJ_TFALSE: fputs("false", g_printOut); break;
  case LJ_TTRUE: fputs("true", g_printOut); break;
  case LJ_TLIGHTUD: fputs("[lightuserdata]", g_printOut); break;
  case LJ_TSTR: fprintf(g_printOut, "\"%s\"", strVdata(v)); break;
  case LJ_TUPVAL: fputs("[upval]", g_printOut); break;
  case LJ_TTHREAD: fputs("[thread]", g_printOut); break;
  case LJ_TPROTO: fputs("[proto]", g_printOut); break;
  case LJ_TFUNC: fputs("[func]", g_printOut); break;
  case LJ_TTRACE: fputs("[trace]", g_printOut); break;
  case LJ_TCDATA: fputs("[cdata]", g_printOut); break;
  case LJ_TTAB: print_table(tabV(v)); break;
  case LJ_TUDATA: fputs("[userdata]", g_printOut); break;
  case LJ_TISNUM:
  default: // assume double.
    if (v->n == (int)v->n)
      fprintf(g_printOut, "%d", (int)v->n);
    else
      fprintf(g_printOut, "%f", v->n);
    break;
  }
}

static void print_table(GCtab* t)
{
  fputs("{", g_printOut);
  TValue k[2];
  setnilV(k);
  int first = 1;
  while (lj_tab_next(t, k, k) > 0) {
    if (first) {
      first = 0;
    } else {
      fputs(", ", g_printOut);
    }
    fputs("[", g_printOut);
    print_value(&k[0]);
    fputs("]=", g_printOut);
    print_value(&k[1]);
  }
  fputs("}", g_printOut);
}

static void lua_print_proto_bc(GCproto* pt)
{
  printf("FUNCTION DUMP %s:%d\n", proto_chunknamestr(pt), lj_debug_line(pt, 0));
  fprintf(g_printOut, "FUNCTION DUMP %s:%d\n", proto_chunknamestr(pt), lj_debug_line(pt, 0));

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
    BCIns* bc = proto_bc(pt);
    BCIns ins = bc[ pc ];
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
    fprintf(g_printOut, "L%04d\t", lj_debug_line(pt, pc));

    // Print memory address.
    fprintf(g_printOut, "0x%p\t", &bc[ pc ]);

    // Print instruction bytes.
    fprintf(g_printOut, "%02X ", op);
    fprintf(g_printOut, "%02X ", a);
    fprintf(g_printOut, "%02X ", b);
    fprintf(g_printOut, "%02X ", c);
    fputs("\t", g_printOut);

    // Print opcode name.
    switch (op) {
#define BCENUM(name, ma, mb, mc, mt) case BC_##name: fputs(#name, g_printOut); break;
      BCDEF(BCENUM)
#undef BCENUM
    default: fputs("UNKNOWN OPCODE", g_printOut); break;
    }

    // Print operands.
    if (ma != BCMnone) {
      fputs("\t", g_printOut);
      print_operand(pt, ma, a);
    }
    if (bcmode_hasd(op)) {
      if (md != BCMnone) {
        fputs("\t", g_printOut);
        print_operand(pt, md, d);
      }
    }
    else {
      if (mb != BCMnone) {
        fputs("\t", g_printOut);
        print_operand(pt, mb, b);
      }
      if (mc != BCMnone) {
        fputs("\t", g_printOut);
        print_operand(pt, mc, c);
      }
    }

    fputs("\n", g_printOut);
  }

  // Print number constants.
  if (pt->sizekn) {
    fprintf(g_printOut, "NUMBER CONSTANTS (%d)\n", pt->sizekn);
    for (unsigned int i = 0; i < pt->sizekn; ++i) {
      cTValue* tv = proto_knumtv(pt, i);
      fprintf(g_printOut, "%8d\t%f\n", i, tv->n);
    }
  }

  // Print GC constants.
  if (pt->sizekgc) {
    fprintf(g_printOut, "GC CONSTANTS (%d)\n", pt->sizekgc);
    GCRef* kr = mref(pt->k, GCRef) - ( ptrdiff_t ) pt->sizekgc;
    for (unsigned int i = 0; i < pt->sizekgc; ++i, ++kr) {
      GCobj* o = gcref(*kr);
      switch (o->gch.gct) {
      case ~LJ_TSTR:
        fprintf(g_printOut, "%8d\t\"%s\"\n", pt->sizekgc - 1 - i, strdata(gco2str(o)));
        break;
      case ~LJ_TTAB:
        fprintf(g_printOut, "%8d\ttable ", pt->sizekgc - 1 - i);
        print_table(gco2tab(o));
        fputs("\n", g_printOut);
        break;
      }
    }
  }

  fputs("DUMP END\n", g_printOut);
}

LUA_API void lua_print_func_bc(lua_State* L)
{
  cTValue* o = L->top - 1;
  lj_checkapi(L->top > L->base, "top slot empty");
  if (tvisfunc(o) && isluafunc(funcV(o))) {
    g_printOutInit();
    lua_print_proto_bc(funcproto(funcV(o)));
    fflush(g_printOut);
  }
}
