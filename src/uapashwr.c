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

void PAObj_Shower(TsRange *tsrange, Obj *obj)
{
  Obj   *cf, *hf, *ss;
  Obj   *nextstate, *prevstate;
  Ts    *ts;
  ts = &tsrange->startts;
  if (PAObj_IsBroken(ts, obj)) nextstate = N("shower-off");
  else {
    cf = PAObj_GetPartState(ts, obj, N("cold-faucet"), N("knob-position"));
    hf = PAObj_GetPartState(ts, obj, N("hot-faucet"), N("knob-position"));
    ss = PAObj_GetPartState(ts, obj, N("shower-switch"),
                                N("on-or-off-state"));
    if (((cf && cf != N("knob-position-0")) ||
         (hf && hf != N("knob-position-0"))) &&
        ss && ss == N("switch-on")) nextstate = N("shower-on");
    else nextstate = N("shower-off");
  }
  if (nextstate ==
      (prevstate = PAObj_GetState(ts, obj, N("on-or-off-state")))) {
    return;
  }
  Dbg(DBGPLAN, DBGDETAIL, "shower state change", E);
  if (prevstate) TE(ts, L(prevstate, obj, E));
  DbAssert(tsrange, L(nextstate, obj, E));
}

/* SUBGOAL shower-on */
void PA_ShowerOn(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_ShowerOn", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = DbRetrievePart(ts, NULL, N("cold-faucet"), I(o, 1)))) {
        goto failure;
      }
      SG(cx, sg, 1, STFAILURE, L(N("knob-position-5"), p, E));
      return;
    case 1:
      if (!(p = DbRetrievePart(ts, NULL, N("hot-faucet"), I(o, 1)))) {
        goto failure;
      }
      SG(cx, sg, 2, STFAILURE, L(N("knob-position-5"), p, E));
      return;
    case 2:
      if (!(p = DbRetrievePart(ts, NULL, N("shower-switch"), I(o, 1)))) {
        goto failure;
      }
      SG(cx, sg, 3, STFAILURE, L(N("switch-on"), p, E));
      return;
    case 3:
      if (WAIT_PTN(cx, sg, 1, STSUCCESS, L(N("shower-on"), I(o, 1), E))) return;
      WAIT_TS(cx, sg, ts, (Dur)10, STFAILURE);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_ShowerOn: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL shower-off */
void PA_ShowerOff(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_ShowerOff", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = DbRetrievePart(ts, NULL, N("cold-faucet"), I(o, 1)))) {
        goto failure;
      }
      SG(cx, sg, 1, STFAILURE, L(N("knob-position-0"), p, E));
      return;
    case 1:
      if (!(p = DbRetrievePart(ts, NULL, N("hot-faucet"), I(o, 1)))) {
        goto failure;
      }
      SG(cx, sg, 2, STFAILURE, L(N("knob-position-0"), p, E));
      return;
    case 2:
      if (WAIT_PTN(cx, sg, 1, STSUCCESS, L(N("shower-off"), I(o, 1), E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)10, STFAILURE);
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_ShowerOff: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL take-shower */
void PA_TakeShower(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_TakeShower", E);
  switch (sg->state) {
    case STBEGIN:
      /* todo: Consider moving all object fetches to here, to enable a
       * quicker "make sense" check than full simulation. For example,
       * if a person doesn't have hair to wash, this will catch it sooner.
       * We could also do these make-sense checks in the activation
       * code in the UA.
       */
      if (!FINDO(cx, ts, N("shower"), o, N("shower"), a, NULL)) goto failure;
      SG(cx, sg, 1, STFAILURE, L(N("strip"), a, E));
      return;
    case 1:
      SG(cx, sg, 2, STFAILURE,
         L(N("shower-on"), GETO(ts, N("shower"), o), E));
      return;
    case 2:
      if (!FINDP(cx, ts, N("hand1"), o, N("right-hand"), a)) goto failure;
      if (!FINDP(cx, ts, N("hair"), o, N("hair"), a)) goto failure;
      if (!FINDP(cx, ts, N("soap-dish"), o, N("soap-dish"),
                 GETO(ts, N("shower"), o))) {
        goto failure;
      }
      if (!FINDP(cx, ts, N("shower-head"), o, N("shower-head"),
                     GETO(ts, N("shower"), o))) {
        goto failure;
      }
      if (I(o,0) == N("take-shower-no-hairwash")) {
        TOSTATE(cx, sg, 200);
      } else {
        TOSTATE(cx, sg, 100);
      }
      return;
/* HAIR WASH */
    case 100:
      if (cx->mode == MODE_SPINNING && sg->spin_to_state < 100) {
        TOSTATE(cx, sg, STBEGIN);
        return;
      }
      if (!(p = FINDO(cx, ts, N("shampoo"), o, N("shampoo"), a, NULL))) {
        goto failure;
      }
      SG(cx, sg, 101, STFAILURE, L(N("grasp"),
                                   GETO(ts, N("hand1"), o), p, E));
      return;
    case 101:
      if (!FINDP(cx, ts, N("hand2"), o, N("left-hand"), a)) goto failure;
      SG(cx, sg, 102, STFAILURE, L(N("action-open"), GETO(ts, N("hand2"), o),
                                   GETO(ts, N("shampoo"), o), E));
      return;
    case 102:
      SG(cx, sg, 103, STFAILURE, L(N("pour-onto"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("shampoo"), o),
                                   GETO(ts, N("hair"), o), E));
      return;
    case 103:
      SG(cx, sg, 104, STFAILURE, L(N("action-close"),
                                   GETO(ts, N("hand2"), o),
                                   GETO(ts, N("shampoo"), o), E));
      return;
    case 104:
      SG(cx, sg, 105, STFAILURE, L(N("move-to"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("hand1"), o),
                                   GETO(ts, N("soap-dish"), o), E));
      return;
    case 105:
      SG(cx, sg, 106, STFAILURE, L(N("release"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("shampoo"), o), E));
      return;
    case 106:
      SG(cx, sg, 107, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("hair"), o), D(30.0), E));
      return;
    case 107:
      SG(cx, sg, 108, STFAILURE, L(N("move-to"), GETO(ts, N("hair"), o),
                                   GETO(ts, N("hair"), o),
                                   GETO(ts, N("shower-head"), o), E));
      return;
    case 108:
      SG(cx, sg, 200, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("hair"), o), D(30.0), E));
      return;
/* WASH */
    case 200:
      if (cx->mode == MODE_SPINNING && sg->spin_to_state < 200) {
        TOSTATE(cx, sg, 100);
        return;
      }
      if (!FINDO(cx, ts, N("soap"), o, N("soap"),
                 GETO(ts, N("shower"), o), NULL)) {
        goto failure;
      } 
      SG(cx, sg, 201, STFAILURE, L(N("grasp"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("soap"), o), E));
      return;
    case 201:
      if (!FINDP(cx, ts, N("face"), o, N("face"), a)) {
        goto failure;
      }
      SG(cx, sg, 202, STFAILURE, L(N("move-to"), GETO(ts, N("face"), o),
                                   GETO(ts, N("face"), o),
                                   GETO(ts, N("shower-head"), o), E));
      return;
    case 202:
      SG(cx, sg, 203, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("face"), o), D(10.0), E));
      return;
    case 203:
      if (!FINDP(cx, ts, N("left-arm"), o, N("left-arm"), a)) goto failure;
      SG(cx, sg, 204, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("left-arm"), o), D(10.0), E));
      return;
    case 204:
      if (!FINDP(cx, ts, N("right-arm"), o, N("right-arm"), a)) goto failure;
      SG(cx, sg, 205, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("right-arm"), o), D(10.0), E));
      return;
    case 205:
      if (!FINDP(cx, ts, N("trunk"), o, N("trunk"), a)) goto failure;
      SG(cx, sg, 206, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("trunk"), o), D(10.0), E));
      return;
    case 206:
      if (!FINDP(cx, ts, N("left-leg"), o, N("left-leg"), a)) goto failure;
      SG(cx, sg, 207, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("left-leg"), o), D(10.0), E));
      return;
    case 207:
      if (!FINDP(cx, ts, N("right-leg"), o, N("right-leg"), a)) goto failure;
      SG(cx, sg, 208, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("right-leg"), o), D(10.0), E));
      return;
    case 208:
      SG(cx, sg, 209, STFAILURE, L(N("move-to"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("hand1"), o),
                                   GETO(ts, N("soap-dish"), o), E));
      return;
    case 209:
      SG(cx, sg, 210, STFAILURE, L(N("release"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("soap"), o), E));
      return;
/* RINSE */
    case 210:
      SG(cx, sg, 211, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("face"), o), D(10.0), E));
      return;
    case 211:
      SG(cx, sg, 212, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("left-arm"), o), D(10.0), E));
      return;
    case 212:
      SG(cx, sg, 213, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("right-arm"), o), D(10.0), E));
      return;
    case 213:
      SG(cx, sg, 214, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("trunk"), o), D(10.0), E));
      return;
    case 214:
      SG(cx, sg, 215, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("left-leg"), o), D(10.0), E));
      return;
    case 215:
      SG(cx, sg, 300, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("right-leg"), o), D(10.0), E));
      return;
/* DRY OFF */
    case 300:
      if (cx->mode == MODE_SPINNING && sg->spin_to_state < 300) {
        TOSTATE(cx, sg, 200);
        return;
      }
      SG(cx, sg, 301, STFAILURE, L(N("shower-off"),
                                   GETO(ts, N("shower"), o), E));
      return;
    case 301:
      if (!(p = FINDO(cx, ts, N("towel"), o, N("towel"), a, NULL))) {
        goto failure;
      }
      SG(cx, sg, 302, STFAILURE, L(N("grasp"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("towel"), o), E));
      return;
    case 302:
      SG(cx, sg, 303, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("hair"), o), D(10.0), E));
      return;
    case 303:
      SG(cx, sg, 304, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("face"), o), D(10.0), E));
      return;
    case 304:
      SG(cx, sg, 305, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("left-arm"), o), D(10.0), E));
      return;
    case 305:
      SG(cx, sg, 306, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("right-arm"), o), D(10.0), E));
      return;
    case 306:
      SG(cx, sg, 307, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("trunk"), o), D(10.0), E));
      return;
    case 307:
      SG(cx, sg, 308, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("left-leg"), o), D(10.0), E));
      return;
    case 308:
      SG(cx, sg, 309, STFAILURE, L(N("rub"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("right-leg"), o), D(10.0), E));
      return;
    case 309:
      if (!(p = FINDO(cx, ts, N("towel-rack"), o, N("towel-rack"), a,
                      NULL))) {
        goto failure;
      }
      SG(cx, sg, 310, STFAILURE, L(N("move-to"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("hand1"), o),
                                   GETO(ts, N("towel-rack"), o), E));
      return;
    case 310:
      SG(cx, sg, 999, STFAILURE, L(N("release"), GETO(ts, N("hand1"), o),
                                   GETO(ts, N("towel"), o), E));
      return;
    case 999:
      if (cx->mode == MODE_SPINNING && sg->spin_to_state < 999) {
        TOSTATE(cx, sg, 300);
        return;
      }
      AR(&sg->startts, ts, o);
      TOSTATE(cx, sg, STSUCCESS);
      return;
    case STSUCCESS:
      if (cx->mode == MODE_SPINNING) {
        TOSTATE(cx, sg, 300);
        return;
      }
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_TakeShower: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

void UA_TakeShower(Actor *ac, Ts *ts, Obj *a, Obj *in)
{
  Subgoal	*sg;
  /* "Jim pours some Wash & Go on his hair." */
  if (ISA(N("pour-onto"), I(in, 0)) &&
      DbIsPartOf(ts, NULL, I(in, 1), a) &&
      ISA(N("shampoo"), I(in, 2)) &&
      ISA(N("hair"), I(in, 3))) {
    for (sg = ac->subgoals; sg; sg = sg->next) {
      if (!ISA(N("take-shower"), I(sg->obj, 0))) continue;
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_EXPECTED);
      PA_SpinTo(ac->cx, sg, 103);
      return;
    }
    sg = TG(ac->cx, ts, a, L(N("take-shower"), a, E));
    if (ObjIsConcrete(I(in, 2))) {
      AS(ts, 0, L(N("shampoo"), sg->obj, I(in, 2), E));
    }
    ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_MOSTLY, NOVELTY_TOTAL);
    PA_SpinTo(ac->cx, sg, 103);
    return;
  }
}

/* End of file. */
