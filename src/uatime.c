/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19951204: rebegun
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "repbasic.h"
#include "repchan.h"
#include "repcxt.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptime.h"
#include "semanaph.h"
#include "semdisc.h"
#include "semtense.h"
#include "ua.h"
#include "uaanal.h"
#include "uaasker.h"
#include "uadict.h"
#include "uaemot.h"
#include "uafriend.h"
#include "uagoal.h"
#include "uaoccup.h"
#include "uapaappt.h"
#include "uapagroc.h"
#include "uapashwr.h"
#include "uapaslp.h"
#include "uaquest.h"
#include "uarel.h"
#include "uaspace.h"
#include "uatime.h"
#include "uatrade.h"
#include "uaweath.h"
#include "utilbb.h"
#include "utildbg.h"
#include "utillrn.h"

void UA_Time_TsRange(Context *cx, TsRange *tsr)
{
  if (DbgOn(DBGGEN, DBGDETAIL)) {
    fprintf(Log, "UA_Time: TsRange input %d ", cx->story_tensestep);
    TsRangePrint(Log, &cx->story_time);
  }
  if (!TsIsNa(&tsr->startts)) {
  /* todo: Only if tsr->startts < cx->story_time.startts? */
    cx->story_time.startts = tsr->startts;
  }
  if (!TsIsNa(&tsr->stopts)) {
    cx->story_time.stopts = tsr->stopts;
  }
  if (cx->dc->tense) {
    cx->story_tensestep = TenseToTenseStep(cx->dc->tense);
  }
  if (DbgOn(DBGGEN, DBGDETAIL)) {
    fprintf(Log, " ------> %d ", cx->story_tensestep);
    TsRangePrint(Log, &cx->story_time);
    fputc(NEWLINE, Log);
  }
}

void UA_Time_Tense(Context *cx, Obj *tense)
{
  Dur		dur;
  TenseStep     tensestep;

  tensestep = TenseToTenseStep(tense);
  if (tensestep != cx->story_tensestep) {
    dur = TenseStepDefaultDistance(cx->story_tensestep, tensestep);
    if (DbgOn(DBGGEN, DBGDETAIL)) {
      fprintf(Log, "UA_Time: Tense input; change %d ", cx->story_tensestep);
      TsRangePrint(Log, &cx->story_time);
    }
    TsIncrement(&cx->story_time.startts, dur);

    /* This seems to be necessary so that "At 7 am" can be handled
     * retrospectively (inper.txt). Otherwise, we would need to backward
     * adjust all existing assertions.
     * But it causes problems in the tutoyer example (intut.txt).
     */
#ifndef MICRO
    TsMidnight(&cx->story_time.startts, &cx->story_time.startts);
    TsIncrement(&cx->story_time.startts, 1L);	/* cf generate time of day */
#endif

    cx->story_time.stopts = cx->story_time.startts;

    cx->story_tensestep = tensestep;
    if (DbgOn(DBGGEN, DBGDETAIL)) {
      fprintf(Log, " --%ld--> <%s> %d ", dur, M(tense),
              cx->story_tensestep);
      TsRangePrint(Log, &cx->story_time);
      fputc(NEWLINE, Log);
    }
  }
}

void UA_Time(Context *cx, Ts *ts, Obj *in)
{
  if (TsRangeIsNaIgnoreCx(&cx->story_time)) {
  /* Should already have been defaulted to NOW. */
    Dbg(DBGGEN, DBGBAD, "UA_Time 1");
    return;
  }

  if (!TsRangeIsNaIgnoreCx(ObjToTsRange(in))) {
    UA_Time_TsRange(cx, ObjToTsRange(in));
  } else if (cx->dc->tense) {
    UA_Time_Tense(cx, cx->dc->tense);
  }
}

/* End of file. */
