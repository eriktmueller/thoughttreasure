/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940601: started this file
 */

#include "tt.h"
#include "pa.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "reptime.h"
#include "utildbg.h"

#include "paatrans.h"

void AmountParse(Obj *amount, /* RESULTS */ Float *notional, Obj **currency)
{
  *notional = ObjToNumber(amount);
  *currency = ObjToNumberClass(amount);
}

ObjList *CashPickOutAmount(Ts *ts, Obj *container, Float *change, Obj *amount)
{
  Float		notional, specnot, largestnot, smallestnot, pickedout;
  Obj		*currency, *spec, *speccurr, *largestspec, *smallestspec;
  ObjList	*objs, *p, *r;
  r = NULL;		
  AmountParse(amount, &notional, &currency);
  objs = RD(ts, L(N("inside"), currency, container, E), 1);
  pickedout = 0.0;
  /* Repeatedly get largest bill less than amount remaining. */
  while (notional - pickedout > 0.0) {
    largestnot = FLOATNEGINF;
    largestspec = NULL;
    for (p = objs; p; p = p->next) {
      spec = R1NI(2, ts, L(N("value-of"), I(p->obj, 1), ObjWild, E), 1);
      AmountParse(spec, &specnot, &speccurr);
      if ((speccurr == currency) &&
          (specnot < (notional - pickedout)) &&
          (specnot > largestnot)) {
        largestnot = specnot;
        largestspec = I(p->obj, 1);
      }
    }
    if (!largestspec) break;
    r = ObjListCreate(largestspec, r);
    pickedout += largestnot;
  }
  if (notional == pickedout) {
    *change = 0.0;
    return(r);
  }
  /* Repeatedly get smallest remaining bill. */
  while (notional - pickedout > 0.0) {
    smallestnot = FLOATPOSINF;
    smallestspec = NULL;
    for (p = objs; p; p = p->next) {
      spec = R1NI(2, ts, L(N("value-of"), I(p->obj, 1), ObjWild, E), 1);
      AmountParse(spec, &specnot, &speccurr);
      if ((speccurr == currency) &&
          (specnot < smallestnot)) {
        smallestnot = specnot;
        smallestspec = I(p->obj, 1);
      }
    }
    if (!smallestspec) break;
    r = ObjListCreate(smallestspec, r);
    pickedout += smallestnot;
  }
  if (pickedout < notional) {
    ObjListFree(r);
    Dbg(DBGGEN, DBGBAD, "not enough money -- have %g, need %g",
        pickedout, notional);
    return(NULL);
  }
  *change = pickedout - notional;
  return(r);
}

ObjList *TicketFindNextAvailable(Ts *ts, Obj *container,
                                 ObjList *exclude, Obj *performance)
{
  ObjList	*objs, *p;
  objs = RD(ts, L(N("inside"), N("ticket"), container, E), 1);
  for (p = objs; p; p = p->next) {
    if (ObjListIn(I(p->obj, 1), exclude)) continue;
    if (performance == DbGetRelationValue(ts, NULL, N("event-of"),
                                          I(p->obj, 1),  NULL)) {
      return(ObjListCreate(I(p->obj, 1), exclude));
    }
  }
  return(NULL);
}

Bool PersonalCheckCalendar(Ts *ts, Obj *human, TsRange *candidate)
{
  /* todo */
  return(1);
}

Bool PersonalCheckBudget(Ts *ts, Obj *human, Obj *amount)
{
  /* todo */
  return(1);
}

/* SUBGOAL buy-ticket ?human ?live-performance */
/* todo: Multiple persons. */
void PA_BuyTicket(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj	*p;
  Dbg(DBGPLAN, DBGOK, "PA_BuyTicket", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, 1, STFAILURE, L(N("dress"), I(o, 1), I(o, 0), E));
      return;
    case 1:
      if (!(p = R1EI(2, &TsNA, L(N("location-of"), I(o,2), ObjWild, E)))) {
        Dbg(DBGPLAN, DBGBAD, "failure: location-of not found", E);
        goto failure;
      }
      SG(cx, sg, 14, STFAILURE, L(N("near-reachable"), a, p, E));
      return;
    case 14:
      if (!(p = SpaceFindNearest(ts, NULL, N("box-office"), a))) goto failure;
      SG(cx, sg, 15, STFAILURE, L(N("near-reachable"), a, p, E));
      return;
    case 15:
      if (!(p = SpaceFindNearest(ts, NULL, N("employee-side-of-counter"), a))) {
        goto failure;
      }
      if (!(sg->w.PuTi.boxofficeperson =
          SpaceFindNearest(ts, NULL, N("human"), p))) {
        goto failure;
      }
      if (!(p = SpaceFindNearest(ts, NULL, N("customer-side-of-counter"), a))) {
        goto failure;
      }
      SG(cx, sg, 2, STFAILURE, L(N("near-reachable"), a, p, E));
      return;
    case 2:
      /* todo: Wait for boxofficeperson to be free/stand on line. */
      SG(cx, sg, 3, STFAILURE, L(N("pre-sequence"), a,
         sg->w.PuTi.boxofficeperson, E));
      return;
    case 3:
      if (WAIT_PTN(cx, sg, 0, 4,
                   L(N("may-I-help-you"), sg->w.PuTi.boxofficeperson, a, E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)10, 2);
      return;
    case 4:
      SG(cx, sg, 5, STFAILURE,
         L(N("interjection-of-greeting"), a, sg->w.PuTi.boxofficeperson, E));
      return;
    case 5:
      SG(cx, sg, 6, STFAILURE,
         L(N("request"), a, sg->w.PuTi.boxofficeperson, o, E));
      return;
    case 6:
      if (WAIT_PTN(cx, sg, 0, 13,
          L(N("I-am-sorry"), sg->w.PuTi.boxofficeperson, a, E))) {
        return;
      }
      if (WAIT_PTN(cx, sg, 0, 8,
                   L(N("describe"), sg->w.PuTi.boxofficeperson, a,
                     N("ticket"), E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)20, 5);
      return;
/*
    case 7:
      sg->w.PuTi.ticket = I(sg->trigger, 3);
       * todo: We really need to maintain knowledge states separately for
       * each person. But listen lets us bypass this by getting people to
       * a single global (common) knowledge state about an object.
       *
      SG(cx, sg, 8, STFAILURE, L(N("listen"), a,
                             sg->w.PuTi.boxofficeperson, sg->w.PuTi.ticket, E));
      return;
*/
    case 8:
      /* todo: Seat number. */
      sg->w.PuTi.ticket = I(sg->trigger, 3);
      sg->w.PuTi.perfts = R1NI(2, ts, L(N("performance-ts-of"),
                               sg->w.PuTi.ticket, ObjWild, E), 1);
      if (WAIT_PTN(cx, sg, 0, 9,
                   L(N("propose-transaction"), sg->w.PuTi.boxofficeperson, a,
                     sg->w.PuTi.ticket, ObjWild, E))) {
        return;
      }
      return;
    case 9:
      /* todo: Check for disagreement with cost-of from ad. */
      sg->w.PuTi.price = I(sg->trigger, 4);
      if (sg->w.PuTi.perfts == NULL) {
        TOSTATE(cx, sg, 13);
      } else if (PersonalCheckCalendar(ts, a, ObjToTsRange(sg->w.PuTi.perfts))
                 && PersonalCheckBudget(ts, a, sg->w.PuTi.price)) {
        SG(cx, sg, 10, STFAILURE,
           L(N("accept"), a, sg->w.PuTi.boxofficeperson, E));
      } else { /* todo: Give a reason? */
        SG(cx, sg, 6, STFAILURE,
           L(N("reject"), a, sg->w.PuTi.boxofficeperson, E));
      }
      return;
    case 10:
      SG(cx, sg, 11, STFAILURE,
         L(N("pay-in-person"), a, sg->w.PuTi.boxofficeperson,
           sg->w.PuTi.price, E));
      return;
    case 11: /* todo: If this fails, get money back */
      SG(cx, sg, 12, 13, L(N("receive-from"), a, sg->w.PuTi.boxofficeperson,
                           sg->w.PuTi.ticket, E));
      return;
    case 12:
      AS(ts, 0, L(N("owner-of"), sg->w.PuTi.ticket, a, E));
      /* todo: Put ticket in wallet. */
      SG(cx, sg, STSUCCESS, STSUCCESS,
         L(N("post-sequence"), a, sg->w.PuTi.boxofficeperson, E));
      return;
    case 13:
      SG(cx, sg, STFAILURE, STFAILURE,
         L(N("post-sequence"), a, sg->w.PuTi.boxofficeperson, E));
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_BuyTicket: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL pay-in-person ?human ?human ?amount */
void PA_PayInPerson(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj	*p;
  Dbg(DBGPLAN, DBGOK, "PA_Pay", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = SpaceFindNearestOwned(ts, NULL, N("wallet"), a, a))) {
        goto failure;
      }
      SG(cx, sg, STSUCCESS, 1, L(N("pay-cash"), a, I(o, 2), I(o, 3), p, E));
      return;
    case 1:
      if (!(p = SpaceFindNearestOwned(ts, NULL, N("wallet"), a, a))) {
        goto failure;
      }
      SG(cx, sg, STSUCCESS, 2, L(N("pay-by-card"), a, I(o, 2), I(o, 3), p, E));
      return;
    case 2:
      if (!(p = SpaceFindNearestOwned(ts, NULL, N("wallet"), a, a))) {
        goto failure;
      }
      SG(cx, sg, STSUCCESS, STFAILURE,
         L(N("pay-by-check"), a, I(o, 2), I(o, 3), p, E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Pay: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL pay-cash ?human ?human ?amount ?cash-container */
void PA_PayCash(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_PayCash", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, 1, STFAILURE, L(N("near-reachable"), a, I(o, 4), E));
      return;
    case 1:
      if (!(sg->w.PaCa.cash =
            CashPickOutAmount(ts, I(o, 4), &sg->w.PaCa.change, I(o, 3)))) {
        goto failure;
      }
      sg->w.PaCa.nextcash = sg->w.PaCa.cash;
      TOSTATE(cx, sg, 2);
      return;
    case 2: /* HAND OVER NEXT BILL */
      if (!sg->w.PaCa.nextcash) {
        ObjListFree(sg->w.PaCa.cash);
        if (sg->w.PaCa.change > 0.0) {
          TOSTATE(cx, sg, 3);
        } else {
          TOSTATE(cx, sg, STSUCCESS);
        }
      } else {
        SG(cx, sg, 2, STFAILURE,
           L(N("hand-to"), a, I(o,2), sg->w.PaCa.nextcash->obj, E));
        sg->w.PaCa.nextcash = sg->w.PaCa.nextcash->next;
      }
      return;
    case 3: /* COLLECT CHANGE */
      SG(cx, sg, STSUCCESS, STFAILURE,
         L(N("collect-cash"), a, I(o, 2), NumberToObj(sg->w.PaCa.change),
           I(o, 4), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_PayCash: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL pay-by-card ?human ?human ?amount ?container */
void PA_PayByCard(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_PayByCard", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, 1, STFAILURE, L(N("near-reachable"), a, I(o, 4), E));
      return;
    case 1:
      if (!(sg->w.PBD.creditcard =
            R1DI(1, ts, L(N("inside"), N("credit-card"), I(o, 4), E), 1))) {
        Dbg(DBGPLAN, DBGBAD, "no credit card inside container");
        goto failure;
      }
      SG(cx, sg, 2, STFAILURE,
         L(N("hand-to"), a, I(o,2), sg->w.PBD.creditcard, E));
      return;
    case 2:
    /* todo: Wait to enter code or sign paper, then wait for card back. */
      TOSTATE(cx, sg, STFAILURE);
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_PayByCard: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL pay-by-check ?human ?human ?amount ?container */
void PA_PayByCheck(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_PayByCheck", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, 1, STFAILURE, L(N("near-reachable"), a, I(o, 4), E));
      return;
    case 1:
      if (!(sg->w.PBC.check =
            R1DI(1, ts, L(N("inside"), N("check"), I(o, 4), E), 1))) {
        Dbg(DBGPLAN, DBGBAD, "no check inside container");
        goto failure;
      }
      SG(cx, sg, 2, STFAILURE, L(N("write"), a, sg->w.PBC.check, I(o, 3), E));
      return;
    case 2:
      SG(cx, sg, 3, STFAILURE,
         L(N("write"), a, sg->w.PBC.check, N("signature"), E));
      return;
    case 3:
      SG(cx, sg, 4, STFAILURE, L(N("hand-to"), a, I(o,2), sg->w.PBC.check, E));
      return;
    case 4: /* todo: wait for id request, verification, etc. */
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_PayByCheck: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL work-box-office ?human ?box-office */
void PA_WorkBoxOffice(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj		 *p;
  TsRange	tsr;
  Dbg(DBGPLAN, DBGOK, "PA_WorkBoxOffice", E);
  switch (sg->state) {
    case STBEGIN:
        /* todo:
      SG(cx, sg, 1, STFAILURE, L(N("dress"), a, I(o, 0), E));
      return;
        */
    case 1:
      SG(cx, sg, 2, STFAILURE, L(N("near-reachable"), a, I(o, 2), E));
      return;
    case 2:
      if (!(p = SpaceFindNearest(ts, NULL, N("employee-side-of-counter"), a))) {
        goto failure;
      }
      if (!(sg->w.WBO.cashregister =
          SpaceFindNearest(ts, NULL, N("cash-register"), p))) {
        goto failure;
      }
      if (!(sg->w.WBO.ticketbox =
          SpaceFindNearest(ts, NULL, N("ticket-box"), p))) {
        goto failure;
      }
      SG(cx, sg, 3, STFAILURE, L(N("near-reachable"), a, p, E));
      return;
    case 3:
      /* todo: Answer phone. Leave when shift over or performance starts */
      /* todo: Something like:
      PUSHON(cx, sg, 101, L(N("attend"), ObjWild, a, E));
      PUSHON(cx, sg, 101, L(N("pre-sequence"), ObjWild, a, E));
      */
      TOSTATE(cx, sg, 100);
      return;
    case 100: /* WAIT FOR NEXT CUSTOMER */
      sg->w.WBO.customer = NULL;
      sg->w.WBO.tickets = NULL;
      sg->w.WBO.cashreceived = 0.0;
      if (WAIT_PTN(cx, sg, 0, 101, L(N("attend"), ObjWild, a, E))) return;
      if (WAIT_PTN(cx, sg, 0, 101, L(N("pre-sequence"), ObjWild, a, E))) return;
      WAIT_TS(cx, sg, ts, (Dur)120, STFAILURE);
        /* For debugging. */
      return;
    case 101: /* HANDLE NEW PERSON */
      if (sg->w.WBO.customer) {
        if (sg->w.WBO.customer != I(sg->trigger, 1)) {
          SG(cx, sg, STPOP, STPOP,
             L(N("please-wait-your-turn"), a, I(sg->trigger, 1), E));
        } else {
          TOSTATE(cx, sg, STPOP);
        }
      } else {
        sg->w.WBO.customer = I(sg->trigger, 1);
        TOSTATE(cx, sg, 102);
      }
      return;
    case 102:
      SG(cx, sg, 103, 100, L(N("may-I-help-you"), a, sg->w.WBO.customer, E));
      return;
    case 103: /* READY FOR CURRENT CUSTOMER'S REQUEST */
      /* todo: If customer leaves, go to 101. */
      if (WAIT_PTN(cx, sg, 0, 104,
                   L(N("request"), sg->w.WBO.customer, a, ObjWild, E))) {
        return;
      }
      if (WAIT_PTN(cx, sg, 0, 110,
                   L(N("end-of-order"), sg->w.WBO.customer, a, ObjWild, E))) {
        return;
      }
      if (WAIT_PTN(cx, sg, 0, 110,
                   L(N("post-sequence"), sg->w.WBO.customer, a, E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)5, 102);
      return;
    case 104: /* HANDLE CURRENT CUSTOMER'S REQUEST */
      if (I(I(sg->trigger, 3), 0) == N("buy-ticket")) {
        sg->w.WBO.retries = 0;
        TOSTATE(cx, sg, 105);
      } else if (I(I(sg->trigger, 3), 0) == N("know") &&
                 I(I(sg->trigger, 3), 2) == N("tod")) {
        p = L(N("current-time-sentence"), a, sg->w.WBO.customer, E);
        TsRangeSetNa(&tsr);
        tsr.startts = *ts;
        tsr.stopts = *ts;
        ObjSetTsRange(p, &tsr);
        /* todo: Using <p> below means that the subgoal can't be parsed.
         * Need to add inline tsr adder.
         */
        SG(cx, sg, 103, 100, p);
      } else {
        SG(cx, sg, 103, 100, L(N("interjection-of-noncomprehension"), a,
                               sg->w.WBO.customer, E));
      }
      return;
    case 105: /* SUGGEST A TICKET */
      sg->w.WBO.retries++;
      if ((sg->w.WBO.retries > 5) ||
          (!(sg->w.WBO.tickets =
            TicketFindNextAvailable(ts, sg->w.WBO.ticketbox,
              sg->w.WBO.tickets, I(I(sg->trigger, 3), 2))))) {
        SG(cx, sg, 103, 100, L(N("I-am-sorry"), a, sg->w.WBO.customer, E));
      } else {
        sg->w.WBO.priceofticket =
          R1NI(2, ts, L(N("ap"), sg->w.WBO.tickets->obj, ObjWild, E), 1);
        SG(cx, sg, 106, 100,
           L(N("describe"), a, sg->w.WBO.customer, sg->w.WBO.tickets->obj, E));
      }
      return;
    case 106:
      SG(cx, sg, 107, 100,
         L(N("propose-transaction"), a, sg->w.WBO.customer,
           sg->w.WBO.tickets->obj, sg->w.WBO.priceofticket, E));
      return;
    case 107: /* WAIT FOR RESPONSE */
      if (WAIT_PTN(cx, sg, 0, 108,
                   L(N("accept"), sg->w.WBO.customer, a, E))) {
        return;
      }
      if (WAIT_PTN(cx, sg, 0, 105,
                   L(N("reject"), sg->w.WBO.customer, a, E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)10, 105);
      return;
    case 108: /* COLLECT */
      SG(cx, sg, 109, 100,
         L(N("collect-payment"), a, sg->w.WBO.customer, 
           sg->w.WBO.priceofticket, sg->w.WBO.cashregister, E));
      return;
    case 109: /* HAND TICKET */
      SG(cx, sg, 103, 100,
         L(N("hand-to"), a, sg->w.WBO.customer, sg->w.WBO.tickets->obj, E));
      return;
    case 110: 
      SG(cx, sg, 100, 100, L(N("post-sequence"), a, sg->w.WBO.customer, E));
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_WorkBoxOffice: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL collect-payment ?human ?human ?amount ?cash-container */
void PA_CollectPayment(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_CollectPayment", E);
  switch (sg->state) {
    case STBEGIN:
      sg->w.CoPa.retries = 0;
      TOSTATE(cx, sg, 1);
      return;
    case 1:
      sg->w.CoPa.retries++;
      if (sg->w.CoPa.retries > 2) {
        goto failure;
      } else {
        SG(cx, sg, 2, STFAILURE, L(N("request-payment"), a, I(o, 2), E));
      }
      return;
    case 2:
      if (WAIT_PTN(cx, sg, 0, 3,
                   L(N("gesture-here"), I(o, 2), a, ObjWild, ObjWild, E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)5, 1);
      return;
    case 3:
      if (ISA(N("cash"), I(sg->trigger, 4))) {
        SG(cx, sg, STSUCCESS, 1,
           L(N("collect-cash"), a, I(o, 2), I(o, 3), I(o, 4), E));
      } else if (ISA(N("credit-card"), I(sg->trigger, 4))) {
        SG(cx, sg, STSUCCESS, 1,
           L(N("collect-card-payment"), a, I(o, 2), I(o, 3), I(o, 4), E));
      } else if (ISA(N("check"), I(sg->trigger, 4))) {
        SG(cx, sg, STSUCCESS, 1,
           L(N("collect-check-payment"), a, I(o, 2), I(o, 3), I(o, 4), E));
      } else {
        TOSTATE(cx, sg, 1);
      }
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_CollectPayment: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL collect-cash ?human ?human ?amount ?cash-container */
void PA_CollectCash(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Float	notional;
  Obj	*p, *spec, *currency;
  Dbg(DBGPLAN, DBGOK, "PA_CollectCash", E);
  switch (sg->state) {
    case STBEGIN:
      sg->w.CoCa.cashreceived = 0.0;
      sg->w.CoCa.amount = ObjToNumber(I(o, 3));
    case 1:
      SG(cx, sg, 2, 100, L(N("receive-from"), a, I(o, 2), N("cash"), E));
      return;
    case 2:
      if (!(p = DbRetrievePart(ts, NULL, N("right-hand"), a))) goto failure;
      if (!(sg->w.CoCa.holding =
          R1DI(2, ts, L(N("holding"), p, N("cash"), E), 2))) {
        if (!(p = DbRetrievePart(ts, NULL, N("left-hand"), a))) goto failure;
        if (!(sg->w.CoCa.holding =
              R1DI(2, ts, L(N("holding"), p, N("cash"), E), 2))) {
          Dbg(DBGPLAN, DBGDETAIL, "failure: not holding", E);
          goto failure;
        }
      }
      spec = R1NI(2, ts, L(N("value-of"), sg->w.CoCa.holding, ObjWild, E), 1);
      AmountParse(spec, &notional, &currency);
      sg->w.CoCa.cashreceived += notional;
      SG(cx, sg, 3, STFAILURE, L(N("inside"), sg->w.CoCa.holding, I(o, 4), E));
      return;
    case 3:
      if (sg->w.CoCa.cashreceived == sg->w.CoCa.amount) {
        SG(cx, sg, STSUCCESS, STSUCCESS,
           L(N("interjection-of-thanks"), a, I(o, 1), E));
      } else if (sg->w.CoCa.cashreceived > sg->w.CoCa.amount) {
        SG(cx, sg, STSUCCESS, STFAILURE, 
           L(N("pay-cash"), a, I(o, 2),
             NumberToObj(sg->w.CoCa.cashreceived - sg->w.CoCa.amount),
             I(o, 4), E));
      } else {
        TOSTATE(cx, sg, 1);
      }
      return;
    case 100:
      SG(cx, sg, 1, STFAILURE,
         L(N("owe"), I(o, 2), a,
           NumberToObj(sg->w.CoCa.amount - sg->w.CoCa.cashreceived),
           E));
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_CollectCash: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* End of file. */
