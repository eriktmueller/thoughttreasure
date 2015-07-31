/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19941017: begun
 * 19941018: minor mods
 * 19941202: parsing transformations begun: inversion deletion
 * 19941207: more parsing
 * 19950511: Eliminated parse transformations. All possible structures
 *           should be handled by Sem_Parse.
 *
 * todo:
 * - English: plural genitive: wordS s -> wordS'
 * - French: je vois le et la -> je les vois
 * - French: S V Prep NP et NP -> S V Prep NP et Prep NP
 * - French: chez la maison de Lucie -> chez Lucie
 * - French: Z pronoun aller -> Z pronoun y aller
 * - French: je lui pense -> je pense à lui
 * - French: X t'est -> X est à toi
 * - French: a été né -> est né
 */

#include "tt.h"
#include "lexentry.h"
#include "lexitem.h"
#include "lexobjle.h"
#include "lextheta.h"
#include "repbasic.h"
#include "repobj.h"
#include "repstr.h"
#include "semdisc.h"
#include "semgen1.h"
#include "semgen2.h"
#include "synfilt.h"
#include "synparse.h"
#include "synpnode.h"
#include "synxbar.h"
#include "utildbg.h"

/* HELPING FUNCTIONS */

LexitemList *LexitemListAddPronoun(Lexitem *lexitem, LexitemList *ll)
{
  /* todo: Splice in pronoun in correct order. */
  return(LexitemListCreate(lexitem, ll));
}

void TransformGenAddPronoun(Obj *type, int gender, int number, int person,
                            Discourse *dc, /* RESULTS */ LexitemList **pronouns)
{
  LexEntry	*le;
  Word		*infl;
  Lexitem		*lexitem;
  if (!(le = ObjToLexEntryGet(type, F_PRONOUN, F_NULL, dc))) {
    Dbg(DBGGENER, DBGBAD, "TransformGenAddPronoun: pronoun not found");
    return;
  }
  if (!(infl = LexEntryGetInflection(le, F_NULL, gender, number, person,
                                     F_NULL, F_NULL, 1, dc))) {
    Dbg(DBGGENER, DBGBAD, "TransformGenAddPronoun: inflection not found");
    return;
  }
  lexitem = LexitemCreate(infl->word, le, infl->features);
  if (pronouns) *pronouns = LexitemListAddPronoun(lexitem, *pronouns);
  else Dbg(DBGGENER, DBGBAD, "TransformGenAddPronoun: NULL pronouns");
}

/* GENERATION TRANSFORMATIONS */

Lexitem *TransformPronoun(Lexitem *in, Obj *to_pronoun_class, Discourse *dc)
{
  LexEntry	*le;
  Word		*infl;
  int		gender, number, person;
  gender = FeatureGet(in->features, FT_GENDER);
  number = FeatureGet(in->features, FT_NUMBER);
  person = FeatureGet(in->features, FT_PERSON);
  if (!(le = ObjToLexEntryGet(to_pronoun_class, F_PRONOUN, F_NULL, dc))) {
    Dbg(DBGGENER, DBGBAD, "TransformPronoun: 1");
    return(in);
  }
  if (!(infl = LexEntryGetInflection(le, F_NULL, gender, number, person,
                                     F_NULL, F_NULL, 1, dc))) {
    Dbg(DBGGENER, DBGBAD, "TransformPronoun: 2");
    return(in);
  }
  return(LexitemCreate(infl->word, le, le->features));
}

/* todo: Don't do the transformation if an interrogative inside the PP. */
Bool TransformGenFrenchPPElim(PNode *pn, Discourse *dc,
                              /* RESULTS */ LexitemList **pronouns)
{
  LexEntry	*le1;
  le1 = PNodeLeftmostLexEntry(pn->pn1);
  if (pn->pn1 && pn->pn1->feature == F_PREPOSITION &&
      pn->pn2 && pn->pn2->feature == F_NP &&
      (!PNodeClassIn(pn->pn2, N("interrogative-adverb"))) &&
      (!PNodeClassIn(pn->pn2, N("interrogative-determiner"))) &&
      (!PNodeClassIn(pn->pn2, N("interrogative-pronoun")))) {
    if (pn->pn2->pn1 && pn->pn2->pn1->feature == F_PRONOUN) {
      if (LexEntryConceptIsAncestor(N("prep-of"), le1)) {
        /* de + pronoun -> en */
        TransformGenAddPronoun(N("pronoun-en"), F_NULL, F_NULL, F_NULL,
                               dc, pronouns);
        /* todoFREE */
        return(1);
      } else if (LexEntryConceptIsAncestor(N("reflexive-pronoun"),
                   PNodeLeftmostLexEntry(pn->pn2->pn1))) {
        /* prep + reflexive-pronoun -> reflexive-pronoun */
        if (pronouns) {
          *pronouns = LexitemListAddPronoun(pn->pn2->pn1->lexitem, *pronouns);
        } else {
          Dbg(DBGGENER, DBGBAD, "TransformGenFrenchPPElim: NULL pronouns");
        }
        return(1);
      } else if (LexEntryConceptIsAncestor(N("prep-to"), le1)) {
        /* à + pronoun -> indirect-object-pronoun */
         if (pronouns) {
          if (LexEntryConceptIsAncestor(N("pronoun-there-location"),
                                        PNodeLeftmostLexEntry(pn->pn2->pn1))) {
            /* "à là-bas" -> "y" */
            TransformGenAddPronoun(N("pronoun-y"), F_NULL, F_NULL, F_NULL,
                                   dc, pronouns);
          } else {
            *pronouns =
            LexitemListAddPronoun(TransformPronoun(pn->pn2->pn1->lexitem,
                                                 N("indirect-object-pronoun"),
                                                 dc),
                                *pronouns);
          }
          /* todoFREE */
        } else Dbg(DBGGENER, DBGBAD, "TransformGenFrenchPPElim: NULL pronouns");
        return(1);
      }
    }
  }
  return(0);
}

Bool TransformGenFrenchNPElim(PNode *pn, Discourse *dc,
                              /* RESULTS */ LexitemList **pronouns)
{
  if (pn->pn1 && pn->pn1->feature == F_PRONOUN &&
      (LexEntryConceptIsAncestor(N("direct-object-pronoun"),
                                 PNodeLeftmostLexEntry(pn->pn1)) ||
       LexEntryConceptIsAncestor(N("indirect-object-pronoun"),
                                 PNodeLeftmostLexEntry(pn->pn1)) ||
       LexEntryConceptIsAncestor(N("reflexive-pronoun"),
                                 PNodeLeftmostLexEntry(pn->pn1)))) {
    if (pronouns) {
      *pronouns = LexitemListAddPronoun(pn->pn1->lexitem, *pronouns);
    } else {
      Dbg(DBGGENER, DBGBAD, "TransformGenFrenchNPElim: NULL pronouns");
    }
    return(1);
  }
  return(0);
}

/*
 * [X [X [D un] [X [N tuteur]]] [Y [R de] [X [H toi]]]] ->
 * [X [D ton] [X [N tuteur]]]
 */
PNode *TransformGenPossessive(PNode *pn, Discourse *dc, /* RESULTS */
                              int *change)
{
  PNode	*pn_possdet, *r;
  if (pn->feature == F_NP &&
      pn->pn1 && pn->pn1->feature == F_NP &&
      pn->pn1->pn1 && pn->pn1->pn1->feature == F_DETERMINER &&
      pn->pn2 && pn->pn2->feature == F_PP &&
      pn->pn2->pn1 && pn->pn2->pn1->feature == F_PREPOSITION &&
      LexEntryConceptIsAncestor(N("prep-of"),
        PNodeLeftmostLexEntry(pn->pn2->pn1)) &&
        pn->pn2->pn2 && pn->pn2->pn2->feature == F_NP &&
        pn->pn2->pn2->pn1 && pn->pn2->pn2->pn1->feature == F_PRONOUN) {
    if (DC(dc).lang == F_ENGLISH) {
      pn_possdet = GenMakeEnglishPossDet(pn->pn2->pn2->pn1->lexitem->features,
                                         dc);
    } else {
      pn_possdet = GenMakeFrenchPossDet(pn->pn2->pn2->pn1->lexitem->features,
                                        pn->pn1->pn1->lexitem->features, dc);
    }
    if (pn_possdet) {
      pn->pn1->pn1 = pn_possdet;
      pn->pn2 = NULL;
      r = pn->pn1;
      *change = 1;
      return(r);
    }
  }
  return(pn);
}

/*
 * [X [X [X [H you]] [9 s]] [X [N sister]]] ->
 * [X [D your] [X [N sister]]]
 */
PNode *TransformGenFixEnglishGenitive(PNode *pn, Discourse *dc, /* RESULTS */
                                      int *change)
{
  PNode	*pn_possdet;
  if (PNodeIsX_XX_X9(pn)) {
    if ((pn_possdet =
         GenMakeEnglishPossDet(pn->pn1->pn1->pn1->lexitem->features, dc))) {
      pn->pn1 = pn_possdet;
      *change = 1;
      return(pn);
    }
  }
  return(pn);
}

/* a onion -> an onion
 * sa amie -> son amie 
 *
 * See (Grevisse, 1986, Sections 45, 46)
 * and (Alexander, 1988, section 3.7).
 * todo: Abbreviations should automatically take from single letters:
 * An RAF, a UN.
 */
PNode *TransformGenPrevowel(PNode *pn, Discourse *dc, /* RESULTS */ int *change)
{
  Word	*alter;
  if (pn->pn1 && pn->pn1->lexitem &&
      pn->pn2 &&
      F_VOCALIC == LexitemInitialSound(PNodeLeftmostLexitem(pn->pn2), dc) &&
      (alter = LexEntryGetAlter(pn->pn1->lexitem->le,
                                pn->pn1->lexitem->features, F_PREVOWEL))) {
    pn->pn1->lexitem->word = alter->word;
    pn->pn1->lexitem->features = alter->features;
    *change = 1;
  }
  return(pn);
}

/* la université -> l'université */
PNode *TransformGenFrenchElision(PNode *pn, Discourse *dc, /* RESULTS */
                                 int *change)
{
  Word		*alter;
  Lexitem	*lexitem1;
  if (pn->pn1 && (lexitem1 = PNodeRightmostLexitem(pn->pn1)) && pn->pn2 &&
      F_VOCALIC == LexitemInitialSound(PNodeLeftmostLexitem(pn->pn2), dc) &&
      (alter = LexEntryGetAlter(lexitem1->le, lexitem1->features, F_ELISION))) {
    lexitem1->word = alter->word;
    lexitem1->features = alter->features;
    *change = 1;
  }
  return(pn);
}

PNode *TransformGenFrenchContractions(PNode *pn, Discourse *dc, /* RESULTS */
                                      int *change)
{
  int	det_gender, det_number;
  PNode	*pnword2;
  Obj	*noun_con;
  pnword2 = PNodeLeftmost(pn->pn2);
  if (pn->pn1 && pn->pn1->feature == F_PREPOSITION &&
      pnword2 && pnword2->feature == F_DETERMINER &&
      pnword2->lexitem &&
      LexEntryConceptIsAncestor(N("definite-article"), pnword2->lexitem->le)) {
    det_number = FeatureGet(pnword2->lexitem->features, FT_NUMBER);
    det_gender = FeatureGet(pnword2->lexitem->features, FT_GENDER);
    if ((!StringIn(F_ELISION, pnword2->lexitem->features)) &&
        (det_number == F_PLURAL || det_gender == F_MASCULINE)) {
      if (LexEntryConceptIsAncestor(N("prep-to"), pn->pn1->lexitem->le)) {
      /* à le -> au
       * à les -> aux
       */
        pn->pn1 = GenMakePrep(N("prep-au"), det_gender, det_number, dc);
        LexitemMakeTrace(pnword2->lexitem);
        *change = 1;
        return(pn);
      } else if (LexEntryConceptIsAncestor(N("prep-of"),
                                           pn->pn1->lexitem->le)) {
      /* de le -> du
       * de les -> des
       */
        pn->pn1 = GenMakePrep(N("prep-du"), det_gender, det_number, dc);
        LexitemMakeTrace(pnword2->lexitem);
        *change = 1;
        return(pn);
      }
    }
    if ((noun_con = PNodeNounConcept(pn->pn2)) &&
        ISA(N("polity"), noun_con) &&
        LexEntryConceptIsAncestor(N("prep-en"), pn->pn1->lexitem->le)) {
      /* If noun is "polity".
       * en la -> en
       * en le + vowel -> en
       * en le + nonvowel -> au
       * en les -> aux
       */
      if (det_gender == F_FEMININE ||
          (F_VOCALIC ==
           LexitemInitialSound(PNodeLeftmostLexitem(pn->pn2), dc))) {
        LexitemMakeTrace(pnword2->lexitem);
        *change = 1;
        return(pn);
      } else {
        pn->pn1 = GenMakePrep(N("prep-au"), det_gender, det_number, dc);
        LexitemMakeTrace(pnword2->lexitem);
        *change = 1;
        return(pn);
      }
    }
  }
  return(pn);
}

/* in/at/to there/where -> there/where */
PNode *TransformGenEnglishAtThere(PNode *pn, Discourse *dc, /* RESULTS */
                                  int *change)
{
  PNode	*r;
  if (pn->feature == F_PP &&
      pn->pn1 && pn->pn1->feature == F_PREPOSITION &&
      pn->pn2 && pn->pn2->feature == F_NP &&
      pn->pn2->pn1 && pn->pn2->pn1->feature == F_PRONOUN &&
      (LexEntryConceptIsAncestor(N("prep-in"),PNodeLeftmostLexEntry(pn->pn1)) ||
       LexEntryConceptIsAncestor(N("prep-at"),PNodeLeftmostLexEntry(pn->pn1)) ||
       LexEntryConceptIsAncestor(N("prep-to"),
                                 PNodeLeftmostLexEntry(pn->pn1))) &&
      (LexEntryConceptIsAncestor(N("pronoun-there-location"),
                                 PNodeLeftmostLexEntry(pn->pn2->pn1)) ||
       LexEntryConceptIsAncestor(N("location-interrogative-pronoun"),
                                 PNodeLeftmostLexEntry(pn->pn2->pn1)))) {
    r = pn->pn2;
    *change = 1;
    return(r);
  }
  return(pn);
}

/* Z->X, [X [Z [K that] [Z ...]]]
 * This doesn't touch the X -> X Z case, where the relative pronoun
 * can't be deleted.
 */
PNode *TransformGenEnglishThatDeletion(PNode *pn, Discourse *dc,
                                       /* RESULTS */ int *change)
{
  if (pn->feature == F_NP &&
      pn->pn1 && pn->pn1->feature == F_S &&
      pn->pn2 == NULL &&
      pn->pn1->pn1 && pn->pn1->pn1->feature == F_CONJUNCTION &&
      pn->pn1->pn1->lexitem &&
      pn->pn1->pn2 && pn->pn1->pn2->feature == F_S &&
      LexEntryConceptIsAncestor(N("standard-subordinating-conjunction"),
                                pn->pn1->pn1->lexitem->le)) {
    pn->pn1 = pn->pn1->pn2;
    *change = 1;
    return(pn);
  }
  return(pn);
}

PNode *TransformGen1(PNode *pn, Discourse *dc, int collect_pronouns,
                     int np_elim, /* RESULTS */ LexitemList **pronouns,
                     int *change)
{
  LexitemList	*sub_pronouns, *p;
  PNode		*pn1, *pn2, *pn_pronoun;
  if (!pn) return(NULL);
  if (pn->lexitem) return(pn);
  if (pn->feature == F_PP) np_elim = 0;
  pn = TransformGenPossessive(pn, dc, change);
  pn = TransformGenPrevowel(pn, dc, change);
  if (DC(dc).lang == F_FRENCH) {
    if (pn->pn2 && pn->pn2->feature == F_PP) {
      if (collect_pronouns && TransformGenFrenchPPElim(pn->pn2, dc, pronouns)) {
        *change = 1;
        return(pn->pn1);
      }
    } else if (np_elim && pn->pn2 && pn->pn2->feature == F_NP) {
      if (collect_pronouns && TransformGenFrenchNPElim(pn->pn2, dc, pronouns)) {
        *change = 1;
        return(pn->pn1);
      }
    }
  } else {
    pn = TransformGenFixEnglishGenitive(pn, dc, change);
    pn = TransformGenEnglishThatDeletion(pn, dc, change);
    pn = TransformGenEnglishAtThere(pn, dc, change);
  }
  if (pn->feature == F_S &&
      pn->pn1 && pn->pn1->feature == F_NP &&
      pn->pn2 && pn->pn2->feature == F_VP) {
    pn1 = TransformGen1(pn->pn1, dc, 0, 0, NULL, change);
    sub_pronouns = NULL;
    pn2 = TransformGen1(pn->pn2, dc, 1, 1, &sub_pronouns, change);
    for (p = sub_pronouns; p; p = p->next) {
      pn_pronoun = PNodeCreate(F_PRONOUN, p->lexitem, NULL, NULL, 0L, 0L,
                               NULL, NULL);
      pn_pronoun->type = PNTYPE_LEXITEM;
      /* todo: Insert before infinitive instead of beginning if there is one.
       * also, location of pronouns for imperative is different.
       */
      pn2 = PNodeConstit(F_VP, pn_pronoun, pn2);
    }
    /* todoFREE: Free sub_pronouns. */
  } else if (pn->feature == F_NP) {
    pn1 = TransformGen1(pn->pn1, dc, 0, 0, NULL, change);
    pn2 = TransformGen1(pn->pn2, dc, 0, 0, NULL, change);
  } else {
    pn1 = TransformGen1(pn->pn1, dc, collect_pronouns, np_elim, pronouns,
                        change);
    pn2 = TransformGen1(pn->pn2, dc, collect_pronouns, np_elim, pronouns,
                        change);
  }
  pn->pn1 = pn1;
  pn->pn2 = pn2;
  if (DC(dc).lang == F_FRENCH) {
    pn = TransformGenFrenchContractions(pn, dc, change);
    pn = TransformGenFrenchElision(pn, dc, change);
       /* This is here to prevent
        * "Mon pied gauche est dans une entrée d'mon appartement loué."
        */
  }
  return(pn);
}

/* TOP-LEVEL FUNCTIONS */

PNode *TransformGen(PNode *pn, Discourse *dc)
{
  int	change, any_change;
  if (!pn) return(NULL);
  any_change = 0;
  if (DbgOn(DBGGENER, DBGHYPER)) {
    PNodePrettyPrint(Log, DiscourseGetInputChannel(dc), pn);
  }
  while (1) {
    change = 0;
    pn = TransformGen1(pn, dc, 0, 0, NULL, &change);
    if (change == 0) break;
    any_change = 1;
  }
  if (any_change && DbgOn(DBGGENER, DBGHYPER)) {
    Dbg(DBGGENER, DBGDETAIL, "==TransformGen==>");
    PNodePrettyPrint(Log, DiscourseGetInputChannel(dc), pn);
  }
  return(pn);
}

/* QUICK TRANSFORMS */

/* Destructive. todo: Leave a trace. */
Bool PNodeElimSubject(PNode *pn)
{
  if (!pn) return(0);
  if (pn->feature == F_S &&
      pn->pn1 && pn->pn1->feature == F_NP &&
      pn->pn2 && pn->pn2->feature == F_VP) {
/*
    PNodeFreeTree(pn->pn1);
 */
    pn->pn1 = pn->pn2;
    pn->pn2 = NULL;
    return(1);
  }
  if (PNodeElimSubject(pn->pn1)) return(1);
  if (PNodeElimSubject(pn->pn2)) return(1);
  return(0);
}

/* He wants [for him to have] => He wants [to have]
 * He sees [him having] => He sees having
 * She persuaded him [for him to go to the store] =>
 *   She persuaded him to go to the store
 * Je veux je avoir => Je veux avoir
 * Je veux il avoir (return value = 0; not allowed in French)
 * I see [him having] (untransformed)
 * I fell after [me tripping]. => I fell after tripping.
 */
Bool TransformSubordinateSubjectDeletion(int subcat, PNode *subj_np,
                                         PNode *obj_np, PNode *subord,
                                         Discourse *dc)
{
  Obj	*main_obj, *subord_obj;
  if (subcat == F_SUBCAT_INFINITIVE || subcat == F_SUBCAT_PRESENT_PARTICIPLE) {
    if (obj_np && obj_np != subord) {
      main_obj = PNodeNounConcept(obj_np);
    } else {
      main_obj = PNodeNounConcept(subj_np);
    }
    subord_obj = PNodeNounConcept(PNodeSubject(subord));
    if (main_obj == subord_obj) {
      PNodeElimSubject(subord);
      if (DC(dc).lang == F_ENGLISH &&
          subord->feature == F_PP &&
          LexEntryConceptIsAncestor(N("prep-grammatical-for"),
            PNodeLeftmostLexEntry(subord->pn1))) {
        /* Delete "for". */
        subord->pn1 = NULL;  /* todo: Leave a trace. */
      }
    } else {
      if (obj_np) return(0);
      /* Here is the case of an intransitive verb in which the subject of
       * the subordinate clause is not deleted.
       */
      if (DC(dc).lang == F_FRENCH) {
      /* Infinitives (and presumably also present participles) cannot have
       * subjects in French. See Alain Rouveret in Chomsky (1982/1987, p. 205).
       * Since this construction doesn't exist in French, the caller
       * must try again.
       */
        return(0);
      }
    }
  }
  return(1);
}

/* End of file. */
