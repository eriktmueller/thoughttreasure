/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19941221: begun
 * 19941222: more work
 * 19941224: more work
 * 19941229: more work
 * 19941230: more work
 * 19941231: more work
 * 19950117: switching to philosophy of treating every single concept
 *           specially to avoid ordering problem
 * 19950323: begun pragmatics
 * 19950328: changed name from Instantiator to UA (Understanding Agent)
 * 19950402: coded sample pass the salt expansion
 * 19950402: cosmetic changes
 * 19951024: switched from consider/incorporate to contexts
 * 19951130: Eliminated Scenes: all Space and Time is handled by
 *           individual UAs.
 * 19951210: Integrated Syn_Parse, Sem_Parse, and Sem_Anaphora. They now
 *           all work in "parallel" on a parse. This should reduce search.
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

void UA_Actor(Discourse *dc, Context *cx, Obj *in)
{
  int	i, len;
  if (!ObjIsList(in)) {
    if (IsActor(in)) {
      ContextActorFind(cx, in, 1);
    }
  }
  for (i = 0, len = ObjLen(in); i < len; i++) {
    UA_Actor(dc, cx, I(in, i));
  }
}

void UnderstandRunActorTsUA(Discourse *dc, Context *cx, Actor *ac,
                            void (*fn)(Actor *, Ts *, Obj *, TsRange *),
                            char *fn_name, TsRange *tsr_in,
                            /* RESULTS */ Float *numer, Float *denom)
{
  ContextRSNReset(cx);
  fn(ac, &cx->story_time.stopts, ac->actor, tsr_in);
  if (DbgOn(DBGUA, DBGDETAIL)) {
    if (cx->rsn.relevance > RELEVANCE_NONE) {
      fprintf(Log, "%16s returns relev %f sense %f novelty %f\n", fn_name,
              cx->rsn.relevance, cx->rsn.sense, cx->rsn.novelty);
      ContextPrintReasons(Log, cx);
    }
  }
  if (cx->rsn.relevance > 0.0) {
    FloatAvgAdd(cx->rsn.sense, numer, denom);
  }
}

void UnderstandRunActorUA(Discourse *dc, Context *cx, Actor *ac,
                          void (*fn)(Actor *, Ts *, Obj *, Obj *),
                          char *fn_name, Obj *in,
                          /* RESULTS */ Float *numer, Float *denom)
{
  ContextRSNReset(cx);
  fn(ac, &cx->story_time.stopts, ac->actor, in);
  if (DbgOn(DBGUA, DBGDETAIL)) {
    if (cx->rsn.relevance > RELEVANCE_NONE) {
      fprintf(Log, "%16s returns relev %f sense %f novelty %f\n", fn_name,
              cx->rsn.relevance, cx->rsn.sense, cx->rsn.novelty);
      ContextPrintReasons(Log, cx);
    }
  }
  if (cx->rsn.relevance > 0.0) {
    FloatAvgAdd(cx->rsn.sense, numer, denom);
  }
}

/* Returns how much sense <in> makes in <ac>. */
void UnderstandRunActorUAs1(Discourse *dc, Context *cx, Actor *ac, Obj *in,
                            /* RESULTS */ Float *numer, Float *denom)
{
  UnderstandRunActorUA(dc, cx, ac, UA_Occupation, "UA_Occupation", in,
                       numer, denom);
  UnderstandRunActorUA(dc, cx, ac, UA_Emotion, "UA_Emotion", in,
                       numer, denom);
  UnderstandRunActorUA(dc, cx, ac, UA_Goal, "UA_Goal", in, numer, denom);
  UnderstandRunActorUA(dc, cx, ac, UA_Friend, "UA_Friend", in,
                       numer, denom);
  UnderstandRunActorUA(dc, cx, ac, UA_Appointment, "UA_Appointment", in,
                       numer, denom);
  UnderstandRunActorUA(dc, cx, ac, UA_TakeShower, "UA_TakeShower", in,
                       numer, denom);
  UnderstandRunActorUA(dc, cx, ac, UA_Sleep, "UA_Sleep", in, numer, denom);
  UnderstandRunActorUA(dc, cx, ac, UA_Trade, "UA_Trade", in, numer, denom);
  UnderstandRunActorTsUA(dc, cx, ac, UA_Emotion_TsrIn, "UA_Emotion_TsrIn",
                         &cx->story_time, numer, denom);
}

/* Returns how much sense <in> makes in all Actors. */
void UnderstandRunActorUAs(Discourse *dc, Context *cx, Obj *in,
                           /* RESULTS */ Float *numer, Float *denom)
{
  Actor	*ac;
  UA_Actor(dc, cx, in);	/* add RSN results to this */
  for (ac = cx->actors; ac; ac = ac->next) {
    if (!ISA(N("animal"), ac->actor)) continue;
    UnderstandRunActorUAs1(dc, cx, ac, in, numer, denom);
  }
}

void UnderstandRunContextUA(Discourse *dc, Context *cx,
                              void (*fn)(Context *, Ts *, Obj *),
                              char *fn_name, Obj *in,
                              /* RESULTS */ Float *numer, Float *denom)
{
  ContextRSNReset(cx);
  fn(cx, &cx->story_time.stopts, in);
  if (DbgOn(DBGUA, DBGDETAIL)) {
    if (cx->rsn.relevance > RELEVANCE_NONE) {
      fprintf(Log, "%16s returns relev %f sense %f novelty %f\n", fn_name,
              cx->rsn.relevance, cx->rsn.sense, cx->rsn.novelty);
      ContextPrintReasons(Log, cx);
    }
  }
  if (cx->rsn.relevance > 0.0) {
    FloatAvgAdd(cx->rsn.sense, numer, denom);
  }
}

/* Returns how much sense <in> makes in Context -- Context global UAs. */
void UnderstandRunContextUAsPre(Discourse *dc, Context *cx, Obj *in,
                                 /* RESULTS */ Float *numer, Float *denom)
{
  UnderstandRunContextUA(dc, cx, UA_NameRequestResponse,
                         "UA_NameRequestResponse", in, numer, denom);
  UnderstandRunContextUA(dc, cx, UA_Time, "UA_Time", in, numer, denom);
}

void UnderstandRunContextUAsPost(Discourse *dc, Context *cx, Obj *in,
                                 /* RESULTS */ Float *numer, Float *denom)
{
  UnderstandRunContextUA(dc, cx, UA_Relation, "UA_Relation", in, numer, denom);
  UnderstandRunContextUA(dc, cx, UA_Weather, "UA_Weather", in, numer, denom);
  if (*numer == 0.0) {
  /* Only run UA_Space if nobody else handled it. */
    UnderstandRunContextUA(dc, cx, UA_Space, "UA_Space", in, numer, denom);
  }
}

Float UA_Statement(Discourse *dc, Context *cx, Obj *obj, int eoschar)
{
  Float		numer, denom, r;
  FloatAvgInit(&numer, &denom);
  UnderstandRunContextUAsPre(dc, cx, obj, &numer, &denom);
  UnderstandRunActorUAs(dc, cx, obj, &numer, &denom);
  UnderstandRunContextUAsPost(dc, cx, obj, &numer, &denom);
  r = FloatAvgResult(numer, denom, 0.0);
  if (DbgOn(DBGUA, DBGDETAIL)) {
    fprintf(Log, "%16s returns %f\n", "UA_Statement", r);
  }
  return(r);
}

/* <*an> not initialized */
Float UnderstandUtterance4(Discourse *dc, Context *cx, Obj *obj, int eoschar,
                           int utype, /* RESULTS */ Answer **an)
{
  ObjSetTsRangeContext(obj, cx);	/* Set context on unrolled concept. */
  switch (utype) {
    case UT_QUESTION:
      return(UA_Question(dc, cx, obj, eoschar, an));
    case UT_STATEMENT:
      if (an) *an = NULL;
      return(UA_Statement(dc, cx, obj, eoschar));
    default:
      break;
  }
  return(0.0);
}

/* <*an> initialized with partial results */
Float UnderstandUtterance3(Discourse *dc, Context *cx, Obj *obj, int eoschar,
                           int utype, /* RESULTS */ Answer **an)
{
  Float		numer, denom, r;
  ObjList	*objs, *p;
  Answer	*an1;
  if (DbgOn(DBGUA, DBGHYPER)) {
    fprintf(Log, "ROLLED %s CONCEPT ",
            utype == UT_QUESTION ? "QUESTION" : "STATEMENT");
    ObjPrettyPrint(Log, obj);
  }
  objs = ObjUnroll(obj);

  if (!ObjListSimilarIn(obj, objs)) {
  /* Add back the unrolled concept:
   * cf [program-output TT [such-that calendar [owner-of calendar Jim]]]
   */
    objs = ObjListCreate(obj, objs);
  }

  FloatAvgInit(&numer, &denom);
  for (p = objs; p; p = p->next) {
    if (IsGarbage(p->obj)) continue;
    if (DbgOn(DBGUA, DBGDETAIL)) {
      fprintf(Log, "UNROLLED %s CONCEPT ",
              utype == UT_QUESTION ? "QUESTION" : "STATEMENT");
      ObjPrettyPrint(Log, p->obj);
    }
    FloatAvgAdd(UnderstandUtterance4(dc, cx, p->obj, eoschar, utype, &an1),
                                     &numer, &denom);
    if (an) *an = AnswerAppendDestructive(*an, an1);
  }
  r = FloatAvgResult(numer, denom, 0.0);
  if (DbgOn(DBGUA, DBGDETAIL)) {
    fprintf(Log, "%16s returns %f\n", "UnderstandUtterance3", r);
  }
  return(r);
}

/* This level is used to unroll the input into manageable chunks.
 * <modifies>: <noun_obj1>.objs = <props1>
 *             <noun_obj2>.objs = <props2>
 * Used to unroll interrogative adverbs (see Morgue).
 * <*an> initialized with partial results
 */
Float UnderstandUtterance2(Discourse *dc, Context *cx, Obj *obj, int eoschar,
                           int utype, /* RESULTS */ Answer **an)
{
  if (ISA(N("tense"), I(obj, 0)) && I(obj, 1)) {
    if (ISA(N("nonfinite-verb"), I(obj, 0))) {
    /* For now, ignore these. cf Jim poured shampoo on his hair. */
      Dbg(DBGUA, DBGDETAIL, "nonfinite verb ignored");
      return(0.0);
    }
    dc->tense = I(obj, 0);
    return(UnderstandUtterance2(dc, cx, I(obj, 1), eoschar, utype, an));
  } else if (ISA(N("temporal-relation"), I(obj, 0)) &&
             I(obj, 1) && I(obj, 2)) {
    /* todo */
    dc->abs_dur = DurationOf(I(obj, 0));
    return(UnderstandUtterance3(dc, cx, obj, eoschar, utype, an));
  } else if ((ISA(N("adverb-of-time"), I(obj, 0)) ||
              ISA(N("temporal-relation"), I(obj, 0))) &&
             I(obj, 2) == NULL) {
    dc->rel_dur = DurValueOf(I(obj, 0));
    return(UnderstandUtterance2(dc, cx, I(obj, 1), eoschar, utype, an));
  } else if (ISA(N("adverb-exactly"), I(obj, 0)) &&
             I(obj, 2) == NULL) {
    dc->exactly = 1;
    return(UnderstandUtterance2(dc, cx, I(obj, 1), eoschar, utype, an));
  } else if (ISA(N("F66"), I(obj, 0)) &&
             (!ISA(N("interrogative-adverb"), I(obj, 0)))) {
  /* Ignore noninterrogative adverbs (such as "justement") for now. */
    return(UnderstandUtterance2(dc, cx, I(obj, 1), eoschar, utype, an));
  } else {
    dc->abs_dur = DurationOf(I(obj, 0));
    return(UnderstandUtterance3(dc, cx, obj, eoschar, utype, an));
  }
  return(0.0);
}

/* <*an> initialized to NULL */
Float UnderstandUtterance1(Discourse *dc, Context *cx, Obj *in, int eoschar,
                           int utype, /* RESULTS */ Answer **an)
{
  Float	r;
  dc->tense = NULL;
  dc->rel_dur = DURNA;
  dc->abs_dur = 1L;
  dc->exactly = 0;
  if (dc->abt && BBrainStopsABrain(dc->abt)) return(0.0);
  r = UnderstandUtterance2(dc, cx, in, eoschar, utype, an);
  if (r != 0.0) r += 0.01*RandomFloat0_1(); /* add some noise */
  return(r);
}

void UA_Infer(Discourse *dc, Context *cx, Ts *ts, Obj *in, Obj *justification)
{
  /* todo: Mark <in> as defeasible? */
  if (dc->abt) BBrainABrainRecursion(dc->abt);
  UnderstandUtterance1(dc, cx, in, ' ', UT_STATEMENT, NULL);
}

Bool UA_CanOnlyBeQuestionConcept(Obj *obj)
{
  Obj	*head;
  head = I(obj, 0);
  return(ISA(N("question-element"), head) ||
         ISA(N("F81"), obj) ||
         ISA(N("F81"), head) ||
         ISADeep(N("question-word"), obj));
}

void UnderstandAsQuestion(Discourse *dc, Obj *obj, PNode *pn, int eoschar,
                          Anaphor *anaphors, Context *parent_cx,
                          /* INPUT AND RESULTS */ Context **children_cxs_p)
{
  Context	*child_cx;

  *children_cxs_p = child_cx =
    ContextSprout(parent_cx, obj, pn, *children_cxs_p);
  child_cx->anaphors = anaphors;
  ObjSetTsRangeContext(obj, child_cx);
  child_cx->answer = NULL;	/* not needed? */
  child_cx->sense = UnderstandUtterance1(dc, child_cx, obj, eoschar,
                                         UT_QUESTION, &child_cx->answer);
  UA_Asker(child_cx, dc);
}

void UnderstandAsStatement(Discourse *dc, Obj *obj, PNode *pn, int eoschar,
                           Anaphor *anaphors, Context *parent_cx,
                           /* INPUT AND RESULTS */ Context **children_cxs_p)
{
  Context	*child_cx;

  *children_cxs_p = child_cx =
    ContextSprout(parent_cx, obj, pn, *children_cxs_p);
  child_cx->anaphors = anaphors;
  ObjSetTsRangeContext(obj, child_cx);
  child_cx->sense = UnderstandUtterance1(dc, child_cx, obj, eoschar,
                                         UT_STATEMENT, NULL);
  UA_Asker(child_cx, dc);
}

void UnderstandAlternative(Discourse *dc, ObjList *p, Obj *obj, ObjList *q,
                           int eoschar, Context *parent_cx, /* RESULTS */
                           Context **children_cxs_r)
{
  Obj	*expanded;
  if ((expanded = UA_AskerInterpretElided(parent_cx, obj))) {
    UnderstandAsStatement(dc, expanded, p->u.sp.pn, eoschar, q->u.sp.anaphors,
                          parent_cx, children_cxs_r);
  } else {
    UnderstandAsQuestion(dc, obj, p->u.sp.pn, eoschar, q->u.sp.anaphors,
                         parent_cx, children_cxs_r);
    if (eoschar != '?' &&
    /* todoSCORE: But this rules out things such as:
     * "I'm kind of bored, you know?"
     */
        (!UA_CanOnlyBeQuestionConcept(obj))) {
      UnderstandAsStatement(dc, obj, p->u.sp.pn, eoschar,
                            q->u.sp.anaphors, parent_cx, children_cxs_r);

    }
  }
}

/* For each alternative Sem_Parse of input,
 *   for each alternative context,
 *    attempt to understand.
 */
void UnderstandAlternatives(Discourse *dc, ObjList *in_objs, int eoschar,
                            /* RESULTS */ Context **children_cxs_r)
{
  ObjList	*p, *q, *objs, *done;
  Context	*parent_cx, *children_cxs;
  children_cxs = NULL;
  done = NULL;
  for (p = in_objs; p; p = p->next) {
    if (ISADeep(N("auxiliary-verb"), p->obj)) continue;
    parent_cx = dc->cx_cur;
    objs = Sem_Anaphora(p->obj, parent_cx, p->u.sp.pn);
    Dbg(DBGUA, DBGDETAIL, "**** UNDERSTANDING AGENCY BEGIN ****");
    for (q = objs; q; q = q->next) {
      q->obj = Sem_AnaphoraPost(q->obj);
      q->u.sp.anaphors = AnaphorAppend(q->u.sp.anaphors, p->u.sp.anaphors);
      if (ObjListSimilarIn(q->obj, done)) continue;
      done = ObjListCreate(q->obj, done);
      DiscourseObjRep(dc, AGENCY_ANAPHORA, q->obj, eoschar);
      if (!(dc->run_agencies & AGENCY_UNDERSTANDING)) continue;
      UnderstandAlternative(dc, p, q->obj, q, eoschar, parent_cx,
                            &children_cxs);
    }
    Dbg(DBGUA, DBGDETAIL, "**** UNDERSTANDING AGENCY END ****");
  }
  ObjListFree(done);
  *children_cxs_r = children_cxs;
}

void UnderstandSwitchAndPrune(Discourse *dc, Context *children_cxs, int eoschar)
{
  /* Switch over to new set of alternatives. */
  ContextFreeAll(dc->cx_alterns);
  dc->cx_alterns = dc->cx_best = children_cxs;

  if (DbgOn(DBGUA, DBGDETAIL)) {
    Dbg(DBGUA, DBGDETAIL, "**** UNPRUNED UNDERSTANDING CONTEXTS ****");
    ContextPrintAll(Log, dc->cx_alterns);
  }

  /* Prune alternatives. */
  dc->cx_alterns = dc->cx_best = ContextPrune(dc->cx_alterns);
  if (dc->cx_best) {
    DiscoursePNodeRep(dc, AGENCY_UNDERSTANDING, dc->cx_best->sproutpn);
    DiscourseObjRep(dc, AGENCY_UNDERSTANDING, dc->cx_best->sproutcon, eoschar);
  }
}

void UnderstandCommitAnaphora(Discourse *dc)
{
  Context	*child_cx;

  /* Commit anaphora. */
  for (child_cx = dc->cx_alterns; child_cx; child_cx = child_cx->next) {
    Sem_AnaphoraCommit(child_cx->anaphors, child_cx);
  }

  if (DbgOn(DBGUA, DBGDETAIL)) {
    Dbg(DBGUA, DBGDETAIL, "**** PRUNED UNDERSTANDING CONTEXTS ****");
    ContextPrintAll(Log, dc->cx_alterns);
  }
}

void UnderstandCommentaryAdd(Context *cx)
{
#ifdef notdef
  But cx->rsn is trashed by now.
  We need to carry cx->rsn.novelty into a cx->novelty
  just as cx->rsn.sense is carried into cx->sense.
 
  if (cx->rsn.relevance <= RELEVANCE_NONE) return;
  if (cx->rsn.novelty <= NOVELTY_NONE) {
  /* "I already knew X." */
    LearnCommentary_I_Know(cx, cx->sproutcon);
  }
#endif
  if (cx->sense < (SENSE_HALF+.1) &&
      cx->not_make_sense_reasons) {
    if (cx->makes_sense_reasons) {
    /* Still, Z. */
      CommentaryAdd(cx,
                    ObjListCreate(L(N("but-true"), 
                                    ObjListToAndObj(cx->makes_sense_reasons),
                                    E),
                                  NULL),
                    NULL);
    }
    /* "But I thought Y." */
    LearnCommentary_I_Thought(cx, ObjListToAndObj(cx->not_make_sense_reasons),
                              0);
  } else if (cx->sense >= SENSE_MOSTLY &&
             cx->makes_sense_reasons) {
    CommentaryAdd(cx,
                  ObjListCreate(L(N("adverb-of-agreement"), 
                                  ObjListToAndObj(cx->makes_sense_reasons),
                                  E),
                                NULL),
                  NULL);
  }
}

void UnderstandAnswerQuestion(Discourse *dc)
{
  AnswerGenInit();	/* matches AnswerGenEnd below */
  AnswerFreeAll(dc->last_answers);
  dc->last_answers = NULL;
  if (dc->cx_best) {
    if (dc->cx_best->answer) {
      dc->last_answers = dc->cx_best->answer;
      AnswerGenAll(dc->cx_best->answer, dc);
    } else {
      UnderstandCommentaryAdd(dc->cx_best);
    }
    CommentaryFlush(dc->cx_best);
  }
  AnswerGenEnd();
}

/* todo: Implement the strategies in
 * (Moeschler and Reboul, 1994, pp. 8, 22, 24, 28).
 * todo: In parsing, adverb-of-politeness -> request.
 * In generation, Searle-ill-s-a-declarative -> aspect-performative.
 */
ObjList *Understand_SpeechAct(ObjList *objs, Discourse *dc)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    if (ISA(N("ability-of"), I(p->obj, 0)) &&
        DiscourseListenerIs(dc, I(p->obj, 1))) {
    /*  Can you pass me the salt?
     * [ability-of listener [hand-to listener speaker saltshaker]] ==>
     * [request speaker listener [hand-to listener speaker saltshaker]]
     */
      objs = ObjListCreate(L(N("request"), DiscourseSpeaker(dc),
                             DiscourseListener(dc), I(p->obj, 2), E), objs);
    } else if (ISA(N("adverb-request"), I(p->obj, 0))) {
    /* Pass the salt, won't you please?
     * [adverb-request [hand-to listener speaker saltshaker]] ==>
     * [request speaker listener [hand-to listener speaker saltshaker]]
     * todo: Request is redundant since imperative used.
     */
      objs = ObjListCreate(L(N("request"), DiscourseSpeaker(dc),
                             DiscourseListener(dc), I(p->obj, 1), E), objs);
    }
  }
  return(objs);
}

void Understand(Discourse *dc, ObjList *in_objs, int eoschar)
{
  Context	*children_cxs;

  if (in_objs == NULL) return;
  if (!(dc->run_agencies & AGENCY_ANAPHORA)) return;

  in_objs = Understand_SpeechAct(in_objs, dc);

  dc->abt = BBrainBegin(N("understand"), 60L, 10);
  UnderstandAlternatives(dc, in_objs, eoschar, &children_cxs);
  BBrainEnd(dc->abt);
  dc->abt = NULL;

  dc->cx_children = ContextAppendDestructive(dc->cx_children, children_cxs);
}

long Sem_ParseCnt, Sem_ParseCntUnpruned;

ObjList *UnderstandSentence2(Channel *ch, size_t lowerb, size_t upperb,
                             int eoschar, Discourse *dc)
{
  DiscourseSetCurrentChannel(dc, DCIN);

  Sem_ParseResults = NULL;
  Syn_ParseParse(ch, dc, lowerb, upperb, eoschar);

/*
  Sem_ParseResults = ObjListUniquifySpecDestSP(Sem_ParseResults);
  This was a nice idea. The trouble is that specifications really
  do have a different meaning. For example, the "universe" can
  either mean our celestial universe as we know it N("our-universe")
  or the set of all possible everythings N("universe").
 */

  Sem_ParseResults = ObjListUniquifyDestSP(Sem_ParseResults);
/*
  Dbg(DBGSEMPAR, DBGDETAIL, "**** UNPRUNED SEMANTIC PARSES ****");
  ObjListPrettyPrint(Log, Sem_ParseResults);
 */
  Sem_ParseCntUnpruned += ObjListLen(Sem_ParseResults);
  Dbg(DBGGEN, DBGDETAIL,
      "%d unpruned semantic parse(s) [session total %ld] of <%.10s>",
      ObjListLen(Sem_ParseResults), Sem_ParseCntUnpruned,
      ch->buf + lowerb);

  Sem_ParseResults = ObjListScoreSortPrune(Sem_ParseResults, 7);

  Dbg(DBGSEMPAR, DBGDETAIL, "**** RESULTS OF SEMANTIC PARSE ****");
  ObjListPrettyPrint(Log, Sem_ParseResults);

  DiscourseObjListRep(dc, AGENCY_SEMANTICS, Sem_ParseResults);

  Sem_ParseCnt += ObjListLen(Sem_ParseResults);
  Dbg(DBGGEN, DBGDETAIL, "%d semantic parse(s) [session total %ld] of <%.10s>",
      ObjListLen(Sem_ParseResults), Sem_ParseCnt, ch->buf + lowerb);
  return(Sem_ParseResults);
}

void UnderstandSentence1(Channel *ch, size_t lowerb, size_t upperb, int eoschar,
                         Discourse *dc)
{
  ObjList	*concepts;
  if (upperb <= lowerb) return;

  dc->relax = 0;
  concepts = UnderstandSentence2(ch, lowerb, upperb, eoschar, dc);

  /* todo: Can we avoid redoing the syntactic parsing work?
  if (!concepts) {
    Dbg(DBGSEMPAR, DBGBAD, "parsing failure; trying relaxed parsing");
    dc->relax = 1;
    concepts = UnderstandSentence2(ch, lowerb, upperb, eoschar, dc);
  }
   */

  if (concepts) {
    Understand(dc, concepts, eoschar);
    Syn_ParseParseDone(ch);
    /* watch out about freeing because of pn->concepts
    ObjListFree(concepts);
     */
  } else {
    Dbg(DBGSEMPAR, DBGBAD, "**** NO SEMANTIC PARSES ****");
  }
}

void UnderstandSentence(Channel *ch, size_t lowerb, size_t upperb, int eoschar,
                        Discourse *dc)
{
  Context	*cx;

  Dbg(DBGUA, DBGDETAIL, "**** PROCESS SENTENCE BEGIN ****");

  ch->input_text = ChannelGetSubstring(ch, lowerb, upperb);

  dc->cx_children = NULL;

  for (cx = dc->cx_alterns; cx; cx = cx->next) {
    Dbg(DBGSEMPAR, DBGDETAIL, "**** PROCESS SENTENCE IN CONTEXT #%ld ****",
        cx->id);
    dc->cx_cur = cx;
    ContextSenseReset(cx);	/* right? */
    UnderstandSentence1(ch, lowerb, upperb, eoschar, dc);
  }

  if (dc->cx_children &&
      (dc->run_agencies & AGENCY_UNDERSTANDING)) {
    UnderstandSwitchAndPrune(dc, dc->cx_children, eoschar);
    UnderstandCommitAnaphora(dc);
    UnderstandAnswerQuestion(dc);
    UA_AskerAskQuestions(dc);
    dc->cx_children = NULL;
  }

/* todoFREE: We can't free this because ch->echo_input_last might point to it.
  MemFree(ch->input_text, "char * Channel");
 */
  ch->input_text = NULL;

  Dbg(DBGUA, DBGDETAIL, "**** PROCESS SENTENCE END ****");
}

/* End of file. */
