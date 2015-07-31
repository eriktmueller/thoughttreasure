/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940313: begun
 * 19940419: modified for levels and flags
 * 19981112: fix to DbgLogClear
 */

#include "tt.h"
#include "lexentry.h"
#include "repbasic.h"
#include "repchan.h"
#include "repcxt.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "reptime.h"
#include "semdisc.h"
#include "toolsh.h"
#include "utildbg.h"
#include "utillrn.h"

FILE	*Log, *Out, *Display;
int	DbgFlags, DbgLevel, DbgStdoutLevel, Interrupt;
Bool	Debugging;
Obj	*DbgLastObj;

void DbgInit()
{
  DbgLastObj = NULL;
  if (NULL == (Log = StreamOpen("log", "w+"))) Log = stdout;
  if (NULL == (Display = StreamOpen("display", "w+"))) Display = stdout;
#ifdef MACOS
#else
  if (Log != stdout) setvbuf(Log, NULL, _IONBF, 0L);
#endif
  Out = stdout;
}

void DbgLogClear()
{
  StreamClose(Log);
  Log = stdout; /* REALLY */
  if (NULL == (Log = StreamOpen("log", "w+"))) Log = stdout;
#ifdef MACOS
#else
  if (Log != stdout) setvbuf(Log, NULL, _IONBF, 0L);
#endif
}

int DbgStringToLevel(char *s)
{
  if (streq(s, "off")) return(DBGOFF);
  else if (streq(s, "bad")) return(DBGBAD);
  else if (streq(s, "ok")) return(DBGOK);
  else if (streq(s, "detail")) return(DBGDETAIL);
  else if (streq(s, "hyper")) return(DBGHYPER);
  else return(-1);
}

int DbgStringToFlags(char *s)
{
  if (streq(s, "all")) return(DBGALL);
  else if (streq(s, "syn")) return(DBGALLSYN);
  else if (streq(s, "sem")) return(DBGALLSEM);
  else if (streq(s, "synsem")) return(DBGALLSYNSEM);
  else return(-1);
}

void DbgSet(int flags, int level)
{
  if (flags != -1) DbgFlags = flags;
  if (level != -1) DbgLevel = level;
}

void DbgSetStdoutLevel(int level)
{
  DbgStdoutLevel = level;
}

Bool DbgOn(int flag, int level)
{
  return((level <= DBGBAD) || ((level <= DbgLevel) && (DbgFlags & flag)));
}

void DbgOPP(int flag, int level, Obj *obj)
{
  if (DbgOn(flag, level)) {
    ObjPrettyPrint(Log, obj);
  }
}

void DbgOP(int flag, int level, Obj *obj)
{
  if (DbgOn(flag, level)) {
    ObjPrint(Log, obj);
    fputc(NEWLINE, Log);
  }
}

void Dbg(int flag, int level, char *fmt, ...)
{
  char          buf[SENTLEN];
  Ts		ts;
  va_list	args;
#ifdef SUNOS
  va_start(args);
#else
  va_start(args, fmt);
#endif
  vsprintf(buf, fmt, args);
  va_end(args);

  TsSetNow(&ts);
  if (level <= DbgStdoutLevel && Log) {
    TsPrint(stdout, &ts);
    if (level <= DBGBAD && DbgLastObj) {
      fprintf(stdout, " <%s>: ", M(DbgLastObj));
    } else {
      fputs(": ", stdout);
    }
    fputs(buf, stdout);
    fputc(NEWLINE, stdout);
  }
  if (Log && DbgOn(flag, level)) {
    TsPrint(Log, &ts);
    fputs(": ", Log);
    if (level <= DBGBAD) {
      if (DbgLastObj) fprintf(Log, "!!!! <%s>: ", M(DbgLastObj));
      else fputs("!!!! ", Log);
    }
    fputs(buf, Log);
    fputc(NEWLINE, Log);
    fflush(Log);
  }
  if (Interrupt) {
    Interrupt = 0;
    while (1) Tool_Shell_Std();
  }
}

int inter()
{
  Interrupt = 1;
  return(314);
}

void UnwindProtect()
{
  ContextCurrentDc = NULL;
  LexEntryInsideName = 0;
  GenOnAssert = 1;
}

void Panic(char *fmt, ...)
{
  va_list args;
#ifdef SUNOS
  va_start(args);
#else
  va_start(args, fmt);
#endif
  UnwindProtect();
  if (DbgLastObj) fprintf(Log, "Panic <%s>: ", M(DbgLastObj));
  else fprintf(Log, "Panic: ");
  vfprintf(Log, fmt, args);
  fprintf(Log, "\n");
  if (DbgLastObj) fprintf(Out, "Panic <%s>: ", M(DbgLastObj));
  else fprintf(Out, "Panic: ");
  vfprintf(Out, fmt, args);
  fprintf(Out, "\n");
  va_end(args);
  MemCheckPrint();
  /* PANIC */
  while (1) Tool_Shell_Std();
}

void Exit(int code)
{
  if (code == 0) Dbg(DBGGEN, DBGOK, "normal exit");
  else Dbg(DBGGEN, DBGBAD, "exiting with code %d", code);
  MemCheckPrint();
  DiscourseFree(StdDiscourse);
  LearnClose();
  StreamClose(Log);
  StreamClose(Display);
  Log = stdout;
  Dbg(DBGGEN, DBGOK, "exit");
  exit(code);
}

#ifndef MACOS
/* A breakpoint should be set on this function. */
void Debugger()
{
}
#endif

void Stop()
{
  Debugging = 1;
  Debugger();
}

void Nop()
{
}

/* End of file. */
