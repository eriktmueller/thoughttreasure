/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940616: touched
 * 19940617: really begun
 * 19940705: modified for new timestamp scheme
 * 19940711: timestamp generation
 * 19940712: cleanup of previous day
 * 19941013: generation of infinitival conjunctions
 * 19950118: genitive
 * 19950323: cleaned up speech-acts/interventions = mtrans
 * 19951129: use of aspect to determine number and articles for generalities
 * 19981112: GenCoordAppendNext fix
 *
 * todo:
 * - Holes in adjective value ranges result in no generation. Instead
 *   this should fall back to generic term with right sign and
 *   use of adverbs. [rich Jim -0.9]
 *   Could make sure there is always a default entry for every
 *   adjective.
 * - Generate nominalizations.
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "lextheta.h"
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
#include "semaspec.h"
#include "semdisc.h"
#include "semgen1.h"
#include "semgen2.h"
#include "semparse.h"
#include "semstyle.h"
#include "semtense.h"
#include "synpnode.h"
#include "syntrans.h"
#include "synxbar.h"
#include "utildbg.h"
#include "utillrn.h"

/* Gen */

ObjList *GenParagraph(ObjList *in_objs, int sort_by_ts, int gen_pairs,
                      Discourse *dc)
{
  Obj		*con;
  ObjList	*objs, *pns, *new;
  PNode		*pn;
  objs = StyleReorganize(in_objs, dc);
  if (sort_by_ts) {
    /*
    objs = ObjListSortByStartTs(objs);
     */
    gen_pairs = 0;	/* todo: Get rid of gen_pairs concept. */
  }
  pns = NULL;
  while (objs) {
    if (gen_pairs && objs->next) {
      con = GenCreateTemporalRelation(objs->obj, objs->next->obj, dc);
      objs = objs->next->next;
    } else {
      con = objs->obj;
      objs = objs->next;
    }
    if ((pn = Gen(F_S, con, NULL, dc))) {
      if (!PNodeSimilarIn(pn, pns)) {
        new = ObjListCreate(con, NULL);
        new->u.sp.pn = pn;
        pns = ObjListAppendDestructive(pns, new);
      }
    }
  }
  if (objs != in_objs) ObjListFree(objs);
  /* todo: Invoke discourse-level transformations here. */
  return(pns);
}

PNode *Gen(int constit, Obj *obj, Obj *comptense, Discourse *dc)
{
  PNode	*pn;
  DiscourseNextSentence(dc);	/* todo: Except when for debugging output. */
  DiscourseAnaphorInit(dc);
  pn = TransformGen(Generate(constit, obj, comptense, dc), dc);
  DiscourseAnaphorCommit(dc, pn);
  return(pn);
}

/* Recurse here. */
PNode *Generate(int constit, Obj *obj, Obj *comptense, Discourse *dc)
{
  Obj	*save_task;
  PNode	*pn;
  if (!obj) {
    Dbg(DBGGENER, DBGBAD, "Generate: empty obj");
    return(NULL);
  }
  save_task = dc->task;
  dc->task = N("generate");
  if ((pn = Generate1(constit, obj, comptense, dc))) {
    if (pn->feature != constit) pn = PNodeConstit(constit, pn, NULL);
  }
  dc->task = save_task;
  return(pn);
}

/* todo */
PNode *GenReference(int constit, Obj *obj, Obj *comptense, Discourse *dc)
{
  PNode	*pn, *pn_see;

  pn = Generate(constit, I(obj, 1), comptense, dc);

  if (N("href") == I(obj, 0) && ObjIsString(I(obj, 2))) {
    pn_see = GenInterventionObj(F_S, N("cf"), dc);
    pn_see->pn2 = GenLiteral(ObjToString(I(obj, 2)), F_NP, F_NOUN);
  } else if (N("usenet-ref") == I(obj, 0) && 
             ObjIsString(I(obj, 2)) &&
             ObjIsString(I(obj, 3))) {
    pn_see = GenInterventionObj(F_S, N("cf"), dc);
    pn_see->pn2 =
      PNodeConstit(F_NP, 
                   GenLiteral(ObjToString(I(obj, 2)), F_NP, F_NOUN),
                   GenLiteral(ObjToString(I(obj, 3)), F_NP, F_NOUN));

  } else if (N("bookref") == I(obj, 0)) {
  /* todo */
    pn_see = GenInterventionObj(F_S, N("cf"), dc);
    pn_see->pn2 = GenLiteral(ObjToString(I(obj, 2)), F_NP, F_NOUN);
  } else {
    pn_see = GenInterventionObj(F_S, N("cf"), dc);
    pn_see->pn2 = GenLiteral(ObjToString(I(obj, 2)), F_NP, F_NOUN);
/*
    Dbg(DBGGENER, DBGBAD, "GenReference 1 <%s>", M(obj));
    pn_see = NULL;
 */
  }

  if (pn_see) {
    if (pn) {
      PNodeAddPuncToEnd(pn, "---");
      pn = PNodeConstit(constit, pn, pn_see);
    } else {
      Dbg(DBGGENER, DBGBAD, "GenReference 2");
      pn = pn_see;
    }
  }
  return(pn);
}

PNode *GenCoordAppendNext(int constit, Obj *coord, PNode *pn, PNode *pn1,
                          int i, int len, Discourse *dc)
{
  if (pn1 == NULL) return(pn);
  if (pn == NULL) {
    pn = pn1;
  } else {
  /* (len-1) is the number of elements in the series: "A,B,C,and D"
   * (len-1) = 4
   */
    if ((len-1) > 2 &&
        (i != (len-1) || DiscourseSerialComma(dc))) {
      PNodeAddPuncToEnd(pn, ",");
    }
    if (i == (len-1)) {
      if (NULL == (pn = GenMakeConj(constit, coord, pn, pn1, dc))) {
        pn = pn1;
      }
    } else {
      pn = PNodeConstit(constit, pn, pn1);
    }
    pn->is_coord = 1;
  }
  return(pn);
}

PNode *GenCoord(int constit, Obj *coord, Obj *obj, Obj *comptense,
                Discourse *dc)
{
  int	i, len;
  PNode	*pn;
  pn = NULL;
  for (i = 1, len = ObjLen(obj); i < len; i++) {
    pn = GenCoordAppendNext(constit, coord, pn,
                            Generate(constit, I(obj, i), comptense, dc),
                            i, len, dc);
  }
  return(pn);
}

/* todo: Make sure we are at the top-level concept.
 * There is no such thing as nested direct speech.
 * constit == F_S is a stopgap measure.
 */
Bool GenIsDirectSpeech(Obj *obj, Discourse *dc)
{
  return(DiscourseSpeakerIs(dc, I(obj, 1)) &&
         DiscourseListenerIs(dc, I(obj, 2)));
}

PNode *GenWithAdverb(int constit, Obj *obj, Discourse *dc)
{
  GenAdvice	save_ga;
  PNode		*pn, *pnadv;
  save_ga = dc->ga;
  if (N("adverb-of-affirmation") == I(obj, 0)) {
    dc->ga.gen_advle = ObjToLexEntryGet(N("adverb-of-confirmation"), F_ADVERB,
                                        F_NULL, dc);
  }
  if ((pn = Generate(constit, I(obj, 1), NULL, dc))) {
    if ((pnadv = GenMakeAdverb(I(obj, 0), F_POSITIVE, dc))) {
    /* todo: Add a feature to the adverb determining whether a comma is
     * required here or not. "c'est vrai que" and "quand".
     */
      /* PNodeAddPuncToEnd(pnadv, ","); cf "Maybe, because ..." */
      pn = PNodeConstit(F_S, pnadv, pn);
      dc->ga = save_ga;
      return(pn);
    }
  }
  dc->ga = save_ga;
  return(NULL);
}

PNode *Generate1(int constit, Obj *obj, Obj *comptense, Discourse *dc)
{
  int		tsrgen_ok;
  Obj		*head;
  PNode		*pn, *pnadv;

  if (constit == F_S &&
      ISA(N("say-to"), I(obj, 0)) &&
      GenIsDirectSpeech(obj, dc)) {
    return(Generate1(constit, I(obj, 2), comptense, dc));
  }

  if (!ObjIsList(obj)) {
    return(GenObjNP(FS_HEADNOUN, obj, obj, N("complement"), 1, F_NULL, NULL,
                    NULL, NULL, F_NULL, F_NULL, F_NULL, N("present-indicative"),
                    1, 0, NULL, NULL, "", 0, 0, dc));
  }

  head = I(obj, 0);
  tsrgen_ok = 1;
  if (ISA(N("tense"), head)) {
  /* Do not alter comptense. Let timestamps take care of this. */
    return(Generate1(constit, I(obj, 1), comptense, dc));
  } else if (ISA(N("intervention"), head)) {
    pn = GenIntervention(constit, obj, dc);
  } else if (ISA(N("F66"), head)) {
    pn = GenWithAdverb(constit, obj, dc);
  } else if (ISA(N("such-that"), head)) {
    pn = GenIntension(constit, obj, dc);
  } else if (ISA(N("question-element"), head)) {
    pn = GenQuestionElement(constit, obj, dc);
/*
  } else if (ISA(N("reference-of"), head)) {
    pn = GenReference(constit, obj, comptense, dc);
 */
  } else if (ISA(N("and"), head) || ISA(N("or"), head)) {
    pn = GenCoord(constit, head, obj, comptense, dc);
    tsrgen_ok = 0;
  } else if (ISA(N("F75"), head) ||
             ISA(N("temporal-relation"), head)) {
    pn = GenConj(constit, head, I(obj, 1), I(obj, 2), dc);
  } else {
    if (!comptense) {
      comptense = TenseGenOne(obj, ObjToTsRange(obj), DCNOW(dc), DC(dc).lang,
                              (F_LITERARY == DC(dc).style), &dc->ga.aspect);
    }
    if (ISA(N("set-relation"), head) && I(obj, 1) && I(obj, 2)) {
      pn = GenIsa(I(obj, 1), I(obj, 2), comptense, dc);
    } else {
      pn = Generate2(obj, comptense, dc);
    }
  }

  if (pn && tsrgen_ok && dc->ga.gen_tsr && (pnadv = GenTsRange(obj, 1, dc))) {
    pn = PNodeConstit(constit, pn, pnadv);
  }
  if (pn && tsrgen_ok && dc->ga.tga.include_dur && (pnadv = GenDur(obj, dc))) {
    pn = PNodeConstit(constit, pn, pnadv);
  }

  return(pn);
}

PNode *GenerateNoTS(int constit, Obj *obj, Obj *comptense, Discourse *dc)
{
  PNode		*pn;
  GenAdvice	save_ga;
  save_ga = dc->ga;
  dc->ga.gen_tsr = 0;
  TsrGenAdviceClear(&dc->ga.tga);
  pn = Generate(constit, obj, comptense, dc);
  dc->ga = save_ga;
  return(pn);
}

void GenGridTraversalIndices(Obj *obj,
                             /* RESULTS */ int *gridi, int *fromrowi,
                             int *fromcoli, int *torowi, int *tocoli)
{
  if (obj == N("grid-walk")) {
    *gridi = 2;
    *fromrowi = 3;
    *fromcoli = 4;
    *torowi = 5;
    *tocoli = 6;
  } else {
    *gridi = 3;
    *fromrowi = 4;
    *fromcoli = 5;
    *torowi = 6;
    *tocoli = 7;
  }
}

Obj *GenGridTraversal(Obj *obj)
{
  Obj		*from, *to, *prop;
  ObjList	*froms, *tos;
  int		gridi, fromrowi, fromcoli, torowi, tocoli;
  GenGridTraversalIndices(I(obj, 0), &gridi, &fromrowi, &fromcoli, &torowi,
                          &tocoli);
  froms = SpaceFindEnclose1(NULL, ObjToTsRange(obj), ObjToTsRange(obj), NULL,
                            I(obj, gridi),
                            (GridCoord)ObjToNumber(I(obj, fromrowi)),
                            (GridCoord)ObjToNumber(I(obj, fromcoli)),
                            NULL, NULL);
  tos = SpaceFindEnclose1(NULL, ObjToTsRange(obj), ObjToTsRange(obj), NULL,
                          I(obj, gridi), (GridCoord)ObjToNumber(I(obj, torowi)),
                          (GridCoord)ObjToNumber(I(obj, tocoli)), NULL, NULL);
  if (froms && tos && froms->obj != tos->obj) {
    from = froms->obj;
    to = tos->obj;
  } else {
    from = SpaceFindNearestInGrid(NULL, ObjToTsRange(obj), I(obj, 1),
                                  I(obj, gridi),
                                  (GridCoord)ObjToNumber(I(obj, fromrowi)),
                                  (GridCoord)ObjToNumber(I(obj, fromcoli)));
    to = SpaceFindNearestInGrid(NULL, ObjToTsRange(obj), I(obj, 1),
                                I(obj, gridi),
                                (GridCoord)ObjToNumber(I(obj, torowi)),
                                (GridCoord)ObjToNumber(I(obj, tocoli)));
  }
  if (from && to) {
    prop = DbGetRelationValue1(&TsNA, NULL, N("canonical-of"), I(obj, 0),
                               I(obj, 0));
    return(L(prop, I(obj, 1), from, to, E));
  }
  return(ObjCopyList(obj));
}

Obj *GenAtGrid(Obj *obj)
{
  Obj			*polity, *grid, *nearx;
  ObjList		*inside;
  GridCoord  	row, col;
  SpaceAtGridGet(obj, &polity, &grid, &row, &col);
  if ((inside = SpaceFindEnclose1(NULL, ObjToTsRange(obj), ObjToTsRange(obj),
                                  NULL, grid, row, col, NULL, NULL))) {
    return(L(N("location-of"), I(obj, 1), inside->obj, E));
  } else if ((nearx = SpaceFindNearestInGrid(NULL, ObjToTsRange(obj),
                                             I(obj, 1),
                                             grid, row, col))) {
    return(L(N("location-of"), I(obj, 1), nearx, E));
  }
  return(ObjCopyList(obj));
}

/* Always returns copy of <obj>.
 * Keep consistent with Sem_ParseToCanonical.
 */
Obj *GenToNonCanonical(Obj *obj)
{
  if (ISA(N("grid-traversal"), I(obj, 0))) {
    return(GenGridTraversal(obj));
  } else if (ISA(N("at-grid"), I(obj, 0))) {
    return(GenAtGrid(obj));
  } else return(ObjCopyList(obj));
}

/* COMPLEX SENTENCE GENERATION */

Obj *GenCreateTemporalRelation(Obj *obj1, Obj *obj2, Discourse *dc)
{
  return(L(AspectTempRelGen(ObjToTsRange(obj1), ObjToTsRange(obj2), DCNOW(dc)),
           obj1, obj2, E));
}

/* "25 breads"
 * count: 25 noun:  bread
 */
PNode *GenQuantifiedNoun(Obj *count, Obj *noun, Discourse *dc)
{
  int			gender, number, person;
  Float			val;
  PNode			*pn_noun, *pn_num;
  val = ObjToNumber(count);
  if (val == 1.0) {
    number = F_SINGULAR;
  } else {
    /* todo: Force noncount noun generation. */
    number = F_PLURAL;
  }
  if (!(pn_noun = GenObjNP(FS_HEADNOUN, noun, noun, N("complement"), 1, F_NULL,
                           N("forced-empty-article"), NULL, NULL, F_NULL,
                           number, F_NULL, N("present-indicative"), 1, 1,
                           NULL, NULL, "", 0, 1, dc))) {
    return(NULL);
  }
  PNodeGetHeadNounFeatures(pn_noun, 1, &gender, &number, &person);
  if (!(pn_num = GenNumber(val, 0, F_ADJECTIVE, gender, dc))) {
    return(NULL);
  }
  return(PNodeConstit(F_NP, pn_num, pn_noun));
}

/* determiner: interrogative-identification-determiner
 * noun:       draftsman
 */
PNode *GenDeterminedNoun(Obj *determiner, Obj *noun, Discourse *dc)
{
  int			paruniv;
  ObjToLexEntry	*adj_ole;

  if (!(adj_ole = ObjToLexEntryGet1(determiner, NULL, FS_ADJDET, F_NULL,
                                    F_NULL, NULL, dc))) {
    Dbg(DBGGENER, DBGBAD, "GenDeterminedNoun: no adj_ole");
    return(NULL);
  }
  return(GenObjNP(FS_HEADNOUN, noun, noun, N("complement"), 1, paruniv,
                  N("forced-empty-article"), NULL, NULL, F_NULL,
                  F_NULL, F_NULL, N("present-indicative"), 1, 1, adj_ole->le,
                  NULL, adj_ole->features, 0, 0, dc));
}

/* value_prop: [heavy ball] "heavy ball"
 *             [atomic-weight-of atom maximal] "densest atom"
 *             [53 bread] "53 breads"
 *             [interrogative-determiner bread] "which bread"
 * todo: Generate adjective PPs. But these never occur in a modified noun
 * context?
 */
PNode *GenModifiedNoun(Obj *value_prop, Discourse *dc)
{
  int			superlative;
  ObjToLexEntry	*adj_ole;
  LexEntry		*adv_le;
  if (ObjIsNumber(I(value_prop, 0))) {
    return(GenQuantifiedNoun(I(value_prop, 0), I(value_prop, 1), dc));
  } else if (ISA(N("interrogative-determiner"), I(value_prop, 0))) {
    return(GenDeterminedNoun(I(value_prop, 0), I(value_prop, 1), dc));
  }
  superlative = 0;
  if ((adv_le = LexEntrySuperlativeAdverb(I(value_prop, 2), dc))) {
    superlative = 1;
    if (I(value_prop, 2) == N("maximal")) superlative = 2;
  } else if ((adv_le = LexEntrySuperlativeAdverb(I(value_prop, 3), dc))) {
    superlative = 1;
    if (I(value_prop, 3) == N("maximal")) superlative = 2;
  }
  if (!(adj_ole = ObjToLexEntryGet1(I(value_prop, 0),
                                    superlative ? NULL : value_prop,
                                    FS_ADJECTIVE, F_NULL, F_NULL,
                                    NULL, dc))) {
    Dbg(DBGGENER, DBGBAD, "GenModifiedNoun: no adj_ole");
    return(NULL);
  }
  return(GenObjNP(FS_HEADNOUN, I(value_prop, 1), I(value_prop, 1), 
                  N("complement"), 1, F_NULL,
                  superlative ? N("forced-definite-article") : NULL, NULL,
                  NULL, F_NULL, F_NULL, F_NULL, N("present-indicative"),
                  1, 1, adj_ole->le, adv_le, adj_ole->features, superlative,
                  0, dc));
}

/* obj: [such-that human [President-of country-USA human]] */
PNode *GenGenitiveMod(Obj *mod_obj, ObjList *props, Discourse *dc)
{
  Obj		*prop;
  if (!ObjIsAbstract(mod_obj)) return(NULL);
  if (!ObjListIsLengthOne(props)) return(NULL);	/* todo: X and Y and ... */
  prop = props->obj;
  if (!ISA(N("relation"), I(prop, 0))) return(NULL);
  if (mod_obj == I(prop, 2)) {
    return(GenRelationNP(I(prop, 0), I(prop, 1), 1, dc));
  } else if (mod_obj == I(prop, 1)) {
    return(GenRelationNP(I(prop, 0), I(prop, 2), 2, dc));
  }
  return(NULL);
}

/* noun:  ball
 * props: [heavy ball] [blue ball]
 */
PNode *GenIntension1(int constit, Obj *noun, ObjList *props, Discourse *dc)
{
  ObjList	*p;
  PNode		*pn, *pn1;
  if ((pn = GenGenitiveMod(noun, props, dc))) return(pn);
  /* todo: gen noun if it is a list? */
  pn = NULL;
  for (p = props; p; p = p->next) {
    if (!ObjIsList(p->obj)) continue;	/* ? */
    /* p->obj: [heavy ball]
     *         [53 ball]
     *         [atomic-weight-of atom maximal]
     */
    if (constit == F_NP) {
      pn1 = GenModifiedNoun(p->obj, dc);
    } else {
      /* Assumes superlatives always occur as F_NP constit. Otherwise,
       * sentence level adjective generation must do superlatives too.
       */
      pn1 = Generate(constit, p->obj, NULL, dc);
    }
    if (pn1) {
      if (pn) pn = GenMakeConj(constit, N("and"), pn, pn1, dc);
      else pn = pn1;
    }
  }
  return(pn);
}

/* "a heavy blue ball"
 * [such-that
 *  ball
 *  [heavy ball]
 *  [blue ball]]
 *
 * "53 heavy blue balls"
 * [such-that
 *  ball
 *  [53 ball]
 *  [heavy ball]
 *  [blue ball]] (basically)
 *
 * assumes I(obj, 0) == N("such-that")
 */
PNode *GenIntension(int constit, Obj *obj, Discourse *dc)
{
  PNode		*r;
  Obj		*noun;
  ObjList	*props;
  if ((props = ObjIntensionParse(obj, 1, &noun))) {
    r = GenIntension1(constit, noun, props, dc);
    ObjListFree(props);
    return(r);
  }
  return(NULL);
}

/* "which heavy blue ball"
 * [question-element
 *  ball
 *  interrogative-identification-determiner
 *  [heavy ball]
 *  [blue ball]]
 *
 * "what designer"
 * [question-element industrial-designer
 *  interrogative-identification-determiner]
 *
 * "what is the heaviest atom?"
 * [question-element
 *  atom
 *  object-interrogative-pronoun
 *  [atomic-weight-of atom minimal]]
 *
 * assumes I(obj, 0) == N("question-element")
 */
PNode *GenQuestionElement(int constit, Obj *obj, Discourse *dc)
{
  PNode		*pn, *subjnp, *vp;
  Obj		*class, *interrogative, *subj;
  ObjList	*props;
  if (!ObjQuestionElementParse(obj, &class, &interrogative, &props)) {
    return(NULL);
  }
  if (constit != F_S) {
    props = ObjListCreate(L(interrogative, class, E), props);
  }
  pn = GenIntension1(F_NP, class, props, dc);
  if (constit == F_S) {
    if (ISA(N("interrogative-determiner"), interrogative)) {
    /* "what is ..." */
      subj = L(interrogative, class, E);
    } else {
    /* "which element is ..." */
      subj = interrogative;
    } 
    if (!(subjnp = Generate(F_NP, subj, NULL, dc))) {
      return(NULL);
    }
    if (!(vp = GenVP(ObjToLexEntryGet(N("standard-copula"), F_VERB, F_NULL,
                                      dc), "",
                     N("present-indicative"), subjnp, NULL, dc))) {
      return(NULL);
    }
    pn = PNodeConstit(F_S, subjnp, PNodeConstit(F_VP, vp, pn));
  }
  ObjListFree(props);
  return(pn);
}

PNode *GenConj(int constit, Obj *rel, Obj *obj1, Obj *obj2, Discourse *dc)
{
  GenAdvice	save_ga;
  PNode		*pn;
  if (rel == N("not")) {
    save_ga = dc->ga;
    if (DC(dc).lang != F_ENGLISH) {
      dc->ga.gen_negle1 =
        ObjToLexEntryGet(N("first-adverb-of-absolute-negation"), F_ADVERB,
                         F_NULL, dc);
    }
    dc->ga.gen_negle2 = ObjToLexEntryGet(N("adverb-of-absolute-negation"),
                                         F_ADVERB, F_NULL, dc);
    pn = Generate(constit, obj1, NULL, dc);
    dc->ga = save_ga;
    return(pn);
  }
  Dbg(DBGGENER, DBGHYPER, "relation <%s>", M(rel));
  /* I moved indicative up to prevent "because of three being the height of
   * an elephant" instead of "because three is the height of an elephant".
   */
  if ((pn = GenConj1(F_SUBCAT_INDICATIVE, constit, rel, obj1, obj2, dc))) {
    return(pn);
  }
  if ((pn = GenConj1(F_SUBCAT_INFINITIVE, constit, rel, obj1, obj2, dc))) {
    return(pn);
  }
  if ((pn = GenConj1(F_SUBCAT_PRESENT_PARTICIPLE, constit, rel, obj1, obj2,
                     dc))) {
    return(pn);
  }
  if ((pn = GenConj1(F_SUBCAT_SUBJUNCTIVE, constit, rel, obj1, obj2, dc))) {
    return(pn);
  }
  return(NULL);
}

Obj *GenSubtense(int subcat, Obj *comptense, Discourse *dc)
{
  switch (subcat) {
    case F_SUBCAT_SUBJUNCTIVE:
      return(TenseSubjunctivize(comptense, DC(dc).style, DC(dc).dialect,
                                DC(dc).lang));
    case F_SUBCAT_INDICATIVE:
      return(TenseIndicativize(comptense, DC(dc).style, DC(dc).dialect,
                               DC(dc).lang));
    case F_SUBCAT_INFINITIVE:
      return(TenseInfinitivize(comptense, DC(dc).lang));
    case F_SUBCAT_PRESENT_PARTICIPLE:
      return(TenseGerundize(comptense));
    default:
      break;
  }
  return(comptense);
}

void GenTrouble(char *s, Obj *con, Obj *subcon, LexEntry *le)
{
  if (le) {
    Dbg(DBGGENER, DBGBAD, "gen trouble %s <%s>.<%s>", s, le->srcphrase,
        le->features);
  } else {
    Dbg(DBGGENER, DBGBAD, "gen trouble %s", s);
  }
  fputs("con----> ", Log);
  if (subcon) {
    fprintf(Log, "[%s] in ", M(I(subcon, 0)));
  }
  ObjPrint(Log, con);
  fputc(NEWLINE, Log);
}

PNode *GenConj1(int subcat, int constit, Obj *rel, Obj *obj1, Obj *obj2,
                Discourse *dc)
{
  Word			*infl;
  PNode			*pn1, *pn2, *pnk;
  Obj			*t1, *t2;
  ObjToLexEntry	*ole;
  if (!(ole = ObjToLexEntryGet1(rel, NULL, FS_CONJUNCTION, subcat, F_NULL,
                                NULL, dc))) {
    return(NULL);
  }
  if (obj2) {
    TenseGenConcord(obj1, obj2, ObjToTsRange(obj1), ObjToTsRange(obj2),
                    DCNOW(dc), DC(dc).lang, (F_LITERARY == DC(dc).style),
                    rel, &t1, &t2);
  } else {
    t1 = t2 = NULL;
  }
  /* todo: Convert this so that tenses, rel, and all are given in concept? */
  t1 = GenSubtense(subcat, t1, dc);
  Dbg(DBGGENER, DBGHYPER, "tenses <%s> <%s>", M(t1), M(t2));
  if (!(pn1 = Generate(constit, obj1, t1, dc))) {
    GenTrouble("GenConj1 obj1", rel, obj1, ole->le);
    return(NULL);
  }
  pn2 = NULL;
  if (obj2 && (!(pn2 = Generate(constit, obj2, t2, dc)))) {
    GenTrouble("GenConj1 obj2", rel, obj2, ole->le);
    return(NULL);
  }
  if (!(infl = LexEntryGetInflection(ole->le, F_NULL, F_NULL, F_NULL, F_NULL,
                                     F_NULL, F_NULL, 1, dc))) {
    return(NULL);
  }
  pnk = PNodeWord(F_CONJUNCTION, infl->word, infl->features, ole->le, rel);
  /* Was if (StringIn(F_COORDINATOR, ole->features)) */
  if (pn2 &&
      (!TransformSubordinateSubjectDeletion(subcat, PNodeSubject(pn2), NULL,
                                            pn1, dc))) {
    /* todoFREE */
    return(NULL);
  }
  if (pn2) {
    PNodeAddPuncToEnd(pn2, ",");
    return(PNodeConstit(constit, pn2, PNodeConstit(constit, pnk, pn1)));
  } else {
    return(PNodeConstit(constit, pnk, pn1));
  }
}

/* INTERVENTION GENERATION */

PNode *GenInterventionObj(int constit, Obj *obj, Discourse *dc)
{
  ObjToLexEntry	*ole;
  Word		*infl;
  PNode		*pn;
  if (!(ole = ObjToLexEntryGet1(obj, NULL, FS_SAOBJ, F_NULL, F_NULL,
                                NULL, dc))) {
    return(NULL);
  }
  if (!(infl = LexEntryGetInflection(ole->le, F_NULL, F_NULL, F_NULL, F_NULL,
                                     F_NULL, F_NULL, 1, dc))) {
    return(NULL);
  }
  pn = PNodeConstit(constit,
                    PNodeWord(FeatureGet(ole->le->features, FT_POS),
                              infl->word, infl->features,
                              ole->le, obj),
                    NULL);
  if (StringIn(F_QUESTION, ole->le->features)) pn->eos = '?';
  return(pn);
}

PNode *GenInterjectionOfGreeting(Discourse *dc)
{
  Obj	*class;
  Float	tod;
  PNode	*pn;
  class = DCCLASS(dc);
  if (class == N("call")) {
    /* todo: How do we know whether we are calling or called party? */
    return(Generate(F_S, N("called-party-telephone-greeting"), NULL, dc));
  }
  if (class != N("email")) {
    tod = (Float)TsToTod(DCNOW(dc));
    pn = GenValueRangeName(tod, N("time-of-day-dependent-greeting"),
                           F_INTERJECTION, F_NULL, F_NULL, F_NULL, dc);
    if (pn) {
      return(PNodeConstit(F_S, pn, NULL));
    }
  }
  return(Generate(F_S, N("interjection-of-greeting"), NULL, dc));
}

/* todo: Generate and parse: "see you" / "à" + time /
 * "have a nice" + time / "bon" + upcoming time.
 */
PNode *GenInterjectionOfDeparture(Discourse *dc)
{
  Obj	*class;
  Float	tod;
  PNode	*pn;
  class = DCCLASS(dc);
  if (class == N("call")) {
    return(Generate(F_S, N("telephone-goodbye"), NULL, dc));
  }
  if (class != N("email")) {
    tod = (Float)TsToTod(DCNOW(dc));
    pn = GenValueRangeName(tod, N("time-of-day-dependent-goodbye"),
                           F_INTERJECTION, F_NULL, F_NULL, F_NULL, dc);
    if (pn) {
      return(PNodeConstit(F_S, pn, NULL));
    }
  }
  return(Generate(F_S, N("interjection-of-departure"), NULL, dc));
}

/* Examples:
 * interjection-of-greeting -----> context-dependent "Hello."
 * how-are-you speaker ----------> "How are you?"
 */
PNode *GenIntervention1(Obj *head, Discourse *dc)
{
  if (head == N("interjection-of-greeting")) {
    return(GenInterjectionOfGreeting(dc));
  } else if (head == N("interjection-of-departure")) {
    return(GenInterjectionOfDeparture(dc));
  } else {
    return(Generate(F_S, head, NULL, dc));
  }
}

PNode *GenIntervention(int constit, Obj *obj, Discourse *dc)
{
  PNode	*pn1, *pn2;
  if (constit == F_S) {
    if (GenIsDirectSpeech(obj, dc)) {
    /* Direct speech:
     * [interjection-of-greeting speaker listener] -----> "Hello."
     * [how-are-you speaker listener] ------------------> "How are you?"
     * [current-time-sentence TT Me] -------------------> "It is 3 o'clock."
     */
      return(GenIntervention1(I(obj, 0), dc));
    } else if (DiscourseSpeakerIs(dc, I(obj, 1))) {
    /* Example:
     * [interjection-of-greeting speaker Peter] -----> "Hello, Peter."
     * When Peter is with the listener. But in this case wouldn't Peter be
     * considered a listener too?
     */
      if (!(pn1 = Generate(constit, I(obj, 0), NULL, dc))) return(NULL);
      PNodeAddPuncToEnd(pn1, ",");
      if (!(pn2 = Generate(constit, I(obj, 2), NULL, dc))) return(NULL);
      return(PNodeConstit(F_S, pn1, pn2));
        /* todo: Follow base rules. */
    }
  }
  /* Indirect speech:
   * [interjection-of-noncomprehension Serv Cust] --> "Serv says what to Cust."
   * [interjection-of-thanks Serv Cust] --> "Serv says thanks to Cust."
   * todo: could also generate using TV script notation, or book notation
   * X said to Y, "-----". Or if it is a long conversation, just -- or " ".
   * todo: [recommend LBJ na Millbrook] is generated as LBJ dit « Millbrook ».
   * todo: ["thank" Serv Cust]
   */
  return(Generate(constit,
                  L(N("say-to"), I(obj, 1), I(obj, 2), I(obj, 0), E), NULL,
                    dc));
}

/* SIMPLE SENTENCE GENERATION */

PNode *Generate2(Obj *obj, Obj *comptense, Discourse *dc)
{
  int	freeme;
  Obj	*obj1;
  PNode	*pn;
  if (!obj) {
    Dbg(DBGGENER, DBGBAD, "Generate2: empty obj");
    return(NULL);
  }
  if (ObjIsList(obj)) {
    obj1 = GenToNonCanonical(obj);
    freeme = 1;
    DbRestrictionGen(obj1, dc);
  } else {
    obj1 = obj;
    freeme = 0;
  }
#ifdef notdef
  pn = GenS_AscriptiveAttachment(obj1, comptense, dc);
#else
  pn = NULL;
#endif
  if (!pn) {
    pn = Generate3(obj1, 0, comptense, dc);
  }
  if (freeme) ObjFree(obj1);
  return(pn);
}

PNode *Generate3(Obj *obj, int recursed, Obj *comptense, Discourse *dc)
{
  int			pos, i, nump, theta_filled[MAXTHETAFILLED];
  ObjToLexEntry	*ole;
  Obj			*predicate, *obj1;
  PNode			*pn;
  if (!obj) {
    Dbg(DBGGENER, DBGBAD, "Generate3: empty obj");
    return(NULL);
  }
  if (!(predicate = I(obj, 0))) {
    Dbg(DBGGENER, DBGBAD, "Generate3: empty predicate");
    return(NULL);
  }
  ConToThetaFilled(obj, theta_filled);
  for (i = 0; i < 3; i++) {
    if ((ole = ObjToLexEntryGet1(predicate, obj, "VAN", F_NULL, F_NULL,
                                 theta_filled, dc))) {
      if (F_VERB ==
          (pos = FeatureGetRequired("Generate3", ole->le->features, FT_POS))) {
        if ((pn = GenS_VerbPred(obj, ole, comptense, dc))) return(pn);
      } else if (F_ADJECTIVE == pos) {
        if ((pn = GenS_AscriptiveAdj(obj, ole, comptense, dc))) {
          return(pn);
        }
      } else if (F_NOUN == pos) {
        if ((pn = GenS_EquativeRole(obj, ole, comptense, dc))) return(pn);
      }
    } else break;
  }
  if (!ObjBarrierISA(predicate)) {
    obj1 = ObjCopyList(obj);	/* todoFREE */
    for (i = 0, nump = ObjNumParents(predicate); i < nump; i++) {
      ObjSetIth(obj1, 0, ObjIthParent(predicate, i));
      if ((pn = Generate3(obj1, 1, comptense, dc))) {
        return(pn);
      }
    }
  }
  /*
  if (recursed == 0 &&
      predicate != N("at-grid") && predicate != N("cpart-of") &&
      predicate != N("warp")) {
    GenTrouble("Generate3", obj, NULL, NULL);
  }
   */
  return(NULL);
}

Bool GenIsCopula(ObjToLexEntry *ole)
{
  LexEntryToObj	*leo;
  for (leo = ole->le->leo; leo; leo = leo->next) {
    if (ISA(N("copula"), leo->obj)) return(1);
  }
  return(0);
}

#define MAXARGS	10

void GenSortArgs(PNode **args, int args_len)
{
  PNode	*tmp;
  int	i, len[MAXARGS];
  for (i = 0; i < args_len; i++) {
    if (i >= MAXARGS) {
      Dbg(DBGGENER, DBGBAD, "GenSortArgs: increase MAXARGS");
      break;
    }
    len[i] = PNodeNumberOfWords(args[i]);
  }
  for (i = 0; i < args_len-1; i++) {
    /* Shift heavier subordinate clauses later in sentence.
     * This is related to Heavy NP shift (Chomsky, 1982/1987, pp. 135-136).
     */
    if (len[i] > 3 && len[i] > len[i+1]) {
      tmp = args[i+1];
      args[i+1] = args[i];
      args[i] = tmp;
    }
  }
}

/* todo: Negatives. */
PNode *GenExpl(PNode *pn, ObjToLexEntry *ole, int position, PNode *subjnp,
               Discourse *dc)
{
  ThetaRole	*theta_roles;
  for (theta_roles = ole->theta_roles;
       theta_roles;
       theta_roles = theta_roles->next) {
    if (theta_roles->cas != N("expl")) continue;
    if (position != (int)theta_roles->position) continue;
    if (position == TRPOS_PRE_VERB) {
      pn = PNodeConstit(F_VP, GenMakeExpl(theta_roles->le, subjnp, dc), pn);
    } else {
      pn = PNodeConstit(F_VP, pn, GenMakeExpl(theta_roles->le, subjnp, dc));
    }
  }
  return(pn);
}

Bool GenArguments1(int start_i, Obj *cas, Obj *gen_cas, PNode *subjnp, Obj *obj,
                   ObjToLexEntry *ole, Obj *comptense, Discourse *dc,
                   /* RESULTS */ PNode **args, int *args_len)
{
  int		i, done, arg_subcat;
  Bool		isoptional;
  Obj		*arg_val, *subcomptense;
  LexEntry	*prep_le;
  PNode		*argnp;
  for (i = start_i; ; i++) {
    arg_val = ThetaRoleGetCaseValueIth(obj, ole->theta_roles, cas, i,
                                       &prep_le, &arg_subcat, &isoptional,
                                       &done);
    if (done) break;
    subcomptense = GenSubtense(arg_subcat, comptense, dc);
    if ((argnp = GenNP(FS_HEADNOUN, arg_val, gen_cas, 1, F_NULL, NULL, NULL,
                       subjnp, F_NULL, F_NULL, F_NULL, subcomptense, 1, 1,
                       dc))) {
      if ((*args_len) >= MAXARGS) {
        Dbg(DBGGENER, DBGBAD, "GenArguments: increase MAXARGS");
        return(0);
      }
      if (ObjIsTsRange(arg_val) ||
      /* GenTsRange1 is responsible for adding an appropriate preposition. */
          N("location-interrogative-pronoun") == arg_val) {
      /* todo: Noninterrogative locations. */
        args[(*args_len)++] = argnp;
      } else if (prep_le) {
        args[(*args_len)++] = GenMakePP1(prep_le, argnp, dc);
      } else {
        args[(*args_len)++] = argnp;
      }
    } else {
      if (arg_val && !isoptional) {
        GenTrouble(M(cas), obj, arg_val, ole->le);
        return(0);
      }
    }
  }
  return(1);
}

/* Some "obj" arguments may have already been generated and added to args.
 * obj_start_i gives the number of "obj"'s already generated and therefore
 * the starting index of the next "obj" (if any) to generate.
 */
PNode *GenArguments(int constit, PNode *xp, Obj *obj_cas, int obj_start_i,
                    PNode *subjnp, Obj *obj, ObjToLexEntry *ole,
                    Obj *comptense, Discourse *dc, /* MODIFIES */
                    PNode **args, int *args_len)
{
  int	i, post_obj_done;
  if (obj_start_i >= 0) {
    if (!GenArguments1(obj_start_i, N("obj"), obj_cas, subjnp, obj, ole,
                       comptense, dc, args, args_len)) {
      return(NULL);
    }
  }
  if (!GenArguments1(0, N("iobj"), N("iobj"), subjnp, obj, ole,
                     comptense, dc, args, args_len)) {
    return(NULL);
  }
  xp = GenExpl(xp, ole, TRPOS_PRE_VERB, subjnp, dc);
  xp = GenExpl(xp, ole, TRPOS_POST_VERB_PRE_OBJ, subjnp, dc);

  if (!StringIn(F_DO_NOT_REORDER, ole->features)) {
    GenSortArgs(args, *args_len);
  }
  post_obj_done = 0;
  for (i = 0; i < *args_len; i++) {
    xp = PNodeConstit(constit, xp, args[i]);
    if (post_obj_done == 0 && args[i]->feature == F_NP) {
      xp = GenExpl(xp, ole, TRPOS_POST_VERB_POST_OBJ, subjnp, dc);
      post_obj_done = 1;
    }
  }
  if (!post_obj_done) {
    xp = GenExpl(xp, ole, TRPOS_POST_VERB_POST_OBJ, subjnp, dc);
  }

  return(xp);
}

PNode *GenS_VerbPred(Obj *obj, ObjToLexEntry *ole, Obj *comptense,
                     Discourse *dc)
{
  int		i, is_copula, subji, args_len, subcat, obj_subcat;
  Obj		*obj_cas, *subcomptense, *subj_val, *obj_val;
  PNode		*subjnp, *vp, *objnp, *r, *args[MAXARGS];
  /* If there are more lexical entries with this property, perhaps it would
   * better be handled with a feature. LexEntry would then have to deal
   * with this new case type "complement".
   */
  if (ISA(N("copula"), I(obj, 0)) || GenIsCopula(ole)) {
    obj_cas = N("complement");
    is_copula = 1;
  } else {
    obj_cas = N("obj");
    is_copula = 0;
  }

  subj_val = ThetaRoleGetCaseValue(obj, ole->theta_roles, N("subj"), &subji,
                                   NULL);
  obj_val = ThetaRoleGetCaseValue(obj, ole->theta_roles, N("obj"), NULL,
                                  &obj_subcat);
  subcomptense = GenSubtense(obj_subcat, comptense, dc);
  if (is_copula && ISA(N("interrogative-pronoun"), I(obj, subji))) {
  /* Subject agrees with object.
   * todo: The above condition can be generalized if it is found to apply
   * to more situations.
   */
    objnp = GenNP(FS_HEADNOUN, obj_val, obj_cas, 1, F_NULL, NULL, NULL, NULL,
                  F_NULL, F_NULL, F_NULL, subcomptense, 1, 1, dc);
    subjnp = GenNP(FS_HEADNOUN, subj_val, N("subj"), 1, F_NULL, NULL, objnp,
                   NULL, F_NULL, F_NULL, F_NULL, comptense, 1, 1, dc);
  } else {
    subjnp = GenNP(FS_HEADNOUN, subj_val, N("subj"), 1, F_NULL, NULL, NULL,
                   NULL, F_NULL, F_NULL, F_NULL, comptense, 1, 1, dc);
    objnp = GenNP(FS_HEADNOUN, obj_val, obj_cas, 1, F_NULL, NULL, NULL,
                  subjnp, F_NULL, F_NULL, F_NULL, subcomptense, 1, 1, dc);
  }
  if (subjnp == NULL && subj_val) {
    GenTrouble("GenS_VerbPred subj", obj, subj_val, ole->le);
    return(NULL);
  }
  if (objnp == NULL && obj_val) {
    GenTrouble("GenS_VerbPred obj", obj, obj_val, ole->le);
    return(NULL);
  }

  args_len = 0;
  if (objnp) {
    args[args_len++] = objnp;
  }

  /* todo: If (subj_val == N("pronoun-there-expletive")) we would like
   * verb somehow to agree with obj_val.
   */
  if (!(vp = GenVP(ole->le, ole->features, comptense, subjnp, NULL, dc))) {
    return(NULL);
  }
  if (!(vp = GenArguments(F_VP, vp, obj_cas, 1, subjnp, obj, ole, comptense,
                          dc, args, &args_len))) {
    return(NULL);
  }

  if (subjnp) {
    r = PNodeConstit(F_S, subjnp, vp);
  } else {
  /* For other languages besides English and French. */
    r = PNodeConstit(F_S, vp, NULL);
  }

  subcat = ThetaRoleGetAnySubcat(ole->theta_roles);
    /* todo: should be specific to each args[i] */
  for (i = 0; i < args_len; i++) {
    if (args[i] && XBarIs_NP_S_Or_PP_NP_S(args[i])) {
      if (!TransformSubordinateSubjectDeletion(subcat, subjnp,
                                               i >= 1 ? args[i-1] : NULL,
                                               args[i], dc)) {
        return(NULL);
      }
    }
  }
  return(r);
}

/* Ascriptive adjective sentence:
 * Jim is funny.
 * [funny Jim]
 */
PNode *GenS_AscriptiveAdj(Obj *obj, ObjToLexEntry *ole, Obj *comptense,
                          Discourse *dc)
{
  int		args_len;
  Obj		*subj_val;
  PNode	*subjnp, *vp, *adjp, *args[MAXARGS];
  subj_val = ThetaRoleGetCaseValue(obj, ole->theta_roles, N("aobj"), NULL,
                                   NULL);
  if (!(subjnp = GenNP(FS_HEADNOUN, subj_val, N("subj"), 1, F_NULL, NULL,
                       NULL, NULL, F_NULL, F_NULL, F_NULL, comptense, 
                       1, 1, dc))) {
    if (subj_val) {
      GenTrouble("GenS_AscriptiveAdj subj", obj, subj_val, ole->le);
      return(NULL);
    }
  }
  if (!(vp = GenVP(ObjToLexEntryGet(N("standard-copula"), F_VERB, F_NULL, dc),
                   "", comptense, subjnp, NULL, dc))) {
    return(NULL);
  }
  if (!(adjp = GenADJPSimple(I(obj, 0), ObjWeight(obj), ole, subjnp, dc))) {
    return(NULL);
  }
  args_len = 0;
  if (!(adjp = GenArguments(F_ADJP, adjp, N("obj"), 0, subjnp, obj, ole,
                            comptense, dc, args, &args_len))) {
    return(NULL);
  }
  vp = PNodeConstit(F_VP, vp, adjp);
  if (subjnp) {
    return(PNodeConstit(F_S, subjnp, vp));
  }
  return(PNodeConstit(F_S, vp, NULL));
}

PNode *GenRelationObjNP(Obj *rel, LexEntry *le, char *features,
                        Obj *rel_article, PNode *subjnp, Discourse *dc)
{
  return(GenObjNP1(rel, rel, le, features, N("obj"), 0,
                   rel_article, subjnp, subjnp, F_NULL, F_NULL,
                   F_NULL, NULL, NULL, "", 0, 0, dc));
}

PNode *GenObjNPSimple(Obj *obj, Discourse *dc)
{
  return(GenObjNP(FS_HEADNOUN, obj, obj, N("complement"), 1, F_NULL, NULL, NULL,
                  NULL, F_NULL, F_NULL, F_NULL, N("present-indicative"), 1, 1,
                  NULL, NULL, "", 0, 0, dc));
}

/* Equative role sentence:
 * Karen is the sister of Jim. [sister-of Jim Karen]
 * todo: Note that relation nouns do not inherit. Perhaps they should, like
 * other lexical items.
 */
PNode *GenS_EquativeRole(Obj *obj, ObjToLexEntry *ole, Obj *comptense,
                         Discourse *dc)
{
  int	reli, subji, iobji, nai, len;
  Obj	*rel_article;
  PNode	*subjnp, *vp, *objnp, *iobjnp, *pp;
  if (!Sem_ParseGetRoleIndices(ole->features, I(obj, 0), &reli, &subji, &iobji,
                              &nai, &len, &rel_article)) {
    return(NULL);
  }
  if (ISA(N("interrogative-pronoun"), I(obj, subji))) {
    if (!(objnp = GenRelationObjNP(I(obj, reli), ole->le, ole->features,
                                   rel_article, NULL, dc))) {
      return(NULL);
    }
    if (!(subjnp = GenNP(FS_HEADNOUN, I(obj, subji), N("subj"), 1, F_NULL,
                         NULL, objnp, NULL, F_NULL, F_NULL, F_NULL,
                         comptense, 1, 1, dc))) {
      return(NULL);
    }
  } else {
    if (!(subjnp = GenNP(FS_HEADNOUN, I(obj, subji), N("subj"), 1, F_NULL,
                         NULL, NULL, NULL, F_NULL, F_NULL, F_NULL, comptense,
                         1, 1, dc))) {
      return(NULL);
    }
    if (!(objnp = GenRelationObjNP(I(obj, reli), ole->le, ole->features,
                                   rel_article, subjnp, dc))) {
      return(NULL);
    }
  }
  if (!(vp = GenVP(ObjToLexEntryGet(N("standard-copula"), F_VERB, F_NULL, dc),
                                    "", comptense, subjnp, NULL, dc))) {
    return(NULL);
  }
  if (!(iobjnp = GenNP(FS_HEADNOUN, I(obj, iobji), N("iobj"), 1, F_NULL, NULL,
                       subjnp, subjnp, F_NULL, F_NULL, F_NULL, comptense, 1,
                       1, dc))) {
    return(NULL);
  }
  if (!(pp = GenMakePP(N("prep-of"), iobjnp, dc))) return(NULL);
  return(PNodeConstit(F_S, subjnp,
                      PNodeConstit(F_VP, vp, PNodeConstit(F_NP,objnp,pp))));
}

#ifdef notdef
Bool GenAscriptiveAttachmentOK(Obj *rel, Obj *value)
{
  int		i, len;
  ObjList	*objs, *p;
  if (value == NULL) return(0);
  if (ISA(N("F75"), I(value, 0))) {
    for (i = 1, len = ObjLen(value); i < len; i++) {
      if (!GenAscriptiveAttachmentOK(rel, I(value, i))) return(0);
    }
    return(1);
  }
  if (!(objs = DbGetRelationAssertions(&TsNA, NULL, N("attachment-rel-of"),
                                       value, 1, NULL))) {
    return(0);
  }
  for (p = objs; p; p = p->next) {
    if (ISA(I(p->obj, 2), rel)) {
      ObjListFree(objs);
      return(1);
    }
  }
  ObjListFree(objs);
  return(0);
}

PNode *GenS_AscriptiveAttachment_VPComp(PNode *subjnp, Obj *value,
                                        Obj *value_prop, Discourse *dc)
{
  Obj		*arttype;
  ObjToLexEntry	*ole;
  PNode		*vpcomp;
  if (!(ole = ObjToLexEntryGet1(value, value_prop, "AN", F_NULL, F_ATTACHMENT,
                                NULL, dc))) {
    return(NULL);
  }
  if (F_ADJECTIVE == FeatureGetRequired("GenS_AscriptiveAttachment_VPComp",
                                        ole->le->features, FT_POS)) {
    vpcomp = GenADJPSimple(value, FLOATNA, ole, subjnp, dc);
  } else {
    if (DC(dc).lang == F_FRENCH && ISA(N("human-occupation"), value)) {
      arttype = N("empty-article");
    } else {
      arttype = N("indefinite-article");
    }
    vpcomp = GenObjNP1(value, value, ole->le, ole->features, N("obj"), 0,
                       arttype, subjnp, subjnp, F_NULL, F_NULL, F_NULL,
                       NULL, NULL, "", 0, 0, dc);
  }
  return(vpcomp);
}

PNode *GenS_AscriptiveAttachment1(Obj *rel, Obj *subj, Obj *value,
                                  Obj *value_prop, Obj *comptense,
                                  Discourse *dc)
{
  int		i, len;
  PNode		*subjnp, *vp, *vpcomp;
  if (!GenAscriptiveAttachmentOK(rel, value)) return(NULL);
  if (!(subjnp = GenNP(FS_HEADNOUN, subj, N("subj"), 1, F_NULL, NULL,
                       NULL, NULL, F_NULL, F_NULL, F_NULL, comptense, 1, 1,
                       dc))) {
    return(NULL);
  }
  if (!(vp = GenVP(ObjToLexEntryGet(N("standard-copula"), F_VERB, F_NULL, dc),
                   "", comptense, subjnp, NULL, dc))) {
    return(NULL);
  }
  if (ISA(N("F75"), I(value, 0))) {
    vpcomp = NULL;
    for (i = 1, len = ObjLen(value); i < len; i++) {
      vpcomp = GenCoordAppendNext(F_NP, I(value, 0), vpcomp,
                                  GenS_AscriptiveAttachment_VPComp(subjnp,
                                                              I(value, i),
                                                              value_prop, dc),
                                  i, len, dc);
    }
  } else {
    vpcomp = GenS_AscriptiveAttachment_VPComp(subjnp, value, value_prop, dc);
  }
  if (vpcomp) {
    return(PNodeConstit(F_S, subjnp, PNodeConstit(F_VP, vp, vpcomp)));
  }
  return(NULL);
}

/* Ascriptive attachment sentence:
 * Joyce is American. (adjectival ascriptive attachment sentence)
 *   [nationality-of Joyce country-USA]
 * Joyce is a lawyer. (nominal ascriptive attachment sentence)
 *   [occupation-of Joyce attorney]
 * Joyce est avocate. (articleless nominal ascriptive attachment sentence)
 */
PNode *GenS_AscriptiveAttachment(Obj *obj, Obj *comptense, Discourse *dc)
{
  int	i, nump;
  PNode	*pn;
  if (!obj) {
    Dbg(DBGGENER, DBGBAD, "GenS_AscriptiveAttachment: empty obj");
    return(NULL);
  }
  if (!ISA(N("relation"), I(obj, 0))) return(NULL);
  if ((pn = GenS_AscriptiveAttachment1(I(obj, 0), I(obj, 1), I(obj, 2), obj,
                                       comptense, dc))) {
    return(pn);
  }
  for (i = 0, nump = ObjNumParents(obj); i < nump; i++) {
    if ((pn = GenS_AscriptiveAttachment(ObjIthParent(obj, i), comptense, dc))) {
      return(pn);
    }
  }
  return(NULL);
}
#endif

/* ADJECTIVE PHRASE GENERATION */

PNode *GenMakeModifiedNoun(int pos, PNode *adjdetp, PNode *np,
                           char *punc1, char *punc2,
                           Discourse *dc)
{
  char	*usagefeat, qfeat[2];
  PNode	*adjdet;
  qfeat[0] = pos;
  qfeat[1] = TERM;
  if ((adjdet = PNodeFindFeat(adjdetp, qfeat, "")) && adjdet->ole) {
    usagefeat = adjdet->ole->features;
  } else {
    usagefeat = "";
  }
  if (LexEntryIsPreposedAdj(usagefeat, pos, dc)) {
    if (punc1) PNodeAddPuncToEnd(adjdetp, punc1);
    if (punc2) PNodeAddPuncToEnd(np, punc2);
    return(PNodeConstit(F_NP, adjdetp, np));
  }
  PNodeAddPuncToEnd(np, punc1);
  PNodeAddPuncToEnd(adjdetp, punc2);
  return(PNodeConstit(F_NP, np, adjdetp));
}

PNode *GenMakeADJP(LexEntry *adj_le, LexEntry *adv_le,
                   Word *adj_infl, Word *adv_infl,
                   Obj *adj_obj, Obj *adv_obj)
{
  PNode	*adjp;
  adjp = PNodeConstit(F_ADJP,
                      PNodeWord(F_ADJECTIVE, adj_infl->word, adj_infl->features,
                                adj_le, adj_obj),
                      NULL);
  if (adv_infl) {
    adjp = PNodeConstit(F_ADJP,
                        PNodeWord(F_ADVERB, adv_infl->word, adv_infl->features,
                                  adv_le, adv_obj),
                        adjp);
  }
  return(adjp);
}

Obj *WeightToAdjAdverb(Float weight)
{
  if (weight == FLOATNA) return(NULL);
  if (weight == WEIGHT_DEFAULT) {
    return(NULL);
  } else if (FloatIsInRange(weight, N("adverb-of-highest-degree"))) {
    return(N("adverb-of-highest-degree"));
  } else if (FloatIsInRange(weight, N("adverb-of-extremely-high-degree"))) {
    return(N("adverb-of-extremely-high-degree"));
  } else if (FloatIsInRange(weight, N("adverb-of-high-degree"))) {
    return(N("adverb-of-high-degree"));
  } else if (FloatIsInRange(weight, N("adverb-of-average-degree"))) {
    return(N("adverb-of-average-degree"));
  } else if (FloatIsInRange(weight, N("adverb-almost"))) {
    return(N("adverb-almost"));
  } else if (FloatIsInRange(weight, N("adverb-of-low-degree"))) {
    return(N("adverb-of-low-degree"));
  } else if (FloatIsInRange(weight, N("adverb-of-near-negation"))) {
    return(N("adverb-of-near-negation"));
  } else if (FloatIsInRange(weight, N("adverb-of-negation"))) {
    return(N("adverb-of-absolute-negation"));
  } else {
    return(NULL);
  }
}

PNode *GenADJPSimple(Obj *obj, Float weight, ObjToLexEntry *ole,
                     PNode *modifies, Discourse *dc)
{
  int		gender, number, person;
  Obj		*adverb;
  LexEntry	*adv_le;
  ObjToLexEntry	*adv_ole;
  Word		*infl, *adv_infl;
  if (!ole->le) {
    Dbg(DBGGENER, DBGBAD, "GenADJPSimple: empty le");
    return(NULL);
  }
  if (DC(dc).lang == F_ENGLISH) {
    infl = LexEntryGetInflection(ole->le, F_NULL, F_NULL, F_NULL, F_NULL,
                                 F_NULL, F_POSITIVE, 1, dc);
  } else {
    PNodeGetHeadNounFeatures(modifies, 1, &gender, &number, &person);
    if (gender == F_NULL) gender = F_MASCULINE;
    if (number == F_NULL) number = F_SINGULAR;
    infl = LexEntryGetInflection(ole->le, F_NULL, gender, number, person,
                                 F_NULL, F_POSITIVE, 1, dc);
  }
  if (!infl) return(NULL);
  adverb = WeightToAdjAdverb(weight);
  adv_le = NULL;
  adv_infl = NULL;
  if (adverb) {
    if ((adv_ole = ObjToLexEntryGet1A(adverb, NULL, FS_ADVERB, FS_NO_BE_E,
                                      F_NULL, F_NULL, NULL, dc))) {
      if ((adv_le = adv_ole->le)) {
        adv_infl = LexEntryGetInflection(adv_le, F_NULL, F_NULL, F_NULL, F_NULL,
                                         F_NULL, F_POSITIVE, 1, dc);
      }
    }
  }
  return(GenMakeADJP(ole->le, adv_le, infl, adv_infl, obj, NULL));
}

PNode *GenADJP(LexEntry *adj_le, LexEntry *adv_le, int superlative,
               int agree_gender, int agree_number, Discourse *dc)
{
  Word	*adj_infl, *adv_infl;
  if (superlative) {
    if (superlative == 2 &&
        (adj_infl = LexEntryGetInflection(adj_le, F_NULL, agree_gender,
                                          agree_number, F_NULL, F_NULL,
                                          F_SUPERLATIVE, 0, dc))) {
      /* "silliest" */
      /* A superlative adjective is always prenoun in English and French. */
      return(GenMakeADJP(adj_le, NULL, adj_infl, NULL, NULL, NULL));
    } else {
      if (!(adj_infl = LexEntryGetInflection(adj_le, F_NULL, agree_gender,
                                             agree_number, F_NULL, F_NULL,
                                             F_POSITIVE, 1, dc))) {
        return(NULL);
      }
      /* "least/most funny" */
      if (!(adv_infl = LexEntryGetInflection(adv_le, F_NULL, F_NULL, F_NULL,
                                             F_NULL, F_NULL, F_SUPERLATIVE,
                                             1, dc))) {
        return(NULL);
      }
      return(GenMakeADJP(adj_le, adv_le, adj_infl, adv_infl, NULL, NULL));
    }
  } else {
    /* "very funny" */
    if (!(adj_infl = LexEntryGetInflection(adj_le, F_NULL, agree_gender,
                                           agree_number, F_NULL, F_NULL,
                                           F_POSITIVE, 1, dc))) {
      return(NULL);
    }
    if (adv_le) {
      if (!(adv_infl = LexEntryGetInflection(adv_le, F_NULL, F_NULL, F_NULL,
                                             F_NULL, F_NULL, F_POSITIVE, 
                                             1, dc))) {
        return(NULL);
      }
    } else adv_infl = NULL;
    return(GenMakeADJP(adj_le, adv_le, adj_infl, adv_infl, NULL, NULL));
  }
}

/* End of file. */
