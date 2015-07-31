/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "pa.h"
#include "repbasic.h"
#include "repchan.h"
#include "repcxt.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "repsubgl.h"
#include "reptime.h"
#include "semanaph.h"
#include "semdisc.h"
#include "synparse.h"
#include "synpnode.h"
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

void UA_Occupation1(Actor *ac, Ts *ts, Obj *a, Obj *occupation)
{
  Subgoal	*sg;
  if (ISA(N("grocer"), occupation)) {
    if ((sg = SubgoalFind(ac, N("grocer")))) {
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_NONE);
    } else {
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
      sg = TG(ac->cx, &ac->cx->story_time.stopts, a, L(N("grocer"), a, E));
      PA_SpinTo(ac->cx, sg, 100);
    }
  }
}

Bool UA_OccupationDoesHandle(Obj *rel)
{
  return(ISA(N("occupation-of"), rel));
}

void UA_Occupation(Actor *ac, Ts *ts, Obj *a, Obj *in)
{
  ObjList	*occupations;
  Intension	*itn;
  if (ISA(N("occupation-of"), I(in, 0)) &&
      (a == I(in, 1))) {
    UA_Relation1(ac->cx, ts, in);
    if (ac->cx->rsn.relevance > RELEVANCE_NONE &&
        ac->cx->rsn.novelty == NOVELTY_NONE) {
    /* We already knew this. */
      return;
    }
    if (!(ac->cx->dc->mode & DC_MODE_CONV)) {
      UA_Occupation1(ac, ts, a, I(in, 2));
    } else if ((occupations = UA_AskerSelect(ac->cx, I(in, 2), &itn))) {
      if (ObjListIsLengthOne(occupations)) {
        UA_Occupation1(ac, ts, a, occupations->obj);
      } else {
        ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
        UA_AskerNarrowDown(ac->cx, 1.0, N("UA_Occupation"), itn, occupations, 
                           L(N("occupation-of"), a, N("?answer"), E), 0);
      }
    }
  }
}

/* End of file. */
