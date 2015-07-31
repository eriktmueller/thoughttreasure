/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "repsubgl.h"
#include "pa.h"
#include "repbasic.h"
#include "repcxt.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "reptime.h"
#include "uaemot.h"
#include "utildbg.h"

Subgoal *SubgoalCreate(Actor *ac, Obj *actor, Ts *ts, Subgoal *supergoal,
                       int onsuccess, int onfailure, Obj *subgoal,
                       Subgoal *next)
{
  Subgoal	*sg;

  sg = CREATE(Subgoal);

  sg->state = STBEGIN;
  sg->supergoal = supergoal;
  sg->ac = ac;
  sg->obj = subgoal;
  sg->cur_goal = NULL;
  sg->startts = *ts;
  sg->lastts = *ts;
  sg->ts = *ts;
  sg->onsuccess = onsuccess;
  sg->onfailure = onfailure;
  sg->next = next;

  sg->child_copy = NULL;

  sg->last_state = STBEGIN;

  sg->spin_emotion = NULL;
  if (supergoal) {
    if (supergoal->spin_to_state != STNOSPIN) sg->spin_to_state = STSPIN;
    else sg->spin_to_state = STNOSPIN;
  } else {
    sg->spin_to_state = STNOSPIN;
  }

  sg->success_causes = sg->failure_causes = NULL;
  sg->demons = NULL;
  sg->trigger = NULL;

  sg->hand1 = sg->hand2 = NULL;
  sg->obj1 = NULL;
  sg->trip = NULL;
  sg->leg = NULL;
  sg->i = 0;
  sg->last_sg = NULL;
  if (supergoal) supergoal->last_sg = subgoal;
  sg->retries = 0;
  sg->return_to_state = STFAILURE;

  return(sg);
}

Subgoal *SubgoalCopy(Subgoal *sg_parent, Actor *ac_child, Context *cx_parent,
                     Context *cx_child, Subgoal *next)
{
  Subgoal	*sg_child;

  sg_child = CREATE(Subgoal);

  sg_child->state = sg_parent->state;
  sg_child->supergoal = sg_parent->supergoal;
    /* sg_child->supergoal is later fixed by SubgoalCopyAll to point to the
     * corresponding subgoal in the child context.
     */
  sg_child->ac = ac_child;
  sg_child->obj = sg_parent->obj;
#ifdef notdef
  sg_child->cur_goal =
    ContextMapAssertion(cx_parent, cx_child, sg_parent->cur_goal);
#endif
  sg_child->cur_goal = sg_parent->cur_goal;
  sg_child->startts = sg_parent->startts;
  sg_child->startts.cx = cx_child;
  sg_child->lastts = sg_parent->lastts;
  sg_child->lastts.cx = cx_child;
  sg_child->ts = sg_parent->ts;
  sg_child->ts.cx = cx_child;
  sg_child->onsuccess = sg_parent->onsuccess;
  sg_child->onfailure = sg_parent->onfailure;
  sg_child->next = next;

  sg_child->child_copy = NULL;

  sg_child->last_state = sg_parent->last_state;

  /* Temporaries -- no need to set or copy (?). */
  sg_child->spin_emotion = NULL;
  sg_child->spin_to_state = STNOSPIN;
  sg_child->success_causes = NULL;
  sg_child->failure_causes = NULL;
  sg_child->demons = NULL;
  sg_child->trigger = NULL;

  /* Very temporary. */
  sg_child->hand1 = sg_child->hand2 = NULL;
  sg_child->obj1 = NULL;
  sg_child->trip = NULL;
  sg_child->leg = NULL;
  sg_child->i = 0;
  sg_child->last_sg = NULL;
  sg_child->retries = 0;
  sg_child->return_to_state = STFAILURE;

  return(sg_child);
}

Subgoal *SubgoalMapSubgoal(Subgoal *sg_parents, Subgoal *sg_parent)
{
  Subgoal	*sg;
  if (sg_parent == NULL) return(NULL);
  for (sg = sg_parents; sg; sg = sg->next) {
    if (sg_parent == sg) return(sg_parent->child_copy);
  }
  Dbg(DBGGEN, DBGBAD, "SubgoalMapSubgoal failure");
  Stop();
  return(NULL);
}

Subgoal *SubgoalCopyAll(Subgoal *sg_parents, Actor *ac_child,
                        Context *cx_parent, Context *cx_child)
{
  Subgoal	*sg_parent, *sg_children, *sg_child;
  sg_children = NULL;
  for (sg_parent = sg_parents; sg_parent; sg_parent = sg_parent->next) {
    sg_children = SubgoalCopy(sg_parent, ac_child, cx_parent, cx_child,
                              sg_children);
    sg_parent->child_copy = sg_children;
  }
  for (sg_child = sg_children; sg_child; sg_child = sg_child->next) {
    sg_child->supergoal = SubgoalMapSubgoal(sg_parents, sg_child->supergoal);
  }
  return(sg_children);
}

Obj *SubgoalStatus(Subgoal *sg)
{
  switch (sg->state) {
    case STSUCCESS: return(N("succeeded-goal"));
    case STFAILURE: return(N("failed-goal"));
    case STFAILURENOPLAN: return(N("failed-goal"));
    default:
      break;
  }
  return(N("active-goal"));
}

void SubgoalStatePrint(FILE *stream, SubgoalState state)
{
  switch (state) {
    case STNA:			fputs("NA", stream); break;
    case STBEGIN:		fputs("BEGIN", stream); break;
    case STWAITING:		fputs("WAITING", stream); break;
    case STFAILURE:		fputs("FAILURE", stream); break;
    case STFAILURENOPLAN:	fputs("FAILURENOPLAN", stream); break;
    case STSUCCESS:		fputs("SUCCESS", stream); break;
    case STPOP:			fputs("STPOP", stream); break;
    case STSPIN:		fputs("STSPIN", stream); break;
    case STNOSPIN:		fputs("STNOSPIN", stream); break;
    default:			fprintf(stream, "%d", state);
  }
}

void SubgoalPrint(FILE *stream, Subgoal *sg, int detail)
{
  SubgoalStatePrint(stream, sg->state);
  if (sg->spin_to_state != STNOSPIN) {
    fputs("--SPINTO-->", stream);
    SubgoalStatePrint(stream, sg->spin_to_state);
  }
  fputc(SPACE, stream);
  ObjPrint(stream, sg->cur_goal);
  fputc(SPACE, stream);
  TsPrint(stream, &sg->ts);
  if (detail) {
    if (sg->spin_emotion) ObjPrint(stream, sg->spin_emotion);
    if (sg->success_causes) {
      fputs("success_causes:\n", stream);
      ObjListPrint(stream, sg->success_causes);
    }
    if (sg->failure_causes) {
      fputs("failure_causes:\n", stream);
      ObjListPrint(stream, sg->failure_causes);
    }
    TsPrint(stream, &sg->startts);
    if (sg->supergoal) {
      fputs("supergoal:\n", stream);
      ObjPrint(stream, sg->supergoal->cur_goal);
      fputc(SPACE, stream);
      SubgoalStatePrint(stream, sg->onsuccess);
      fputc(SPACE, stream);
      SubgoalStatePrint(stream, sg->onfailure);
    }
    if (sg->trigger) {
      fputs("trigger:\n", stream);
      ObjPrint(stream, sg->trigger);
    }
    DemonPrintAll(stream, sg->demons);
  }
}

void SubgoalStatusChange(Subgoal *sg, Obj *goal_status, ObjList *causes)
{
  ObjList	*p;
  /* Change goal status in db. */
  if (sg->cur_goal) {
    TE(&sg->ts, sg->cur_goal);
  }
  sg->cur_goal = L(goal_status, sg->ac->actor, sg->obj, E);
  AS(&sg->ts, 0, sg->cur_goal);

  /* Create causal links. */
  for (p = causes; p; p = p->next) {
    LEADTO(&sg->ts, p->obj, sg->cur_goal);
  }

  /* Generate emotional responses. */
  if (sg->spin_emotion) {
    if (sg->spin_to_state == sg->state) {
      LEADTO(&sg->ts, sg->cur_goal, sg->spin_emotion);
      sg->spin_emotion = NULL;
    }
  } else {
    if (!sg->supergoal) {
      UA_EmotionGoal(sg->ac, &sg->ts, sg->ac->actor, sg->cur_goal, causes);
    }
  }
}

Subgoal *SubgoalFind(Actor *ac, Obj *class)
{
  Subgoal	*sg;
  for (sg = ac->subgoals; sg; sg = sg->next) {
    if (ISA(class, I(sg->obj, 0))) return(sg);
  }
  return(NULL);
}

/* End of file. */
