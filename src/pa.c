/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940419: begun: control structure, basic shower
 * 19940421: shower objects, filling in more shower rules, part-of routines
 * 19940422: Problems with part-of abstract/concreteness. Added timers.
 *           Mostly finished shower script.
 * 19940425: moving held objects
 * 19940510: more clothes planning
 * 19950329: redoing for Actor
 * 19950329: moved general code here
 * 19950401: cosmetic changes to Demon
 * 19951024: to new context-based scheme
 *
 * todo:
 * - Convert all planning agents to use ADDO/GETO instead of fields of Subgoal.
 * - Add more code to planning agents to note causes of success or failure.
 * - Continue coding the many possible planning agents.
 */

#include "tt.h"
#include "pa.h"
#include "paatrans.h"
#include "pacloth.h"
#include "paenter.h"
#include "pagrasp.h"
#include "pamtrans.h"
#include "paphone.h"
#include "paptrans.h"
#include "repbasic.h"
#include "repcxt.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "repsubgl.h"
#include "reptime.h"
#include "semdisc.h"
#include "uaanal.h"
#include "uafriend.h"
#include "uapaappt.h"
#include "uapagroc.h"
#include "uapashwr.h"
#include "uapaslp.h"
#include "utildbg.h"

Subgoal *PA_StartSubgoal(Context *cx, Ts *ts, Obj *actor, Subgoal *supergoal,
                         int onsuccess, int onfailure, Obj *subgoal)
{
  Actor	*ac;
  Subgoal	*sg;
  if (actor == NULL) {
    if (IsActor(I(subgoal, 1))) {
      actor = I(subgoal, 1);
    } else if (IsActor(I(subgoal, 2))) {
    /* cf [President-of country-FRA Chirac]
     * todo: This does not work in general, since it is assumed that the
     * actor is slot 1 in most other places in the code.
     */
      actor = I(subgoal, 2);
    } else if (supergoal && supergoal->ac) {
      actor = supergoal->ac->actor;
    } else {
      Dbg(DBGPLAN, DBGBAD, "PA_StartSubgoal actor not found");
      actor = N("human");
    }
  }
  if (supergoal && actor == supergoal->ac->actor) {
    ac = supergoal->ac;
  } else if (actor) {
    ac = ContextActorFind(cx, actor, 1);
  } else {
    Dbg(DBGPLAN, DBGBAD, "PA_StartSubgoal NULL actor");
    ac = NULL;
  }
  if (DbgOn(DBGPLAN, DBGDETAIL)) {
    fputs("****START NEW SUBGOAL ", Log);
    ObjPrint(Log, subgoal);
    fputc(NEWLINE, Log);
  }
  DbRestrictValidate(subgoal, 1);
  if (YES(ZRE(ts, subgoal))) {
    Dbg(DBGPLAN, DBGOK, "subgoal already achieved:", E);
    DbgOP(DBGPLAN, DBGOK, subgoal);

    /* BEGIN: This appears to be needed for "Chirac succeeded at being elected"
     * to generate emotional responses.
     */
    ac->subgoals = sg =
      SubgoalCreate(ac, actor, ts, supergoal, onsuccess, onfailure, subgoal,
                    ac->subgoals);
    TOSTATE(cx, sg, STSUCCESS);
    /* END */

    if (supergoal) {
      TOSTATE(cx, supergoal, onsuccess);
    }
    return(NULL);
  }
  ac->subgoals = sg =
    SubgoalCreate(ac, actor, ts, supergoal, onsuccess, onfailure, subgoal,
                  ac->subgoals);
  SubgoalStatusChange(sg, N("active-goal"), NULL);
  if (supergoal) {
    TOSTATE(cx, supergoal, STWAITING);
    /* The caller can reset this if desired. */
  }
  return(sg);
}

Bool PA_RunSubgoal(Context *cx, Subgoal *sg)
{
  Bool	pending;
  char	*h;
  Obj	*head;
  if (DbgOn(DBGPLAN, DBGDETAIL)) {
    fprintf(Log, "-----subgoal <%s> ", M(sg->ac->actor));
    ObjPrint(Log, sg->obj);
    fputc(SPACE, Log);
    SubgoalStatePrint(Log, sg->state);
    fputs(" was ", Log);
    SubgoalStatePrint(Log, sg->last_state);
    fputc(' ', Log);
    fputs(ContextModeString(cx->mode), Log);
    fputc(NEWLINE, Log);
  }
/* No longer necessary?
  if (sg->state == STWAITING && cx->mode == MODE_SPINNING) {
  Take PA out of WAIT if spinning. "Lin was sleeping."
    DemonStop(cx, sg);
  }
*/
/* todo: Not necessary? In any case, this messes up planning for inside,
   because it never does the release step.
  if (YES(ZRE(&sg->ts, sg->obj))) {
    Dbg(DBGPLAN, DBGOK, "subgoal seen to be achieved", E);
    TOSTATE(cx, sg, STSUCCESS);
    PlanStopSubgoalsOf(sg);
    return(1);
  }
*/
  if (DemonTsTest(cx, sg, &pending)) return(1);
  if (DemonWaitidleTest(cx, sg)) return(1);
  if (sg->state == STWAITING) {
    TsIncrement(&sg->ts, 1);
    return(pending);	/* Only times are considered to be activity. */
  }
  /* todo: Convert this to static array. */
  head = ObjIth(sg->obj, 0);
  h = ObjToName(head);
  /* ACTIONS */
  if (streq(h, "grasp"))
    PA_Grasp(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "release"))
    PA_Release(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "rub"))
    PA_Rub(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "pour-onto"))
    PA_PourOnto(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "grid-walk"))
    PA_GridWalk(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "grid-drive-car"))
    PA_GridDriveCar(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "warp"))
    PA_Warp(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "flip-to"))
    PA_FlipTo(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "move-to"))
    PA_MoveTo(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "action-open"))
    PA_ActionOpen(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "action-close"))
    PA_ActionClose(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "pull-off"))
    PA_PullOff(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "pull-on"))
    PA_PullOn(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "connect-to"))
    PA_ConnectTo(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "sit-on"))
    PA_SitOn(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "stand-on"))
    PA_StandOn(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "lie-on"))
    PA_LieOn(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "gesture-here"))
    PA_GestureHere(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "pick-up"))
    PA_PickUp(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "hang-up"))
    PA_HangUp(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  /* STATES */
  else if (streq(h, "near-reachable"))
    PA_NearReachable(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "near-graspable"))
    PA_NearGraspable(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "near-audible"))
    PA_NearAudible(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "shower-on"))
    PA_ShowerOn(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "shower-off"))
    PA_ShowerOff(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "TV-set-on"))
    PA_TVSetOn(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "TV-set-off"))
    PA_TVSetOff(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "motor-vehicle-on"))
    PA_MotorVehicleOn(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "motor-vehicle-off"))
    PA_MotorVehicleOff(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "off-hook"))
    PA_OffHook(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "on-hook"))
    PA_OnHook(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "open"))
    PA_Open(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "closed"))
    PA_Closed(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "holding"))
    PA_Holding(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "inside"))
    PA_Inside(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "wearing-of"))
    PA_Wearing(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "connected-to"))
    PA_ConnectedTo(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "sitting"))
    PA_Sitting(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "standing"))
    PA_Standing(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (streq(h, "lying"))
    PA_Lying(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("knob-position"), head))
    PA_KnobPosition(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("on-or-off-state"), head))
    PA_SwitchX(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  /* TOP-LEVEL GOALS */
  else if (ISA(N("s-entertainment"), head))
    PA_SEntertainment(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  /* SCRIPTS */
  else if (ISA(N("dial"), head))
    PA_Dial(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("drive"), head))
    PA_Drive(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("call"), head))
    PA_Call(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("online-chat"), head))
    PA_OnlineChat(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("stay"), head))
    PA_Stay(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("obtain-permission"), head))
    PA_ObtainPermission(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("handle-call"), head))
    PA_HandleCall(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("pack"), head))
    PA_Pack(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("unpack"), head))
    PA_Unpack(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("handle-proposal"), head))
    PA_HandleProposal(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("mtrans"), head))
    PA_Mtrans(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("hand-to"), head))
    PA_HandTo(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("receive-from"), head))
    PA_ReceiveFrom(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("attend-performance"), head))
    PA_AttendPerformance(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("buy-ticket"), head))
    PA_BuyTicket(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("pay-in-person"), head))
    PA_PayInPerson(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("pay-cash"), head))
    PA_PayCash(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("pay-by-card"), head))
    PA_PayByCard(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("pay-by-check"), head))
    PA_PayByCheck(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("collect-payment"), head))
    PA_CollectPayment(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("collect-cash"), head))
    PA_CollectCash(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("dress"), head))
    PA_Dress(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("work-box-office"), head))
    PA_WorkBoxOffice(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("take-shower"), head))
    PA_TakeShower(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("appointment"), head))
    PA_Appointment(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("watch-TV"), head))
    PA_WatchTV(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("dress-take-off"), head))
    PA_DressTakeOff(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("dress-put-on"), head))
    PA_DressPutOn(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("strip"), head))
    PA_Strip(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("sleep"), head))
    PA_Sleep(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("grocer"), head))
    PA_Grocer(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else if (ISA(N("maintain-friend-of"), head))
    PA_MaintainFriendOf(cx, sg, &sg->ts, sg->ac->actor, sg->obj);
  else {
    Dbg(DBGPLAN, DBGOK, "PA_RunSubgoal: no plan", E);
    TOSTATE(cx, sg, STFAILURENOPLAN);
  }
  return(1);
}

/* Demon */

void DemonPrint(FILE *stream, Demon *d)
{
  fputs("demon", Log);
  if (!TsIsNa(&d->ts)) {
    fputc(SPACE, stream);
    TsPrint(stream, &d->ts);
  }
  if (d->ptn) {
    fputc(SPACE, stream);
    ObjPrint(stream, d->ptn);
  }
  if (d->waitidle != DURNA) {
    fputc(SPACE, stream);
    fprintf(stream, "idle %ld secs", d->waitidle);
  }
  fputs("->", Log);
  SubgoalStatePrint(stream, d->tostate);
  fputc(NEWLINE, stream);
}

void DemonPrintAll(FILE *stream, Demon *d)
{
  for (; d; d = d->next) {
    DemonPrint(stream, d);
  }
}

void DemonSet(Context *cx, Subgoal *sg, Ts *ts, int tostate, Obj *ptn,
              Dur waitidle)
{
  Demon	*d;
  d = CREATE(Demon);
  d->ts = *ts;
  d->ptn = ptn;
  d->tostate = tostate;
  d->waitidle = waitidle;
  d->next = sg->demons;
  sg->demons = d;
  if (DbgOn(DBGPLAN, DBGDETAIL)) {
    fputs("SET ", Log);
    DemonPrint(Log, d);
  }
}

void DemonClearAll(Context *cx, Subgoal *sg)
{
  Demon	*d;
  Dbg(DBGPLAN, DBGDETAIL, "demons CLEARED");
  while (sg->demons) {
    d = sg->demons;
    sg->demons = d->next;
    MemFree(d, "Demon");
  }
}

int DemonTsTest(Context *cx, Subgoal *sg, /* RESULTS */ Bool *pending)
{
  Demon	*d;
  *pending = 0;
  for (d = sg->demons; d; d = d->next) {
    if (!TsIsNa(&d->ts)) {
      *pending = 1;
	/* Was inside the TsGE, but that was wrong? */
      if (TsGE(&sg->ts, &d->ts)) {
        if (DbgOn(DBGPLAN, DBGDETAIL)) {
          fputs("FIRES ts", Log);
          DemonPrint(Log, d);
        }
        TOSTATE(cx, sg, d->tostate);
        DemonClearAll(cx, sg);
        return(1);
      }
    }
  }
  return(0);
}

int DemonWaitidleTest(Context *cx, Subgoal *sg)
{
  Demon	*d;
  for (d = sg->demons; d; d = d->next) {
    if ((d->waitidle != DURNA) &&
        (DiscourseInputIdleTime(cx->dc) > d->waitidle)) {
      if (DbgOn(DBGPLAN, DBGDETAIL)) {
        fputs("FIRES waitidle", Log);
        DemonPrint(Log, d);
      }
      TOSTATE(cx, sg, d->tostate);
      DemonClearAll(cx, sg);
      return(1);
    }
  }
  return(0);
}

Bool DemonPtnTest(Context *cx, Subgoal *sg, Obj *assertion)
{
  Demon	*d;
  for (d = sg->demons; d; d = d->next) {
    if (d->ptn && ObjMatchList(d->ptn, assertion)) {
      if (DbgOn(DBGPLAN, DBGDETAIL)) {
        fputs("FIRES ptn", Log);
        DemonPrint(Log, d);
      }
      sg->trigger = assertion;
      TOSTATE(cx, sg, d->tostate);
      DemonClearAll(cx, sg);
      return(1);
    }
  }
  return(0);
}

Bool DemonStop(Context *cx, Subgoal *sg)
{
  Demon	*d;
  for (d = sg->demons; d; d = d->next) {
    if (DbgOn(DBGPLAN, DBGDETAIL)) {
      fputs("FIRES break ", Log);
      DemonPrint(Log, d);
    }
    TOSTATE(cx, sg, d->tostate);
    DemonClearAll(cx, sg);
    return(1);
  }
  return(0);
}

void DemonPtnTestAll(Context *cx, Obj *assertion)
{
  Actor	*ac;
  Subgoal	*sg;
  for (ac = cx->actors; ac; ac = ac->next) {
    for (sg = ac->subgoals; sg; sg = sg->next) {
      if (SubgoalStateIsStopped(sg->state)) continue;
      if (sg->demons) DemonPtnTest(cx, sg, assertion);
    }
  }
}

/*****************************************************************************
 COMMONLY USED PA FUNCTIONS
 *****************************************************************************/

Subgoal *TG(Context *cx, Ts *ts, Obj *actor, Obj *goal)
{
  return(PA_StartSubgoal(cx, ts, actor, NULL, STNA, STNA, goal));
}

Subgoal *SG(Context *cx, Subgoal *supergoal, int onsuccess, int onfailure,
            Obj *goal)
{
  return(PA_StartSubgoal(cx, &supergoal->ts, NULL, supergoal, onsuccess,
                         onfailure, goal));
}

void ADDO(Ts *ts, Obj *rel, Obj *scriptprop, Obj *concrete, int always_assert)
{
  if (always_assert || !R1EI(2, ts, L(rel, scriptprop, concrete, E)))
    AS(ts, 0, L(rel, scriptprop, concrete, E));
}

Obj *FINDP(Context *cx, Ts *ts, Obj *rel, Obj *scriptprop, Obj *abstract_part,
           Obj *whole)
{
  Obj	*concrete_part, *obj;
  if ((obj = R1EI(2, ts, L(rel, scriptprop, ObjWild, E)))) {
    return(obj);
  }
  if (abstract_part == N("hand")) {
    concrete_part = PA_GetFreeHand(ts, whole);
  } else {
    concrete_part = DbRetrievePart(ts, NULL, abstract_part, whole);
  }
  if (concrete_part) {
    ADDO(ts, rel, scriptprop, concrete_part, 0);
    return(concrete_part);
  }
  /* todo: Invoke analogy here because we must return something. */
  return(ObjCreateInstance(abstract_part, NULL));
}

/* Examples:
 * role      scriptprop        findnearest  nearx     owner
 * --------  ----------------- ------------ --------- -----
 * bed       [sleep Charlotte] bed          Charlotte Charlotte
 *
 * todo: Generalize owner-of to user-of so that one sleeps in hotel bed.
 */
Obj *FINDO(Context *cx, Ts *ts, Obj *rel, Obj *scriptprop, Obj *findnearest,
           Obj *nearx, Obj *owner)
{
  Obj	*obj;
  if ((obj = R1EI(2, ts, L(rel, scriptprop, ObjWild, E)))) {
    return(obj);
  }
  if (findnearest &&
      (obj = SpaceFindNearestOwned(ts, NULL, findnearest, nearx, owner))) {
    AS(ts, 0, L(rel, scriptprop, obj, E));
    return(obj);
  }
  if (owner && (obj = UA_Analogy_ObjectNearActor(cx, ts, scriptprop,
                                                 findnearest, owner))) {
    ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
    AS(ts, 0, L(rel, scriptprop, obj, E));
    return(obj);
  }
  return(NULL);
}

Obj *GETO(Ts *ts, Obj *rel, Obj *scriptprop)
{
  Obj	*obj;
  if (!(obj = R1EI(2, ts, L(rel, scriptprop, ObjWild, E)))) {
    Dbg(DBGPLAN, DBGBAD, "PlanObjGet");
    return(ObjNA);
  }
  return(obj);
}

Obj *GETOO(Ts *ts, Obj *rel, Obj *scriptprop)
{
  Obj	*obj;
  if (!(obj = R1EI(2, ts, L(rel, scriptprop, ObjWild, E))))
    return(ObjWild);
  return(obj);
}

void TOSTATE(Context *cx, Subgoal *sg, SubgoalState state)
{
  if (sg->state == state) return;
  if (state == STNA) return;
  if (DbgOn(DBGPLAN, DBGOK)) {
    SubgoalStatePrint(Log, sg->state);
    fputs("->", Log);
    SubgoalStatePrint(Log, state);
    fprintf(Log, " <%s> ", M(sg->ac->actor));
    ObjPrint(Log, sg->obj);
    fputc(NEWLINE, Log);
  }
  if (SubgoalStateIsStopped(sg->state)) {
    Dbg(DBGPLAN, DBGBAD, "attempt to revive stopped goal ignored");
    return;
  }
  sg->last_state = sg->state;
  sg->state = state;
  if (state == STSUCCESS) {
    SubgoalStatusChange(sg, N("succeeded-goal"), sg->success_causes);
    if (sg->supergoal) {
      if (TsLT(&sg->supergoal->ts, &sg->ts)) {
        Dbg(DBGPLAN, DBGDETAIL, "advancing supergoal ts", E);
        sg->supergoal->ts = sg->ts;
      }
      TOSTATE(cx, sg->supergoal, sg->onsuccess);
    }
  } else if (SubgoalStateIsFailure(state)) {
    SubgoalStatusChange(sg, N("failed-goal"), sg->failure_causes);
    if (sg->supergoal) {
      if (cx->mode == MODE_SPINNING && 
          sg->supergoal->spin_to_state != STNOSPIN) {
        Dbg(DBGPLAN, DBGDETAIL, "RELAXED SUBGOAL FAILURE (SPINNING)");
        TOSTATE(cx, sg->supergoal, sg->onsuccess);
      } else {
        TOSTATE(cx, sg->supergoal, sg->onfailure);
      }
    }
  }
  if (SubgoalStateIsOutcome(sg->last_state) &&
      (!SubgoalStateIsOutcome(sg->state))) {
  /* Reviving subgoal. */
    sg->success_causes = sg->failure_causes = NULL;
  }
}

/* Wait until <ts>+<dur>. */
void WAIT_TS(Context *cx, Subgoal *sg, Ts *ts, Dur dur, int tostate)
{
  Ts	tscopy;
  tscopy = *ts;
  TsIncrement(&tscopy, dur);
  DemonSet(cx, sg, &tscopy, tostate, NULL, DURNA);
  TOSTATE(cx, sg, STWAITING);
}

/* Wait until <ptn> is asserted. Returns 1 if <ptn> already true. */
Bool WAIT_PTN(Context *cx, Subgoal *sg, int immed_check, int tostate, Obj *ptn)
{
  Obj	*assertion;
  DbRestrictValidate(ptn, 1);
  if (immed_check && (assertion = ZR1E(&sg->ts, ptn))) {
    Dbg(DBGPLAN, DBGOK, "WAIT pattern already achieved");
    sg->trigger = assertion;
    TOSTATE(cx, sg, tostate);
    return(1);
  }
  DemonSet(cx, sg, &TsNA, tostate, ptn, DURNA);
  TOSTATE(cx, sg, STWAITING);
  return(0);
}

/* Wait until the Discourse input channel has been idle for <waitidle>
 * seconds.
 */
void WAIT_IDLE(Context *cx, Subgoal *sg, Ts *ts, Dur waitidle, int tostate)
{
  DemonSet(cx, sg, &TsNA, tostate, NULL, waitidle);
  TOSTATE(cx, sg, STWAITING);
}

/* PLANNING CONTROL STRUCTURE */

int PlanNoActivity;

void PA_MainLoop(Context *cx, int mode)
{
  Discourse	*dc;
  Dbg(DBGPLAN, DBGOK, "PA_MainLoop");

  dc = cx->dc;
  cx->mode = mode;
  ContextCurrentDc = dc;	/* todoTHREAD */
  dc->run_agencies = AGENCY_ALL;
  PlanNoActivity = 0;
  while (PA_Pass(cx)) {
    if (cx->mode == MODE_PERFORMANCE) {
      if (!DiscourseReadUnderstand(cx->dc)) {
        Dbg(DBGPLAN, DBGDETAIL, "PA_MainLoop exited because eof received");
        return;
      }
      cx = dc->cx_best;
      cx->mode = mode;
    }
  }
}

void Daydream()
{
  if (StdDiscourse->cx_best == NULL) {
    DiscourseInit(StdDiscourse, NULL, NULL, NULL, NULL, 0);
  }
  PA_MainLoop(StdDiscourse->cx_best, MODE_DAYDREAMING);
}

Bool PA_SubgoalIneligible(Context *cx, Subgoal *sg, Ts *now,
                          /* RESULTS */ int *depends_on_now)
{
  if (SubgoalStateIsStopped(sg->state)) {
    return(1);
  }
  if (cx->mode == MODE_PERFORMANCE && TsGT(&sg->ts, now)) {
    if (depends_on_now) *depends_on_now = 1;
    return(1);
  }
  if (cx->mode == MODE_SPINNING && sg->spin_to_state == STNOSPIN) {
    return(1);
  }
  return(0);
}

Bool PA_SpinIsFinished(Context *cx, Subgoal *sg)
{
  if (cx->mode == MODE_SPINNING &&
      sg->spin_to_state != STSPIN &&
      sg->state == sg->spin_to_state) {
    if (DbgOn(DBGPLAN, DBGDETAIL)) {
      StreamSep(Log);
      fprintf(Log, "FINISHED SPINNING\n");
      StreamSep(Log);
    }
    return(1);
  }
  return(0);
}

void PA_AdvanceStoryTime(Context *cx)
{
#ifdef notdef
  Ts story_time;
  ContextHighestTs(cx, &story_time);
  if (TsIsSpecific(&story_time)) {
    cx->story_time.startts = story_time;
    cx->story_time.stopts = story_time;
  }
#endif
}

Bool PA_Pass(Context *cx)
{
  int		depends_on_now;
  Bool		activity;
  Ts		lowestts, now;
  Actor	*ac;
  Subgoal	*sg;
  activity = 0;
  Dbg(DBGPLAN, DBGHYPER, "PA_Pass");
  TsSetNow(&now);
  TsSetNa(&lowestts);
  depends_on_now = 0;
  for (ac = cx->actors; ac; ac = ac->next) {
    if (!ISA(N("animal"), ac->actor)) continue;
    for (sg = ac->subgoals; sg; sg = sg->next) {
      if (PA_SubgoalIneligible(cx, sg, &now, &depends_on_now)) continue;
      if ((!TsIsSpecific(&lowestts)) || TsLT(&sg->ts, &lowestts)) {
        lowestts = sg->ts;
      }
    }
  }

  if (!TsIsSpecific(&lowestts)) {
    if (depends_on_now) {
      goto exit1;
    } else {
      goto exit0;
    }
  }
  if (DbgOn(DBGPLAN, DBGDETAIL)) {
    Dbg(DBGPLAN, DBGDETAIL, "lowestts:");
    TsPrint(Log, &lowestts);
    fputc(NEWLINE, Log);
  }

  if ((cx->mode != MODE_PERFORMANCE) && DbgOn(DBGPLAN, DBGHYPER)) {     
    ContextPrint(Log, cx);
  }
  cx->story_time.stopts = lowestts;
  for (ac = cx->actors; ac; ac = ac->next) {
    if (!ISA(N("animal"), ac->actor)) continue;
    for (sg = ac->subgoals; sg; sg = sg->next) {
      if (PA_SubgoalIneligible(cx, sg, &now, NULL)) continue;
      if (TsEQ(&sg->ts, &lowestts)) {
        if (PA_SpinIsFinished(cx, sg)) {
          goto exit0;
        }
        if (PA_RunSubgoal(cx, sg)) {
          activity = 1;
          sg->lastts = sg->ts;
        }
        if (PA_SpinIsFinished(cx, sg)) {
          goto exit0;
        }
      }
    }
  }
  if (activity) PlanNoActivity = 0;
  else PlanNoActivity++;
  if ((cx->mode != MODE_PERFORMANCE) && (PlanNoActivity > 50)) {
    Dbg(DBGPLAN, DBGDETAIL, "no activity for 50 ticks");
    goto exit0;
  }
exit1:
#ifdef MICRO
  PA_AdvanceStoryTime(cx);
#endif
  return(1);
exit0:
#ifdef MICRO
  PA_AdvanceStoryTime(cx);
#endif
  return(0);
}

void PA_SpinTo(Context *cx, Subgoal *sg, int spin_to_state)
{
  Dbg(DBGPLAN, DBGOK, "PA_SpinTo <%s> <%d>", M(I(sg->obj, 0)),
      spin_to_state);
  if (DbgOn(DBGPLAN, DBGDETAIL)) {
    StreamSep(Log);
    fprintf(Log, "START SPINNING\n");
    StreamSep(Log);
  }
  ContextClearSpin(cx);	/* REDUNDANT */
  sg->spin_to_state = spin_to_state;
  PA_MainLoop(cx, MODE_SPINNING);
  ContextClearSpin(cx);
}

/* Spin_emotion is an already known (e.g. input) emotion causing the spin. */
void PA_SpinToGoalStatus(Context *cx, Subgoal *sg, Obj *goal_status,
                         Obj *spin_emotion)
{
  if (sg->cur_goal && (I(sg->cur_goal, 0) == goal_status)) {
    Dbg(DBGPLAN, DBGOK, "PA_SpinToGoalStatus <%s> already in state <%s>",
        M(I(sg->obj, 0)), M(goal_status));
    return;
  }
  sg->spin_emotion = spin_emotion;
  if (ISA(N("active-goal"), goal_status)) {
    /* todo: Only if at terminal state? */
    PA_SpinTo(cx, sg, STBEGIN);
  } else if (ISA(N("succeeded-goal"), goal_status)) {
    PA_SpinTo(cx, sg, STSUCCESS);
  } else if (ISA(N("failed-goal"), goal_status)) {
    PA_SpinTo(cx, sg, STFAILURE);
  } else {
    Dbg(DBGPLAN, DBGBAD, "PA_SpinToGoalStatus 1");
    PA_SpinTo(cx, sg, STFAILURE);
  }
}

/* Assumes objects already instantiated: only accepts concrete matches. */
void PA_ObjStateChange(TsRange *tsrange, Obj *part, int depth)
{
  ObjList *objs, *p;
  Obj *whole;
  if (depth >= MAXPARTDEPTH) {
    Dbg(DBGPLAN, DBGBAD, "PA_ObjStateChange: MAXPARTDEPTH reached <%s>",
        M(part));
    return;
  }
  for (p = objs = RE(&tsrange->startts, L(N("cpart-of"), part, ObjWild, E));
       p; p = p->next) {
    whole = I(p->obj, 2);
    PA_ObjStateChange(tsrange, whole, depth+1);
    if (ISA(N("shower"), whole)) PAObj_Shower(tsrange, whole);
    else if (ISA(N("TV-set"), whole)) PAObj_TVSet(tsrange, whole);
    else if (ISA(N("motor-vehicle"), whole)) {
      PAObj_MotorVehicle(tsrange, whole);
    } else if (ISA(N("phone"), whole)) PAObj_Phone(tsrange, whole);
    /* todo: Add other objects here. */
  }
  ObjListFree(objs);
}

Bool PAObj_IsBroken(Ts *ts, Obj *obj)
{
/*
  if (YES(RE(ts, L(N("good-condition"), obj, ObjWild, E)))) return(1);
  ObjWild = negative value
*/
  return(0);
}

Obj *PAObj_GetState(Ts *ts, Obj *obj, Obj *state)
{
  return(R1DI(0, ts, L(state, obj, E), 0));
}

/* Assumes only one part of given type per object. Hence only one state
 * to return.
 */
Obj *PAObj_GetPartState(Ts *ts, Obj *obj, Obj *part, Obj *state)
{
  Obj *o;
  if (!(o = DbRetrievePart(ts, NULL, part, obj))) {
    Dbg(DBGPLAN, DBGBAD, "no %s", M(part), E);
    return(NULL);
  }
  if (PAObj_IsBroken(ts, o)) {
    Dbg(DBGPLAN, DBGBAD, "%s is broken", M(part), E);
    return(NULL);
  }
  return(PAObj_GetState(ts, o, state));
}

/* End of file. */
