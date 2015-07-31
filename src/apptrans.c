/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Transfer-based translation.
 *
 * 19950309: begun
 * 19950310: more work
 * 19950311: more work
 * 19950312: more work
 * 19950313: more work
 * 19950314: more work
 *
 * Notes:
 * - Alternative translations may occur at any level in the PNode tree.
 *   They should be stored at the lowest possible level, to give the most
 *   economical representation of alternatives. (Though they are
 *   flattened for printout anyway...)
 * - DC(dc).lang is the target language inside all translation functions.
 *
 * todo:
 * - This code has not been kept up to date with the base component.
 *   For example, it needs to be updated to handle nominalized verbs
 *   (X-MAXes).
 * - Lexical gaps should trigger transpositions, as described in
 *   Chuquet and Paillard (1989).
 *   Example: diplomé.
 * - Add translation rule: French X de Y <-> English Y X. Examples:
 *     plan de sauvetage <-> rescue plan
 *     montage financier de l'Etat <-> State financial arrangement
 * - Write grammar checker (also transfer-based translation posteditor):
 *     Maintain noun-adjective agreement.
 *     Enforce usage features:
 *       Use articles as specified in database. Example mistake:
 *         "Christine Ockrent was educated at the Cambridge University."
 *         should be "Christine Ockrent was educated at Cambridge University."
 *       Use singular/plural as specified in database. Example: jean in
 *       French becomes jeans in English.
 *     Correct verb subcategorization restrictions (including prepositions,
 *       tense of subordinate S-bar).
 */

#include "tt.h"
#include "apptrans.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "lextheta.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "semdisc.h"
#include "semgen1.h"
#include "semgen2.h"
#include "semtense.h"
#include "synfilt.h"
#include "synparse.h"
#include "synpnode.h"
#include "syntrans.h"
#include "synxbar.h"
#include "utildbg.h"

Bool TranslationOn;

ObjList *TranslateConcepts;	/* todo: This shouldn't be global. */

/******************************************************************************
 HELPING FUNCTIONS
 ******************************************************************************/

/* Arguments */

void ArgumentsClear(/* RESULTS */ Arguments *args)
{
  args->subjnp_theta_role_i = args->objnp1_theta_role_i = -1;
  args->objnp2_theta_role_i = args->iobjnp1_theta_role_i = -1;
  args->iobjnp2_theta_role_i = args->iobjnp3_theta_role_i = -1;

  args->subjnp_src = args->objnp1_src = args->objnp2_src =
    args->iobjpp1_src = NULL;
  args->iobjpp2_src = args->iobjpp3_src = args->adjp_src =
    args->adv1 = args->adv2 = NULL;
  args->adv3 = args->preadv1 = args->postadv1 = NULL;

  args->subjnp_theta_marked = args->objnp1_theta_marked =
    args->objnp2_theta_marked = 0;
  args->iobjpp1_theta_marked = args->iobjpp2_theta_marked =
    args->iobjpp3_theta_marked = 0;
}

void ArgumentsToThetaFilled(Arguments *args, /* RESULTS */ int *theta_filled)
{
  int	i;
  for (i = 0; i < MAXTHETAFILLED; i++) {
    theta_filled[i] = 0;
  }
  if (args->subjnp_theta_role_i >= 0) {
    theta_filled[args->subjnp_theta_role_i] = 1;
  }
  if (args->objnp1_theta_role_i >= 0) {
    theta_filled[args->objnp1_theta_role_i] = 1;
  }
  if (args->objnp2_theta_role_i >= 0) {
    theta_filled[args->objnp2_theta_role_i] = 1;
  }
  if (args->iobjnp1_theta_role_i >= 0) {
    theta_filled[args->iobjnp1_theta_role_i] = 1;
  }
  if (args->iobjnp2_theta_role_i >= 0) {
    theta_filled[args->iobjnp2_theta_role_i] = 1;
  }
  if (args->iobjnp3_theta_role_i >= 0) {
    theta_filled[args->iobjnp3_theta_role_i] = 1;
  }
}

Bool ArgumentsAddSUBJ(PNode *subj, /* RESULTS */ Arguments *args)
{
  if (args->subjnp_src) {
    Dbg(DBGGENER, DBGBAD, "ArgumentsAddSUBJ: too many objs");
    return(0);
  }
  args->subjnp_src = subj;
  return(1);
}

Bool ArgumentsAddOBJ(PNode *obj, /* RESULTS */ Arguments *args)
{
  if (args->objnp2_src) {
    Dbg(DBGGENER, DBGBAD, "ArgumentsAddOBJ: too many objs");
    return(0);
  }
  if (args->objnp1_src) args->objnp2_src = obj;
  else args->objnp1_src = obj;
  return(1);
}

Bool ArgumentsAddIOBJ(PNode *iobj, /* RESULTS */ Arguments *args)
{
  if (args->iobjpp3_src) {
    Dbg(DBGGENER, DBGBAD, "ArgumentsAddIOBJ: too many iobjs");
    return(0);
  }
  if (args->iobjpp2_src) args->iobjpp3_src = iobj;
  else if (args->iobjpp1_src) args->iobjpp2_src = iobj;
  else args->iobjpp1_src = iobj;
  return(1);
}

Bool ArgumentsAddADJP(PNode *adjp, /* RESULTS */ Arguments *args)
{
  if (args->adjp_src) {
    Dbg(DBGGENER, DBGBAD, "ArgumentsAddADJP: too many adjps");
    return(0);
  }
  args->adjp_src = adjp;
  return(1);
}

Bool ArgumentsAddPreAdv(PNode *adv, /* RESULTS */ Arguments *args)
{
  if (args->preadv1) {
    Dbg(DBGGENER, DBGBAD, "ArgumentsAddPreAdv: too many preadvs");
    return(0);
  }
  args->preadv1 = adv;
  return(1);
}

Bool ArgumentsAddPostAdv(PNode *adv, /* RESULTS */ Arguments *args)
{
  if (args->postadv1) {
    Dbg(DBGGENER, DBGBAD, "ArgumentsAddPostAdv: too many postadvs");
    return(0);
  }
  args->postadv1 = adv;
  return(1);
}

/* See Chomsky (1982/1987, p. 26). */
Bool ArgumentsSatisfiesThetaCriterion(Arguments *args)
{
  return((args->subjnp_src == NULL || args->subjnp_theta_marked == 1) &&
         (args->objnp1_src == NULL || args->objnp1_theta_marked == 1) &&
         (args->objnp2_src == NULL || args->objnp2_theta_marked == 1) &&
         (args->iobjpp1_src == NULL || args->iobjpp1_theta_marked == 1) &&
         (args->iobjpp2_src == NULL || args->iobjpp2_theta_marked == 1) &&
         (args->iobjpp3_src == NULL || args->iobjpp3_theta_marked == 1));
}

/* Get canonical (language-independent, interlingual) index of theta-role
 * for each component.
 */
void ArgumentsFillCasei(ObjToLexEntry *ole_src, /* RESULTS */ Arguments *args)
{
  args->subjnp_theta_role_i =
    ThetaRoleGetCaseIndex(ole_src->theta_roles, N("subj"), NULL,
                                                 NULL);
  args->objnp1_theta_role_i =
    ThetaRoleGetCaseIndex(ole_src->theta_roles, N("obj"), NULL, NULL);
  /* todo: Handle multiple objs. */
  args->iobjnp1_theta_role_i = args->iobjnp2_theta_role_i =
    args->iobjnp3_theta_role_i = -1;
  if (args->iobjpp1_src) {
    args->iobjnp1_theta_role_i =
    ThetaRoleGetIOBJIndex(ole_src->theta_roles,
                          args->iobjpp1_src->pn1->lexitem->le, NULL);
  }   
  if (args->iobjpp2_src) {
    args->iobjnp2_theta_role_i =
    ThetaRoleGetIOBJIndex(ole_src->theta_roles,
                          args->iobjpp2_src->pn1->lexitem->le, NULL);    
  }   
  if (args->iobjpp3_src) {
    args->iobjnp3_theta_role_i =
    ThetaRoleGetIOBJIndex(ole_src->theta_roles,
                          args->iobjpp3_src->pn1->lexitem->le, NULL);
  }
}

/* Find the ObjToLexEntry link between obj and le. */
ObjToLexEntry *ObjToLexEntryFindLink(Obj *obj, LexEntry *le)
{
  ObjToLexEntry	*ole;
  for (ole = obj->ole; ole; ole = ole->next) {
    if (ole->le == le) return(ole);
  }
  Dbg(DBGGENER, DBGBAD, "no link <%s>----<%s>.<%s>", M(obj), le->srcphrase,
      le->features);
  return(NULL);
}

ObjToLexEntry *ObjToLexEntryTransGet2(Obj *obj, Obj *value, char *feat,
                                      int paruniv, int *theta_filled,
                                      Discourse *dc)
{
  int		save_style;
  ObjToLexEntry	*ole;
  ole = ObjToLexEntryGet1(obj, value, feat, F_NULL, paruniv, theta_filled, dc);
  if (ole == NULL && DC(dc).style != F_NULL) {
    save_style = DC(dc).style;
    DC(dc).style = F_NULL;
    ole = ObjToLexEntryGet1(obj, value, feat, F_NULL, paruniv, theta_filled,
                            dc);
    DC(dc).style = save_style;
  }
  return(ole);
}

ObjToLexEntry *ObjToLexEntryTransGet1(Obj *obj, Obj *value, char *feat,
                                      int paruniv, int depth,
                                      int *theta_filled, Discourse *dc)
{
  int	i, nump;
  Obj	*parent;
  ObjToLexEntry	*ole;
  if (depth > 4) return(NULL);
  if ((ole = ObjToLexEntryTransGet2(obj, value, feat, paruniv,
                                   theta_filled, dc))) {
    return(ole);
  }
  /* todo: Look for transpositions at this level first. */
  for (i = 0, nump = ObjNumParents(obj); i < nump; i++) {
    parent = ObjIthParent(obj, i);
    if (ObjBarrierISA(parent)) {
      Dbg(DBGGENER, DBGBAD, "ISA barrier <%s>", M(parent));
      return(NULL);
    }
    if ((ole = ObjToLexEntryTransGet1(parent, value, feat, paruniv,
                                      depth+1, theta_filled,
                                     dc))) {
      return(ole);
    }
  }
  return(NULL);
}

/* Find ObjToLexEntry (lexical entry) with similar arguments and usage
 * to ole_src.
 *
 * Example:
 * obj: N("like-human")
 * ole_src->theta_roles: 1:subj, 2:obj
 * ole_src->features: TÔ
 * tgtlang: F_FRENCH
 * Returns:
 * ole->theta_roles: 1:iobj:à 2:subj
 * ole->le: plaire.Vy
 *
 * Multiple possibilities are generated only in the case of multiple
 * interpretations. todo: Possibly provide more synonyms as an option.
 */
ObjToLexEntry *ObjToLexEntryTransGet(Obj *obj, int pos, int *theta_filled,
                                     int tgtlang, ObjToLexEntry *ole_src,
                                     Discourse *dc)
{
  char			feat[2];
  int			save_lang, save_style;
  GenAdvice		save_ga;
  ObjToLexEntry		*ole;

  /* Set up dc environment. */
  save_lang = DC(dc).lang;
  save_style = DC(dc).style;
  save_ga = dc->ga;
  DC(dc).lang = tgtlang;
  DC(dc).style = FeatureGet(ole_src->features, FT_STYLE);
  if (DC(dc).style == F_SLANG) DC(dc).style = F_INFORMAL;
  dc->ga.consistent = 1;

  /* Prepare features */
  feat[0] = pos;
  feat[1] = TERM;

  ole = ObjToLexEntryTransGet1(obj, NULL, feat,
                               FeatureGet(ole_src->features, FT_PARUNIV),
                               0, theta_filled, dc);

  if (ole == NULL) {
    Dbg(DBGGENER, DBGBAD, "lexical gap <%s> <%c%c>", M(obj), pos, tgtlang);
  }

  /* Restore previous dc environment. */
  DC(dc).lang = save_lang;
  DC(dc).style = save_style;
  dc->ga = save_ga;

  return(ole);
}

void FeatureDefault(int lang, int pos, /* RESULTS */ int *tense, int *gender,
                    int *number, int *person, int *degree)
{
  if (lang == F_ENGLISH) {
    if (*number == F_NULL && pos == F_NOUN) *number = F_SINGULAR;
  } else {
    if (*number == F_NULL && (pos == F_NOUN || pos == F_ADJECTIVE)) {
      *number = F_SINGULAR;
    }
    if (*gender == F_NULL) *gender = F_MASCULINE;
  }
  if (pos == F_VERB) {
    if (*tense == F_NULL) *tense = F_INDICATIVE;
    if (*person == F_NULL) *person = F_THIRD_PERSON;
  }
  if (pos == F_ADJECTIVE) {
    if (*degree == F_NULL) *degree = F_POSITIVE;
  }
}

void TranslateGetFeatures(PNode *pn, int tgtlang, int pos, /* RESUTS */
                          int *tense, int *gender, int *number, int *person,
                          int *degree)
{
  *tense = FeatureGet(pn->lexitem->features, FT_TENSE);
  *gender = FeatureGet(pn->lexitem->features, FT_GENDER);
  *number = FeatureGet(pn->lexitem->features, FT_NUMBER);
  *person = FeatureGet(pn->lexitem->features, FT_PERSON);
  *degree = FeatureGet(pn->lexitem->features, FT_DEGREE);
  FeatureDefault(tgtlang, pos, tense, gender, number, person, degree);
}

/* Assumes srclang != tgtlang. */
Obj *TranslateCompoundTense(Obj *comptense, int srclang, int tgtlang)
{
  Obj	*rel, *r;
  if (comptense == NULL) return(NULL);
  if (tgtlang == F_FRENCH) rel = N("fr-translation-of");
  else rel = N("eng-translation-of");
  if ((r = R1EI(2, &TsNA, L(rel, comptense, ObjWild, E)))) {
    return(r);
  }
  return(comptense);
}

ObjList *PNodeConceptsFor1(PNode *pn, Obj *con, ObjList *r)
{
  int		i, len;
  if (!ObjIsList(con)) return(r);
  for (i = 0, len = ObjLen(con); i < len; i++) {
    if (PNI(con, i) == pn) {
      r = ObjListCreate(I(con, i), r);
    } else {
      r = PNodeConceptsFor1(pn, I(con, i), r);
    }
  }
  return(r);
}

ObjList *PNodeConceptsFor(PNode *pn, ObjList *concepts)
{
  ObjList	*r, *p;
  r = NULL;
  for (p = concepts; p; p = p->next) {
    if (p->u.sp.pn == pn) r = ObjListCreate(p->obj, r);
    r = PNodeConceptsFor1(pn, p->obj, r);
  }
  return(r);
}

ObjList *TranslateGetAllcons(PNode *pn, LexEntry *le)
{
  ObjList	*cons;
  if ((cons = PNodeConceptsFor(pn, TranslateConcepts))) {
  /* todo: It would be nice to get rid of pn->allcons, but this better
   * way of mapping from pn to possible concepts according to Sem_Parse
   * doesn't work for verbs yet, since PNI(obj, 0) isn't set by
   * Sem_ParseThetaMarking_Pred.
   */
    return(cons);
  } else if (pn->allcons) {
    return(pn->allcons);
  } else if (le) {
    return(LexEntryGetLeoObjs(le));
  } else {
    return(NULL);
  }
}

PNode *TranslateObjList(ObjList *objs, Discourse *dc)
{
  ObjList	*p;
  PNode		*r, *r1;
  r = NULL;
  for (p = objs; p; p = p->next) {
    r1 = Generate(F_NP, p->obj, N("present-indicative"), dc);
    r1->next_altern = r;
    r = r1;
  }
  return(r);
}

Bool TranslateGetOles(int pos, Obj *con, LexEntry *src_le, int *theta_filled,
                      int tgtlang, Discourse *dc, /* RESULTS */
                      ObjToLexEntry **ole_src, ObjToLexEntry **ole_tgt)
{
  int	theta_filled1[MAXTHETAFILLED];
  /* Get source language argument information. */
  if (!(*ole_src = ObjToLexEntryFindLink(con, src_le))) {
    return(0);
  }

  if (theta_filled == NULL) {
    /* If we don't know what theta roles are filled in the source language
     * parse, try to match the theta roles filled in the source language
     * lexical entry.
     */
    theta_filled = theta_filled1;
    ThetaRoleToThetaFilled((*ole_src)->theta_roles, theta_filled);
  }

  /* Select target language lexical entry and argument information. */
  if (!(*ole_tgt = ObjToLexEntryTransGet(con, pos, theta_filled, tgtlang,
                                         *ole_src, dc))) {
    return(0);
  }
  return(1);
}

/******************************************************************************
 TRANSLATION TOP-LEVEL AND DISPATCH FUNCTIONS
 ******************************************************************************/

FILE *TranslateStream;

void TranslateInit()
{
  TranslateStream = StreamOpen("outtrans.txt", "w+");
  TranslationOn = 0;
}

void TranslateBegin(Channel *ch)
{
  ch->translations = NULL;	/* todo: Freeing. */
}

void TranslateSpitUntranslated(Channel *ch, size_t lowerb, size_t upperb)
{
  fputc(UNTRANS, TranslateStream);
  PNodePrintSubstring(TranslateStream, ch, lowerb, upperb,
                      0, INTPOSINF);
  fputc(UNTRANS, TranslateStream);
}

void Translate(PNode *src_pn, ObjList *concepts, Channel *ch, int srclang,
               int tgtlang, Discourse *dc)
{
  int	save_lang;
  Obj	*save_task;
  PNode	*tgt_pn;
  TranslateConcepts = concepts;
  save_lang = DC(dc).lang;
  DC(dc).lang = tgtlang;
  save_task = dc->task;
  dc->task = N("translate");
  Dbg(DBGSEMPAR, DBGDETAIL, "**** TRANSLATION BEGIN ****");
  PNodePrettyPrint(Log, DiscourseGetInputChannel(dc), src_pn);
  tgt_pn = Translate1(src_pn, NULL, N("Z-MAX"), NULL, srclang, tgtlang, dc);
  PNodePrettyPrint(Log, DiscourseGetInputChannel(dc), tgt_pn);
  Dbg(DBGSEMPAR, DBGDETAIL, "**** TRANSLATION END ****");
  DC(dc).lang = save_lang;
  ch->translations = PNodeAppendDestructive(ch->translations, tgt_pn);
  dc->task = save_task;
}

void TranslationEnd(Channel *ch, int eoschar, int tgtlang, Discourse *dc)
{
  int			save_lang;
  StringArray	*sa;
  PNode			*p, *pn;
  save_lang = DC(dc).lang;
  DC(dc).lang = tgtlang;
  ch->translations = PNodeFlattenAltern(ch->translations);
  sa = StringArrayCreate();
  for (p = ch->translations; p; p = p->next_altern) {
    pn = TransformGen(p, dc);
    PNodePrintStringArrayAdd(sa, pn, eoschar, dc);
  }
  fputc(NEWLINE, TranslateStream);
  StreamSep(TranslateStream);
  StringArrayPrint(Log, sa, 0, 0);
  StringArrayPrint(TranslateStream, sa, 0, 0);
  fflush(TranslateStream);
  StringArrayFreeCopy(sa);
  ch->translations = NULL;	/* todo: Freeing. */
  DC(dc).lang = save_lang;
}

PNode *Translate1(PNode *pn, PNode *pnp, Obj *max, PNode *agree_np,
                  int srclang, int tgtlang, Discourse *dc)
{
  PNode	*r;
  if (pn == NULL) return(NULL);
  if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
    IndentUp();
    IndentPrint(Log);
    PNodePrint(Log, DiscourseGetInputChannel(dc), pn);
  }
  switch (pn->type) {
    case PNTYPE_LEXITEM:
      r = TranslateWord(pn, pnp, max, agree_np, srclang, tgtlang, dc);
      break;
    case PNTYPE_CONSTITUENT:
      r = TranslateConstituent(pn, pnp, max, agree_np, srclang, tgtlang, dc);
      break;
    case PNTYPE_POLITY:
      r = TranslateObjList(pn->u.polities, dc);
      break;
    case PNTYPE_MEDIA_OBJ:
      r = TranslateObjList(pn->u.media_obj, dc);
      break;
    case PNTYPE_PRODUCT:
      r = TranslateObjList(pn->u.product, dc);
      break;
    case PNTYPE_NUMBER:
      r = pn;	/* todo */
      break;
    /* todo: Add other types. */
    default:
      r = pn;
  }
  if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
    IndentPrint(Log);
    PNodePrint(Log, DiscourseGetInputChannel(dc), r);
    IndentDown();
  }
  return(r);
}

PNode *TranslateWord(PNode *pn, PNode *pnp, Obj *max, PNode *agree_np,
                     int srclang, int tgtlang, Discourse *dc)
{
  switch (pn->feature) {
    case F_ADJECTIVE:
    case F_DETERMINER:
    case F_PREPOSITION:
      return(TranslateWordAgreeNP(pn, pnp, max, pn->feature, agree_np,
                                  srclang, tgtlang, dc));
    case F_NOUN:
    case F_PRONOUN: /* todo: More complex handling? */
    case F_VERB:
      return(TranslateWordInflected(pn, pnp, max, pn->feature, srclang,
                                    tgtlang, dc));
    case F_EXPLETIVE:
      return(pn);
    default:
      break;
  }
  return(TranslateWordUninflected(pn, pnp, max, pn->feature, srclang,
                                  tgtlang, dc));
}

PNode *TranslateConstituent(PNode *pn, PNode *pnp, Obj *max, PNode *agree_np,
                            int srclang, int tgtlang, Discourse *dc)
{
  PNode	*pn1;
  if (pn->pn2 == NULL) {
    pn1 = Translate1(pn->pn1, pn, max, agree_np, srclang, tgtlang, dc);
    return(PNodeConstit(pn->feature, pn1, NULL));
  }
  if (XBarIsRank2Projection(pn, pnp)) {
    max = XBarConstitToMax(pn->feature);
  }
  switch (pn->feature) {
    case F_S:
      return(TranslateS(pn, pnp, max, srclang, tgtlang, dc));
    case F_NP:
      return(TranslateNP(pn, pnp, max, srclang, tgtlang, dc));
    case F_VP:
      return(TranslateVP(pn, pnp, max, srclang, tgtlang, dc));
    case F_PP:
      return(TranslatePP(pn, pnp, max, srclang, tgtlang, dc));
    case F_ADJP:
      return(TranslateADJP(pn, pnp, max, agree_np, srclang, tgtlang, dc));
    default:
      break;
  }
  return(TranslateJuxtaposition(pn, pnp, max, agree_np, srclang, tgtlang, dc));
}

/******************************************************************************
 INDIVIDUAL WORD TRANSLATION
 ******************************************************************************/

void TranslatePunc(char *tgt, char *src, int srclang, int tgtlang)
{
  StringCpy(tgt, src, PUNCLEN);
  if (srclang == F_ENGLISH && tgtlang == F_FRENCH) {
    StringMap(tgt, PUNCT_ENGLISH, PUNCT_FRENCH);
  } else if (srclang == F_FRENCH && tgtlang == F_ENGLISH) {
    StringMap(tgt, PUNCT_FRENCH, PUNCT_ENGLISH);
  }
}

PNode *TranslateAWord(PNode *pn, PNode *pnp, Obj *max, int pos, int tense,
                      int gender, int number, int person, int degree,
                      int srclang, int tgtlang, Discourse *dc)
{
  int			number1;
  ObjList		*p;
  ObjToLexEntry	*ole_src, *ole_tgt;
  Word			*infl;
  PNode			*r, *r1;
  r = NULL;
  for (p = TranslateGetAllcons(pn, pn->lexitem ? pn->lexitem->le : NULL);
       p; p = p->next) {
    if (ObjIsList(p->obj)) {
      continue;
    }
/* todo: Put FT_FILTER checks here. */
    if (!TranslateGetOles(pos, p->obj, pn->lexitem->le, NULL, tgtlang, dc,
                          &ole_src, &ole_tgt)) {
      continue;
    }
    if (StringIn(F_COMMON_INFL, ole_tgt->features)) {
      number1 = FeatureGet(ole_tgt->features, FT_NUMBER);
    } else {
      number1 = number;
    }
    if (!(infl = LexEntryGetInflection(ole_tgt->le, tense, gender, number1,
                                       person, F_NULL, degree, 1, dc))) {
      continue;
    }
    r1 = PNodeWord(pos, infl->word, infl->features, ole_tgt->le, p->obj);
    if (pn->punc[0]) TranslatePunc(r1->punc, pn->punc, srclang, tgtlang);
    r1->ole = ole_tgt;
    r1->obj = p->obj;
    r1->next_altern = r;
    r = r1;
  }
  if (r == NULL) {
    return(pn);
  }
  return(r);
}

/* Translate a word and attempt to match the source language inflections. */
PNode *TranslateWordInflected(PNode *pn, PNode *pnp, Obj *max, int pos,
                              int srclang, int tgtlang, Discourse *dc)
{
  int	tense, gender, number, person, degree;
  if (pos == F_VERB) Dbg(DBGSEMPAR, DBGBAD, "TranslateWordInflected verb");
  TranslateGetFeatures(pn, tgtlang, pos, &tense, &gender, &number, &person,
                       &degree);
  return(TranslateAWord(pn, pnp, max, pos, tense, gender, number, person,
                        degree, srclang, tgtlang, dc));
}

/* Translate conjunctions, adverbs. */
PNode *TranslateWordUninflected(PNode *pn, PNode *pnp, Obj *max, int pos,
                                int srclang, int tgtlang, Discourse *dc)
{
  return(TranslateAWord(pn, pnp, max, pos, F_NULL, F_NULL, F_NULL, F_NULL,
                        F_NULL, srclang, tgtlang, dc));
}

void TranslateGenderNumberPersonDef(PNode *pn, int tgtlang, /* RESULTS */
                                    int *gender, int *number, int *person)
{
  if (pn) {
    PNodeGetHeadNounFeatures(pn, 1, gender, number, person);
  } else {
    if (tgtlang == F_FRENCH) *gender = F_MASCULINE;
    else *gender = F_NULL;
    *number = F_SINGULAR;
    *person = F_NULL;
  }
}

/* Translate adjectives, determiners, and prepositions which (might) need to
 * agree with an np.
 */
PNode *TranslateWordAgreeNP(PNode *pn, PNode *pnp, Obj *max, int pos,
                            PNode *agree_np, int srclang, int tgtlang,
                            Discourse *dc)
{
  int	gender, number, person;
  char	*owner_feat;
  if (pos == F_DETERMINER &&
      LexEntryConceptIsAncestor(N("possessive-determiner"),
                                PNodeLeftmostLexEntry(pn))) {
    owner_feat = pn->lexitem->features;
    if (tgtlang == F_FRENCH) {
      TranslateGenderNumberPersonDef(agree_np, tgtlang, &gender, &number,
                                     &person);
      return(GenMakeFrenchPossDet1(FeatureGet(owner_feat, FT_NUMBER),
                                   FeatureGet(owner_feat, FT_PERSON),
                                   gender,
                                   number,
                                   dc));
    } else {
      return(GenMakeEnglishPossDet(owner_feat, dc));
    }
  } else {
    TranslateGenderNumberPersonDef(agree_np, tgtlang, &gender, &number,
                                   &person);
    /* todo: stupider/stupidest => plus stupide */
    return(TranslateAWord(pn, pnp, max, pos, F_NULL, gender, number, person,
                          FeatureGet(pn->lexitem->features, FT_DEGREE),
                          srclang, tgtlang, dc));
  }
}

/******************************************************************************
 CONSTITUENT TRANSLATION
 ******************************************************************************/

/* "silly Trix"
 * pnp is parent of pn is parent of np/adjp
 */
PNode *TranslateAdjDetp_NP(PNode *pn, PNode *pnp, Obj *max, int pos,
                           PNode *adjp, PNode *np, char *punc1, char *punc2,
                           int srclang, int tgtlang, Discourse *dc)
{
  PNode	*np_tgt, *adjp_tgt, *p, *q, *r, *r1;
  np_tgt = Translate1(np, pn, max, NULL, srclang, tgtlang, dc);
  r = NULL;
  for (p = np_tgt; p; p = p->next_altern) {
    adjp_tgt = Translate1(adjp, pn, max, p, srclang, tgtlang, dc);
    for (q = adjp_tgt; q; q = q->next_altern) {
      r1 = GenMakeModifiedNoun(pos, q, p, punc1, punc2, dc);
      r1->next_altern = r;
      r = r1;
    }
    PNodeBreakup(adjp_tgt);
  }
  PNodeBreakup(np_tgt);
  if (r == NULL) return(pn);
  return(r);
}

/* "towards the book" */
PNode *TranslatePrep_NP(PNode *pn, PNode *pnp, Obj *max, PNode *prep, PNode *np,
                        int srclang, int tgtlang, Discourse *dc)
{
  PNode	*np_tgt, *prep_tgt, *p, *q, *r, *r1;
  np_tgt = Translate1(np, pn, max, NULL, srclang, tgtlang, dc);
  r = NULL;
  for (p = np_tgt; p; p = p->next_altern) {
    prep_tgt = Translate1(prep, pn, max, p, srclang, tgtlang, dc);
    for (q = prep_tgt; q; q = q->next_altern) {
      r1 = PNodeConstit(F_PP, q, p);
      r1->next_altern = r;
      r = r1;
    }
    PNodeBreakup(prep_tgt);
  }
  PNodeBreakup(np_tgt);
  if (r == NULL) return(pn);
  return(r);
}

/* "Trix's book" */
PNode *TranslateGenitive(PNode *pn, PNode *pnp, Obj *max, PNode *pn_np,
                         PNode *pn_of_np, int srclang, int tgtlang,
                         Discourse *dc)
{
  int	of_animate;
  PNode	*np_tgt, *of_np_tgt, *pn1, *pn2, *r, *r1;
  np_tgt = Translate1(pn_np, pn, max, NULL, srclang, tgtlang, dc);
  of_np_tgt = Translate1(pn_of_np, pn, max, NULL, srclang, tgtlang, dc);
  of_animate = pn_of_np->pn1->type == PNTYPE_NAME;
    /* todo: other animate cases */
  r = NULL;
  for (pn1 = np_tgt; pn1; pn1 = pn1->next_altern) {
    for (pn2 = of_np_tgt; pn2; pn2 = pn2->next_altern) {
      r1 = GenGenitive2(pn1, pn2,
                        of_animate ||
                        ISA(N("animate-object"), PNodeNounConcept(pn2)),
                        dc);
      r1->next_altern = r;
      r = r1;
    }
  }
  PNodeBreakup(np_tgt);
  PNodeBreakup(of_np_tgt);
  return(r);
}

/* Catch-all constituent translation. */
PNode *TranslateJuxtaposition(PNode *pn, PNode *pnp, Obj *max, PNode *agree_np,
                              int srclang, int tgtlang, Discourse *dc)
{
  return(PNodeConstit(pn->feature,
                      Translate1(pn->pn1, pn, max, agree_np, srclang, tgtlang,
                                 dc),
                      Translate1(pn->pn2, pn, max, agree_np, srclang, tgtlang,
                                 dc)));
}

PNode *TranslatePP(PNode *pn, PNode *pnp, Obj *max, int srclang, int tgtlang,
                   Discourse *dc)
{
  if (pn->pn1->feature == F_PREPOSITION && pn->pn2->feature == F_NP) {
    return(TranslatePrep_NP(pn, pnp, max, pn->pn1, pn->pn2, srclang, tgtlang,
                            dc));
  }
  return(TranslateJuxtaposition(pn, pnp, max, NULL, srclang, tgtlang, dc));
}

PNode *TranslateVP1(PNode *pn, PNode *pnp, Obj *mainverb, Obj *max,
                    LexEntry *le_mainverb, int gender, int number, int person,
                    PNode *adv1, PNode *adv2, PNode *adv3, Obj *comptense,
                    int srclang, int tgtlang, PNode *r, Discourse *dc)
{
  PNode			*adv1_tgt, *adv2_tgt, *adv3_tgt, *r1;
  ObjToLexEntry	*ole_src, *ole_tgt;
  if (!TranslateGetOles(F_VERB, mainverb, le_mainverb, NULL, tgtlang, dc,
                        &ole_src, &ole_tgt)) {
    return(r);
  }
  adv1_tgt = Translate1(adv1, pn, N("W-MAX"), NULL, srclang, tgtlang, dc);
  adv2_tgt = Translate1(adv2, pn, N("W-MAX"), NULL, srclang, tgtlang, dc);
  adv3_tgt = Translate1(adv3, pn, N("W-MAX"), NULL, srclang, tgtlang, dc);
  r1 = GenVPAdv1(ole_tgt->le, "", comptense, gender, number, person, NULL,
                 adv1_tgt, adv2_tgt, adv3_tgt, dc);
  if (r1) {
    r1->next = r;
    r = r1;
  }
  return(r);
}

/* Used if TranslateNPVP isn't applied. */
PNode *TranslateVP(PNode *pn, PNode *pnp, Obj *max, int srclang, int tgtlang,
                   Discourse *dc)
{
  int		tense, gender, number, person, degree;
  Obj		*comptense, *mood;
  ObjList	*mainverbs, *p;
  PNode		*pn_mainverb, *pn_agreeverb, *adv1, *adv2, *adv3, *r;
  LexEntry	*le_mainverb;
  if (PNodeIsCompoundTense(pn)) {
    TenseParseCompTense(pn, srclang, &pn_mainverb, &le_mainverb, &pn_agreeverb,
                        &comptense, &mood, &adv1, &adv2, &adv3);
    if (pn_mainverb && le_mainverb && comptense) {
      TranslateGetFeatures(pn_agreeverb, tgtlang, F_VERB, &tense, &gender,
                           &number, &person, &degree);
      comptense = TranslateCompoundTense(comptense, srclang, tgtlang);
      mainverbs = TranslateGetAllcons(pn_mainverb, le_mainverb);
      r = NULL;
      for (p = mainverbs; p; p = p->next) {
         if (ObjIsList(p->obj)) continue;
        r = TranslateVP1(pn, pnp, p->obj, max, le_mainverb, gender, number,
                         person, adv1, adv2, adv3, comptense, srclang, tgtlang,
                         r, dc);
      }
      return(r);
    }
  }
  return(TranslateJuxtaposition(pn, pnp, max, NULL, srclang, tgtlang, dc));
}

PNode *TranslateNP(PNode *pn, PNode *pnp, Obj *max, int srclang, int tgtlang,
                   Discourse *dc)
{
  if (pn->pn1->feature == F_ADJP && pn->pn2->feature == F_NP) {
    return(TranslateAdjDetp_NP(pn, pnp, max, F_ADJECTIVE, pn->pn1, pn->pn2,
                               PNodeGetEndPunc(pn->pn1),
                               PNodeGetEndPunc(pn->pn2),
                               srclang, tgtlang, dc));
  } else if (pn->pn1->feature == F_NP && pn->pn2->feature == F_ADJP) {
    return(TranslateAdjDetp_NP(pn, pnp, max, F_ADJECTIVE, pn->pn2, pn->pn1,
                               PNodeGetEndPunc(pn->pn1),
                               PNodeGetEndPunc(pn->pn2),
                               srclang, tgtlang, dc));
  } else if (pn->pn1->feature == F_NP && pn->pn2->feature == F_NP) {
    if (Syn_ParseIsNPNP_Genitive(pn, srclang)) {
      return(TranslateGenitive(pn, pnp, max, pn->pn2, pn->pn1->pn1, srclang,
                               tgtlang, dc));
    }
  } else if (pn->pn1->feature == F_DETERMINER && pn->pn2->feature == F_NP) {
    return(TranslateAdjDetp_NP(pn, pnp, max, F_DETERMINER, pn->pn1, pn->pn2,
                               PNodeGetEndPunc(pn->pn1),
                               PNodeGetEndPunc(pn->pn2),
                               srclang, tgtlang, dc));
  } else if (pn->pn1->feature == F_NP && pn->pn2->feature == F_PP) {
    if (Syn_ParseIsNPPP_Genitive(pn)) {
      return(TranslateGenitive(pn, pnp, max, pn->pn1, pn->pn2->pn2, srclang,
                               tgtlang, dc));
    }
  }
  /* NP NP apposition, CONJ NP, NP S, and anything else unhandled above. */
  return(TranslateJuxtaposition(pn, pnp, max, NULL, srclang, tgtlang, dc));
}

PNode *TranslateADJP(PNode *pn, PNode *pnp, Obj *max, PNode *agree_np,
                     int srclang, int tgtlang, Discourse *dc)
{
  if (pn->pn1->feature == F_ADJP && pn->pn2->feature == F_PP) {
    return(TranslateADJP_PP(pn, pnp, max, agree_np, srclang, tgtlang, dc));
  }
  return(TranslateJuxtaposition(pn, pnp, max, agree_np, srclang, tgtlang, dc));
}

PNode *TranslateS(PNode *pn, PNode *pnp, Obj *max, int srclang, int tgtlang,
                  Discourse *dc)
{
  if (pn->pn1->feature == F_NP && pn->pn2->feature == F_VP) {
    return(TranslateNPVP(pn, pnp, max, srclang, tgtlang, dc));
  }
  return(TranslateJuxtaposition(pn, pnp, max, NULL, srclang, tgtlang, dc));
}

/* "Trix is silly" (TranslateS_AscriptiveAdj)
 * "Trix sat on the rug" (TranslateS_VerbPred)
 */
PNode *TranslateNPVP(PNode *pn, PNode *pnp, Obj *max, int srclang, int tgtlang,
                     Discourse *dc)
{
  int			theta_filled[MAXTHETAFILLED];
  PNode			*vp;
  LexEntry		*le_mainverb;
  Obj			*comptense, *mood;
  ObjList		*mainverbs, *p;
  PNode			*r, *pn_mainverb, *pn_agreeverb;
  Arguments		args;
  le_mainverb = NULL;
  comptense = mood = NULL;
  mainverbs = NULL;
  ArgumentsClear(&args);
  if (!ArgumentsAddSUBJ(pn->pn1, &args)) goto failure;
  vp = pn->pn2;
  while (vp) {
    if (PNodeIsCompoundTense(vp)) {
      TenseParseCompTense(vp, srclang, &pn_mainverb, &le_mainverb,
                          &pn_agreeverb, &comptense, &mood, &args.adv1,
                          &args.adv2, &args.adv3);
      if (le_mainverb == NULL) {
        Dbg(DBGSEMPAR, DBGBAD, "TranslateNPVP 1");
        return(pn);
      }
      comptense = TranslateCompoundTense(comptense, srclang, tgtlang);
      mainverbs = TranslateGetAllcons(pn_mainverb, le_mainverb);
      vp = NULL;
    } else if (vp->pn2 == NULL) {
      vp = vp->pn1;
    } else if (vp->pn1->feature == F_VP && vp->pn2->feature == F_ADJP) {
      if (!ArgumentsAddADJP(vp->pn2, &args)) goto failure;
      vp = vp->pn1;
    } else if (vp->pn1->feature == F_VP && vp->pn2->feature == F_PP) {
      if (!ArgumentsAddIOBJ(vp->pn2, &args)) goto failure;
      vp = vp->pn1;
    } else if (vp->pn1->feature == F_VP && vp->pn2->feature == F_NP) {
      if (!ArgumentsAddOBJ(vp->pn2, &args)) goto failure;
      vp = vp->pn1;
    } else if (vp->pn1->feature == F_VERB && vp->pn2->feature == F_PRONOUN) {
      Dbg(DBGSEMPAR, DBGBAD, "TranslateNPVP VH should be transformed away?");
      if (!ArgumentsAddOBJ(vp->pn2, &args)) goto failure;
      vp = vp->pn1;
    } else if (vp->pn1->feature == F_PRONOUN && vp->pn2->feature == F_VP) {
      Dbg(DBGSEMPAR, DBGBAD, "TranslateNPVP HW should be transformed away?");
      if (!ArgumentsAddOBJ(vp->pn1, &args)) goto failure;
      vp = vp->pn2;
    } else if (vp->pn1->feature == F_ADVERB && vp->pn2->feature == F_VP) {
    /* But this doesn't preserve exact order. But that is OK. */
      if (!ArgumentsAddPreAdv(vp->pn1, &args)) goto failure;
      vp = vp->pn2;
    } else if (vp->pn1->feature == F_VP && vp->pn2->feature == F_ADVERB) {
      if (!ArgumentsAddPostAdv(vp->pn2, &args)) goto failure;
      vp = vp->pn1;
    } else if (vp->pn1->feature == F_VP && vp->pn2->feature == F_EXPLETIVE) {
      vp = vp->pn1;
    } else if (vp->pn1->feature == F_VP && vp->pn2->feature == F_VP) {
      Dbg(DBGSEMPAR, DBGBAD, "TranslateNPVP WW?");
      vp = vp->pn1;
    } else {
      vp = NULL;
    }
  }
  if (le_mainverb == NULL) {
    Dbg(DBGSEMPAR, DBGBAD, "TranslateNPVP 2");
    return(pn);
  }
  r = NULL;
  ArgumentsToThetaFilled(&args, theta_filled);
  if (args.adjp_src) {
    if (mainverbs == NULL) {
      Dbg(DBGSEMPAR, DBGBAD, "TranslateNPVP: empty mainverbs");
      mainverbs = ObjListCreate(N("standard-copula"), NULL); /* temporary */
    }
    for (p = mainverbs; p; p = p->next) {
      if (ObjIsList(p->obj)) continue;
      if (!ISA(N("copula"), p->obj)) continue;
      /* todo: This is inefficent, because we retranslate components each time
       * through loop. Same below.
       */
      r = TranslateS_AscriptiveAdj(pn, pnp, max, p->obj, le_mainverb,
                                   comptense, args.subjnp_src, args.adjp_src,
                                   theta_filled, srclang, tgtlang, r, dc);
    }
  } else {
  /* Intransitive (including neither obj nor iobj) and transitive verbs. */
    for (p = mainverbs; p; p = p->next) {
      if (ObjIsList(p->obj)) continue;
      r = TranslateS_VerbPred(pn, pnp, max, p->obj, le_mainverb, comptense,
                              &args, theta_filled, srclang, tgtlang, r, dc);
    }
  }
  if (r) return(r);
failure:
  /* todo: In this case, prefer other translations besides this one. */
  return(TranslateJuxtaposition(pn, pnp, max, NULL, srclang, tgtlang, dc));
}

/* "Trix is silly" */
PNode *TranslateS_AscriptiveAdj(PNode *pn, PNode *pnp, Obj *max, Obj *mainverb,
                                LexEntry *le_mainverb, Obj *comptense,
                                PNode *subjnp_src, PNode *adjp_src,
                                int *theta_filled, int srclang, int tgtlang,
                                PNode *r, Discourse *dc)
{
  PNode			*subjnp_tgt, *adjp_tgt, *p, *q, *r1, *vp;
  ObjToLexEntry	*ole_src, *ole_tgt;

  if (!TranslateGetOles(F_VERB, mainverb, le_mainverb, theta_filled, tgtlang,
                        dc, &ole_src, &ole_tgt)) {
    return(r);
  }

  subjnp_tgt = Translate1(subjnp_src, pn, max, NULL, srclang, tgtlang, dc);
  for (p = subjnp_tgt; p; p = p->next_altern) {
    adjp_tgt = Translate1(adjp_src, pn, max, p, srclang, tgtlang, dc);
    for (q = adjp_tgt; q; q = q->next_altern) {
      vp = GenVP(ole_tgt->le, "", comptense, subjnp_tgt, NULL, dc);
      r1 = PNodeConstit(F_S, p, PNodeConstit(F_VP, vp, q));
      r1->next_altern = r;
      r = r1;
    }
    PNodeBreakup(adjp_tgt);
  }
  PNodeBreakup(subjnp_tgt);
  return(r);
}

PNode *TranslateVerbAdjArgument(PNode *pn, PNode *pnp, Obj *max,
                                int theta_role_i, LexEntry *preple,
                                Arguments *args, int srclang,
                                int tgtlang, Discourse *dc)
{
  /* todo: Pass case down (for pronoun generation, etc.). */
  if (theta_role_i == args->subjnp_theta_role_i) {
    args->subjnp_theta_marked++;
    return(Translate1(args->subjnp_src, pn, max, NULL, srclang, tgtlang, dc));
  } else if (theta_role_i == args->objnp1_theta_role_i) {
    args->objnp1_theta_marked++;
    return(Translate1(args->objnp1_src, pn, max, NULL, srclang, tgtlang, dc));
  } else if (theta_role_i == args->objnp2_theta_role_i) {
    args->objnp2_theta_marked++;
    return(Translate1(args->objnp2_src, pn, max, NULL, srclang, tgtlang, dc));
  } else if (theta_role_i == args->iobjnp1_theta_role_i) {
    args->iobjpp1_theta_marked++;
    return(Translate1(args->iobjpp1_src->pn2, pn, max, NULL, srclang, tgtlang,
                      dc));
  } else if (theta_role_i == args->iobjnp2_theta_role_i) {
    args->iobjpp2_theta_marked++;
    return(Translate1(args->iobjpp2_src->pn2, pn, max, NULL, srclang, tgtlang,
                      dc));
  } else if (theta_role_i == args->iobjnp3_theta_role_i) {
    args->iobjpp3_theta_marked++;
    return(Translate1(args->iobjpp3_src->pn2, pn, max, NULL, srclang, tgtlang,
                      dc));
  }
  Dbg(DBGSEMPAR, DBGBAD, "TranslateS_VerbPred1 1");
  return(NULL);
}

/* pnxx is used only for iobjs. */
PNode *PNodeTranslateArgRecurse(PNode *pn, PNode *pnp, Obj *max, PNode *pnxx,
                                int constit, Arguments *args,
                                ObjToLexEntry *ole_tgt, Obj *match_cas,
                                int srclang, int tgtlang, Discourse *dc,
                                /* RESULTS */ int *success)
{
  int		i, subcat;
  Bool		isoptional;
  Obj		*cas;
  ThetaRole	*theta_roles;
  PNode		*tgt;
  LexEntry	*preple;
  *success = 1;
  for (i = 1, theta_roles = ole_tgt->theta_roles;
       theta_roles;
       i++, theta_roles = theta_roles->next) {
    ThetaRoleGet(theta_roles, &preple, &cas, &subcat, &isoptional);
    if (cas == match_cas) {
      if (!(tgt = TranslateVerbAdjArgument(pn, pnp, max, i, preple, args,
                                           srclang, tgtlang, dc))) {
        if (isoptional) return(pnxx);	/* seems to work */
        Dbg(DBGSEMPAR, DBGBAD, "no target case <%s> <%s>.<%s>", M(cas),
            preple ? preple->srcphrase : "",
            preple ? preple->features : "");
        *success = 0;
        return(NULL);
      }
      if (cas == N("iobj")) {
        pnxx = PNodeConstit(constit, pnxx, GenMakePP1(preple, tgt, dc));
      } else {
        return(tgt);
      }
    }
  }
  return(pnxx);
}

/* Example:
 * "I like rain."
 * mainverb: like
 * le_mainverb = /like.Vz/
 * subjnp_src: [X [N I]]
 * objnp_src: [X [N rain]]
 *
 * La pluie me plait.
 */
PNode *TranslateS_VerbPred(PNode *pn, PNode *pnp, Obj *max, Obj *mainverb,
                           LexEntry *le_mainverb, Obj *comptense,
                           Arguments *args, int *theta_filled, int srclang,
                           int tgtlang, PNode *r, Discourse *dc)
{
  int		success;
  ObjToLexEntry	*ole_src, *ole_tgt;
  PNode		*subjnp_tgt, *objnp_tgt, *vp, *pnxx, *adv1_tgt;
  PNode		*adv2_tgt, *adv3_tgt, *preadv1_tgt, *postadv1_tgt;

  if (!TranslateGetOles(F_VERB, mainverb, le_mainverb, theta_filled, tgtlang,
                        dc, &ole_src, &ole_tgt)) {
    return(r);
  }
  ArgumentsFillCasei(ole_src, args);

  /* Translate target subject. */
  subjnp_tgt = PNodeTranslateArgRecurse(pn, pnp, max, NULL, F_NULL, args,
                                        ole_tgt, N("subj"), srclang, tgtlang,
                                        dc, &success);
  if (!subjnp_tgt) return(r);

  /* Translate target object. */
  objnp_tgt = PNodeTranslateArgRecurse(pn, pnp, max, NULL, F_NULL, args,
                                       ole_tgt, N("obj"), srclang, tgtlang,
                                       dc, &success);
  if (!success) return(r);

  /* Generate target main verb.
   * todo: If subjnp_tgt is a list, we might need to generate different
   * agreements?
   */
  adv1_tgt = Translate1(args->adv1, pn, max, NULL, srclang, tgtlang, dc);
  adv2_tgt = Translate1(args->adv2, pn, max, NULL, srclang, tgtlang, dc);
  adv3_tgt = Translate1(args->adv3, pn, max, NULL, srclang, tgtlang, dc);
  vp = GenVPAdv(ole_tgt->le, "", comptense, subjnp_tgt, objnp_tgt, adv1_tgt,
                adv2_tgt, adv3_tgt, dc); /* todo: correct adverb order? */

  /* todo: Preserve source ordering of obj wrt iobjs.
   * This improves result in case parsing is way off base.
   * Though, one could implement heuristics for ideal generation based on
   * the length of each clause. For example, if the direct object is very long,
   * you might want to put it after the indirect object:
   * "Lucie suggests Martine come spend a few days at the house to Fabienne."
   */

  /* Add target object to target vp. */
  if (objnp_tgt) vp = PNodeConstit(F_VP, vp, objnp_tgt);

  /* Translate target iobjs. */
  vp = PNodeTranslateArgRecurse(pn, pnp, max, vp, F_VP, args, ole_tgt,
                                N("iobj"), srclang, tgtlang, dc, &success);
  if (!success) return(r);

  /* Translate pre and post adverbs (those adverbs not associated with
   * compound tense).
   */
  preadv1_tgt = Translate1(args->preadv1, pn, max, NULL, srclang, tgtlang, dc);
  if (preadv1_tgt) vp = PNodeConstit(F_VP, preadv1_tgt, vp);
  postadv1_tgt = Translate1(args->postadv1, pn, max, NULL, srclang, tgtlang,
                            dc);
  if (postadv1_tgt) vp = PNodeConstit(F_VP, vp, postadv1_tgt);

  if (!ArgumentsSatisfiesThetaCriterion(args)) return(r);

  pnxx = PNodeConstit(F_S, subjnp_tgt, vp);
  pnxx->next_altern = r;
  return(pnxx);
}

/* Example:
 * adjp_tgt: "very dependent" (translated).
 * iobjpp1_src: "on them" (untranslated).
 * Note multiple adjectives here (e.g. "Trix is stupid dumb") are not
 * grammatical.
 */
PNode *TranslateADJP_PP1(PNode *pn, PNode *pnp, Obj *max, PNode *adjp_src,
                         PNode *adjp_tgt, Arguments *args, int srclang,
                         int tgtlang, PNode *r, Discourse *dc)
{
  int			success;
  ObjToLexEntry	*ole_src, *ole_tgt;
  PNode			*adj_src, *adj_tgt, *r1;
  if (!(adj_src = PNodeFindHeadAdj(adjp_src))) {
    return(r);
  }
  if (!(adj_tgt = PNodeFindHeadAdj(adjp_tgt))) {
    return(r);
  }
  if (adj_tgt->obj == NULL) {
    return(r);
  }
  if (!TranslateGetOles(F_ADJECTIVE, adj_tgt->obj, adj_src->lexitem->le, NULL,
                        tgtlang, dc, &ole_src, &ole_tgt)) {
    return(r);
  }
  ArgumentsFillCasei(ole_src, args);
  /* todo: Add adjective direct objects once these are implemented elsewhere. */
  r1 = adjp_tgt;
  r1 = PNodeTranslateArgRecurse(pn, pnp, max, r1, F_ADJP, args, ole_tgt,
                                N("iobj"), srclang, tgtlang, dc, &success);
  if (!success) {
    return(r);
  }
  if (!ArgumentsSatisfiesThetaCriterion(args)) {
    return(r);
  }
  r1->next_altern = r;
  return(r1);
}

/* "addicted to cocaine"
 * "very dependent on them"
 * "diplomé du MIT"
 */
PNode *TranslateADJP_PP(PNode *pn, PNode *pnp, Obj *max, PNode *agree_np,
                        int srclang, int tgtlang, Discourse *dc)
{
  PNode			*adjp, *r, *p, *adjp_src, *adjp_tgt;
  Arguments		args;
  ArgumentsClear(&args);
  adjp = pn;
  while (adjp) {
    if (adjp->pn1->feature == F_ADJP && adjp->pn2->feature == F_PP) {
      ArgumentsAddIOBJ(adjp->pn2, &args);
      adjp = adjp->pn1;
    } else if (adjp->pn1->feature == F_ADJP && adjp->pn2->feature == F_NP) {
      /* todo: Case of /appelé.Aéz/ not yet implemented. */
      ArgumentsAddOBJ(adjp->pn2, &args);
      adjp = adjp->pn1;
    } else {
      adjp_src = adjp;
      adjp_tgt = Translate1(adjp, pn, max, agree_np, srclang, tgtlang, dc);
      adjp = NULL;
    }
  }
  if (adjp_tgt == NULL) {
    return(TranslateJuxtaposition(pn, pnp, max, agree_np, srclang, tgtlang,
                                  dc));
  }
  r = NULL;
  for (p = adjp_tgt; p; p = p->next_altern) {
    r = TranslateADJP_PP1(pn, pnp, max, adjp_src, p, &args, srclang, tgtlang,
                          r, dc);
  }
  if (r == NULL) return(TranslateJuxtaposition(pn, pnp, max, agree_np,
                                               srclang, tgtlang, dc));
  return(r);
}

/* End of file. */
