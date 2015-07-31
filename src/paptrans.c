/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "pa.h"
#include "pagrasp.h"
#include "paptrans.h"
#include "repbasic.h"
#include "repcloth.h"
#include "repdb.h"
#include "repgrid.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "reptime.h"
#include "reptrip.h"
#include "utildbg.h"

/* SUBGOAL near-reachable ?human ?object [?baggage]
 * todo: Baggage here is inelegant?
 */
void PA_NearReachable(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  int		i;
  GridCoord	torow, tocol;
  GridSubspace	*gp;
  Ts		arrive_before;
  Dbg(DBGPLAN, DBGOK, "PA_NearReachable", E);
  switch (sg->state) {
    case STBEGIN:
      arrive_before = *ts;
      TsIncrement(&arrive_before, 2L*SECONDSPERDAYL);
      if (!(sg->trip = TripFind(I(o,1), I(o,1), I(o,2), ts, &arrive_before))) {
        cx->rsn.sense *= 0.8; /* Lin walks to her bedroom. */
        goto failure;
      }
      sg->leg = sg->trip->legs; 
      TOSTATE(cx, sg, 1);
      return;
    case 1:
      if (sg->leg == NULL) {
        TOSTATE(cx, sg, 990);
        TripFree(sg->trip);
        return;
      }
      if (sg->leg->path) {
        if (sg->leg->action == N("grid-walk")) {
          GridSubspaceSimplify(sg->leg->path);
            /* To speed debugging: Move the actor immediately to the
             * destination instead of simulating each step along the
             * path.
             */
          sg->i = 0;
          TOSTATE(cx, sg, 100);
          return;
        }
        if (sg->leg->action == N("grid-drive-car")) {
          TOSTATE(cx, sg, 300);
          return;
        }
      }
      if (sg->leg->wormhole) {
        TOSTATE(cx, sg, 200);
        return;
      }
      Dbg(DBGPLAN, DBGBAD, "PA_NearReachable: unknown leg type");
      return;
    case 100:
      gp = sg->leg->path;
      if ((i = sg->i) >= gp->len-1) {
        sg->leg = sg->leg->next;
        TOSTATE(cx, sg, 1);
        return;
      }
      torow = gp->rows[i+1];
      tocol = gp->cols[i+1];
      if (GridObjAtRowCol(sg->leg->grid, torow, tocol)) { /* todo */
      /* Replan if obstacles placed in way while walking. */
        TOSTATE(cx, sg, STBEGIN); return;
      }
      SG(cx, sg, 100, STFAILURE, L(N("grid-walk"), a, sg->leg->grid,
                                      DI(gp->rows[i]), DI(gp->cols[i]), 
                                      DI(torow), DI(tocol), E));
      sg->i = i+1;
      return;
    case 200:
      SG(cx, sg, 1, STFAILURE, L(N("warp"), a, sg->leg->from, sg->leg->to,
                                 sg->leg->wormhole, E));
      sg->leg = sg->leg->next;
      return;
    case 300:
      SG(cx, sg, 1, STFAILURE,
         L(N("drive"), sg->leg->driver, a, sg->leg->transporter,
           TripLegToObj(sg->leg), I(o, 3), E));
      sg->leg = TripLegNextSegment(sg->leg);
      return;
    case 990:
      if (ISA(N("large-container"), I(o,2))) {
      /* -> (L3)
       * todo: Opening door. The lack of opening door here is analogous to not
       * opening doors in buildings. Should eventually be added.
       */
        AS(ts, 0, L(N("inside"), I(o,1), I(o,2), E));
      }
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_NearReachable: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL grid-walk ?actor ?grid ?fromrow ?fromcol ?torow ?tocol */
void PA_GridWalk(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur d;
  Dbg(DBGPLAN, DBGOK, "PA_GridWalk", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, 1, STFAILURE, L(N("standing"), a, ObjNA, E));
      return;
    case 1:
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      TsIncrement(ts, d);
      PA_ActorMove(ts, I(o,1), I(o,2),
                   (GridCoord)ObjToNumber(I(o,5)),
                   (GridCoord)ObjToNumber(I(o,6)), 1);
      TOSTATE(cx, sg, STSUCCESS); return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_GridWalk: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL sitting ?animal ?object */
void PA_Sitting(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_Sitting", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("sit-on"), I(o,1), I(o,2), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Sitting: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL standing ?animal ?object */
void PA_Standing(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_Standing", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("stand-on"), I(o,1), I(o,2), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Standing: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL lying ?animal ?object */
void PA_Lying(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_Lying", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("lie-on"), I(o,1), I(o,2), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Lying: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL sit-on ?animal ?object */
void PA_SitOn(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur d;
  Dbg(DBGPLAN, DBGOK, "PA_SitOn", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, 1, STFAILURE, L(N("near-reachable"), I(o,1), I(o,2), E));
      return;
    case 1:
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      TsIncrement(ts, d);
      TD(ts, L(N("body-position"), I(o,1), ObjWild, E), 0);
      AS(ts, 0, L(N("sitting"), I(o,1), I(o,2), E));
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_SitOn: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL stand-on ?animal ?object */
void PA_StandOn(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur d;
  Dbg(DBGPLAN, DBGOK, "PA_StandOn", E);
  switch (sg->state) {
    case STBEGIN:
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      TsIncrement(ts, d);
      TD(ts, L(N("body-position"), I(o,1), ObjWild, E), 0);
      AS(ts, 0, L(N("standing"), I(o,1), I(o,2), E));
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_StandOn: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL lie-on ?animal ?object */
void PA_LieOn(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur d;
  Dbg(DBGPLAN, DBGOK, "PA_LieOn", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, 1, STFAILURE, L(N("near-reachable"), I(o,1), I(o,2), E));
      return;
    case 1:
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      TsIncrement(ts, d);
      TD(ts, L(N("body-position"), I(o,1), ObjWild, E), 0);
      AS(ts, 0, L(N("lying"), I(o,1), I(o,2), E));
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_LieOn: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL warp ?actor ?fromgrid ?togrid ?wormhole */
void PA_Warp(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj	*togridreg;
  Dur	d;
  Dbg(DBGPLAN, DBGOK, "PA_Warp", E);
  switch (sg->state) {
    case STBEGIN:
    /* todo: This does not work for cars. This is OK to leave out since
     * warp is only called by supergoals that know what they are doing.
      SG(cx, sg, XXX, STFAILURE, L(N("near-reachable"), I(o,1), I(o,4), E));
     */
      if (!(togridreg = R1EI(3, ts, L(N("at-grid"), I(o,4), I(o,3),
                                      ObjWild, E)))) {
        Dbg(DBGPLAN, DBGDETAIL, "<%s> does not lead to <%s>",
            M(I(o,4)), M(I(o,3)), E);
        goto failure;
      }
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      TsIncrement(ts, d);
      TE(ts, L(N("at-grid"), I(o,1), ObjWild, ObjWild, E));
      AS(ts, (Dur)0, L(N("at-grid"), I(o,1), I(o,3), togridreg, E));
        /* todo: Should make copy of gridreg here? No reference counts? */
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Warp: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL drive ?driver ?passenger ?motor-vehicle ?legs [?baggage] */
void PA_Drive(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  int		i;
  GridCoord	torow, tocol;
  GridSubspace	*gp;
  Dbg(DBGPLAN, DBGOK, "PA_Drive", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(sg->leg = ObjToTripLeg(I(o, 4)))) {
        Dbg(DBGPLAN, DBGBAD, "PA_Drive: trip legs not provided");
        goto failure;
      }
      if (I(o, 5)) {
        SG(cx, sg, 11, STFAILURE, L(N("inside"), I(o, 5), I(o, 3), E));
      } else TOSTATE(cx, sg, 11);
      return;
    case 11:
      SG(cx, sg, 12, STFAILURE, L(N("inside"), I(o, 2), I(o, 3), E));
      return;
    case 12:
      SG(cx, sg, 2, STFAILURE, L(N("inside"), a, I(o, 3), E));
      return;
    case 2:
      SG(cx, sg, 3, STFAILURE, L(N("motor-vehicle-on"), I(o, 3), E));
      return;
    case 3: /* Execute next leg. */
      if (sg->leg->wormhole) {
        TOSTATE(cx, sg, 200);
        return;
      }
      if (!sg->leg->path) {
        Dbg(DBGPLAN, DBGBAD, "PA_Drive 1");
        return;
      }
      if (sg->leg->action != N("grid-drive-car")) {
        Dbg(DBGPLAN, DBGBAD, "PA_Drive 2");
        return;
      }
      GridSubspaceSimplify(sg->leg->path); /* to speed debugging */
      sg->i = 0;
      TOSTATE(cx, sg, 100);
      return;
    case 4:	/* Get next leg. */
      sg->leg = sg->leg->next;
      if (sg->leg->start) TOSTATE(cx, sg, 5);
      else TOSTATE(cx, sg, 3);
      return;
    case 5:
      SG(cx, sg, 999, STFAILURE, L(N("motor-vehicle-off"), I(o, 3), E));
      return;
    /* todo: Outside car. */
    case 100:
      gp = sg->leg->path;
      if ((i = sg->i) >= gp->len-1) {
        TOSTATE(cx, sg, 4);
        return;
      }
      torow = gp->rows[i+1];
      tocol = gp->cols[i+1];
      if (GridObjAtRowCol(sg->leg->grid, torow, tocol)) { /* todo */
      /* Replan if obstacles placed in way while walking. */
        TOSTATE(cx, sg, STBEGIN); return;
      }
      SG(cx, sg, 100, STFAILURE,
         L(N("grid-drive-car"), a, I(o, 3), sg->leg->grid,
           DI(gp->rows[i]), DI(gp->cols[i]), 
           DI(torow), DI(tocol), E));
      sg->i = i+1;
      return;
    case 200:
      SG(cx, sg, 4, STFAILURE, L(N("warp"), I(o, 3), sg->leg->from, sg->leg->to,
                                 sg->leg->wormhole, E));
      return;
    case 999:
      AR(&sg->startts, ts, o);
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Drive: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL grid-drive-car ?actor ?motor-vehicle ?grid ?fromx ?fromy ?tox ?toy */
void PA_GridDriveCar(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur d;
  Dbg(DBGPLAN, DBGOK, "PA_GridDriveCar", E);
  switch (sg->state) {
    case STBEGIN:
      TOSTATE(cx, sg, 1); /* todo: Various preconditions? */
      return;
    case 1:
      d = DurationOf(I(o, 0));	/* todo: Correct duration? */
      AA(ts, d, o);
      TsIncrement(ts, d);
      PA_LargeContainerMove(ts, I(o, 2), I(o, 3),
                            (GridCoord)ObjToNumber(I(o, 6)),
                            (GridCoord)ObjToNumber(I(o, 7)));
      TOSTATE(cx, sg, STSUCCESS); return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_GridDriveCar: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL motor-vehicle-on ?motor-vehicle */
/* todo: Gas. */
void PA_MotorVehicleOn(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_MotorVehicleOn", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = DbRetrievePart(ts, NULL, N("ignition-switch"), I(o,1))))
        goto failure;
      SG(cx, sg, 1, STFAILURE, L(N("switch-on"), p, E));
      return;
    case 1:
      if (WAIT_PTN(cx, sg, 1, STSUCCESS,
                   L(N("motor-vehicle-on"), I(o, 1), E))) {
        return;
      }
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_MotorVehicleOn: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL motor-vehicle-off ?motor-vehicle */
void PA_MotorVehicleOff(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_MotorVehicleOff", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = DbRetrievePart(ts, NULL, N("ignition-switch"), I(o,1))))
        goto failure;
      SG(cx, sg, 1, STFAILURE, L(N("switch-off"), p, E));
      return;
    case 1:
      if (WAIT_PTN(cx, sg, 1, STSUCCESS,
                   L(N("motor-vehicle-off"), I(o, 1), E))) {
        return;
      }
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_MotorVehicleOff: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* todo: Gas. */
void PAObj_MotorVehicle(TsRange *tsrange, Obj *obj)
{
  Obj   *ps;
  Obj   *nextstate, *prevstate;
  Ts    *ts;
  ts = &tsrange->startts;
  if (PAObj_IsBroken(ts, obj)) nextstate = N("motor-vehicle-off");
  else {
    ps = PAObj_GetPartState(ts, obj, N("ignition-switch"),
                            N("on-or-off-state"));
    if (ps == N("switch-on")) nextstate = N("motor-vehicle-on");
    else nextstate = N("motor-vehicle-off");
  }
  if (nextstate ==
      (prevstate = PAObj_GetState(ts, obj, N("on-or-off-state")))) {
    return;
  }   
  Dbg(DBGPLAN, DBGDETAIL, "motor-vehicle state change", E);
  if (prevstate) TE(ts, L(prevstate, obj, E));
  DbAssert(tsrange, L(nextstate, obj, E));
}

/* SUBGOAL near-audible ?human ?object */
void PA_NearAudible(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_NearAudible", E);
  switch (sg->state) {
    case STBEGIN:
      if (SpaceIsNearAudible(ts, NULL, I(o,1), I(o,2))) {
        Dbg(DBGPLAN, DBGDETAIL, "already near-audible", E);
        TOSTATE(cx, sg, 101);
        return;
      }
      if (SpaceDistance(ts, NULL, I(o,1), I(o,2)) < 500.0) {
        TOSTATE(cx, sg, 100);
        return;
      }
      /* todo: Use some kind of guide to choose call or near-reachable as
       * subgoal.
       */
      SG(cx, sg, STNA, 100, L(N("call"), I(o,1), I(o,2), E));
      if (WAIT_PTN(cx, sg, 0, STSUCCESS,
                   L(N("near-audible"), I(o,1), I(o,2), E))) {
        return;
      }
      /* todo: Note this is allowing success of this subgoal before its
       * subgoals are completed. Will this work? We want the call subgoal
       * to continue so it can handle the hanging up of the phone. 
       */
      return;
    case 100:
      SG(cx, sg, 101, STFAILURE, L(N("near-reachable"), I(o,1), I(o,2), E));
      return;
    case 101:
      if ((!ObjIsVar(I(o, 1))) && (!ObjIsVar(I(o, 2)))) {
        AS(ts, 0, L(N("near-audible"), I(o,1), I(o,2), E));
      }
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_NearAudible: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL stay ?human ?human ?object ?unit-of-time */
void PA_Stay(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj	*p;
  Dbg(DBGPLAN, DBGOK, "PA_Stay", E);
  switch (sg->state) {
    case STBEGIN:
      ADDO(ts, N("traveler"), o, a, 0);
      ADDO(ts, N("destination-of"), o, I(o,2), 0);
      if (!(p = FINDO(cx, ts, N("baggage"), o, N("suitcase"), a, a))) {
        goto failure;
      }
      if (!(p = GuardianOf(ts, NULL, a))) {
        TOSTATE(cx, sg, 1);
      } else {
        SG(cx, sg, 1, STFAILURE, L(N("obtain-permission"), a, p, o, E));
      }
      return;
    case 1:
      SG(cx, sg, 2, STFAILURE,
         L(N("pack"), a, GETO(ts, N("baggage"), o), o, E));
      return;
    case 2:
      SG(cx, sg, 3, STFAILURE, L(N("near-reachable"), a, I(o, 3), E));
      return;
    case 3:
      SG(cx, sg, 4, STFAILURE,
         L(N("unpack"), a, GETO(ts, N("baggage"), o), E));
      return;
    case 4:
      /* todo: Do stuff and wait until you have to go back. */
      SG(cx, sg, 5, STFAILURE, L(N("pack"), a, GETO(ts, N("baggage"), o),
                                 N("owner-of"), E));
      return;
    case 5:
      if (!(p = R1EI(2, ts, L(N("residence-of"), a, ObjWild, E)))) {
        Dbg(DBGPLAN, DBGBAD, "PA_Stay: no residence found");
        goto failure;
      }
      SG(cx, sg, 6, STFAILURE, L(N("near-reachable"), a, p, E));
      return;
    case 6:
      SG(cx, sg, 999, STFAILURE,
         L(N("unpack"), a, GETO(ts, N("baggage"), o), E));
      return;
    case 999:
      AR(&sg->startts, ts, o);
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Stay: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL pack ?human ?baggage ?concept */
void PA_Pack(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_Pack", E);
  switch (sg->state) {
    case STBEGIN:
      /* todo: Take multiple outfits: one for every day. */
      if (I(o, 3) == N("owner-of")) {
        /* todo: Instead of within radius, should be in same grid, or better yet
         * in same apartment/house as actor.
         */
        sg->w.Dr.objs =
          SpaceFindWithinRadiusOwned(ts, NULL, N("concept"), a, 200.0, a);
      } else {
        if (!(sg->w.Dr.objs =
              ClothingSelectOutfit(ts, a, I(o, 3)))) {
          goto failure;
        }
      }
      if (DbgOn(DBGPLAN, DBGDETAIL)) {
        Dbg(DBGPLAN, DBGDETAIL, "selected outfit:");
        ObjListPrettyPrint(Log, sg->w.Dr.objs);
      }
      sg->w.Dr.nextobjs = sg->w.Dr.objs;
      TOSTATE(cx, sg, 1);
      return;
    case 1:
      if (sg->w.Dr.nextobjs == NULL) {
        ObjListFree((void *)sg->w.Dr.objs);
        TOSTATE(cx, sg, 999);
      } else {
        SG(cx, sg, 1, STFAILURE, L(N("inside"), sg->w.Dr.nextobjs->obj,
                                      I(o, 2), E));
        sg->w.Dr.nextobjs = sg->w.Dr.nextobjs->next;
      }
      return;
    case 999:
      AR(&sg->startts, ts, o);
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Pack: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL unpack ?human ?baggage */
void PA_Unpack(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj	*p;
  Dbg(DBGPLAN, DBGOK, "PA_Unpack", E);
  switch (sg->state) {
    case STBEGIN:
      /* todo: Shelf should be inside host's house. */
      if (!(p = FINDO(cx, ts, N("shelf"), o, N("clothing-shelf"),
                      a, NULL))) {
        goto failure;
      }
      if (!(sg->hand1 = PA_GetFreeHand(ts, a))) goto failure;
      SG(cx, sg, 1, STFAILURE, L(N("holding"), sg->hand1, I(o, 2), E));
      return;
    case 1:
      SG(cx, sg, 2, STFAILURE,
         L(N("near-reachable"), a, GETO(ts, N("shelf"), o), E));
      return;
    case 2:
      SG(cx, sg, 3, STFAILURE, L(N("release"), sg->hand1, I(o, 2), E));
      return;
    case 3:
      if (!(sg->obj1 = R1EI(1, ts, L(N("inside"), ObjWild, I(o, 2), E))))  {
        TOSTATE(cx, sg, 999);
        return;
      }
      SG(cx, sg, 4, STFAILURE, L(N("holding"), sg->hand1, sg->obj1, E));
      return;
    case 4:
      SG(cx, sg, 5, STFAILURE,
         L(N("move-to"), sg->hand1, sg->hand1, GETO(ts, N("shelf"), o), E));
      return;
    case 5:
      SG(cx, sg, 3, STFAILURE, L(N("release"), sg->hand1, sg->obj1, E));
      return;
    case 999:
      AR(&sg->startts, ts, o);
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Unpack: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* End of file. */
