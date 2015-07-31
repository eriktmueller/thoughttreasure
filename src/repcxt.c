/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "pa.h"
#include "repactor.h"
#include "repbasic.h"
#include "repcxt.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "reptime.h"
#include "semdisc.h"
#include "synpnode.h"
#include "uaasker.h"
#include "uaquest.h"
#include "utildbg.h"

long		ContextNextTopId;
Discourse	*ContextCurrentDc;
Context         *ContextRoot;

void ContextInit()
{
  Ts  ts;
  ContextCurrentDc = NULL;	/* todoTHREAD */
  ContextNextTopId = 1L;
  TsSetNow(&ts);
  ContextRoot = NULL;
  ContextRoot = ContextCreate(&ts, 0, NULL, NULL);
}

Context *ContextCreate(Ts *ts, TenseStep tensestep, Discourse *dc,
                       Context *next)
{
  Context	*cx;
  cx = CREATE(Context);

  cx->parent = ContextRoot;
  cx->sense = SENSE_TOTAL;
  TsRangeSetNa(&cx->story_time);
  cx->story_time.startts = *ts;
  cx->story_time.stopts = *ts;
  TsRangeSetContext(&cx->story_time, cx);
  cx->story_time.cx = cx;
  cx->story_tensestep = tensestep;
  cx->actors = NULL;
  cx->last_question = NULL;
  cx->dc = dc;
  cx->next = next;

  TsSetNa(&cx->TsNA);
  cx->TsNA.cx = cx;

  cx->answer = NULL;
  cx->commentary = NULL;
  cx->makes_sense_reasons = NULL;
  cx->not_make_sense_reasons = NULL;
  cx->reinterp = NULL;
  cx->anaphors = NULL;
  cx->questions = NULL;

  cx->mode = MODE_STOPPED;
  ContextRSNReset(cx);

  cx->id = ContextNextTopId;
  cx->numchildren = 0;
  ContextNextTopId++;
  cx->sproutcon = NULL;
  cx->sproutpn = NULL;

  Dbg(DBGPLAN, DBGDETAIL, "created Context %ld", cx->id);

  return(cx);
}

Context *ContextSprout(Context *parent, Obj *sproutcon, PNode *sproutpn,
                       Context *next)
{
  Context	*cx;
  cx = CREATE(Context);

  cx->parent = parent;
  cx->sense = parent->sense;
  cx->story_time = parent->story_time;
  TsRangeSetContext(&cx->story_time, cx);
  cx->story_tensestep = parent->story_tensestep;

  cx->last_question = parent->last_question;
  cx->dc = parent->dc;
  cx->next = next;

  TsSetNa(&cx->TsNA);
  cx->TsNA.cx = cx;

  cx->answer = NULL;
  cx->commentary = NULL;
  cx->makes_sense_reasons = parent->makes_sense_reasons;
  cx->not_make_sense_reasons = parent->not_make_sense_reasons;
  cx->reinterp = NULL;
  cx->anaphors = NULL;
  cx->questions = NULL;

  cx->mode = MODE_STOPPED;
  cx->rsn.sense = parent->rsn.sense;
  cx->rsn.relevance = parent->rsn.relevance;
  cx->rsn.novelty = parent->rsn.novelty;

  parent->numchildren++;
  cx->id = parent->id*10L + parent->numchildren;
  cx->numchildren = 0;
  cx->sproutcon = sproutcon;
  cx->sproutpn = sproutpn;

  cx->actors = NULL;

#ifdef notdef
  ContextRepairChildAssertions(parent, cx);
#endif

  cx->actors = ActorCopyAll(parent->actors, parent, cx);

  Dbg(DBGPLAN, DBGDETAIL, "sprouted Context %ld", cx->id);
  if (DbgOn(DBGPLAN, DBGHYPER)) {
    ContextPrint(Log, cx);
  }

  return(cx);
}

Bool ContextIsAncestor(Context *anc, Context *des)
{
  if (anc == NULL) anc = ContextRoot;
  if (des == NULL) des = ContextRoot;
  while (1) {
    if (des == NULL) return 0;
    if (des == anc) return 1;
    des = des->parent;
  }
}

Context *ContextLast(Context *cx)
{
  while (cx->next) cx = cx->next;
  return(cx);
}

Context *ContextAppendDestructive(Context *cx1, Context *cx2)
{
  if (cx1) {
    ContextLast(cx1)->next = cx2;
    return(cx1);
  } else return(cx2);
}

void ContextPrintName(FILE *stream, Context *cx)
{
  if (cx == ContextRoot) {
    fprintf(stream, "#ROOT");
  } else {
    fprintf(stream, "#%ld", cx->id);
  }
}

void ContextNameToString(Context *cx, /* RESULTS */ char *out)
{
  sprintf(out, "#%ld", cx->id);
}

void ContextPrintAllNames(FILE *stream, Context *cxs)
{
  Context	*cx;
  for (cx = cxs; cx; cx = cx->next) {
    fprintf(stream, "0x%p #%ld\n", cx, cx->id);
  }
}

char *ContextModeString(int mode)
{
  switch (mode) {
    case MODE_STOPPED:			return("STOPPED");
    case MODE_SPINNING:			return("SPINNING");
    case MODE_DAYDREAMING:		return("DAYDREAMING");
    case MODE_PERFORMANCE:		return("PERFORMANCE");
    default:
      break;
  }
  return("???");
}

void ContextPrintReasons(FILE *stream, Context *cx)
{
  if (cx->makes_sense_reasons) {
    fputs("Makes sense because:\n", stream);
    ObjListPrint(stream, cx->makes_sense_reasons);
  }
  if (cx->not_make_sense_reasons) {
    fputs("Does not make sense because:\n", stream);
    ObjListPrint(stream, cx->not_make_sense_reasons);
  }
}

void ContextPrint(FILE *stream, Context *cx)
{
  StreamSep(stream);
  fprintf(stream, "Context %ld sense %g ", cx->id, cx->sense);
  fputs(ContextModeString(cx->mode), stream);
  fputc(SPACE, stream);
  TsRangePrint(stream, &cx->story_time);
  fprintf(stream, " tensestep %d\n", cx->story_tensestep);
  if (cx->sproutcon) {
    fputs("last concept ", stream);
    ObjPrint(stream, cx->sproutcon);
    fputc(NEWLINE, stream);
  }
  if (cx->sproutcon) {
    fputs("last pn:\n", stream);
    PNodePrettyPrint(stream, NULL, cx->sproutpn);
  }
  AnswerPrintAll(stream, cx->answer, cx->dc);
  AnswerPrintAll(stream, cx->commentary, cx->dc);
  ContextPrintReasons(stream, cx);
  if (cx->questions) {
    QuestionPrintAll(stream, cx->questions);
  }
  if (cx->reinterp) {
    fputs("Reinterp ", stream);
    ObjPrint(stream, cx->reinterp);
    fputc(NEWLINE, stream);
  }
  ActorPrintAll(stream, cx->actors);
}

void ContextPrintAll(FILE *stream, Context *cxs)
{
  Context	*cx;
  for (cx = cxs; cx; cx = cx->next) {
    ContextPrint(stream, cx);
  }
}

void ContextFree(Context *cx)
{
#ifdef notdef
  /* todoFREE: Contents. */
  MemFree(cx, "Context");
  Dbg(DBGPLAN, DBGDETAIL, "freed Context %ld", cx->id);
#endif
}

void ContextSetSense(Context *cx, Float val)
{
  if (val < cx->rsn.sense) cx->rsn.sense = val;
}

void ContextSetRelevance(Context *cx, Float val)
{
  if (val > cx->rsn.relevance) cx->rsn.relevance = val;
}

void ContextSetNovelty(Context *cx, Float val)
{
  if (val > cx->rsn.novelty) cx->rsn.novelty = val;
}

void ContextRSNReset(Context *cx)
{
  cx->rsn.sense = SENSE_TOTAL;
  cx->rsn.relevance = RELEVANCE_NONE;
  cx->rsn.novelty = NOVELTY_NONE;
}

void ContextSenseReset(Context *cx)
{
  cx->makes_sense_reasons = NULL;
  cx->not_make_sense_reasons = NULL;
}

void ContextRSNPrint(FILE *stream, Context *cx)
{
  fprintf(stream, "sense %g relevance %g novelty %g\n",
          cx->rsn.sense, cx->rsn.relevance, cx->rsn.novelty);
}

void ContextSetRSN(Context *cx, Float relevance, Float sense, Float novelty)
{
  ContextSetRelevance(cx, relevance);
  ContextSetSense(cx, sense);
  ContextSetNovelty(cx, novelty);
}

void ContextAddMakeSenseReason(Context *cx, Obj *obj)
{
  cx->makes_sense_reasons =
    ObjListAddIfSimilarNotIn(cx->makes_sense_reasons, obj);
}

void ContextAddMakeSenseReasons(Context *cx, ObjList *objs)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    cx->makes_sense_reasons =
      ObjListAddIfSimilarNotIn(cx->makes_sense_reasons, p->obj);
  }
}

void ContextAddNotMakeSenseReason(Context *cx, Obj *obj)
{
  cx->not_make_sense_reasons =
    ObjListAddIfSimilarNotIn(cx->not_make_sense_reasons, obj);
}

void ContextAddNotMakeSenseReasons(Context *cx, ObjList *objs)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    cx->not_make_sense_reasons =
      ObjListAddIfSimilarNotIn(cx->not_make_sense_reasons, p->obj);
  }
}

void ContextSetJustifiedSenses(Context *cx, Float sense,
                               ObjList *justifications)
{
  ContextSetSense(cx, sense);
  if (sense <= SENSE_LITTLE) {
    ContextAddNotMakeSenseReasons(cx, justifications);
  } else if (sense >= SENSE_MOSTLY) {
    ContextAddMakeSenseReasons(cx, justifications);
  }
}

void ContextSetJustifiedSense(Context *cx, Float sense, Obj *justification)
{
  ContextSetSense(cx, sense);
  if (sense <= SENSE_LITTLE) {
    ContextAddNotMakeSenseReason(cx, justification);
  } else if (sense >= SENSE_MOSTLY) {
    ContextAddMakeSenseReason(cx, justification);
  }
}

void ContextClearSpin(Context *cx)
{
  Actor		*ac;
  Subgoal	*sg;
  for (ac = cx->actors; ac; ac = ac->next) {
    for (sg = ac->subgoals; sg; sg = sg->next) {
      sg->spin_to_state = STNOSPIN;
    }
  }
}

Context *ContextFindBest(Context *cxs, Context *exclude1, Context *exclude2)
{
  Float		maxsense;
  Context	*cx, *cx_best;
  maxsense = FLOATNEGINF;
  cx_best = NULL;
  for (cx = cxs; cx; cx = cx->next) {
    if (cx == exclude1) continue;
    if (cx == exclude2) continue;
    if (cx->sense > maxsense) {
      maxsense = cx->sense;
      cx_best = cx;
    }
  }
  return(cx_best);
}

Context *ContextPrune3(Context *cxs)
{
  Context	*cx_best1, *cx_best2, *cx_best3, *cx, *n;
  cx_best1 = ContextFindBest(cxs, NULL, NULL);
  cx_best2 = ContextFindBest(cxs, cx_best1, NULL);
  cx_best3 = ContextFindBest(cxs, cx_best1, cx_best2);
  for (cx = cxs; cx; cx = n) {
    n = cx->next;
    if (cx == cx_best1) continue;
    if (cx == cx_best2) continue;
    if (cx == cx_best3) continue;
    ContextFree(cx);
  }
  if (cx_best1 && cx_best2) {
    if (cx_best3) cx_best3->next = NULL;
    cx_best2->next = cx_best3;
    cx_best1->next = cx_best2;
  } else if (cx_best1) {
    cx_best1->next = NULL;
  }
  return(cx_best1);
}

Context *ContextPrune(Context *cxs)
{
  Context	*cx_best1, *cx, *n;
  cx_best1 = ContextFindBest(cxs, NULL, NULL);
  for (cx = cxs; cx; cx = n) {
    n = cx->next;
    if (cx == cx_best1) continue;
    ContextFree(cx);
  }
  if (cx_best1) cx_best1->next = NULL;
  return(cx_best1);
}

void ContextFreeAll(Context *cxs)
{
  Context	*cx, *n;
  for (cx = cxs; cx; cx = n) {
    n = cx->next;
    ContextFree(cx);
  }
}

void ContextInitiateHandlers(Context *cx, Obj *actor)
{
  int	save_goa;
  save_goa = GenOnAssert; 
  GenOnAssert = 0;
  TG(cx, &cx->story_time.stopts, actor, L(N("handle-proposal"), actor, E));
  TG(cx, &cx->story_time.stopts, actor, L(N("sleep"), actor, E));
  GenOnAssert = save_goa;
}

Actor *ContextActorFind(Context *cx, Obj *actor, int create_ok)
{
  Actor	*ac;
  for (ac = cx->actors; ac; ac = ac->next) {
    if (ac->actor == actor) return(ac);
  }
  if (create_ok) {
    ac = ActorCreate(actor, cx, cx->actors);
    cx->actors = ac;
    ContextInitiateHandlers(cx, actor);
    return(ac);
  }
  return(NULL);
}

void ContextOnAssert(TsRange *tsrange, Obj *assertion)
{
  Discourse	*dc;
  Context	*cx;
  if (!(dc = ContextCurrentDc)) return;	/* todoTHREAD: not thread safe */
    /* Of course, the above = is correct. NOT == */
  /* todo: Add this info to db. [affected-object-of off-hook phone] */
  if (ISA(N("state"), I(assertion, 0))) {
    Dbg(DBGPLAN, DBGHYPER, "object state assertion", E);
    PA_ObjStateChange(tsrange, I(assertion, 1), 0);
  } else if (N("dial") == I(assertion, 0)) {
  /* todo: Perhaps dial should be a plan for a state [dialed-digits ]? Nah. */
    PA_ObjStateChange(tsrange, I(assertion, 2), 0);
  }
  if (tsrange->cx == ContextRoot) {
  /* Root assertion gets run in all contexts. */
    for (cx = dc->cx_alterns; cx; cx = cx->next) {
      DemonPtnTestAll(cx, assertion);
    }
  } else {
  /* Context assertion gets run only in that context. */
    DemonPtnTestAll(tsrange->cx, assertion);
  }
}

/* Find specific subgoal objectives unifying with pattern. */
ObjList *ContextFindMatchingSubgoals(Context *cx, Obj *subgoal_obj)
{
  Actor	*ac;
  Subgoal	*sg;
  ObjList	*r;
  r = NULL;
  for (ac = cx->actors; ac; ac = ac->next) {
    for (sg = ac->subgoals; sg; sg = sg->next) {
      if (ObjUnify(subgoal_obj, sg->obj)) {	/* todoFREE: bd */
        r = ObjListCreate(sg->obj, r);
      }
    }
  }
  return(r);
}

/* Find subgoal object whose objective closely matches provided one. */
Subgoal *ContextFindSubgoal(Context *cx, Obj *subgoal_obj)
{
  Actor	*ac;
  Subgoal	*sg;
  for (ac = cx->actors; ac; ac = ac->next) {
    for (sg = ac->subgoals; sg; sg = sg->next) {
      if (ObjMatchList(subgoal_obj, sg->obj)) {	/* todoFREE: bd */
        if (sg->supergoal) return(sg);
      }
    }
  }
  return(NULL);
}

/* Finds supergoal pattern. Not for low-level use. */
Obj *ContextSupergoal(Context *cx, Obj *subgoal_obj)
{
  Subgoal	*sub, *super;
  if ((sub = ContextFindSubgoal(cx, subgoal_obj)) && sub->supergoal) {
    super = sub->supergoal;
    while (super && super->ac->actor != sub->ac->actor) {
      super = super->supergoal; /* was offloaded */
    }
    if (super) return(super->obj);
  }
  return(NULL);
}

ObjList *ContextSubgoals(Context *cx, Obj *subgoal_obj)
{
  ObjList	*r;
  Actor		*ac;
  Subgoal	*super, *sg;
  r = NULL;
  if ((super = ContextFindSubgoal(cx, subgoal_obj))) {
    for (ac = cx->actors; ac; ac = ac->next) {
      for (sg = ac->subgoals; sg; sg = sg->next) {
        if (sg->supergoal == super) r = ObjListCreate(sg->obj, r);
      }
    }
  }
  return(r);
}

/* Caller must look at element 2 of each result object. */
ObjList *ContextGetObjectRels(TsRange *tsr, Obj *scriptprop, Obj *obj)
{
  return(RER(tsr, L(ObjWild, scriptprop, obj, E)));
}

#ifdef notdef
/* For now, this is not required, since leadto's are never retracted? */

Obj *ContextMapAssertion(Context *cx_parent, Context *cx_child, Obj *obj_parent)
{
  ObjList	*p;
  if (obj_parent == NULL) return(NULL);
  for (p = cx_parent->assertions; p; p = p->next) {
    if (p->obj == obj_parent) return(p->u.child_copy);
  }
  Dbg(DBGGEN, DBGBAD, "ContextMapAssertion failure");
  ObjPrint(Log, obj_parent);
  fprintf(Log, "\nnot in:\n");
  ObjListPrint(Log, cx_parent->assertions);
  Stop();
  return(NULL);
}

ObjList *ContextMapAssertions(Context *cx_parent, Context *cx_child,
                              ObjList *objs_parent)
{
  Obj		*obj;
  ObjList	*r, *p;
  r = NULL;
  for (p = objs_parent; p; p = p->next) {
    if ((obj = ContextMapAssertion(cx_parent, cx_child, p->obj))) {
      r = ObjListCreate(obj, r);
    }
  }
  return(r);
}

void ContextRepairChildAssertions(Context *cx_parent, Context *cx_child)
{
  Obj		*obj;
  ObjList	*p;
  for (p = cx_child->assertions; p; p = p->next) {
    if (N("leadto") != I(p->obj, 0)) continue;
      /* For now, these are the only types of objects needing repair */
    if (DbgOn(DBGDB, DBGHYPER)) {
      fputs("REPAIRED ", Log);
      ObjPrint1(Log, p->obj, NULL, 5, 1, 0, 1, 0);
      fputs(" to ", Log);
    }
    if ((obj = ContextMapAssertion(cx_parent, cx_child, I(p->obj, 1)))) {
      ObjSetIth(p->obj, 1, obj);
      if (ObjToTsRange(obj)->cx != cx_child) {
        Stop();
      }
    }
    if ((obj = ContextMapAssertion(cx_parent, cx_child, I(p->obj, 2)))) {
      ObjSetIth(p->obj, 2, obj);
      if (ObjToTsRange(obj)->cx != cx_child) {
        Stop();
      }
    }
    if (DbgOn(DBGDB, DBGHYPER)) {
      ObjPrint1(Log, p->obj, NULL, 5, 1, 0, 1, 0);
      fputc(NEWLINE, Log);
    }
  }
}
#endif

ObjList *ContextFindGrids(Context *cx)
{
  Actor		*ac;
  Obj		*polity, *grid;
  ObjList	*r;
  GridCoord	row, col;
  r = NULL;
  for (ac = cx->actors; ac; ac = ac->next) {
    if (SpaceLocateObject(NULL, &cx->story_time, ac->actor, NULL, 1,
                          &polity, &grid, &row, &col)) {
      r = ObjListCreate(grid, r);
    }
  }
  return(r);
}

ObjList *ContextFindObjects(Context *cx, Obj *class)
{
  Actor		*ac;
  Obj		*polity, *grid;
  ObjList	*r, *p, *objs;
  GridCoord	row, col;
  if (N("human") == class) {
  /* Find humans in context = Actors. */
    r = NULL;
    for (ac = cx->actors; ac; ac = ac->next) {
      r = ObjListCreate(ac->actor, r);
    }
    return(r);
  } else {
  /* Find objects near actors. */
    r = NULL;
    for (ac = cx->actors; ac; ac = ac->next) {
      if (ISAP(class, ac->actor) && !ObjListIn(ac->actor, r)) {
        r = ObjListCreate(ac->actor, r);
      }
      if (SpaceLocateObject(NULL, &cx->story_time, ac->actor, NULL, 0,
                            &polity, &grid, &row, &col)) {
        objs = SpaceGetAllInGrid(NULL, &cx->story_time, grid);
        for (p = objs; p; p = p->next) {
          if (ISAP(class, p->obj) && !ObjListIn(p->obj, r)) {
            r = ObjListCreate(p->obj, r);
          }
        }
        ObjListFree(objs);
      }
    }
    return(r);
  }
}

Bool ContextIsObjectFound(Context *cx, Obj *class)
{
  ObjList	*objs;
  if ((objs = ContextFindObjects(cx, class))) {
    ObjListFree(objs);
    return(1);
  }
  ObjListFree(objs);
  return(0);
}

/* Decay all Antecedents in Context for all Channels. */
void ContextAntecedentDecayAllCh(Context *cx)
{
  int		i;
  Actor		*ac;
  for (ac = cx->actors; ac; ac = ac->next) {
    for (i = 0; i < DCMAX; i++) {
      if (NULL == DiscourseGetIthChannel(cx->dc, i)) continue;
      AntecedentDecay(&ac->antecedent[i]);
    }
  }
}

/* Decay all Antecedents in Context for one Channel (<cx->dc->curchannel>) */
void ContextAntecedentDecayOneCh(Context *cx)
{
  Actor		*ac;
  for (ac = cx->actors; ac; ac = ac->next) {
    AntecedentDecay(&ac->antecedent[cx->dc->curchannel]);
  }
}

/* Refresh (or create for the first time) an Antecedent in all relevant
 * Channels associated with one Context. Relevant Channels are those
 * which are the same language as the input (including the input Channel).
 * For other channels, the input is regenerated (translated) and the
 * Antecedents added by Syn_Gen/DiscourseAnaphorCommit.
 */
void ContextAntecedentRefreshAllCh(Context *cx, Obj *obj, PNode *pn,
                                   PNode *pn_top, int gender, int number,
                                   int person)
{
  int		i, save_channel;
  save_channel = DiscourseGetCurrentChannel(cx->dc);
  for (i = 0; i < DCMAX; i++) {
    if (NULL == DiscourseSetCurrentChannelSameLangAsInput(cx->dc, i)) continue;
    ContextAntecedentRefreshOneCh(cx, obj, pn, pn_top, gender, number, person);
  }
  DiscourseSetCurrentChannel(cx->dc, save_channel);
}

Bool ContextIsAntecedent(Obj *obj, Context *cx)
{
  if (!ISA(N("object"), obj)) return(0);
  if (ISA(N("F72"), obj)) return(0);	/* pronoun */
  if (ISA(N("human"), obj) && !ObjIsConcrete(obj)) return(0);
  if (ObjListIn(obj, DCSPEAKERS(cx->dc))) return(0);
  if (ObjListIn(obj, DCLISTENERS(cx->dc))) return(0);
  return(1);
}

/* Refresh (or create for the first time) an Antecedent in one Channel
 * (<cx->dc->curchannel>) associated with one Context.
 */
void ContextAntecedentRefreshOneCh(Context *cx, Obj *obj, PNode *pn,
                                   PNode *pn_top, int gender, int number,
                                   int person)
{
  int		i, len;
  Obj		*noun;
  ObjList	*props;
  Actor		*ac;
  if (ObjIsNa(obj)) return;
  if ((props = ObjIntensionParse(obj, 0, &noun))) {
    ObjListFree(props);
    ContextAntecedentRefreshOneCh(cx, noun, pn, pn_top, gender, number, person);
    return;
  }
  if (I(obj, 0) == N("and")) {
    for (i = 1, len = ObjLen(obj); i < len; i++) {
      ContextAntecedentRefreshOneCh(cx, I(obj, i), PNI(obj, i), pn_top,
                                    gender, number, person);
    }
    return;
  }
  if (!ContextIsAntecedent(obj, cx)) return;

  for (ac = cx->actors; ac; ac = ac->next) {
    if (ac->actor == obj) {
      AntecedentRefresh(&ac->antecedent[cx->dc->curchannel],
                        obj, pn, pn_top, gender, number, person);
      return;
    }
  }
  cx->actors = ActorCreate(obj, cx, cx->actors);
  AntecedentRefresh(&cx->actors->antecedent[cx->dc->curchannel],
                    obj, pn, pn_top, gender, number, person);
}

#ifdef notdef
void ContextHighestTs(Context *cx, /* OUTPUT */ Ts *ts)
{
  ObjList *p;
  TsRange *tsr;

  TsSetNa(ts);
  for (p = cx->assertions; p; p = p->next) {
    tsr = ObjToTsRange(p->obj);
    if (TsIsSpecific(&tsr->startts)) {
      if ((!TsIsSpecific(ts)) || TsGT(&tsr->startts, ts)) {
        *ts = tsr->startts;
      }
    }
    if (TsIsSpecific(&tsr->stopts)) {
      if ((!TsIsSpecific(ts)) || TsGT(&tsr->stopts, ts)) {
        *ts = tsr->stopts;
      }
    }
  }
  if (!TsIsSpecific(ts)) *ts = cx->story_time.stopts;
}
#endif

/* End of file. */
