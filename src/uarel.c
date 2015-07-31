/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19951117: merged handling of different reltypes
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
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

#define INCOMPAT	0
#define COMPAT		1
#define LEARNED		2

/* INCOMPAT = incompatible ts's  & mods not done
 * COMPAT   = compatible ts's    & mods not done
 * LEARNED  = specification      & mods done
 */
int UA_Relation_TsDestructive(Ts *existing, Ts *new, Context *cx)
{
  if (TsIsNa(new) || TsEQ(existing, new)) return(COMPAT);
  if (TsIsNa(existing) || TsIsSpecification(new, existing)) {
    existing = new;
    return(LEARNED);
  }
  return(INCOMPAT);
}

/* Invoked when the input relation is exactly the same as existing assertion,
 * except (perhaps) the tsr.
 * The purpose of this function is to (possibly) learn parts of the tsr.
 * Examples:
 * rstart   rstop    existing                          new
 * ------   -------  --------------------------------- -------------------------
 * COMPAT   COMPAT   @1960:1994|[chair-of Sony Morita] @1960:1994|[chair-of S M]
 * COMPAT   LEARNED  @1960:na|[chair-of Sony Morita]   @na:1994|[chair-of S M]
 * LEARNED  COMPAT   @na:1994|[chair-of Sony Morita]   @1960:na|[chair-of S M]
 * LEARNED  COMPAT   @1960:1994|[chair-of Sony Morita] @19600202:1994|[c-of S M]
 * INCOMPAT COMPAT   @1960:1994|[chair-of Sony Morita] @1961:1994|[chair-of S M]
 *     disjoint      @na:1965|[chair-of Sony Morita]   @1970:na|[chair-of S M]
 *     disjoint      @1960:1965|[chair-of Sony Morita] @1970:na|[chair-of S M]
 *     disjoint      @1960:1965|[chair-of Sony Morita] @1970:1994|[chair-of S M]
 *     disjoint      @1960:na|[chair-of Sony Morita]   @na:1955|[chair-of S M]
 */
void UA_RelationIdentical012(Obj *existing, Obj *new, Context *cx,
                             /* RESULTS */ int *assertnew)
{
  TsRange	*existing_tsr, *new_tsr;
  int		rstart, rstop;
  existing_tsr = ObjToTsRange(existing);
  new_tsr = ObjToTsRange(new);
  if ((TsIsSpecific(&existing_tsr->stopts) &&
       TsIsSpecific(&new_tsr->startts) &&
       TsGT(&new_tsr->startts, &existing_tsr->stopts)) ||
      (TsIsSpecific(&existing_tsr->startts) &&
       TsIsSpecific(&new_tsr->stopts) &&
       TsLT(&new_tsr->stopts, &existing_tsr->startts))) {
    /* New is temporally disjoint with existing.
     * todo: this might not make sense for certain relations.
     */
    ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_MOSTLY, NOVELTY_HALF);
    ContextAddMakeSenseReason(cx, existing);
    return;
  }
  /* todoSCORE: Note this does learning of one ts even if the other one is
   * incompatible. We might want to reject both ts's if one is incompatible.
   */
  rstart = UA_Relation_TsDestructive(&existing_tsr->startts, &new_tsr->startts,
                                     cx);
  rstop = UA_Relation_TsDestructive(&existing_tsr->stopts, &new_tsr->stopts,
                                    cx);
  if (rstart == INCOMPAT || rstop == INCOMPAT) {
  /* Either or both of the start/stop timestamps disagree with what was already
   * known. Note that timestamp specifications (e.g. January 1965 to January 5,
   * 1965) are handled.
   * todo: Perhaps allow a certain amount of timestamp fuzz.
   */
    ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_LITTLE, NOVELTY_HALF);
    ContextAddNotMakeSenseReason(cx, existing);
      /* It is possible that one ts is INCOMPAT and the other LEARNED.
       * In this case the commentary lies about what exactly it already knew.
       * We might want to make copy of existing_tsr?
       */
  }
  if (rstart == LEARNED || rstop == LEARNED) {
  /* New provides new information. */
    *assertnew = 0;
    ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_MOSTLY);
    ContextAddMakeSenseReason(cx, existing);
    LearnCommentary_Specification(cx, existing, new, 1);
    LearnAssertDest(existing, cx->dc);
  }
  if (rstart == COMPAT && rstop == COMPAT) {
  /* New is identical or more general than existing. */
    *assertnew = 0;
    ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_NONE);
    ContextAddMakeSenseReason(cx, existing);
  }
}

/* Set (learn) the stopts of existing assertions.
 *
 * Examples:
 * case existing                          new
 * ---- --------------------------------- ----------------------------------
 *    1 @1960:na|[chair-of Sony Morita]   @1994:na|[chair-of Sony Kimba]
 *    1 @na:na|[chair-of Sony Morita]     @1994:na|[chair-of Sony Kimba]
 *      @1960:1994|[chair-of Sony Morita] @1994:na|[chair-of Sony Kimba]
 *      @1960:na|[chair-of Sony Morita]   @na:1990|[chair-of Sony Kimba]
 */
void UA_RelationSuccession(ObjList *existing, Obj *new, Context *cx)
{
  ObjList	*p;
  TsRange	*existing_tsr, *new_tsr;
  new_tsr = ObjToTsRange(new);
  for (p = existing; p; p = p->next) {
    existing_tsr = ObjToTsRange(p->obj);
    if (TsIsNa(&existing_tsr->stopts) &&
        TsIsSpecific(&new_tsr->startts) &&
        (TsIsNa(&existing_tsr->startts) ||
         TsGT(&new_tsr->startts, &existing_tsr->startts))) {
      /* case (1) */
      ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_MOSTLY);
      LearnRetract(&new_tsr->startts, p->obj, cx->dc); 
    }
  }
}

void UA_RelationSpecification(Obj *old, Obj *new, Context *cx)
{
  ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_HALF);
  ContextAddMakeSenseReason(cx, old);
  TE(&cx->TsNA, old); /* todo: retract at all ts's */
  LearnCommentary_Specification(cx, old, new, 0);
}

/* todo: More logic needed for one-to-one? For example, spouse-of. */
void UA_Relation2(Obj *new, Obj *reltype, Context *cx)
{
  int		assertnew;
  ObjList	*objs, *succession, *p;
  objs = RE(&cx->TsNA, L(I(new, 0), I(new, 1), ObjWild, E));
  succession = NULL;
  assertnew = 1;
  for (p = objs; p; p = p->next) {
    if (I(new, 1) == I(p->obj, 1) &&
        I(new, 2) == I(p->obj, 2)) {
    /* IDENTICAL
     * reltype:  N("many-to-one")       NULL (= many-to-many)
     * existing: [chair-of Sony Morita] [hobby-of Jim electronics]
     * new:      [chair-of Sony Morita] [hobby-of Jim electronics]
     */
      UA_RelationIdentical012(p->obj, new, cx, &assertnew);
    } else if (I(new, 1) == I(p->obj, 1) &&
               ObjIsSpecifPart(I(new, 2), I(p->obj, 2))) {
    /* SLOT 1 IDENTICAL, SLOT 2 SPECIFICATION
     * reltype:  N("many-to-one")       NULL (= many-to-many)
     * existing: [chair-of Sony human]  [hobby-of Jim electronics]
     * new:      [chair-of Sony Morita] [hobby-of Jim PC-boards]
     */
      UA_RelationSpecification(p->obj, new, cx);
    } else if (ObjIsSpecifPart(I(new, 1), I(p->obj, 1)) &&
               I(new, 2) == I(p->obj, 2)) {
    /* SLOT 1 SPECIFICATION, SLOT 2 IDENTICAL
     * reltype:  N("many-to-one")           NULL (= many-to-many)
     * existing: [chair-of company Morita]  [hobby-of human electronics]
     * new:      [chair-of Sony Morita]     [hobby-of Jim electronics]
     */
      UA_RelationSpecification(p->obj, new, cx);
    } else if (I(new, 1) == I(p->obj, 1)) {
    /* SLOT 1 IDENTICAL, SLOT 2 DIFFERENT
     * reltype:  N("many-to-one")       NULL (= many-to-many)
     * existing: [chair-of Sony Morita] [hobby-of Jim electronics]
     * new:      [chair-of Sony Kimba]  [hobby-of Jim music]
     */
      if (reltype == N("many-to-one")) {
        succession = ObjListCreate(p->obj, succession);
      }
    } else if (I(new, 2) == I(p->obj, 2)) {
    /* SLOT 1 DIFFERENT, SLOT 2 IDENTICAL */
      if (reltype == N("one-to-many")) {
      /* No examples of this as of 19951117105228. */
        succession = ObjListCreate(p->obj, succession);
      }
    } else {
    /* SLOT 1 DIFFERENT, SLOT 2 DIFFERENT: do nothing */
    } 
  }
  ObjListFree(objs);
  if (succession) {
    UA_RelationSuccession(succession, new, cx);
    ObjListFree(succession);
  }
  if (assertnew) {
    if (cx->rsn.relevance == RELEVANCE_NONE) {
      ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
    }
    LearnAssert(new, cx->dc);
  }
    /* todo: Learning relations such as [owner-of Jim calendar] where
     * calendar is abstract, are suspect.
     */
}

/* todo: If in = [unique-author-of human23 media-object88]
 * then learn that media-object88 is book.
 * In general, do class specification based on restrictions.
 */
void UA_Relation1(Context *cx, Ts *ts, Obj *in)
{
  if (ObjInDeep(ObjWild, in)) return;
  if ((I(in, 1) && ObjIsList(I(in, 1))) ||
      (I(in, 2) && ObjIsList(I(in, 2)))) {
  /* cf [cpart-of [appointment] Jim] */
    return;
  }
  if (ISA(N("set-relation"), I(in, 0))) {
      /* todo */
  } else {
    UA_Relation2(in, DbGetEnumValue(&TsNA, NULL, N("relation-type"), I(in, 0),
                                    NULL),
                 cx);
  }
}

/******************************************************************************
 * Top-level Relation Understanding Agent.
 ******************************************************************************/
void UA_Relation(Context *cx, Ts *ts, Obj *in)
{
  Obj	*rel;
  if (ISA(N("relation"), I(in, 0))) {
    rel = I(in, 0);
    if (UA_OccupationDoesHandle(rel) ||
        UA_FriendDoesHandle(rel) ||
        UA_GoalDoesHandle(rel)) {
    /* Another UA is responsible for handling this relation.
     * UA_Relation subfunctions may in fact be called anyway by that UA.
     */
      return;
    }
    UA_Relation1(cx, ts, in);
  }
}

/* End of file. */
