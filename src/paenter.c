/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940513: begun
 */

#include "tt.h"
#include "pa.h"
#include "paenter.h"
#include "pagrasp.h"
#include "repbasic.h"
#include "repcloth.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "reptime.h"
#include "utildbg.h"

/* SUBGOAL s-entertainment ?human */
void PA_SEntertainment(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_SEntertainment", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("watch-TV"), I(o,1), E));
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_SEntertainment: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL attend-performance ?human */
void PA_AttendPerformance(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_AttendPerformance", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(sg->w.AtPe.performance =
            R1EI(2, ts, L(N("live-performance"), o, ObjWild, E)))) {
        Dbg(DBGPLAN, DBGBAD, "no live-performance role", E);
        goto failure; /* todo: Select media object. */
      }  
      SG(cx, sg, 1, STFAILURE,
         L(N("buy-ticket"), a, sg->w.AtPe.performance, E));
      return;
    case 1:
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_AttendPerformance: undefined state %d",
          sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

Obj *TVSetFreqToSSP(Ts *ts, Obj *freq)
{
  char	*s;
  s = ObjToName(freq);
  /* todo: This is the setting of a French TV in Paris. */
  if (streq(s, "25frch")) return(N("ssp01"));
  else if (streq(s, "22frch")) return(N("ssp02"));
  else if (streq(s, "28frch")) return(N("ssp03"));
  else if (streq(s, "6frch")) return(N("ssp04"));
  else if (streq(s, "30frch")) return(N("ssp05"));
  else if (streq(s, "33frch")) return(N("ssp06"));
  else return(NULL);
}

/* todo: Take into account viewer preferences. Actor asks other participants. */
Obj *TVShowSelect(Ts *ts, Obj *viewer1, Obj *viewer2, Obj *viewer3, Obj *class,
                  Obj *tvset, /* RESULTS */ Ts *tsend, Obj **ssp)
{
  ObjList	*p1, *objs1, *p2, *objs2;
  Obj		*connector, *comms, *feedclass, *f, *o, *sp;

  if (!(connector =
        DbRetrievePart(ts, NULL, N("female-television-connector"), tvset))) {
    return(NULL);
  }
  if (!(comms = R1EI(2, ts, L(N("connected-to"), connector, ObjWild, E)))) {
    Dbg(DBGPLAN, DBGBAD, "antenna not connected");
    return(NULL);
  }
  if (ISA(N("antenna"), comms)) {
    feedclass = N("TV-land-broadcast");
    if (!(comms = SpaceFindNearest(ts, NULL, feedclass, tvset)))
      return(NULL);
  } else feedclass = comms;
  objs1 = RD(ts, L(N("video-channel-of"), class, ObjWild, E), 1);
  ObjListPrettyPrint(Log, objs1);
  for (p1 = objs1; p1; p1 = p1->next) {
    objs2 = RD(ts, L(N("feed-of"), I(p1->obj, 2), feedclass, E), 2);
    for (p2 = objs2; p2; p2 = p2->next) {
      if ((f = R1EI(2, ts, L(N("frequency-of"), I(p2->obj, 2), ObjWild, E))) &&
          (sp = TVSetFreqToSSP(ts, f))) {
        o = I(p1->obj, 1);
        ObjListFree(objs1);
        ObjListFree(objs2);
        *ssp = sp;
        TsRangeQueryStopTs(ObjToTsRange(p1->obj), ts, tsend);
        Dbg(DBGPLAN, DBGDETAIL, "stop ts:");
        TsPrint(Log, tsend);
        fputc(NEWLINE, Log);
        return(o);
      }
    }
    ObjListFree(objs2);
  }
  ObjListFree(objs1);
  Dbg(DBGPLAN, DBGBAD, "media object <%s> not found", M(class));
  return(NULL);
}

/* SUBGOAL TV-set-on ?TV-set */
void PA_TVSetOn(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p, *q;
  Dbg(DBGPLAN, DBGOK, "PA_TVSetOn", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = DbRetrievePart(ts, NULL, N("AC-plug"), I(o, 1)))) goto failure;
      if (!(RD(ts, L(N("connected-to"), p, N("AC-outlet"), E), 2))) {
        if (!(q = SpaceFindNearest(ts, NULL, N("AC-outlet"), I(o, 1)))) {
          goto failure;
        }
        SG(cx, sg, 1, STFAILURE, L(N("connected-to"), p, q, E));
      } else {
        TOSTATE(cx, sg, 1);
      }
      return;
    case 1:
      if (!(p = DbRetrievePart(ts, NULL, N("female-television-connector"),
                               I(o, 1)))) {
        goto failure;
      }
      if (!(R1E(ts, L(N("connected-to"), p, ObjWild, E)))) {
        if ((q = SpaceFindNearest(ts, NULL, N("TV-cable"), I(o, 1)))) {
          SG(cx, sg, 1, STFAILURE, L(N("connected-to"), p, q, E));
        } else if ((q = SpaceFindNearest(ts, NULL, N("antenna"), I(o, 1)))) {
          SG(cx, sg, 1, STFAILURE, L(N("connected-to"), p, q, E));
        } else goto failure;
      } else {
        TOSTATE(cx, sg, 2);
      }
      return;
    case 2:
      if (!(p = DbRetrievePart(ts, NULL, N("power-switch"), I(o, 1)))) {
        goto failure;
      }
      SG(cx, sg, 3, STFAILURE, L(N("switch-on"), p, E));
      return;
    case 3:
      if (WAIT_PTN(cx, sg, 1, STSUCCESS, L(N("TV-set-on"), I(o, 1), E))) return;
      WAIT_TS(cx, sg, ts, (Dur)10, STFAILURE);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_TVSetOn: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL TV-set-off */
void PA_TVSetOff(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_TVSetOff", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = DbRetrievePart(ts, NULL, N("power-switch"), I(o, 1)))) {
        goto failure;
      }
      SG(cx, sg, 1, STFAILURE, L(N("switch-off"), p, E));
      return;
    case 1:
      if (WAIT_PTN(cx, sg, 1, STSUCCESS, L(N("TV-set-off"), I(o, 1), E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)10, STFAILURE);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_TVSetOff: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL watch-TV */
void PA_WatchTV(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj	*tvsetclass, *p;
  Dur	d;
  Dbg(DBGPLAN, DBGOK, "PA_WatchTV", E);
  switch (sg->state) {
    case STBEGIN:
      ADDO(ts, N("viewer-of"), o, a, 0);
      /* todoDATABASE */
      if (N("country-USA") == SpaceCountryOfObject(ts, NULL, a)) {
        tvsetclass = N("NTSC-TV-set");
      } else {
        tvsetclass = N("PAL-SECAM-TV-set");
      }
      /* <tvsetclass> is required so <a> does not turn on an NTSC TV
       * in PAL-SECAM land.
       */
      if (!(p = FINDO(cx, ts, N("TV-set"), o, tvsetclass, a, NULL))) {
        goto failure;
      }
      /* todo: Chair should be near-visible TV. */
      if (!FINDO(cx, ts, N("chair"), o, N("chair"), p, NULL)) {
        goto failure;
      }
      SG(cx, sg, 1, STFAILURE, L(N("TV-set-on"), p, E));
      return;
    case 1:
      if (!(p = TVShowSelect(ts, a, NULL, NULL, N("TV-show"),
                             GETO(ts, N("TV-set"), o),
                             &sg->w.WaTV.tvshowend, &sg->w.WaTV.tvssp))) {
          goto failure;
      }
      ADDO(ts, N("TV-show"), o, p, 1);
      TOSTATE(cx, sg, 2);
      return;
    case 2:
      if (!(p = DbRetrievePart(ts, NULL, N("channel-selector"),
                               GETO(ts, N("TV-set"), o)))) {
        goto failure;
      }
      SG(cx, sg, 3, STFAILURE, L(sg->w.WaTV.tvssp, p, E));
      return;
    case 3:
      SG(cx, sg, 4, STFAILURE,
         L(N("sitting"), a, GETO(ts, N("chair"), o), E));
      /* todo: Get comfortable. */
      return;
    case 4:
      if (TsGT(ts, &sg->w.WaTV.tvshowend)) {
        TOSTATE(cx, sg, 5);
      } else {
        d = (Dur)60*5;
        AA(ts, d, L(N("attend"), a, GETO(ts, N("TV-show"), o), E));
        TsIncrement(ts, d);
      }
      return;
    case 5:
      if (!(p = TVShowSelect(ts, a, NULL, NULL, N("TV-show"),
                             GETO(ts, N("TV-set"), o),
                             &sg->w.WaTV.tvshowend, &sg->w.WaTV.tvssp))) {
        TOSTATE(cx, sg, 6);
      } else {
        ADDO(ts, N("TV-show"), o, p, 1);
        TOSTATE(cx, sg, 2);
      }
      return;
    case 6:
      SG(cx, sg, 999, STFAILURE,
         L(N("TV-set-off"), GETO(ts, N("TV-set"), o), E));
      return;
    case 999:
      AR(&sg->startts, ts, o);
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_WatchTV: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

Bool PAObj_HasAntenna(Ts *ts, Obj *obj)
{
  Obj	*connector, *antenna;
  
  if (!(connector = DbRetrievePart(ts, NULL, N("female-television-connector"),
                                   obj))) {
    Dbg(DBGPLAN, DBGDETAIL, "<%s> has no female-television-connector", M(obj));
    return(0);
  }
  if (PAObj_IsBroken(ts, connector)) return(0);
  if (!(antenna = R1EI(2, ts, L(N("connected-to"), connector, ObjWild, E)))) {
    Dbg(DBGPLAN, DBGDETAIL, "<%s> not connected to antenna", M(obj));
    return(0);
  }
  if (PAObj_IsBroken(ts, antenna)) return(0);
  return(1);
}

Bool PAObj_IsPluggedIn(Ts *ts, Obj *obj)
{
  Obj	*plug, *outlet;
  
  if (!(plug = DbRetrievePart(ts, NULL, N("AC-plug"), obj))) {
    Dbg(DBGPLAN, DBGDETAIL, "<%s> has no AC-plug", M(obj));
    return(0);
  }
  if (PAObj_IsBroken(ts, plug)) return(0);
  if (!(outlet = R1DI(2, ts, L(N("connected-to"), plug, N("AC-outlet"), E),
                      2))) {
    Dbg(DBGPLAN, DBGDETAIL, "<%s> not plugged in", M(obj));
    return(0);
  }
  if (PAObj_IsBroken(ts, outlet)) return(0);
  return(1);
}

void PAObj_TVSet(TsRange *tsrange, Obj *obj)
{
  Obj	*ps;
  Obj	*nextstate, *prevstate;
  Ts	*ts;
  ts = &tsrange->startts;
  if (PAObj_IsBroken(ts, obj)) nextstate = N("TV-set-off");
  else {
    ps = PAObj_GetPartState(ts, obj, N("power-switch"), N("on-or-off-state"));
    if (PAObj_IsPluggedIn(ts, obj) &&
        PAObj_HasAntenna(ts, obj) &&
        ps && ps == N("switch-on")) nextstate = N("TV-set-on");
    else nextstate = N("TV-set-off");
  }
  if (nextstate ==
      (prevstate = PAObj_GetState(ts, obj, N("on-or-off-state")))) {
    return;
  }
  Dbg(DBGPLAN, DBGDETAIL, "TV-set state change", E);
  if (prevstate) TE(ts, L(prevstate, obj, E));
  DbAssert(tsrange, L(nextstate, obj, E));
}

/* End of file. */
