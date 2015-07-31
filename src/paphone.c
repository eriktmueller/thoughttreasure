/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "pa.h"
#include "pagrasp.h"
#include "paphone.h"
#include "repbasic.h"
#include "repcloth.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "reptime.h"
#include "utildbg.h"

Obj *PAObj_DigitsToPhone(Ts *ts, Obj *digits)
{
  return(R1EI(1, ts, L(N("phone-number-of"), ObjWild, digits, E)));
}

Obj *PAObj_PhoneToDigits(Ts *ts, Obj *phone)
{
  return(R1EI(2, ts, L(N("phone-number-of"), phone, ObjWild, E)));
}

/* todo: This is omniscient: It has access to state it should not have
 * access to. Planner should use strategies such as looking the phone
 * number up in a phone or address book.
 */
Obj *PAObj_LookupPhoneNumber(Ts *ts, Obj *human)
{
  Obj	*phone, *digits;
  if (!(phone = SpaceFindNearestOwned(ts, NULL, N("phone"), human, NULL))) {
    return(NULL);
  }
  if (!(digits = PAObj_PhoneToDigits(ts, phone))) {
    Dbg(DBGPLAN, DBGDETAIL, "no phone-number-of <%s>", M(phone), E);
    return(NULL);
  }
  return(digits);
}

Obj *PAObj_Phonestate(Ts *ts, Obj *phone, /* RESULTS */ Obj **r2)
{
  Obj	*p;
  if ((p = R1D(ts, L(N("phonestate"), phone, ObjWild, E), 0))) {
    if (r2) *r2 = I(p, 2);
    return(I(p, 0));
  }
  if (r2) *r2 = NULL;
  return(N("phonestate-idle"));
}

void PAObj_ChangePhoneState(Ts *ts, Obj *phone, Obj *prevstate, Obj *nextstate,
                            Obj *r2)
{
  Dbg(DBGPLAN, DBGDETAIL, "phone <%s> state change", M(phone));
  TE(ts, L(prevstate, phone, ObjWild, E));
  if (r2) AS(ts, 0, L(nextstate, phone, r2, E));
  else AS(ts, 0, L(nextstate, phone, E));
}

Obj *PAObj_Phone2(Ts *ts, Obj *phone, Obj *state, Obj *otherphone,
                  Obj *hookstate, Obj *digits,
                  /* RESULTS */ Obj **otherphone_r)
{
  *otherphone_r = NULL;
  if (PAObj_IsBroken(ts, phone)) return(N("phonestate-idle"));
  if (state == N("phonestate-idle")) {
    if (hookstate == N("off-hook")) return(N("phonestate-dialtone"));
  } else if (state == N("phonestate-dialtone")) {
    if (hookstate == N("on-hook")) return(N("phonestate-idle"));
    if (digits) {
      if ((otherphone = PAObj_DigitsToPhone(ts, digits))) {
        if (N("phonestate-idle") != PAObj_Phonestate(ts, otherphone, NULL)) {
          return(N("phonestate-busy-signal"));
        }
        PAObj_ChangePhoneState(ts, otherphone, N("phonestate-idle"),
                                 N("phonestate-ringing"), phone);
        *otherphone_r = otherphone;
        return(N("phonestate-audible-ring"));
      } else return(N("phonestate-intercept"));
    }
  } else if (state == N("phonestate-ringing")) {
    if (hookstate == N("off-hook")) {
      PAObj_ChangePhoneState(ts, otherphone,
                             PAObj_Phonestate(ts, otherphone, NULL),
                             N("phonestate-voice-connection"), phone);
      *otherphone_r = otherphone;
      return(N("phonestate-voice-connection"));
    }
  } else if (state == N("phonestate-audible-ring")) {
    if (hookstate == N("on-hook")) {
      PAObj_ChangePhoneState(ts, otherphone,
                             PAObj_Phonestate(ts, otherphone, NULL),
                             N("phonestate-idle"), NULL);
      return(N("phonestate-idle"));
    }
  } else if (state == N("phonestate-voice-connection")) {
    if (hookstate == N("on-hook")) {
      PAObj_ChangePhoneState(ts, otherphone,
                             PAObj_Phonestate(ts, otherphone, NULL),
                             N("phonestate-dialtone"), NULL);
      return(N("phonestate-idle"));
    }
  }
  return(state);
}

void PAObj_Phone1(Ts *ts, Obj *phone)
{
  Obj	*dial, *hookstate, *digits, *r2, *next_r2;
  Obj	*nextstate, *prevstate;
  prevstate = PAObj_Phonestate(ts, phone, &r2);
  dial = DbRetrievePart(ts, NULL, N("phone-dial"), phone);
  if (!(hookstate = PAObj_GetPartState(ts, phone, N("phone-handset"),
                                       N("hook-state")))) {
    return;
  }
  digits = R1EI(3, ts, L(N("dial"), ObjWild, dial, ObjWild, E));
  nextstate = PAObj_Phone2(ts, phone, prevstate, r2, hookstate, digits,
                           &next_r2);
  if (nextstate == prevstate) return;
  PAObj_ChangePhoneState(ts, phone, prevstate, nextstate, next_r2);
}

void PAObj_Phone(TsRange *tsrange, Obj *phone)
{
  PAObj_Phone1(&tsrange->startts, phone);
}

/* SUBGOAL call */
void PA_Call(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj	*p, *phonenum;
  Dbg(DBGPLAN, DBGOK, "PA_Call", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = FINDO(cx, ts, N("phone"), o, N("phone"), a, NULL))) {
        goto failure;
      }
      if (!FINDP(cx, ts, N("phone-handset"), o, N("phone-handset"), p)) {
        goto failure;
      }
      if (!FINDP(cx, ts, N("phone-dial"), o, N("phone-dial"), p)) {
        goto failure;
      }
      if (!FINDP(cx, ts, N("hand1"), o, N("hand"), a)) goto failure;
      if (!FINDP(cx, ts, N("hand2"), o, N("hand"), a)) goto failure;
      sg->i = 0;
      TOSTATE(cx, sg, 1);
      return;
    case 1:
      SG(cx, sg, 21, STFAILURE,
         L(N("off-hook"), GETO(ts, N("phone-handset"), o), E));
      return;
    case 21:
      if (WAIT_PTN(cx, sg, 1, 22,
                   L(N("phonestate-dialtone"),
                     GETO(ts, N("phone"), o), E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)30, 777);
      return;
    case 22:
      if (!(phonenum = PAObj_LookupPhoneNumber(ts, I(o, 2)))) goto failure;
      SG(cx, sg, 3, STFAILURE, L(N("dial"), GETO(ts, N("hand2"), o),
                                 GETO(ts, N("phone-dial"), o),
                                 phonenum, E));
      return;
    case 3:
      if (WAIT_PTN(cx, sg, 1, 777,
                   L(N("phonestate-busy-signal"),
                     GETO(ts, N("phone"), o), E))) {
        return;
      }
      if (WAIT_PTN(cx, sg, 1, 888,
                   L(N("phonestate-intercept"),
                     GETO(ts, N("phone"), o), E))) {
        return;
      }
      if (WAIT_PTN(cx, sg, 1, 4,
               L(N("phonestate-audible-ring"),
                 GETO(ts, N("phone"), o), E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)10, 777);
      return;
    case 4:
      if (WAIT_PTN(cx, sg, 1, 5,
                   L(N("phonestate-voice-connection"),
                     GETO(ts, N("phone"), o), ObjWild, E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)30, 777);
      return;
    case 5:
      if (WAIT_PTN(cx, sg, 1, 61,
                   L(N("interjection-of-greeting"), ObjWild, ObjWild, E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)30, 777);
      return;
    case 61:
      ADDO(ts, N("called-party"), o, I(sg->trigger, 1), 0);
      if (I(sg->trigger, 1) != I(o, 2)) {
        /* todo: May I please speak to. */
        TOSTATE(cx, sg, 990);
        return;
      }
      AS(ts, 0, L(N("near-audible"), a, GETO(ts, N("called-party"), o), E));
      SG(cx, sg, 62, STFAILURE, L(N("calling-party-telephone-greeting"), a,
                                     GETO(ts, N("called-party"), o), E));
     return;
    case 62:
      if (WAIT_PTN(cx, sg, 1, 7, L(N("mtrans"), a, I(o, 2), E))) return;
      WAIT_TS(cx, sg, ts, (Dur)5, 990);
      /* todo: Allow for more than one mtrans transaction. */
      return;
    case 7:
      if (WAIT_PTN(cx, sg, 1, 990, L(N("mtrans"), I(o, 2), a, E))) return;
      WAIT_TS(cx, sg, ts, (Dur)5, 990);
      return;
    case 777:	/* Retry. */
      if (sg->i > 3) {
        TOSTATE(cx, sg, 888);
      } else {
        sg->i++;
        SG(cx, sg, 1, STFAILURE,
           L(N("on-hook"), GETO(ts, N("phone-handset"), o), E));
      }
      return;
    case 888:	/* Failure script termination. */
      SG(cx, sg, STFAILURE, STFAILURE,
         L(N("on-hook"), GETO(ts, N("phone-handset"), o), E));
      return;
    case 990:	/* End conversation. */
      SG(cx, sg, 991, STFAILURE,
         L(N("interjection-of-departure"), a,
           GETO(ts, N("called-party"), o), E));
      return;
    case 991:
      if (WAIT_PTN(cx, sg, 1, 998, L(N("interjection-of-departure"),
                                 GETO(ts, N("called-party"), o), a, E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)5, 998);
      return;
    case 998:	/* Success script termination. */
      TE(ts, L(N("near-audible"), a, GETO(ts, N("called-party"), o), E));
      TE(ts, L(N("near-audible"), GETO(ts, N("called-party"), o), a, E));
      SG(cx, sg, 999, STFAILURE,
         L(N("on-hook"), GETO(ts, N("phone-handset"), o), E));
      return;
    case 999:
      AR(&sg->startts, ts, o);
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Call: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL handle-call ?human */
void PA_HandleCall(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj	*p;
  Dbg(DBGPLAN, DBGOK, "PA_HandlePhoneCalls", E);
  switch (sg->state) {
    case STBEGIN:
      /* todo: Inelegant. For young children, parents answer if they are
       * there. Teenage children answer parents' phone if they don't have
       * their own.
       */
      if (GuardianOf(ts, NULL, a)) goto failure;
      if (!(p = FINDO(cx, ts, N("phone"), o, N("phone"), a, NULL))) {
        goto failure;
      }
      if (!FINDP(cx, ts, N("phone-handset"), o, N("phone-handset"), p)) {
        goto failure;
      }
      TOSTATE(cx, sg, 1);
      return;
    case 1:
      if (WAIT_PTN(cx, sg, 1, 2,
                   L(N("phonestate-ringing"), GETO(ts, N("phone"), o),
                               ObjWild, E))) return;
      return;
    case 2:
      if (RE(ts, L(N("holding"), ObjWild,
             GETO(ts, N("phone-handset"), o), E))) {
        Dbg(DBGPLAN, DBGDETAIL, "abandon: someone else is handling it", E);
        TOSTATE(cx, sg, STSUCCESS);
        return;
      }
      SG(cx, sg, 3, STFAILURE,
         L(N("off-hook"), GETO(ts, N("phone-handset"), o), E));
      return;
    case 3:
      SG(cx, sg, 4, STFAILURE, L(N("telephone-greeting"), a, ObjWild, E));
      return;
    case 4:
      if (WAIT_PTN(cx, sg, 1, 5, L(N("mtrans"), ObjWild, a, E))) return;
      WAIT_TS(cx, sg, ts, (Dur)10, 7);
      return;
    case 5:
      ADDO(ts, N("calling-party"), o, I(sg->trigger, 1), 0);
      TOSTATE(cx, sg, 6);
      return;
    case 6:
      if (ISA(N("interjection-of-departure"), I(sg->trigger, 0))) {
        TOSTATE(cx, sg, 7);
        return;
      }
      if (WAIT_PTN(cx, sg, 1, 6,
                   L(N("mtrans"), GETO(ts, N("calling-party"), o), a, E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)30, 7);
      return;
    case 7:
      SG(cx, sg, 998, STFAILURE,
         L(N("interjection-of-departure"), a,
           GETOO(ts, N("calling-party"), o), E));
      return;
    case 998:
      SG(cx, sg, 999, STFAILURE,
         L(N("on-hook"), GETO(ts, N("phone-handset"), o), E));
      return;
    case 999:
      AR(&sg->startts, ts, o);
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_HandlePhoneCalls: undefined state %d",
          sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL off-hook ?phone-handset */
void PA_OffHook(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_OffHook", E);
  switch (sg->state) {
    case STBEGIN:
      if (!FINDP(cx, ts, N("hand1"), o, N("hand"), a)) goto failure;
      SG(cx, sg, STSUCCESS, STFAILURE,
         L(N("pick-up"), GETO(ts, N("hand1"), o), I(o,1), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_OffHook: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL on-hook ?phone-handset */
void PA_OnHook(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_OnHook", E);
  switch (sg->state) {
    case STBEGIN:
      if ((p = R1EI(1, ts, L(N("holding"), ObjWild, I(o, 1), E))) &&
          (a == DbRetrieveWhole(ts, NULL, N("human"), p))) {
        ADDO(ts, N("hand1"), o, p, 1);
      } else {
        if (!FINDP(cx, ts, N("hand1"), o, N("hand"), a)) goto failure;
      }
      SG(cx, sg, STSUCCESS, STFAILURE,
         L(N("hang-up"), GETO(ts, N("hand1"), o), I(o, 1), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_OnHook: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL pick-up ?grasper ?phone-handset */
void PA_PickUp(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_PickUp", E);
  switch (sg->state) {
    case STBEGIN:
      if (!FINDP(cx, ts, N("ear"), o, N("ear"), a)) goto failure;
      SG(cx, sg, 1, STFAILURE, L(N("grasp"), I(o, 1), I(o, 2), E));
      return;
    case 1:
      SG(cx, sg, 2, STFAILURE,
         L(N("move-to"), I(o, 1), I(o, 1), GETO(ts, N("ear"), o), E));
      return;
    case 2:
      TE(ts, L(N("on-hook"), I(o, 2), E));
      AR(&sg->startts, ts, o);
      AS(ts, 0, L(N("off-hook"), I(o, 2), E));
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_PickUp: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL hang-up ?grasper ?phone-handset */
void PA_HangUp(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_HangUp", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, 1, STFAILURE, L(N("holding"), I(o, 1), I(o, 2), E));
      return;
    case 1:
      if (!(p = DbRetrieveWhole(ts, NULL, N("phone"), I(o, 2)))) goto failure;
      SG(cx, sg, 2, STFAILURE, L(N("move-to"), I(o, 1), I(o, 1), p, E));
      return;
    case 2:
      SG(cx, sg, 3, STFAILURE, L(N("release"), I(o, 1), I(o, 2), E));
      return;
    case 3:
      TE(ts, L(N("off-hook"), I(o, 2), E));
      AR(&sg->startts, ts, o);
      AS(ts, 0, L(N("on-hook"), I(o, 2), E));
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_HangUp: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL dial ?grasper ?phone-dial ?number */
void PA_Dial(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_Dial", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, 1, STFAILURE, L(N("move-to"), I(o, 1), I(o, 1), I(o, 2), E));
      return;
    /* todo: More detailed dialing action. */
    case 1:
      AR(&sg->startts, ts, o);
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Dial: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* End of file. */
