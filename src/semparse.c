/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Semantic parser.
 *
 * 19940101: begun
 * 19940108: added SENT and MSENT roles
 * 19940114: added filling in of preposition translations
 * 19940702: redid code for new parsing data structure
 * 19940704: removed surface translation and modified for multiple parses
 * 19941212: mods for channels
 * 19950317: redid parsing of determiners and genitives to be more elegant:
 *           they are now all X-MAX arguments
 * 19950420: added parsing of (nonnominal) relative clauses
 * 19950425: simple parsing of temporal and spatial adjuncts begun
 * 19950427: added expl case; revisions for new phrasal verb parsing mechanism
 * 19950428: more work on new phrasal verb parsing
 * 19950501: added parsing of nominal relative clauses
 * 19950511: adding new question forms and eliminating need for parse
 *           transformations
 * 19950731: broke modify into such-that, question-element, and, interrogative
 * 19951007: started adding scores
 * 19951008: continued adding scores
 * 19951027: added PNodes inside Obj results
 * 19980630: added compound noun parsing
 * 19980701: more compound noun parsing
 * 19980702: more work
 *
 * The semantic parser should always return intensions. Intensions are
 * possibly later evaluated into extensions by the anaphoric parser
 * or the understanding agency.
 *
 * todo:
 * - Uses of Sem_ParseToPN and Sem_ParseOfPN prevent Sem_Anaphora from
 *   applying coreference constraints. Need to generate new instances
 *   of these and splice them into the PNode tree properly.
 * - Parse passives. In French, "sera réorientée" is not an être verb
 *   yet it is being parsed as an active.
 * - Handle prepositional phrases juxtaposed (no conjunction):
 *   "I speak to her, to him."
 * - Parse negative adjectives such as "so not happy".
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "lextheta.h"
#include "repbasic.h"
#include "repchan.h"
#include "repcxt.h"
#include "repdb.h"
#include "repgroup.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "repstr.h"
#include "reptime.h"
#include "semanaph.h"
#include "semaspec.h"
#include "semcase.h"
#include "semdisc.h"
#include "semgen1.h"
#include "semgen2.h"
#include "semparse.h"
#include "semstyle.h"
#include "semtense.h"
#include "synfilt.h"
#include "synparse.h"
#include "synpnode.h"
#include "syntrans.h"
#include "synxbar.h"
#include "ta.h"
#include "taemail.h"
#include "tatable.h"
#include "utildbg.h"
#include "utillrn.h"

LexEntry	*Sem_ParseEnglishOfLE, *Sem_ParseFrenchOfLE;
PNode		*Sem_ParseEnglishOfPN, *Sem_ParseFrenchOfPN;
LexEntry	*Sem_ParseEnglishToLE, *Sem_ParseFrenchToLE;
PNode		*Sem_ParseEnglishToPN, *Sem_ParseFrenchToPN;

void Sem_ParseInitEnglish()
{
  if (!(Sem_ParseEnglishOfLE = LexEntryFind("of", FS_PREPOSITION,
                                            EnglishIndex))) {
    Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseInit: English of not found");
    return;
  }
  if (!(Sem_ParseEnglishToLE = LexEntryFind("to", FS_PREPOSITION,
                                            EnglishIndex))) {
    Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseInit: English to not found");
    return;
  }
  Sem_ParseEnglishOfPN = PNodeWord(F_PREPOSITION,
                                  Sem_ParseEnglishOfLE->srcphrase,
                                  Sem_ParseEnglishOfLE->features,
                                  Sem_ParseEnglishOfLE,
                                  NULL);
  Sem_ParseEnglishToPN = PNodeWord(F_PREPOSITION,
                                  Sem_ParseEnglishToLE->srcphrase,
                                  Sem_ParseEnglishToLE->features,
                                  Sem_ParseEnglishToLE,
                                  NULL);
}

void Sem_ParseInitFrench()
{
  if (!(Sem_ParseFrenchOfLE = LexEntryFind("de", FS_PREPOSITION,
                                           FrenchIndex))) {
    Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseInit: French of not found");
  }
  if (!(Sem_ParseFrenchToLE = LexEntryFind("à", FS_PREPOSITION,
                                           FrenchIndex))) {
    Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseInit: French à not found");
  }
  Sem_ParseFrenchOfPN = PNodeWord(F_PREPOSITION,
                                 Sem_ParseFrenchOfLE->srcphrase,
                                 Sem_ParseFrenchOfLE->features,
                                 Sem_ParseFrenchOfLE,
                                 NULL);
  Sem_ParseFrenchToPN = PNodeWord(F_PREPOSITION,
                                 Sem_ParseFrenchToLE->srcphrase,
                                 Sem_ParseFrenchToLE->features,
                                 Sem_ParseFrenchToLE,
                                 NULL);
}

void Sem_ParseInit()
{
  if (StringIn(F_FRENCH, StdDiscourse->langs)) {
    Sem_ParseInitFrench();
  }
  if (StringIn(F_ENGLISH, StdDiscourse->langs)) {
    Sem_ParseInitEnglish();
  }
}

LexEntry *Sem_ParseOfLE(Discourse *dc)
{
  if (DC(dc).lang == F_FRENCH) return(Sem_ParseFrenchOfLE);
  return(Sem_ParseEnglishOfLE);
}

PNode *Sem_ParseOfPN(Discourse *dc)
{
  if (DC(dc).lang == F_FRENCH) return(Sem_ParseFrenchOfPN);
  return(Sem_ParseEnglishOfPN);
}

PNode *Sem_ParseToPN(Discourse *dc)
{
  if (DC(dc).lang == F_FRENCH) return(Sem_ParseFrenchToPN);
  return(Sem_ParseEnglishToPN);
}

ObjList *Sem_ParseResults;

/* This canonicalizes certain pseudo representations due to "the structure
 * of language". Keep this consistent with GenToNonCanonical.
 * grid-traversal is done by Sem_Anaphora.
 * todo: Freeing.
 */
Obj *Sem_ParseToCanonical1(Obj *in)
{
  int		i, len;
  Obj		*out;
  if (N("action") == I(in, 0) &&
      ISA(N("object-interrogative-pronoun"), I(in, 2))) {
    out = L(N("action-interrogative-pronoun"), I(in, 1), E);
  } else if (N("appearance-copula") == I(in, 0) &&
             ISA(N("object-interrogative-pronoun"), I(in, 2))) {
    out = L(N("attribute-interrogative-pronoun"), I(in, 1), E);
  } else {
    out = in;
  }
  for (i = 0, len = ObjLen(out); i < len; i++) {
    ObjSetIth(out, i, Sem_ParseToCanonical1(I(out, i)));
  }
#ifdef notdef
  /* Let UA_Time deal with this. */
  if (TsRangeIsAlways(ObjToTsRange(out))) {
    TsRangeSetNow(&tsr);
    ObjSetTsRange(out, &tsr);
  }
#endif
  return(out);
}

Obj *Sem_ParseToCanonical(Obj *obj, Discourse *dc)
{
  if (ISA(N("intervention"), obj)) {
    return(Sem_ParseToCanonical1(L(obj, DiscourseSpeaker(dc),
                                   DiscourseListener(dc), E)));
  }
  return(Sem_ParseToCanonical1(obj));
}

/* Call this only on all representations. Destructive. */
void Sem_ParseToCanonicals(ObjList *objs, Discourse *dc)
{
  while (objs) {
    objs->obj = Sem_ParseToCanonical(objs->obj, dc);
    objs = objs->next;
  }
}

Obj *Sem_ParseToCanonicalTop(Obj *obj, Discourse *dc)
{
  if (N("such-that") == I(obj, 0) &&
      ObjInDeep(I(obj, 1), I(obj, 2))) {
    /* Eliminate top-level adjectival such-that's.
     * todo: The ObjInDeep condition can be eliminated now?
     */
    return(Sem_ParseToCanonical1(I(obj, 2)));
  }
  return(obj);
}

/* Call this only on top-level representations. Destructive. */
void Sem_ParseToCanonicalsTop(ObjList *objs, Discourse *dc)
{
  while (objs) {
    objs->obj = Sem_ParseToCanonicalTop(objs->obj, dc);
    objs = objs->next;
  }
}

/* <concepts> returned for Translation only. */
ObjList *Sem_ParseParse(PNode *pn, Discourse *dc)
{
  int		success;
  ObjList	*concepts;
  if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
    Dbg(DBGSEMPAR, DBGDETAIL, "**** SEMANTIC PARSE BEGIN ****");
    PNodePrettyPrint(Log, DiscourseGetInputChannel(dc), pn);
    IndentInit();
  }
  if (!(dc->run_agencies & AGENCY_SEMANTICS)) {
    Dbg(DBGSEMPAR, DBGDETAIL, "SEMANTICS OFF");
    Sem_ParseResults = ObjListCreateSP(N("concept"), pn->score,
                                       NULL, pn, NULL, Sem_ParseResults);
    Dbg(DBGSEMPAR, DBGDETAIL, "**** SEMANTIC PARSE END ****");
    return(NULL);
  }
  concepts = Sem_ParseParse1(pn, NULL, dc);

  if (!concepts) {
    success = 0;
  } else {
    success = 1;
  }
  Sem_ParseToCanonicalsTop(concepts, dc);
  ObjListSetPNodeSP(concepts, pn);
  Sem_ParseResults = ObjListAppendNonequalSP(Sem_ParseResults, concepts,
                                             pn, pn->score);
  Dbg(DBGSEMPAR, DBGDETAIL, "**** SEMANTIC PARSE END ****");
  return(concepts);
}

ObjList *Sem_ParseParse1(PNode *pn, CaseFrame *cf, Discourse *dc)
{
  char		*text;
  ObjList	*cons;
  if (pn == NULL) {
    Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseParse1: null pn");
    return(NULL);
  }
  if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
    IndentUp();
    IndentPrint(Log);
    fputs("SC ", Log);
    PNodePrintShort(Log, DiscourseGetInputChannel(dc), pn);
    CaseFramePrint(Log, cf);
  }
#ifdef INTEGSYNSEM
  if (pn->concepts &&
      pn->concepts_parent == XBarParent(pn, dc->pn_root)) {
    Dbg(DBGSEMPAR, DBGDETAIL, "INTEGRATED PARSING: node already Sem_Parsed");
    cons = pn->concepts;
    goto done;
  }
#endif
  switch (pn->type) {
    case PNTYPE_LEXITEM:
      cons = Sem_ParseWord(pn, cf, dc);
      break;
    case PNTYPE_CONSTITUENT:
      cons = Sem_ParseConstituent(pn, cf, dc);
      break;
    case PNTYPE_NAME:
      cons = Sem_ParseName(pn, pn->u.name, dc);
      break;
    case PNTYPE_TSRANGE:
      cons = Sem_ParseTsRange(pn, pn->u.tsr, cf, dc);
      break;
    case PNTYPE_TELNO:
      cons = Sem_ParseTelno(pn, pn->u.telno, dc);
      break;
    case PNTYPE_MEDIA_OBJ:
      cons = Sem_ParseMediaObject(pn, pn->u.media_obj, dc);
      break;
    case PNTYPE_PRODUCT:
      cons = Sem_ParseProduct(pn, pn->u.product, dc);
      break;
    case PNTYPE_NUMBER:
      cons = Sem_ParseNumber(pn, pn->u.number, dc);
      break;
    case PNTYPE_COMMUNICON:
      cons = Sem_ParseCommunicon(pn, pn->u.communicons, dc);
      break;
    case PNTYPE_ATTRIBUTION:
      cons = Sem_ParseAttribution(pn, pn->u.attribution, dc);
      break;
    case PNTYPE_EMAILHEADER:
      cons = Sem_ParseEmailHeader(pn, pn->u.emh, dc);
      break;
    case PNTYPE_TABLE:
      text = ChannelGetSubstring(DiscourseGetInputChannel(dc), pn->lowerb,
                                 pn->upperb);
      TA_TableProc(text, pn->u.table->flds, pn->u.table->num_flds,
                   TABSIZE_UNIX, dc);
      cons = NULL;
      break;
    default:
      Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseParse1: PNTYPE");
      return(NULL);
  }
  Sem_ParseToCanonicals(cons, dc);
  if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
    IndentPrint(Log);
    fputs("SR ", Log);
    ObjListPrint1(Log, cons, 1, 0, 1, 1);
    IndentDown();
  }
  return(cons);
}

/* todo: Sem_Parse really needs to return grammatical subject concept
 * of each parse.
 */
Obj *Sem_ParseSubjectOf(PNode *pn, Obj *obj)
{
  ObjList	*props;
  Obj		*noun;
  if (ISA(N("tense"), I(obj, 0)) && I(obj, 1)) {
    return(Sem_ParseSubjectOf(pn, I(obj, 1)));
  } else if ((I(obj, 1))) {
    if ((props = ObjIntensionParse(I(obj, 1), 0, &noun))) {
      ObjListFree(props);
      return(noun);
    } else {
      return(I(obj, 1));
    }
  } else {
    return(N("human"));
  }
#ifdef notdef
  ObjList		*props;
  PNode			*subj_np, *noun;
  LexEntryToObj	*leo;
  if (!(subj_np = PNodeSubject(pn))) return(ObjNA);
  if (!(noun = PNodeLeftmostFeat(subj_np, F_NOUN))) return(ObjNA);
  if (noun->lexitem && noun->lexitem->le) {
    for (leo = noun->lexitem->le->leo; leo; leo = leo->next) {
      if (ObjInDeep(leo->obj, obj)) return(leo->obj);
    }
  }
  return(ObjNA);
#endif
}

/* Semantic Cartesian product:
 * Feed results of <pnfrom> to <pnto>. Result is <pnto>.
 */
ObjList *Sem_CartProduct(PNode *pnfrom, PNode *pnto, Obj *from_max, Obj *to_max,
                         CaseFrame *cf, Discourse *dc, Obj *cas, Obj *subjcas,
                         PNode *cas_pn)
{
  int			cf_from_removeme;
  Obj			*subjcon;
  ObjList		*fromcons, *p, *r1, *r;
  CaseFrame		*cf_from, *cf_to;
  SP			subj_sp;
  CompTenseHolder	save_cth;
  cf_from_removeme = 0;
  if ((pnfrom->feature == F_NP && pnfrom->pn1 && pnfrom->pn1->feature == F_S) ||
      (pnfrom->feature == F_PP &&
         pnfrom->pn1 && pnfrom->pn1->feature == F_PREPOSITION &&
         pnfrom->pn2 && pnfrom->pn2->feature == F_NP &&
           pnfrom->pn2->pn1 && pnfrom->pn2->pn1->feature == F_S &&
           pnfrom->pn2->pn2 == NULL)) {
  /* Pass down "subj" only.
   * Je veux [X [Z m'en aller]]
   * Je décide [Y de [X [Z m'en aller]]]
   */
    if ((subjcon = CaseFrameGet(cf, N("subj"), &subj_sp))) {
      cf_from = CaseFrameAddSP(NULL, N("subj"), subjcon, &subj_sp);
      cf_from_removeme = 1;
    } else {
      cf_from = NULL;
    }
  } else if (from_max) {
  /* For example, we must clear case frame when recursing into an X-MAX
   * because otherwise, the verb's iobjs would get processed as X-MAX
   * arguments or adjuncts.
   */
    cf_from = NULL;
  } else {
  /* S1 Conj S2-Inf without subject -> subject of S2 is subject of S1 */
    cf_from = cf;
  }
  fromcons = Sem_ParseParse1(pnfrom, cf_from, dc);
  if (cf_from_removeme) CaseFrameRemove(cf_from);
  if (N("X-MAX") == from_max) {
  /* todo: It is awkward for the caller to have to specify <from_max>.
   * It would be easier if every PNode pointed to its parent. But this
   * will not work since multiple parses share the same structure.
   * Perhaps we should make a copy and set up its parent pointers
   * before calling Sem_Parse.
   */
  } else if (N("Y-MAX") == from_max) {
    if (cas == N("iobj") && pnfrom->pn1 &&
                            pnfrom->pn1->feature == F_PREPOSITION &&
                            pnfrom->pn2 &&
                            pnfrom->pn2->type == PNTYPE_TSRANGE) {
      cas = N("tsrobj");
    }
  }
  r = NULL;
  for (p = fromcons; p; p = p->next) {
    if (cas == N("subj") &&
        (ISA(N("relative-pronoun"), p->obj) ||
         ISA(N("determining-interrogative-pronoun"), p->obj))) {
      continue;
    }
    cf_to = CaseFrameAdd(cf, cas, p->obj, p->u.sp.score,
                         cas_pn ? cas_pn : pnfrom, p->u.sp.anaphors,
                         p->u.sp.leo);
    if (subjcas) {
      cf_to = CaseFrameAddSP(cf_to, subjcas,
                             Sem_ParseSubjectOf(pnfrom, p->obj),
                             &p->u.sp);
    }
    if (N("W-MAX") == to_max) {
      save_cth = dc->cth;
      CompTenseHolderInit(&dc->cth);
    }
    if ((r1 = Sem_ParseParse1(pnto, cf_to, dc))) {
      if (N("W-MAX") == to_max) {
        r1 = ObjListTenseAddSP(r1, dc->cth.tense_r);
      }
      r = ObjListAppendDestructive(r, r1);
    }
    if (N("W-MAX") == to_max) {
      dc->cth = save_cth;
    }
    if (subjcas) {
      cf_to = CaseFrameRemove(cf_to);
    }
    CaseFrameRemove(cf_to);
  }
/* Watch out about freeing because of pn->concepts.
  ObjListFree(fromcons);
*/
  return(r);
}

ObjList *Sem_ParseParse2_OfIOBJ(PNode *pnfrom, PNode *pnto, CaseFrame *cf,
                                Discourse *dc)
{
  ObjList	*fromcons, *p, *r1, *r;
  CaseFrame	*cf1;
  fromcons = Sem_ParseParse1(pnfrom, NULL, dc);
  r = NULL;
  for (p = fromcons; p; p = p->next) {
    cf1 = CaseFrameAdd(cf, N("iobj"), p->obj, p->u.sp.score,
                       Sem_ParseOfPN(dc), p->u.sp.anaphors,
                       p->u.sp.leo);
    if ((r1 = Sem_ParseParse1(pnto, cf1, dc))) {
      r = ObjListAppendDestructive(r, r1);
    }
    CaseFrameRemove(cf1);
  }
  return(r);
}

/* Feed subject of <pnfrom> to <pnto>. Result is [and <pnfrom> <pnto>]. */
ObjList *Sem_ParseParse3(PNode *pn, PNode *pnfrom, PNode *pnto, CaseFrame *cf,
                         Discourse *dc, Obj *subjcas)
{
  int		from_complete;
  ObjList	*fromcons, *p, *q, *tocons, *r;
  CaseFrame	*cf1;
  from_complete = Syn_ParseIsCompleteSentence(pnfrom, DC(dc).lang);
  fromcons = Sem_ParseParse1(pnfrom, cf, dc);
  r = NULL;
  for (p = fromcons; p; p = p->next) {
    if (from_complete) {
      cf1 = CaseFrameAddSP(cf, subjcas, Sem_ParseSubjectOf(pnfrom, p->obj),
                           &p->u.sp);
    } else {
    /* [What,] [are you crazy?]: */
      cf1 = cf;
    }
    if ((tocons = Sem_ParseParse1(pnto, cf1, dc))) {
      for (q = tocons; q; q = q->next) {
        r = ObjListCreateSPCombine(ObjMakeAnd(p->obj, q->obj), pn, p, q, r);
      }
      /* Watch out about freeing because of pn->concepts.
      ObjListFree(tocons);
       */
    }
    if (from_complete) {
      CaseFrameRemove(cf1);
    }
  }
  /* Watch out about freeing because of pn->concepts.
  ObjListFree(fromcons);
   */
  return(r);
}

PNode *BuiltInPrepositionOfPronoun(Obj *pronoun, Discourse *dc)
{
  Obj *prep;
  /* todo: Do this dynamically. Must then free later on. */
  prep = DbGetRelationValue(&TsNA, NULL, N("preposition-of"), pronoun,
                            N("prep-of"));
  if (prep == N("prep-of")) return(Sem_ParseOfPN(dc));
  else if (prep == N("prep-from")) return(Sem_ParseOfPN(dc)); /* todo */
  else if (prep == N("prep-at")) return(Sem_ParseToPN(dc));   /* todo */
  else if (prep == N("prep-to")) return(Sem_ParseToPN(dc));
  Dbg(DBGSEMPAR, DBGBAD, "BuiltInPrepositionOfPronoun");
  return(Sem_ParseOfPN(dc));
}

/* Examples:
 * (1) pronoun:[What] relative:[drives me crazy]      "subj"
 *     pronoun:[Ce qui] relative:[me rend fou]        "subj"
 * (2) pronoun:[What] relative:[I hate]               "obj"
 *     pronoun:[Ce que] relative:[je déteste]         "obj"
 * (3) pronoun:[de quoi] relative:[remplir un verre]  "iobj"+de
 */
ObjList *Sem_ParseNominalRelClause(LexEntryToObj *leo, Obj *pronoun,
                                   PNode *relative, ObjList *r, CaseFrame *cf,
                                   Discourse *dc)
{
  Obj		*cf1_cas, *cf1_con;
  ObjList	*r0, *r1;
  PNode		*cf1_pn;
  CaseFrame	*cf1;

  if (ISA(N("nominal-rel-pronoun-subj"), pronoun)) {
    if (relative->feature != F_VP) return(r);
    cf1_cas = N("subj");
    cf1_pn = NULL;
  } else if (ISA(N("nominal-rel-pronoun-obj"), pronoun)) {
    if (relative->feature != F_S) return(r);
    cf1_cas = N("obj");
    cf1_pn = NULL;
  } else if (ISA(N("nominal-rel-pronoun-iobj"), pronoun)) {
    if (relative->feature != F_S) return(r);
    cf1_cas = N("iobj");
    cf1_pn = BuiltInPrepositionOfPronoun(pronoun, dc);
  } else {
    Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseNominalRelClause1 1");
    cf1_cas = N("obj");
    cf1_pn = NULL;
  }
  if (ISA(N("nominal-rel-pronoun-human"), pronoun)) {
    cf1_con = N("human");
  } else if (ISA(N("nominal-rel-pronoun-nonhuman"), pronoun)) {
    cf1_con = N("nonhuman");
  } else {
    cf1_con = N("concept");
  }

  /* todo: Could use FeatureGetDefault(leo->features, FT_SUBCAT,
   * F_SUBCAT_INDICATIVE) to prune <relative>. Though this would mean
   * "quoi que"+indicative wouldn't parse. Could inherit subject. But
   * subject in these constructions can be anything in the discourse?
   */

  /* Parse subordinate clause. */
  cf1 = CaseFrameAdd(cf, cf1_cas, cf1_con, SCORE_MAX, cf1_pn, NULL, NULL);
    /* todoSCORE */
  r1 = Sem_ParseParse1(relative, cf1, dc);
  CaseFrameRemove(cf1);

  /* Wrap in N("such-that").
   * "who I like": [such-that human [like Jim human]]
   * todo: Get a more specific cf1_con based on selectional restriction?
   */
  for (r0 = r1; r0; r0 = r0->next) {
    r = ObjListCreateSPFrom(ObjIntension1(cf1_con, r1->obj),
                            relative,
                            r0,
                            r);
  }

  return(r);
}

/* Examples:
 * (1) np:[La femme] relative:[W [H qui] [W tenait en reserve la bouteille]]
 *     "subj"
 * (2A) np:[la bouteille] relative:[Z [X [H que]] [Z Püchl tenait ...]]  "obj"
 * (2B) np:[la bouteille] relative:[Z [X [H dont]] [Z Püchl parlait]]
 *     "iobj"+de
 * (3) np:[la bouteille] relative:[Z [Y [R avec] [X [H laquelle]]] [Z ...]]
 *     "iobj"+avec
 */
ObjList *Sem_ParseNonNominalRelClause(PNode *pn, PNode *np, PNode *relative,
                                      ObjList *r, CaseFrame *cf, Discourse *dc)
{
  Obj		*cf1_cas, *pronoun_con, *noun;
  ObjList	*npcons, *p, *relative_cons, *q, *props;
  CaseFrame	*cf1;
  LexEntry	*pronoun_le;
  PNode		*cf1_pn;
  CompTenseHolder	save_cth;
  if (relative->feature == F_VP) {
  /* Case (1) */
    cf1_cas = N("subj");
    cf1_pn = NULL;
  } else if (relative->feature == F_S) {
    if (relative->pn1 && relative->pn1->feature == F_NP &&
        relative->pn1->pn1 && relative->pn1->pn1->feature == F_PRONOUN &&
        relative->pn1->pn2 == NULL) {
      pronoun_le = PNodeLeftmostLexEntry(relative->pn1->pn1);
      if ((pronoun_con =
            /* Presumably there is only one choice of <pronoun_con>. */
           LexEntryGetConIsAncestor(N("rel-pronoun-iobj-preposition-built-in"),
                                    pronoun_le))) {
      /* Case (2B) */
        cf1_cas = N("iobj");
        cf1_pn = BuiltInPrepositionOfPronoun(pronoun_con, dc);
        relative = relative->pn2;
      } else {
      /* Case (2A) */
        cf1_cas = N("obj");
        cf1_pn = NULL;
        relative = relative->pn2;
      }
    } else if (relative->pn1 && relative->pn1->feature == F_PP &&
               relative->pn1->pn1 &&
               relative->pn1->pn1->feature == F_PREPOSITION) {
    /* Case (3) */
      cf1_cas = N("iobj");
      cf1_pn = relative->pn1->pn1;
      relative = relative->pn2;
    } else {
      Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseNonNominalRelClause 1");
      return(NULL);
    }
  } else {
    Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseNonNominalRelClause 2");
    return(NULL);
  }
  npcons = Sem_ParseParse1(np, NULL, dc);
  for (p = npcons; p; p = p->next) {
    if ((props = ObjIntensionParse(p->obj, 1, &noun))) {
    /* props: [nationality-of human country-USA]
     * noun: human
     */
      cf1 = CaseFrameAdd(cf, cf1_cas, noun, p->u.sp.score, cf1_pn,
                         p->u.sp.anaphors, p->u.sp.leo);
    } else {
      cf1 = CaseFrameAdd(cf, cf1_cas, p->obj, p->u.sp.score, cf1_pn,
                         p->u.sp.anaphors, p->u.sp.leo);
    }
      /* todoSCORE */
    if (relative->feature == F_VP) {
    /* todo: Better way of detecting W-MAXes, so this code doesn't have to
     * be repeated.
     */
      save_cth = dc->cth;
      CompTenseHolderInit(&dc->cth);
    }
    if ((relative_cons = Sem_ParseParse1(relative, cf1, dc))) {
      if (relative->feature == F_VP) {
        relative_cons = ObjListTenseAddSP(relative_cons, dc->cth.tense_r);
      }
      for (q = relative_cons; q; q = q->next) {
      /* 
       * p->obj: [such-that [definite-article human]
       *                    [nationality-of human country-USA]]
       * q->obj: [present-indicative [daydream <p->obj> na]]
       * r: [such-that [definite-article human]
       *               [nationality-of human country-USA]
       *               [daydream human na]]
       */
        if (props) {
          r = ObjListCreateSPCombine(ObjIntension(I(p->obj, 1),
                 ObjListCreateSP(q->obj, SCORE_MAX, NULL, NULL, NULL, props)),
                                     pn, p, q, r);
        } else {
          r = ObjListCreateSPCombine(ObjIntension1(p->obj, q->obj),
                                     pn, p, q, r);
        }
      }
    }
    if (relative->feature == F_VP) {
      dc->cth = save_cth;
    }
    CaseFrameRemove(cf1);
  }
  return(r);
}

/* Relative clauses. */
ObjList *Sem_ParseRelClause(PNode *pn, PNode *np, PNode *relative,
                            CaseFrame *cf, Discourse *dc)
{
  int			non_nominal_done;
  LexEntryToObj	*leo;
  ObjList		*r;
  if (np->pn1 && np->pn1->feature == F_PRONOUN && np->pn2 == NULL) {
    non_nominal_done = 0;
    r = NULL;
    for (leo = LexEntryGetLeo(np->pn1->lexitem->le); leo; leo = leo->next) {
      if (ISA(N("nominal-relative-pronoun"), leo->obj)) {
        r = Sem_ParseNominalRelClause(leo, leo->obj, relative, r, cf, dc);
      } else {
        if (!non_nominal_done) {
        /* Do this only once, because it evaluates all nonnominal meanings
         * of <np>.
         */
          r = Sem_ParseNonNominalRelClause(pn, np, relative, r, cf, dc);
          non_nominal_done = 1;
        }
      }
    }
    return(r);
  } else {
    return(Sem_ParseNonNominalRelClause(pn, np, relative, NULL, cf, dc));
  }
}

#ifdef notdef
/* Examples:
 * attach_obj: N("country-USA")
 * result:     [attachment-rel-of country nationality-of]
 *             [attachment-rel-of polity residence-of]
 * attach_obj: N("grocer")
 *             [attachment-rel-of human-occupation occupation-of]
 */
ObjList *Sem_ParseGetAttachments(Obj *attach_obj)
{
  if (ISA(N("location-interrogative-pronoun"), attach_obj)) {
    return(NULL);
  }
  return(DbGetRelationAssertions(&TsNA, NULL, N("attachment-rel-of"),
                                 attach_obj, 1, NULL));
}

ObjList *Sem_ParseExpandAttachmentNoun(ObjList *objs)
{
  ObjList	*attachments, *p, *q, *r, *props;
  r = NULL;
  for (p = objs; p; p = p->next) {
    if ((attachments = Sem_ParseGetAttachments(p->obj))) {
      for (q = attachments; q; q = q->next) {
        /* q->obj: [attachment-rel-of human-occupation occupation-of]
         * obj:    N("grocer")
         * result: [such-that human [occupation-of human grocer]]
         */
        props = ObjListCreateSPFrom(L(I(q->obj, 2), N("human"), p->obj, E),
                                    p->u.sp.pn, p, NULL);
        r = ObjListCreateSPFrom(ObjIntension(N("human"), props), p->u.sp.pn, p,
                                r);
      }
      ObjListFree(attachments);
    } else {
      r = ObjListCreateSPFrom(p->obj, p->u.sp.pn, p, r);
    }
  }
  return(r);
}
#endif

Obj *ObjRestrictionUnroll(Obj *obj, /* RESULTS */ ObjList **props)
{
  int	i, len;
  Obj	*class, *class1;
  if (!ObjIsList(obj)) return(obj);
  if (N("or") == I(obj, 0)) {
    return(ObjRestrictionUnroll(I(obj, 1), props));
  } else if (N("and") == I(obj, 0)) {
  /* obj: [and [or robot human] [and telephone-collector total-gnurd]]
   * r: human
   * props: [isa human telephone-collector]
   *        [isa human total-gnurd]
   */
    class = ObjRestrictionUnroll(I(obj, 1), props);
    i = 2;
    len = ObjLen(obj);
    while (class == N("object") && i < len) {
      class = ObjRestrictionUnroll(I(obj, i), props);
      i++;
    }
    for (; i < len; i++) {
      class1 = ObjRestrictionUnroll(I(obj, i), props);
      *props = ObjListCreate(L(N("isa"), class, class1, E), *props);
    }
    return(class);
  } else if (N("not") == I(obj, 0)) {
    return(N("object"));
  }
  return(N("object"));
}

/*
 * Returns intensions of the form [such-that class <assertion involving class>]
 *
 *     text              obj        of_obj
 * --------------------- ---------- --------
 * (1) ATTACHMENT WITH GENITIVE RELATION
 *      "grocer of Wiesengasse"
 *                       grocer     Wiesengasse
 *             ===> [such-that human [occupation-of human grocer]
 *                                   [of human Wiesengasse]]
 * (2) THETA ROLE ACTOR
 *     "Jim's appointments"
 *                  [appointment] Jim
 *             ===> [appointment Jim]
 *    (I think appositive code is responsible for finding a known one of these.)
 * (3) ROLE (such-that class is based on restrictions)
 *      "C's mother"      mother-of C
 *             ===> [such-that human [mother-of C human]]
 *      "US's president"  President-of US
 *             ===> [such-that human [President-of US human]]
 * (4) OTHER GENITIVE
 *      "Jim's stereo"       stereo    Jim
 *             ===> [such-that stereo [of stereo Jim]]
 *      "Jim's calendar"     calendar Jim
 *             ===> [such-that calendar [of calendar Jim]]
 *      "Jim's book"     book      Jim
 *             ===> [such-that book [of book Jim]]
 *
 * todo: This probably needs to ObjIntensionParse <obj>.
 */
ObjList *Sem_ParseGenitive1(Obj *obj, Obj *of_obj, Float score,
                            LexEntryToObj *leo, PNode *pn, Anaphor *anaphors,
                            int iobji, Discourse *dc, ObjList *r)
{
  Obj		*class, *newobj;
  ObjList	*more_restrictions;

#ifdef notdef
  if ((attachments = Sem_ParseGetAttachments(obj))) {
  /* (1) ATTACHMENT WITH GENITIVE RELATION */
    for (p = attachments; p; p = p->next) {
      /* todo: find known in database?, else:
       * p->obj: [attachment-rel-of occupation occupation-of]
       * obj:    N("grocer")
       * of_obj: N("Wiesengasse")
       * result: [such-that human [occupation-of human grocer]
       *                    [of human Wiesengasse]]
       */
      props = NULL;
      props = ObjListCreateSP(L(I(p->obj, 2), N("human"), obj, E), SCORE_NA,
                              NULL, NULL, NULL, props);
      props = ObjListCreateSP(L(N("of"), N("human"), of_obj, E), SCORE_NA,
                              NULL, NULL, NULL, props);
      r = ObjListCreateSP(ObjIntension(N("human"), props), score, leo,
                          pn, anaphors, r);
    }
    ObjListFree(attachments);
    return(r);
  }
#endif

  if (ISA(N("action"), I(obj, 0))) {
  /* (2) THETA ROLE ACTOR
   * "my appointment" => [appointment Jim]
   * todo: This should be handled by general theta marking instead.
   */
    newobj = ObjCopyList(obj);
    ObjSetIth(newobj, 1, of_obj);
    r = ObjListCreateSP(newobj, score, leo, pn, anaphors, r);
  } else if (ISA(N("relation"), obj)) {
  /* (3) ROLE */
    if (ISAP(DbGetRestriction(obj, iobji), of_obj)) {
    /* Example:
     * obj: N("President-of")
     * of_obj: N("country-USA")
     */
      if (N("concept") == (class = DbGetRestriction(obj, FLIP12(iobji)))) {
        class = N("object");
      }
      more_restrictions = NULL;
      if (ObjIsList(class)) {
        class = ObjRestrictionUnroll(class, &more_restrictions);
      }
      if (iobji == 1) {
        r = ObjListCreateSP(ObjIntension(class,
              ObjListCreateSP(L(obj, of_obj, class, E), SCORE_MAX, NULL,
                              NULL, NULL, more_restrictions)),
                            score, leo, pn, anaphors, r);
      } else if (iobji == 2) {
        r = ObjListCreateSP(ObjIntension(class,
              ObjListCreate(L(obj, class, of_obj, E), more_restrictions)),
                            score, leo, pn, anaphors, r);
      }
    }
  } else {
  /* (4) OTHER GENITIVE (later handled by Sem_AnaphoraIntensionParse, UAs) */
    r = ObjListCreateSP(ObjIntension1(obj, L(N("of"), obj, of_obj, E)),
                        score, leo, pn, anaphors, r);
  }
  return(r);
}

/* Returns list of:
 *     [such-that <obj> <prop1> <prop2>]
 *     [such-that <¤obj> <prop1> <prop2>]
 *     <¤obj>.
 */
ObjList *Sem_ParseGenitive(Obj *obj, Obj *of_obj, Float score,
                           LexEntryToObj *leo, PNode *pn, Anaphor *anaphors,
                           Discourse *dc, ObjList *r,
                           /* RESULTS */ int *accepted)
{
  /* todo: The word used to retrieve obj might have a relation index other
   * than 1! But note this does not apply to other genitives.
   */
  *accepted = 1;
  return(Sem_ParseGenitive1(obj, of_obj, score, leo, pn, anaphors, 1, dc, r));
}

/* todo: Need to call this. See genitive processing. */
void PNodeRuleOut(Obj *obj, PNode *pn)
{
  if (pn == NULL) return;
  pn->allcons = ObjListRemove(pn->allcons, obj);
  PNodeRuleOut(obj, pn->pn1);
  PNodeRuleOut(obj, pn->pn2);
}

ObjList *Sem_ParseWord(PNode *pn, CaseFrame *cf, Discourse *dc)
{
  switch (pn->feature) {
    case F_ADJECTIVE:
      return(Sem_ParseAdj(pn, pn->lexitem->le, cf, dc));
    case F_ADVERB:
      return(Sem_ParseAdv(pn, pn->lexitem->le, pn->lexitem->word, cf, dc));
    case F_CONJUNCTION:
      return(Sem_ParseConj(pn, pn->lexitem->le, cf, dc));
    case F_DETERMINER:
      return(Sem_ParseDeterminer(pn, pn->lexitem, cf, dc));
    case F_PREPOSITION:
      return(Sem_ParsePrep(pn, pn->lexitem->le, cf, dc));
    case F_NOUN:
      if (dc->mode & DC_MODE_COMPOUND_NOUN) {
        return(Sem_ParseOther(pn, pn->lexitem->le));
      } else {
        return(Sem_ParseNoun(pn->lexitem->le, pn, cf, dc));
      }
    case F_PRONOUN:
      return(Sem_ParsePronoun(pn, pn->lexitem, dc));
    case F_VERB:
      return(Sem_ParseVerb(pn, cf, dc));
    default:
      if (pn->lexitem && pn->lexitem->le) {
        return(Sem_ParseOther(pn, pn->lexitem->le));
      }
  }
  Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseWord 2");
  return(NULL);
}

ObjList *Sem_ParseConstituent(PNode *pn, CaseFrame *cf, Discourse *dc)
{
  if (pn->pn2 == NULL) {
    return(Sem_ParseParse1(pn->pn1, cf, dc));
  } else if (pn->feature == F_VP) {
    return(Sem_ParseVP(pn, cf, dc));
  } else if (pn->feature == F_ADJP) {
    return(Sem_ParseAdjP(pn, cf, dc));
  } else if (pn->feature == F_NP) {
    return(Sem_ParseNP(pn, cf, dc));
  } else if (pn->feature == F_S) {
    return(Sem_ParseS(pn, cf, dc));
  } else if (pn->feature == F_PP) {
    return(Sem_ParsePP(pn, cf, dc));
  } else {
    PNodePrint(Log, DiscourseGetInputChannel(dc), pn);
    Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseConstituent: unknown case");
    if (pn->pn1) Sem_ParseParse1(pn->pn1, cf, dc);
    return(Sem_ParseParse1(pn->pn2, cf, dc));
  }
}

/* THETA MARKING */

/* is_pred: 0
 * le_prep:  "de"
 * args1:    "grocer" "salesperson"
 * arg2:     "Wiesengasse"
 * results:  [such-that human [occupation-of human grocer]
 *                            [of human Wiesengasse]]
 *           [such-that salesperson [of grocer Wiesengasse]]
 */
ObjList *Sem_Parse_PrepositionAdjunct1(Float score, Obj *prep, ObjList *args1,
                                       Obj *arg2, int is_pred,
                                       Discourse *dc)
{
  Obj		*restrict_r1, *restrict_r2, *mod, *noun;
  ObjList	*p, *r, *props;
  restrict_r1 = DbGetRestriction(prep, 1);
  restrict_r2 = DbGetRestriction(prep, 2);
  if (!ISA(N("prep-adjunct-OK"), prep)) {
    if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
      fprintf(Log, "<%s> rejected because not a <prep-adjunct-OK>\n", M(prep));
    }
    return(NULL);
  }
  if (!ISAP(restrict_r2, arg2)) {
    if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
      fprintf(Log, "<%s> rejected because <%s> not a <%s>\n",
              M(prep), M(arg2), M(restrict_r2));
    }
    return(NULL);
  }
  r = NULL;
#ifdef notdef
  if (!is_pred) {
    args1 = Sem_ParseExpandAttachmentNoun(args1);
    /* todo: Freeing. */
  }
#endif
  for (p = args1; p; p = p->next) {
    if (!ISAP(restrict_r1, p->obj)) {
      if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
        fprintf(Log, "<%s> rejected because <%s> not a <%s>\n",
                M(prep), M(p->obj), M(restrict_r1));
      }
      continue;
    }
    mod = L(prep, p->obj, arg2, E);
    if (is_pred) {
      r = ObjListCreateSPFrom(mod, p->u.sp.pn, p, r);
    } else {
      props = ObjIntensionParse(p->obj, 0, &noun);
      props = ObjListCreateSP(mod, SCORE_NA, NULL, NULL, NULL, props);
      r = ObjListCreateSPFrom(ObjIntension(noun, props), p->u.sp.pn, p, r);
    }
  }
  ObjListSPScoreCombineWith(r, score);
  return(r);
}

/* Example:
 * is_pred: 1
 * le_prep:  "for"
 * args1:    [leave Katie] [ptrans Katie]
 * arg2:     [glance Katie street]
 * results:  [prep-for [leave Katie] [glance Katie street]]
 *           [prep-for [leave Katie] [ptrans Katie street]]
 *
 * is_pred: 0
 * le_prep:  "de"
 * args1:    list of one: "grocer"
 * arg2:     "Wiesengasse"
 * results:
 *
 * is_pred: 0
 * le_prep:  "de"
 * args1:    "grocer" "salesperson"
 * arg2:     "Wiesengasse"
 * results:  [such-that grocer [of grocer Wiesengasse]]
 *           [such-that salesperson [of grocer Wiesengasse]]
 */
ObjList *Sem_Parse_PrepositionAdjunct(Float score, LexEntryToObj *leo,
                                      LexEntry *le_prep, ObjList *args1,
                                      Obj *arg2, int is_pred,
                                      Discourse *dc)
{
  int			accepted;
  ObjList		*r, *r1, *p;
  LexEntryToObj	*leo1;
  r = NULL;
  if ((!is_pred) && LexEntryConceptIsAncestor(N("prep-of"), le_prep)) {
  /* todoSCORE: Genitive is handled specially for nonpredicate X-MAXes
   * and this locks out other possibilities.
   */
    for (p = args1; p; p = p->next) {
      if ((r1 = Sem_ParseGenitive(p->obj, arg2, score, leo,
                                  p->u.sp.pn, p->u.sp.anaphors,
                                  dc, NULL, &accepted))) {
        r = ObjListAppendDestructive(r, r1);
      }
    }
  } else {
    for (leo1 = LexEntryGetLeo(le_prep); leo1; leo1 = leo1->next) {
      if ((r1 = Sem_Parse_PrepositionAdjunct1(score, leo1->obj, args1, arg2,
                                              is_pred, dc))) {
        r = ObjListAppendDestructive(r, r1);
      }
    }
  }
  return(r);
}

ObjList *Sem_ParseThetaMarkingAdjuncts(Float score, LexEntryToObj *leo,
                                       ObjList *r, int is_pred,
                                       CaseFrame *cf, Discourse *dc)
{
  ObjList	*r1;
  CaseFrame	*cf1;
  TsRange	*tsr;
  LexEntry	*le_prep;
  for (cf1 = cf; cf1; cf1 = cf1->next) {
    if (cf1->theta_marked) continue;
    /************************************************************************/
    if (is_pred && cf1->cas == N("expl")) {
      if (cf1->sp.pn && cf1->sp.pn->feature == F_ADVERB) {
      /* An untheta-marked "expl" adverb argument upgrades from expletive
       * status to normal adverb status.
       */
        r = Sem_ParseAdvMod(score, r, cf1->sp.pn, cf1, dc);
        cf1->theta_marked = 1;
      }
    /************************************************************************/
    } else if (cf1->cas == N("iobj")) {
      if ((le_prep = PNodeLeftmostPreposition(cf1->sp.pn))) {
        if ((r1 = Sem_Parse_PrepositionAdjunct(score, leo, le_prep, r,
                                               cf1->concept, is_pred,
                                               dc))) {
          r = r1;
          cf1->theta_marked = 1;
        }
      }
    /************************************************************************/
    } else if (is_pred && cf1->cas == N("tsrobj")) {
      tsr = ObjToTsRange(cf1->concept);
      /* Example:
       * tsr: 1000tod
       * todo: Change tsr based on PNodeLeftmostPreposition(cf1->sp.pn)
       * (until, starting, at, etc.)
       */
      ObjListSetTsRange(r, tsr);
      cf1->theta_marked = 1;
    /************************************************************************/
    }
  }
  return(r);
}

Bool Sem_ParseSatisfiesSSubcategRestrict(PNode *z, int subcat, Discourse *dc)
{
  int		tense, mood, r;
  PNode		*auxverb, *mainverb;
  LexEntry	*prep_le;

  if (z && z->feature != F_S) {
    if (z->feature == F_NP &&
        z->pn2 == NULL &&
        z->pn1 && z->pn1->feature == F_S) {
    /* "I want to buy a desk phone." */
      z = z->pn1;
    } else if (z->feature == F_PP &&
               z->pn1 && z->pn1->feature == F_PREPOSITION &&
               z->pn2 && z->pn2->feature == F_NP &&
               z->pn2->pn1 && z->pn2->pn1->feature == F_S) {
    /* "succeeded at being elected President" */
      z = z->pn2->pn1;
    } else {
      z = NULL;
    }
  }

  /* At this point,
   * subcat: F_NULL (not a sentence), otherwise from FT_SUBCAT (a sentence)
   * z:      NULL (not a sentence), otherwise a sentence
   */

  if (subcat == F_NULL && z == NULL) {
  /* A nonsentence argument satisfies the subcategorization restriction.
   * "I dropped the spoon."
   */
    return(1);
  }

  if (subcat == F_NULL) {
  /* A sentence argument violates the NULL subcategorization restriction.
   * "I dropped Billy went to the store."
   */
    Dbg(DBGSEMPAR, DBGDETAIL, "subcat rejects subordinate clause");
    return(0);
  }

  if (z == NULL) {
  /* A nonsentence argument violates the non-NULL subcategorization
   * restriction.
   * "I told her spoon."
   */
    Dbg(DBGSEMPAR, DBGDETAIL, "subcat rejects nonsubordinate clause");
    return(0);
  }

  /* Otherwise, we have a sentence argument and a non-NULL subcategorization
   * restriction.
   */

  PNodeFindHeadVerbS(z, &auxverb, &mainverb);
  if (auxverb && auxverb->lexitem) {
    tense = FeatureGet(auxverb->lexitem->features, FT_TENSE);
    mood = FeatureGet(auxverb->lexitem->features, FT_MOOD);
    switch (subcat) {
      case F_SUBCAT_SUBJUNCTIVE:
        r = FeatureMatch(mood, F_SUBJUNCTIVE) &&
            StringIn(tense, FS_FINITE_TENSE);
        if (!r) {
          Dbg(DBGSEMPAR, DBGDETAIL,
              "subcat rejects nonsubjunctive mood %c tense %c",
              (char)mood, (char)tense);
        }
        return(r);
      case F_SUBCAT_INDICATIVE:
        r = FeatureMatch(mood, F_INDICATIVE) &&
            StringIn(tense, FS_FINITE_TENSE);
        if (!r) {
          Dbg(DBGSEMPAR, DBGDETAIL,
              "subcat rejects nonindicative mood %c tense %c",
              (char)mood, (char)tense);
        }
        return(r);
      case F_SUBCAT_INFINITIVE:
        if (DC(dc).lang == F_FRENCH) {
          prep_le = NULL;
          r = (tense == F_INFINITIVE);
        } else {
          prep_le = PNodeLeftmostPreposition(z);
          r = (tense == F_INFINITIVE) &&
              (Sem_ParseEnglishToLE == prep_le);
        }
        if (!r) {
          Dbg(DBGSEMPAR, DBGDETAIL,
              "subcat rejects noninfinitive tense %c prep %s ",
              (char)tense, prep_le ? prep_le->srcphrase : "");
        }
        return(r);
      case F_SUBCAT_PRESENT_PARTICIPLE:
        r = (tense == F_PRESENT_PARTICIPLE);
        if (!r) {
          Dbg(DBGSEMPAR, DBGDETAIL,
              "subcat rejects nonpresent participle tense %c",
              (char)tense);
        }
        return(r);
      default:
        break;
    }
  }
  return(1);
}

Bool Sem_ParseIsImperative(Obj *comptense, int lang)
{
  if (lang == F_ENGLISH) {
  /* Imperatives are not represented as distinct inflections in English, since
   * the form is always the same as the infinitive.
   */
    return(N("infinitive") == comptense);
  } else {
    return(N("imperative") == comptense);
  }
}

Obj *Sem_ParseThetaMarking_Pred(LexEntryToObj *leo, int pos, Obj *comptense,
                                Obj *aobj, SP *aobj_sp,
                                CaseFrame *cf, Discourse *dc,
                                /* RESULTS */ Float *args_score,
                                Anaphor **anaphors)
{
  int		slotnum, subcat;
  Float		score_result, score_elem;
  Bool		isoptional;
  Obj		*elems[MAXLISTLEN], *cas, *con_elem, *subj;
  PNode		*pn_elems[MAXLISTLEN], *pn_elem;
  ThetaRole	*theta_roles;
  LexEntry	*tr_le;
  SP		elem_sp;

  *anaphors = NULL;
  elems[0] = leo->obj;
  pn_elems[0] = NULL;	/* todo */
  subj = NULL;
  score_result = SCORE_MAX;
  *args_score = SCORE_MAX;
  for (slotnum = 1, theta_roles = leo->theta_roles;
       theta_roles;
       slotnum++, theta_roles = theta_roles->next) {
    ThetaRoleGet(theta_roles, &tr_le, &cas, &subcat, &isoptional);
    if (cas == N("expl")) {
      if (!CaseFrameThetaMarkExpletive(cf, tr_le)) {
      /* All expl in ThetaRole must be found. A possible exception to implement:
       * "not" may be omitted in which case [not <r>] would be the result.
       * Example: [bad]=/ne pas être terrible.Vy/, "C'est terrible"=>[not [bad]]
       */
        if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
          fprintf(Log, "<%s> rejected because expl <%s> not in case frame\n",
                  M(leo->obj), tr_le ? tr_le->srcphrase : "");
        }
        return(NULL);
      }
      slotnum--;
      continue;
    }
    if (cas == N("aobj") &&
        ((aobj == NULL) ||
         (aobj_sp == NULL))) {
      Dbg(DBGSEMPAR, DBGBAD, "NULL aobj?!");
    }
    if (cas == N("aobj") && aobj && aobj_sp) {
      con_elem = aobj;
      score_elem = aobj_sp->score;
      pn_elem = aobj_sp->pn;
      *anaphors = AnaphorAppendDestructive(*anaphors,
                                           AnaphorCopyAll(aobj_sp->anaphors));
    } else {
      score_elem = SCORE_MAX;
      pn_elem = NULL;
      elem_sp.anaphors = NULL;
      con_elem = CaseFrameGetLe(cf, cas, tr_le, subcat, &elem_sp);
      pn_elem = elem_sp.pn;
      score_elem = elem_sp.score;
      if (dc->defer_ok && con_elem == NULL && cas == N("subj")) {
        return(OBJDEFER);
      }
      if (con_elem == NULL && 
          ISA(N("spatial"), leo->obj) &&
          tr_le && LexEntryConceptIsAncestor(N("spatial-preposition"), tr_le)) {
      /* "She went where?" satisfies ==ptrans/spatial/go* to+.Véz/ etc. */
        con_elem = CaseFrameGetClass(cf, N("obj"),
                                     N("location-interrogative-pronoun"),
                                     &elem_sp);
        pn_elem = elem_sp.pn;
        score_elem = elem_sp.score;
      }
      if (con_elem == NULL && cas == N("subj")) {
      /* For "Un temps à s'offrir un petit schnaps".
       * todoSCORE: If this is too relaxed, we could instead set an na
       * subject in NP PP -> NP.
       */
        if (Sem_ParseIsImperative(comptense, DC(dc).lang)) {
          Dbg(DBGSEMPAR, DBGHYPER, "assumed listener subj");
          con_elem = DiscourseListener(dc);
          pn_elem = NULL;
        } else {
          Dbg(DBGSEMPAR, DBGHYPER, "assumed na subj");
          con_elem = N("na");
          pn_elem = NULL;
        }
      }
      if (con_elem && cas != N("subj") && cas != N("aobj") &&
          !Sem_ParseSatisfiesSSubcategRestrict(pn_elem, subcat, dc)) {
        if (isoptional) {
          con_elem = NULL;
        } else {
          if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
            fprintf(Log,
"<%s> rejected because <%s> violates subcategorization restriction\n",
                    M(leo->obj), M(cas));
            return(NULL);
          }
        }
      }
      *anaphors = AnaphorAppendDestructive(*anaphors,
                                           AnaphorCopyAll(elem_sp.anaphors));
    }
    if (cas == N("subj") || cas == N("aobj")) {
      subj = con_elem;
    }
    if (!con_elem) {
      if (isoptional) {
        elems[slotnum] = ObjNA;
        pn_elems[slotnum] = NULL;
      } else {
        if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
          if (tr_le) {
            fprintf(Log, "<%s> rejected because <%s> <%s> not in case frame\n",
                    M(leo->obj), M(cas), tr_le->srcphrase);
          } else {
            fprintf(Log, "<%s> rejected because <%s> not in case frame\n",
                    M(leo->obj), M(cas));
          }
        }
        return(NULL);
      }
    } else {
      elems[slotnum] = con_elem;
      pn_elems[slotnum] = pn_elem;
      score_result = ScoreCombine(score_result, score_elem);
    }
  }

  if (!DbRestrictionParse(elems, pn_elems, &slotnum, dc)) {
    return(NULL);
  }

  *args_score = score_result;

  return(ObjCreateListPNode1(elems, pn_elems, slotnum));
}

/* Theta-marking is done here for the classes:
 * E-MAX, W-MAX, X-MAX, and, indirectly, Y-MAX.
 */
ObjList *Sem_ParseThetaMarking1(Float head_score, LexEntryToObj *head_leo,
                                PNode *head_pn, int pos, Obj *comptense,
                                Obj *aobj, SP *aobj_sp, int epith_pred,
                                ObjList *props, ObjList *rest,
                                CaseFrame *cf, Discourse *dc,
                                /* RESULTS */ int *accepted)
{
  int		is_pred;
  Float		args_score;
  Obj		*con, *con1, *aobj1;
  ObjList	*r;
  Anaphor	*anaphors;
  *accepted = 0;
  CaseFrameThetaMarkClear(cf);
  if (ISA(N("F68"), I(aobj, 0))) {
  /* aobj: [definite-article woman]
   * aobj1: woman
   */
    aobj1 = I(aobj, 1);
    aobj_sp->pn = PNI(aobj, 1);
  } else {
    aobj1 = aobj;
  }
  if ((is_pred = (pos != F_NOUN || head_leo->theta_roles ||
                  ISA(N("action"), head_leo->obj)))) {
    con = Sem_ParseThetaMarking_Pred(head_leo, pos, comptense,
                                     aobj1, aobj_sp, cf, dc,
                                     &args_score, &anaphors);
    if (con == OBJDEFER) {
      return(OBJLISTDEFER);
    } else if (con == NULL) {
      return(rest);
    }
  } else {
    con = head_leo->obj;
    args_score = SCORE_MAX;
    anaphors = NULL;
  }

  r = NULL;
  if (pos == F_ADJECTIVE) {
    /* aobj: N("human")
     * con:   [stupid human]
     * props: [NAME-of human NAME:"Jim Garnier"]
     */
    if (epith_pred == F_PREDICATIVE_ADJ) {
    /* "Jim is stupid": [stupid [such-that human [NAME-of human ...]]] */
      con1 = con;
      ObjSetIth(con1, 1, ObjIntension(aobj, props));
      /* todo: Set PNodes. */
    } else {
    /* "stupid Jim": [such-that human [stupid human] [NAME-of human ...]] */
      con1 = ObjIntension(aobj, ObjListCreate(con, props));
      ObjSetIthPNode(con1, 1, aobj_sp->pn);
    }
    r = ObjListCreateSP(con1,
                        ScoreCombine(ScoreCombine(head_score, aobj_sp->score),
                                     args_score),
                        head_leo,
                        head_pn,
                        anaphors,
                        r);
  } else {
    r = ObjListCreateSP(con,
                        ScoreCombine(head_score, args_score),
                        head_leo,
                        head_pn,
                        anaphors,
                        r);
  }

  r = Sem_ParseThetaMarkingAdjuncts(head_score, head_leo, r, is_pred,
                                    cf, dc);

  if (CaseFrameDisobeysThetaCriterion(cf, head_leo->obj)) {
    /* todo: Freeing. */
    return(rest);
  }

  if (r) *accepted = 1;
  return(ObjListAppendDestructive(r, rest));
}

void Sem_ParseCopulaSetTsr(Obj *copula, TsRange *tsr, Discourse *dc)
{
  if (ISA(N("begin-copula"), copula)) {
    tsr->startts = *DCNOW(dc);
  } else if (ISA(N("end-copula"), copula)) {
    tsr->stopts = *DCNOW(dc);
  }
}

/* Copula lexical item is assumed to have either one "obj" OR one "iobj"
 * argument. Examples:
 *   be.Véz/                    "Muriel is a student."
 *   be* re#B-elected#V as+.Vz/ "Mitterand was re-elected as President."
 * In addition to that, there may be another "iobj", usually prep-of:
 *   "Mitterand is President of France."
 *   "Mitterand was re-elected as President of France."
 * todo: There could be two prep-ofs, so we need to try both possibilities.
 * The same comment applies to Sem_ParseThetaMarking1.
 */
void Sem_ParseGetCopulaArgs(LexEntryToObj *leo, CaseFrame *cf,
                            /* RESULTS */ Obj **obj, SP *obj_sp,
                            Obj **iobj, SP *iobj_sp)
{
  Obj		*o;
  ThetaRole	*theta_roles;
  *obj = *iobj = NULL;
  SPInit(obj_sp);
  SPInit(iobj_sp);
  for (theta_roles = leo->theta_roles;
       theta_roles;
       theta_roles = theta_roles->next) {
    if (theta_roles->cas == N("obj")) {
      o = CaseFrameGet(cf, N("obj"), obj_sp);
      if ((N("such-that") == I(o, 0)) && ISA(N("relation"), I(o, 1)) &&
          N("one") == I(I(o, 2), 0) && I(o, 1) == I(I(o, 2), 1)) {
      /* [such-that vice-president-of [one vice-president-of]] =>
       * vice-president-of
       */
        o = I(o, 1);
      }
      *obj = o;
      *iobj = CaseFrameGet(cf, N("iobj"), iobj_sp);
              /* Relaxed. We accept any preposition. */
      return;
    } else if (theta_roles->cas == N("iobj")) {
      *obj = CaseFrameGetLe(cf, N("iobj"), theta_roles->le, F_NULL, obj_sp);
      *iobj = CaseFrameGetNonThetaMarked(cf, N("iobj"), iobj_sp);
              /* Relaxed. We accept any preposition. */
      return;
    }
  }
}

ObjList *Sem_ParseS_EquativeRole1(Float score, Obj *subj, SP *subj_sp,
                                  Obj *parsed_article, Obj *rel, SP *rel_sp,
                                  Obj *iobj, SP *iobj_sp,
                                  LexEntryToObj *copula_leo, 
                                  Discourse *dc)
{
  int	reli, subji, iobji, nai, len;
  Obj	*elems[MAXLISTLEN], *rel_article, *con;
  PNode	*pn_elems[MAXLISTLEN];
  if (Sem_ParseGetRoleIndices(rel_sp->leo ? rel_sp->leo->features : "",
                              rel, &reli, &subji, &iobji,
                              &nai, &len, &rel_article)) {
  /* Like GenS_EquativeRole: (could complain about incorrect rel_article)
   * "Bill Clinton is the President of the United States."
   * "Jim is an employee of Daydreaming Machines, Inc."
   */
    if (parsed_article && parsed_article != rel_article) {
    /* "Mary is a mother of John." "a"?! */
      score = score * 0.8;	/* todoSCORE */
    }
    elems[reli] = rel;
    pn_elems[reli] = rel_sp->pn;
    elems[subji] = subj;
    pn_elems[subji] = subj_sp->pn;
    elems[iobji] = iobj;
    pn_elems[iobji] = iobj_sp->pn;
    if (nai >= 0) {
      elems[nai] = ObjNA;
      pn_elems[nai] = NULL;
    }
    if (DbRestrictionParse(elems, pn_elems, &len, dc)) {
      con = ObjCreateListPNode1(elems, pn_elems, len);
#ifdef notdef
      /* UA_Time should deal with this. */
      if (copula_leo) {
        Sem_ParseCopulaSetTsr(copula_leo->obj, ObjToTsRange(con), dc);
      }
#endif
      return(ObjListCreateSP(con, score, copula_leo, NULL,	/* todo */
                             AnaphorAppend3(subj_sp->anaphors,
                                            rel_sp->anaphors,
                                            iobj_sp->anaphors),
                             NULL));
    }
  }
  return(NULL);
}

/* subj: Bill Clinton
 * rel:  President-of
 * iobj: country-USA
 */
ObjList *Sem_ParseS_EquativeRole(Float score, Obj *subj, SP *subj_sp,
                                 Obj *rel, SP *rel_sp, Obj *iobj, SP *iobj_sp,
                                 LexEntryToObj *copula_leo,
                                 CaseFrame *cf, Discourse *dc)
{
  Obj	*parsed_article;
  if (!CaseFrameDisobeysThetaCriterion(cf, copula_leo->obj)) {
    if (ISA(N("determiner-article"), I(rel, 0))) {
      rel = I(rel, 1);
      parsed_article = I(rel, 0);
    } else {
      parsed_article = NULL;
    }
    return(Sem_ParseS_EquativeRole1(score, subj, subj_sp, parsed_article, rel,
                                    rel_sp, iobj, iobj_sp, copula_leo, dc));
  }
  return(NULL);
}

#ifdef notdef
ObjList *Sem_ParseS_AscriptiveAttachment(Float score, LexEntryToObj *leo,
                                         Obj *subj, SP *subj_sp, Obj *obj,
                                         SP *obj_sp, CaseFrame *cf,
                                         Discourse *dc)
{
  int		len;
  Obj		*elems[MAXLISTLEN], *con, *noun;
  PNode		*pn_elems[MAXLISTLEN];
  ObjList	*rels, *r, *p, *props;

  if ((props = ObjIntensionParse(obj, 1, &noun))) {
  /* obj: [such-that [indefinite-article human] [occupation-of human grocer]] */
    con = ObjListToAndObj(props);	/* should contain the PNodes */
    con = ObjSubst1(con, noun, subj, subj_sp->pn, NULL);
    return(ObjListCreateSP(con, score, leo, NULL,
                           AnaphorAppend(subj_sp->anaphors, obj_sp->anaphors),
                           NULL));
  }

  /* Perhaps the below isn't used anymore? */

  if (ISA(N("indefinite-article"), I(obj, 0))) obj = I(obj, 1);
  if ((rels = Sem_ParseGetAttachments(obj))) {
    if (!CaseFrameDisobeysThetaCriterion(cf, leo->obj)) {
      /* Like GenS_AscriptiveAttachment:
       * "Mrs. Püchl is a grocer"
       */
      r = NULL;
      for (p = rels; p; p = p->next) {
        elems[0] = I(p->obj, 2);
        elems[1] = subj;
        elems[2] = obj;
        pn_elems[0] = NULL;
        pn_elems[1] = subj_sp->pn;
        pn_elems[2] = obj_sp->pn;
        len = 3;
        if (DbRestrictionParse(elems, NULL, &len, dc)) {
          con = ObjCreateListPNode1(elems, pn_elems, len);
#ifdef notdef
          Sem_ParseCopulaSetTsr(leo->obj, ObjToTsRange(con), dc);
#endif
          r = ObjListCreateSP(con, score, leo, NULL,
                              AnaphorAppend(subj_sp->anaphors,
                                            obj_sp->anaphors),
                              r);
        }
      }
      ObjListFree(rels);
      return(r);
    }
  }
  return(NULL);
}
#endif

ObjList *Sem_ParseCopula(Float head_score, LexEntryToObj *leo, PNode *head_pn,
                         Obj *comptense, ObjList *r, CaseFrame *cf,
                         Discourse *dc, /* RESULTS */ int *accepted)
{
  Obj		*subj, *obj, *iobj;
  ObjList	*r1;
  SP		subj_sp, obj_sp, iobj_sp;

  CaseFrameThetaMarkClear(cf);
  *accepted = 0;
  if ((subj = CaseFrameGet(cf, N("subj"), &subj_sp))) {
    Sem_ParseGetCopulaArgs(leo, cf, &obj, &obj_sp, &iobj, &iobj_sp);
    if (obj) {
#ifdef notdef
      if (ISA(N("possessive-determiner"), I(obj, 0))) {
        obj = I(obj, 1);
        obj_sp.pn = PNI(obj, 1);
        iobj = I(obj, 0);
        iobj_sp.pn = PNI(obj, 0);
        iobj_sp.score = obj_sp.score;
      } else if ((props = ObjIntensionParse(obj, 0, &class)) &&
                 ObjListIsLengthOne(props) &&
                 ISA(N("relation"), I(props->obj, 0))) {
      /* todo: This is hopelessly nongeneral. */
        obj = I(props->obj, 0);
        obj_sp.pn = PNI(props->obj, 0);
        if (class == I(props->obj, 2)) {
          iobj = I(props->obj, 1);
          iobj_sp.pn = PNI(props->obj, 1);
        } else {
          iobj = I(props->obj, 2);
          iobj_sp.pn = PNI(props->obj, 2);
        }
        iobj_sp.score = obj_sp.score;
      }
#endif
      if (iobj) {
        if ((r1 = Sem_ParseS_EquativeRole(ScoreCombine(head_score,
                                                  ScoreCombine(obj_sp.score,
                                                               iobj_sp.score)),
                                          subj, &subj_sp, obj, &obj_sp,
                                          iobj, &iobj_sp, leo, cf, dc))) {
          *accepted = 1;
          return(ObjListAppendDestructive(r, r1));
        }
      } else if (ObjInDeep(N("adjective-argument"), obj)) {
        *accepted = 1;
        return(ObjListCreateSP(ObjSubstPNode(obj, N("adjective-argument"),
                                             subj, subj_sp.pn),
                               ScoreCombine(head_score, obj_sp.score), leo,
                               NULL,
                               AnaphorAppend(subj_sp.anaphors,
                                             obj_sp.anaphors),
                               r));
      } else if (ISA(N("interrogative-pronoun"), subj)) {
        *accepted = 1;
        r = ObjListCreateSP(L(N("standard-copula"), subj, obj, E),
                            ScoreCombine(head_score, obj_sp.score), leo,
                            NULL,
                            AnaphorAppend(subj_sp.anaphors, obj_sp.anaphors),
                            r);
        return(r);
      } else {
#ifdef notdef
        if ((r1 = Sem_ParseS_AscriptiveAttachment(ScoreCombine(head_score,
                                                              obj_sp.score),
                                                  leo, subj, &subj_sp, obj,
                                                  &obj_sp, cf, dc))) {
          *accepted = 1;
          return(ObjListAppendDestructive(r, r1));
        }
#endif
      }      
    }
  }
  /* If no luck doing special copula parsing: */
  return(Sem_ParseThetaMarking1(head_score, leo, head_pn, F_VERB, comptense,
                                NULL, NULL, F_NULL, NULL, r, cf, dc,
                                accepted));
}

ObjList *Sem_ParseThetaMarking(Float head_score, LexEntryToObj *head_leo,
                               PNode *head_pn, int pos, Obj *comptense,
                               Obj *aobj, SP *aobj_sp, int epith_pred,
                               ObjList *props, ObjList *r,
                               CaseFrame *cf, Discourse *dc,
                               /* RESULTS */ int *accepted)
{
  if (ISA(N("copula"), head_leo->obj)) {
    if (props) Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseThetaMarking: copula & props");
    return(Sem_ParseCopula(head_score, head_leo, head_pn, comptense, r, cf, dc,
                           accepted));
  } else {
    return(Sem_ParseThetaMarking1(head_score, head_leo, head_pn, pos,
                                  comptense, aobj, aobj_sp, epith_pred,
                                  props, r, cf, dc, accepted));
  }
}

/* ADJECTIVE PARSING */

/* See also GenGetCase. */
Bool Sem_ParseGetRoleIndices(char *features, Obj *rel,
                             /* RESULTS */ int *reli, int *subji, int *iobji,
                             int *nai, int *len, Obj **rel_article)
{
  Obj	*reltype;
  reltype = DbGetEnumValue(&TsNA, NULL, N("relation-type"), rel, NULL);
  *reli = 0;

  if (!ISA(N("relation"), rel)) return(0);

  if (reltype == N("many-to-one")) {
    *rel_article = N("definite-article");
  } else if (reltype == N("one-to-many")) {
    *rel_article = N("indefinite-article");
  } else if (reltype == N("one-to-one")) {
    *rel_article = N("definite-article");
  } else {
    *rel_article = N("indefinite-article");
  }
  *subji = 2; *iobji = 1; *nai = -1; *len = 3;

  return(1);
}

ObjList *Sem_ParseAdj(PNode *pn, LexEntry *le, CaseFrame *cf, Discourse *dc)
{
  int		accepted, preposed, epith_pred;
  Obj		*aobj;
  ObjList	*props, *r;
  LexEntryToObj	*leo;
  SP		aobj_sp;

  if ((aobj = CaseFrameGet(cf, N("aobj-preposed"), &aobj_sp))) {
    epith_pred = F_EPITHETE_ADJ;
    preposed = 1;
  } else if ((aobj = CaseFrameGet(cf, N("aobj-postposed"), &aobj_sp))) {
    epith_pred = F_EPITHETE_ADJ;
    preposed = 0;
  } else if ((aobj = CaseFrameGet(cf, N("aobj-predicative"), &aobj_sp))) {
    epith_pred = F_PREDICATIVE_ADJ;
    preposed = -1;
  } else {
    Dbg(DBGSEMPAR, DBGBAD, "empty aobj");
    return(NULL);
  }
  props = ObjIntensionParse(aobj, 1, &aobj);
  r = NULL;
  for (leo = LexEntryGetLeo(le); leo; leo = leo->next) {
    if (!FeatureMatch(epith_pred, FeatureGet(leo->features, FT_EPITH_PRED))) {
    /* todoSCORE ? */
      continue;
    }
    if (preposed >= 0 &&
        preposed != LexEntryIsPreposedAdj(leo->features, F_ADJECTIVE, dc)) {
    /* todoSCORE ? */
      continue;
    }
    r = Sem_ParseThetaMarking(LexEntryToObjScore(leo, dc), leo, pn,
                              F_ADJECTIVE, NULL, aobj, &aobj_sp,
                              epith_pred, props, r, cf, dc, &accepted);
    if (accepted) {
      PNodeAddAllcons(pn, leo->obj);
    }
  }
  return(r);
}

ObjList *Sem_ParseAdjP(PNode *pn, CaseFrame *cf, Discourse *dc)
{
  if (pn->pn1->feature == F_ADJP && pn->pn2->feature == F_PP) {
    return(Sem_CartProduct(pn->pn2, pn->pn1, N("Y-MAX"), NULL, cf, dc,
                           N("iobj"), NULL, NULL));
  } else if (pn->pn1->feature == F_ADJP && pn->pn2->feature == F_NP) {
    return(Sem_CartProduct(pn->pn2, pn->pn1, N("X-MAX"), NULL, cf, dc, N("obj"),
                           NULL, NULL));
  } else if (pn->pn1->feature == F_ADVERB && pn->pn2->feature == F_ADJP) {
    return(Sem_CartProduct(pn->pn2, pn->pn1, NULL, NULL, cf, dc, N("bobj"),
                           NULL, NULL));
  } else if (pn->pn1->feature == F_ADJP && pn->pn2->feature == F_ADVERB) {
    return(Sem_CartProduct(pn->pn1, pn->pn2, NULL, NULL, cf, dc, N("bobj"),
                           NULL, NULL));
  } else if (pn->pn1->feature == F_DETERMINER && pn->pn2->feature == F_ADJP) {
    return(Sem_ParseAdjP(pn->pn2, cf, dc));
  } else if (pn->pn1->feature == F_ADJP && pn->pn2->feature == F_ADJP) {
    if (LexEntryAnyConceptsFeat(F_COORDINATOR,
                                PNodeLeftmostLexEntry(pn->pn2))) {
      return(Sem_CartProduct(pn->pn1, pn->pn2, NULL, NULL, cf, dc,
                             N("mkobj2"), NULL, NULL));
    }
  } else if (pn->pn1->feature == F_CONJUNCTION && pn->pn2->feature == F_ADJP) {
    return(Sem_CartProduct(pn->pn2, pn->pn1, NULL, NULL, cf, dc, N("mkobj1"),
                           NULL, NULL));
  }
  PNodePrint(Log, DiscourseGetInputChannel(dc), pn);
  Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseAdjP: unknown case");
  return(NULL);
}

/* ADVERB PARSING */

ObjList *Sem_ParseAdvMod(Float score, ObjList *r, PNode *adv, CaseFrame *cf,
                         Discourse *dc)
{
  ObjList	*r0, *r1, *p;
  CaseFrame	*cf1;
  r0 = NULL;
  for (p = r; p; p = p->next) {
    cf1 = CaseFrameAddSP(cf, N("bobj"), p->obj, &p->u.sp);
    if ((r1 = Sem_ParseParse1(adv, cf1, dc))) {
          /* Was passing "" for word; Why? */
      r0 = ObjListAppendDestructive(r0, r1);
    }
    CaseFrameRemove(cf1);
  }
  /* Watch out about freeing because of pn->concepts.
  ObjListFree(r);
   */
  ObjListSPScoreCombineWith(r0, score);
  return(r0);
}

Float Sem_ParseAdvModAdj(Obj *adv, Float adj_value)
{
  /* todo: This needs a bit of work to mesh exactly with generation
   * and UA_QuestionDegreeAdverb and Lex_WordForm2.
   */
  return FloatValueOf(adv);
}

/* todo: Incorrect if props length > 1, because there is only one bobj_leo. */
ObjList *Sem_ParseAdvObjMod(ObjList *props, Obj *noun, Float score,
                            LexEntryToObj *leo, SP *bobj_sp,
                            ObjList *r)
{
  Float		value;
  ObjList	*p, *newprops;
  newprops = NULL;
  for (p = props; p; p = p->next) {
    if (I(p->obj, 1) != noun) {
      Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseAdvObjMod");
      return(NULL);
    }
    value = Sem_ParseAdvModAdj(leo->obj, ObjModGetPropValue(p->obj));
    newprops = ObjListCreateSP(ObjModCreateProp(I(p->obj, 0), noun,
                                                NumberToObj(value)),
                               SCORE_NA, NULL, NULL, NULL, newprops);
  }
  return(ObjListCreateSP(ObjIntension(noun, newprops), score, bobj_sp->leo,
                         bobj_sp->pn, ObjListSPAnaphorAppend(newprops), r));
}

/* bobj:   adjective-argument
 * props:  [big adjective-argument]
 * result: [degree-interrogative-adverb [big adjective-argument]]
 */
ObjList *Sem_ParseIntAdv(Obj *bobj, Float score, ObjList *props,
                         LexEntryToObj *leo, SP *bobj_sp,
                         ObjList *r)
{
  if ((!StringIn(F_NO_BE_E, leo->features)) &&
      N("adjective-argument") == bobj) {
    r = ObjListCreateSP(ObjAdverb(leo->obj, props),
                        ScoreCombine(score, ObjListSPScoreCombine(props)),
                        bobj_sp->leo, bobj_sp->pn, 
                        ObjListSPAnaphorAppend(props),
                        r);
  } else if ((!StringIn(F_NO_BW_W, leo->features)) &&
             (!StringIn(F_NO_WB_W, leo->features)) &&
             N("adjective-argument") != bobj) {
    r = ObjListCreateSP(ObjAdverb(leo->obj, ObjListCreate(bobj, props)),
                        score, bobj_sp->leo, bobj_sp->pn, bobj_sp->anaphors,
                        r);
  }
  return(r);
}

ObjList *Sem_ParseNegAdv(Obj *bobj, Float score, LexEntryToObj *leo,
                         SP *bobj_sp, ObjList *r)
{
  return(ObjListCreateSP(L(N("not"), bobj, E), score, bobj_sp->leo, bobj_sp->pn,
                         bobj_sp->anaphors, r));
}

ObjList *Sem_ParseSuperlative(Obj *superlative, Obj *noun, Float noun_score,
                              ObjList *props, CaseFrame *cf, Discourse *dc)
{
  ObjList	*new_props, *p;
  new_props = NULL;
  for (p = props; p; p = p->next) {
    new_props = ObjListCreateSPFrom(ObjModCreateProp(I(p->obj, 0), noun,
                                                     superlative),
                                    p->u.sp.pn, p, new_props);
  }
  return(ObjListIntension(noun, new_props, noun_score, NULL, NULL, NULL));
}

ObjList *Sem_ParseAdv(PNode *pn, LexEntry *le, char *word, CaseFrame *cf,
                      Discourse *dc)
{
  Float		score;
  Obj		*bobj, *superlative;
  ObjList	*r, *props;
  LexEntryToObj	*leo;
  SP		bobj_sp;
  if (!(bobj = CaseFrameGet(cf, N("bobj"), &bobj_sp))) {
    Dbg(DBGSEMPAR, DBGBAD, "empty bobj");
    bobj = ObjNA;
  }
  props = ObjIntensionParse(bobj, 1, &bobj);
  if (props &&
      (superlative = LexEntrySuperlativeClass(word, DC(dc).lang))) {
    return(Sem_ParseSuperlative(superlative, bobj, bobj_sp.score,
                                props, cf, dc));
  }
  r = NULL;
  for (leo = LexEntryGetLeo(le); leo; leo = leo->next) {
    score = ScoreCombine(bobj_sp.score, LexEntryToObjScore(leo, dc));
    if (ISA(N("interrogative-adverb"), leo->obj)) {
      r = Sem_ParseIntAdv(bobj, score, props, leo, &bobj_sp, r);
    } else if (ISA(N("adverb-of-absolute-negation"), leo->obj) && !props) {
      r = Sem_ParseNegAdv(bobj, score, leo, &bobj_sp, r);
    } else if (ISA(N("first-adverb-of-absolute-negation"), leo->obj) &&
               !props) {
      r = ObjListCreateSP(bobj, score, leo, pn, bobj_sp.anaphors, r);
    } else if (ISA(N("adverb-of-absolute-degree"), leo->obj) && props) {
      r = Sem_ParseAdvObjMod(props, bobj, score, leo, &bobj_sp, r);
    } else if (ISA(N("question-preamble"), leo->obj)) {
    /* todo: Perhaps return that this is a question. But be careful as a similar
     * adverb may have exclamative value. But that would be another translation.
     */
      r = ObjListCreateSP(bobj, score, leo, pn, bobj_sp.anaphors, r);
    } else if (ISA(N("adverb-of-time"), leo->obj) ||
               ISA(N("temporal-relation"), leo->obj) ||
               ISA(N("adverb-exactly"), leo->obj)) {
      r = ObjListCreateSP(ObjAdverb1(leo->obj, bobj), score, leo, pn,
                          bobj_sp.anaphors, r);
    } else {
      r = ObjListCreateSP(ObjAdverb1(leo->obj, bobj), score, leo, pn,
                          bobj_sp.anaphors, r);
    }
    PNodeAddAllcons(pn, leo->obj);
  }
  return(r);
}

/* CONJUNCTION PARSING */

/* todo: Parser should maintain tenses (perhaps even ts's) for each S.
 * A conjunction concept will have two more slots giving the tenses.
 */
ObjList *Sem_ParseConj(PNode *pn, LexEntry *le, CaseFrame *cf, Discourse *dc)
{
  Float			score;
  LexEntryToObj		*leo;
  Obj			*obj1, *obj2;
  ObjList		*r;
  SP			obj1_sp, obj2_sp;
  r = NULL;
  obj1_sp.score = obj2_sp.score = SCORE_MAX;
  if ((obj1 = CaseFrameGet(cf, N("mkobj1"), &obj1_sp))) {
    obj2 = CaseFrameGet(cf, N("mkobj2"), &obj2_sp);
  } else if ((obj1 = CaseFrameGet(cf, N("kobj1"), &obj1_sp))) {
    obj2 = CaseFrameGet(cf, N("kobj2"), &obj2_sp);
  } else {
    Dbg(DBGSEMPAR, DBGBAD, "no kobjs");
    return(NULL);
  }
  for (leo = LexEntryGetLeo(le); leo; leo = leo->next) {
    score = ScoreCombine(LexEntryToObjScore(leo, dc),
                         ScoreCombine(obj1_sp.score, obj2_sp.score));
    /************************************************************************/
    if (ISA(N("logical-relation"), leo->obj)) {
      r = ObjListCreateSP(ObjMakeCoordConj(leo->obj, obj1, obj2), score,
                          leo, NULL,
                          AnaphorAppend(obj1_sp.anaphors, obj2_sp.anaphors),
                          r);
    /************************************************************************/
    } else if (ISA(N("standard-subordinating-conjunction"), leo->obj) &&
               !obj2) {
      r = ObjListCreateSP(obj1, score, leo, obj1_sp.pn, obj1_sp.anaphors, r);
    /************************************************************************/
    } else if (ISA(N("temporal-relation"), leo->obj)) {
    /* Time UA completes the parsing job later on. */
      if (obj2) {
        r = ObjListCreateSP(ObjMakeTemporal(leo->obj, obj1, obj2), score,
                            leo, NULL,
                            AnaphorAppend(obj1_sp.anaphors, obj2_sp.anaphors),
                            r);
      } else {
        r = ObjListCreateSP(L(leo->obj, obj1, E), score, leo, NULL,
                            obj1_sp.anaphors, r);
      }
    }
    /************************************************************************/
    PNodeAddAllcons(pn, leo->obj);
  }
  return(r);
}

/* DETERMINER PARSING */

/* Determiner concepts are passed as N("dobj") arguments to X-MAX's.
 * Examples:
 * lexitem    returns
 * -------    --------
 * "the"      N("definite-article")
 * "which"    N("interrogative-determiner")
 * "my"       N("possessive-determiner")
 */
ObjList *Sem_ParseDeterminer(PNode *pn, Lexitem *lexitem, CaseFrame *cf,
                             Discourse *dc)
{
  LexEntryToObj	*leo;
  ObjList	*r;
  r = NULL;
  for (leo = LexEntryGetLeo(lexitem->le); leo; leo = leo->next) {
    r = ObjListCreateSP(leo->obj, LexEntryToObjScore(leo, dc), leo, pn,
                        NULL, r);
    PNodeAddAllcons(pn, leo->obj);
  }
  return(r);
}

/* NOUN PARSING */

#ifdef notdef
/* in        r
 * ------    ------
 * grocer    [such-that human [occupation-of human grocer]]
 * American  [such-that human [nationality-of human US]]
 *           [such-that human [residence-of human US]]
 */
ObjList *Sem_ParseAttachmentExpand(ObjList *in, Float score, LexEntryToObj *leo,
                                   Discourse *dc)
{
  Obj		*con;
  ObjList	*p, *r, *attachments, *q;
  r = NULL;
  for (p = in; p; p = p->next) {
    if (StringIn(F_ATTACHMENT, leo->features) &&
        (attachments = Sem_ParseGetAttachments(p->obj))) {
      for (q = attachments; q; q = q->next) {
      /*            "grocer"                        "New Jerseyite"
       * p->obj:    N("grocer")                     N("New-Jersey")
       * q->obj:    [attachment-rel-of occupation occupation-of] 
       *                            [attachment-rel-of polity residence-of]
       */
        con = L(I(q->obj, 2), N("human"), p->obj, E);
        if (p->u.sp.pn) {
          ObjSetIthPNode(con, 2, p->u.sp.pn);
        } else {
          Stop();
        }
        r = ObjListCreateSP(ObjIntension1(N("human"), con), score, leo,
                            p->u.sp.pn, p->u.sp.anaphors, r);
      }
      ObjListFree(attachments);
    } else {
      r = ObjListCreateSP(p->obj, score, leo, p->u.sp.pn, p->u.sp.anaphors, r);
    }
  }
  /* todoFREE
  ObjListFree(in);
   */
  return(r);
}
#endif

#ifdef notdef
    The below was part of Sem_ParseNoun:
    Perhaps the below was put in for "What is ___?" questions.
    In any case it trashes the relation which is needed by copula parsing.
    If it is put back, also do the vanilla
       r = ObjListCreateSP(leo->obj, 1.0, leo, r)
    if (ISA(N("relation"), leo->obj)) {
      /* todo: Should be able to use relation to prune search. */
      r = ObjListCreateSP(DbGetRelationValue(&TsNA, NULL, N("r2"), 1.0,
                           leo->obj, N("concept")), NULL, r);
    }
#endif

/* todo: Must eliminate antecedents in semantic parses which are later
 * rejected.
 */
ObjList *Sem_ParseNounAdjuncts(PNode *pn, Float head_score,
                               LexEntryToObj *head_leo, Obj *noun,
                               CaseFrame *cf, Discourse *dc,
                               /* RESULTS */ int *accepted)
{
  Float		result_score;
  CaseFrame	*cf1;
  ObjList	*props, *r1, *noun_list;
  Obj		*of_iobj;
  LexEntry	*le_prep;
  SP		iobj_sp;

  if ((of_iobj = CaseFrameGetLe_DoNotMark(cf, N("iobj"), Sem_ParseOfLE(dc),
                                          &iobj_sp))) {
  /* todoSCORE: Genitive shouldn't lock out other adjuncts? 
   * Can Sem_ParseNounAdjuncts be merged with MAX adjuncts parsing?
   */
    return(Sem_ParseGenitive(noun, of_iobj,
                             ScoreCombine(head_score, iobj_sp.score), NULL,
                             pn, NULL, dc, NULL, accepted));
  } else {
    /* Penalize all noun adjuncts, so that verb arguments and adjuncts
     * take priority. See Hindle and Rooth (1993, p. 104).
     */
    result_score = ScoreCombine(0.9, head_score);

    props = NULL;
    noun_list = ObjListCreate(noun, NULL);
    for (cf1 = cf; cf1; cf1 = cf1->next) {
      if (cf1->cas != N("iobj")) continue;
      if ((le_prep = PNodeLeftmostPreposition(cf1->sp.pn))) {
        if ((r1 = Sem_Parse_PrepositionAdjunct(SCORE_MAX, head_leo, le_prep,
                                               noun_list, cf1->concept,
                                               0, dc))) {
          result_score = ScoreCombine(result_score, cf1->sp.score);
          props = ObjListAppendDestructive(props, r1);
          cf1->theta_marked = 1;
        } else {
        /* ? */
          return(NULL);
        }
      }
    }
    return(ObjListCreateSP(ObjIntension(noun, props), result_score, head_leo,
                           pn, NULL, NULL));
  }
}

Obj *Sem_ParseNounDeterminerWrap(Obj *obj, PNode *pn_obj, Obj *det_parsed,
                                 PNode *pn_det, int det_expected, Float inscore,
                                 /* RESULTS */ Float *score)
{
  ObjList	*props;
  if (ISA(N("interrogative-determiner"), det_parsed)) {
  /* "which book"
   * obj: book
   * det_parsed: interrogative-identification-determiner
   * result: [question-element book interrogative-identification-determiner]
   *
   * "how many breads"
   * obj: bread
   * det_parsed: interrogative-quantity-determiner
   * result: [question-element bread interrogative-quantity-determiner]
   */
    *score= ScoreCombine(inscore, 1.0);
    return(ObjQuestionElement(obj, det_parsed, NULL));
  } else if (det_expected == F_DEFINITE_ART) {
    if (ISA(N("definite-article"), det_parsed)) {
    /* "the Who" rock-group-the-Who */
      *score= ScoreCombine(inscore, 1.0);
      return(obj);
    } else if (det_parsed == NULL) {
    /* "who" rock-group-the-Who */
      *score= ScoreCombine(inscore, 0.7);
      return(obj);
    } else if (ISA(N("indefinite-article"), det_parsed)) {
    /* "a who" rock-group-the-Who */
      *score= ScoreCombine(inscore, 0.5);
      return(obj);
    } else if (ISA(N("possessive-determiner"), det_parsed)) {
    /* todoSCORE: "my Who" is perhaps possible, but for now we rule out. */
      *score = 0.0;
      return(NULL);
    } else {
      *score= ScoreCombine(inscore, 0.2);
      return(obj);
    }
  } else if (det_expected == F_EMPTY_ART) {
    if (det_parsed == NULL) {
    /* "Sony" Sony */
      *score = ScoreCombine(inscore, 1.0);
      return(obj);
    } else if (ISA(N("possessive-determiner"), det_parsed)) {
    /* todoSCORE: "my Sony" is perhaps possible, but for now we rule out. */
      *score = 0.0;
      return(NULL);
    } else {
    /* "the Sony", "a Sony", etc. */
      *score = ScoreCombine(inscore, 0.5);
      return(obj);
    }
  } else {	/* det_expected == F_NULL */
    if (ISA(N("F68"), det_parsed)) {
      *score = ScoreCombine(inscore, 1.0);
      if ((props = ObjIntensionParse(obj, 0, &obj))) {
      /* obj: [such-that human [political-affiliation-of human communist]]
       * =>
       * obj: human
       * props: [political-affiliation-of human communist]
       * =>
       * obj: [such-that [definite-article human]
       *                 [political-affiliation-of human communist]]
       */
        obj = L(det_parsed, obj, E);
        ObjSetIthPNode(obj, 0, pn_det);
        ObjSetIthPNode(obj, 1, pn_obj);
        obj = ObjIntension(obj, props);
      } else {
        obj = L(det_parsed, obj, E);
        ObjSetIthPNode(obj, 0, pn_det);
        ObjSetIthPNode(obj, 1, pn_obj);
      }
      return(obj);
    } else {
      *score = ScoreCombine(inscore, 1.0);
      return(obj);
    }
  }
}

ObjList *Sem_ParseNounDeterminerWraps(ObjList *in, Obj *det_parsed,
                                      PNode *pn_det, int det_expected)
{
  Float		score;
  Obj		*obj;
  ObjList	*p, *r;
  r = NULL;
  for (p = in; p; p = p->next) {
    if ((obj = Sem_ParseNounDeterminerWrap(p->obj, p->u.sp.pn, det_parsed,
                                           pn_det, det_expected, p->u.sp.score,
                                           &score))) {
      r = ObjListCreateSP(obj, score, p->u.sp.leo, p->u.sp.pn,
                          p->u.sp.anaphors, r);
    }
  }
/* todoFREE
  ObjListFree(in);
 */
  return(r);
}

ObjList *Sem_ParseNoun(LexEntry *le, PNode *pn, CaseFrame *cf, Discourse *dc)
{
  int			gender, number, accepted;
  Float			score;
  Obj			*det;
  ObjList		*r, *r1;
  LexEntryToObj		*leo;
  CaseFrame		*cf_removeme;
  SP			det_sp;

  cf_removeme = NULL;

  /* Map possessive-determiner N("dobj") to of+N("iobj"). */
  SPInit(&det_sp);
  det = CaseFrameGet(cf, N("dobj"), &det_sp);
  /* todo: "my going to the store" not a genitive but a theta marking. */

  PNodeGetFeatures(pn, &gender, &number);
  if (number == F_NULL) number = F_SINGULAR;
    /* Because of inflection collapsing. */
  r = NULL;
  for (leo = LexEntryGetLeo(le); leo; leo = leo->next) {
    if (F_NULL != FeatureGet(leo->features, FT_PARUNIV)) continue;
    if (TA_FilterOut(leo->obj, leo->features)) continue;
    score = LexEntryToObjScore(leo, dc);
    if (leo->theta_roles || ISA(N("action"), leo->obj)) {
      /* This noun takes arguments. todo: Auto-nominalization of all verbs. */
      r1 = Sem_ParseThetaMarking(score, leo, pn, F_NOUN, NULL, NULL, NULL,
                                 F_NULL, NULL, NULL, cf, dc, &accepted);
    } else {
      r1 = Sem_ParseNounAdjuncts(pn, score, leo, leo->obj, cf, dc, &accepted);
      accepted = 1;
    }
#ifdef notdef
    r1 = Sem_ParseAttachmentExpand(r1, score, leo, dc);
#endif
    r1 = Sem_ParseNounDeterminerWraps(r1, det, det_sp.pn,
                                      FeatureGet(leo->features, FT_ARTICLE));
    r = ObjListAppendDestructive(r, r1);
    if (accepted) PNodeAddAllcons(pn, leo->obj);
  }

  if (cf_removeme) CaseFrameRemove(cf_removeme);
  return(r);
}

/* Examples:
 * objs (respectively): [unique-author-of m.o.b. Art-Klep]
 *                      [mother-of Sara12 human]
 *                      [occupation-of human grocer] [of human Wiesengasse]
 *                      [nationality-of human country-USA]
 *                      [NAME-of human NAME:"Karen Garnier"]
 * noun_abstract:       media-object-book
 *                      human
 *                      human
 *                      human
 *                      human
 * noun_concrete:       Millbrook
 *                      Karen25
 *                      Jeanne-Püchl
 *                      [such-that human [NAME-of human NAME:"Karen Garnier"]]
 *                      [possessive-determiner sister-of]
 * Returns:             [such-that Millbrook [unique-author-of Millbrook
 *                                            Art-Klep]]
 *                      [such-that Karen25 [mother-of Sara12 Karen25]]
 *                      [such-that Jeanne-Püchl
 *                                 [occupation-of Jeanne-Püchl grocer]
 *                                 [of Jeanne-Püchl Wiesengasse]]
 *                      [such-that human [nationality-of human country-USA]
 *                                       [NAME-of human NAME:"Karen Garnier"]]
 *                      [such-that [possessive-determiner sister-of]
 *                                 [NAME-of [p-d s-o] NAME:"Karen Garnier"]]
 *
 * todo: The first three such-thats are an old format, since I(suchthat, 1)
 * should be a class, not an instance. Karen25 would not arise here anyway
 * since names are not resolved until Sem_Anaphora.
 */
ObjList *Sem_ParseAppositive2(ObjList *objs, Obj *noun_abstract,
                              Obj *noun_concrete, ObjList *r, Discourse *dc)
{
  Obj		*new, *noun;
  ObjList	*p, *props;

  if ((!ObjIsList(noun_concrete)) &&
      (!ISAP(noun_abstract, noun_concrete))) {
  /* "into the street at what time" */
    return(NULL);
  }

  if (N("possessive-determiner") == I(noun_concrete, 0)) {
    objs = ObjListCreate(L(N("standard-copula"), noun_concrete,
                           noun_abstract, E),
                         objs);
    r = ObjListIntension(noun_abstract, objs, SCORE_MAX, NULL, NULL, r);
    return(r);
  }
  if ((props = ObjIntensionParse(noun_concrete, 1, &noun))) {
    if (noun != noun_abstract) {
      Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseAppositive2: nonce"); /* todo */
    }
    props = ObjListAppendDestructive(props, objs);
    r = ObjListIntension(noun_abstract, props, SCORE_MAX, NULL, NULL, r);
    return(r);
  }

  props = NULL;
  for (p = objs; p; p = p->next) {
    new = ObjSubst(p->obj, noun_abstract, noun_concrete);
    TsRangeSetNa(ObjToTsRange(new));
    /* new: [unique-author-of Millbrook Art-Klep] */
    props = ObjListCreateSPFrom(new, p->u.sp.pn, p, props);
  }
  r = ObjListIntension(noun_concrete, props, SCORE_MAX, NULL, NULL, r);
  return(r);
}

/* Examples:
 *
 * Case (A):
 * cons1: [such-that [indefinite-article human]
 *                   [nationality-of human country-USA]]
 * cons2: [such-that human [NAME-of human NAME:"Karen Garnier"]]
 *
 * cons1: [such-that human [NAME-of human NAME:"Karen Garnier"]]
 * cons2: [possessive-determiner sister-of]
 *
 * "Art Kleps's book _Millbrook_"
 * cons1: [such-that book [unique-author-of book NAME:"Art Klep"]]
 * cons2: Millbrook
 * result: [such-that Millbrook [unique-author-of Millbrook NAME:"Art Klep"]]
 *
 * "l'épicière de la Wiesengasse, Mme Jeanne Püchl"
 * cons1: [such-that human [occupation-of human grocer] [of human Wiesengasse]]
 * cons2: Jeanne-Püchl
 *
 * "Jean Gandois, ancien président de Pechiney et président de Cockerill-Sambre"
 * cons1: [such-that human [president-of Pechiney human]
 *                      [president-of Pechiney Cockerill-Sambre]]
 * cons2: Jean-Gandois
 *
 * "Sara's mother Karen"
 * cons1: [such-that human [mother-of Sara12 human]]
 * cons2: Karen25
 *
 * "le reste --- à peine de quoi remplir un verre à liqueur"
 * cons1: [such-that nonhuman [fill ? liquor-glass nonhuman]]
 * cons2: remainder
 * result: [such-that remainder [fill ? liquor-glass remainder]]
 *
 * Case (B):
 * "l'épicière Mme Jeanne Püchl"
 * cons1: grocer
 * cons2: Jeanne-Püchl
 * result: [such-that Jeanne-Püchl [occupation-of Jeanne-Püchl grocer]]
 *
 * Case (C):
 * "my friend Mary"
 * cons1: [possessive-determiner friend-of]
 * cons2: Mary2
 * result: [such-that Mary2 [equiv Mary2 [possessive-determiner friend-of]]]
 *
 * todo:
 * - Tenses of props.
 * - The <class>, <instance>.
 */
ObjList *Sem_ParseAppositive1(PNode *pn, ObjList *cons1, ObjList *cons2,
                              CaseFrame *cf, Discourse *dc)
{
  Obj		*noun;
  ObjList	*p1, *p2, *r, *objs;
  r = NULL;
  for (p1 = cons1; p1; p1 = p1->next) {
#ifdef notdef
    if ((attachments = Sem_ParseGetAttachments(p1->obj))) {
    /* Case (B) */
      for (p2 = cons2; p2; p2 = p2->next) {
        for (q = attachments; q; q = q->next) {
        /* p1->obj:   N("grocer")
         * q->obj:    [attachment-rel-of occupation occupation-of]
         * p2->obj:   Jeanne-Püchl
         * result:    [such-that Jeanne-Püchl
         *             [occupation-of Jeanne-Püchl grocer]]
         */
          if (ISA(N("human"), p2->obj)) {
          /* todo: Attachments only apply to humans? */
            r = ObjListCreateSP(ObjIntension1(p2->obj,
                                      L(I(q->obj, 2), p2->obj, p1->obj, E)),
                                ScoreCombine(p1->u.sp.score, p2->u.sp.score),
                                NULL, pn,
                                AnaphorAppend(p1->u.sp.anaphors,
                                              p2->u.sp.anaphors),
                                r);
          }
        }
      }
      ObjListFree(attachments);
#else
    if (0) {
#endif
    } else if ((objs = ObjIntensionParse(p1->obj, 1, &noun))) {
    /* Case (A)
     * Example:
     * objs: [unique-author-of media-object-book Art-Klep]
     * noun: media-object-book
     */
      for (p2 = cons2; p2; p2 = p2->next) {
        if (!ISA(N("linguistic-concept"), noun)) {
        /* p2->obj: Millbrook */
          r = Sem_ParseAppositive2(objs, noun, p2->obj, r, dc);
        }
      }
      ObjListFree(objs);
    } else if (ISA(N("F68"), I(p1->obj, 0))) {
    /* Case (C)
     * Example:
     * p1->obj: [possessive-determiner friend-of]
     * cons2: Mary2
     * result: [such-that Mary2 [equiv Mary2 [possessive-determiner friend-of]]]
     * todo: This needs to be handled by Sem_Anaphora.
     */
      for (p2 = cons2; p2; p2 = p2->next) {
        if (ObjIsList(p2->obj)) continue;
        r = ObjListCreateSP(ObjIntension1(p2->obj,
                                        L(N("equiv"), p2->obj, p1->obj, E)),
                            ScoreCombine(p1->u.sp.score, p2->u.sp.score),
                            NULL, pn,
                            AnaphorAppend(p1->u.sp.anaphors,
                                          p2->u.sp.anaphors),
                            r);
      }
    }
  }
  return(r);
}

/* [[X Art Kleps's book] [X _Millbrook_]]
 * "Jean Gandois, ancien président de Pechiney et président de Cockerill-S,"
 * todo:
 * César le dalmatien.
 * Tang orange drink/Carnation breakfast drink.
 */
ObjList *Sem_ParseAppositive(PNode *pn, PNode *pn1, PNode *pn2, CaseFrame *cf,
                             Discourse *dc)
{
  ObjList	*cons1, *cons2, *r;
  if (NULL == (cons1 = Sem_ParseParse1(pn1, cf, dc))) return(NULL);
  if (NULL == (cons2 = Sem_ParseParse1(pn2, cf, dc))) return(NULL);
  if ((r = Sem_ParseAppositive1(pn, cons1, cons2, cf, dc))) return(r);
  if ((r = Sem_ParseAppositive1(pn, cons2, cons1, cf, dc))) return(r);
  return(NULL);
}

Float CommonInflScore(PNode *pn, SP *sp)
{
  if (pn && pn->lexitem && sp && sp->leo) {
    if (StringIn(F_COMMON_INFL, sp->leo->features)) {
      if (FeatureGet(pn->lexitem->features, FT_NUMBER) !=
          FeatureGet(sp->leo->features, FT_NUMBER)) {
        return 0.1;
      }
    }
  }
  return 1.0;
}

Obj *CNCToClass(char *cnc)
{
  if (cnc[0] == 'a') return N("action");
  if (cnc[0] == 'b') return N("attribute");
  if (cnc[0] == 'c') return N("concept");
  if (cnc[0] == 'h') return N("human");
  if (cnc[0] == 'o') return N("organization");
  if (cnc[0] == 'p') return N("physical-object");
  if (cnc[0] == 't') return N("temporal");
  return N("concept");
}

Obj *CNFocus(Obj *obj)
{
  char *cnc;
  int i, len, digit;
  if (!ObjIsList(obj)) return obj;
  if (!ISAP(N("compound-noun"), I(obj, 0))) return obj;
  cnc = M(I(obj, 0));
  len = strlen(cnc);
  digit = ' ';
  for (i = 0; i < len; i++) {
    if (cnc[i] == '-') break;
    if (cnc[i] == '1' || cnc[i] == '2') {
      digit = cnc[i];
      break;
    }
  }
  if (digit == '1') {
    return I(obj, 1);
  } else if (digit == '2') {
    return I(obj, 2);
  }
  return CNCToClass(cnc);
}

Bool CNISA(Obj *class, Obj *obj)
{
  if (ISAP(class, obj)) return 1;
  if (!ObjIsList(obj)) return 0;
  if (!ISAP(N("compound-noun"), I(obj, 0))) return 0;
  return CNISA(class, CNFocus(obj));
}

Bool CNIsActor(Obj *obj)
{
  return CNISA(N("animal"), obj) ||
         CNISA(N("organization"), obj);
}

Bool CNISArea(Obj *obj)
{
  return CNISA(N("interior-area"), obj) ||
         CNISA(N("exterior-area"), obj);
}

Bool CNIsLocation(Obj *obj)
{
  return CNISA(N("location"), obj);
}

Bool CNIsOccupation(Obj *obj)
{
  return CNISA(N("occupation"), obj) ||
         CNISA(N("employee-of"), obj);
}

ObjList *CNAdd(Obj *prop, Obj *o1, Obj *o2, PNode *pn1, PNode *pn2,
               Float score, ObjList *r)
{
  Obj	*elems[MAXLISTLEN];
  PNode	*pn_elems[MAXLISTLEN];
  Obj	*con;

  elems[0] = prop;
  elems[1] = o1;
  elems[2] = o2;
  pn_elems[0] = NULL;
  pn_elems[1] = pn1;
  pn_elems[2] = pn2;
  con = ObjCreateListPNode1(elems, pn_elems, 3);
  r = ObjListCreate(con, r);
  r->u.sp.score = score;
  return r;
}

/* part: "leg"
 * whole: "table"
 * [part-of leg human]
 */
Float CNPartScore(Obj *part, Obj *whole)
{
  ObjList *objs, *p;
  objs = RD(&TsNA, L(N("part-of"), part, ObjWild, E), 1);
  if (objs == NULL) return 0.0;
  for (p = objs; p; p = p->next) {
/*
    Dbg(DBGSEMPAR, DBGDETAIL, "CNParseScore: p->obj =");
    ObjPrint(Log, p->obj);
    fputc(NEWLINE, Log);
 */
    if (CNISA(I(p->obj, 2), whole)) return 0.1;
  }
  return 0.05;
}

Bool CNSubjectOf(Obj *action, Obj *subj)
{
  return DbRestrictValidate1(NULL, action, 1, subj, 0, 0);
}

Bool CNObjectOf(Obj *action, Obj *obj)
{
  return DbRestrictValidate1(NULL, action, 2, obj, 0, 0);
}

Bool CNHasObject(Obj *action)
{
  return DbHasRestriction(action, 2);
}

Bool CNEndsIn(PNode *pn, char *suffix)
{
  Lexitem *lexitem;
  lexitem = PNodeRightmostLexitem(pn);
  if (lexitem && lexitem->word) {
    return StringTailEq(lexitem->word, suffix);
  }
  return 0;
}

#ifdef notdef
ObjList *Sem_ParseCompoundNoun_NN1(PNode *pn1, PNode *pn2, Obj *o1, Obj *o2,
                                   SP *sp1, SP *sp2, CaseFrame *cf,
                                   Discourse *dc, ObjList *r)
{
  Float score, score1, size1, size2;
  Bool  sr;
  score =  0.8 *
           sp1->score * sp2->score *
           CommonInflScore(pn1, sp1) * CommonInflScore(pn2, sp2);
  if (score <= 0.0) return r;
  if (CNISA(N("matter"), o1) &&
      CNISA(N("physical-object"), o2)) {
    if (CNISA(N("container"), o2)) {
      /* todo: need to recurse into nested compound nouns to
       * determine size.
       */
      size1 = RoughSizeOf(&TsNA, CNFocus(o1));
      size2 = RoughSizeOf(&TsNA, CNFocus(o2));
      score1 = score;
      if (size1 != FLOATNA && size2 != FLOATNA) {
        if (size2 < size1) score1 *= size2/size1;
      } else {
        score1 += .04;
      } 
      r = CNAdd(N("p2-which-contains-p1"), o1, o2, pn1, pn2, score1, r);
    }
    r = CNAdd(N("p2-which-is-made-of-p1"), o1, o2, pn1, pn2,
              score+.01, r);
  }
  if (CNISA(N("physical-object"), o1) &&
      CNISA(N("physical-object"), o2)) {
    if (0.0 < (score1 = CNPartScore(o2, o1))) {
      score1 += score;
      r = CNAdd(N("p2-which-is-part-of-p1"), o1, o2, pn1, pn2, score1, r);
    }
    r = CNAdd(N("p2-which-resembles-p1-in-shape"), o1, o2, pn1, pn2, score, r);
    if (CNISA(N("power-source"), o1)) {
      score1 = score;
      if (CNISA(N("transportation-vehicle"), o2)) score1 += .04;
      r = CNAdd(N("p2-which-consumes-p1"), o1, o2, pn1, pn2, score1, r);
    }
    if (CNISA(N("building"), o2) ||
        CNISA(N("underground-area"), o2)) {
      r = CNAdd(N("p2-where-p1-is-produced"), o1, o2, pn1, pn2, score, r);
    }
    if (CNISArea(o2)) {
      r = CNAdd(N("p2-where-p1-is-located"), o1, o2, pn1, pn2, score, r);
    }
/*
    r = CNAdd(N("p2-which-holds-together-p1"), o1, o2, pn1, pn2, score, r);
    r = CNAdd(N("p1-which-resembles-p2-in-function"), o1, o2, pn1, pn2,
              score, r);
    r = CNAdd(N("h-who-has-p2-which-resembles-p1"), o1, o2, pn1, pn2, score, r);
 */
  }
  if (CNISA(N("physical-object"), o2) &&
      CNISA(N("action"), o1)) {
    if (CNISA(N("clothing"), o2)) {
      r = CNAdd(N("p2-which-is-worn-for-a1"), o1, o2, pn1, pn2, score+.04, r);
    }
    score1 = score;
    if (CNEndsIn(pn1, "ing")) score1 += .03;
    r = CNAdd(N("p2-which-is-used-for-a1"), o1, o2, pn1, pn2, score1, r);
    if (CNISArea(o2)) {
      r = CNAdd(N("p2-where-a1-occurs"), o1, o2, pn1, pn2, score+.02, r);
    }
    if (sr = CNObjectOf(o1, o2)) {
      score1 = score + ((sr == 1) ? 0.02 : 0.01);
      r = CNAdd(N("p2-which-h-a1"), o1, o2, pn1, pn2, score1, r);
    }
  }
  if (CNISA(N("physical-object"), o1) &&
      CNISA(N("action"), o2)) {
    if (sr = CNObjectOf(o2, o1)) {
      score1 = score + ((sr == 1) ? 0.02 : 0.01);
      if (CNISA(N("food"), o1)) {
        r = CNAdd(N("p-which-is-part-of-p1-obtained-by-a2-p1"),
                  o1, o2, pn1, pn2, score1+.02, r);
      }
      r = CNAdd(N("p-which-a2-p1"), o1, o2, pn1, pn2,
                score1, r); /* o2 should be infinitive or -er */
    }
  }
  if (CNISA(N("physical-object"), o2) &&
      CNISA(N("temporal"), o1)) {
    if (CNISA(N("clothing"), o2)) {
      r = CNAdd(N("p2-which-is-used-at-t1"), o1, o2, pn1, pn2, score+.02, r);
    }
    if (CNISA(N("media-object"), o2)) {
      r = CNAdd(N("p2-which-is-produced-at-t1"), o1, o2, pn1, pn2, score, r);
    }
  }
  if (CNISA(N("physical-object"), o2)) {
    if (CNISA(N("financial-instrument"), o1) ||
        CNISA(N("number"), o1)) {
      r = CNAdd(N("p2-whose-value-is-1"), o1, o2, pn1, pn2, score, r);
    }
    if (CNISA(N("force"), o1)) {
      r = CNAdd(N("p2-which-operates-by-creating-1"), o1, o2, pn1, pn2, score,
                r);
    }
    if (CNIsLocation(o1)) {
      score1 = score;
      if (CNISA(N("polity"), o1) ||
          CNISA(N("area"), o1) ||
          CNISArea(o2)) {
        score1 += .04;
      }
      r = CNAdd(N("p2-whose-source-is-1"), o1, o2, pn1, pn2, score1, r);
      r = CNAdd(N("p2-whose-destination-is-1"), o1, o2, pn1, pn2, score1, r);
      r = CNAdd(N("p2-which-is-located-at-1"), o1, o2, pn1, pn2, score1+.01, r);
    }
/*
    r = CNAdd(N("h-who-has-p2-which-resembles-p2-of-1"), o1, o2, pn1, pn2,
              score, r);
 */
  }
  if (CNIsActor(o1) && CNISA(N("object"), o2)) {
    score1 = score;
    if (CNISA(N("organization"), o2)) score1 += .01;
    r = CNAdd(N("2-which-is-owned-by-1"), o1, o2, pn1, pn2, score1, r);
  }
  if (CNISA(N("action"), o2) &&
      CNISA(N("temporal"), o1)) {
    r = CNAdd(N("a2-which-occurs-at-t1"), o1, o2, pn1, pn2, score+.04, r);
  }

  if (CNIsOccupation(o2) &&
      CNISA(N("part-of-the-day"), o1)) {
    r = CNAdd(N("h-who-is-2-at-t1"), o1, o2, pn1, pn2, score+.02, r);
  }
  if (CNISA(N("action"), o2)) {
    if (CNISA(N("location"), o1)) {
      r = CNAdd(N("h-who-a2-in-1"), o1, o2, pn1, pn2, score+.02, r);
    }
/*
    r = CNAdd(N("a2-whose-goal-is-1"), o1, o2, pn1, pn2, score, r);
 */
    if (sr = CNObjectOf(o2, o1)) {
      score1 = score + ((sr == 1) ? 0.02 : 0.01);
      r = CNAdd(N("a2-1"), o1, o2, pn1, pn2, score1, r);
      r = CNAdd(N("h-who-a2-1"), o1, o2, pn1, pn2, score1, r);
    }
    if (sr = CNSubjectOf(o2, o1)) {
      score1 = score + ((sr == 1) ? 0.02 : 0.01);
      r = CNAdd(N("c-which-results-from-1-a2"), o1, o2, pn1, pn2, score1, r);
      if (CNHasObject(o2)) {
        r = CNAdd(N("c-which-1-a2"), o1, o2, pn1, pn2, score1, r);
      }
    }
  } 
  if (CNISA(N("action"), o1)) {
/*
    r = CNAdd(N("h-who-is-2-of-a1"), o1, o2, pn1, pn2,
              score, r);
 */
    if (CNISA(N("information"), o2)) {
      r = CNAdd(N("2-which-results-from-a1"), o1, o2, pn1, pn2, score, r);
    }
    if (sr = CNSubjectOf(o1, o2)) {
      score1 = score + ((sr == 1) ? 0.02 : 0.01);
      r = CNAdd(N("2-which-a1"), o1, o2, pn1, pn2, score1, r);
    }
  }
  if (ISAP(N("attribute"), o2) && /* REALLY */
      (!CNISA(N("state"), o1)) &&
      (sr = CNSubjectOf(o2, o1))) {
    score1 = score + ((sr == 1) ? 0.02 : 0.01);
    r = CNAdd(N("b2-of-1"), o1, o2, pn1, pn2, score1, r);
  }
  if (CNISA(N("organization"), o1) &&
      CNISA(N("organization"), o2)) {
    r = CNAdd(N("o2-which-is-part-of-o1"), o1, o2, pn1, pn2, score+.01, r);
  }
  if (CNISA(N("organization"), o1)) {
    if (CNIsOccupation(o2)) {
      r = CNAdd(N("h-who-is-2-and-employed-by-o1"), o1, o2,
                pn1, pn2, score+.02, r);
    }
    if (CNISA(N("diploma"), o2) ||
        CNISA(N("financial-instrument"), o2)) {
      r = CNAdd(N("2-which-is-issued-by-o1"), o1, o2, pn1, pn2, score, r);
    }
  }
  if (CNISA(N("organization"), o2) || CNISA(N("group"), o2)) {
    r = CNAdd(N("o2-whose-topic-is-1"), o1, o2, pn1, pn2, score, r);
  }
  if ((CNISA(N("human"), o2) || CNIsOccupation(o2)) &&
      (CNISA(N("polity"), o1) || CNISA(N("area"), o1))) {
    r = CNAdd(N("h2-whose-residence-is-1"), o1, o2, pn1, pn2, score+.04, r);
  }
  if (CNISA(N("media-object"), o2) ||
      CNISA(N("legal-concept"), o2)) {
    r = CNAdd(N("2-whose-topic-is-1"), o1, o2, pn1, pn2, score, r);
  }
  if (CNISA(N("name"), o1)) {
    r = CNAdd(N("2-whose-name-is-1-2"), o1, o2, pn1, pn2, score, r);
  }
  if (CNISA(N("company"), o1)) {
    r = CNAdd(N("2-whose-brand-is-1"), o1, o2, pn1, pn2, score, r);
  }
  if (CNISA(N("polity"), o1)) {
    r = CNAdd(N("2-which-was-first-produced-in-1"), o1, o2, pn1, pn2, score, r);
  }
/*
  r = CNAdd(N("1-which-is-part-of-2"), o1, o2, pn1, pn2, score, r);
 */
  if (CNIsOccupation(o1) &&
      CNIsOccupation(o2)) {
    r = CNAdd(N("1-and-2"), o1, o2, pn1, pn2, score, r);
  }
/*
  r = CNAdd(N("1-2-is-a-2"), o1, o2, pn1, pn2, score, r);
 */
  return r;
}
#endif

ObjList *Sem_ParseCompoundNoun_NN1(PNode *pn1, PNode *pn2, Obj *o1, Obj *o2,
                                   SP *sp1, SP *sp2, CaseFrame *cf,
                                   Discourse *dc, ObjList *r)
{
  Float score, score1, size1, size2;
  Bool  sr;
  score =  0.8 *
           sp1->score * sp2->score *
           CommonInflScore(pn1, sp1) * CommonInflScore(pn2, sp2);
  score1 = score;
  if (score <= 0.0) return r;
  if (CNISA(N("matter"), o1) &&
      CNISA(N("physical-object"), o2)) {
    if (CNISA(N("container"), o2)) {
      size1 = RoughSizeOf(&TsNA, CNFocus(o1));
      size2 = RoughSizeOf(&TsNA, CNFocus(o2));
      score1 = score;
      if (size1 != FLOATNA && size2 != FLOATNA) {
        if (size2 < size1) score1 *= size2/size1;
      } else {
        score1 += .04;
      } 
      r = CNAdd(N("2-for-1"), o1, o2, pn1, pn2, score1, r);
    }
    r = CNAdd(N("2-of-1"), o1, o2, pn1, pn2,
              score+.01, r);
  }
  if (CNISA(N("physical-object"), o1) &&
      CNISA(N("physical-object"), o2)) {
    if (0.0 < (score1 = CNPartScore(o2, o1))) {
      score1 += score;
      r = CNAdd(N("2-of-1"), o1, o2, pn1, pn2, score1, r);
    }
    if (CNISA(N("power-source"), o1)) {
      score1 = score;
      if (CNISA(N("transportation-vehicle"), o2)) score1 += .04;
      r = CNAdd(N("2-of-1"), o1, o2, pn1, pn2, score1, r);
    }
    if (CNISA(N("building"), o2) ||
        CNISA(N("underground-area"), o2)) {
      r = CNAdd(N("2-for-1"), o1, o2, pn1, pn2, score, r);
    }
    if (CNISArea(o2)) {
      r = CNAdd(N("2-for-1"), o1, o2, pn1, pn2, score, r);
    }
  }
  if (CNISA(N("physical-object"), o2) &&
      CNISA(N("action"), o1)) {
    if (CNISA(N("clothing"), o2)) {
      r = CNAdd(N("2-for-1"), o1, o2, pn1, pn2, score+.04, r);
    }
    score1 = score;
    if (CNEndsIn(pn1, "ing")) score1 += .03;
    r = CNAdd(N("2-for-1"), o1, o2, pn1, pn2, score1, r);
    if (CNISArea(o2)) {
      r = CNAdd(N("2-for-1"), o1, o2, pn1, pn2, score+.02, r);
    }
  }
  if (CNISA(N("physical-object"), o1) &&
      CNISA(N("action"), o2)) {
    if ((sr = CNObjectOf(o2, o1))) {
      score1 = score + ((sr == 1) ? 0.02 : 0.01);
      if (CNISA(N("food"), o1)) {
        r = CNAdd(N("2-of-1"), o1, o2, pn1, pn2, score1+.02, r);
      }
    }
  }
  if (CNISA(N("physical-object"), o2) &&
      CNISA(N("temporal"), o1)) {
    if (CNISA(N("clothing"), o2)) {
      r = CNAdd(N("2-at-1"), o1, o2, pn1, pn2, score+.02, r);
    }
    if (CNISA(N("media-object"), o2)) {
      r = CNAdd(N("2-in-1"), o1, o2, pn1, pn2, score, r);
    }
  }
  if (CNISA(N("physical-object"), o2)) {
    if (CNISA(N("financial-instrument"), o1) ||
        CNISA(N("number"), o1)) {
      r = CNAdd(N("2-of-1"), o1, o2, pn1, pn2, score, r);
    }
    if (CNISA(N("force"), o1)) {
      r = CNAdd(N("2-of-1"), o1, o2, pn1, pn2, score,
                r);
    }
    if (CNIsLocation(o1)) {
      score1 = score;
      if (CNISA(N("polity"), o1) ||
          CNISA(N("area"), o1) ||
          CNISArea(o2)) {
        score1 += .04;
      }
      r = CNAdd(N("2-from-1"), o1, o2, pn1, pn2, score1, r);
      r = CNAdd(N("2-at-1"), o1, o2, pn1, pn2, score1+.01, r);
    }
  }
  if (CNIsActor(o1) && CNISA(N("object"), o2)) {
    score1 = score;
    if (CNISA(N("organization"), o2)) score1 += .01;
    r = CNAdd(N("2-of-1"), o1, o2, pn1, pn2, score1, r);
  }
  if (CNISA(N("action"), o2) &&
      CNISA(N("temporal"), o1)) {
    r = CNAdd(N("2-at-1"), o1, o2, pn1, pn2, score+.04, r);
  }

  if (CNIsOccupation(o2) &&
      CNISA(N("part-of-the-day"), o1)) {
    r = CNAdd(N("2-at-1"), o1, o2, pn1, pn2, score+.02, r);
  }
  if (CNISA(N("action"), o2)) {
    if (CNISA(N("location"), o1)) {
      r = CNAdd(N("2-in-1"), o1, o2, pn1, pn2, score+.02, r);
    }
  } 
  if (CNISA(N("action"), o1)) {
    if (CNISA(N("information"), o2)) {
      r = CNAdd(N("2-from-1"), o1, o2, pn1, pn2, score, r);
    }
  }
  if (ISAP(N("attribute"), o2) && /* REALLY */
      (!CNISA(N("state"), o1)) &&
      (sr = CNSubjectOf(o2, o1))) {
    score1 = score + ((sr == 1) ? 0.02 : 0.01);
    r = CNAdd(N("2-of-1"), o1, o2, pn1, pn2, score1, r);
  }
  if (CNISA(N("organization"), o1) &&
      CNISA(N("organization"), o2)) {
    r = CNAdd(N("2-of-1"), o1, o2, pn1, pn2, score+.01, r);
  }
  if (CNISA(N("organization"), o1)) {
    if (CNIsOccupation(o2)) {
      r = CNAdd(N("2-at-1"), o1, o2,
                pn1, pn2, score+.02, r);
    }
    if (CNISA(N("diploma"), o2) ||
        CNISA(N("financial-instrument"), o2)) {
      r = CNAdd(N("2-from-1"), o1, o2, pn1, pn2, score, r);
    }
  }
  if (CNISA(N("organization"), o2) || CNISA(N("group"), o2)) {
    r = CNAdd(N("2-of-1"), o1, o2, pn1, pn2, score, r);
  }
  if ((CNISA(N("human"), o2) || CNIsOccupation(o2)) &&
      (CNISA(N("polity"), o1) || CNISA(N("area"), o1))) {
    r = CNAdd(N("2-from-1"), o1, o2, pn1, pn2, score+.04, r);
  }
  if (CNISA(N("media-object"), o2) ||
      CNISA(N("legal-concept"), o2)) {
    r = CNAdd(N("2-of-1"), o1, o2, pn1, pn2, score, r);
  }
  if (CNISA(N("company"), o1)) {
    r = CNAdd(N("2-of-1"), o1, o2, pn1, pn2, score, r);
  }
  if (CNISA(N("polity"), o1)) {
    r = CNAdd(N("2-from-1"), o1, o2, pn1, pn2, score, r);
  }
  return r;
}

ObjList *Sem_ParseCompoundNoun_NN(PNode *pn1, PNode *pn2,
                                  CaseFrame *cf, Discourse *dc)
{
  ObjList *cons1, *cons2, *p1, *p2, *r;

  Dbg(DBGSEMPAR, DBGDETAIL, "CompoundNoun: N N");
  cons1 = Sem_ParseParse1(pn1, cf, dc);
  cons2 = Sem_ParseParse1(pn2, cf, dc);
  r = NULL;
  for (p1 = cons1; p1; p1 = p1->next) {
    for (p2 = cons2; p2; p2 = p2->next) {
      r = Sem_ParseCompoundNoun_NN1(pn1, pn2, p1->obj, p2->obj,
                                    &p1->u.sp, &p2->u.sp, cf, dc, r);
    }
  }
  return r;
}

ObjList *Sem_ParseNP(PNode *pn, CaseFrame *cf, Discourse *dc)
{
  /************************************************************************/
  if (pn->pn1->feature == F_NP && pn->pn2->feature == F_ADJP) {
    return(Sem_CartProduct(pn->pn1, pn->pn2, NULL, NULL, cf, dc,
                           N("aobj-postposed"), NULL, NULL));
  /************************************************************************/
  } else if (pn->pn1->feature == F_ADJP && pn->pn2->feature == F_NP) {
    return(Sem_CartProduct(pn->pn2, pn->pn1, NULL, NULL, cf, dc,
                           N("aobj-preposed"), NULL, NULL));
  /************************************************************************/
  } else if (pn->pn1->feature == F_NP && pn->pn2->feature == F_NP) {
    if (dc->mode & DC_MODE_COMPOUND_NOUN) {
      return(Sem_ParseCompoundNoun_NN(pn->pn1, pn->pn2, cf, dc));
    } else if (Syn_ParseIsNPNP_Genitive(pn, DC(dc).lang)) {
      /* Jim's stereo */
      return(Sem_ParseParse2_OfIOBJ(pn->pn1->pn1, pn->pn2, cf, dc));
#ifdef notdef
    /* I don't know what this was for anymore. It should already be handled
     * by the apposition case.
     */
    } else if (LexEntryConceptIsAncestor(N("simple-demonstrative-pronoun"),
                                         PNodeSingletonLexEntry(pn->pn2))) {
      return(Sem_ParseParse1(pn->pn1, cf, dc));
#endif
    } else if (LexEntryAnyConceptsFeat(F_COORDINATOR,
                                       PNodeLeftmostLexEntry(pn->pn2))) {
    /* todo: But what about a coordinator that has a noncoordinator meaning?
     * Are there any?
     */
      return(Sem_CartProduct(pn->pn1, pn->pn2, NULL, NULL, cf, dc,
                             N("mkobj2"), NULL, NULL));
    } else {
      /* [[X Art Kleps's book] [X _Millbrook_]] */
      return(Sem_ParseAppositive(pn, pn->pn1, pn->pn2, cf, dc));
    }
  /************************************************************************/
  } else if (pn->pn1->feature == F_CONJUNCTION && pn->pn2->feature == F_NP) {
    return(Sem_CartProduct(pn->pn2, pn->pn1, NULL, NULL, cf, dc, N("mkobj1"),
                           NULL, NULL));
  /************************************************************************/
  } else if (pn->pn1->feature == F_DETERMINER && pn->pn2->feature == F_NP) {
    return(Sem_CartProduct(pn->pn1, pn->pn2, NULL, NULL, cf, dc, N("dobj"),
                           NULL, NULL));
  /************************************************************************/
  } else if (pn->pn1->feature == F_NP && pn->pn2->feature == F_PP) {
    /* of+PP genitives are handled in Sem_ParseNoun.
     * Complete GenS_EquativeRole's are parsed in Sem_ParseCopula: the PP is an
     * argument to the verb, not the np. That is the right place because the
     * subject NP is available only in the verb.
     * In contrast, in Sem_ParseNoun an existing OF retrieval is done.
     */
    return(Sem_CartProduct(pn->pn2, pn->pn1, N("Y-MAX"), NULL, cf, dc,
                           N("iobj"), NULL, NULL));
  /************************************************************************/
  } else if (pn->pn1->feature == F_NP && pn->pn2->feature == F_ELEMENT) {
  /* This is supposed to be parsed at a higher level. If we see it here,
   * then this parse is ruled out.
   */
    return(NULL);
  /************************************************************************/
  } else if (pn->pn1->feature == F_NP && pn->pn2->feature == F_VP) {
    return(Sem_ParseRelClause(pn, pn->pn1, pn->pn2, cf, dc));
  /************************************************************************/
  } else if (pn->pn1->feature == F_NP && pn->pn2->feature == F_S) {
    return(Sem_ParseRelClause(pn, pn->pn1, pn->pn2, cf, dc));
  /************************************************************************/
  } else {
    Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseNP: unknown case");
    return(NULL);
  }
}

/* PREPOSITION PARSING */

ObjList *Sem_ParsePrep(PNode *pn, LexEntry *le, CaseFrame *cf, Discourse *dc)
{
  LexEntryToObj	*leo;
  Obj		*pobj, *res;
  ObjList	*r;
  SP		pobj_sp;
  r = NULL;
  if (!(pobj = CaseFrameGet(cf, N("pobj"), &pobj_sp))) {
    Dbg(DBGSEMPAR, DBGBAD, "empty pobj");
    return(NULL);
  }
  for (leo = LexEntryGetLeo(le); leo; leo = leo->next) {
    if ((N("prep-chez") == leo->obj) &&
        (res = R1EI(2, DCNOW(dc), L(N("residence-of"), pobj, ObjWild, E)))) {
    /* todo: Multiple ts and multiple residences in a given ts. */
      r = ObjListCreateSP(res,
                          ScoreCombine(pobj_sp.score,
                                       LexEntryToObjScore(leo, dc)),
                          leo,
                          pobj_sp.pn,
                          pobj_sp.anaphors,
                          r);
    }
    PNodeAddAllcons(pn, leo->obj);
  }
  return(ObjListCreateSP(pobj, pobj_sp.score, leo, pobj_sp.pn,
                         pobj_sp.anaphors, r));
}

ObjList *Sem_ParsePP(PNode *pn, CaseFrame *cf, Discourse *dc)
{
  if (pn->pn1->feature == F_ADVERB && pn->pn2->feature == F_PP) {
    /* todo: Parse modifications in pn1. */
    if (pn->pn1->type == PNTYPE_TSRANGE) {
    /* todo: Inelegant. Just for Perutz sentence 1: If we don't process
     * a tsr on the PP, then this isn't a valid parse. This goes away
     * when we "parse modifications in pn1".
     */
      return(NULL);
    }
    return(Sem_ParseParse1(pn->pn2, cf, dc));
  } else if (pn->pn1->feature == F_PP && pn->pn2->feature == F_PP) {
    return(Sem_CartProduct(pn->pn1, pn->pn2, NULL, NULL, cf, dc, N("mkobj2"),
                           NULL, NULL));
  } else if (pn->pn1->feature == F_CONJUNCTION && pn->pn2->feature == F_PP) {
    return(Sem_CartProduct(pn->pn2, pn->pn1, NULL, NULL, cf, dc, N("mkobj1"),
                           NULL, NULL));
  } else if (pn->pn1->feature == F_PREPOSITION && pn->pn2->feature == F_NP) {
    return(Sem_CartProduct(pn->pn2, pn->pn1, N("X-MAX"), NULL, cf, dc,
                           N("pobj"), NULL, NULL));
  } else if (pn->pn1->feature == F_PREPOSITION &&
             pn->pn2->feature == F_ADVERB) {
    if (pn->pn2->type == PNTYPE_TSRANGE) {
      return(ObjListCreateSP(TsRangeToObj(pn->pn2->u.tsr), SCORE_MAX, NULL,
                             NULL, NULL, NULL));
        /* todoSCORE */
    } else {
      return(NULL);
    }
  }
  PNodePrint(Log, DiscourseGetInputChannel(dc), pn);
  Dbg(DBGSEMPAR, DBGBAD, "Sem_ParsePP: unknown case");
  return(NULL);
}

/* PRONOUN PARSING */

/* todo: Handle en, y. */
ObjList *Sem_ParsePronoun1(PNode *pn, LexEntryToObj *leo, Obj *obj,
                           Lexitem *lexitem, ObjList *r, Discourse *dc)
{
  if (ISA(N("personal-pronoun"), obj)) {
    return(ObjListCreateSP(obj, SCORE_MAX, leo, pn, NULL, r));
  } else if (ISA(N("interrogative-pronoun"), obj)) {
    return(ObjListCreateSP(obj, SCORE_MAX, leo, pn, NULL, r));
  } else if (ISA(N("expletive-pronoun"), obj)) {
    if (FeatureMatch(F_THIRD_PERSON,
                     FeatureGet(lexitem->features, FT_PERSON)) &&
        FeatureMatch(F_SINGULAR, FeatureGet(lexitem->features, FT_NUMBER))) {
      return(ObjListCreateSP(obj, SCORE_MAX, leo, pn, NULL, r));
    }
  } else {
  /* Other things such as N("relative-pronoun") are dealt with elsewhere.
   * If we return them here, then we return superfluous parses.
   */
    return(NULL);
  }
  return(r);
}

ObjList *Sem_ParsePronoun(PNode *pn, Lexitem *lexitem, Discourse *dc)
{
  LexEntryToObj	*leo;
  ObjList		*r;
  r = NULL;
  for (leo = LexEntryGetLeo(lexitem->le); leo; leo = leo->next) {
    r = Sem_ParsePronoun1(pn, leo, leo->obj, lexitem, r, dc);
    PNodeAddAllcons(pn, leo->obj);
  }
  if (!r) {
    r = ObjListCreateSP(N("concept"), SCORE_MAX, leo, pn, NULL, r);
  }
  return(r);
}

/* VERB PARSING */

Bool ProgressiveIsOK(Obj *obj, char *features)
{
  if (StringIn(F_NO_PROGRESSIVE, features)) return(0);
#ifdef notdef
    "Jim Garnier was sleeping.": This is progressive but it
    refers to what is considered a state in ThoughtTreasure.
  if (ISA(N("state"), obj)) {
  /* See Lyons (1977, pp. 706, 712). */
    return(0);
  }
#endif
  return(1);
}

ObjList *Sem_ParseVerb1(PNode *pn_mainverb, Obj *comptense, CaseFrame *cf,
                        Discourse *dc)
{
  int			accepted, progressive;
  LexEntry		*le;
  LexEntryToObj	*leo;
  ObjList		*r;
  if (!(le = PNodeLeftmostLexEntry(pn_mainverb))) {
    Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseVerb");
    return(NULL);
  }
  r = NULL;
  progressive = ISA(N("progressive-tense"), comptense);
  for (leo = LexEntryGetLeo(le); leo; leo = leo->next) {
    if (ISA(N("auxiliary-verb"), leo->obj)) continue;
    if (progressive && !ProgressiveIsOK(leo->obj, leo->features)) {
    /* todoSCORE? "I have dinner" => [part-of Jim dinner] */
      continue;
    }
    r = Sem_ParseThetaMarking(LexEntryToObjScore(leo, dc), leo, pn_mainverb,
                              F_VERB, comptense, NULL, NULL, F_NULL,
                              NULL, r, cf, dc, &accepted);
    if (accepted) {
      PNodeAddAllcons(pn_mainverb, leo->obj);
    }
  }
  return(r);
}

ObjList *Sem_ParseVerb(PNode *pn, CaseFrame *cf, Discourse *dc)
{
  Obj		*tense, *comptense;
  ObjList	*r;
  r = NULL;
  if (dc->cth.rightmost) {
  /* We found the main verb. Set a global variable used by the below
   * (whether now or in a later call back to here).
   */
    dc->cth.mainverb = pn;
  }

  tense = FeatToCon(FeatureGet(pn->lexitem->features, FT_TENSE));
  dc->cth.mood_r = FeatToCon(FeatureGet(pn->lexitem->features, FT_MOOD));
  dc->cth.tense_r = TenseFindInflTense(DC(dc).lang, tense, dc->cth.mood_r);

  if (dc->cth.leftmost && dc->cth.mainverb) {
  /* We found the auxiliary verb. Time to do theta-marking. */
    comptense = TenseFindMainAux(DC(dc).lang, dc->cth.tense_r,
                                 PNodeLeftmostLexEntry(pn),
                                 dc->cth.maintense_a);
      /* todo: This word is duplicated on return. */
    r = Sem_ParseVerb1(dc->cth.mainverb, comptense, cf, dc);
  }

  dc->cth.mainverb_r = pn;

  return(r);
}

ObjList *Sem_ParseVP_Pronoun(PNode *vp, PNode *pronoun, CaseFrame *cf,
                             Discourse *dc)
{
  int		do_expl;
  CaseFrame	*cf1;
  LexEntry	*le;
  ObjList	*r, *r1;
  le = PNodeLeftmostLexEntry(pronoun);

  do_expl = 0;
  r = NULL;
  if (LexEntryConceptIsAncestor(N("subject-pronoun"), le)) {
  /* todo: If a subj is already in <cf>, ignore it. */
    if (DC(dc).lang == F_FRENCH &&
        CaseFrameGet(cf, N("subj"), NULL)) {
    /* Redundant subject pronoun, as in: "Où Bonaparte est-il né".
     * todo: Verify agreement of <pronoun> and <cf> subj.
     */
      r1 = Sem_ParseParse1(vp, cf, dc);
    } else {
      r1 = Sem_CartProduct(pronoun, vp, NULL, NULL, cf, dc, N("subj"), NULL,
                           NULL);
    }
    r = ObjListAppendDestructive(r, r1);
  }
  if (LexEntryConceptIsAncestor(N("direct-object-pronoun"), le)) {
    r1 = Sem_CartProduct(pronoun, vp, NULL, NULL, cf, dc, N("obj"), NULL, NULL);
    r = ObjListAppendDestructive(r, r1);
  }
  if (LexEntryConceptIsAncestor(N("indirect-object-pronoun"), le)) {
    r1 = Sem_CartProduct(pronoun, vp, NULL, NULL, cf, dc, N("iobj"), NULL,
                         Sem_ParseToPN(dc));
    r = ObjListAppendDestructive(r, r1);
  }
  if (LexEntryConceptIsAncestor(N("pronoun-en"), le)) {
    do_expl = 1;
    r1 = Sem_CartProduct(pronoun, vp, NULL, NULL, cf, dc, N("iobj"), NULL,
                         Sem_ParseOfPN(dc));
    r = ObjListAppendDestructive(r, r1);
  }
  if (LexEntryConceptIsAncestor(N("pronoun-y"), le)) {
    do_expl = 1;
    r1 = Sem_CartProduct(pronoun, vp, NULL, NULL, cf, dc, N("iobj"), NULL,
                         Sem_ParseToPN(dc));
    r = ObjListAppendDestructive(r, r1);
  }
  if (LexEntryConceptIsAncestor(N("reflexive-pronoun"), le)) {
    do_expl = 1;
    r1 = Sem_CartProduct(pronoun, vp, NULL, NULL, cf, dc, N("obj"), NULL, NULL);
    r = ObjListAppendDestructive(r, r1);
  }
  if (do_expl) {
    cf1 = CaseFrameAdd(cf, N("expl"), NULL, SCORE_MAX, pronoun, NULL,
                       NULL);
      /* todoSCORE */
    r1 = Sem_ParseParse1(vp, cf1, dc);
    CaseFrameRemove(cf1);
    r = ObjListAppendDestructive(r, r1);
  }
  return(r);
}

ObjList *Sem_ParseVP_Adv(PNode *vp, PNode *adv, CaseFrame *cf, Discourse *dc)
{
  int		is_just;
  ObjList	*r;
  CaseFrame	*cf1;
  is_just = (DC(dc).lang == F_ENGLISH && adv->lexitem && adv->lexitem->le &&
             streq("just", adv->lexitem->le->srcphrase));
  if (is_just) {
    cf1 = cf;
  } else if (adv->type == PNTYPE_TSRANGE) {
    cf1 = CaseFrameAdd(cf, N("tsrobj"), TsRangeToObj(adv->u.tsr),
                       SCORE_MAX, NULL, NULL, NULL);
      /* todoSCORE */
  } else {
    cf1 = CaseFrameAdd(cf, N("expl"), NULL, SCORE_MAX, adv, NULL, NULL);
      /* todoSCORE */
  }
  /* dc->cth.leftmost and dc->cth.rightmost arguments unchanged. */
  r = Sem_ParseParse1(vp, cf1, dc);
  if (is_just) {
    if (dc->cth.tense_r == N("preterit-indicative")) {
      dc->cth.tense_r = N("past-recent");
    }
  } else {
    CaseFrameRemove(cf1);
  }
  /* Otherwise keep return values dc->cth.mainverb_r, dc->cth.tense_r,
   * dc->cth.mood_r from recursive call.
   */
  return(r);
}

ObjList *Sem_ParseVP(PNode *pn, CaseFrame *cf, Discourse *dc)
{
  int			save_leftmost, save_rightmost;
  Obj			*subj, *tense2;
  ObjList		*r, *r1;
  CaseFrame		*cf1;
  PNode			*pnverb2;
  SP			subj_sp;

  /************************************************************************/
  if (pn->pn1->feature == F_VP && pn->pn2->feature == F_ADJP) {
    /* Adj parsing needs work:
     * Problem examples:
     * "La bouteille n'était pas vide." -- negation is ignored.
     * "Je me suis couché sur le lit." -- accepts the Adj reading.
     */

    if (!(subj = CaseFrameGet(cf, N("subj"), &subj_sp))) {
      if (Syn_ParseIsNPVerbInversionVP(pn->pn1, DC(dc).lang)) {
        /* We ignore copula */
        return(Sem_CartProduct(pn->pn1->pn2, pn->pn2, N("X-MAX"), N("E-MAX"),
                               cf, dc, N("aobj-predicative"), NULL, NULL));
      } else {
        Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseVP: empty subj and not inversion");
        return(NULL);
      }
    } else {
      /* Ignore meaning of copula for now, but at least process the verb
       * phrase to determine its compound tense and set dc->ct_*.
       */
      Sem_ParseParse1(pn->pn1, NULL, dc);
      cf1 = CaseFrameAddSP(cf, N("aobj-predicative"), subj, &subj_sp);
      r = Sem_ParseParse1(pn->pn2, cf1, dc);
      CaseFrameRemove(cf1);
      return(r);
    }
  /************************************************************************/
  } else if (pn->pn1->feature == F_VP && pn->pn2->feature == F_VERB) {
    save_leftmost = dc->cth.leftmost;
    save_rightmost = dc->cth.rightmost;

    /* Parse the main verb first. */
    dc->cth.leftmost = 0;
    dc->cth.maintense_a = NULL;
    Sem_ParseParse1(pn->pn2, NULL, dc);
    dc->cth.leftmost = save_leftmost;
    dc->cth.rightmost = save_rightmost;
    pnverb2 = dc->cth.mainverb_r;
    tense2 = dc->cth.tense_r;

    /* Parse the auxiliary second, because verb theta-marking is done inside
     * the auxiliary and all its arguments should be available at that point.
     */
    dc->cth.rightmost = 0;
    dc->cth.maintense_a = dc->cth.tense_r;
    r = Sem_ParseParse1(pn->pn1, cf, dc);
    dc->cth.leftmost = save_leftmost;
    dc->cth.rightmost = save_rightmost;

    dc->cth.tense_r =
      TenseFindMainAux(DC(dc).lang, dc->cth.tense_r,
                       PNodeLeftmostLexEntry(dc->cth.mainverb_r),
                       tense2);
    dc->cth.mainverb_r = pnverb2;
    /* Keep return value dc->cth.mood_r from recursive call on auxiliary. */
    return(r);
  /************************************************************************/
  } else if (pn->pn1->feature == F_VP && pn->pn2->feature == F_ADVERB) {
    return(Sem_ParseVP_Adv(pn->pn1, pn->pn2, cf, dc));
  /************************************************************************/
  } else if (pn->pn1->feature == F_ADVERB && pn->pn2->feature == F_VP) {
    return(Sem_ParseVP_Adv(pn->pn2, pn->pn1, cf, dc));
  /************************************************************************/
  } else if (pn->pn1->feature == F_VP && pn->pn2->feature == F_PRONOUN) {
    return(Sem_ParseVP_Pronoun(pn->pn1, pn->pn2, cf, dc));
  /************************************************************************/
  } else if (pn->pn1->feature == F_PRONOUN && pn->pn2->feature == F_VP) {
    if (LexEntryConceptIsAncestor(N("relative-pronoun"),
                                  PNodeLeftmostLexEntry(pn->pn1))) {
    /* Relative clause: [X [X femme] [W [H qui] [W ...]]] */
      return(Sem_ParseParse1(pn->pn2, cf, dc));
    } else {
      return(Sem_ParseVP_Pronoun(pn->pn2, pn->pn1, cf, dc));
    }
  /************************************************************************/
  } else if (pn->pn1->feature == F_PREPOSITION && pn->pn2->feature == F_VP) {
    r = Sem_ParseParse1(pn->pn2, cf, dc);
    if (dc->cth.tense_r == N("infinitive") && DC(dc).lang == F_ENGLISH &&
        streq("to", pn->pn1->lexitem->le->srcphrase)) {
      dc->cth.tense_r = N("infinitive-with-to");
    }
    return(r);
  /************************************************************************/
  } else if (pn->pn1->feature == F_VP && pn->pn2->feature == F_EXPLETIVE) {
    cf1 = CaseFrameAdd(cf, N("expl"), NULL, SCORE_MAX, pn->pn2, NULL,
                       NULL);
      /* todoSCORE? */
    r = Sem_ParseParse1(pn->pn1, cf1, dc);
    CaseFrameRemove(cf1);
    return(r);
  /************************************************************************/
  } else if (pn->pn1->feature == F_VP && pn->pn2->feature == F_PP) {
    return(Sem_CartProduct(pn->pn2, pn->pn1, N("Y-MAX"), NULL, cf, dc,
                           N("iobj"), NULL, NULL));
  /************************************************************************/
  } else if (pn->pn1->feature == F_VP && pn->pn2->feature == F_NP) {
    if (NULL == CaseFrameGet(cf, N("subj"), NULL) &&
        Syn_ParseIsNPVerbInversion(pn->pn1, pn->pn2, DC(dc).lang)) {
    /* Also, if there is already an "obj" or "iobj" in <cf> (from XZ_Z or
     * YZ_Z), we are probably on the right track.
     */
      r1 = Sem_CartProduct(pn->pn2, pn->pn1, N("X-MAX"), NULL, cf, dc,
                           N("subj"), NULL, NULL);
    } else {
      r1 = NULL;
    }
    r = Sem_CartProduct(pn->pn2, pn->pn1, N("X-MAX"), NULL, cf, dc, N("obj"),
                        NULL, NULL);
    return(ObjListAppendDestructive(r1, r));
  }
  /************************************************************************/
  PNodePrint(Log, DiscourseGetInputChannel(dc), pn);
  Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseVP: unknown case");
  return(NULL);
}

/* SENTENCE PARSING */

Bool Sem_ParseS1(PNode *pn1, PNode *pn2, CaseFrame *cf, Discourse *dc,
                 ObjList **r)
{
  if (pn1->feature == F_S && pn2->feature == F_S && PNodeKIsLeftmost(pn1)) {
    *r = Sem_CartProduct(pn2, pn1, NULL, NULL, cf, dc, N("kobj2"), N("subj"),
                         NULL);
    return(1);
  } else if (pn1->feature == F_CONJUNCTION && pn2->feature == F_S) {
  /* todo: Move this up to caller. */
    *r = Sem_CartProduct(pn2, pn1, NULL, NULL, cf, dc, N("kobj1"), NULL, NULL);
    return(1);
  } else if (pn1->feature == F_ADVERB && pn2->feature == F_S) {
    *r = Sem_CartProduct(pn2, pn1, NULL, NULL, cf, dc, N("bobj"), NULL, NULL);
    return(1);
  } else {
    *r = NULL;
    return(0);
  }
}

ObjList *Sem_ParseS(PNode *pn, CaseFrame *cf, Discourse *dc)
{
  ObjList	*r;
  PNode		*vp;
  CaseFrame	*cf1;
  /************************************************************************/
  if (pn->pn1->feature == F_NP && pn->pn2->feature == F_S) {
  /* Interrogative "obj": "What did you buy?"
   * The relative pronoun case is caught by Sem_ParseNonNominalRelClause
   * before we get here.
   */
    return(NULL);
#ifdef notdef
    /* todo: too many parses generated; need to prune garbage */
    return(Sem_CartProduct(pn->pn1, pn->pn2, N("X-MAX"), NULL, cf, dc, N("obj"),
                           NULL, NULL));
      /* pn1 and pn2 used to be flipped. this seems wrong. */
#endif
  /************************************************************************/
  } else if (pn->pn1->feature == F_PP && pn->pn2->feature == F_S) {
  /* (1) The relative pronoun case is caught by Sem_ParseNonNominalRelClause
   *   before we get here. Remaining cases are:
   * (2) "De quel instrument jouait Paganini?"
   *   This is when PNodeClassIn(pn->pn1, N("question-word"))
   * (3) "Par précaution, elle rangea la bouteille dans l'armoire de cuisine."
   *   todo: If these are always adjuncts, we could mark them as such and make
   *   use of this fact. However it is not clear this is the case. For example:
   *   "Off to the store she went."
   *   Actually, if we know this to be an adjunct, better to deal with it here:
   *   (A) Parse pn->pn1, parse pn->pn2.
   *   (B) Return their Cartesian product [<adjunct-relation> <r1i> <r2i>]
   */
    return(Sem_CartProduct(pn->pn1, pn->pn2, N("Y-MAX"), NULL, cf, dc,
                           N("iobj"), NULL, NULL));
  /************************************************************************/
  } else if (pn->pn1->feature == F_ADJP && pn->pn2->feature == F_VP) {
  /* "How big is she?" cf Sem_ParseCopula */
    cf1 = CaseFrameAdd(cf, N("aobj-preposed"), N("adjective-argument"),
                       SCORE_MAX, NULL, NULL, NULL);
    r = Sem_CartProduct(pn->pn1, pn->pn2, NULL, NULL, cf1, dc, N("obj"),
                        NULL, NULL);
    CaseFrameRemove(cf1);
    r = ObjListRemoveInDeep(r, N("adjective-argument"));
    return(r);
  /************************************************************************/
  } else if (pn->pn1->feature == F_NP && pn->pn2->feature == F_ADJP) {
    return(Sem_CartProduct(pn->pn1, pn->pn2, N("X-MAX"), N("E-MAX"), cf, dc,
                           N("aobj-predicative"), NULL, NULL));
  /************************************************************************/
  } else if (pn->pn1->feature == F_NP && pn->pn2->feature == F_VP) {
#ifdef notdef
    /* This doesn't work very well. "Puchl is a grocer" */
    if (Syn_ParseIsNPVerbInversionVP(pn->pn2, DC(dc).lang)) {
    /* "What did she do?" "Who did you call?" */
      r1 = Sem_CartProduct(pn->pn1, pn->pn2, N("X-MAX"), N("W-MAX"), cf, dc,
                           N("obj"), NULL, NULL);
    } else {
      r1 = NULL;
    }
#endif
    r = Sem_CartProduct(pn->pn1, pn->pn2, N("X-MAX"), N("W-MAX"), cf, dc,
                        N("subj"), NULL, NULL);
    return(r);
#ifdef notdef
    return(ObjListAppendDestructive(r, r1));
#endif
  /************************************************************************/
  } else if (pn->pn1->feature == F_PRONOUN && pn->pn2->feature == F_NP) {
    if (!(vp = GenVP(ObjToLexEntryGet(N("standard-copula"), F_VERB, F_NULL,
                                      dc), "",
                     N("present-indicative"), pn->pn1, NULL, dc))) {
      return(NULL);
    }
    /* todo: Freeing. */
    vp = PNodeConstit(F_VP, vp, pn->pn2);
    return(Sem_ParseParse1(PNodeConstit(F_S, PNodeConstit(F_NP, pn->pn1, NULL),
                                        vp), cf, dc));
  /************************************************************************/
  } else if (pn->pn1->feature == F_ADJP && pn->pn2->feature == F_S) {
    return(Sem_ParseParse3(pn, pn->pn2, pn->pn1, cf, dc,
                           N("aobj-predicative")));
  /************************************************************************/
  } else if (Sem_ParseS1(pn->pn1, pn->pn2, cf, dc, &r)) {
    return(r);
  /************************************************************************/
  } else if (Sem_ParseS1(pn->pn2, pn->pn1, cf, dc, &r)) {
    return(r);
  /************************************************************************/
  } else if (pn->pn1->feature == F_S && pn->pn2->feature == F_S) {
    /* Sem_ParseS1 above failed to accept this case for conjunctions, so it must
     * be a run-on sentence (with possible empty/elided subject in pn->pn2).
     */
    return(Sem_ParseParse3(pn, pn->pn1, pn->pn2, cf, dc, N("subj")));
  /************************************************************************/
  } else {
    PNodePrint(Log, DiscourseGetInputChannel(dc), pn);
    Dbg(DBGSEMPAR, DBGBAD, "Sem_ParseS: unknown case");
    return(Sem_CartProduct(pn->pn2, pn->pn1, NULL, NULL, cf, dc, N("kobj1"),
                           NULL, NULL));
  /************************************************************************/
  }
}

ObjList *Sem_ParseName(PNode *pn, Name *nm, Discourse *dc)
{
  return(ObjListCreate(ObjIntension1(N("human"),
                                    L(N("NAME-of"),
                                      N("human"),
                                      HumanNameToObj(nm),
                                      E)),
                       NULL));
}

ObjList *Sem_ParseTelno(PNode *pn, Obj *telno, Discourse *dc)
{
  return(ObjListCreateSP(telno, SCORE_MAX, NULL, pn, NULL, NULL));
    /* todoSCORE */
}

ObjList *Sem_ParseMediaObject(PNode *pn, ObjList *media_obj, Discourse *dc)
{
  return(media_obj);
}

ObjList *Sem_ParseProduct(PNode *pn, ObjList *product, Discourse *dc)
{
  Obj	*obj;
  if (product == NULL) return(NULL);
  if (ObjListIsLengthOne(product)) {
    return(ObjListCreateSP(product->obj, SCORE_MAX, NULL, pn, NULL, NULL));
      /* todoSCORE */
  }
  if ((obj = ObjListCommonAncestor(product))) {
    return(ObjListCreateSP(obj, SCORE_MAX, NULL, pn, NULL, NULL));
      /* todoSCORE */
  }
  return(NULL);
}

ObjList *Sem_ParseNumber(PNode *pn, Obj *number, Discourse *dc)
{
  return(ObjListCreateSP(number, SCORE_MAX, NULL, pn, NULL, NULL));
  	/* todoSCORE */
}

TsRange *ObjToTsRange_ThroughTense(Obj *obj)
{
  if (ISA(N("tense"), I(obj, 0))) {
    return(ObjToTsRange_ThroughTense(I(obj, 1)));
  } else {
    return(ObjToTsRange(obj));
  }
}

void ObjSetTsRange_ThroughTense(Obj *obj, TsRange *tsr)
{
  if (ISA(N("tense"), I(obj, 0))) {
    ObjSetTsRange_ThroughTense(I(obj, 1), tsr);
  } else {
    ObjSetTsRange(obj, tsr);
  }
}

void ObjSetTsRangeSpec_ThroughTense(Obj *obj, TsRange *tsr)
{
  if (TsRangeIsSpecification(tsr, ObjToTsRange_ThroughTense(obj))) {
    ObjSetTsRange_ThroughTense(obj, tsr);
  }
}

/* todo: Eliminate code duplication with Sem_ParseAdvWord (doesn't exist?)?
 * Deal with Tod's and Dur's making it here.
 */
ObjList *Sem_ParseTsRange(PNode *pn, TsRange *tsr, CaseFrame *cf, Discourse *dc)
{
  Obj			*bobj;
  ObjList		*props, *p;
  SP			bobj_sp;
  if (!(bobj = CaseFrameGet(cf, N("bobj"), &bobj_sp))) {
    Dbg(DBGSEMPAR, DBGBAD, "empty bobj");
    return(NULL);
  }
  props = ObjIntensionParse(bobj, 0, &bobj);
  /* todo: What about multiple interpretations? Do we need to copy obj? */
  if (!TsRangeValid(tsr)) return(NULL);
  if (props) {
    for (p = props; p; p = p->next) {
      ObjSetTsRangeSpec_ThroughTense(p->obj, tsr);
    }
    bobj = ObjIntension(bobj, props);
  } else {
    ObjSetTsRangeSpec_ThroughTense(bobj, tsr);
  }
  return(ObjListCreateSP(bobj, bobj_sp.score, NULL, pn, bobj_sp.anaphors,
                         NULL));
}

/* BLOCK PARSING */

ObjList *Sem_ParseCommunicon(PNode *pn, ObjList *communicons, Discourse *dc)
{
  return(communicons);
}

ObjList *Sem_ParseEmailHeader(PNode *pn, EmailHeader *emh, Discourse *dc)
{
  ObjList		*speakers, *listeners;
  Obj			*class, *action;
  Ts			now, *now_ts;
  TsRange		*tsr;
  Dbg(DBGGEN, DBGDETAIL, "** Sem_ParseEmailHeader");
  if (emh->header_type == N("post-email-header")) {
  /* REDUNDANT: These contain redundant information merged into other
   * "pre" headers.
   */
    return(NULL);
  }
  DiscourseSetCurrentChannel(dc, DCIN);
  DiscourseDeicticStackClear(dc);
    /* todo: Should also clear at end of message. */
  /* todo: Handle entire nested quoted messages. */
  if (emh->newsgroup) class = emh->newsgroup;
  else class = N("email");
  speakers = ObjListCreateSP(emh->from_obj, SCORE_MAX, NULL, pn, NULL, NULL);
    /* todoSCORE */
  listeners = ObjListAppendDestructive(emh->to_objs, emh->cc_objs);

  if ((now_ts = EmailHeaderTs(emh))) now = *now_ts;
  else TsSetNow(&now);

  DiscourseDeicticStackPush(dc, class, speakers, listeners, &now, 0);

  action = L(N("send-email"),
             ObjListHead(speakers, N("human")),
             ObjListHead(listeners, N("human")),
             emh->message_id,
             E);
  tsr = ObjToTsRange(action);
  tsr->startts = now;
  tsr->stopts = now;
  TsIncrement(&tsr->stopts, DurationOf(I(action, 0)));
  return(ObjListCreateSP(action, SCORE_MAX, NULL, pn, NULL, NULL));
    /* todoSCORE */
}

ObjList *Sem_ParseAttribution(PNode *pn, Attribution *attribution,
                              Discourse *dc)
{
  ObjList	*speakers, *listeners;
  Obj		*class;
  Dbg(DBGGEN, DBGDETAIL, "** Sem_ParseAttribution <%s>",
      M(attribution->speaker));
  class = DCCLASS(dc);
  speakers = ObjListCreateSP(attribution->speaker, SCORE_MAX, NULL, pn, NULL,
                             NULL);
    /* todoSCORE */
  listeners = DCSPEAKERS(dc);
  DiscourseDeicticStackPush(dc, class, speakers, listeners, &attribution->ts,
                            0);
  DiscourseDeicticStackPop(dc);
  return(NULL);
}

/* OTHER PARSING */

ObjList *Sem_ParseOther(PNode *pn, LexEntry *le)
{
  LexEntryToObj	*leo;
  ObjList		*r;
  r = NULL;
  for (leo = LexEntryGetLeo(le); leo; leo = leo->next) {
    if (leo->obj) {
      r = ObjListCreateSP(leo->obj, SCORE_MAX, leo, pn, NULL, r);
    }
  }
  return(r);
}

/* PHRASE PARSING */

/* Assumes no leading or trailing spaces in s.
 * todo: This needs to be redone for new scheme?
 * todo: Call TA_ParseName with revok=1.
 */
ObjList *Sem_ParsePhraseText(char *s, Obj *class, HashTable *ht, ObjList *r,
                             Discourse *dc)
{
  int			success, freeme;
  char			phrase[PHRASELEN];
  Obj			*obj;
  IndexEntry	*ie, *orig_ie;
  LexEntryToObj	*leo;
  success = 0;
  if (StringIsNumber(s)) {
    r = ObjListCreate(StringToObj(s, NULL, 0), r);
  }
  LexEntryDbFileToPhrase(s, phrase);
  for (ie = orig_ie = LexEntryFindPhrase(ht, phrase, INTPOSINF, 1, 0, &freeme);
      ie; ie = ie->next) {
    for (leo = ie->lexentry->leo; leo; leo = leo->next) {
      if (ObjListIn(leo->obj, r)) continue;
      if (class && (!ISAP(class, leo->obj))) continue;
      r = ObjListCreate(leo->obj, r);
      success = 1;
    }
  }
  if (freeme) IndexEntryFree(orig_ie);
  if ((obj = NameToObj(s, OBJ_NO_CREATE))) {
    if (ISAP(N("string"), obj)) {
      r = ObjListCreate(obj, r);
      success = 1;
    }
  }
  if ((!success) && class && ISAP(N("string"), class)) {
    /* No learning needed here because string tokens are generated specially,
     * and they will later be created from their relations.
     */
    if (StringIsDigitOr(s, PHONE_WHITESPACE)) {
      /* todo: Note we trash <s> in this case. */
      StringElims(s, PHONE_WHITESPACE, NULL);
    }
    r = ObjListCreate(StringToObj(s, class, 0), r);
    success = 1;
  }
  return(r);
}

/* End of file. */
