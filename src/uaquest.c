/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940705: begun
 * 19940721: added Answer objects
 * 19940901: added temporal relation questions
 * 19941201: adding determining questions
 * 19950402: changed name to UA and other cosmetic changes
 * 19950529: explanations
 * 19950605: more work
 * 19950731: plenty of work and uniformizing
 * 19951024: more work
 * 19981207: duration-of mods
 *
 * todo:
 * - "Do you understand the question?" => Regenerate previous question concept.
 * - "What color is a rose?" => Look at descendants and list different kinds
 *   of roses and their colors.
 * - "Where are TV sets usually found?" /"Where might I find a TV set?" =>
 *   "In the living room, bedroom."
 * - "Do you know of any books about the Internet?" => Use topic-of relation.
 * - "Does person A know person B?" => Infer from friends-of, famous, and so on.
 * - "Please give me information on X" => Quote from source material.
 * - "How long does it take to take a shower?"
 * - "Who is the President of France?" after having already generated the
 *   answer => "I just told you. Jacques Chirac." cf "As I was saying..."
 *   "Anyway..."
 * - "What is the verb in the last sentence?" => Answer from the saved
 *   understanding parse tree.
 * - Elided followup questions similar to explanation-request: "When?"
 *   "Exactly?" "Exactly how big?"
 * - For DC_MODE_TURING, answers should always be short.
 * - BBrain should stop QA_Question working on answers if a high
 *   sense answer is already found or if enough answers are found.
 *   cf "Who is extension-of{the President of the US}" which gets
 *   bogged down in lots of UA_QuestionDescribes.
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
#include "repprove.h"
#include "repspace.h"
#include "repstr.h"
#include "reptext.h"
#include "reptime.h"
#include "semanaph.h"
#include "semaspec.h"
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

Bool SenseIsValid(Float sense)
{
  return((sense == 0.0 || sense >= .000000001)
         && sense <= 2.0);
}

void senc(Float sense)
{
  if (!SenseIsValid(sense)) {
    Dbg(DBGUA, DBGBAD, "bad sense");
    Stop();
  }
}

/* Answer */

Answer *AnswerCreate(Obj *ua, Obj *question, Obj *qclass, Float sense,
                     ObjList *answer, int sort_by_ts, int gen_pairs,
                     int gen_tsr, TsrGenAdvice *tga, Answer *next)
{
  int		i;
  Answer	*an;
  an = CREATE(Answer);
  an->ua = ua;
  if (!ISA(N("UA"), ua)) ObjAddIsa(ua, N("UA"));
  an->question = question;
  an->qclass = qclass;
  an->sense = sense;
  an->answer = ObjListReverseDest(ObjListUniquifyDest(answer));
  an->rephrased_answer = NULL;
  an->eos = '.';
  for (i = 0; i < DCMAX; i++) an->answer_text[i] = NULL;
  an->sort_by_ts = sort_by_ts;
  an->gen_pairs = gen_pairs;
  an->gen_tsr = gen_tsr;
  if (tga) an->tga = *tga;
  else TsrGenAdviceClear(&an->tga);
  an->next = next;
  return(an);
}

Answer *AnswerCreateQWQ(Obj *ua, Obj *question, Float sense,
                        ObjList *answer, Answer *next)
{
  return(AnswerCreate(ua, question, N("question-word-question"), sense,
                      answer, 0, 0, 0, NULL, next));
}

void AnswerFree(Answer *an)
{
  int	i;
  ObjListFree(an->answer);
  for (i = 0; i < DCMAX; i++) {
    if (an->answer_text[i]) TextFree(an->answer_text[i]);
  }
  MemFree(an, "Answer");
}

void AnswerFreeAll(Answer *an)
{
  Answer	*n;
  while (an) {
    n = an->next;
    AnswerFree(an);
    an = n;
  }
}

Answer *AnswerLast(Answer *an)
{
  while (an->next) an = an->next;
  return(an);
}

Answer *AnswerAppendDestructive(Answer *an1, Answer *an2)
{
  if (an1) {
    AnswerLast(an1)->next = an2;
    return(an1);
  } else return(an2);
}

Float AnswerMaxSense(Answer *an)
{
  Float	maxsense;
  maxsense = SENSE_NONE;
  while (an) {
    if (an->sense > maxsense) maxsense = an->sense;
    an = an->next;
  }
  return(maxsense);
}

Float ObjGenSpecificity(Obj *obj)
{
  int	i, len;
  Float	numer, denom;
  Obj	*elem;

  FloatAvgInit(&numer, &denom);
  numer = 0.0;
  for (i = 0, len = ObjLen(obj); i < len; i++) {
    if (ObjIsList(elem = I(obj, i))) {
      FloatAvgAdd(ObjGenSpecificity(elem), &numer, &denom);
    } else {
      if (elem->ole) {
        FloatAvgAdd(1.0, &numer, &denom);
      } else if (ObjIsString(elem) ||
                 ObjIsNumber(elem) ||
                 ObjIsTsRange(elem)) {
        FloatAvgAdd(1.0, &numer, &denom);
      } else {
        FloatAvgAdd(0.0, &numer, &denom);
      }
    }
  }
  return(FloatAvgResult(numer, denom, 1.0));
}

Float ObjListGenSpecificity(ObjList *objs)
{
  Float	numer, denom;
  ObjList	*p;

  FloatAvgInit(&numer, &denom);
  for (p = objs; p; p = p->next) {
    FloatAvgAdd(ObjGenSpecificity(p->obj), &numer, &denom);
  }
  return(FloatAvgResult(numer, denom, 1.0));
}

void AnswerAdjustSense(Answer *an)
{
  while (an) {
    if (an->answer) {
      an->sense = an->sense * ObjListGenSpecificity(an->answer);
    } else {
    /* If we are going to penalize object answers, we should
     * also penalize text answers.
     * cf What is her occupation? UA_QuestionPronoun vs. UA_QuestionDescribe
     */
      an->sense = an->sense * 0.9;
    }
    an = an->next;
  }
}

Answer *AnswerRemoveBelowThresh(Answer *an, Float thresh)
{
  Answer	*p, *prev;
  prev = NULL;
  for (p = an; p; p = p->next) {
    if (p->sense < thresh) {
      /* Splice out. */
      if (DbgOn(DBGUA, DBGDETAIL)) {
        Dbg(DBGUA, DBGDETAIL, "an with sense %g below threshold %g removed:",
            an->sense, thresh);
        ObjListPrettyPrint(Log, an->answer);
      }
      if (prev) {
        prev->next = p->next;
        AnswerFree(p);
      } else an = p->next;
    } else prev = p;
  }
  return(an);
}

/* ANSWER GENERATION */

ObjList	*Answers;

void AnswerGenInit()
{
  Answers = NULL;
}

void AnswerGenAdd(ObjList *answers)
{
  Answers = ObjListAppendDestructive(Answers, answers);
}

void AnswerGenEnd()
{
  ObjListFree(Answers);
  Answers = NULL;
}

void AnswerGen(Answer *an, Discourse *dc)
{
  int			i, is_text;
  ObjList		*objs;
  GenAdvice		save_ga;
  if (an->answer) {
    objs = ObjListReverseDest(ObjListSubtractSimilar(an->answer, Answers));
  } else {
    objs = NULL;
  }
  is_text = 0;
  for (i = 0; i < DCMAX; i++) {
    if (an->answer_text[i]) is_text = 1;
  }
  if ((!is_text) && objs == NULL) {
    Dbg(DBGUA, DBGDETAIL, "skip empty or duplicate answer");
    return;
  }

  /* Generate question. */
  Dbg(DBGUA, DBGDETAIL, "QUESTION <%s> <%s>", M(an->ua), M(an->qclass));

  if (an->question) {
    save_ga = dc->ga;
    if (an->qclass == N("question-word-question") ||
        an->qclass == N("canned-question")) {
      dc->ga.qwq = 1;
    }
    ObjPrettyPrint(Log, an->question);
/* This is now done in DiscourseObjRep, so it should not also be
   done here.
    DiscourseFlipListenerSpeaker(dc);
    DiscourseGen(dc, 1, ") ", an->question, '?');
    DiscourseFlipListenerSpeaker(dc);
 */
    dc->ga = save_ga;
  }

  DiscourseEchoInput(dc);

  /* output text answers */
  Dbg(DBGUA, DBGDETAIL, "ANSWER sense %g", an->sense);
  for (i = 0; i < DCMAX; i++) {
    if (an->answer_text[i]) {
      TextPrint(DCI(dc, i).stream, an->answer_text[i]);
      /* The text should already contain a newline.
         cf DictObj
      fputc(NEWLINE, DCI(dc, i).stream);
       */
    }
  }

  save_ga = dc->ga;

  /* Generate object answers. */
  if (objs) {
    ObjListPrint(Log, objs);
    dc->ga.gen_tsr = an->gen_tsr;
    dc->ga.tga = an->tga;
    if (an->eos == '?' && ObjListIsLengthOne(objs)) {
      DiscourseGen(dc, 0, "", objs->obj, an->eos);
    } else {
      DiscourseGenParagraph(dc, "", objs, an->sort_by_ts, an->gen_pairs);
    }
    AnswerGenAdd(objs);
  }

  dc->ga = save_ga;
}

void AnswerGenAll(Answer *an, Discourse *dc)
{
  Answer	*a;
  for (a = an; a; a = a->next) {
    AnswerGen(a, dc);
  }
}

void AnswerPrint(FILE *stream, Answer *an, Discourse *dc)
{
  int		i;
  Channel	*ch;
  fprintf(stream, "Answer <%s> <%s> sense %g\nQ: ", M(an->ua),
          M(an->qclass), an->sense);
  ObjPrint(stream, an->question);
  fputc(NEWLINE, stream);
  fputs("A: ", stream);
  for (i = 0; i < DCMAX; i++) {
    if (NULL == (ch = DiscourseGetIthChannel(dc, i))) continue;
    if (an->answer_text[i]) TextPrint(stream, an->answer_text[i]);
  }
  ObjListPrint(Log, an->answer);
}

void AnswerPrintAll(FILE *stream, Answer *an, Discourse *dc)
{
  Answer	*a;
  for (a = an; a; a = a->next) {
    AnswerPrint(stream, a, dc);
  }
}

/* HELPING FUNCTIONS */

Obj *UA_Question_IsIntAdverb(Obj *question, Obj *adverb_class,
                             /* RESULTS */ Obj **adverb)
{
  /* todo: Handle multiple interrogative adverbs as in
   * "Why and how did you ...".
   */
  if (ISA(adverb_class, I(question, 0))) {
    if (adverb) *adverb = I(question, 0);
    return(I(question, 1));
  }
  return(NULL);
}

int IsDescriptionCopula(Obj *obj)
{
  return(ISA(N("copula"), obj) || ISA(N("meaning-of"), obj));
}

Obj *ObjListDeref(Obj *obj)
{
  if (ObjIsList(obj) && ObjLen(obj) == 1) {
    return(I(obj, 0));
  } else {
    return(obj);
  }
}

Obj *UA_Question_IsDescription(Obj *question, Obj *pronoun_class,
                               Obj *obj_class)
{
  Obj	*r;
  if (IsDescriptionCopula(I(question, 0)) &&
      ((ISA(pronoun_class, I(question, 1)) &&
        (r = ObjListDeref(I(question, 2))) &&
        ISAP(obj_class, r)) ||
       ((r = ObjListDeref(I(question, 1))) &&
        ISAP(obj_class, r) &&
        ISA(pronoun_class, I(question, 2))))) {
    if (ObjIsList(r) || ObjIsNumber(r)) {
    /* Rule out [such-that ... ]. */
      return(NULL);
    } else {
      return(r);
    }
  }
  return(NULL);
}

Obj *UA_Question_IsInterrogative(Obj *question, /* RESULTS */
                                 Obj **pronoun)
{
  if (ISA(N("copula"), I(question, 0))) {
    if (ISA(N("interrogative-pronoun"), I(question, 1))) {
      *pronoun = I(question, 1);
      return(I(question, 2));
    } else if (ISA(N("interrogative-pronoun"), I(question, 2))) {
      *pronoun = I(question, 2);
      return(I(question, 1));
    }
  }
  *pronoun = NULL;
  return(NULL);
}

/* ADVERB QUESTIONS */

Answer *UA_QuestionLocationAdverb(Obj *question, TsRange *tsr, Answer *an,
                                  Discourse *dc)
{
  Obj		*adverb_q;
  ObjList	*p, *objs, *answer;
  answer = NULL;
  if ((adverb_q = UA_Question_IsIntAdverb(question,
                                          N("location-interrogative-adverb"),
                                          NULL))) {
    for (p = objs = ProveRetrieve(NULL, tsr, adverb_q, 0); p; p = p->next) {
      if (ISA(N("physical-object"), I(p->obj, 1))) {
        answer = SpaceFindEnclose(NULL, ObjToTsRange(p->obj), I(p->obj, 1),
                                  NULL, answer);
      } else if (ISA(N("physical-object"), I(p->obj, 2))) {
        answer = SpaceFindEnclose(NULL, ObjToTsRange(p->obj), I(p->obj, 2),
                                  NULL, answer);
      }
    }
    ObjListFree(objs);
  }
  if (ISA(N("copula"), I(question, 0)) &&
      I(question, 1) == N("location-interrogative-adverb")) {
    Dbg(DBGUA, DBGBAD, "UA_QuestionLocationAdverb");
  }
  if (answer) {
    an = AnswerCreateQWQ(N("UA_QuestionLocationAdverb"), question, SENSE_TOTAL,
                         answer, an);
  }
  return(an);
}

/* todo: Can also respond "after" or "before" another event in the episode,
 * except when question is time-of-day-interrogative-adverb.
 */
Answer *UA_QuestionTimeAdverb(Obj *question, TsRange *tsr, Answer *an,
                              Discourse *dc)
{
  Obj			*adverb_q, *adverb;
  ObjList		*answer;
  TsrGenAdvice	tga;
  if (!(adverb_q =
        UA_Question_IsIntAdverb(question,
                                N("absolute-time-interrogative-adverb"),
                                &adverb))) {
    return(an);
  }
  if ((answer = ProveRetrieve(NULL, tsr, adverb_q, 0))) {
    TsrGenAdviceGet(adverb, dc, &tga, NULL, NULL);
    an = AnswerCreate(N("UA_QuestionTimeAdverb"), question,
                      N("question-word-question"), SENSE_TOTAL,
                      answer, 0, 0, 1, &tga, an);
  }
  return(an);
}

Obj *UA_Question_SpaceToNearReachable(Obj *subgoal)
{
  if (N("ptrans") == I(subgoal, 0) &&
      ObjNA == I(subgoal, 2) &&
      NotNA(I(subgoal, 3))) {
    return(L(N("near-reachable"), I(subgoal, 1), I(subgoal, 3), E));
  }
  return(subgoal);
}

Obj *UA_Question_SpaceToPtrans(Obj *subgoal)
{
  if (N("near-reachable") == I(subgoal, 0) &&
      ISA(N("human"), I(subgoal, 1)) &&
      NotNA(I(subgoal, 2))) {
    return(L(N("ptrans"), I(subgoal, 1), ObjNA, I(subgoal, 2), E));
  }
  return(subgoal);
}

#ifdef notdef
ObjList *UA_Question_TripToActions(Trip *trip)
{
  ObjList	*r;
  TripLeg	*tl;
  r = NULL;
  for (tl = trip->legs; tl; tl = tl->next) {
    if (tl->wormhole) continue;
    if (tl->action == N("grid-walk") && tl->transporter) {
      r = ObjListCreate(L(N("ptrans-walk"), tl->transporter, tl->from,
                          tl->to, E), r);
    } else if (tl->action == N("grid-drive-car") && tl->driver) {
      r = ObjListCreate(L(N("ptrans-drive"), tl->driver, tl->from, tl->to,
                          E), r);
    } else if (tl->action == N("warp")) {
      /* do nothing */
    } else {
      Dbg(DBGUA, DBGBAD, "UA_Question_TripToActions: unknown action type <%s>",
          M(tl->action));
    }
  }
  return(ObjListReverseDest(r));
}
#endif

Answer *UA_QuestionMeansPlan(Obj *question, Obj *subgoal, TsRange *tsr,
                             Answer *an, Discourse *dc)
{
  ObjList	*subgoals, *objs, *p;
  Context	*cx;
  for (cx = dc->cx_alterns; cx; cx = cx->next) {
    objs = ContextFindMatchingSubgoals(cx, subgoal);
    for (p = objs; p; p = p->next) {
      if ((subgoals = ContextSubgoals(cx, p->obj))) {
        an = AnswerCreateQWQ(N("UA_QuestionMeansPlan"), question, SENSE_TOTAL,
                             subgoals, an);
      }
    }
    ObjListFree(objs);
  }
  return(an);
}

Answer *UA_QuestionMeansAdverb(Obj *question, TsRange *tsr, Answer *an,
                               Discourse *dc)
{
  Obj		*subgoal;
  if ((subgoal = UA_Question_IsIntAdverb(question,
                                         N("means-interrogative-adverb"),
                                         NULL))) {
    an = UA_QuestionMeansPlan(question,
                              UA_Question_SpaceToNearReachable(subgoal),
                              tsr, an, dc);
  }
  return(an);
}

Answer *UA_QuestionReasonAdverb(Obj *question, TsRange *tsr, Answer *an,
                                Discourse *dc)
{
  Obj		*ptn, *supergoal;
  ObjList	*objs, *objs2, *answer, *p, *q;
  Context	*cx;
  answer = NULL;
  if ((ptn = UA_Question_IsIntAdverb(question, N("reason-interrogative-adverb"),
                                     NULL))) {
    /* todo: If human in ptn. */
    if ((objs = ProveRetrieve(NULL, tsr, ptn, 0))) {
      for (p = objs; p; p = p->next) {
/* todo: Use best context. */
        for (cx = dc->cx_alterns; cx; cx = cx->next) {
          objs2 = ContextFindMatchingSubgoals(cx, p->obj);
          for (q = objs2; q; q = q->next) {
            if ((supergoal = ContextSupergoal(cx, q->obj))) {
              answer = ObjListCreate(L(N("since"), supergoal, p->obj, E),
                                       answer);
            } else {
              answer = ObjListCreate(N("adverb-of-empty-reason"), answer);
              /* Or could also generate "active-goal". */
            }
          }
          ObjListFree(objs2);
        }
      }
    }
    /* todo: Otherwise if not human, look for subgoals. */
  }
  if (answer) {
    an = AnswerCreateQWQ(N("UA_QuestionReasonAdverb"), question, SENSE_TOTAL,
                         answer, an);
  }
  return(an);
}

Answer *UA_QuestionAppearanceAdverb(Obj *question, TsRange *tsr, Answer *an,
                                    Discourse *dc)
{
  return(an);
}

Answer *UA_QuestionMannerAdverb(Obj *question, TsRange *tsr, Answer *an,
                                Discourse *dc)
{
  return(an);
}

Answer *UA_QuestionQuantityAdverb(Obj *question, TsRange *tsr, Answer *an,
                                  Discourse *dc)
{
  return(an);
}

/* "How big is X?"
 * [degree-interrogative-adverb [big X]]
 * todo: Yes-No questions in a similar vein. Could also be implemented if
 * system forward inferred attributes from relations.
 */
Answer *UA_QuestionDegreeAdverb(Obj *question, TsRange *tsr, Answer *an,
                                Discourse *dc)
{
  Obj		*attr, *answer_obj;
  ObjList	*inferences, *p, *objs, *q, *reasons, *answer, *related;
  Float		numer, denom, weight;
  if ((attr = UA_Question_IsIntAdverb(question,
                                      N("degree-interrogative-adverb"),
                                      NULL))) {
    /* attr: [big X] */
    FloatAvgInit(&numer, &denom);
    inferences = RAD(&TsNA, L(N("attr-rel"), I(attr, 0), E), 1, 0);
    reasons = related = NULL;
    for (p = inferences; p; p = p->next) {
      /* [attr-rel-proportional big height-of]
       */
      objs = DbGetRelationAssertions(NULL, tsr, I(p->obj, 2), I(attr, 1), 1,
                                     NULL);
      if (ISA(N("attr-rel-value"), I(p->obj, 0))) {
      /* p->obj: [attr-rel-value scholarly diploma-of 0.6u Masters-degree] */
        for (q = objs; q; q = q->next) {
        /* q->obj: [diploma-of X Masters-degree] */
          if (I(q->obj, 2) == I(p->obj, 4)) {
            FloatAvgAdd(ObjToNumber(I(p->obj, 3)), &numer, &denom);
            reasons = ObjListCreate(q->obj, reasons);
          }
        }
      } else if (ISA(N("attr-rel-range"), I(p->obj, 0))) {
      /* p->obj: [attr-rel-range tall height-of -0.2u +0.2u 1.6m 1.8m]
       *         [attr-rel-range tall height-of +0.2u +Inf  1.8m +Inf]
       */
        for (q = objs; q; q = q->next) {
        /* q->obj: [height-of X 1.7m] */
        /* todo: This needs work. Unless we want to do more database entering,
         * it would be better to scale here.
         */
          if (ObjToNumber(I(q->obj, 2)) >= ObjToNumber(I(p->obj, 5)) &&
              ObjToNumber(I(q->obj, 2)) < ObjToNumber(I(p->obj, 6))) {
            if (FLOATNA != (weight = FloatAvgMinMax(ObjToNumber(I(p->obj, 3)),
                                                 ObjToNumber(I(p->obj, 4)),
                                                 -1.0, 1.0))) {
              FloatAvgAdd(weight, &numer, &denom);
              reasons = ObjListCreate(q->obj, reasons);
            } else {
              related = ObjListCreate(q->obj, related);
            }
          }
        }
      } else if (ISA(N("attr-rel-proportional"), I(p->obj, 0))) {
        for (q = objs; q; q = q->next) {
          related = ObjListCreate(q->obj, related);
        }
      }
      ObjListFree(objs);
    }

    if (FLOATNA != (weight = FloatAvgResult(numer, denom, FLOATNA))) {
      if (dc->exactly) {
      /* "The height of X is 1.9m." */
        an = AnswerCreateQWQ(N("UA_QuestionDegreeAdverb"), question,
                             SENSE_TOTAL, reasons, an);
        ObjListFree(related);
      } else {
      /* "X is very tall." explanation-request->"Because the height of X is
       * 1.9m."
       */
        answer_obj = L(I(attr, 0), I(attr,1), NumberToObj(weight), E);
        ObjSetJustification(answer_obj, reasons);
        answer = ObjListCreate(answer_obj, NULL);
        an = AnswerCreateQWQ(N("UA_QuestionDegreeAdverb"), question,
                             SENSE_TOTAL-.1, answer, an);
             /* the -1 serves to give dc->exactly priority */
        ObjListFree(related);
      }
    } else {
    /* "The height of X is 1.7 meters."
     * "X has a Master's degree."
     */
      if (related) {
        an = AnswerCreateQWQ(N("UA_QuestionDegreeAdverb"), question,
                             SENSE_TOTAL, related, an);
      }
      ObjListFree(reasons);
    }
  }
  return(an);
}

/* PRONOUN QUESTIONS */

Answer *UA_QuestionWeather2(Obj *question, TsRange *tsr, Answer *an,
                            Obj *grid, Discourse *dc)
{
  ObjList	*answer0, *answer, *p;
  if ((answer0 = RDR(tsr, L(N("weather"), grid, E), 0))) {
    answer = NULL;
    for (p = answer0; p; p = p->next) {
      answer = ObjListCreate(L(I(p->obj, 0), N("pronoun-it-expletive"), E),
                             answer);
      ObjSetTsRange(answer->obj, ObjToTsRange(p->obj));
    }
    ObjListFree(answer0);
    an = AnswerCreateQWQ(N("UA_QuestionWeather2"), question,
                         SENSE_TOTAL, answer, an);
  }
  return(an);
}

Answer *UA_QuestionWeather1(Obj *question, Context *cx, TsRange *tsr,
                            Answer *an, Discourse *dc)
{
  ObjList	*grids, *p;
  if ((grids = ContextFindGrids(cx))) {
    for (p = grids; p; p = p->next) {
      an = UA_QuestionWeather2(question, tsr, an, p->obj, dc);
    }
  }
  return(an);
}

Answer *UA_QuestionWeather(Obj *question, Context *cx, TsRange *tsr, Answer *an,
                           Discourse *dc)
{
  if (ISA(N("copula"), I(question, 0)) &&
      ISA(N("manner-interrogative-pronoun"), I(question, 1)) &&
      ISA(N("weather"), I(question, 2))) {
    return(UA_QuestionWeather1(question, cx, tsr, an, dc));
  }
  if (ISA(N("copula"), I(question, 0)) &&
      ISA(N("weather"), I(question, 1)) &&
      ISA(N("manner-interrogative-pronoun"), I(question, 2))) {
    return(UA_QuestionWeather1(question, cx, tsr, an, dc));
  }
  return(an);
}

/* Caller must initialize *found to 0. */
Obj *ObjBuildQueryPtn(Obj *obj, /* RESULTS */ int *found)
{
  int 	i, len;
  Obj	*elems[MAXLISTLEN];
  if (ISA(N("interrogative-pronoun"), obj)) {
    *found = 1;
    if (ISA(N("object-interrogative-pronoun"), obj)) return(N("?nonhuman"));
    else if (ISA(N("human-interrogative-pronoun"), obj)) return(N("?human"));
    else if (ISA(N("action-interrogative-pronoun"), obj)) return(N("?action"));
    else if (ISA(N("attribute-interrogative-pronoun"), obj)) {
      return(N("?attribute"));
    } else {
      return(ObjWild); /* todo: better restrictions for location, etc.? */
    }
  }
  if (!ObjIsList(obj)) return(obj);
  if (ObjLen(obj) == 1 && ISA(N("action"), I(obj, 0))) {
    return(I(obj, 0));
  }
  if (N("question-element") == I(obj, 0)) {
  /* todo: Add propositions of the question-element as further constraints. */
    *found = 1;
    return(ObjClassToVar(I(obj, 1)));
  }
  for (i = 0, len = ObjLen(obj); i < len; i++) {
    elems[i] = ObjBuildQueryPtn(I(obj, i), found);
  }
  return(ObjCreateList1(elems, len));
}

/* As contrasted with UA_QuestionElementTop, this returns a complete
 * sentence including the queried object.
 *
 * "You told her what?"
 * [mtrans TT Sarah object-interrogative-pronoun]
 *
 * "What is the circumference of the Earth?"
 * [circumference-of Earth determining-interrogative-pronoun]
 *
 * "Who created Bugs Bunny?"
 * [create human-interrogative-pronoun Bugs-Bunny]
 *
 * Also handles embedded N("question-element").
 */
Answer *UA_QuestionPronoun(Obj *question, TsRange *tsr, Answer *an,
                           Discourse *dc)
{
  int		found;
  Obj		*ptn;
  ObjList	*answer;
  TsrGenAdvice	tga;

  if (!ObjIsList(question)) {
    return(an);
  }
  found = 0;
  ptn = ObjBuildQueryPtn(question, &found);

  if (DbgOn(DBGUA, DBGDETAIL)) {
    fputs("UA_QuestionPronoun: query pattern: ", Log);
    ObjPrint(Log, ptn);
    fputc(NEWLINE, Log);
  }
  
  if (!found) {
   ObjFree(ptn);
   return(an);
  }
  answer = ProveRetrieve(NULL, tsr, ptn, 1);
  if (answer) {
    TsrGenAdviceSet(&tga);
    an = AnswerCreate(N("UA_QuestionPronoun"), question,
                      N("question-word-question"),
                      SENSE_TOTAL, answer, 0, 0, !ObjListIsLengthOne(answer),
                      &tga, an);
  }
  return(an);
}

Bool UA_Question_IsLocationQuestion(Obj *question, /* RESULTS */ Obj **object)
{
  if (N("location-of") == I(question, 0)) {
    if (ISA(N("location-interrogative-pronoun"), I(question, 2))) {
      *object = I(question, 1);
      return(1);
    }
    if (ISA(N("location-interrogative-pronoun"), I(question, 1)) &&
        I(question, 2)) {
      *object = I(question, 2);
      return(1);
    }
  }
  return(0);
}

/* todo: The routines answering location questions could use some merging. */
Answer *UA_QuestionLocationPronoun(Obj *question, TsRange *tsr, Answer *an,
                                   Discourse *dc)
{
  Obj		*obj;
  ObjList	*answer, *answer1;
  /* Note the case of "Where is the TV set?" should be handled by location-of,
   * not by a copula.
   */
  if (UA_Question_IsLocationQuestion(question, &obj)) {
    Dbg(DBGUA, DBGDETAIL, "UA_QuestionLocationPronoun: question recognized");
    TsRangePrint(Log, tsr);
    fputc(NEWLINE, Log);
    if ((answer = SpaceFindEnclose(NULL, tsr, obj, NULL, NULL))) {
    /* Only give most specific answer. cf "Jim is where?" */
      answer1 = ObjListCreate(answer->obj, NULL);
      ObjListFree(answer);
      an = AnswerCreateQWQ(N("UA_QuestionLocationPronoun"), question,
                           SENSE_TOTAL, answer1, an);
    }
  }
  return(an);
}

Answer *UA_QuestionLocation(Obj *question, Obj *object,
                            Obj *enclose_restriction, TsRange *tsr,
                            Answer *an, Discourse *dc)
{
  ObjList	*answer, *answer1;
  if ((answer = SpaceFindEnclose(NULL, tsr, object, enclose_restriction,
                                 NULL))) {
  /* Only give most specific answer. */
    answer1 = ObjListCreate(answer->obj, NULL);
    ObjListFree(answer);
    an = AnswerCreateQWQ(N("UA_QuestionLocation"), question, SENSE_TOTAL,
                         answer1, an);
  }
  return(an);
}

/* DETERMINER QUESTIONS */

/* As contrasted with UA_QuestionPronoun, this returns the object matching
 * the query---not a complete sentence.
 *
 * [question-element name object-interrogative-pronoun [name-of TT name]]
 * "What is the heaviest element?"
 * [question-element atom object-interrogative-pronoun
 *                        [atomic-weight-of atom maximal]]
 *
 * "Quel est l'élément chimique le plus dense ?"
 * (WAS: [question-element atom determining-interrogative-pronoun
 *                        [atomic-weight-of atom maximal]])
 * [question-element atom object-interrogative-pronoun
 *                        [dense atom maximal]]
 *
 * [question-element object object-interrogative-pronoun
 *                          [height-of Jim object]]
 *
 */
Answer *UA_QuestionElementTop1(Obj *question, TsRange *tsr, Answer *an,
                               Discourse *dc)
{
  Obj		*class, *pronoun;
  ObjList	*props, *answer;
  Intension	itn;
  if (ObjQuestionElementParse(question, &class, &pronoun, &props) &&
      (pronoun != N("location-interrogative-pronoun"))) {
  /* class: atom
   * pronoun: object-interrogative-pronoun
   * props: [atomic-weight-of atom maximal]
   */
#ifdef notdef
    /* I really think this is not needed: */
    if (ISA(N("object-interrogative-pronoun"), pronoun)
        && (!ISADeep(N("adverb-of-superlative-degree"), question))) {
      return(an);
    }
#endif
    IntensionInit(&itn);
    itn.ts = NULL;
    itn.tsr = tsr;
    itn.class = class;
    itn.props = props;
    if ((answer = DbFindIntension(&itn, NULL))) {
      return(AnswerCreateQWQ(N("UA_QuestionElementTop"), question, SENSE_TOTAL,
                             answer, an));
    }
  }
  return(an);
}

Answer *UA_QuestionElementTop(Obj *question, TsRange *tsr, Answer *an,
                              Discourse *dc)
{
  Obj		*suchthat, *from, *pronoun;
  ObjList	*props;
  if ((suchthat = UA_Question_IsInterrogative(question, &pronoun)) &&
      (props = ObjIntensionParse(suchthat, 0, &from)) &&
      ObjIsAbstract(from)) {
  /* "What is your name?"
   * question: [standard-copula object-interrogative-pronoun
   *            [such-that given-name [first-name-of TT given-name]]]
   * props:  [first-name-of TT name]
   * from:   given-name
   * mapped to: [question-element given-name
   *             object-interrogative-pronoun
   *             [first-name-of TT name]]
   *
   * "Who is the President of the US?"
   * question: [standard-copula human-interrogative-pronoun
   *            [such-that human [President-of country-USA human]]]
   *
   * too general:
   * question: [standard-copula object-interrogative-pronoun
   *            [such-that object [height-of Jim object]]
   *
   * "Who is Noam Chomsky?" is not handled here---NAME-of should be
   * eliminated by Sem_Anaphora:
   * question: [standard-copula human-interrogative-pronoun
   *            [such-that human [NAME-of human NAME:"Noam Chomsky"]]]
   */
    return(UA_QuestionElementTop1(ObjQuestionElement(from, pronoun, props),
                                  tsr, an, dc));
  }
  /* todo: Is this case still used? */
  return(UA_QuestionElementTop1(question, tsr, an, dc));
}

/* "How many breads are there?"
 * [standard-copula [question-element bread interrogative-quantity-determiner]
 *                  pronoun-there-expletive]
 *
 * "There are how many breads?"
 * [standard-copula pronoun-there-expletive
 *                  [question-element bread interrogative-quantity-determiner]]
 */
Obj *UA_Question_IsInterDetQuestion(Obj *question, /* RESULTS */ Obj **class)
{
  int	i;
  if (N("standard-copula") != I(question, 0)) return(NULL);
  if (N("pronoun-there-expletive") == I(question, 2)) {
    i = 1;
  } else if (N("pronoun-there-expletive") == I(question, 1)) {
    i = 2;
  } else {
    return(NULL);
  }
  if (N("question-element") != I(I(question, i), 0)) return(NULL);
#ifdef notdef
  /* Obsolete format?: */
  if (ISA(N("interrogative-determiner"), I(I(I(question, i), 2), 0))) {
    *class = I(I(question, i), 1);
    return(I(I(I(question, i), 2), 0));
  }
#endif
  if (ISA(N("interrogative-determiner"), I(I(question, i), 2))) {
    *class = I(I(question, i), 1);
    return(I(I(question, i), 2));
  }
  return(NULL);
}

/* See examples in UA_Question_IsInterDetQuestion. */
Answer *UA_QuestionQuantityAdjective(Obj *question, TsRange *tsr, Answer *an,
                                     Discourse *dc)
{
  Obj		*inter_adj, *class;
  ObjList	*answer;
  if ((inter_adj = UA_Question_IsInterDetQuestion(question, &class)) &&
      inter_adj == N("interrogative-quantity-determiner")) {
    /* question: "There are how many breads?"
     *           [standard-copula
     *            pronoun-there-expletive
     *            [question-element bread interrogative-quantity-determiner]]
     * result: "There are 25 breads."
     *         [standard-copula
     *          pronoun-there-expletive
     *          [such-that bread [25 bread]]]
     */
    answer = ObjListCreate(ThereAreXClass((Float)ObjNumDescendants(class,
                                                                   OBJTYPEANY),
                                          class),
                           NULL);
    return(AnswerCreateQWQ(N("UA_QuestionQuantityAdjective"), question,
                           SENSE_TOTAL, answer, an));
  }
  return(an);
}

/*
 * todo: These are now handled by UA_QuestionPronoun?
 * Example1:
 * "What designer created Bugs Bunny?"
 * INPUT: [create
 *         [question-element industrial-designer
 *          interrogative-identification-determiner]
 *         Bugs-Bunny]
 * OUTPUT: [create ? Bugs-Bunny]
 * modified: industrial-designer
 *
 * Example2:
 * "What color is an elephant?"
 * INPUT:  [standard-copula
 *          [question-element color interrogative-identification-determiner]
 *          elephant]
 * OUTPUT: [color elephant]
 * modified: color
 *
 */
Obj *ObjParseInterrogativeDeterminer(Obj *obj, Obj *det_class,
                                     /* RESULTS */ Obj **modified)
{
  int 	i, len;
  Obj	*elems[MAXLISTLEN];
  if (!ObjIsList(obj)) return(obj);

  /* Example2: */
  if (ISA(N("standard-copula"), I(obj, 0)) &&
      I(I(obj, 1), 0) == N("question-element") &&
      ISA(N("attribute"), I(I(obj, 1), 1)) &&
      ISA(det_class, I(I(obj, 1), 2)) &&
      I(obj, 2)) {
    *modified = I(I(obj, 1), 1);
    return(L(I(I(obj, 1), 1), I(obj, 2), E));
  }

  /* Example1: */
  if (I(obj, 0) == N("question-element") && ISA(det_class, I(obj, 2))) {
    *modified = I(obj, 1);
    return(ObjWild);
  }
  for (i = 0, len = ObjLen(obj); i < len; i++) {
    elems[i] = ObjParseInterrogativeDeterminer(I(obj, i), det_class, modified);
  }
  return(ObjCreateList1(elems, len));
}

Answer *UA_QuestionIdentDetGeneral(Obj *question, TsRange *tsr, Answer *an,
                                   Discourse *dc)
{
  Obj		*ptn, *modified;
  ObjList	*answer;
  TsrGenAdvice	tga;
  if (!ObjIsList(question)) return(an);
  modified = NULL;
  ptn = ObjParseInterrogativeDeterminer(question,
                            N("interrogative-identification-determiner"),
                                        &modified);
  if (modified == NULL) {
   ObjFree(ptn);
   return(an);
  }
  answer = ProveRetrieve(NULL, tsr, ptn, 1);
  /* todo: Further restrict possibilities with modified. */
  if (answer) {
    TsrGenAdviceSet(&tga);
    an = AnswerCreate(N("UA_QuestionIdentDetGeneral"), question,
                      N("question-word-question"), SENSE_TOTAL, answer,
                      0, 0, !ObjListIsLengthOne(answer), &tga, an);
  }
  return(an);
}

Bool UA_Question_IsSimpIdentDet1(Obj *head, Obj *con0, Obj *con1, Obj *con2,
                                 /* RESULTS */ Obj **object, Obj **restrct)
{
  /* "What electronic device is near the stove?"
   * "The stove is near what electronic device?"
   * question: [near [question-element electronics
   *                                   interrogative-identification-determiner]
   *                 stove]
   */
  if (con0 == head &&
      I(con2, 0) == N("question-element") &&
      I(con2, 2) == N("interrogative-identification-determiner")) {
    *object = con1;
    *restrct = I(con2, 1);
    return(1);
  }
  /* "What is near the stove?"
   * "The stove is near what?"
   * [near object-interrogative-pronoun stove]
   */
  if (con0 == head &&
      con2 == N("object-interrogative-pronoun")) {
    *object = con1;
    *restrct = N("nonhuman");
    return(1);
  }
  /* "Who is near the stove?" */
  if (con0 == head &&
      con2 == N("human-interrogative-pronoun")) {
    *object = con1;
    *restrct = N("human");
    return(1);
  }
  return(0);
}

Bool UA_Question_IsSimpIdentDet(Obj *head, Obj *con,
                                /* RESULTS */ Obj **object, Obj **restrct)
{
  if (UA_Question_IsSimpIdentDet1(head, I(con, 0), I(con, 1), I(con, 2), object,
                                  restrct)) {
    return(1);
  }
  if (UA_Question_IsSimpIdentDet1(head, I(con, 0), I(con, 2), I(con, 1), object,
                                  restrct)) {
    return(1);
  }
  return(0);
}

Bool UA_Question_IsLocIdentDet(Obj *con, /* RESULTS */ Obj **object,
                                              Obj **restrct)
{
  return(UA_Question_IsSimpIdentDet(N("location-of"), con, object, restrct));
}

/* todo: Merge this with UA_QuestionPronoun and various Location agents. */
Answer *UA_QuestionIdentificationDeterminer(Obj *question, TsRange *tsr,
                                            Answer *an, Discourse *dc)
{
  Obj	*object, *restrct;
  if (UA_Question_IsLocIdentDet(question, &object, &restrct)) {
    return(UA_QuestionLocation(question, object, restrct, tsr, an, dc));
  } else {
    return(UA_QuestionIdentDetGeneral(question, tsr, an, dc));
  }
}

Answer *UA_QuestionNear(Obj *question, Context *cx, TsRange *tsr, Answer *an,
                        Discourse *dc)
{
  Obj		*object, *restrct;
  ObjList	*answer;
  if (UA_Question_IsSimpIdentDet(N("near"), question, &object, &restrct)) {
  /* todo: Should obey other restrictions besides class in the intension. */
    if ((answer = SpaceFindNear(NULL, tsr, object, restrct, NULL))) {
      return(AnswerCreateQWQ(N("UA_QuestionNear"), question, SENSE_TOTAL,
                             answer, an));
    }
  }
  return(an);
}

/* TEMPORAL RELATION QUESTIONS */

Answer *UA_QuestionTemporalRelation2(Obj *rel, Obj *question, Context *cx,
                                     Obj *retrieved, int question_i,
                                     Obj *top_q, Answer *an, Discourse *dc)
{
  TsRange	query_tsr, *ret_tsr;
  Answer	*an1, *an2;
  Obj		*answer;
  ObjList	*p, *r;
  ret_tsr = ObjToTsRange(retrieved);
  AspectTempRelQueryTsr(rel, question_i, ret_tsr, &query_tsr);
  an1 = UA_QuestionWordQuestion(question, cx, &query_tsr, NULL, dc);
  for (an2 = an1; an2; an2 = an2->next) {
    r = NULL;
    for (p = an2->answer; p; p = p->next) {
      if (question_i == 1) {
        if (rel != AspectTempRelGen(ObjToTsRange(p->obj), ret_tsr, DCNOW(dc))) {
          continue;
        }
        answer = L(rel, p->obj, retrieved, E);
      } else {
        if (rel != AspectTempRelGen(ret_tsr, ObjToTsRange(p->obj), DCNOW(dc))) {
          continue;
        }
        answer = L(rel, retrieved, p->obj, E);
      }
      r = ObjListCreate(answer, r);
    }
    if (r) {
      an = AnswerCreateQWQ(N("UA_QuestionTemporalRelation2"), top_q,
                           SENSE_TOTAL, r, an);
    }
  }
  AnswerFreeAll(an1);
  return(an);
}

Answer *UA_QuestionTemporalRelation1(Obj *rel, Obj *question, Context *cx,
                                     Obj *anchor, int question_i, Obj *top_q,
                                     TsRange *tsr, Answer *an, Discourse *dc)
{
  ObjList	*objs, *p;
  for (p = objs = ProveRetrieve(NULL, tsr, anchor, 0); p; p = p->next) {
    an = UA_QuestionTemporalRelation2(rel, question, cx, p->obj, question_i,
                                      top_q, an, dc);
  }
  ObjListFree(objs);
  return(an);
}

/* [superset-simultaneity
 *  [mtrans Martine-Mom Lucie na]
 *  [standard-copula location-interrogative-pronoun Martine-Mom]]
 */
Answer *UA_QuestionTemporalRelation(Obj *question, Context *cx, TsRange *tsr,
                                    Answer *an, Discourse *dc)
{
  if (ISA(N("temporal-relation"), I(question, 0))) {
    if (ISADeep(N("question-word"), I(question, 2))) {
      return(UA_QuestionTemporalRelation1(I(question, 0), I(question, 2), cx,
                                          I(question, 1), 2, question, tsr, an,
                                          dc));
    } else if (ISADeep(N("question-word"), I(question, 1))) {
      return(UA_QuestionTemporalRelation1(I(question, 0), I(question, 1), cx,
                                          I(question, 2), 1, question, tsr, an,
                                          dc));
    } else {
      Dbg(DBGUA, DBGBAD,"UA_QuestionTemporalRelation: question word not found");
    }
  }
  return(an);
}

/* DURATION QUESTION */

Answer *UA_QuestionDuration(Obj *question, TsRange *tsr, Answer *an,
                            Discourse *dc)
{
  Obj			*ptn;
  ObjList		*objs, *p;
  TsrGenAdvice	tga;
  if (I(question, 0) != N("duration-of") ||
      I(question, 2) != N("duration-interrogative-pronoun")) {
    return(an);
  }
  /* todo: Trip -> [near-reachable ? ?]. */
  ptn = I(question, 1);
  TsrGenAdviceClear(&tga);
  tga.include_dur = 1;
  for (p = objs = ProveRetrieve(NULL, tsr, ptn, 0); p; p = p->next) {
    an = AnswerCreate(N("UA_QuestionDuration"), question,
                      N("question-word-question"),
                      SENSE_TOTAL, ObjListCreate(p->obj, NULL),
                      0, 0, 0, &tga, an);
  }
  ObjListFree(objs);
  return(an);
}

/* DESCRIPTION QUESTIONS */

/* todo: "Tell me something about X." */
Answer *UA_QuestionDescribe(Obj *question, Obj *obj, TsRange *tsr,
                            Answer *an, Discourse *dc)
{
  int		i, success, save_channel;
  Text		*text;
  Channel	*ch;
  Answer	*an_orig;
  an_orig = an;
  save_channel = DiscourseGetCurrentChannel(dc);
  success = 0;
  an = AnswerCreate(N("UA_QuestionDescribe"), question,
                    N("question-word-question"),
                    SENSE_TOTAL-.1, NULL, 0, 0, 0, NULL, an);
                    /* the -.1 above is to give UA_QuestionElementTop priority.
                     * cf "What is your name?"
                     */
  for (i = 0; i < DCMAX; i++) {
    if (NULL == (ch = DiscourseSetCurrentChannelWrite(dc, i))) continue;
    text = TextCreat(dc);
    DictObj(text, obj, F_NOUN, NULL, dc);
    if (TextLen(text) > 0) {
      an->answer_text[i] = text;
      success = 1;
    } else an->answer_text[i] = NULL;
  }
  DiscourseSetCurrentChannel(dc, save_channel);
  if (success) {
    return(an);
  } else {
    /* AnswerFreeAll(an)? */
    return(an_orig);
  }
}

Answer *UA_QuestionDescribeHuman(Obj *question, TsRange *tsr, Answer *an,
                                 Discourse *dc)
{
  Obj		*human;
  if ((human = UA_Question_IsDescription(question,
                                         N("human-interrogative-pronoun"),
                                         N("human")))) {
    return(UA_QuestionDescribe(question, human, tsr, an, dc));
  } else if ((human = UA_Question_IsDescription(question,
                                         N("object-interrogative-pronoun"),
                                         N("human")))) {
  /* What is an intelligentphobe? */
    return(UA_QuestionDescribe(question, human, tsr, an, dc));
  } else {
    return(an);
  }
}

Answer *UA_QuestionDescribeObject(Obj *question, TsRange *tsr, Answer *an,
                                  Discourse *dc)
{
  Obj		*nonhuman;
  if (!(nonhuman = UA_Question_IsDescription(question,
                                             N("object-interrogative-pronoun"),
                                             N("nonhuman")))) {
    return(an);
  }
  return(UA_QuestionDescribe(question, nonhuman, tsr, an, dc));
}

/* CANNED QUESTIONS */

Answer *AnswerPromoteReasons(Obj *ua, Obj *question, Answer *last_an,
                             Answer *an)
{
  ObjList	*p, *new_answer, *reasons;
  for (; last_an; last_an = last_an->next) {
    new_answer = NULL;
    for (p = last_an->answer; p; p = p->next) {
      if ((reasons = ObjToJustification(p->obj))) {
        new_answer = ObjListCreate(L(N("since"), ObjListToAndObj(reasons),
                                     p->obj, E),
                                   new_answer);
      }
    }
    if (new_answer) {
      an = AnswerCreate(ua, question, last_an->qclass, last_an->sense,
                        new_answer, last_an->sort_by_ts,
                        last_an->gen_pairs, last_an->gen_tsr,
                        &last_an->tga, an);
    }
  }
  return(an);
}

Answer *AnswerPromoteRephrasedAnswers(Obj *ua, Obj *question, Answer *last_an,
                                    Answer *an)
{
  for (; last_an; last_an = last_an->next) {
    if (last_an->rephrased_answer) {
      an = AnswerCreate(ua, question, last_an->qclass, last_an->sense,
                        last_an->rephrased_answer, last_an->sort_by_ts,
                        last_an->gen_pairs, last_an->gen_tsr,
                        &last_an->tga, an);
      an->eos = last_an->eos;
    }
  }
  return(an);
}

Answer *UA_QuestionCanned(Obj *question, Context *cx, TsRange *tsr, Answer *an,
                          Discourse *dc)
{
  Obj			*current;
  ObjList		*answer;
  TsRange		now;
  TsrGenAdvice	tga;
  if (ISA(N("intervention"), I(question, 0)) &&
      Me == I(question, 2)) {
    question = I(question, 0);
  }

  if (ISA(N("current-timestamp-request"), question)) {
    TsrGenAdviceGet(question, dc, &tga, &current, NULL);
    TsRangeSetNow(&now);
    ObjSetTsRange(current, &now);
    answer = ObjListCreate(current, NULL);
    an = AnswerCreate(N("UA_QuestionCanned"), question, N("canned-question"),
                      SENSE_TOTAL, answer, 0, 0, 1, &tga, an);
  } else if (ISA(N("interjection-of-noncomprehension"), question) &&
             cx->last_question &&
             cx->last_question->full_question) {
    answer = ObjListCreate(cx->last_question->full_question, NULL);
    an = AnswerCreate(N("UA_QuestionCanned"), question, N("canned-question"),
                      SENSE_TOTAL, answer, 0, 0, 0, NULL, an);
    an->eos = '?';
    /* todo: If cx->last_question is aged, refresh the aging here. */
  } else if (dc->last_answers &&
             ISA(N("interjection-of-noncomprehension"), question) &&
             dc->last_answers->rephrased_answer) {
    an = AnswerPromoteRephrasedAnswers(N("UA_QuestionCanned"), question,
                                       dc->last_answers, an);
  } else if (dc->last_answers &&
             (ISA(N("explanation-request"), question) ||
              ISA(N("interjection-of-noncomprehension"), question))) {
    an = AnswerPromoteReasons(N("UA_QuestionCanned"), question,
                              dc->last_answers, an);
    dc->last_answers = NULL;	/* todo: added due to bus error */
  }
  return(an);
}

/* STIMULUS QUESTIONS */

Answer *UA_QuestionStimulusSentence(Obj *question, TsRange *tsr, Answer *an,
                                    Discourse *dc)
{
  ObjList		*answer;
  Obj			*response;
  TsRange		now;

  if ((response = DbGetRelationValue(&TsNA, NULL, N("response-of"), question,
                                     NULL)) ||
      (response = DbGetRelationValue(&TsNA, NULL, N("response-of"),
                                     I(question, 0), NULL))) {
    TsRangeSetNow(&now);
    ObjSetTsRange(response, &now);
    answer = ObjListCreate(response, NULL);
    an = AnswerCreate(N("UA_QuestionStimulusSentence"), question,
                      N("stimulus-sentence"), SENSE_TOTAL, answer,
                      0, 0, 0, NULL, an);
  }
  return(an);
}

/* YES-NO QUESTIONS */

Obj *ObjSetMemberType(Obj *des)
{
  if (ObjIsConcrete(des)) return(N("isa"));
  else return(N("ako"));
}

Obj *ObjISA(Obj *anc, Obj *des)
{
  return(L(ObjSetMemberType(des), des, anc, E));
}

Obj *YesResponse(Obj *assertion)
{
  return(L(N("adverb-of-affirmation"),
           assertion,
           E));
}

Obj *NoResponse(Obj *assertion)
{
  return(L(N("sentence-adverb-of-negation"),
           L(N("not"), assertion, E),
           E));
}

Obj *NoButResponse(Obj *but)
{
  return(L(N("sentence-adverb-of-negation"),
           L(N("but"), but, E),
           E));
}

Answer *UA_QuestionISA(Obj *question, TsRange *tsr, Answer *an, Discourse *dc)
{
  Obj		*anc, *des;
  ObjList	*answer, *common_anc, *p, *r;
  if ((des = I(question, 1)) && (des != ObjNA) &&
      (anc = I(question, 2)) && (anc != ObjNA)) {
    if (ISAP(anc, des)) {
      answer = ObjListCreate(YesResponse(ObjISA(anc, des)), NULL);
      return(AnswerCreate(N("UA_QuestionISA"), question, N("Yes-No-question"),
                          SENSE_TOTAL, answer, 0, 0, 0, NULL, an));
    } else if (ISAP(des, anc)) {
      answer = ObjListCreate(ObjISA(des, anc), NULL);
      return(AnswerCreate(N("UA_QuestionISA"), question, N("Yes-No-question"),
                          SENSE_SOME, answer, 0, 0, 0, NULL, an));
    } else if ((common_anc = ObjCommonAncestors(anc, des))) {
      r = NULL;
      for (p = common_anc; p; p = p->next) {
        r = ObjListCreate(L(ObjSetMemberType(des),
                            L(N("and"), anc, des, E),
                            p->obj,
                            E),
                          r);
      }
      answer = ObjListCreate(NoButResponse(ObjListToAndObj(r)), NULL);
      return(AnswerCreate(N("UA_QuestionISA"), question, N("Yes-No-question"),
                          SENSE_LITTLE, answer, 0, 0, 0, NULL, an));
    } /* todo: Else "no"? */
  }
  return(an);
}

ObjList *UA_QuestionYesNo3(Obj *question, Obj *top_question, TsRange *tsr,
                           Discourse *dc, int depth, int maxdepth, Obj *attr,
                           Float in_sense, /* RESULTS */ Float *out_sense)
{
  ObjList	*objs, *p, *answer;
#ifdef notdef
  int		i;
  Float		weight, out_sense1;
  Obj		*question1;
  ObjList	*answer1;
#endif
  *out_sense = SENSE_NONE;
  if (depth >= maxdepth) return(NULL);
  answer = NULL;
  if ((objs = ProveRetrieve(NULL, tsr, question, 0))) {
    for (p = objs; p; p = p->next) {
      if ((attr == NULL) ||
          (FloatSign(ObjModGetPropValue(question)) ==
           FloatSign(ObjModGetPropValue(p->obj)))) {
        *out_sense = in_sense;
        if (depth == 0 &&
            I(p->obj, 0) == I(question, 0)) { /* cf "What did she do?" */
          answer = ObjListCreate(YesResponse(p->obj), answer);
        } else {
          answer = ObjListCreate(p->obj, answer);
        }
      } else {
        *out_sense = in_sense/2.0;
        if (depth == 0) {
          answer = ObjListCreate(NoResponse(p->obj), answer);
        } else {
          answer = ObjListCreate(p->obj, answer);
        }
      }
    }
    ObjListFreeFirst(objs);
  }
#ifdef notdef
  /* This is often misleading. */
  if (answer == NULL && attr) {
  /* Try querying on ancestors of attribute, to find answer which
   * may or may not really relate to the original question.
   * todo: Not so sure this is good. We get:
   *   "Q: Are elephants stupid?"
   *   "A: No, elephant are not stupid. Elephants are gray."
   */
    if (depth == 0) {
      answer = ObjListCreate(NoResponse(question), answer);
    }
    weight = ObjWeight(question);
    for (i = 0; i < attr->u1.nlst.numparents; i++) {
      if (ObjBarrierISA(attr->u1.nlst.parents[i])) continue;
      question1 = L(attr->u1.nlst.parents[i], I(question, 1),
                    NumberToObj(weight), E);
      answer1 = UA_QuestionYesNo3(question1, top_question, tsr, dc, depth+1,
                                  maxdepth, attr->u1.nlst.parents[i],
                                  in_sense*0.6, &out_sense1);
      if (answer1) {
        *out_sense = out_sense1;
        answer = ObjListAppendDestructive(answer, answer1);
      } else {
        ObjFree(question1);
      }
    }
  }
#endif
  return(answer);
}

Answer *UA_QuestionYesNo2(Obj *question, TsRange *tsr, Answer *an,
                          Discourse *dc)
{
  Float		sense;
  Obj		*attr;
  ObjList	*answer;

  if (ISA(N("attribute"), I(question, 0))) {
    attr = I(question, 0);
  } else {
    attr = NULL;
  }
  if ((answer = UA_QuestionYesNo3(question, question, tsr, dc, 0, MAXISADEPTH,
                                  attr, SENSE_TOTAL, &sense))) {
    return(AnswerCreate(N("UA_QuestionYesNo2"), question, N("Yes-No-question"),
                        sense, answer, 0, 0, !ObjListIsLengthOne(answer),
                        NULL, an));
  } else {
    answer = ObjListCreate(NoResponse(question), NULL);
    return(AnswerCreate(N("UA_QuestionYesNo2"), question, N("Yes-No-question"),
                        SENSE_LITTLE, answer, 0, 0,
                        !ObjListIsLengthOne(answer), NULL, an));
  }
}

Answer *UA_QuestionYesNo1(Obj *question, TsRange *tsr, Answer *an,
                          Discourse *dc)
{
  if (!ObjIsList(question)) return(an);
  if (ISA(N("standard-copula"), I(question, 0))) return(an);
  if (ISA(N("set-relation"), I(question, 0))) {
    an = UA_QuestionISA(question, ObjToTsRange(question), an, dc);
  } else {
    an = UA_QuestionYesNo2(question, tsr, an, dc);
  }
  return(an);
}

Answer *UA_QuestionYesNo(Obj *question, TsRange *tsr, Answer *an, Discourse *dc)
{
  Obj		*noun;
  ObjList	*questions, *p;
  if (ISA(N("explanation-request"), I(question, 0)) ||
      ISA(N("interjection-of-noncomprehension"), I(question, 0))) {
  /* This is to rule out triggering on "Why?". Other cases
   * where it should trigger? ("Did Harry ask Sally why?")
   */
    return(an);
  }
  if ((questions = ObjIntensionParse(question, 0, &noun))) {
    for (p = questions; p; p = p->next) {
      an = UA_QuestionYesNo1(p->obj, tsr, an, dc);
    }
  } else {
    an = UA_QuestionYesNo1(question, tsr, an, dc);
  }
  return(an);
}

/* QUESTION WORD QUESTIONS */

Answer *UA_QuestionWordQuestion(Obj *question, Context *cx, TsRange *tsr,
                                Answer *an, Discourse *dc)
{
  an = UA_QuestionLocationAdverb(question, tsr, an, dc);
  an = UA_QuestionTimeAdverb(question, tsr, an, dc);
  an = UA_QuestionMannerAdverb(question, tsr, an, dc);
  an = UA_QuestionAppearanceAdverb(question, tsr, an, dc);
  an = UA_QuestionReasonAdverb(question, tsr, an, dc);
  an = UA_QuestionMeansAdverb(question, tsr, an, dc);
  an = UA_QuestionQuantityAdverb(question, tsr, an, dc);
  an = UA_QuestionDegreeAdverb(question, tsr, an, dc);

  an = UA_QuestionPronoun(question, tsr, an, dc);
  an = UA_QuestionLocationPronoun(question, tsr, an, dc);

  an = UA_QuestionTemporalRelation(question, cx, tsr, an, dc);
  an = UA_QuestionDuration(question, tsr, an, dc);

  an = UA_QuestionDescribeHuman(question, tsr, an, dc);
  an = UA_QuestionDescribeObject(question, tsr, an, dc);
  
  an = UA_QuestionElementTop(question, tsr, an, dc);
  an = UA_QuestionQuantityAdjective(question, tsr, an, dc);
  an = UA_QuestionIdentificationDeterminer(question, tsr, an, dc);

  an = UA_QuestionNear(question, cx, tsr, an, dc);
  an = UA_QuestionWeather(question, cx, tsr, an, dc);

  return(an);
}

/* ALL QUESTIONS */

Answer *UA_Question2(Obj *question, Context *cx, int eoschar, Answer *an,
                     Discourse *dc)
{
  TsRange	*tsr;
  tsr = ObjToTsRange(question);

  if (ISA(N("present-tense"), dc->tense) &&
      TsRangeIsNaIgnoreCx(tsr)) {
  /* Allows present tense questions to be answered properly:
   * Where is Lin?
   * Is Lin awake?
   */
    TsRangeSetNow(tsr);
    TsRangeSetContext(tsr, cx);
    Dbg(DBGUA, DBGDETAIL, "UA_Question: assume now");
  }

  Dbg(DBGUA, DBGDETAIL, "UA_Question: TsRange =");
  TsRangePrint(Log, tsr);
  fputc(NEWLINE, Log);
  an = UA_QuestionCanned(question, cx, tsr, an, dc);
  an = UA_QuestionStimulusSentence(question, tsr, an, dc);
  an = UA_AppointmentQuestion(question, cx, tsr, an, dc);

  if (ISADeep(N("question-word"), question)) {
    an = UA_QuestionWordQuestion(question, cx, tsr, an, dc);
  } else if (eoschar == '?') {
    an = UA_QuestionYesNo(question, tsr, an, dc);
  }
  return(an);
}

Answer *UA_Question1(Obj *question, Context *cx, int eoschar, Answer *an,
                     Discourse *dc)
{
  /* There was some obsolete timestamp logic here. */
  return(UA_Question2(question, cx, eoschar, an, dc));
}

/* <*answer> not initialized */
Float UA_Question(Discourse *dc, Context *cx, Obj *con, int eoschar,
                  /* RESULTS */ Answer **answer)
{
  Answer	*an;
  Float		maxsense;
  if ((an = UA_Question1(con, cx, eoschar, NULL, dc))) {
    AnswerAdjustSense(an);
    maxsense = AnswerMaxSense(an);
    if (an->next) an = AnswerRemoveBelowThresh(an, maxsense*0.75);
    *answer = an;
    return(maxsense);
  } else {
    *answer = NULL;
    return(0.0);
  }
}

/* Commentary */

Answer *Commentary;

void CommentaryInit()
{
  Commentary = NULL;
}

void CommentaryAdd(Context *cx, ObjList *objs, Obj *tga_question)
{
  int			gen_tsr;
  Answer		*an;
  TsrGenAdvice	tga;
  TsrGenAdviceGet(tga_question, StdDiscourse, &tga, NULL, &gen_tsr);
  an = AnswerCreate(N("CommentaryAdd"), N("na"), N("commentary"),
                    SENSE_TOTAL, objs, 0, 0, gen_tsr, &tga,
                    NULL);
  if (cx) {
    cx->commentary = AnswerAppendDestructive(cx->commentary, an);
  } else {
    Commentary = AnswerAppendDestructive(Commentary, an);
  }
}

void CommentaryFlush(Context *cx)
{
  Discourse	*dc;
  if (cx) dc = cx->dc; else dc = StdDiscourse;
  if (cx && cx->commentary) {
    AnswerGenAll(cx->commentary, dc);
  }
  if (Commentary) {
    AnswerGenAll(Commentary, dc);
    AnswerFreeAll(Commentary);
    Commentary = NULL;
  }
}

/* End of file. */
