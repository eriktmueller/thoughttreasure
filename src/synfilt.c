/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "lexentry.h"
#include "lextheta.h"
#include "repbasic.h"
#include "repobj.h"
#include "repstr.h"
#include "synfilt.h"
#include "synparse.h"
#include "synpnode.h"
#include "synxbar.h"
#include "utildbg.h"

/* Constraints on number of arguments to maximal projections.
 * E-MAX = AdjP
 * W-MAX = VP
 * X-MAX = NP
 * todo: Other arguments such as E-MAX arguments to X-MAX ("being funny").
 */
#define E_MAX_NPS       0 /* Raise to 1 once these are handled. */
#define E_MAX_PPS       2
#define E_MAX_ARGS      (E_MAX_NPS+E_MAX_PPS)
#define W_MAX_NPS       2 /* Does not include subject. */
#define W_MAX_PPS       3
#define W_MAX_ARGS      4
#define X_MAX_NPS       0
#define X_MAX_PPS       (W_MAX_PPS+1)
#define X_MAX_ARGS      (X_MAX_PPS+X_MAX_NPS)

Float Syn_ParseFilterEY_E(PNode *e, PNode *y, int lang)
{
  if (!XBarSatisfiesArgRestrictions(e, E_MAX_NPS, E_MAX_PPS-1, E_MAX_ARGS-1)) {
    return(0.0);
  }
  return(1.0);
}

/* Accepts:
 * [W [V vais]]
 * [W [H me] [W [V va]]]
 * [W [H vraiment] [W [V va]]]
 * Rejects:
 * [W [W [V vais]] [B pas]]
 */
Float Syn_ParseFilter_Verb_Or_InterHB(PNode *w)
{
  int		pos;
  LexEntry	*le;
  if (w->pn1 && w->pn1->feature == F_VERB && w->pn2 == NULL) return(1.0);
  if (NULL == (le = PNodeLeftmostLexEntry(w))) return(0.0);
  pos = FeatureGet(le->features, FT_POS);
  if (pos != F_PRONOUN && pos != F_ADVERB) return(0.0);
  return(1.0);
}

Bool Syn_ParseFilter_IsXE(PNode *z)
{
  return(z->pn1 && z->pn1->feature == F_NP &&
         z->pn2 && z->pn2->feature == F_ADJP);
}

/* NEGATIVES:
 *   English "not" occurs as W B.
 *   French "ne" occurs as B W.
 *   French "pas" occurs as B W (in "ne pas") or W B.
 * OTHER ADVERBS:
 *   English adverbs can occur as B W or W B.
 *   todo: Use W B when W is auxiliary verb.
 *   French adverbs (except for "ne") can occur only as W B.
 * EXAMPLES:
 *   [W [W [W [W [V should]] [B happily]] [B not]] [W [V change]]]
 *   [W [W [W [V do]] [B not]] [W [B happily] [W [V change]]]]
 *   "Je [B ne] [W sais] pas"
 *   "I [B [W have] [B carefully]] [W checked] the reactor settings."
 *   "I [W have] [[B carefully] [W checked]] the reactor settings."
 *   "I [B carefully] [W check] the reactor settings."
 *   "I [W check] [B carefully] to see what the problem is."
 *   "Paganini [W played the violin] [B beautifully]."
 *
 * One might think that some restrictions-such as "just" coming before
 * the verb or after the auxiliary-might not be needed in parsing because
 * we are only presented with well-formed input. But we can always come
 * up with cases where they are needed: "I finished just my homework."
 * "I just finished my homework."
 *
 * todoSCORE: Adverbs of manner tend to occur at end of sentence or between
 * subject and verb.
 */
Float Syn_ParseFilterBW_WB_W(PNode *b, PNode *w, int is_bw, int lang)
{
  LexEntry	*adv_le;
  if (Syn_Parse_IsOnlyRelativeW(w, lang)) return(0.0);
  if ((adv_le = PNodeLeftmostLexEntry(b))) {
    if (LexEntryAllConceptsFeat(is_bw ? F_NO_BW_W : F_NO_WB_W, adv_le)) {
    /* Disallow intensifiers such as "very" and "très" and so on. */
      return(0.0);
    }
    /* Hardcoded word restrictions: */
    if (lang == F_FRENCH) {
      if (streq(adv_le->srcphrase, "pas")) {
        if (PNodeFeaturesIn(w, FS_NP_PP)) return(0.0);
      } else if (streq(adv_le->srcphrase, "ne")) {
        if (!is_bw) return(0.0);
        return(Syn_ParseFilter_Verb_Or_InterHB(w));
      } else {
        if (is_bw) return(0.0);
      }
    } else if (lang == F_ENGLISH) {
      if (streq(adv_le->srcphrase, "not")) {
        if (is_bw || (!PNodeFeaturesIn(w, FS_NP_PP))) {
          return(0.0);
        }
      }
    }
  }
  return(1.0);
}

/* "That is not untrue."
 * "That is really not untrue."
 * "That is very untrue."
 * "That is so not true."
 * "That is really very untrue."
 * "Let me tell you a not-untrue story."
 * See (Alexander, 1988, chapter 7).
 */
Float Syn_ParseFilterBE_E(PNode *b, PNode *e, int lang)
{
  LexEntry	*adv_le;
#ifdef notdef
  /* todo: Too extreme. Achieve a similar effect by adding more
   * FT_FILTERs.
   */
  if (!LexEntryConceptIsAncestor(N("adverb-of-absolute-degree"),
                                 PNodeLeftmostLexEntry(b))) {
    return(0.0);
  }
#endif
  if (b->type == PNTYPE_TSRANGE) {
  /* Adverbs of place and time do not usually modify adjectives, but even
   * here you can find counterexamples:
   * "He is stupid in Lille, but smart in Paris."
   * "She is happy at 5 pm, and bittersweet at 9 pm."
   */
    return(0.0);
  }
  if (PNodeNumberOfWords(b) > 1) {
  /* Phrasal adverbs modifying adjectives not allowed?
   * Possible Counterexample: "à peine".
   */
    return(0.0);
  }
  if ((adv_le = PNodeLeftmostLexEntry(b))) {
    if (LexEntryAllConceptsFeat(F_NO_BE_E, adv_le)) {
    /* Disallow "beaucoup intelligent". */
      return(0.0);
    }
  }
  return(1.0);
}

/* "le plus dense" */
Float Syn_ParseFilterDE_E(PNode *d, PNode *e, int lang)
{
  if (lang == F_FRENCH) {
    if (e->pn1->feature == F_ADVERB &&
        e->pn2->feature == F_ADJP &&
        LexEntrySuperlativeClass(e->pn1->lexitem->word, lang)) {
      return(1.0);
    }
    /* No need to handle "meilleur" and "pire" here because they come before
     * adjective.
     */
  }
  return(0.0);
}

/* todo: In French, adjective should agree in gender and number with noun.
 * Also in EX_X.
 */
Float Syn_ParseFilterXE_X(PNode *x, PNode *e, int lang)
{
  /* todoSCORE: Low score if adjective never comes after noun. */
  return(1.0);
}

Float Syn_ParseFilterEX_X(PNode *e, PNode *x, int lang)
{
  /* todoSCORE: Low score if adjective never preposed.
   * Sem_Parse already constrains on an individual concept basis.
   */
  if (PNodeFeatureIn(e, F_PP) || PNodeFeatureIn(x, F_DETERMINER)) return(0.0);
  if (x->pn1->feature == F_ADJP) {
  /* Disallow e:[E ...] x:[X [E ] ...]: the E's should be one.
   * "I'm having dinner"
   */
    return(0.0);
  }
  if (x->pn1 && x->pn1->feature == F_PRONOUN &&
      x->pn2 == NULL) {
  /* Disallow [American who]. */
    return(0.0);
  }
  return(1.0);
}

Float Syn_ParseFilterDX_X(PNode *d, PNode *x, int lang)
{
  if (PNodeFeatureIn(x, F_DETERMINER)) {
  /* todo: However, if <d> is a predeterminer, this is acceptable
   * (Battye and Hintze, 1992, p. 216).
   * Might be OK as is, because tout(e)(s) is currently considered an
   * adjective.
   */
    return(0.0);
  }
  if (x->pn1 && x->pn1->feature == F_S) return(0.0);
  if (PNodeFeaturesIn(x, FS_VP_PP)) {
  /* Determiners should occur at the lowest, innermost level, since they
   * are X-MAX arguments.
   * That is, we allow:
   * [X [X [D the] [X [N grocer]]]
   *     [Y [R of] .]]
   * and disallow:
   * [X [D the]
   *     [X [X [N grocer]] [Y [R of] ...]]]
   * also [the [grocer who did whatever]]
   */
    return(0.0);
  }
  return(1.0);
}

/* Determine whether an adverb can be "shifted" into the VP from the NP,
 * PP, or AdjP. This is to eliminate supposedly redundant parses.
 * [W
 *  [W
 *   [B n]
 *   [W [V était]]]
 *  [X
 *   [E
 *    [B pas]
 *    [E [A une]]]
 *   [X [N belle journée]]]]
 */
Bool Syn_Parse_IsAdvShiftableToVP(PNode *w, PNode *x, int lang)
{
  int		w_pos;
  LexEntry	*w_le;
  PNode		*adv_pn;

  if ((adv_pn = PNodeLeftmost(x)) && adv_pn->feature == F_ADVERB) {
    if (0.0 == Syn_ParseFilterBW_WB_W(adv_pn, w, 0, lang)) {
      return(0);
    }
    if (!(w_le = PNodeRightmostLexEntry(w))) {
      return(0);
    }
    w_pos = FeatureGet(w_le->features, FT_POS);
    if (w_pos == F_VERB || w_pos == F_ADVERB) {
    /* The adverb which is the leftmost terminal of <x> can modify verbs
     * and it is already to the right of a verb or adverb, so this adverb can
     * be "shifted" into the verb phrase to its left. No shifting actually
     * occurs here but this parse is ruled out and it is left to other rules
     * to introduce the adverb inside the verb phrase.
     */
      return(1);
    }
  }
  return(0);
}

Float Syn_ParseFilterWE_W(PNode *w, PNode *e, int lang)
{
  if (Syn_Parse_IsOnlyRelativeW(w, lang)) return(0.0);
  if (!LexEntryConceptIsAncestor(N("copula"), PNodeFindMainverbLe(w))) {
    return(0.0);
  }
  if (Syn_Parse_IsAdvShiftableToVP(w, e, lang)) return(0.0);
  return(1.0);
}

/* "Je ne vais pas m'en aller" */
Bool Syn_ParseRightmostIsPronoun(PNode *w)
{
  LexEntry	*le;
  le = PNodeRightmostLexEntry(w);
  return(le && (F_PRONOUN == FeatureGet(le->features, FT_POS)));
}

/* result cnt score_max_cnt score_min_cnt
 *  1.0     1            10            20
 *  1.0    10            10            20
 *  0.5    15            10            20
 *  0.0    20            10            20
 *  0.0    30            10            20
 */
Float ScoreCount(int cnt, int score_max_cnt, int score_min_cnt)
{
  if (cnt <= score_max_cnt) return(SCORE_MAX);
  if (cnt >= score_min_cnt) return(SCORE_MIN);
  return(1.0 -
         ((cnt - score_max_cnt)/((Float)(score_min_cnt - score_max_cnt))));
}

Float Syn_ParseFilterWY_W(PNode *w, PNode *y, int lang)
{
  Float	score;
  if (Syn_Parse_IsOnlyRelativeW(w, lang)) return(0.0);

  if (Syn_ParseRightmostIsPronoun(w)) return(0.0);
  if (Syn_Parse_IsAdvShiftableToVP(w, y, lang)) return(0.0);
  if (!XBarSatisfiesArgRestrictions(w, W_MAX_NPS, W_MAX_PPS-1, W_MAX_ARGS-1)) {
    return(0.0);
  }
  if (LexEntryConceptIsAncestor(N("prep-of"), PNodeLeftmostLexEntry(y))) {
    /* See Hindle and Rooth (1993, p. 109)
     * "default preference with of for noun attachment" todoSCORE 
     */
    score = 0.8;
  } else {
    score = SCORE_MAX;
  }
  if (!PNodeEndGroupingPunc(w)) {
    return(ScoreCombine(score, ScoreCount(PNodeNumberOfWords(w), 8, 20)));
      /* todoSCORE */
  }
  return(score);
}

Float Syn_ParseFilterWX_W(PNode *w, PNode *x, int lang)
{
  LexEntry	*leftmost_le;
  if (Syn_Parse_IsOnlyRelativeW(w, lang)) return(0.0);

  if (Syn_ParseRightmostIsPronoun(w)) return(0.0);

/* todo: This prevents "Where is she?" from parsing.
  if (!XBarSatisfiesCaseFilter(x, NULL, N("obj"), lang)) return(0.0);
 */

  /* if (PNodeFeatureIn(w, F_ADJP)) return(0.0); still needed? */
  /* No PPs are allowed in w because of "NP arguments must come before PP
   * arguments": See (Pinker, 1994, p. 116).
   * Was W_MAX_PPS.
   */
  if (!XBarSatisfiesArgRestrictions(w, W_MAX_NPS-1, 0, W_MAX_ARGS-1)) {
    return(0.0);
  }
  if (Syn_Parse_IsAdvShiftableToVP(w, x, lang)) return(0.0);

  /* Heuristic rule for French to eliminate multiple parses involving
   * "pas" as noun when "pas" is really the adverb of negation.
   */
  if (lang == F_FRENCH &&
      (leftmost_le = PNodeLeftmostLexEntry(x)) &&
      LexEntryWordIs(leftmost_le, "pas", F_NOUN, F_FRENCH) &&
      PNodeWordIn(w, "ne", F_ADVERB, F_FRENCH) &&
      (!PNodeWordIn(w, "pas", F_ADVERB, F_FRENCH))) {
    return(0.0);
  }

  if (!PNodeEndGroupingPunc(w)) {
  /* todo: Is this right? For example, "She gave a special 567-page
   * 1996 edition of the user's manual for the interositor with betatron
   * integrator demodulator to her friend." seems fine.
   */
    return(ScoreCount(PNodeNumberOfWords(w), 8, 20));	/* todoSCORE */
  }

  return(1.0);
  /* Was (!(x->pn1 && x->pn1->feature == F_PRONOUN)). */
}

Float Syn_ParseFilterBW_W(PNode *b, PNode *w, int lang)
{
  return(Syn_ParseFilterBW_WB_W(b, w, 1, lang));
}

Float Syn_ParseFilterWB_W(PNode *w, PNode *b, int lang)
{
  return(Syn_ParseFilterBW_WB_W(b, w, 0, lang));
}

/* "Is he stupid?"
 * "Est-il stupide?"
 */
Bool Syn_ParseIsPronounVerbInversion(PNode *v, PNode *h, int lang)
{
  if (!LexEntryConceptIsAncestor(N("subject-pronoun"),
                                 PNodeLeftmostLexEntry(h))) {
    return(0);
  }
  if (lang == F_ENGLISH) {
    if (v->lexitem) {
      return(LexEntryIsEnglishInvertibleVerb(v->lexitem->le,
                                             v->lexitem->features));
    } else {
      Dbg(DBGSYNPAR, DBGBAD, "Syn_ParseIsPronounVerbInversion: empty lexitem");
      return(0);
    }
  } else {
    return(1);
  }
}

/* "Is John stupid?"
 * "Is he stupid?"
 * "Is the elephant stupid?"
 * *"Is him stupid?"
 * *"Est John stupid?"
 *
 * w: Is
 * x: the elephant
 */
Bool Syn_ParseIsNPVerbInversion(PNode *w, PNode *x, int lang)
{
  PNode	*h;
  if (lang == F_ENGLISH) {
    if (w->pn1 && w->pn1->feature == F_VERB && w->pn2 == NULL &&
        w->pn1->lexitem &&
        LexEntryIsEnglishInvertibleVerb(w->pn1->lexitem->le,
                                        w->pn1->lexitem->features)) {
      if (x->feature == F_PRONOUN) {
        h = x;
      } else if (x->pn1 && x->pn1->feature == F_PRONOUN && x->pn2 == NULL) {
        h = x->pn1;
      } else {
        h = NULL;
      }
      if (h && !LexEntryConceptIsAncestor(N("subject-pronoun"),
                                          PNodeLeftmostLexEntry(h))) {
        return(0);
      }
      return(1);
    }
  } else if (lang == F_FRENCH) {
  /* todoSCORE: This much higher if eos == '?'. Otherwise it is either a
   * stylistic inversion or there is a missing '?'.
   */
    return(1);
  }
  return(0);
}

Bool Syn_ParseIsNPVerbInversionVP(PNode *w, int lang)
{
  return(w->pn1 && w->pn1->feature == F_VP &&
         w->pn2 && (w->pn2->feature == F_NP ||
                    w->pn2->feature == F_PRONOUN) &&
         Syn_ParseIsNPVerbInversion(w->pn1, w->pn2, lang));
}

Bool Syn_ParseIsInversion(PNode *w, int lang)
{
  if (w->pn1 && w->pn1->feature == F_VP &&
      w->pn2 && w->pn2->feature == F_NP &&
      Syn_ParseIsNPVerbInversion(w->pn1, w->pn2, lang)) {
    return(1);
  }
  if (w->pn1 && w->pn1->feature == F_VP) {
    return(Syn_ParseIsInversion(w->pn1, lang));
  }
  return(0);
}

Bool Syn_Parse_IsVerbPhrasePronoun(PNode *h, int lang)
{
  /* That's right: There are no English verb phrase pronouns. */
  return(lang == F_FRENCH &&
         LexEntryConceptIsAncestor(N("verb-phrase-pronoun"),
                                   PNodeLeftmostLexEntry(h)));
}

Float Syn_ParseFilterWH_HW_W(PNode *w, PNode *h, int lang)
{
  if (!XBarSatisfiesArgRestrictions(w, 0, 0, 0)) return(0.0);
  if (!Syn_Parse_IsVerbPhrasePronoun(h, lang)) return(0.0);
  return(1.0);
}

Float Syn_ParseFilterWH_W(PNode *w, PNode *h, int lang)
{
  if (Syn_Parse_IsOnlyRelativeW(w, lang)) return(0.0);
  if (w->pn1 && w->pn1->feature == F_VERB && w->pn2 == NULL) {
    if (Syn_ParseIsPronounVerbInversion(w->pn1, h, lang)) return(1.0);
  }
  if (!Syn_ParseFilterWH_HW_W(w, h, lang)) return(0.0);
  return(1.0);
}

Float Syn_ParseFilterHW_W(PNode *h, PNode *w, int lang)
{
  if (Syn_Parse_IsOnlyRelativeW(w, lang)) return(0.0);
  if (LexEntryConceptIsAncestor(N("rel-pronoun-subj"),
                                PNodeLeftmostLexEntry(h))) {
  /* This is allowed here for use by XW_X. It is filtered from other uses
   * by Syn_Parse_IsOnlyRelativeW.
   */
    return(1.0);
  }
  if (!Syn_ParseFilterWH_HW_W(w, h, lang)) return(0.0);
  return(1.0);
}

Float Syn_ParseFilterWV_W(PNode *w, PNode *v, int lang)
{
  if (Syn_Parse_IsOnlyRelativeW(w, lang)) return(0.0);
  if (PNodeFeaturesIn(w, FS_NP_PP)) {
  /* Verb arguments should go at a higher level.
   * "The American who is Jim Garnier's sister, ate."
   */
    return(0.0);
  }
  return(1.0);
}

Float Syn_ParseFilterKX_X(PNode *k, PNode *x, int lang)
{
  if (!XBarValidX_MAX(x)) return(0.0);
  if (!LexEntryAnyConceptsFeat(F_COORDINATOR, PNodeLeftmostLexEntry(k))) {
    return(0.0);
  }
  return(1.0);
}

Bool Syn_ParsePresPartPrep(LexEntry *le)
{
  return(LexEntryConceptIsAncestor(N("prep-to"), le) ||
           /* "get accustomed to going" */
         LexEntryConceptIsAncestor(N("prep-at"), le) ||
           /* "succeed at going" */
         LexEntryConceptIsAncestor(N("prep-grammatical-for"), le) ||
           /* "prepare for going" */
         LexEntryConceptIsAncestor(N("prep-toward"), le));
           /* "lean toward going" */
  /* What preposition really ISN'T acceptable here? */
}

Float Syn_ParseFilterRX_Y(PNode *r, PNode *x, int lang)
{
  PNode	*v_pn;
  if (x->pn1 && x->pn1->feature == F_S) {
    if (lang == F_FRENCH) {
      if (LexEntryConceptIsAncestor(N("conj-pp-subord"),
                                    PNodeLeftmostLexEntry(x->pn1)) &&
          LexEntryConceptIsAncestor(N("prep-to"), PNodeLeftmostLexEntry(r))) {
      /* "à ce que" + indic */
        return(1.0);
      } else if (LexEntryConceptIsAncestor(N("prep-to"),
                                           PNodeLeftmostLexEntry(r))) {
      /* "à" + inf todo: Enforce infinitive? Already enforced by
       * Syn_ParseFilterZ_X.
       */
        return(1.0);
      } else if (LexEntryConceptIsAncestor(N("prep-of"),
                                           PNodeLeftmostLexEntry(r))) {
      /* "de" + inf */
        return(1.0);
      }
    } else {
      if (Syn_ParsePresPartPrep(PNodeLeftmostLexEntry(r)) &&
          (v_pn = PNodeLeftmost(x)) && v_pn->lexitem && 
          (F_PRESENT_PARTICIPLE ==
             FeatureGet(v_pn->lexitem->features, FT_TENSE))) {
      /* "to" + pres part */
        return(1.0);
      } else {
        return(0.0);
      }
    }
  } else {
    if (!XBarSatisfiesCaseFilter(x, NULL, N("iobj"), lang)) return(0.0);
  }
  return(1.0);
}

Float Syn_ParseFilterRB_Y(PNode *r, PNode *b, int lang)
{
  /* "rendez-vous de dix heures" cf tsrobj */
  if (b->type != PNTYPE_TSRANGE) return(0.0);
  return(1.0);
}

Float Syn_ParseFilterKY_Y(PNode *k, PNode *y, int lang)
{
  if (!LexEntryConceptIsAncestor(N("and"), /* todo: Allow other coord conjs? */
                                 PNodeLeftmostLexEntry(k))) {
    return(0.0);
  }
  return(1.0);
}

Float Syn_ParseFilterXY_X(PNode *x, PNode *y, int lang)
{
  if (!XBarValidX_MAX(x)) return(0.0);
  if (x->pn1 &&
      x->pn1->feature == F_S &&
      (!LexEntryConceptIsAncestor(N("standard-subordinating-conjunction"),
                                  PNodeLeftmostLexEntry(x->pn1)))) {
  /* todo: Not in English: "the man I gave my watch to". */
    return(0.0);
  }
  if (!XBarSatisfiesArgRestrictions(x, X_MAX_NPS, X_MAX_PPS-1, X_MAX_ARGS-1)) {
    return(0.0);
  }
  if (PNodeFeatureIn(x, F_S) || PNodeFeatureIn(x, F_VP)) {
  /* If <x> contains an S or VP, this is unlikely, since the PP should probably
   * be attached inside the S or VP, not here.
   * "la pause de dix heures"
   * x: "Emmanuel qui était en train de préparer la carriole"
   * y: "dans la cour"
   */
    return(0.1);	/* todoSCORE */
  }
  return(1.0);
}

/* todo: This might be deletable once Sem_Parse is more merged with
 * Syn_Parse.
 */
Bool Syn_ParseIsXX_X_NotApposition(PNode *x1, PNode *x2, int lang)
{
  if (LexEntryAnyConceptsFeat(F_COORDINATOR, PNodeLeftmostLexEntry(x2))) {
    return(1);
  }
  if (LexEntryConceptIsAncestor(N("simple-demonstrative-pronoun"),
                                PNodeSingletonLexEntry(x2))) {
    return(1);
  }
  if (lang == F_ENGLISH &&
      LexEntryConceptIsAncestor(N("genitive-element"),
                                PNodeRightmostLexEntry(x1))) {
    return(1);
  }
  return(0);
}

int Syn_Parse_RuleAppCount(PNode *pn, int lhs1, int lhs2, int rhs)
{
  if (pn == NULL) return(0);
  if (pn->feature == rhs &&
      pn->pn1 && pn->pn1->feature == lhs1 &&
      pn->pn2 && pn->pn2->feature == lhs2) {
    return(1 + Syn_Parse_RuleAppCount(pn->pn1, lhs1, lhs2, rhs) +
               Syn_Parse_RuleAppCount(pn->pn2, lhs1, lhs2, rhs));
  }
  return(0);
}

/* "l'épicière, elle"
 * Returns:
 * 0: is illegal pronoun appositive
 * 1: is not pronoun appositive
 * 2: is legal pronoun appositive
 */
int Syn_ParseFilter_PronounAppositive(PNode *x, int lang)
{
  if (x->pn1 && x->pn1->feature == F_PRONOUN && x->pn2 == NULL) {
    if (lang != F_FRENCH ||
        (!LexEntryConceptIsAncestor(N("disjunctive-pronoun"),
                                   PNodeLeftmostLexEntry(x->pn1)))) {
    /* Allow "l'épicière, elle" in French. (Also sometimes in English?) */
      return(0);
    }
    return(2);
  }
  return(1);
}

Float Syn_ParseFilterKE_E(PNode *k, PNode *e, int lang)
{
  if (!XBarValidE_MAX(e)) return(0.0);
  if (!LexEntryAnyConceptsFeat(F_COORDINATOR, PNodeLeftmostLexEntry(k))) {
    return(0.0);
  }
  return(1.0);
}

Float Syn_ParseFilterEE_E(PNode *e1, PNode *e2, int lang)
{
  if (e2->pn1 && e2->pn1->feature == F_CONJUNCTION) return(1.0);
  return(0.0);
}

/* The philosophy regarding appositives is to handle as many of them as
 * possible using specialized TAs. These can really explode parse
 * alternatives otherwise.
 */
Float Syn_ParseFilterXX_X(PNode *x1, PNode *x2, int lang)
{
  int	pa1, pa2, len1, len2;

  if (x2->pn1 && x2->pn1->feature == F_CONJUNCTION) return(1.0);

  if (Syn_ParseIsNP_Genitive(x1, lang)) {
    return(1.0);
  }

  if (0 == (pa1 = Syn_ParseFilter_PronounAppositive(x1, lang))) {
    return(0.0);
  }
  if (0 == (pa2 = Syn_ParseFilter_PronounAppositive(x2, lang))) {
    return(0.0);
  }

  if (x2->pn1 && x2->pn1->feature == F_NP &&
      x2->pn1->pn1 && x2->pn1->pn1->feature == F_PRONOUN) {
  /* Disallow something resembling a relative clause inside an appositive NP:
   * this should be attached at a higher level.
   */
    return(0.0);
  }
  if (x1->pn1 && x1->pn1->feature == F_NP &&
      x1->pn1->pn1 && x1->pn1->pn1->feature == F_PRONOUN) {
    return(0.0);
  }

  /* Enforce right branching: eliminates redundant parses due to
   * binary nature of parse trees.
   */
  if (Syn_Parse_RuleAppCount(x1, F_NP, F_NP, F_NP) > 0) return(0.0);

  /* Appositives of more than 2 elements are unlikely. */
  if (Syn_Parse_RuleAppCount(x2, F_NP, F_NP, F_NP) >= 2) return(0.1);
    /* todoSCORE */

  /* Some additional filtering must be done by Sem_Parse. */

  if (PNodeFeatureIn(x1, F_S)) {
  /* x1: "à peine de quoi remplir..." x2: "le reste" is rejected, but
   * x1: "le reste ---" x2: "à peine de quoi remplir..." is accepted.
   */
    return(0.0);
  }
  if (PNodeIsProperNP(x1) &&
      PNodeIsProperNP(x2)) {
  /* Filter out "Wiesengasse Mme Püchl" (wrong). */
    return(0.0);
  }
  len1 = PNodeNumberOfTerminals(x1);
  len2 = PNodeNumberOfTerminals(x2);
  if (pa1 != 2 && pa2 != 2 && len1 == 1) {
    if (len2 == 1) {
    /* Filter out "Tante Lucie" (handled by TA_Name).
     * All word-word appositives should be handled by TA's.
     * Note that compound nouns in English (and French for that matter)
     * are considered separate from appositives and should be handled either
     * by entering the compound noun as a phrase into the lexicon, or by a
     * special compound noun TA yet to be written.
     */
      return(0.0);
    } else if (x2->pn1 && x2->pn1->feature == F_NP &&
               x2->pn1->pn1 && x2->pn1->pn1->feature == F_NOUN) {
    /* Filter out "Tante [Lucie ...]" */
      return(0.0);
    }
  }
  if (len1 >= 4 || len2 >= 4) {
  /* Long appositive */
    if ((PNodeEndGroupingPunc(x1) || PNodeIsProperNP(x1)) &&
        (PNodeEndGroupingPunc(x2) || PNodeIsProperNP(x2))) {
    /* Long appositive is set off with punctuation, so it passes. */
    /* todo: Proper NPs need to keep punctuation too. */
    } else {
      return(0.1);	/* todoSCORE */
    }
  }
  return(1.0);
}

Float Syn_ParseFilterX9_X(PNode *x, PNode *element, int lang)
{
  if (lang != F_ENGLISH) return(0.0);
  if (!XBarValidX_MAX(x)) return(0.0);
  if (!LexEntryConceptIsAncestor(N("genitive-element"),
                                 PNodeLeftmostLexEntry(element))) {
    return(0.0);
  }
  return(1.0);
}

Float Syn_ParseFilterYY_Y(PNode *y1, PNode *y2, int lang)
{
  if (!LexEntryAnyConceptsFeat(F_COORDINATOR, PNodeLeftmostLexEntry(y2))) {
    return(0.0);
  }
  return(1.0);
}

Bool Syn_ParseFilter_IsPrepRel(PNode *y)
{
  return(y->pn1 && y->pn1->feature == F_PREPOSITION &&
         y->pn2 && y->pn2->feature == F_NP &&
         y->pn2->pn1 && y->pn2->pn1->feature == F_PRONOUN &&
         LexEntryConceptIsAncestor(N("rel-pronoun-iobj"),
                                   PNodeLeftmostLexEntry(y->pn2->pn1)));
}

Bool Syn_ParseIsNonNominalRelativeZ(PNode *z)
{
  if (z->pn2 == NULL || z->pn2->feature != F_S) return(0);
  if (z->pn1 && z->pn1->feature == F_PRONOUN &&
      LexEntryConceptIsAncestor(N("rel-pronoun-obj"), PNodeLeftmostLexEntry(z))) {
  /* x:La bouteille z:[que [Mme Püchl tenait en main]] */
    return(1);
  }
  if (z->pn1 && z->pn1->feature == F_PP && Syn_ParseFilter_IsPrepRel(z->pn1)) {
  /* x:Les amis z:[[avec qui] [vous parlez francais]] */
    return(1);
  }
  return(0);
}

/* Two cases:
 * nominal relative clauses
 * nonnominal relative clauses
 */
Float Syn_ParseFilterXZ_X(PNode *x, PNode *z, int lang)
{
  LexEntry	*pronoun_le;
  if (Syn_ParseFilter_IsXE(z)) return(0.0);

  if (x->pn1 && x->pn1->feature == F_PRONOUN && x->pn2 == NULL &&
      (pronoun_le = PNodeLeftmostLexEntry(x)) &&
      (LexEntryConceptIsAncestor(N("nominal-rel-pronoun-obj"), pronoun_le) ||
       LexEntryConceptIsAncestor(N("nominal-rel-pronoun-iobj"), pronoun_le))) {
  /* Nominal relative clause. */
    return(1.0);
  } else if (Syn_ParseIsNonNominalRelativeZ(z)) {
  /* Nonnominal relative clause. */
    if (XBarValidX_MAX(x)) return(1.0);
  }
  return(0.0);
}

Bool Syn_ParseIsCompleteSentence(PNode *z, int lang)
{
  return((z->pn1 && z->pn1->feature == F_NP &&
          z->pn2 && z->pn2->feature == F_VP) ||
         (z->pn1 && z->pn1->feature == F_VP &&
          z->pn2 == NULL && 
          Syn_ParseIsInversion(z->pn1, lang)));
}

Float Syn_ParseFilterXZ_Z(PNode *x, PNode *z, int lang)
{
  LexEntry	*le;
  if (Syn_ParseFilter_IsXE(z)) return(0.0);
  if (Syn_ParseIsCompleteSentence(z, lang) && x->pn1) {
    if (x->pn1->feature == F_PRONOUN && x->pn2 == NULL &&
        (le = PNodeLeftmostLexEntry(x->pn1))) {
      if (LexEntryConceptIsAncestor(N("interrogative-pronoun"), le)) {
      /* interrogative pronoun + Sentence:
       * interrogative pronoun will be an "obj" argument
       * "Où Bonaparte est-il né?"
       * "Qu'est-ce que vous avez achetez?"
       * "What did you give your sister?"
       */
        return(1.0);
      }
      if (LexEntryConceptIsAncestor(N("rel-pronoun-obj"), le) ||
          LexEntryConceptIsAncestor(N("rel-pronoun-iobj-preposition-built-in"),
                                    le)) {
      /* relative pronoun:
       * [la bouteille] x:[X [H que]] z:[Z Mme Püchl tenait en reserve]
       */
        return(1.0);
      }
    }
    if (LexEntryConceptIsAncestor(N("interrogative-determiner"),
                                  PNodeLeftmostLexEntry(x))) {
    /* "Which song did you send to the record company?" */
      return(1.0);
    }
  }
  return(0.0);
}

Bool Syn_Parse_IsOnlyRelativeW(PNode *w, int lang)
{
  return(w->pn1 && w->pn1->feature == F_PRONOUN &&
         w->pn2 && w->pn2->feature == F_VP &&
         (!Syn_Parse_IsVerbPhrasePronoun(w->pn1, lang)));
}

Float Syn_ParseFilterXW_X(PNode *x, PNode *w, int lang)
{
  if (w->pn1 && w->pn1->feature == F_PRONOUN &&
      w->pn2 && w->pn2->feature == F_VP &&
      LexEntryConceptIsAncestor(N("rel-pronoun-subj"),
                                PNodeLeftmostLexEntry(w->pn1))) {
  /* x:[X femme] w:[W [H qui] [W ...]] */
    if (PNodeEndGroupingPunc(x) && (!PNodeEndGroupingPunc(w))) {
    /* If grouping punctuation indicates beginning of clause, the same
     * puncuation should also occur at the end.
     * todo: Grouping punctuation should be same type (, --- etc.)
     * Disallow x:[X [N <Pierre.SFNy¸:pierre><, >]]
     *          w:[W [H <qui.Hy¸>] [W [V <était en train de>]]]
     *      [préparer...]
     */
      return(0.1);  /* todoSCORE */
    }
    return(1.0);
  }
  if (x->pn1 && x->pn1->feature == F_NP &&
      x->pn2 == NULL &&
        x->pn1->pn1 && x->pn1->pn1->feature == F_PRONOUN &&
        x->pn1->pn2 == NULL &&
        LexEntryConceptIsAncestor(N("nominal-rel-pronoun-subj"),
                                  PNodeLeftmostLexEntry(x->pn1->pn1))) {
  /* x:[X [H what]] w:[W really appeals to me] */
    return(1.0);
  }
  return(0.0);
}

/* [Z [H Quoi] [X [D un] [X [N chat]]]] */
Float Syn_ParseFilterHX_Z(PNode *h, PNode *x, int lang)
{
  if (!LexEntryConceptIsAncestor(N("object-interrogative-pronoun"),
                                 PNodeLeftmostLexEntry(h))) {
    return(0.0);
  }
  return(1.0);
}

/* "le ciel couvert": an elipsis */
Float Syn_ParseFilterXE_Z(PNode *x, PNode *e, int lang)
{
  return(1.0);
}

Float Syn_ParseFilterXW_Z(PNode *x, PNode *w, int lang)
{
  int	noun_gender, noun_number, noun_person, verb_tense;
  Obj	*cas;
  PNode	*auxverb, *mainverb;

  if (Syn_Parse_IsOnlyRelativeW(w, lang)) return(0.0);

  verb_tense = F_NULL;
  PNodeFindHeadVerb(w, &auxverb, &mainverb);
  if (auxverb && auxverb->lexitem &&
      (verb_tense = FeatureGet(auxverb->lexitem->features, FT_TENSE)) &&
      (!StringIn(verb_tense, FS_FINITE_TENSE))) {
  /* Subjects of nonfinite verbs are in objective case.
   * See Chomsky (1982/1987, p. 207).
   */
    cas = N("obj");
  } else {
    cas = N("subj");
  }
  if (!XBarSatisfiesCaseFilter(x, NULL, cas, lang)) return(0.0);

  if (x->pn1 && x->pn1->feature == F_S && x->pn2 == NULL &&
      x->pn1->pn1 && x->pn1->pn1->feature == F_VP && x->pn1->pn2 == NULL &&
      x->pn1->pn1->pn1 && x->pn1->pn1->pn1->feature == F_VERB &&
        x->pn1->pn1->pn2 == NULL) {
  /* The case of an infinitive subject. [X [Z [W [V <garder.fVy¸>]]]]
   * "Aimer"
   * This is indeed possible, but it is causing a lot of extra parses.
   * "Aimer quelqu'un" is allowed.
   */
    return(0.1);	/* todoSCORE */
  }

  /* Subject-verb agreement check. */
  if (auxverb == NULL) return(1.0);
  if (auxverb->lexitem == NULL) return(1.0);
  if (F_IMPERATIVE == FeatureGet(auxverb->lexitem->features, FT_MOOD)) {
  /* Imperative with subject. */
    return(0.2);	/* todoSCORE */
  }
  if (!PNodeGetHeadNounFeatures(x, 0, &noun_gender, &noun_number,
                                &noun_person)) {
    return(1.0);
  }
  if (x->pn1 && x->pn2 &&
      x->pn1->feature == F_NP && x->pn2->feature == F_NP &&
      x->pn2->pn1 && x->pn2->pn1->feature == F_CONJUNCTION) {
    return(1.0); /* todo: Real conjunction agreement rules? */
  }
#ifdef notdef
  /* Seems unnecessary in light of FeatureMatch below */
  if (F_NULL == noun_person && F_NULL == noun_number) {
    /* todoSCORE: This is too relaxed? In any case it allows parsing of
     * Où étais (sic) mon pied gauche ?
     * Où étaient mes... ?
     * When would noun_number be F_NULL?
     */
    return(0.6);	/* todoSCORE */
  }
#endif
  if (F_NULL == noun_person) {
  /* This is necessary to rule out "I am" where "I" = "isospin". */
    noun_person = F_THIRD_PERSON;
  }
  if (FeatureMatch(noun_number,
                   FeatureGet(auxverb->lexitem->features, FT_NUMBER)) &&
      FeatureMatch(noun_person,
                   FeatureGet(auxverb->lexitem->features, FT_PERSON))) {
    return(1.0);
  }
  if (Syn_ParseIsNPVerbInversionVP(w, lang)) {
  /* "What color are elephants? */
    return(1.0);
  }
  return(0.0);
}

Float Syn_ParseFilterYZ_Z(PNode *y, PNode *z, int lang)
{
  if (Syn_ParseFilter_IsXE(z)) return(0.0);

  if (Syn_ParseFilter_IsPrepRel(y)) {
  /* y:[avec qui] z:[vous parlez francais] */
    return(1.0);
  }

  if (StringIn(',', PNodeGetEndPunc(y))) {
  /* "par précaution," */
    return(1.0);
  }

  if ((PNodeClassIn(y, N("interrogative-pronoun")) ||
       PNodeClassIn(y, N("interrogative-determiner"))) &&
      Syn_ParseIsCompleteSentence(z, lang)) {
  /* "De quel instrument jouait Paganini?" */
    return(1.0);
  }

  return(0.0);
}

/* "How tall is she?" */
Float Syn_ParseFilterEW_Z(PNode *y, PNode *w, int lang)
{
  if (Syn_Parse_IsOnlyRelativeW(w, lang)) return(0.0);
  if (!PNodeClassIn(y, N("interrogative-adverb"))) return(0.0);
  if (!Syn_ParseIsNPVerbInversionVP(w, lang)) return(0.0);
  return(1.0);
}

/* Jim's */
Bool Syn_ParseIsNP_Genitive(PNode *pn, int lang)
{
  return(lang == F_ENGLISH &&
         pn->pn1 && pn->pn1->feature == F_NP &&
         pn->pn2 && pn->pn2->feature == F_ELEMENT &&
         LexEntryConceptIsAncestor(N("genitive-element"),
                                   PNodeLeftmostLexEntry(pn->pn2)));
}

/* Jim's stereo */
Bool Syn_ParseIsNPNP_Genitive(PNode *pn, int lang)
{
  return(lang == F_ENGLISH && pn->pn1 &&
         Syn_ParseIsNP_Genitive(pn->pn1, lang));
}

Bool Syn_ParseIsNPPP_Genitive(PNode *pn)
{
  return(pn->pn2->pn1->feature == F_PREPOSITION &&
         pn->pn2->pn2->feature == F_NP &&
         LexEntryConceptIsAncestor(N("prep-of"),
                                   PNodeLeftmostLexEntry(pn->pn2->pn1)));
}

Bool Syn_ParseIsSimpleAdjP(PNode *e)
{
  return(e->pn1 && e->pn1->feature == F_ADJECTIVE &&
         e->pn2 == NULL);
}

/* For "adjectivals".
 * [E Née le 16 mars 1939 à Bruxelles,] [Z Christine est diplômée de X]
 */
Float Syn_ParseFilterEZ_Z(PNode *e, PNode *z, int lang)
{
  if (!StringIn(',', PNodeGetEndPunc(e))) return(0.0);	/* require punc */
  if (Syn_ParseIsSimpleAdjP(e)) return(0.0);	/* require some complexity */
  return(1.0);
}

/* All conjunctions usable at the sentential level MUST be marked
 * with an FT_SUBCAT. Those that can't (such as "well at least")
 * aren't.
 */
Bool Syn_ParseZZ_ZConjunctionOK(PNode *pn)
{
  LexEntryToObj	*leo;
  if (pn == NULL) return(0);
  if (pn->lexitem == NULL) return(0);
  if (pn->lexitem->le == NULL) return(0);
  if (pn->lexitem->le->leo == NULL) return(0);
  for (leo = pn->lexitem->le->leo; leo; leo = leo->next) {
    if (F_NULL != ThetaRoleGetAnySubcat(leo->theta_roles)) return(1);
  }
  return(0);
}

Float Syn_ParseFilterZZ_Z(PNode *z1, PNode *z2, int lang)
{
  PNode	*pn;
  if (z1->pn1 && z1->pn1->feature == F_INTERJECTION &&
      z1->pn2 == NULL) {
    /* "What, are you thinking?" ACCEPT THIS PARSE vs.
     * "What are you thinking" REJECT THIS PARSE
     */
    if (StringIn(',', PNodeGetEndPunc(z1))) return(1.0);
    else return(0.0);
  }

  if ((pn = PNodeLeftmost(z1)) &&
      (pn->feature == F_CONJUNCTION)) {
    if (!Syn_ParseZZ_ZConjunctionOK(pn)) {
      return(0.0);
    } else if (pn->lexitem && pn->lexitem->le &&
               LexEntryAllConceptsFeat(F_CLAUSE2_ONLY, pn->lexitem->le)) {
      return(0.0);
    } else {
      return(1.0);
    }
  }
  if ((pn = PNodeLeftmost(z2)) &&
      (pn->feature == F_CONJUNCTION)) {
    if (!Syn_ParseZZ_ZConjunctionOK(pn)) {
      return(0.0);
    } else {
      return(1.0);
    }
  }

  if (StringIn(',', PNodeGetEndPunc(z1))) {
  /* Allow without conjunction if comma is used. In French these are frequent.
   * In English these are frowned upon "run-on sentences":
   * "The air was cold and humid, I hate that."
   * However if there is an elipsis, this is not considered a run-on sentence:
   * "The air was cold and humid, the sky overcast."
   */
    if (PNodeNumberOfWords(z1) >= 5 &&
        PNodeNumberOfWords(z2) <= 1) {
      /* Disallow if too lopsided:
       * "The American who is Jim Garnier's sister, ate."
       * todoSCORE
       */
    } else {
      return(1.0);
    }
  }
  /* Disallow. */
  return(0.0);
}

/* See Greenbaum and Quirk (1990, chapter 8) on adverbials. */

Float Syn_ParseFilterBZ_Z(PNode *b, PNode *z, int lang)
{
  LexEntry	*adv_le;
  if ((adv_le = PNodeLeftmostLexEntry(b))) {
    if (LexEntryAllConceptsFeat(F_NO_BZ_Z, adv_le)) {
      /* Disallow intensifiers "very"/"très", disallow "ne", and so on. */
      return(0.0);
    }
  }
  return(1.0);
}

Float Syn_ParseFilterZB_Z(PNode *z, PNode *b, int lang)
{
  LexEntry	*adv_le;
  if (PNodeKIsLeftmost(z)) return(0.0);
  if ((adv_le = PNodeLeftmostLexEntry(b))) {
    if (LexEntryAllConceptsFeat(F_NO_ZB_Z, adv_le)) {
      /* Disallow intensifiers "very"/"très", disallow "ne", and so on. */
      return(0.0);
    }
  }
  return(1.0);
}

Float Syn_ParseFilterH_X(PNode *h, int lang)
{
  LexEntry	*le;
  le = PNodeLeftmostLexEntry(h);
  if ((!LexEntryConceptIsAncestor(N("noun-phrase-pronoun"), le)) &&
      (!LexEntryConceptIsAncestor(N("nominal-relative-pronoun"), le))) {
    return(0.0);
  }
  return(1.0);
}

Float Syn_ParseFilterW_Z(PNode *w, int lang)
{
  LexEntry	*le_rightmost, *le_leftmost;
  Lexitem	*lexitem_verb;
  if (Syn_Parse_IsOnlyRelativeW(w, lang)) return(0.0);
  le_rightmost = PNodeRightmostLexEntry(w);
  if (LexEntryConceptIsAncestor(N("F72"), le_rightmost)) {
    if ((lexitem_verb = PNodeFindAuxverbLexitem(w)) &&
        (F_IMPERATIVE != FeatureGet(lexitem_verb->features, FT_MOOD))) {
    /* Rule out [Z [W [W garder] [H le]]]
     * Allow [Z [W [W gardez] [H le]]]
     */
      return(0.0);
    }
  }
  le_leftmost = PNodeLeftmostLexEntry(w);
  if (le_leftmost &&
      (F_PREPOSITION == FeatureGet(le_leftmost->features, FT_POS)) &&
      (!LexEntryConceptIsAncestor(N("prep-to"), le_leftmost))) {
  /* "Chirac succeeded at being elected President." (reject here)
   * but "Peter wants to eat." (accept here)
   */
    return(0.0);
  }
  return(1.0);
}

Float Syn_ParseFilterZ_X(PNode *z, int lang)
{
  int		tense;
  LexEntry	*le;
  PNode	*pn;
  if (Syn_ParseFilter_IsXE(z)) return(0.0);
  le = PNodeLeftmostLexEntry(z);
  if (le &&
      (F_CONJUNCTION == FeatureGet(le->features, FT_POS)) &&
      LexEntryConceptIsAncestor(N("standard-subordinating-conjunction"), le)) {
  /* Was N("subordinating-conjunction"). */
    return(1.0);
  }
  /* todo: Recode the below using full Tense parsing, which will be more
   * reliable?
   */
  if ((pn = PNodeLeftmostFeat(z, F_VERB)) && pn->lexitem) {
    tense = FeatureGet(pn->lexitem->features, FT_TENSE);
    if (F_INFINITIVE == tense ||
        (lang == F_ENGLISH && F_PRESENT_PARTICIPLE == tense)) {
      return(1.0);
    } else {
      return(0.0);
    }
  }
  return(0.0);
}

/* End of file. */
