/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19951106: begun; finding the best cut
 * 19951107: more work
 *
 * Classes of questions generated:
 * N("question-word-question")
 *   At what time? / At what time is the appointment?
 *   Where? / Where is the appointment?
 *   What year? / What year is the Fiat Spyder?
 * N("question-about-alternatives")
 *   Tea or coffee? / Would you like tea or coffee?
 *   Are you meeting Amy Newton or Ruth Norville?
 *   Red or white? / A red or white Fiat Spyder?
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

/* Question */

Question *QuestionCreate(Obj *qclass, Float importance, Obj *asking_ua,
                         Obj *answer_class, ObjList *preface,
                         Obj *elided_question, Obj *full_question,
                         Obj *full_question_var, Intension *itn,
                         Question *next)
{
  Question	*q;
  q = CREATE(Question);
  q->qclass = qclass;
  q->importance = importance;
  q->asking_ua = asking_ua;
  q->answer_class = answer_class;
  q->preface = preface;
  q->elided_question = elided_question;
  q->full_question_var = full_question_var;
  if (full_question == NULL) {
    q->full_question =
      ObjSubst(full_question_var, N("?answer"), elided_question);
  } else {
    q->full_question = full_question;
  }
  q->itn = itn;
  q->next = next;
  if (DbgOn(DBGUA, DBGDETAIL)) {
    QuestionPrint(Log, q);
  }
  return(q);
}

void QuestionPrint(FILE *stream, Question *q)
{
  fprintf(stream, "Question to ask <%s> %g <%s> <%s>\n",
          M(q->qclass), q->importance, M(q->asking_ua), M(q->answer_class));
  if (q->preface) {
    fprintf(stream, "Preface:\n");
    ObjListPrint1(stream, q->preface, 0, 0, 0, 0);
  }
  if (q->elided_question) {
    fprintf(stream, "elided question ");
    ObjPrint(stream, q->elided_question);
    fputc(NEWLINE, stream);
  }
  if (q->full_question) {
    fprintf(stream, "full question ");
    ObjPrint(stream, q->full_question);
    fputc(NEWLINE, stream);
  }
  if (q->full_question_var) {
    fprintf(stream, "full question var ");
    ObjPrint(stream, q->full_question_var);
    fputc(NEWLINE, stream);
  }
  if (q->itn) IntensionPrint(stream, q->itn);
}

void QuestionPrintAll(FILE *stream, Question *questions)
{
  for (; questions; questions = questions->next) {
    QuestionPrint(stream, questions);
  }
}

Question *QuestionMostImportant(Question *questions)
{
  Float		maximportance;
  Question	*q, *maxquestion;
  maximportance = FLOATNEGINF;
  maxquestion = NULL;
  for (q = questions; q; q = q->next) {
    if (q->importance > maximportance) {
      maximportance = q->importance;
      maxquestion = q;
    }
  }
  return(maxquestion);
}

/* UA_Asker */

void UA_AskerFindBestCut(Intension *itn, ObjList *selected,
                         /* RESULTS */ ObjList **isas_r, ObjList **attrs_r)

{
  Float		attr_score, isa_score;
  ObjList	*p, *q, *objs, *attrs, *isas, *attrs_best, *isas_best;
  ObjListList	*attrs_equiv, *isas_equiv;

  /* Find attributes for cutting up data. */
  attrs = NULL;
  for (p = selected; p; p = p->next) {
    objs = DbRetrieval(itn->ts, itn->tsr,
                       L(ObjWild, p->obj, E), NULL, NULL, 1);
    /* [black WE-1020AL]
     * [atom-nickel WE-1020B]
     * ------------------------
     * [red red-1978-Fiat-1800-Spider]
     */
    for (q = objs; q; q = q->next) {
      if (!ISA(N("attribute"), I(q->obj, 0))) continue;
      if (!ObjListIn(I(q->obj, 0), attrs)) {
        attrs = ObjListCreate(I(q->obj, 0), attrs);
      }
    }
    ObjListFree(objs);
  }

  if (DbgOn(DBGUA, DBGDETAIL)) {
    fprintf(Log, "Attributes:\n");
    ObjListPrint1(Log, attrs, 0, 0, 0, 0);
  }

  attrs_equiv = ObjListPartition(attrs);
  ObjListListAttributeCount(itn->ts, itn->tsr, attrs_equiv, selected);
  attrs_best = ObjListListBestCut(attrs_equiv);

  if (DbgOn(DBGUA, DBGDETAIL)) {
    fprintf(Log, "Attributes_equiv:\n");
    ObjListListPrint(Log, attrs_equiv, 0, 1, 0);
    fprintf(Log, "attrs_best:\n");
    ObjListPrint1(Log, attrs_best, 0, 1, 0, 0);
  }

  /* Find classes for cutting up data. */
  isas = ObjCutClasses(selected);
    /* N("single-line-phone") N("multiline-phone")
     * N("manual-phone") N("dial-phone")
     * ---------------------------------------------
     * N("Convertible") N("Sport")
     */

  if (DbgOn(DBGUA, DBGDETAIL)) {
    fprintf(Log, "Isas:\n");
    ObjListPrint1(Log, isas, 0, 0, 0, 0);
  }

  isas_equiv = ObjListPartition(isas);
  ObjListListISACount(isas_equiv, selected);
  isas_best = ObjListListBestCut(isas_equiv);

  if (DbgOn(DBGUA, DBGDETAIL)) {
    fprintf(Log, "Isas_equiv:\n");
    ObjListListPrint(Log, isas_equiv, 0, 1, 0);
    fprintf(Log, "isas_best:\n");
    ObjListPrint1(Log, isas_best, 0, 1, 0, 0);
  }

  attr_score = ObjListCutScore(attrs_best);
  isa_score = ObjListCutScore(isas_best);
  if (isa_score > attr_score) {
    *isas_r = isas_best;
    *attrs_r = NULL;
  } else {
    *isas_r = NULL;
    *attrs_r = attrs_best;
  }
}

Bool UA_AskerNarrowDown(Context *cx, Float importance, Obj *asking_ua,
                        Intension *itn, ObjList *selected,
                        Obj *full_question_var, int elided_ok)
{
  int		count;
  Obj		*elided_question, *full_question;
  Obj		*answer_class;
  ObjList	*isas, *attrs, *preface;

  count = ObjListLen(selected);

  if (DbgOn(DBGUA, DBGDETAIL)) {
    fprintf(Log, "Selected %d possibilities\n", count);
    if (DbgOn(DBGUA, DBGHYPER)) ObjListPrint1(Log, selected, 0, 0, 0, 0);
  }

  if (count <= 3) {
  /* If we have a small number of possibilities, just use those as
   * alternatives.
   */
    isas = selected;
    attrs = NULL;
  } else {
    UA_AskerFindBestCut(itn, selected, &isas, &attrs);
  }

  if (isas) {
    answer_class = ObjListCommonAncestor(isas);
    elided_question = ObjListToOrObj(isas);
    full_question = ObjSubst(full_question_var, N("?answer"), elided_question);
    full_question_var =
      ObjSubst(full_question_var,
               N("?answer"),
               L(N("itn-continuation-isa"), N("?answer"), E));
    /* elided_question:   [or multiline-phone single-line-phone]
     * full_question:
     * [active-goal E [buy E na [or multiline-phone single-line-phone]]]
     * full_question_var:
     * [active-goal E [buy E na [itn-continuation-isa multiline-phone]]]
     */
  } else if (attrs) {
    answer_class = ObjListCommonAncestor(attrs);
    elided_question = ObjListToOrObj(attrs);
    full_question =
      ObjSubst(full_question_var,
               N("?answer"),
               L(elided_question, ObjListCommonAncestors(selected), E));
    full_question_var =
      ObjSubst(full_question_var,
               N("?answer"),
               L(N("itn-continuation-attr"), N("?answer"), E));
    /* elided_question:   [or blue red] "red or blue?"
     * full_question:
     * [active-goal E [buy E na [[or blue red] desk-phone]]]
     *     todo: this needs to be a such-that?
     * full_question_var:
     * [active-goal E [buy E na [itn-continuation-attr ?answer]]]
     */
  } else {
    return(0);
  }

  if (cx->dc->mode & DC_MODE_TURING) {
    /* todo: Something like "There must be N kinds of X." */
    preface = NULL;
  } else {
    preface = ObjListCreate(ThereAreXClass(count, N("possibility")), NULL);
  }
  if (!(cx->dc->mode & DC_MODE_CONV)) {
    Dbg(DBGUA, DBGBAD, "UA_AskerNarrowDown");
  }
  cx->questions = QuestionCreate(N("question-about-alternatives"), importance,
                                 asking_ua, answer_class, preface,
                                 elided_ok ? elided_question : NULL,
                                 full_question, full_question_var, itn,
                                 cx->questions);
  return(1);
}

ObjList *UA_AskerSelect(Context *cx, Obj *obj, /* RESULTS */ Intension **itn_r)
{
  ObjList	*props, *r;
  Intension	*itn;

  if (cx->last_question && cx->last_question->itn &&
      ISA(N("itn-continuation"), I(obj, 0))) {
  /* Continuation of existing query. */
    itn = cx->last_question->itn;
    if (ISA(N("itn-continuation-isa"), I(obj, 0))) {
    /* [itn-continuation-isa [or multiline-phone DTMF-phone]] */
      itn->isas = ObjListListCreate(OrObjToObjList(I(obj, 1)), itn->isas);
    } else if (ISA(N("itn-continuation-attr"), I(obj, 0))) {
      itn->attributes = ObjListListCreate(OrObjToObjList(I(obj, 1)),
                                          itn->attributes);
    } else {
      Dbg(DBGUA, DBGBAD, "UA_AskerSelect");
      return(NULL);
    }
    *itn_r = itn;
    return(DbFindIntension(itn, NULL));
  } else if ((props = ObjIntensionParse(obj, 1, &obj))) {
  /* New query based on propositions. */
    itn = IntensionCreate();
    itn->class = obj;
    itn->props = props;
    *itn_r = itn;
    return(DbFindIntension(itn, NULL));
  } else if (!ObjIsList(obj)) {
  /* New query based on class. */
    itn = IntensionCreate();
    itn->class = obj;
    *itn_r = itn;
    if ((r = ObjDescendants(obj, OBJTYPEANY))) {
      return(r);
    } else {
      return(ObjListCreate(obj, NULL));
    }
  } else {
  /* Something else. Not understood. */
    *itn_r = NULL;
    return(NULL);
  }
}

#ifdef notdef
   /* This is currently coded separately in each UA. */
ObjList *UA_AskerSelectAndNarrowDown(Context *cx, Float importance,
                                     Obj *asking_ua, Obj *obj, int threshold,
                                     Obj *full_question_var)
{
  Intension	*itn;
  ObjList	*selected;

  selected = UA_AskerSelect(cx, obj, &itn);

  if (ObjListLen(selected) <= threshold) {
  /* Successfully narrowed down to <threshold> or less possibilities. */
    return(selected);
  }

  /* Attempt to narrow down; a continuation will bring us back here if a
   * matching answer is given.
   */
  UA_AskerNarrowDown(cx, importance, asking_ua, itn, selected,
                     full_question_var, 1);
  return(NULL);
}
#endif

void UA_AskerQWQ(Context *cx, Float importance, Obj *asking_ua,
                 Obj *answer_class, Obj *elided_question, Obj *full_question,
                 Obj *full_question_var)
{
  if (!(cx->dc->mode & DC_MODE_CONV)) {
    Dbg(DBGUA, DBGBAD, "UA_AskerQWQ");
  }
  cx->questions = QuestionCreate(N("question-word-question"), importance,
                                 asking_ua, answer_class, NULL, elided_question,
                                 full_question, full_question_var,
                                 NULL, cx->questions);
}

void UA_Asker(Context *cx, Discourse *dc)
{
  UA_NameRequest(cx, dc);
}

Obj *UA_AskerInterpretElided(Context *parent_cx, Obj *elided)
{
  Obj	*obj;
  if (parent_cx->last_question &&
      ISAP(parent_cx->last_question->answer_class, elided)) {
  /* Understand as elided question answer. (Unelided answers are
   * understood as normal statements).
   */
    obj = ObjSubst(parent_cx->last_question->full_question_var,
                   N("?answer"), elided);
    if (DbgOn(DBGUA, DBGDETAIL)) {
      Dbg(DBGUA, DBGDETAIL, "elided answer expanded to ");
      ObjPrint(Log, obj);
      fputc(NEWLINE, Log);
    }
    return(obj);
  }
  return(NULL);
}

void UA_AskerAskQuestions(Discourse *dc)
{
  Question	*q;
  Context	*cx;

  if (dc->cx_best->questions == NULL) return;
  if (NULL == (q = QuestionMostImportant(dc->cx_best->questions))) return;

  Dbg(DBGUA, DBGDETAIL, "ASKING QUESTION <%s> <%s>", M(q->asking_ua),
      M(q->qclass));

  DiscourseEchoInput(dc);

  if (q->preface) {
    ObjListPrettyPrint(Log, q->preface);
    DiscourseGenParagraph(dc, "", q->preface, 0, 0);
  }

  if (q->elided_question) {
    ObjPrettyPrint(Log, q->elided_question);
    DiscourseGen(dc, 0, "", q->elided_question, '?');
  } else if (q->full_question) {
    ObjPrettyPrint(Log, q->full_question);
    DiscourseGen(dc, 0, "", q->full_question, '?');
  } else {
    Dbg(DBGUA, DBGBAD, "UA_AskerAskQuestions");
    return;
  }

  /* todo: Go back to unanswered questions? Or will these get
   * regenerated by the UAs?
   * Questions are stored in contexts, because a question could
   * have multiple interpretations depending on the context
   * (overdetermination). This feature is not yet used though.
   */
  q->next = NULL;
  for (cx = dc->cx_alterns; cx; cx = cx->next) {
    cx->questions = NULL;	/* todoFREE: all but q */
    cx->last_question = q;
  }
}

/* End of file. */
