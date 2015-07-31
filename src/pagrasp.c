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

/******************************************************************************
 * Utility functions
 ******************************************************************************/

ObjList *PA_Graspers(Ts *ts, Obj *obj)
{
  ObjList       *r;
  Obj           *o;
  r = NULL;
  if (ISA(N("animal"), obj)) {
    if ((o = DbRetrievePart(ts, NULL, N("right-hand"), obj))) {
      r = ObjListCreate(o, r);
    }
    if ((o = DbRetrievePart(ts, NULL, N("left-hand"), obj))) {
      r = ObjListCreate(o, r);
    }
  }
  return(r);
}

/* todo: Pick hand not already in use. */
Obj *PA_GetFreeHand(Ts *ts, Obj *human)
{
  Obj *o;
  if ((o = DbRetrievePart(ts, NULL, N("right-hand"), human))) return(o);
  if ((o = DbRetrievePart(ts, NULL, N("left-hand"), human))) return(o);
  Dbg(DBGPLAN, DBGDETAIL, "no hand", E);
  return(NULL);
}

/******************************************************************************
 * Object change of location management.
 *
 * SMALL CONTAINER STATES:
 * (S1) actor----grasper
 * (S2) small-container
 * (S3) noncontainer-objects
 * (S4) small-container--INSIDE---noncontainer-objects
 * (S5) actor----grasper--HOLD--small-container
 * (S6) actor----grasper--HOLD--noncontainer-objects
 * (S7) actor----grasper--HOLD--small-container--INSIDE--noncontainer-objects
 * (S8) actor----grasper--HOLD----+
 *                                +--noncontainer-objects
 *      small-container--INSIDE---+
 * It seems we can do without (S8).
 * LARGE CONTAINER STATES:
 * (L1) actor
 * (L2) large-container
 * (L3) large-container--CONTAIN--actor
 *
 * object          at-grid stored?
 * ______________  _______________
 * large container             yes
 * small container             yes
 * actor                       yes
 * held object                 yes
 * inside object               yes
 * grasper                      no
 * clothing           no when worn
 ******************************************************************************/

void PA_MoveObject(Ts *ts, Obj *obj, Obj *grid, GridCoord torow,
                   GridCoord tocol)
{
  if (grid) {
    TE(ts, L(N("at-grid"), obj, grid, ObjWild));
    AS(ts, 0, ObjAtGridCreate(obj, grid, torow, tocol));
  }
}

void PA_SmallContainedObjectMove(Ts *ts, Obj *small_contained, Obj *grid,
                                 GridCoord torow, GridCoord tocol)
{
  PA_MoveObject(ts, small_contained, grid, torow, tocol);
}

void PA_HeldObjectMove(Ts *ts, Obj *held, Obj *grid, GridCoord torow,
                       GridCoord tocol)
{
  ObjList	*objs, *p;
  /* (S8) -> (S2)+(S6): Held object which moves is no longer
   * inside anything. (Holding overrides inside.)
   * todo: Actor holds key in pocket while walking.
   */
  TE(ts, L(N("inside"), held, ObjWild, E));

  if (grid) { /* Optimization. */
    /* (S7): <held> is small container. */
    objs = RE(ts, L(N("inside"), ObjWild, held, E));
    for (p = objs; p; p = p->next) {
      PA_SmallContainedObjectMove(ts, I(p->obj, 1), grid, torow, tocol);
    }
    ObjListFree(objs);
  }

  PA_MoveObject(ts, held, grid, torow, tocol);
}

void PA_GrasperMove(Ts *ts, Obj *grasper, Obj *grid, GridCoord torow,
                    GridCoord tocol)
{
  ObjList	*objs, *p;
  /* Grasper which moves is no longer near-graspable anything. */
  TE(ts, L(N("near-graspable"), grasper, ObjWild, E));

  /* (S5), (S6), (S7), (S8) */
  objs = RE(ts, L(N("holding"), grasper, ObjWild, E));
  for (p = objs; p; p = p->next) {
    PA_HeldObjectMove(ts, I(p->obj, 2), grid, torow, tocol);
  }
  ObjListFree(objs);
}

void PA_ActorMove(Ts *ts, Obj *a, Obj *grid, GridCoord torow,
                  GridCoord tocol, int is_walk)
{
  ObjList	*objs, *p;
  if (is_walk) {
  /* (L3) -> (L1) */
    TE(ts, L(N("inside"), a, ObjWild, E));
  }

  objs = PA_Graspers(ts, a);
  for (p = objs; p; p = p->next) {
    PA_GrasperMove(ts, p->obj, grid, torow, tocol);
  }
  ObjListFree(objs);

  /* todo: Need to do parts of <a> and parts (such as pockets containing
   * keys) of clothing worn by <a>.
   */

  PA_MoveObject(ts, a, grid, torow, tocol);
}

void PA_LargeContainerMove(Ts *ts, Obj *large_container, Obj *grid,
                           GridCoord torow, GridCoord tocol)
{
  ObjList	*objs, *p;
  objs = RE(ts, L(N("inside"), ObjWild, large_container, E));
  for (p = objs; p; p = p->next) {
  /* (L3) */
    PA_ActorMove(ts, I(p->obj, 1), grid, torow, tocol, 0);
  }
  ObjListFree(objs);

  PA_MoveObject(ts, large_container, grid, torow, tocol);
}

/******************************************************************************
 * Grasper planning agents
 ******************************************************************************/

/* SUBGOAL grasp ?grasper ?object */
void PA_Grasp(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur d;
  Dbg(DBGPLAN, DBGOK, "PA_Grasp", E);
  switch (sg->state) {
    case STBEGIN:
      if (!DbIsPartOf(ts, NULL, I(o,1), a)) {
        Dbg(DBGPLAN, DBGDETAIL, "failure: grasper not part of actor", E);
        goto failure;
      }
      SG(cx, sg, 1, STFAILURE, L(N("near-graspable"), I(o,1), I(o,2), E));
      return;
    case 1:
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      /* (S5), (S6), (S7), (S8) */
      AS(ts, d, L(N("holding"), I(o,1), I(o,2), E));
      TsIncrement(ts, d);
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Grasp: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL release ?grasper ?object */
void PA_Release(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur d;
  Dbg(DBGPLAN, DBGOK, "PA_Release", E);
  switch (sg->state) {
    case STBEGIN:
      if (!DbIsPartOf(ts, NULL, I(o,1), a)) {
        Dbg(DBGPLAN, DBGDETAIL, "failure: grasper not part of actor", E);
        goto failure;
      }
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      TsIncrement(ts, d);
      if (!TE(ts, L(N("holding"), I(o,1), I(o,2), E))) {
        Dbg(DBGPLAN, DBGBAD, "not already holding object", E);
      }
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_Release: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL inside ?object ?container */
void PA_Inside(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_Inside", E);
  switch (sg->state) {
    case STBEGIN:
      if (ISA(N("animate-object"), I(o, 1)) &&
          ISA(N("large-container"), I(o, 2))) {
        TOSTATE(cx, sg, 200);
      } else {
        TOSTATE(cx, sg, 100);
      }
      return;
    case 100:
      SG(cx, sg, 101, STFAILURE, L(N("open"), I(o,2), E));
      return;
    case 101:
      if (!(sg->w.Inside.hand = PA_GetFreeHand(ts, a))) goto failure;
      /* Subgoal is (S6). */
      SG(cx, sg, 102, STFAILURE, L(N("holding"), sg->w.Inside.hand, I(o,1), E));
      return;
    case 102:
      SG(cx, sg, 103, STFAILURE, L(N("move-to"), sg->w.Inside.hand,
                                   sg->w.Inside.hand, I(o,2), E));
      return;
    case 103:
      SG(cx, sg, 104, STFAILURE, L(N("release"), sg->w.Inside.hand,
                                   I(o,1), E));
      return;
    case 104:
      AS(ts, 0, L(N("inside"), I(o,1), I(o,2), E));
      TOSTATE(cx, sg, STSUCCESS);
      return;
    case 200:
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("near-reachable"), I(o,1),
                                         I(o,2), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Inside: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL move-to na ?grasper ?object */
void PA_MoveTo(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj	*p;
  Dur	d;
  Dbg(DBGPLAN, DBGOK, "PA_MoveTo", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = DbRetrieveWhole(ts, NULL, N("human"), I(o, 2)))) goto failure;
      if (p != a) {
        Dbg(DBGPLAN, DBGDETAIL, "failure: grasper not part of actor", E);
        goto failure;
      }
      SG(cx, sg, 1, STFAILURE, L(N("near-reachable"), p, I(o,3), E));
      return;
    case 1:
      if ((p = R1E(ts, L(N("inside"), I(o,3), ObjWild, E))) &&
          YES(RE(ts, L(N("closed"), I(p, 2), E)))) {
        SG(cx, sg, 2, STFAILURE, L(N("open"), I(p, 2), E));
      } else {
        TOSTATE(cx, sg, 2);
      }
      return;
    case 2:
      PA_GrasperMove(ts, I(o, 2), NULL, 0, 0);

      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      TsIncrement(ts, d);
      AS(ts, 0, L(N("near-graspable"), I(o,2), I(o,3), E));
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_MoveTo: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL near-graspable ?grasper ?object */
void PA_NearGraspable(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_NearGraspable", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = DbRetrieveWhole(ts, NULL, N("human"), I(o, 1)))) goto failure;
      if (p != a) {
        Dbg(DBGPLAN, DBGDETAIL, "failure: grasper not part of actor", E);
        goto failure;
      }
      SG(cx, sg, 1, STFAILURE, L(N("near-reachable"), p, I(o,2), E));
      return;
    case 1:
      if (SpaceIsNearGraspable(ts, I(o,1), I(o,2))) {
        Dbg(DBGPLAN, DBGDETAIL, "already near-graspable", E);
        TOSTATE(cx, sg, STSUCCESS);
      } else {
        SG(cx, sg, STSUCCESS, STFAILURE, L(N("move-to"), I(o,1), I(o,1),
                                           I(o,2), E));
      }
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_NearGraspable: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL holding ?grasper ?object */
void PA_Holding(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_Holding", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("grasp"), I(o,1), I(o,2), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Holding: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL open ?object */
void PA_Open(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_Open", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = PA_GetFreeHand(ts, a))) goto failure;
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("action-open"), p, I(o,1), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Open: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL closed ?object */
void PA_Closed(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_Closed", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = PA_GetFreeHand(ts, a))) goto failure;
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("action-close"), p, I(o,1), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Closed: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL action-open ?grasper ?object */
void PA_ActionOpen(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur d;
  Dbg(DBGPLAN, DBGOK, "PA_ActionOpen", E);
  switch (sg->state) {
    case STBEGIN:
      if (!DbIsPartOf(ts, NULL, I(o,1), a)) {
        Dbg(DBGPLAN, DBGDETAIL, "failure: grasper not part of actor");
        goto failure;
      }
      /* todo: Cannot already be holding too much. */
      SG(cx, sg, 1, STFAILURE, L(N("near-graspable"), I(o,1), I(o,2), E));
      return;
    case 1:
      d = DurationOf(I(o, 0));	/* Depends on object. */
      AA(ts, d, o);
      TsIncrement(ts, d);
      if (!TE(ts, L(N("closed"), I(o,2), E))) {
        Dbg(DBGPLAN, DBGDETAIL, "object not already closed");
      }
      AS(ts, 0, L(N("open"), I(o,2), E));
      TOSTATE(cx, sg, STSUCCESS); return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_ActionOpen: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL action-close ?grasper ?object */
void PA_ActionClose(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur d;
  Dbg(DBGPLAN, DBGOK, "PA_ActionClose", E);
  switch (sg->state) {
    case STBEGIN:
      if (!DbIsPartOf(ts, NULL, I(o,1), a)) {
        Dbg(DBGPLAN, DBGDETAIL, "failure: grasper not part of actor", E);
        goto failure;
      }
      /* todo: Cannot already be holding too much. */
      SG(cx, sg, 1, STFAILURE, L(N("near-graspable"), I(o,1), I(o,2), E));
      return;
    case 1:
      d = DurationOf(I(o, 0));	/* Depends on object. */
      AA(ts, d, o);
      TsIncrement(ts, d);
      if (!TE(ts, L(N("open"), I(o,2), E))) {
        Dbg(DBGPLAN, DBGBAD, "object not already open", E);
      }
      AS(ts, 0, L(N("closed"), I(o,2), E));
      TOSTATE(cx, sg, STSUCCESS); return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_ActionClose: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL connected-to ?object1 ?object2 */
void PA_ConnectedTo(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_ConnectedTo", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = PA_GetFreeHand(ts, a))) goto failure;
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("connect-to"), p, I(o,1),
                                         I(o,2), E));
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_ConnectedTo: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL connect-to ?grasper ?object1 ?object2 */
void PA_ConnectTo(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur	d;
  Dbg(DBGPLAN, DBGOK, "PA_Connect", E);
  switch (sg->state) {
    case STBEGIN:
      if (!DbIsPartOf(ts, NULL, I(o,1), a)) {
        Dbg(DBGPLAN, DBGDETAIL, "failure: grasper not part of actor", E);
        goto failure;
      }
      SG(cx, sg, 1, STFAILURE, L(N("grasp"), I(o,1), I(o,2), E));
      return;
    case 1:
      SG(cx, sg, 2, STFAILURE, L(N("near-graspable"), I(o,1), I(o,3), E));
      return;
    case 2:
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      TsIncrement(ts, d);
      TOSTATE(cx, sg, 3);
      return;
    case 3: /* todo: Perhaps merge release into this type of connect? */
      SG(cx, sg, 4, STFAILURE, L(N("release"), I(o,1), I(o,2), E));
      return;
    case 4:
      AS(ts, 0, L(N("connected-to"), I(o,2), I(o,3), E));
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Connect: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/******************************************************************************
 * Misc grasper planning agents
 ******************************************************************************/

/* SUBGOAL rub ?grasper ?object ?seconds */
void PA_Rub(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur d;
  Dbg(DBGPLAN, DBGOK, "PA_Rub", E);
  switch (sg->state) {
    case STBEGIN:
      if (!DbIsPartOf(ts, NULL, I(o,1), a)) {
        Dbg(DBGPLAN, DBGDETAIL, "failure: grasper not part of actor", E);
        goto failure;
      }
      SG(cx, sg, 1, STFAILURE, L(N("near-graspable"), I(o,1), I(o,2), E));
      return;
    case 1:
      d = (Dur)ObjToNumber(I(o, 3));
      AA(ts, d, o);
      TsIncrement(ts, d);
      TOSTATE(cx, sg, STSUCCESS); return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Rub: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL pour-onto ?grasper ?holding ?onto */
void PA_PourOnto(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur d;
  Dbg(DBGPLAN, DBGOK, "PA_PourOnto", E);
  switch (sg->state) {
    case STBEGIN:
      if (!DbIsPartOf(ts, NULL, I(o,1), a)) {
        Dbg(DBGPLAN, DBGDETAIL, "failure: grasper not part of actor", E);
        goto failure;
      }
      SG(cx, sg, 2, STFAILURE, L(N("holding"), I(o,1), I(o,2), E));
      return;
    case 2:
      SG(cx, sg, 3, STFAILURE, L(N("near-graspable"), I(o,1), I(o,3), E));
      return;
    case 3:
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      TsIncrement(ts, d);
      TOSTATE(cx, sg, STSUCCESS); return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_PourOnto: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL knob-position ?knob */
void PA_KnobPosition(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_KnobPosition", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(p = PA_GetFreeHand(ts, a))) goto failure;
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("flip-to"), p, I(o,1),
                                         ObjNA, I(o,0), E));
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_KnobPosition: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL switch-on ?knob
 * SUBGOAL switch-off ?knob
 */
void PA_SwitchX(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_SwitchX", E);
  switch (sg->state) {
    case STBEGIN:
      if (!ISA(N("lock"), I(o,1))) {
        TOSTATE(cx, sg, 1);
        return;
      }
      if (!(p = R1EI(2, ts, L(N("key-of"), I(o,1), ObjWild, E)))) {
        Dbg(DBGPLAN, DBGBAD, "PA_SwitchX: no key found");
        goto failure;
      }
      SG(cx, sg, 1, STFAILURE, L(N("inside"), p, I(o,1), E));
      return;
    case 1:
      if (!(p = PA_GetFreeHand(ts, a))) goto failure;
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("flip-to"), p, I(o,1), ObjNA,
                                         I(o,0), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_SwitchX: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL flip-to ?grasper ?knob na ?knob-position */
void PA_FlipTo(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur d;
  Dbg(DBGPLAN, DBGOK, "PA_FlipTo", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, 1, STFAILURE, L(N("near-graspable"), I(o,1), I(o,2), E));
      return;
    case 1:
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      TsIncrement(ts, d);
      TD(ts, L(N("knob-position"), I(o,2), E), 0);
      AS(ts, 0, L(I(o,4), I(o,2), E));
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_FlipTo: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL gesture-here ?human ?human ?grasper ?object */
void PA_GestureHere(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur	d;
  Dbg(DBGPLAN, DBGOK, "PA_GestureHere", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, 1, STFAILURE, L(N("near-reachable"), I(o, 1), I(o, 2), E));
      return;
    case 1:
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      TsIncrement(ts, d);
      TOSTATE(cx, sg, STSUCCESS); return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_GestureHere: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL hand-to ?human ?human ?object */
void PA_HandTo(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_HandTo", E);
  switch (sg->state) {
    case STBEGIN:
      if (!(sg->w.HaTo.hand = PA_GetFreeHand(ts, a))) goto failure;
      SG(cx, sg, 1, STFAILURE, L(N("holding"), sg->w.HaTo.hand, I(o, 3), E));
      return;
    case 1:
      SG(cx, sg, 2, STFAILURE,
         L(N("gesture-here"), a, I(o, 2), sg->w.HaTo.hand, I(o, 3), E));
      return;
    case 2:
      if (WAIT_PTN(cx, sg, 1, 3, L(N("grasp"), ObjWild, I(o, 3), E))) return;
      WAIT_TS(cx, sg, ts, (Dur)10, 1);
      return;
    case 3:
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("release"), sg->w.HaTo.hand,
                                         I(o, 3), E));
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_HandTo: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL receive-from ?human ?human ?object */
void PA_ReceiveFrom(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Obj *p;
  Dbg(DBGPLAN, DBGOK, "PA_ReceiveFrom", E);
  switch (sg->state) {
    case STBEGIN:
      /* todo: Put out hand. */
      if (WAIT_PTN(cx, sg, 0, 2,
                   L(N("gesture-here"), I(o, 2), a, ObjWild, I(o, 3), E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)10, STFAILURE);
      return;
    case 2: /* todo: Empty hands. */
      if (!(p = PA_GetFreeHand(ts, a))) goto failure;
      SG(cx, sg, STSUCCESS, STFAILURE, L(N("grasp"), p, I(sg->trigger, 4), E));
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_ReceiveFrom: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/* End of file. */
