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

/******************************************************************************
 HELPING FUNCTIONS
 ******************************************************************************/

void UA_GoalSetStatus(Actor *ac, Subgoal *sg, Obj *new_status,
                      Obj *spin_emotion)
{
  Obj	*prev_status;
  prev_status = SubgoalStatus(sg);
  if (prev_status == new_status) {
    /* No change in this subgoal's status. */
 /* todo: This case isn't needed because existing goals should already have
  * created these associated emotions and hence the existing emotion would
  * be found in UA_Emotion?
  */
    ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_EXPECTED);
    return;
  }
  if (new_status == N("active-goal")) {
  /* Flashback? */
    ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_HALF, NOVELTY_TOTAL);
  } else {
  /* Possible irony. */
    ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_LITTLE, NOVELTY_TOTAL);
    ContextAddNotMakeSenseReason(ac->cx, sg->cur_goal);
  }
  PA_SpinToGoalStatus(ac->cx, sg, new_status, spin_emotion);
}

/******************************************************************************
 * Example:
 * subgoal: [leave Mme-Jeanne-Püchl small-city-food-store23]
 * goal:    [glance Mme-Jeanne-Püchl street]
 ******************************************************************************/
void UA_GoalSubgoal(Actor *ac, Ts *ts, Obj *a, Obj *in, Obj *goal, Obj *subgoal)
{
}

/******************************************************************************
 UNDERSTANDING FUNCTIONS
 ******************************************************************************/

/******************************************************************************
 * Top-level Goal Understanding Agent.
 ******************************************************************************/
void UA_Goal(Actor *ac, Ts *ts, Obj *a, Obj *in)
{
  Obj		*goal;
  Subgoal	*sg;
  if (ISA(N("goal-status"), I(in, 0)) &&
      a == I(in, 1)) {
    /* Goal is stated explicitly.
     * Examples:
     * [active-goal Jim [s-employment Jim]]
     * [succeeded-goal Jim [s-employment Jim]]
     * [failed-goal Jim [s-employment Jim]]
     * Note that emotional responses occur as a result of spinning
     * the goal.
     */
    goal = I(in, 2);
    /* If <in> corresponds to existing subgoal, steer that subgoal according
     * to <in>.
     */
    for (sg = ac->subgoals; sg; sg = sg->next) {
      if (ObjSimilarList(goal, sg->obj)) {
        UA_GoalSetStatus(ac, sg, I(in, 0), NULL);
        return;
      }
    }
    /* This is a new subgoal. */
    /* todo: Add more heuristics for making sense.
     * For example, N("s-employment") doesn't make sense if <a> already has
     * a job.
     */
    ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
    if ((sg = TG(ac->cx, &ac->cx->story_time.stopts, a, goal))) {
      PA_SpinToGoalStatus(ac->cx, sg, I(in, 0), NULL);
    }
  } else if (ISA(N("prep-goal-for"), I(in, 0))) {
    UA_GoalSubgoal(ac, ts, a, in, I(in, 2), I(in, 1));
  } else if (ISA(N("intends"), I(in, 0))) {
    UA_GoalSubgoal(ac, ts, a, in, I(in, 1), I(in, 2));
  }
}

Bool UA_GoalDoesHandle(Obj *rel)
{
  return(ISA(N("goal-status"), rel));
}

/* End of file. */
