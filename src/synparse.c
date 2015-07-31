/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940101: begun
 * 19940115: added rule filters
 * 19940116: added fragment parsing; improved logging
 * 19940702: cosmetics and refamiliarization
 * 19940704: more cleanup
 * 19950209: modified for new text agent based scheme
 * 19950313: added fragment parsing
 * 19950316: added more filters
 * 19950420: added relative clauses
 * 19950422: adding more filters
 * 19950424: yet more filters
 * 19950425: yet more filters
 * 19950429: changing around compound tense scheme
 * 19950502: yet more filters
 * 19950502: moved MAX validation to one place where possible
 * 19951007: added scores
 * 19951210: integrated parsing work; Syn_ParseGetSemParseConstituent
 * 19951212: added OBJLISTRULEDOUT and call to Sem_AnaphoraParses
 * 19951213: integrated parsing ifdefed out
 * 19980630: mods for compound noun parsing
 */

#include "tt.h"
#include "apptrans.h"
#include "lexentry.h"
#include "lextheta.h"
#include "repbasic.h"
#include "repobj.h"
#include "repstr.h"
#include "semparse.h"
#include "semtense.h"
#include "synbase.h"
#include "synfilt.h"
#include "synparse.h"
#include "synpnode.h"
#include "synxbar.h"
#include "utilbb.h"
#include "utildbg.h"

int	SA1, SA2, SAFEAT;

/* For debugging. */
void StopAtInit()
{
  SA1 = 0;
  SA2 = 0;
  SAFEAT = F_VP;
}

void StopAtNodes(PNode *pn1, PNode *pn2, int feat)
{
  if (pn1 && pn1->num == SA1 && pn2 && pn2->num == SA2 && feat == SAFEAT) {
    Stop();
  }
}

void StopAtSet(int sa1, int sa2, int safeat)
{
  SA1 = sa1;
  SA2 = sa2;
  SAFEAT = safeat;
}

/* EXTERNAL FUNCTIONS */

void Syn_ParseSem_Parse(PNode *pn, Channel *ch, int tgtlang, Discourse *dc)
{
  ObjList *concepts;
  dc->defer_ok = 0;
  dc->pn_root = pn;
  concepts = Sem_ParseParse(pn, dc);
  if (TranslationOn) {
    Translate(pn, concepts, ch, DC(dc).lang, tgtlang, dc);
  }
}

#define MAXPNS 15

void Syn_ParseFragments(Channel *ch, int tgtlang, Discourse *dc, int eoschar)
{
  size_t start_lowerb, max_upperb;
  int    max_pns_len, i, in_gap, c;
  PNode  *pn, *max_pns[MAXPNS];
  start_lowerb = ch->synparse_lowerb;
  in_gap = 0;
  while (start_lowerb < ch->synparse_upperb) {
    /* Find longest PNodes starting at start_lowerb. */
    max_upperb = SIZENEGINF;
    max_pns_len = 0;
    for (pn = ch->synparse_pns; pn; pn = pn->next) {
      if (pn->lowerb != start_lowerb) continue;
      if (pn->upperb > max_upperb) {
        max_upperb = pn->upperb;
        max_pns_len = 0;
        max_pns[max_pns_len++] = pn;
      } else if (pn->upperb == max_upperb) {
        if (max_pns_len < MAXPNS) max_pns[max_pns_len++] = pn;
      }
    }
    if (max_pns_len == 0) {
      Dbg(DBGSEMPAR, DBGDETAIL, "Syn_ParseFragments gap");
      if (TranslationOn) {
        if (in_gap == 0) fputc(UNTRANS, TranslateStream);
        if ((c = ch->buf[start_lowerb]) != NEWLINE) {
          fputc(c, TranslateStream);
        }
      }
      in_gap = 1;
      start_lowerb++;
      continue;
    }
    if (in_gap) {
      if (TranslationOn) fputc(UNTRANS, TranslateStream);
      in_gap = 0;
    }
    if (TranslationOn) TranslateBegin(ch);
    for (i = 0; i < max_pns_len; i++) {
      Dbg(DBGSEMPAR, DBGDETAIL, "**** SEMANTIC PARSE FRAGMENT ****");
      Syn_ParseSem_Parse(max_pns[i], ch, tgtlang, dc);
    }
    if (TranslationOn) TranslationEnd(ch, ' ', tgtlang, dc);
    start_lowerb = max_upperb+1L;
  }
  if (TranslationOn) {
    if (in_gap) {
      fputc(UNTRANS, TranslateStream);
    }
  }
}

long Syn_ParseCnt;

void Syn_ParseParse(Channel *ch, Discourse *dc, size_t in_lowerb,
                    size_t in_upperb, int eoschar)
{
  int		changed, lang, tgtlang;
  size_t	lowerb,	upperb;
  PNode		*pn;
  ABrainTask	*abt;

  Dbg(DBGGEN, DBGDETAIL, "**** SYNTACTIC PARSE BEGIN ****");

  abt = BBrainBegin(N("parse"), 120L, INTNA);
  lang = DC(dc).lang;
  tgtlang = FeatureFlipLanguage(lang);
  ch->synparse_pns = NULL;
  ch->synparse_pnnnext = PNUMSTART;
  ch->synparse_sentences = 0;
  lowerb = SIZEPOSINF;
  upperb = SIZENEGINF;
  for (pn = ch->pnf->first; pn; pn = pn->next) {
    if (pn->type >= PNTYPE_MIN_SYNPARSE && pn->type <= PNTYPE_MAX_SYNPARSE) {
      if (pn->lowerb >= in_lowerb &&
          pn->lowerb <= in_upperb) {
        ch->synparse_pns = PNodeCopy(pn, ch->synparse_pns);
        Syn_ParseAssignPnum(ch, ch->synparse_pns);
        if (pn->lowerb < lowerb) lowerb = pn->lowerb;
        if (pn->upperb > upperb) upperb = pn->upperb;
      }
    }
  }
  Dbg(DBGSYNPAR, DBGHYPER, "Syn_ParseParse %ld %ld -> %ld %ld\n",
      in_lowerb, in_upperb, lowerb, upperb);
  if (lowerb == SIZEPOSINF) lowerb = in_lowerb;
  if (upperb == SIZENEGINF) upperb = in_upperb;
  if (DbgOn(DBGSYNPAR, DBGDETAIL)) {
    PNodePrintSubstring(Log, ch, lowerb, upperb, 1, LINELEN);
    fputc(NEWLINE, Log);
  }
  ch->synparse_lowerb = lowerb;
  ch->synparse_upperb = upperb;

  if (in_lowerb < lowerb) {
    TranslateSpitUntranslated(ch, in_lowerb, lowerb-1);
  }

  TranslateBegin(ch);
  changed = 1;
  while (changed) {
    changed = 0;
    for (pn = ch->synparse_pns; pn; pn = pn->next) {
      if (BBrainStopsABrain(abt)) {
        changed = 0;
        break;
      }
      if (!pn->appliedsingletons) {
        if (Syn_ParseApplySingletonRules(ch, dc, pn, lang)) changed = 1;
        pn->appliedsingletons = 1;
      }
      if (Syn_ParseApplyRules(ch, dc, pn, lang)) changed = 1;
    }
  }
  BBrainEnd(abt);

  if (Sem_ParseResults == NULL) {
  /* Note Sem_ParseResults must be initialized to NULL by caller of this
   * function.
   */
    Syn_ParseFragments(ch, tgtlang, dc, eoschar);
  } else if (TranslationOn) {
    TranslationEnd(ch, eoschar, tgtlang, dc);
  }

  if (upperb < in_upperb) {
    TranslateSpitUntranslated(ch, upperb+1, in_upperb);
  }

  /* <here> */
  if (DbgOn(DBGGEN, DBGHYPER) || (ch->synparse_sentences == 0)) {
    Syn_ParsePrintPNodes(Log, ch);
  }
  Syn_ParseCnt += ch->synparse_sentences;

  Dbg(DBGGEN, DBGDETAIL, "**** SYNTACTIC PARSE END ****");

  Dbg(DBGGEN, DBGDETAIL, "%d syntactic parse(s) [session total %ld] of <%.10s>",
      ch->synparse_sentences, Syn_ParseCnt, ch->buf + lowerb);
}

void Syn_ParseParseDone(Channel *ch)
{
#ifdef notdef
  /* todoFREE: We can't free because cx->sproutpn might point to some of
   * these.
   */
  for (pn = ch->synparse_pns; pn; ) {
    n = pn->next;
    PNodeFree(pn);
    pn = n;
  }
#endif
  ch->synparse_pns = NULL;
  ch->synparse_pnnnext = PNUMSTART;
  ch->synparse_lowerb = 0L;
  ch->synparse_upperb = 0L;
}

/* INTERNAL ROUTINES */

void Syn_ParseAssignPnum(Channel *ch, PNode *pn)
{
  pn->num = ch->synparse_pnnnext;
  ch->synparse_pnnnext++;
}

Bool Syn_ParseIsTopLevelSentence(PNode *pn)
{
  PNode	*auxverb, *mainverb;
  if (pn->pn1 && pn->pn1->feature == F_NP &&
      pn->pn2 && pn->pn2->feature == F_ADJP) {
  /* "Le ciel couvert" must occur embedded in a larger S. */
    return(0);
  }
  PNodeFindHeadVerbS(pn, &auxverb, &mainverb);
  if (auxverb && auxverb->lexitem &&
      F_PRESENT_PARTICIPLE ==
        FeatureGet(auxverb->lexitem->features, FT_TENSE)) {
    /* Disallow sentence "John going to the store."
     * "[X I'm] [W having dinner.]"
     */
    return(0);
  }
  return(1);
}

/* Basically, X-MAXes are SemParseConstituents. */
PNode *Syn_ParseGetSemParseConstituent(PNode *pn, PNode *pn1, PNode *pn2)
{
  if (pn->feature == F_NP) {
#ifdef notdef
    /* todo */
    if (pn1 && pn1->feature == F_NP &&
        pn2 && pn2->feature == F_NP &&
        (!Syn_ParseIsXX_X_NotApposition(pn1, pn2, lang))) {
    }
#endif
    if (pn1 && pn1->feature == F_S &&
        pn2 == NULL) {
      return(pn1);
    }
  } else if (pn->feature == F_PP) {
    if (pn1 && pn1->feature == F_PREPOSITION &&
        pn2 && pn2->feature == F_NP) {
      return(pn2);
    }
  } else if (pn->feature == F_VP) {
    if (pn1 && pn1->feature == F_VP &&
        pn2 && pn2->feature == F_NP) {
      return(pn2);
    }
  } else if (pn->feature == F_S) {
    if (pn1 && pn1->feature == F_NP &&
        pn2 && pn2->feature == F_VP) {
      return(pn1);
    }
  } else if (pn->feature == F_ADJP) {
    if (pn1 && pn1->feature == F_ADJP &&
        pn2 && pn2->feature == F_NP) {
      return(pn2);
    }
  }
  return(NULL);
}

void Syn_ParseAdd(Channel *ch, Discourse *dc, char feature,
                  PNode *pn1, PNode *pn2, Float score, size_t lowerb,
                  size_t upperb, int lang)
{
  PNode		*pn;

  Dbg(DBGSYNPAR, DBGHYPER, "Adding parse node <%c:%d> %ld %ld ",
      feature, ch->synparse_pnnnext-1, lowerb, upperb);

  if (DbgOn(DBGSYNPAR, DBGDETAIL)) {
    fprintf(Log, "SYN %c <- ", (char)feature);
    if (pn1) PNodePrint1(Log, ch, pn1, 0);
    if (pn2) {
      fputs(" + ", Log);
      PNodePrint1(Log, ch, pn2, 0);
    }
    fputc(NEWLINE, Log);
  }

  pn = PNodeCreate(feature, NULL, pn1, pn2, lowerb, upperb, NULL, NULL);
  pn->type = PNTYPE_CONSTITUENT;
  pn->score = score;
  Syn_ParseAssignPnum(ch, pn);

#ifdef INTEGSYNSEM
  if (pn1_2 = Syn_ParseGetSemParseConstituent(pn, pn1, pn2)) {
    if (pn1_2->concepts == OBJLISTRULEDOUT) {
    /* Sem_Parse of this node already attempted and it was ruled out. */
      goto failure;
    }
    if (pn1_2->concepts == NULL) {
      Dbg(DBGSEMPAR, DBGDETAIL,
          "**** INTEGRATED PARSING: SEMANTIC PARSE CONSTITUENT ****");
      dc->defer_ok = 1;
      dc->pn_root = pn1_2;
      objs = Sem_ParseParse1(pn1_2, NULL, dc);
      if (objs == OBJLISTDEFER) {
      /* <pn1_2> can't be parsed yet due to lack of sufficient
       * information (cf CaseFrame), so allow <pn> and (possibly) try
       * parsing this again later.
       */
      } else {
#ifdef INTEGSYNSEMANA
        objs = Sem_AnaphoraParses(objs, pn1_2, dc->cx_cur, NULL);
#endif
        if (objs == NULL) {
        /* <pn1_2> can never be parsed, so rule out <pn> which derives
         * from it.
         */
          pn1_2->concepts = OBJLISTRULEDOUT;
          goto failure;
        } else {
        /* Otherwise, <pn1_2> has been parsed and will never
         * need to be parsed again.
         */
          pn1_2->concepts = objs;
          pn1_2->concepts_parent = pn;
        }
      }
    }
  }
#endif
  if (pn->feature == F_S &&
      pn->lowerb == ch->synparse_lowerb &&
      pn->upperb == ch->synparse_upperb &&
      Syn_ParseIsTopLevelSentence(pn)) {
    ch->synparse_sentences++;
    Dbg(DBGSEMPAR, DBGDETAIL, "**** SEMANTIC PARSE TOP SENTENCE ****");
    Syn_ParseSem_Parse(pn, ch, FeatureFlipLanguage(lang), dc);
  }

  pn->next = ch->synparse_pns;
  ch->synparse_pns = pn;
  return;
/*
failure:
  Dbg(DBGSYNPAR, DBGDETAIL, "Sem_Parse failure");
  todo: Free pn. ch->synparse_pnnnext-- ?
*/
}

Bool Syn_ParseApplySingletonRules(Channel *ch, Discourse *dc, PNode *pn,
                                  int lang)
{
  Float	score;
  int	k, changed;
  changed = 0;
  for (k = 0; k < NUMPOS; k++) {
    if (0.0 < (score = Syn_ParseDoesRuleFire(pn, NULL, k, lang, dc))) {
      score = ScoreCombine(score, pn->score);
      Syn_ParseAdd(ch, dc, IPOSG(k), pn, NULL, score, pn->lowerb,
                   pn->upperb, lang);
      changed = 1;
    }
  }
  return(changed);
}

Bool Syn_ParseApplyRules(Channel *ch, Discourse *dc, PNode *pn1, int lang)
{
  int	k, changed;
  Float	score;
  PNode *pn2;
  changed = 0;
  for (pn2 = ch->synparse_pns; pn2; pn2 = pn2->next) {
    if (pn2 == pn1) continue;
    if (pn2->upperb == (pn1->lowerb-1)) {
      if (PNodeDidVersus(pn2, pn1)) continue;
      PNodeAddVersus(pn2, pn1);
      for (k = 0; k < NUMPOS; k++) {
        if (0.0 < (score = Syn_ParseDoesRuleFire(pn2, pn1, k, lang, dc))) {
          score = ScoreCombine(score, pn1->score);
          score = ScoreCombine(score, pn2->score);
          Syn_ParseAdd(ch, dc, IPOSG(k), pn2, pn1, score, pn2->lowerb,
                       pn1->upperb, lang);
          changed = 1;
        }
      }
    } else if (pn2->lowerb == (pn1->upperb+1)) {
      if (PNodeDidVersus(pn1, pn2)) continue;
      PNodeAddVersus(pn1, pn2);
      for (k = 0; k < NUMPOS; k++) {
        if (0.0 < (score = Syn_ParseDoesRuleFire(pn1, pn2, k, lang, dc))) {
          score = ScoreCombine(score, pn1->score);
          score = ScoreCombine(score, pn2->score);
          Syn_ParseAdd(ch, dc, IPOSG(k), pn1, pn2, score, pn1->lowerb,
                       pn2->upperb, lang);
          changed = 1;
        }
      }
    }
  }
             
  return(changed);
}

void Syn_ParsePrintPNodes(FILE *stream, Channel *ch)
{
  PNode	*pn;
  fprintf(stream, "*********All Syn_Parse PNodes*********\n");
  for (pn = ch->synparse_pns; pn; pn = pn->next) {
    PNodePrettyPrint(stream, ch, pn);
  }
}

Bool Syn_ParseIsVPRestrict(PNode *w1, PNode *w2)
{
  int	obj, iobj;
  PNodeCountVPObjects(w1, w2, &obj, &iobj);
  if (obj > 2) return(1);
  if (iobj > 4) return(1);	/* todo: OK? */
  if (iobj > 0 && obj > 1) return(1);
  return(0);
}

Bool Syn_IsBadVP(PNode *pn, int lang)
{
  return(pn && pn->feature == F_VP &&
         PNodeIsCompoundTense(pn) &&
         (!TenseParseIsSubCompTenseOK(pn, lang)));
}

Bool Syn_ParseIsValidMAX(PNode *pn1, PNode *pn2, int k, int lang)
{
  if (k != POSGI(F_NP)) {
    if (pn1 && pn1->feature == F_NP && (!XBarValidX_MAX(pn1))) return(0);
    if (pn2 && pn2->feature == F_NP && (!XBarValidX_MAX(pn2))) return(0);
  }
  if (k != POSGI(F_PP)) {
    if (pn1 && pn1->feature == F_PP && (!XBarValidY_MAX(pn1))) return(0);
    if (pn2 && pn2->feature == F_PP && (!XBarValidY_MAX(pn2))) return(0);
  }
  if (k != POSGI(F_VP)) {
    if (pn1 && pn1->feature == F_VP && (!XBarValidW_MAX(pn1, lang))) return(0);
    if (pn2 && pn2->feature == F_VP && (!XBarValidW_MAX(pn2, lang))) return(0);
  }
  if (k != POSGI(F_ADJP)) {
    if (pn1 && pn1->feature == F_ADJP && (!XBarValidE_MAX(pn1))) return(0);
    if (pn2 && pn2->feature == F_ADJP && (!XBarValidE_MAX(pn2))) return(0);
  }
  return(1);
}

/* 0.0 = rule doesn't fire
 * else returns score with which rule fires
 */
Float Syn_ParseDoesRuleFire(PNode *pn1, PNode *pn2, int k, int lang,
                            Discourse *dc)
{
  StopAtNodes(pn1, pn2, IPOSG(k));
  if (Syn_IsBadVP(pn1, lang)) return(0.0);
  if (Syn_IsBadVP(pn2, lang)) return(0.0);
  if (!Syn_ParseIsValidMAX(pn1, pn2, k, lang)) return(0.0);
  if (pn2 == NULL) {
    if (0 < BaseRules[POSGI(pn1->feature)][POSGI(F_NULL)][k]) {
      if (pn1->feature == F_PRONOUN && k == POSGI(F_NP)) {
        return(Syn_ParseFilterH_X(pn1, lang));
      } else if (pn1->feature == F_S && k == POSGI(F_NP)) {
        return(Syn_ParseFilterZ_X(pn1, lang));
      } else if (pn1->feature == F_VP && k == POSGI(F_S)) {
        return(Syn_ParseFilterW_Z(pn1, lang));
      } else {
        return(1.0);
      }
    }
  } else if (0 < BaseRules[POSGI(pn1->feature)][POSGI(pn2->feature)][k]) {
    if (dc->mode & DC_MODE_COMPOUND_NOUN) {
      if (pn1->feature == F_NP && pn2->feature == F_NP &&
          k == POSGI(F_NP)) {
        return 1.0;
      } else {
        return 0.0;
      }
    }
    if (k == POSGI(F_VP)) {
      if (Syn_ParseIsVPRestrict(pn1, pn2)) return(0.0);
    }
    if (pn1->feature == F_ADVERB && pn2->feature == F_ADJP &&
        k == POSGI(F_ADJP)) {
      return(Syn_ParseFilterBE_E(pn1, pn2, lang));
    } else if (pn1->feature == F_DETERMINER && pn2->feature == F_ADJP &&
               k == POSGI(F_ADJP)) {
      return(Syn_ParseFilterDE_E(pn1, pn2, lang));
    } else if (pn1->feature == F_ADJP && pn2->feature == F_PP &&
               k == POSGI(F_ADJP)) {
      return(Syn_ParseFilterEY_E(pn1, pn2, lang));
    } else if (pn1->feature == F_ADJP && pn2->feature == F_ADJP &&
               k == POSGI(F_ADJP)) {
      return(Syn_ParseFilterEE_E(pn1, pn2, lang));
    } else if (pn1->feature == F_CONJUNCTION && pn2->feature == F_ADJP &&
               k == POSGI(F_ADJP)) {
      return(Syn_ParseFilterKE_E(pn1, pn2, lang));
    } else if (pn1->feature == F_VP && pn2->feature == F_ADJP &&
               k == POSGI(F_VP)) {
      return(Syn_ParseFilterWE_W(pn1, pn2, lang));
    } else if (pn1->feature == F_DETERMINER && pn2->feature == F_NP &&
               k == POSGI(F_NP)) {
      return(Syn_ParseFilterDX_X(pn1, pn2, lang));
    } else if (pn1->feature == F_ADJP && pn2->feature == F_NP &&
               k == POSGI(F_NP)) {
      return(Syn_ParseFilterEX_X(pn1, pn2, lang));
    } else if (pn1->feature == F_NP && pn2->feature == F_ADJP &&
               k == POSGI(F_NP)) {
      return(Syn_ParseFilterXE_X(pn1, pn2, lang));
    } else if (pn1->feature == F_VP && pn2->feature == F_PP &&
               k == POSGI(F_VP)) {
      return(Syn_ParseFilterWY_W(pn1, pn2, lang));
    } else if (pn1->feature == F_VP && pn2->feature == F_VERB &&
               k == POSGI(F_VP)) {
      return(Syn_ParseFilterWV_W(pn1, pn2, lang));
    } else if (pn1->feature == F_ADVERB && pn2->feature == F_VP &&
               k == POSGI(F_VP)) {
      return(Syn_ParseFilterBW_W(pn1, pn2, lang));
    } else if (pn1->feature == F_VP && pn2->feature == F_ADVERB &&
               k == POSGI(F_VP)) {
      return(Syn_ParseFilterWB_W(pn1, pn2, lang));
    } else if (pn1->feature == F_PRONOUN && pn2->feature == F_VP &&
               k == POSGI(F_VP)) {
      return(Syn_ParseFilterHW_W(pn1, pn2, lang));
    } else if (pn1->feature == F_PP && pn2->feature == F_S &&
               k == POSGI(F_S)) {
      return(Syn_ParseFilterYZ_Z(pn1, pn2, lang));
    } else if (pn1->feature == F_VP && pn2->feature == F_PRONOUN &&
               k == POSGI(F_VP)) {
      return(Syn_ParseFilterWH_W(pn1, pn2, lang));
    } else if (pn1->feature == F_VP && pn2->feature == F_NP &&
               k == POSGI(F_VP)) {
      return(Syn_ParseFilterWX_W(pn1, pn2, lang));
    } else if (pn1->feature == F_CONJUNCTION && pn2->feature == F_NP &&
               k == POSGI(F_NP)) {
      return(Syn_ParseFilterKX_X(pn1, pn2, lang));
    } else if (pn1->feature == F_CONJUNCTION && pn2->feature == F_PP &&
               k == POSGI(F_PP)) {
      return(Syn_ParseFilterKY_Y(pn1, pn2, lang));
    } else if (pn1->feature == F_PREPOSITION && pn2->feature == F_NP &&
               k == POSGI(F_PP)) {
      return(Syn_ParseFilterRX_Y(pn1, pn2, lang));
    } else if (pn1->feature == F_PREPOSITION && pn2->feature == F_ADVERB &&
               k == POSGI(F_PP)) {
      return(Syn_ParseFilterRB_Y(pn1, pn2, lang));
    } else if (pn1->feature == F_NP && pn2->feature == F_PP &&
               k == POSGI(F_NP)) {
      return(Syn_ParseFilterXY_X(pn1, pn2, lang));
    } else if (pn1->feature == F_NP && pn2->feature == F_S &&
               k == POSGI(F_NP)) {
      return(Syn_ParseFilterXZ_X(pn1, pn2, lang));
    } else if (pn1->feature == F_NP && pn2->feature == F_VP &&
               k == POSGI(F_NP)) {
      return(Syn_ParseFilterXW_X(pn1, pn2, lang));
    } else if (pn1->feature == F_NP && pn2->feature == F_NP &&
               k == POSGI(F_NP)) {
      return(Syn_ParseFilterXX_X(pn1, pn2, lang));
    } else if (pn1->feature == F_NP && pn2->feature == F_ELEMENT &&
               k == POSGI(F_NP)) {
      return(Syn_ParseFilterX9_X(pn1, pn2, lang));
    } else if (pn1->feature == F_PP && pn2->feature == F_PP &&
               k == POSGI(F_PP)) {
      return(Syn_ParseFilterYY_Y(pn1, pn2, lang));
    } else if (pn1->feature == F_NP && pn2->feature == F_VP &&
               k == POSGI(F_S)) {
      return(Syn_ParseFilterXW_Z(pn1, pn2, lang));
    } else if (pn1->feature == F_NP && pn2->feature == F_ADJP &&
               k == POSGI(F_S)) {
      return(Syn_ParseFilterXE_Z(pn1, pn2, lang));
    } else if (pn1->feature == F_PRONOUN && pn2->feature == F_NP &&
               k == POSGI(F_S)) {
      return(Syn_ParseFilterHX_Z(pn1, pn2, lang));
    } else if (pn1->feature == F_S && pn2->feature == F_S &&
               k == POSGI(F_S)) {
      return(Syn_ParseFilterZZ_Z(pn1, pn2, lang));
    } else if (pn1->feature == F_NP && pn2->feature == F_S &&
               k == POSGI(F_S)) {
      return(Syn_ParseFilterXZ_Z(pn1, pn2, lang));
    } else if (pn1->feature == F_ADJP && pn2->feature == F_VP &&
               k == POSGI(F_S)) {
      return(Syn_ParseFilterEW_Z(pn1, pn2, lang));
    } else if (pn1->feature == F_ADJP && pn2->feature == F_S &&
               k == POSGI(F_S)) {
      return(Syn_ParseFilterEZ_Z(pn1, pn2, lang));
    } else if (pn1->feature == F_ADVERB && pn2->feature == F_S &&
               k == POSGI(F_S)) {
      return(Syn_ParseFilterBZ_Z(pn1, pn2, lang));
    } else if (pn1->feature == F_S && pn2->feature == F_ADVERB &&
               k == POSGI(F_S)) {
      return(Syn_ParseFilterZB_Z(pn1, pn2, lang));
    } else {
      return(1.0);
    }
  }
  return(0.0);
}

/* End of file. */
