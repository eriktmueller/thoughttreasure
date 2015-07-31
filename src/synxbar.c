/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Notions from Chomsky's X-bar theory (Jackendoff, 1977; Chomsky, 1986).
 *
 * ThoughtTreasure doesn't explicitly represent the difference between X'
 * and X'' but this can be inferred. It might help to explicitly represent
 * this in order to distinguish adjuncts from arguments.
 * 
 * 19950316: begun
 */

#include "tt.h"
#include "lexentry.h"
#include "lextheta.h"
#include "repbasic.h"
#include "repobj.h"
#include "repstr.h"
#include "semtense.h"
#include "synfilt.h"
#include "synparse.h"
#include "synpnode.h"
#include "synxbar.h"
#include "utildbg.h"

/******************************************************************************
 * Infer X-bar categories.
 ******************************************************************************/

Bool XBarCanBeRank1Or2Projection(PNode *pn)
{
  return(StringIn(pn->feature, FT_CONSTIT));
}

/* Used after parse complete. */
Bool XBarCanBeRank2Projection(PNode *pn, PNode *pn_parent)
{
  return(pn_parent == NULL || pn->feature != pn_parent->feature);
}

/* Used during parsing when parent is not available. */
Bool XBarValidY_MAX(PNode *y)
{
  LexEntry	*le;
  le = PNodeLeftmostLexEntry(y);
  if (le && F_PREPOSITION == FeatureGet(le->features, FT_POS)) {
    return(1);
  }
  if (y->pn1 && y->pn1->feature == F_ADVERB &&
      y->pn2 && y->pn2->feature == F_PP) {
    return(XBarValidY_MAX(y->pn2));
  }
  return(0);
}

/* Used during parsing when parent is not available. */
Bool XBarValidX_MAX(PNode *x)
{
  LexEntry	*le;
  if ((le = PNodeLeftmostLexEntry(x)) &&
      StringIn(F_CONJUNCTION, le->features)) {
    return(0);
  }
  if (x->pn1 && x->pn1->feature == F_PRONOUN && x->pn2 == NULL) {
  /* Reject [Z [X [H <à peine de quoi.Hy>]] [W [W [V <remplir.fVy>]]]] */
    if (!LexEntryConceptIsAncestor(N("noun-phrase-pronoun"),
                                   PNodeLeftmostLexEntry(x))) {
      return(0);
    }
  }
  return(1);
}

/* Used during parsing when parent is not available. */
Bool XBarValidE_MAX(PNode *e)
{
  LexEntry	*le;
  if ((le = PNodeLeftmostLexEntry(e)) &&
      StringIn(F_CONJUNCTION, le->features)) {
    return(0);
  }
  return(1);
}

/* Used during parsing when parent is not available. */
Bool XBarValidW_MAX(PNode *w, int lang)
{
  if (w == NULL) return(1);
  if (w && w->feature == F_VP && PNodeIsCompoundTense(w)) {
  /* todo: Inefficent. Reimplement by doing recursion here analogous
   * to Sem_ParseVP.
   */
    return(TenseParseIsTopCompTenseOK(w, lang));
  }
  return(XBarValidW_MAX(w->pn1, lang) && XBarValidW_MAX(w->pn2, lang));
}

/* Used whenever. */
Bool XBarIsRank0Projection(PNode *pn)
{
  return(StringIn(pn->feature, FT_POS));
}

/* Used after parse complete. */
Bool XBarIsRank1Projection(PNode *pn, PNode *pn_parent)
{
  return(XBarCanBeRank1Or2Projection(pn) &&
         (!XBarCanBeRank2Projection(pn, pn_parent)));
}

/* Used after parse complete. */
Bool XBarIsRank2Projection(PNode *pn, PNode *pn_parent)
{
  return(XBarCanBeRank1Or2Projection(pn) &&
         XBarCanBeRank2Projection(pn, pn_parent));
}

/* Used whenever.
 * todo: Must a barrier be of a specific type?
 * But this would seem wrong for XBarSatisfiesCaseFilter.
 */
Bool XBarIsBarrier(PNode *pn, PNode *pn_parent)
{
  return(XBarIsRank2Projection(pn, pn_parent));
}

/******************************************************************************
 * Tree relationships.
 ******************************************************************************/

/* todo: Make this more efficient by storing parents in PNodes. */
PNode *XBarParent(PNode *pn, PNode *root)
{
  PNode	*r;
  if (root == NULL) return(NULL);
  if (root->pn1 == pn) return(root);
  if (root->pn2 == pn) return(root);
  if ((r = XBarParent(pn, root->pn1))) return(r);
  if ((r = XBarParent(pn, root->pn2))) return(r);
  return(NULL);
}

PNode *XBarImproperAncestorNP(PNode *pn, PNode *root)
{
  if (pn == NULL) return(NULL);
  if (pn->feature == F_NP) return(pn);
  return(XBarImproperAncestorNP(XBarParent(pn, root), root));
}

/* Whether pn1 is an improper ancestor of pn2. */
Bool XBarDominates(PNode *pn1, PNode *pn2)
{
  if (pn1 == NULL) return(0);
  if (pn1 == pn2) return(1);
  return(XBarDominates(pn1->pn1, pn2) ||
         XBarDominates(pn1->pn2, pn2));
}

/* Depth-first search. */
Bool XBarBarrierBetween1(PNode *pn1, PNode *pn2, PNode *root, int barrier_seen)
{
  if (pn1 == NULL) return(0);
  if (pn1 == pn2) return(barrier_seen);
  if (XBarIsBarrier(pn1, XBarParent(pn1, root))) barrier_seen = 1;
  if (XBarBarrierBetween1(pn1->pn1, pn2, root, barrier_seen)) return(1);
  if (XBarBarrierBetween1(pn1->pn2, pn2, root, barrier_seen)) return(1);
  return(0);
}

/* Assumes pn1 is an ancestor of pn2. */
Bool XBarBarrierBetween(PNode *pn1, PNode *pn2, PNode *root)
{
  if (XBarBarrierBetween1(pn1->pn1, pn2, root, 0)) return(1);
  if (XBarBarrierBetween1(pn1->pn2, pn2, root, 0)) return(1);
  return(0);
}

/* Example: P governs a NP contained in the PP. */
Bool XBarCCommands(PNode *pn1, PNode *pn2, PNode *root,
                   /* RESULTS */ PNode **pn1_parent)
{
  if (XBarDominates(pn1, pn2)) return(0);
  if (XBarDominates(pn2, pn1)) return(0);
  return((*pn1_parent = XBarParent(pn1, root)) &&
         XBarDominates(*pn1_parent, pn2));
}

/* Example: P governs a NP contained in the PP not separated by a barrier. */
Bool XBarGoverns(PNode *pn1, PNode *pn2, PNode *root)
{
  PNode	*pn1_parent;
  return(XBarIsRank0Projection(pn1) &&
         XBarCCommands(pn1, pn2, root, &pn1_parent) &&
         (!XBarBarrierBetween(pn1_parent, pn2, root)));
}

/* Assumes the appropriate part of speech c-commands pn1.
 * Example:
 * pn1: [N him]               [N je]         [N lui]
 * case: N("obj")             N("subj")      N("iobj")
 * lang: F_ENGLISH            F_FRENCH       F_FRENCH
 * assumed: verb commands pn1 (from S)       preposition commands pn1
 * Called from Syn_Parse where the c-command is already known.
 */
Bool XBarSatisfiesCaseFilter(PNode *pn1, PNode *pn1_parent, Obj *cas, int lang)
{
  if (pn1 == NULL) return(1);
  if (pn1_parent && XBarIsBarrier(pn1, pn1_parent)) return(1);
  if (pn1->type == PNTYPE_LEXITEM) {
    return(LexEntrySatisfiesCaseFilter(cas, PNodeLexEntry(pn1), lang));
  }
  if (pn1->feature == F_NP &&
      pn1->pn1 && pn1->pn1->feature == F_NP &&
      pn1->pn2 && pn1->pn2->feature == F_NP &&
      pn1->pn2->pn1 && pn1->pn2->pn1->feature == F_CONJUNCTION &&
      pn1->pn2->pn2 && pn1->pn2->pn2->feature == F_NP) {
    /* Headless construction of an AND/OR coordinating conjunction.
     * Note this applies only to coordinated NPs and not to coordinated
     * PPs: "*to he and to she".
     */ 
    return(1);
  }
  if (!XBarSatisfiesCaseFilter(pn1->pn1, pn1, cas, lang)) return(0);
  if (!XBarSatisfiesCaseFilter(pn1->pn2, pn1, cas, lang)) return(0);
  return(1);
}

/* Example:
 * pn: [W [W [W put] [X the cup]] [Y [R in] [X the cupboard]]]
 * nps: 1
 * pps: 1
 * Returns: whether pn is a well-formed argument tree.
 * Examples to be considered if this is modified:
 * "Elle sortit vers sept heures de son magasin." but
 * "*She came out around 7 o'clock of her store." but
 * "She went around 7 o'clock to the store.".
 * If 0 is returned, then <pn> is not a well-formed argument tree. Thus
 * we should not add more arguments at this level in the tree. They should
 * be added at a lower level.
 */
Bool XBarCountArguments(PNode *pn, int max_nps, int max_pps, int max_total,
                        /* RESULTS */ int *nps, int *pps)
{
  int	nps_cnt, pps_cnt, constit;
  nps_cnt = pps_cnt = 0;
  constit = pn->feature;
  while (pn) {
    if (pn->pn1 && pn->pn1->feature == constit &&
        pn->pn2 && pn->pn2->feature == F_PP) {
      if (nps_cnt > 0) {
      /* NP arguments must come before PP arguments. todoSCORE
       * See (Pinker, 1994, p. 116).
       * Note the arguments are traversed in this function in reverse order.
       */
        return(0);
      }
      pps_cnt++;
      if (pps_cnt > max_pps || ((pps_cnt + nps_cnt) > max_total)) return(0);
      pn = pn->pn1;
    } else if (pn->pn1 && pn->pn1->feature == constit &&
               pn->pn2 && pn->pn2->feature == F_NP) {
      if (constit == F_NP) return(0);
      if (constit == F_ADJP) return(0);
        /* todo: Allow X-MAX argument to E-MAX. */
      nps_cnt++;
      if (nps_cnt > max_nps || ((pps_cnt + nps_cnt) > max_total)) return(0);
      pn = pn->pn1;
    } else if (pn->pn1 && pn->pn1->feature == F_VP &&
               pn->pn2 && pn->pn2->feature == F_EXPLETIVE) {
    /* todo: Should this count as a NP? */
      pn = pn->pn1;
    } else if (constit == F_VP && PNodeIsCompoundTense(pn)) {
      break;
    } else if (constit != F_NP &&
               pn->pn1 && pn->pn1->feature == F_ADVERB &&
               pn->pn2 && pn->pn2->feature == constit) {
      return(0);
    } else if (constit != F_NP &&
               pn->pn1 && pn->pn1->feature == constit &&
               pn->pn2 && pn->pn2->feature == F_ADVERB) {
      return(0);
    } else if (pn->feature == F_ADJP) {
      break;
    } else if (pn->feature == F_NP) {
      break;
    } else {
      return(0);
    }
  }
  if (nps) *nps = nps_cnt;
  if (pps) *pps = pps_cnt;
  return(1);
}

Bool XBarSatisfiesArgRestrictions(PNode *pn, int max_nps, int max_pps,
                                  int max_total)
{
  int	nps, pps;
  return(XBarCountArguments(pn, max_nps, max_pps, max_total, &nps, &pps));
}

Obj *XBarConstitToMax(int constit)
{
  switch (constit) {
    case F_NP:   return(N("X-MAX"));
    case F_VP:   return(N("W-MAX"));
    case F_ADJP: return(N("E-MAX"));
    case F_PP:   return(N("Y-MAX"));
    case F_S:    return(N("Z-MAX"));
    default:
      break;
  }
  return(N("W-MAX"));
}

Bool XBarIs_NP_S_Or_PP_NP_S(PNode *pn)
{
  if (pn->feature == F_NP && pn->pn1 && pn->pn1->feature == F_S) {
    return(1);
  } else if (pn->feature == F_PP && pn->pn2 && pn->pn2->feature == F_NP &&
             pn->pn2->pn1 && pn->pn2->pn1->feature == F_S) {
    return(1);
  }
  return(0);
}

Bool XBarIs_NP_S(PNode *pn)
{
  return(pn->feature == F_NP && pn->pn1 && pn->pn1->feature == F_S);
}

/* End of file. */
