/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "semaspec.h"
#include "lexentry.h"
#include "repbasic.h"
#include "repcxt.h"
#include "repdb.h"
#include "repgroup.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "repstr.h"
#include "reptime.h"
#include "semdisc.h"
#include "semgen1.h"
#include "semgen2.h"
#include "semparse.h"
#include "semtense.h"
#include "synpnode.h"
#include "synxbar.h"
#include "utildbg.h"
#include "utillrn.h"

Bool AspectISA(Obj *class, Obj *obj)
{
  if (N("and") == I(obj, 0)) return(AspectISA(class, I(obj, 1)));
  return(ISA(class, I(obj, 0)));
}

Obj *AspectDetermine1(TsRange *focus_tsr, Obj *obj, int situational)
{
  TsRange	*obj_tsr;
  obj_tsr = ObjToTsRange(obj);
  if (ISA(N("goal-status"), I(obj, 0))) {
  /* todo: Inelegant. "you want to buy a 1200, an 1800, or a 124?" */
    return(N("aspect-unknown"));
  }
  if (TsRangeIsAlwaysIgnoreCx(obj_tsr) ||
      TsRangeIsNaIgnoreCx(obj_tsr)) {
    if (ISA(N("attribute"), I(obj, 0))) {
      return(N("aspect-generality-ascriptive"));
    } else if (ISA(N("copula"), I(obj, 0)) ||
               ISA(N("set-relation"), I(obj, 0))) {
      return(N("aspect-generality-equative"));
    } else {
      return(N("aspect-generality"));
    }
  }
  if (AspectISA(N("state"), obj)) {
    if (TsRangeIsProperContained(obj_tsr, focus_tsr)) {
      return(N("aspect-stable-situation"));
    } else if (TsRangeTsIsContained(focus_tsr, &obj_tsr->startts) ||
               TsRangeTsIsContained(focus_tsr, &obj_tsr->stopts)) {
      return(N("aspect-changed-situation"));
    } else {
    /* "Mais je croyais." tsr = -inf:now */
      return(N("aspect-stable-situation"));
    }
  }
  if (AspectISA(N("action"), obj)) {
    if (TsRangeIsProperContained(obj_tsr, focus_tsr)) {
    /* focus_tsr        __
     * obj_tsr     ____________
     */
      return(N("aspect-inaccomplished-situational"));
    } else if (TsRangeIsContained(focus_tsr, obj_tsr)) {
    /* obj_tsr           __
     * focus_tsr     ___________
     */
      if (situational) {
        return(N("aspect-accomplished-situational"));
      } else {
        return(N("aspect-accomplished-aorist"));
      }
    } else if (TsRangeGT(focus_tsr, obj_tsr)) {
    /* obj_tsr       ___
     * focus_tsr          ___
     */
      if (situational) {
        return(N("aspect-accomplished-situational"));
      } else {
        return(N("aspect-accomplished-aorist"));
      }
    } else if (TsRangeLT(focus_tsr, obj_tsr)) {
    /* obj_tsr            ___
     * focus_tsr     ___
     */
      return(N("aspect-accomplished-aorist")); /* todo */
    }
  }
  return(N("aspect-unknown"));
}

Obj *AspectDetermine(TsRange *focus_tsr, Obj *obj, int situational)
{
  Obj	*aspect;
  aspect = AspectDetermine1(focus_tsr, obj, situational);
  if (DbgOn(DBGTENSE, DBGDETAIL)) {
    fputs("ASPECT focus ", Log);
    TsRangePrint(Log, focus_tsr);
    fputs(" obj ", Log);
    TsRangePrint(Log, ObjToTsRange(obj));
    fputc(' ', Log);
    ObjPrint(Log, obj);
    if (situational) fputs(" situational", Log);
    else fputs(" nonsituational", Log);
    fprintf(Log, ": <%s>\n", M(aspect));
  }
  return(aspect);
}

Obj *AspectToTense1(Obj *aspect, int tensestep, int is_literary, int lang)
{
  char	rel[DWORDLEN];
  Obj	*tense;
  sprintf(rel, "%s-tense-%s%s%d-of",
               lang == F_ENGLISH ? "eng" : "fr",
               is_literary ? "literary-" : "",
               (tensestep == 0) ? "" : ((tensestep < 0) ? "neg-" : "pos-"),
               IntAbs(tensestep));
  if ((tense = DbGetRelationValue(&TsNA, NULL, N(rel), aspect, NULL))) {
    return(tense);
  }
  if (is_literary) {
    return(AspectToTense1(aspect, tensestep, 0, lang));
  }
  if (aspect != N("aspect-unknown")) {
    return(AspectToTense1(N("aspect-unknown"), tensestep, is_literary, lang));
  }
  return(NULL);
}

Obj *AspectToTense(Obj *aspect, int tensestep, int is_literary, int lang)
{
  Obj	*tense;
  /*
    For testing passé simple
  if (lang == F_FRENCH) is_literary = 1;
   */
  tense = AspectToTense1(aspect, tensestep, is_literary, lang);
  if (DbgOn(DBGTENSE, DBGDETAIL)) {
    fprintf(Log, "ASPECT <%s> tensestep %d literary %d => TENSE <%s>\n",
            M(aspect), tensestep, is_literary, M(tense));
  }
  return(tense);
}

Obj *AspectDetermineSingleton(Obj *obj)
{
  return(AspectDetermine(ObjToTsRange(obj), obj, ISA(N("goal-status"),
                         I(obj, 0))));
}

/* rel obj1, obj2 */
void AspectDetermineRelatedPair(Obj *rel, Obj *obj1, Obj *obj2, /* RESULTS */
                                Obj **aspect1, Obj **aspect2)
{
  Dbg(DBGTENSE, DBGDETAIL, "ASPECT RELATED PAIR <%s>", M(rel));
  *aspect1 = AspectDetermine(ObjToTsRange(obj2), obj1, 0);
  *aspect2 = AspectDetermine(ObjToTsRange(obj1), obj2,
                             ISA(N("persistent-posteriority"), rel));
}

/* Called by UAs.
 * todo: Need to convert subjunctive/progressive into indicative.
 */
Bool AspectIsCompatibleTense(Obj *aspect, Obj *tense, int lang)
{
  if (lang == F_ENGLISH) {
    return(YES(RD(&TsNA, L(N("eng-tense-of"), aspect, tense, E), 0)));
  } else {
    return(YES(RD(&TsNA, L(N("fr-tense-of"), aspect, tense, E), 0)));
  }
}

/* Called by UAs. */
Bool AspectIsAccomplished(Discourse *dc)
{
  return(AspectIsCompatibleTense(N("aspect-accomplished-situational"),
                                 dc->tense, DC(dc).lang) ||
         (dc->abs_dur != DURNA &&
          AspectIsCompatibleTense(N("aspect-accomplished-aorist"), dc->tense,
                                  DC(dc).lang)));
}

/* This generates query timestamps which only restrict partially. The results
 * of the query must still be checked for their exact temporal relation against
 * a given target using AspectTempRelGen.
 */
void AspectTempRelQueryTsr(Obj *rel, int qi, TsRange *tsr_in, /* RESULTS */
                           TsRange *tsr_out)
{
  TsRangeSetAlways(tsr_out);
  if (qi == 2) {
    if (rel == N("disjoint-posteriority") ||
        rel == N("adjacent-posteriority")) {
      tsr_out->startts = tsr_in->stopts;
      TsSetPosInf(&tsr_out->stopts);
    } else if (rel == N("persistent-posteriority")) {
      tsr_out->startts = tsr_in->startts;
      TsSetPosInf(&tsr_out->stopts);
    } else if (rel == N("priority")) {
      TsSetNegInf(&tsr_out->startts);
      tsr_out->stopts = tsr_in->startts;
    } else if (rel == N("subset-simultaneity") ||
               rel == N("equally-long-simultaneity")) {
      /* Always. */
    } else if (rel == N("superset-simultaneity")) {
      tsr_out->startts = tsr_in->startts;
      tsr_out->stopts = tsr_in->stopts;
    }
  } else if (qi == 1) {
    if (rel == N("disjoint-posteriority") ||
        rel == N("adjacent-posteriority")) {
      TsSetNegInf(&tsr_out->startts);
      tsr_out->stopts = tsr_in->startts;
    } else if (rel == N("persistent-posteriority")) {
      /* Always? */
    } else if (rel == N("priority")) {
      tsr_out->startts = tsr_in->stopts;
      TsSetPosInf(&tsr_out->stopts);
    } else if (rel == N("superset-simultaneity") ||
               rel == N("equally-long-simultaneity")) {
      /* Always. */
    } else if (rel == N("subset-simultaneity")) {
      tsr_out->startts = tsr_in->startts;
      tsr_out->stopts = tsr_in->stopts;
    }
  } else {
    Dbg(DBGTENSE, DBGBAD, "AspectTempRelQueryTsr: invalid qi");
  }
}

#ifdef notdef
/* Could instead regenerate the relation and compare, but this is probably
 * too restrictive.
 */
Bool AspectTempRelConsistent(Obj *rel, TsRange *tsr1, TsRange *tsr2, Ts *now)
{
  if (TsIsNa(&tsr1->startts) &&
      TsIsNa(&tsr1->stopts) &&
      TsIsNa(&tsr2->startts) &&
      TsIsNa(&tsr2->stopts)) {
    return(1);
  }
  if (ISA(N("priority"), rel)) {
    if (TsGT2(&tsr2->stopts, &tsr1->startts)) return(0);
    if (TsGT2(&tsr2->startts, &tsr1->startts)) return(0);
    if (TsGT2(&tsr2->stopts, &tsr1->stopts)) return(0);
    if (TsGT2(&tsr2->startts, &tsr1->stopts)) return(0);
  } else if (N("persistent-posteriority") == rel) {
    if (TsGT2(now, &tsr2->stopts)) return(0);
  } else if (ISA(N("posteriority"), rel)) {
    if (TsGT2(&tsr1->stopts, &tsr2->startts)) return(0);
    if (TsGT2(&tsr1->startts, &tsr2->startts)) return(0);
    if (TsGT2(&tsr1->stopts, &tsr2->stopts)) return(0);
    if (TsGT2(&tsr1->startts, &tsr2->stopts)) return(0);
  } else if (ISA(N("simultaneity"), rel)) {
    if (!TsRangeOverlaps(tsr1, tsr2)) return(0);
  }
  return(1);      
}
#endif

/* TEMPORAL RELATIONS
 * References:
 * (Grevisse, 1986, section 1081)
 * (Carlut and Meiden, 1976, p. 110)
 */

Obj *AspectTempRelGen(TsRange *tsr1, TsRange *tsr2, Ts *now)
{
  Float		sep1_2, sep2_1, sep_start, sep_stop, sep1_now, dur1, dur2;
  Float		timescale;
  TsRange	r1, r2;
  TsRangeDefault(tsr1, tsr2, &r1);
  TsRangeDefault(tsr2, tsr1, &r2);
  sep1_2 = (Float)TsMinus(&r2.startts, &r1.stopts);
  dur1 = (Float)TenseDuration(&r1);
  dur2 = (Float)TenseDuration(&r2);
  timescale = dur1 + dur2;
  Dbg(DBGTENSE, DBGHYPER, "timescale %g sep1_2 %g", timescale, sep1_2);
  if (((FloatAbs(sep1_2)/timescale) < .1) && dur1 <= TENSEIMPERFECTABS) {
    return(N("adjacent-posteriority"));
  }
  if (TsLE1(&r1.stopts, &r2.startts)) {
    return(N("disjoint-posteriority"));
  }
  sep2_1 = (Float)TsMinus(&r1.startts, &r2.stopts);
  Dbg(DBGTENSE, DBGHYPER, "sep2_1 %g", sep2_1);
  if (((FloatAbs(sep2_1)/timescale) < .1) && dur2 >= TENSEIMPERFECTABS) {
    return(N("adjacent-priority"));
  }
  if (TsLE1(&r2.stopts, &r1.startts)) {
    return(N("disjoint-priority"));
  }
  sep_start = (Float)TsMinus(&r1.startts, &r2.startts);
  Dbg(DBGTENSE, DBGHYPER, "sep_start %g", sep_start);
  sep1_now = TsFloatMinus(now, &r1.startts);
  if ((sep1_now > TENSEPERSISTENT) &&
      ((FloatAbs(sep_start)/timescale) < .1) &&
      (TsIsNa(&r2.stopts) || TsGT1(&r2.stopts, now))) {
    return(N("persistent-posteriority"));
  }
  if (TsRangeIsContained(&r1, &r2)) {
    return(N("superset-simultaneity"));
  }
  if (TsRangeIsContained(&r2, &r1)) {
    return(N("subset-simultaneity"));
  }
  Dbg(DBGTENSE, DBGHYPER, "sep_stop %g", sep_stop);
  sep_stop = (Float)TsMinus(&r1.stopts, &r2.stopts);
  if (((FloatAbs(sep_start)/timescale) < .1) && ((sep_stop/timescale) < .1)
      && dur1 >= TENSEIMPERFECTABS) {
    return(N("equally-long-simultaneity"));
  }
  return(N("and"));
}

#ifdef notdef
void UA_TimeTemporalRelation(Discourse *dc, Obj *rel, Obj *obj1, Obj *obj2)
{
  TsRange	*tsr1, *tsr2;
  tsr1 = ObjToTsRange(obj1);
  tsr2 = ObjToTsRange(obj2);
  if (!AspectTempRelConsistent(rel, tsr1, tsr2, DCNOW(dc))) {
    Dbg(DBGUA, DBGDETAIL, "inconsistent temporal relation <%s>", M(rel));
    return;
  }
  /* todo: How much separation should be used for disjoint, subset, and
   * superset? Currently 0. (disjoint=adjacent, subset=superset=equally-long)
   */
  if (N("persistent-posteriority") == rel) {
    TsRangeFillPersistentPosteriority(tsr1, tsr2);
  } else if (ISA(N("posteriority"), rel)) {
    TsRangeFillAdjacentPosteriority(tsr1, tsr2);
  } else if (ISA(N("priority"), rel)) {
    TsRangeFillAdjacentPosteriority(tsr2, tsr1);
  } else if (ISA(N("simultaneity"), rel)) {
    TsRangeFillEquallyLongSimultaneity(tsr1, tsr2);
  }
}
#endif

/* End of file. */
