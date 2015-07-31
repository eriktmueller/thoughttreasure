/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "pa.h"
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

/* SUBGOAL strip */
void PA_Strip(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_Strip", E);
  switch (sg->state) {
    case STBEGIN:
      TOSTATE(cx, sg, 1);
      return;
    case 1:
      if ((p = R1EI(2, ts, L(N("wearing-of"), a, ObjWild, E)))) {
        SG(cx, sg, 1, STFAILURE, L(N("dress-take-off"), a, p, E));
      } else {
        TOSTATE(cx, sg, STSUCCESS);
      }  
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Strip: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL dress */
void PA_Dress(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj	*wallet, *clothing, *pocket;
  Dbg(DBGPLAN, DBGOK, "PA_Dress", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(sg->w.Dr.objs = ClothingSelectOutfit(ts, a, I(o,2)))) goto failure;
      if (DbgOn(DBGPLAN, DBGDETAIL)) {
        Dbg(DBGPLAN, DBGDETAIL, "selected outfit:");
        ObjListPrettyPrint(Log, sg->w.Dr.objs);
      }
      sg->w.Dr.wobjs = sg->w.Dr.wnextobjs =
        REI(2, ts, L(N("wearing-of"), a, ObjWild, E));
      TOSTATE(cx, sg, 1);
      return;
    case 1: /* Take off all items not in selected outfit. */
      while (sg->w.Dr.wnextobjs &&
             ObjListIn(sg->w.Dr.wnextobjs->obj, sg->w.Dr.objs)) {
        sg->w.Dr.wnextobjs = sg->w.Dr.wnextobjs->next;
      }
      if (sg->w.Dr.wnextobjs == NULL) {
        ObjListFree((void *)sg->w.Dr.wobjs);
        sg->w.Dr.nextobjs = sg->w.Dr.objs;
        TOSTATE(cx, sg, 2);
      } else {
        SG(cx, sg, 1, STFAILURE,
           L(N("dress-take-off"), a, sg->w.Dr.wnextobjs->obj, E));
        sg->w.Dr.wnextobjs = sg->w.Dr.wnextobjs->next;
      }
      return;
    case 2: /* Wear all items in selected outfit. */
      if (sg->w.Dr.nextobjs == NULL) {
        ObjListFree((void *)sg->w.Dr.objs);
        TOSTATE(cx, sg, 3);
      } else {
        SG(cx, sg, 2, STFAILURE,
           L(N("wearing-of"), a, sg->w.Dr.nextobjs->obj, E));
        sg->w.Dr.nextobjs = sg->w.Dr.nextobjs->next;
      }
      return;
    case 3:
      if (!(wallet = SpaceFindNearestOwned(ts, NULL, N("wallet"), a, a))) {
        goto failure;
      }
      if (!(clothing = R1DI(2, ts, L(N("wearing-of"), a, N("pants"), E), 2))) {
        if (!(clothing =
              R1DI(2, ts, L(N("wearing-of"), a, N("shorts"), E), 2))) {
          Dbg(DBGPLAN, DBGBAD, "PA_Dress: not wearing pants");
          goto failure;
        }
      }
      if (!(pocket = DbRetrievePart(ts, NULL, N("pocket"), clothing))) {
        goto failure;
      }
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("inside"), wallet, pocket, E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Dress: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL wearing-of ?human ?clothing */
void PA_Wearing(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_Wearing", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("dress-put-on"), I(o,1),
                                         I(o,2), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Wearing: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL dress-put-on */
void PA_DressPutOn(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_DressPutOn", E);
  switch (sg->state) {
    case STBEGIN:
      TOSTATE(cx, sg, 1);
      return;
    case 1:
      if ((p = ClothingFindOver(ts, a, I(o,2)))) {
        SG(cx, sg, 1, STFAILURE, L(N("dress-take-off"), a, p, E));
      } else {
        TOSTATE(cx, sg, 2);
      }
      return;
    case 2:
      if (!(sg->w.TaOf.hand1 = PA_GetFreeHand(ts, a))) goto failure;
      SG(cx, sg, STSUCCESS, STFAILURE,
         L(N("pull-on"), sg->w.TaOf.hand1, I(o,1), I(o,2), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD,
                 "PA_DressPutOn: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL dress-take-off */
void PA_DressTakeOff(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_DressTakeOff", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(sg->w.TaOf.hand1 = PA_GetFreeHand(ts, a))) goto failure;
      SG(cx, sg, 1, STFAILURE,
         L(N("pull-off"), sg->w.TaOf.hand1, I(o,1), I(o,2), E));
      return;
    case 1:
      if (!(sg->w.TaOf.shelf = R1EI(2, ts, L(N("shelf"), o, ObjWild, E)))) {
        if (!(sg->w.TaOf.shelf =
              SpaceFindNearest(ts, NULL, N("clothing-shelf"), a))) {
        /* If no shelf, just drop clothing here.
         * todo: Put on bed, chair.
         */
          TOSTATE(cx, sg, 2);
          return;
        }
        AS(ts, 0, L(N("shelf"), o, sg->w.TaOf.shelf, E));
      }
      SG(cx, sg, 2, STFAILURE, L(N("near-reachable"), a, sg->w.TaOf.shelf, E));
      return;
    case 2:
      SG(cx, sg, 999, STFAILURE, L(N("release"), sg->w.TaOf.hand1, I(o,2), E));
      return;
    case 999:
      AR(&sg->startts, ts, o);
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD,
                 "PA_DressTakeOff: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

void PA_PlaceObjectAt(Ts *ts, Obj *obj, Obj *at)
{
  Obj                   *polity, *grid;
  GridCoord             row, col;
  if (SpaceLocateObject(ts, NULL, at, NULL, 0, &polity, &grid, &row, &col)) {
    AS(ts, 0, ObjAtGridCreate(obj, grid, row, col));
  } else {
    Dbg(DBGPLAN, DBGBAD, "PA_PlaceObjectAt: cannot locate <%s>", M(at));
  }
}

/* SUBGOAL pull-on ?grasper ?human ?clothing */
void PA_PullOn(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur d;
  Dbg(DBGPLAN, DBGOK, "PA_PullOn", E);
  switch (sg->state) {
    case STBEGIN:
      if (!DbIsPartOf(ts, NULL, I(o,1), a)) {
        Dbg(DBGPLAN, DBGDETAIL, "failure: grasper not part of actor", E);
        goto failure;
      }
      SG(cx, sg, 1, STFAILURE, L(N("holding"), I(o,1), I(o,3), E));
      return;
    case 1:
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      TsIncrement(ts, d);
      if (!TE(ts, L(N("holding"), I(o,1), I(o,3), E))) {
        Dbg(DBGPLAN, DBGBAD, "not already holding", E);
        goto failure;
      }
      if (!TE(ts, L(N("at-grid"), I(o,3), ObjWild, ObjWild, E))) {
        Dbg(DBGPLAN, DBGBAD, "not already at-grid", E);
        goto failure;
      }
      AS(ts, 0, L(N("wearing-of"), I(o,2), I(o,3), E));
      TOSTATE(cx, sg, STSUCCESS); return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_PullOn: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL pull-off ?grasper ?human ?clothing */
void PA_PullOff(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur			d;
  Dbg(DBGPLAN, DBGOK, "PA_PullOff", E);
  switch (sg->state) {
    case STBEGIN:
      if (!DbIsPartOf(ts, NULL, I(o,1), a)) {
        Dbg(DBGPLAN, DBGDETAIL, "failure: grasper not part of actor", E);
        goto failure;
      }
      SG(cx, sg, 1, STFAILURE, L(N("near-graspable"), I(o,1), I(o,3), E));
      return;
    case 1:
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      TsIncrement(ts, d);
      if (!TE(ts, L(N("wearing-of"), I(o,2), I(o,3), E))) {
        Dbg(DBGPLAN, DBGBAD, "not already wearing-of", E);
        goto failure;
      }
      AS(ts, 0, L(N("holding"), I(o,1), I(o,3), E));
      PA_PlaceObjectAt(ts, I(o,3), a);
      TOSTATE(cx, sg, STSUCCESS); return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_PullOff: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* End of file. */
