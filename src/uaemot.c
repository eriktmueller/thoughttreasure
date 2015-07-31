/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19950330: More work.
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

/******************************************************************************
 * Examples:
 * rel:        N("success-emotion-of")  N("failure-emotion-of")
 *                                                    N("activated-emotion-of")
 * goal_class: N("a-social-esteem")     N("a-social-esteem")    N("s-air")
 * result:     pride                    embarrassment           suffocation
 ******************************************************************************/
Obj *UA_Emotion_GCtoEC(Obj *rel, Obj *goal_class)
{
  return(DbGetRelationValue(&TsNA, NULL, rel, goal_class, NULL));
}

/******************************************************************************
 * Examples:
 * goal_status: N("succeeded-goal")   N("failed-goal")        N("active-goal")
 * goal_class:  N("a-social-esteem")  N("a-social-esteem")    N("s-air")
 * result:      pride                 embarrassment           suffocation
 ******************************************************************************/
Obj *UA_Emotion_GSCtoEC(Obj *goal_status, Obj *goal_class)
{
  Obj	*r;
  if (ISA(N("active-goal"), goal_status)) {
    if ((r = UA_Emotion_GCtoEC(N("activated-emotion-of"), goal_class))) {
      return(r);
    }
    return(N("motivation"));
  } else if (ISA(N("failed-goal"), goal_status)) {
    if ((r = UA_Emotion_GCtoEC(N("failure-emotion-of"), goal_class))) {
      return(r);
    }
    return(N("sadness"));
  } else if (ISA(N("succeeded-goal"), goal_status)) {
    if ((r = UA_Emotion_GCtoEC(N("success-emotion-of"), goal_class))) {
      return(r);
    }
    return(N("happiness"));
  } else {
    Dbg(DBGUA, DBGBAD, "UA_Emotion_GSCtoEC 1");
    return(N("sadness"));
  }
}

Obj *UA_Emotion_GStoAEC(Obj *goal_status)
{
  if (ISA(N("failed-goal"), goal_status)) {
    return(N("anger"));
  } else if (ISA(N("succeeded-goal"), goal_status)) {
    return(N("gratitude"));
  } else {
    return(NULL);
  }
}

/******************************************************************************
 * Examples:
 * rel:        N("success-emotion-of")  N("failure-emotion-of")
 *                                                     N("activated-emotion-of")
 * emot_class: N("pride")               N("embarrassment")      N("suffocation")
 * goal:       [a-social-esteem J.]     [a-social-esteem J.]    [s-air J.]
 * result:     1                        1                       1
 ******************************************************************************/
Bool UA_Emotion_IsForGoal(Obj *rel, Obj *emot_class, Obj *goal)
{
  return(emot_class == UA_Emotion_GCtoEC(rel, I(goal, 0)));
}

Float UA_Emotion_Decay(Actor *ac, Float factor)
{
  Float		r;
  ObjList	*p, *prev;
  r = 0.0;
  prev = NULL;
  for (p = ac->emotions; p; p = p->next) {
    ObjWeightSet(p->obj, factor*ObjWeight(p->obj));
    if (ObjWeight(p->obj) < .05) {
      /* Splice out. */
      if (prev) {
        prev->next = p->next;
        /* todoFREE */
      } else ac->emotions = p->next;
    } else prev = p;
  }
  return(r);
}

/******************************************************************************
 INTER-AGENT
 ******************************************************************************/

/******************************************************************************
 * todo: Use this to influence various things like doing favors.
 ******************************************************************************/
Float UA_Emotion_Overall(Actor *ac)
{
  Float		r;
  ObjList	*e;
  r = 0.0;
  for (e = ac->emotions; e; e = e->next) {
    r += ObjWeight(e->obj);
  }
  return(r);
}

/******************************************************************************
 * Example:
 * emotion: [pride Jim WEIGHT_DEFAULT]
 * goal: [succeeded-goal Jim [a-social-esteem Jim]]
 ******************************************************************************/
void UA_EmotionAdd(Actor *ac, Ts *ts, Obj *emotion, Obj *goal, Obj *action)
{
  ac->emotions = ObjListCreate(emotion, ac->emotions);
  AS(ts, 0, emotion);
  if (action) LEADTO(ts, action, emotion);
  if (goal) LEADTO(ts, goal, emotion);
}

/******************************************************************************
 * This is called when a goal is activated, fails, or succeeds, in order to
 * generate the appropriate emotional response.
 * This is not called if the emotional response was already input.
 * Example:
 * in: [succeeded-goal Jim [a-social-esteem Jim]]
 * asserts: [pride Jim WEIGHT_DEFAULT] [leadto [succeeded-goal ...]
 *          [pride ...]]
 * todo: Generate anger/gratitude. Goal importance determines emotion weight.
 ******************************************************************************/
void UA_EmotionGoal(Actor *ac, Ts *ts, Obj *a, Obj *goal, ObjList *causes)
{
  int		causing_actors_cnt;
  Float		weight, weight1;
  Obj		*goal_status, *objective, *actor, *emot_class, *emotion;
  ObjList	*p, *causing_actors;

  goal_status = I(goal, 0);

  /* todo: Determine weight based on importance of goal.
   * Intrinsic importance of goal.
   * Dynamic importance of goal, such as whether an appointment was wanted.
   * For example, dentist appointments aren't normally wanted.
   */
  weight = WEIGHT_DEFAULT;

  /* Generate anger/gratitude emotion(s). */

  if ((emot_class = UA_Emotion_GStoAEC(goal_status))) {

    /* Figure out who besides self caused this goal failure. */
    causing_actors = NULL;
    causing_actors_cnt = 0;
    for (p = causes; p; p = p->next) {
      actor = I(p->obj, 1);
      if (actor != a) {
        if (!ObjListIn(actor, causing_actors)) {
          causing_actors = ObjListCreate(actor, causing_actors);
          causing_actors->u.tgt_obj = p->obj;
          causing_actors_cnt++;
        }
      }
    }

    /* Divide up the anger/gratitude among the causing actors,
     * factoring in general attitude.
     * Examples:
     * A goal failure is caused by an actor <a> is neutral about:
     *   Half the emotion is anger toward the actor.
     *   Half the emotion is undirected.
     * A goal failure is caused by an actor <a> hates:
     *   All of the emotion is anger toward the actor.
     * A goal failure is caused by an actor <a> loves:
     *   All of the emotion is undirected.
     * A goal failure is caused by two actors <a> is neutral about:
     *   One quarter of the emotion is anger toward actor1.
     *   One quarter of the emotion is anger toward actor2.
     *   Half the emotion is undirected.
     */
    for (p = causing_actors; p; p = p->next) {
      if (weight < 0) break;
      weight1 = (weight*0.5*(1.0 - UA_FriendAttitude(ts, a, p->obj, 1, NULL)))/
                causing_actors_cnt;
      emotion = L(emot_class, a, p->obj, D(weight1), E);
      weight -= weight1;
      UA_EmotionAdd(ac, ts, emotion, goal, p->u.tgt_obj);
    }
  }

  /* todo: Generate FortunesOfOthers emotions for other actors who
   * know about this goal outcome for <a>. Or just wait for them
   * to be stated?
   */

  /* Generate undirected pos/neg emotion. */
  if (weight > 0.0) {
    objective = I(goal, 2);
    emot_class = UA_Emotion_GSCtoEC(goal_status, I(objective, 0));
    emotion = L(emot_class, a, D(weight), E);
    UA_EmotionAdd(ac, ts, emotion, goal, NULL);
  }
}

/******************************************************************************
 UNDERSTANDING FUNCTIONS
 ******************************************************************************/

/******************************************************************************
 * Examples:
 * a:        Jim                    Jim                    Jim
 * other:    Mary                   Mary                   Mary
 * in:       [happy-for Jim Mary]   [sorry-for J. M.]      [gloating J. M.]
 *             (positive-emotion)     (negative-emotion)     (positive-emotion)
 * attitude: like-human             like-human             like-human
 * weight:   WEIGHT_DEFAULT         WEIGHT_DEFAULT         -WEIGHT_DEFAULT
 * o.e.c.:   positive-emotion       negative-emotion       negative-emotion
 * att:      [like-human J. M. WD]  [like-human J. M. WD]  [like-human J. M.
 *                                                          -WD]
 *
 * todo: Generalize this to work for all the related like/love attitudes as well
 *       as friends/enemies relations.
 *       The intensity rules are more complex: something like
 *         value of actor2's emotion = value of actor1's emotion x
 *                                      value of how much actor2 likes actor1.
 ******************************************************************************/
void UA_Emotion_FortunesOfOthers(Actor *ac, Ts *ts, Obj *a, Obj *in,
                                 Obj *other, Float weight,
                                 Obj *other_emot_class)
{
  int		found;
  Float		weight1;
  Obj		*other_emot_class1;
  ObjList	*causes, *objs, *atts, *p, *q;

  /* Relate <a>'s emotion to <a>'s attitudes. */
  if (0.0 != (weight1 = UA_FriendAttitude(ts, a, other, 1, &atts))) {
    if (FloatSign(weight1) == FloatSign(weight)) {
    /* The input emotion agrees with known attitudes. */
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_HALF);
      ContextAddMakeSenseReasons(ac->cx, atts);
    } else {
    /* The input emotion disagrees with known attitudes. */
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_LITTLE, NOVELTY_TOTAL);
      ContextAddNotMakeSenseReasons(ac->cx, atts);
    }
  } else {
    /* Attitude of <a> toward <other> is unknown. */
    ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_MOSTLY, NOVELTY_MOSTLY);
    UA_Infer(ac->cx->dc, ac->cx, ts,
             L(N("like-human"), a, other, NumberToObj(weight), E), in);
  }

  /* Relate <a>'s emotion to <other>'s emotion.
   * found = <other>'s emotion implied by <a>'s emotion is already known,
   * excluding motivating emotions.
   */
  objs = RD(ts, L(other_emot_class, other, E), 0);
  found = 0;
  for (p = objs; p; p = p->next) {
    if (ISA(N("motivation"), I(p->obj, 0))) continue;
    if ((causes = CAUSES(p->obj, ac->cx))) {
      for (q = causes; q; q = q->next) {
        if (!ISA(N("active-goal"), I(q->obj, 0))) {
          found = 1;
          ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_HALF);
          ContextAddMakeSenseReason(ac->cx, q->obj);
        }
      }
    } else {
      found = 1;
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_HALF);
      ContextAddMakeSenseReason(ac->cx, p->obj);
    }
    ObjListFree(causes);
  }
  ObjListFree(objs);

  if (!found) {
    /* <other>'s emotion implied by <a>'s emotion is not yet known. */
    ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_MOSTLY, NOVELTY_MOSTLY);
    if (other_emot_class == N("positive-emotion")) {
      other_emot_class1 = N("happiness");
    } else if (other_emot_class == N("negative-emotion")) {
      other_emot_class1 = N("sadness");
    } else {
      other_emot_class1 = other_emot_class;
    }
    UA_Infer(ac->cx->dc, ac->cx, ts,
             L(other_emot_class1, other, NumberToObj(FloatAbs(weight)), E),
             in);
  }

  /* todo: Relate <a>'s emotion to <other>'s goal. */
}

/******************************************************************************
 * Find existing goals related to the emotion and alter their status if
 * necessary.
 * Examples:
 * in:         [embarrassment J.]   [suffocation J.] [pride J.]
 * sg->obj:    [a-social-esteem J.] [s-air J.]       [a-social-esteem J.]
 * old status: active-goal          active-goal      failed-goal
 * set status: failed-goal          (no change)      (no change)
 * result:     1                    1                0 (irony?)
 ******************************************************************************/
Bool UA_Emotion_ExistingGoals(Actor *ac, Ts *ts, Obj *a, Obj *in)
{
  int		found;
  Obj		*emot_class;
  Subgoal	*sg;
  emot_class = I(in, 0);
  for (sg = ac->subgoals; sg; sg = sg->next) {
    if (SubgoalStateIsStopped(sg->state)) continue;
    if (sg->supergoal) continue;
    if (UA_Emotion_IsForGoal(N("success-emotion-of"), emot_class, sg->obj)) {
      found = 1;
      UA_GoalSetStatus(ac, sg, N("succeeded-goal"), in);
    } else if (UA_Emotion_IsForGoal(N("failure-emotion-of"), emot_class,
                                    sg->obj)) {
      found = 1;
      UA_GoalSetStatus(ac, sg, N("failed-goal"), in);
    } else if (UA_Emotion_IsForGoal(N("activated-emotion-of"), emot_class,
                                    sg->obj)) {
      found = 1;
      UA_GoalSetStatus(ac, sg, N("active-goal"), in);
    }
  }
  return(found);
}

/******************************************************************************
 * This function is invoked in case no existing goals are found related to the
 * input emotion. Goals are inferred here.
 * Examples:
 * in:         [embarrassment Jim]                    [suffocation Jim]
 * rel:        failure-emotion-of                     activated-emotion-of
 * goal_status:failed-goal                            active_goal
 * goal_class: p-social-esteem                        s-air
 * infer:      [failed-goal J. [p-social-esteem J.]]  [active-goal Jim
 *                                                     [s-air Jim]]
 ******************************************************************************/
Bool UA_Emotion_NewGoals(Actor *ac, Ts *ts, Obj *a, Obj *in, Obj *rel,
                         Obj *goal_status)
{
  Obj		*goal_class;
  Subgoal	*sg;
  if (!(goal_class = DbGetRelationValue1(&TsNA, NULL, rel, I(in, 0), NULL))) {
    return(0);
  }
  if (goal_class == N("goal-objective")) {
  /* This is not specific enough to bother with. */
    return(0);
  }
  ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_MOSTLY, NOVELTY_TOTAL);
  sg = TG(ac->cx, &ac->cx->story_time.stopts, a, L(goal_class, a, E));
  PA_SpinToGoalStatus(ac->cx, sg, goal_status, in);
  return(1);
}

/******************************************************************************
 * Understanding previously unknown emotions.
 * todo: Angry, grateful.
 ******************************************************************************/
void UA_Emotion1(Actor *ac, Ts *ts, Obj *a, Obj *in)
{
  Obj	*emot_class, *other;
  emot_class = I(in, 0);
  if (ISA(N("fortunes-of-others-emotion"), emot_class)) {
    other = I(in, 2);
    if (ISA(N("happy-for"), emot_class)) {
      UA_Emotion_FortunesOfOthers(ac, ts, a, in, other,
                                  ObjWeight(in), N("positive-emotion"));
    } else if (ISA(N("sorry-for"), emot_class)) {
      UA_Emotion_FortunesOfOthers(ac, ts, a, in, other,
                                  ObjWeight(in), N("negative-emotion"));
    } else if (ISA(N("gloating"), emot_class)) {
      UA_Emotion_FortunesOfOthers(ac, ts, a, in, other,
                                  -ObjWeight(in), N("negative-emotion"));
    } else if (ISA(N("resentment"), emot_class)) {
      UA_Emotion_FortunesOfOthers(ac, ts, a, in, other,
                                  -ObjWeight(in), N("positive-emotion"));
    } else Dbg(DBGUA, DBGBAD, "UA_Emotion1 1");
    return;
  }
  if (UA_Emotion_ExistingGoals(ac, ts, a, in)) return;
  if (UA_Emotion_NewGoals(ac, ts, a, in, N("failure-emotion-of"),
                          N("failed-goal"))) {
    return;
  }
  if (UA_Emotion_NewGoals(ac, ts, a, in, N("success-emotion-of"),
                          N("succeeded-goal"))) {
    return;
  }
  if (UA_Emotion_NewGoals(ac, ts, a, in, N("activated-emotion-of"),
                          N("active-goal"))) {
    return;
  }
  /* todo: In worst case, assume emotion resulted from the most recently
   * performed action?
   */
}

/******************************************************************************
 * Top-level Emotion Understanding Agent.
 * todo: Anger reduces friendship level slightly, increases enemy level.
 ******************************************************************************/
void UA_Emotion(Actor *ac, Ts *ts, Obj *a, Obj *in)
{
  Obj		*emot_class, *from, *to;
  ObjList	*p, *causes;
  if (GetActorSpatial(in, a, &from, &to)) {
  /* todo: If you ptrans to a place you like, you are happy. */
  } else if (ISA(N("emotion"), I(in, 0)) &&
             (!ISA(N("attitude"), I(in, 0))) &&
             a == I(in, 1)) {
    /* [happy Jim] [sad Jim] [happy-for Mary Mary-roommate] */
    emot_class = I(in, 0);
    for (p = ac->emotions; p; p = p->next) {
      if (I(p->obj, 0) == emot_class) {
        /* Emotion is same as an existing one: alter intensity.
         * For example, after A canceling an appointment with B, the input
         * "B is angry at A" makes sense because this emotion was
         * already added by the cancel.
         */
        ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_EXPECTED);
        if ((causes = CAUSES(p->obj, ac->cx))) {
          ContextAddMakeSenseReasons(ac->cx, causes);
          ObjListFree(causes);
        }
        ObjWeightSet(p->obj, ObjWeight(p->obj) + ObjWeight(in));
        return;
      }
      /* todo: Perhaps handle emotion specifications and generalizations.
       * But this might not have so much meaning: You can be both happy
       * and proud.
       */
    }
    UA_Emotion1(ac, ts, a, in);
    UA_EmotionAdd(ac, ts, in, NULL, NULL);
  }
}

Float UA_EmotionSignedWeight(Obj *emot)
{
  Float sgn;
  if (ISA(N("negative-emotion"), I(emot, 0))) {
    sgn = -1.0;
  } else if (ISA(N("positive-emotion"), I(emot, 0))) {
    sgn = 1.0;
  } else {
    sgn = 0.0; /* ? */
  }
  return sgn*ObjWeight(emot);
}

/******************************************************************************
 * Top-level Emotion TsrIn Understanding Agent.
 ******************************************************************************/
void UA_Emotion_TsrIn(Actor *ac, Ts *ts, Obj *a, TsRange *tsr)
{
  /* todo: Base decay factor on ts - previous ts. */
  UA_Emotion_Decay(ac, 0.95);
}

/* End of file. */
