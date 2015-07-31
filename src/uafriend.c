/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19950327: begun
 * 19950330: more work
 * 19950331: attitudes and common sense checks
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

Friend *FriendCreate(Actor *ac, Ts *ts, Obj *actor)
{
  Friend	*f;
  f = CREATE(Friend);
  f->actor = actor;
  f->assertions = NULL;
  TsSetNa(&f->ts_last_seen);
  f->see_interval = SECONDSPERDAYL*7L;
  f->subgoal_maintain = NULL;
  f->next = ac->friends;
  ac->friends = f;
  return(f);
}

Friend *FriendFind(Actor *ac, Ts *ts, Obj *actor, int create_ok)
{
  Friend	*p;
  for (p = ac->friends; p; p = p->next) {
    if (p->actor == actor) return(p);
  }
  if (create_ok) {
    return(FriendCreate(ac, ts, actor));
  }
  return(NULL);
}

void FriendUpdate(Actor *ac, Ts *ts, Friend *f, Obj *old_con, Obj *new_con)
{
  ObjList	*p;
  for (p = f->assertions; p; p = p->next) {
    if (p->obj == old_con) {
      LearnRetract(ts, p->obj, ac->cx->dc);
      p->obj = new_con;
      LearnAssertTs(ts, new_con, ac->cx->dc);
      return;
    }
  }
  LearnAssertTs(ts, new_con, ac->cx->dc);
  f->assertions = ObjListCreate(new_con, f->assertions);
}

Friend *FriendCopy(Friend *f0, Context *cx_parent, Context *cx_child,
                   Subgoal *sg_parents, Friend *next)
{
  Friend	*f;
  f = CREATE(Friend);
  f->actor = f0->actor;
#ifdef notdef
  f->assertions = ContextMapAssertions(cx_parent, cx_child, f0->assertions);
#endif
  f->assertions = f0->assertions;
  f->ts_last_seen = f0->ts_last_seen;
  f->see_interval = f0->see_interval;
  f->subgoal_maintain = SubgoalMapSubgoal(sg_parents, f0->subgoal_maintain);
  f->next = next;
  return(f);
}

Friend *FriendCopyAll(Friend *f, Context *cx_parent, Context *cx_child,
                      Subgoal *sg_parents)
{
  Friend	*r;
  r = NULL;
  while (f) {
    r = FriendCopy(f, cx_parent, cx_child, sg_parents, r);
    f = f->next;
  }
  return(r);
}

void FriendPrint(FILE *stream, Friend *f)
{
  fputs("Friend ", stream);
  fputs(M(f->actor), stream);
  fputc(SPACE, stream);
  TsPrint(stream, &f->ts_last_seen);
  fputc(NEWLINE, stream);
  ObjListPrint(stream, f->assertions);
  if (f->subgoal_maintain) {
    SubgoalPrint(stream, f->subgoal_maintain, 0);
    fputc(NEWLINE, stream);
  }
}

void FriendPrintAll(FILE *stream, Friend *f)
{
  for (; f; f = f->next) {
    FriendPrint(stream, f);
  }
}

#ifdef notdef
/******************************************************************************
 * Take into account degree of agreement of a given trait <attr>.
 * Note this doesn't generate (admittedly weak) inferences if attribute values
 * are not known.
 ******************************************************************************/
void UA_AttrConcord(Actor *ac, Ts *ts, Obj *a, Obj *in, Obj *attr, Obj *actor1,
                    Obj *actor2)
{
  Obj	*attr1_a, *attr2_a;
  Float	attr_concord;
  attr1_a = DbGetAttributeAssertion(ts, NULL, attr, actor1);
  attr2_a = DbGetAttributeAssertion(ts, NULL, attr, actor2);
  attr_concord = ObjWeightConcordance(attr1_a, attr2_a);
  attr_concord = attr_concord*attr_concord;	/* exaggerate differences */
  ContextSetJustifiedSense(ac->cx, attr_concord, attr1_a);
  ContextSetJustifiedSense(ac->cx, attr_concord, attr2_a);
}
#endif

/******************************************************************************
 * Return all the attitude assertions of <a>.
 ******************************************************************************/
ObjList *UA_Friend_AttitudesOf(Ts *ts, Obj *a)
{
  return(RAD(ts, L(N("attitude"), a, ObjWild, E), 1, 0));
}

/******************************************************************************
 * Return all the objects <a> is known to have attitudes about.
 ******************************************************************************/
ObjList *UA_Friend_AttitudeObjects(Ts *ts, Obj *a)
{
  ObjList	*r, *p, *objs;
  objs = UA_Friend_AttitudesOf(ts, a);
  r = NULL;
  for (p = objs; p; p = p->next) {
    if (!ObjListIn(p->obj, r)) r = ObjListCreate(p->obj, r);
  }
  ObjListFree(objs);
  return(r);
}

/******************************************************************************
 * Look up the known overall attitude of an actor toward an actor or object.
 * Returns: attitude weight and assertions in the database.
 ******************************************************************************/
Float UA_Friend_KnownAttitude(Ts *ts, Obj *a, Obj *toward, /* RESULTS */
                              ObjList **assertions)
{
  ObjList	*objs, *p;
  Float		n, d;
  objs = RD(ts, L(N("attitude"), a, toward, E), 0);
  FloatAvgInit(&n, &d);
  for (p = objs; p; p = p->next) {
    FloatAvgAdd(ObjWeight(p->obj), &n, &d);
  }
  *assertions = objs;
  return(FloatAvgResult(n, d, 0.0));
}

/******************************************************************************
 * Look up the known signed-ipr of an actor with an actor.
 * Returns: weight (-1.0=enemies ... 1.0=friends) and assertions in the
 * database.
 ******************************************************************************/
Float UA_Friend_KnownSignedIpr(Ts *ts, Obj *a1, Obj *a2, /* RESULTS */
                               ObjList **assertions)
{
  ObjList	*objs, *objs1, *objs2, *p;
  Float		n, d;
  /* todo: Make this more economical. Should be always infer the symmetric
   * relation or should this be asymmetric anyway. But language is vague
   * on this point for friends.
   */
  objs1 = RD(ts, L(N("signed-ipr"), a1, a2, E), 0);
  objs2 = RD(ts, L(N("signed-ipr"), a2, a1, E), 0);
  objs = ObjListAppendDestructive(objs1, objs2);
  FloatAvgInit(&n, &d);
  for (p = objs; p; p = p->next) {
    if (ISA(N("neg-ipr"), I(p->obj, 0))) {
      FloatAvgAdd(-ObjWeight(p->obj), &n, &d);
    } else {
      FloatAvgAdd(ObjWeight(p->obj), &n, &d);
    }
  }
  *assertions = objs;
  return(FloatAvgResult(n, d, 0.0));
}

/******************************************************************************
 * Infer the overall attitude of an actor toward an actor, by using the rule
 * that actors like actors who like the same things they like.
 * Returns: attitude weight and justifications.
 * todo: Take into account concordance of other personality traits,
 * occupation, etc.
 ******************************************************************************/
Float UA_Friend_InferredAttitude(Ts *ts, Obj *a1, Obj *a2, /* RESULTS */
                                 ObjList **justifications)
{
  ObjList	*objs1, *p, *r, *assertions1, *assertions2;
  Float		val1, val2, concordance, n, d;

  r = NULL;
  FloatAvgInit(&n, &d);
  objs1 = UA_Friend_AttitudeObjects(ts, a1);
  for (p = objs1; p; p = p->next) {
    val1 = UA_Friend_KnownAttitude(ts, a1, p->obj, &assertions1);
    val2 = UA_Friend_KnownAttitude(ts, a2, p->obj, &assertions2);
    concordance = WeightConcordance(val1, val2);
    if (concordance >= .9 || concordance <= .1) {
    /* Extremal values of concordance are taken to be justifications */
      r = ObjListAppendDestructive(r, assertions1);
      r = ObjListAppendDestructive(r, assertions2);
    } else {
      ObjListFree(assertions1);
      ObjListFree(assertions2);
    }
    FloatAvgAdd(Weight01toNeg1Pos1(concordance), &n, &d);
  }
  ObjListFree(objs1);
  return(FloatAvgResult(n, d, 0.0));
}

/******************************************************************************
 * Determine the overall attitude (-1.0...1.0) of an actor toward an actor
 * or other object, whether known or inferred.
 * use_existing_ipr: whether to take into account known iprs, failing known
 * attitudes.
 * Returns: attitude weight and justifications.
 ******************************************************************************/
Float UA_FriendAttitude(Ts *ts, Obj *a, Obj *toward, int use_existing_ipr,
                        /* RESULTS */ ObjList **justifications)
{
  Float		weight;
  ObjList	*just;
  weight = UA_Friend_KnownAttitude(ts, a, toward, &just);
  if (just) goto success;
  if (IsActor(toward)) {
    if (use_existing_ipr) {
      weight = UA_Friend_KnownSignedIpr(ts, a, toward, &just);
      goto success;
    }
    weight = UA_Friend_InferredAttitude(ts, a, toward, &just);
    goto success;
  }
  if (justifications) *justifications = NULL;
  return(0.0);
success:
  if (justifications) {
    *justifications = just;
  } else {
    ObjListFree(just);
  }
  return(weight);
}

/******************************************************************************
 Understand a new friend-of/enemy-of relation.
 ******************************************************************************/
void UA_Friend_FriendEnemy(Actor *ac, Ts *ts, Obj *a, Obj *friend, Float sgn,
                           Obj *in)
{
  ObjList	*justifications;
  Float		sense;
  sense = WeightNeg1Pos1to01(sgn*UA_FriendAttitude(ts, a, friend, 0,
                             &justifications));
  ContextSetJustifiedSenses(ac->cx, sense, justifications);
}

/******************************************************************************
 Understand a new interpersonal relation.
 ******************************************************************************/
void UA_Friend_NewIpr(Actor *ac, Ts *ts, Obj *a, Obj *friend, Obj *in)
{
  Obj	*rel;
  rel = I(in, 0);
  if (ISA(N("friend-of"), rel)) {
    UA_Friend_FriendEnemy(ac, ts, a, friend, 1.0, in);
  } else if (ISA(N("enemy-of"), rel)) {
    UA_Friend_FriendEnemy(ac, ts, a, friend, -1.0, in);
  }
}

/******************************************************************************
 <ipr1> is contrary to <ipr2>
 ******************************************************************************/
Bool UA_Friend_IprIsConflicting(Obj *ipr1, Obj *ipr2)
{
  return((ISA(N("neg-ipr"), ipr1) && ISA(N("pos-ipr"), ipr2)) ||
         (ISA(N("pos-ipr"), ipr1) && ISA(N("neg-ipr"), ipr2)));
}

/******************************************************************************
 <was> may transition to <is>
 ******************************************************************************/
Bool UA_Friend_IprNextState(Obj *was, Obj *is)
{
  return(YES(RE(&TsNA, L(N("next-state-of"), was, is, E))));
}

void UA_Friend_Maintain(Actor *ac, Ts *ts, Obj *a, Obj *in, Friend *f)
{
  Obj	*maint;
  if (f->subgoal_maintain) {
    TOSTATE(ac->cx, f->subgoal_maintain, STFAILURE);
    f->subgoal_maintain = NULL;
  }
  /* Start up the new one. todoDATABASE */
  if (ISA(N("friend-of"), I(in, 0))) {
    maint = N("maintain-friend-of");
  } else {
    maint = NULL;
  }
  if (maint) {
    f->subgoal_maintain = TG(ac->cx, ts, a, L(maint, a, f->actor, E));
  }
}

void UA_Friend_Ipr(Actor *ac, Ts *ts, Obj *a, Obj *in, Obj *other, Friend *f)
{
  int		found;
  Obj		*pred;
  ObjList	*p;
  pred = I(in, 0);
  found = 0;
  for (p = f->assertions; p; p = p->next) {
    if (I(p->obj, 0) == pred) {
      /* NO CHANGE: p->obj: [friend-of a b] in: [friend-of a b] */
      found = 1;
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_NONE);
      ac->cx->reinterp = p->obj;
        /* e.g. "In fact, we've been friends for 10 yrs." */
    } else if (UA_Friend_IprNextState(I(p->obj, 0), pred)) {
      /* NEXT STATE: p->obj: [acquaintance-of a b] in: [friend-of a b] */
      found = 1;
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
      ContextAddMakeSenseReason(ac->cx, p->obj);
      FriendUpdate(ac, ts, f, p->obj, in);
      UA_Friend_Maintain(ac, ts, a, in, f);
    } else if (ISA(N("pos-ipr"), I(p->obj, 0)) && ISA(N("pos-ipr"), pred)) {
      /* RANDOM SKIPPING: p->obj: [acquaintance-of a b] in: [ex-spouse-of a b]
       */
      found = 1;
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_LITTLE, NOVELTY_TOTAL);
      ContextAddNotMakeSenseReason(ac->cx, p->obj);
      FriendUpdate(ac, ts, f, p->obj, in);
    } else if (UA_Friend_IprIsConflicting(I(p->obj, 0), pred)) {
      /* TURNAROUND: p->obj: [enemy-of a b] in: [friend-of a b] */
      found = 1;
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_LITTLE, NOVELTY_TOTAL);
      ContextAddNotMakeSenseReason(ac->cx, p->obj);
      FriendUpdate(ac, ts, f, p->obj, in);
      UA_Friend_Maintain(ac, ts, a, in, f);
    }
  }
  if (!found) {
    /* ADD: f->assertions: [colleague-of a b]
     * in: [friend-of a b]
     */
    ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
    UA_Friend_NewIpr(ac, ts, a, other, in);
    FriendUpdate(ac, ts, f, NULL, in);
    UA_Friend_Maintain(ac, ts, a, in, f);
  }
}

void UA_Friend_Attitude(Actor *ac, Ts *ts, Obj *a, Obj *in)
{
  Float		weight, new_weight;
  Obj		*other;
  ObjList	*objs;
  other = I(in, 2);
  new_weight = ObjWeight(in);
  weight = UA_FriendAttitude(ts, a, other, 1, &objs);
  if (objs) {
  /* Attitudes already known. */
    if (FloatSign(weight) != FloatSign(new_weight)) {
      /* Attitudes are fairly stable, but they can change too. */
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_HALF, NOVELTY_TOTAL);
      ContextAddNotMakeSenseReasons(ac->cx, objs);
      DbRetractList(ts, objs);
      LearnAssertTs(ts, in, ac->cx->dc);
    } else {
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_HALF);
      ContextAddMakeSenseReasons(ac->cx, objs);
      TE(ts, L(I(in, 0), a, other, E));	/* todo: learning */
      LearnAssertTs(ts, in, ac->cx->dc);
    }
    ObjListFree(objs);
  } else {
  /* No attitudes yet known. */
    ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
    weight = UA_Friend_InferredAttitude(ts, a, other, &objs);
    ContextSetJustifiedSenses(ac->cx, WeightConcordance(weight, new_weight),
                              objs);
  }
}

/******************************************************************************
 * Examples:
 * (1)
 * in: [grandmother-of a gm]
 * db: [mother-of a m] [father-of a f]
 * infer either: [mother-of m gm] or [mother-of f gm]
 * (2)
 * in: [mother-of a m]
 * db: [mother-of m mgm]
 * infer: [maternal-grandmother-of a mgm]
 * Enforce one mother-of and one father-of
 * todo: Only keep mother-of/father-of unless other relations input
 *       (can mother-of/father-of ever be inferred from others?
 *        or are they always ambiguous?)
 *       Use theorem prover for QA.
 * Infer sex here if previously unknown.
 ******************************************************************************/
void UA_Friend_Relative(Actor *ac, Ts *ts, Obj *a, Obj *in)
{
  /* ProveRetrieve(ts, NULL, in, 0); */
}

void UA_Friend_Tutoyer(Actor *ac, Ts *ts, Obj *a, Obj *in, Obj *other)
{
  ObjList	*ipr_assertions, *ena1, *ena2, *prev, *p;
  Float		friendship;
  if (a == other) {
    return;
  }
  friendship = UA_Friend_KnownSignedIpr(ts, a, other, &ipr_assertions);
  ena1 = RE(ts, L(N("diploma-of"), a, ObjWild, ObjWild, N("ENA"), E));
  ena2 = RE(ts, L(N("diploma-of"), other, ObjWild, ObjWild, N("ENA"), E));
  if (ISA(N("tutoyer"), I(in, 0))) {
    if (ena1 && ena2) {
      ContextAddMakeSenseReasons(ac->cx, ena1);
      ContextAddMakeSenseReasons(ac->cx, ena2);
    }
    if (friendship < 0.0) {
      if (ena1 && ena2) {
        ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
      } else {
      /* todo: Policemen tutoyer in arrest script makes total sense, etc. */
        ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_HALF, NOVELTY_TOTAL);
        ContextAddNotMakeSenseReasons(ac->cx, ipr_assertions);
      }
    } else {
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
      ContextAddMakeSenseReasons(ac->cx, ipr_assertions);
    }
  } else if (ISA(N("vouvoyer"), I(in, 0))) {
    if (ena1 && ena2) {
      ContextAddNotMakeSenseReasons(ac->cx, ena1);
      ContextAddNotMakeSenseReasons(ac->cx, ena2);
    }
    if (friendship < 0.0) {
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
      ContextAddMakeSenseReasons(ac->cx, ipr_assertions);
    } else {
      if (ena1 && ena2) {
        ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_LITTLE, NOVELTY_TOTAL);
        ContextAddNotMakeSenseReasons(ac->cx, ipr_assertions);
      } else {
      /* todo: Use "vous" when very angry at friend, but broken friendship
       * would handle this?
       */
        ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_HALF, NOVELTY_TOTAL);
      }
    }
  }
  if (!(prev = RD(ts, L(N("tutoyer-vouvoyer"), a, other, E), 0))) {
    LearnAssertTs(ts, in, ac->cx->dc);
    return;
  }
  for (p = prev; p; p = p->next) {
    if (I(in, 0) != I(p->obj, 0)) {
      LearnRetract(ts, p->obj, ac->cx->dc);
      LearnAssertTs(ts, in, ac->cx->dc);
      return;
    }
  }
}

Bool DiscourseIsSpeakerStranger(Discourse *dc)
{
  return(ObjListIsLengthOne(DCSPEAKERS(dc)) &&
         ISA(N("stranger"), DCSPEAKERS(dc)->obj));
}
 
void UA_NameRequest(Context *cx, Discourse *dc)
{
  return;	/* Keeps asking "What's your name?" */
#ifdef notdef
  /* If speaker is a stranger, "What is your name?" */
  if ((cx->dc->mode & DC_MODE_CONV) &&
      DiscourseIsSpeakerStranger(dc)) {
    UA_AskerQWQ(cx, 1.0, N("UA_Asker"), N("human-name"),
                NULL,
                N("name-request"),
                L(N("NAME-of"), N("role-speaker"), N("human-name"), E));
  }
#endif
}

Bool IsFirstConversationWith(Obj *human)
{
/* todo: Take into account email messages from human to Me. */
  return(!YES(RE(&TsNA, L(N("online-communication"), Me, human, E))));
}

/* "My name is Jim Garnier."
 * "Jim Garnier."
 * [name-of stranger2381 NAME:"Jim Garnier"]
 * [name-of role-speaker NAME:"Jim Garnier"]
 * todo: Handoffs as in "I'm going to put my friend X on." or
 *       "Hi, this is X cutting in."
 * todo: If information has already been gathered about this
 *       person, "transplant" the stranger concept to the
 *       new nonstranger human.
 */
void UA_NameRequestResponse(Context *cx, Ts *ts, Obj *in)
{
  Name		*nm;
  Obj		*newhuman;
  ObjList	*candidates;
  if (DiscourseIsSpeakerStranger(cx->dc) &&
      ISA(N("NAME-of"), I(in, 0)) &&
      (ISA(N("possessive-determiner"), I(I(in, 1), 0)) &&
       ISA(N("name"), I(I(in, 1), 1))) &&
      (ISA(N("stranger"), I(in, 1)) ||
       ISA(N("role-speaker"), I(in, 1))) &&
      (nm = ObjToHumanName(I(in, 2)))) {
    if ((candidates = DbHumanNameRetrieve(&TsNA, NULL, nm))) {
    /* This name has been heard before. */
      if (ObjListIsLengthOne(candidates)) {
        ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_MOSTLY);
        DiscourseDeicticStackSpeakersSet(cx->dc,
                                         ObjListCreate(candidates->obj, NULL));
        if (IsFirstConversationWith(candidates->obj)) {
          CommentaryAdd(cx, ObjListCreate(N("greeting-first-meeting"), NULL),
                        NULL);
        }
      } else {
      /* todo: Several possibilities. If the names are textually the
       * same, we are going to have to ask other questions to disambiguate,
       * such as email addresses. Otherwise, we can query on the name
       * possibilities though privacy issues could come into play here.
       * Better to ask for additional information such as middle initial,
       * last name, etc.
       */
      }
    } else {
    /* We have never heard this name before. */
    /* todo: intention? */
      ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
      newhuman = LearnHuman(nm, nm->fullname, cx->dc, NULL);
      DiscourseDeicticStackSpeakersSet(cx->dc, ObjListCreate(newhuman, NULL));
      CommentaryAdd(cx, ObjListCreate(N("greeting-first-meeting"), NULL), NULL);
    }
  }
}

/******************************************************************************
 * Top-level Friend Understanding Agent.
 * todo: Assimilate intensity values as well.
 *       Activate goal to see friend.
 * todo: It seems like we might need to run understanders on known information
 *       on boot, to set Actor up with friends, etc.
 ******************************************************************************/
void UA_Friend(Actor *ac, Ts *ts, Obj *a, Obj *in)
{
  Obj		*other, *pred;
  Friend	*f;
  pred = I(in, 0);
  if (!GetActorPair(in, a, &other)) return;
  if (a == other) return;
  if (ISA(N("ipr"), pred) && other &&
      (f = FriendFind(ac, ts, other, 1))) {
        /* FriendFind always succeeds here. */
    UA_Friend_Ipr(ac, ts, a, in, other, f);
  }
  if (ISA(N("attitude"), pred) && a == I(in, 1)) {
    UA_Friend_Attitude(ac, ts, a, in);
  }
  if (ISA(N("relative-of"), pred)) {
    UA_Friend_Relative(ac, ts, a, in);
  }
  if (ISA(N("tutoyer-vouvoyer"), pred) && other) {
    UA_Friend_Tutoyer(ac, ts, a, in, other);
  }
}

Bool UA_FriendDoesHandle(Obj *rel)
{
  return(ISA(N("ipr"), rel) ||
         ISA(N("attitude"), rel) ||
         ISA(N("relative-of"), rel) ||
         ISA(N("tutoyer-vouvoyer"), rel));
}

/* [maintain-friend-of <a> other] */
void PA_MaintainFriendOf(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Ts		ts_see;
  Friend	*f;
  Dbg(DBGPLAN, DBGOK, "PA_MaintainFriendOf", E);
  f = FriendFind(sg->ac, ts, I(o, 2), 1);
  switch (sg->state) {
    case STBEGIN:
      f->ts_last_seen = *ts;
      f->see_interval = SECONDSPERDAYL*7L;
      TOSTATE(cx, sg, 100);
      return;
    case 100:
      ts_see = f->ts_last_seen;
      TsIncrement(&ts_see, f->see_interval);
      if (TsGT(ts, &ts_see)) {
        SG(cx, sg, 200, 300, L(N("appointment"), a, f->actor, ObjNA, ObjNA,
                               ObjNA, E));
      }
      return;
    case 200:
      f->ts_last_seen = *ts;
      TOSTATE(cx, sg, 100);
      return;
    case 300:
      TOSTATE(cx, sg, 100);
      return;
  }
}

/* End of file. */
