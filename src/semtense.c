/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940705: converted to db objs from defines
 * 19940707: parsing complex tenses
 * 19940707: fixed tense generation to have clear recursion base
 * 19941230: some minor tense work for time parsing
 * 19950319: added Aspect
 *
 * - Compound verb parsing cases which do not work yet in the new scheme:
 *   PPDA vient de se coucher sur le lit.
 *   PPDA est en train de se coucher sur le lit.
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

/* TENSE PARSING */

Obj *TenseFindInflTense(int lang, Obj *tense, Obj *mood)
{
  Obj		*r;
  ObjList	*objs, *p;
  if (lang == F_ENGLISH) {
    if (mood == N("F63") && tense != N("F100") && tense != N("F101") &&
        tense != N("F102")) {
    /* This is because English inflections except for "be" do not specify
     * mood. Indicative is assumed.
     */ 
      mood = N("F71");
    }
    if (mood == N("F63")) mood = N("F71");
      /* This is because of inflection collapsing. */
    objs = RE(&TsNA, L(N("eng-infl-tense-of"), ObjWild, tense, E));
    for (p = objs; p; p = p->next) {
      if (YES(RE(&TsNA, L(N("eng-infl-mood-of"), I(p->obj, 1), mood, E)))) {
        r = I(p->obj, 1);
        ObjListFree(objs);
        return(r);
      }
    }
  } else {
    if (mood == N("F63")) mood = N("F71");
      /* This is because of inflection collapsing. */
    objs = RE(&TsNA, L(N("fr-infl-tense-of"), ObjWild, tense, E));
    for (p = objs; p; p = p->next) {
      if (YES(RE(&TsNA, L(N("fr-infl-mood-of"), I(p->obj, 1), mood, E)))) {
        r = I(p->obj, 1);
        ObjListFree(objs);
        return(r);
      }
    }
  }
  ObjListFree(objs);
  Dbg(DBGTENSE, DBGBAD, "no infl tense <%s> mood <%s>", M(tense), M(mood));
  return(NULL);
}

Obj *TenseFindMainAux(int lang, Obj *auxtense, LexEntry *auxverb,
                      Obj *maintense)
{
  Obj		*r;
  ObjList	*objs1, *objs2, *p, *q;
  if (maintense == NULL) {
  /* "List my appointments." */
    return(auxtense);
  }
  if ((!auxtense) || (!auxverb)) return(NULL);
  if (lang == F_ENGLISH) {
    objs1 = RE(&TsNA, L(N("eng-aux-tense-of"), ObjWild, auxtense, E));
    for (p = objs1; p; p = p->next) {
      if (YES(RE(&TsNA, L(N("eng-main-tense-of"), I(p->obj, 1), maintense, E)))) {
        objs2 = RE(&TsNA, L(N("eng-aux-verb-of"), I(p->obj, 1), ObjWild, E));
        for (q = objs2; q; q = q->next) {
          if (ObjIsForLexEntry(I(q->obj, 2), auxverb)) {
            r = I(p->obj, 1);
            ObjListFree(objs1);
            ObjListFree(objs2);
            return(r);
          }
        }
        ObjListFree(objs2);
      }
    }
  } else {
    objs1 = RE(&TsNA, L(N("fr-aux-tense-of"), ObjWild, auxtense, E));
    for (p = objs1; p; p = p->next) {
      if (YES(RE(&TsNA, L(N("fr-main-tense-of"), I(p->obj, 1), maintense, E)))) {
        objs2 = RE(&TsNA, L(N("fr-aux-verb-of"), I(p->obj, 1), ObjWild, E));
        for (q = objs2; q; q = q->next) {
          if (ObjIsForLexEntry(I(q->obj, 2), auxverb)) {
            r = I(p->obj, 1);
            ObjListFree(objs1);
            ObjListFree(objs2);
            return(r);
          }
        }
        ObjListFree(objs2);
      }
    }
  }
  ObjListFree(objs1);
/* Here invalid compound tenses such as "set seems" are ruled out.
  Dbg(DBGTENSE, DBGDETAIL, "no aux-tense <%s> aux-verb <%s> main-tense <%s>",
      M(auxtense), auxverb ? auxverb->srcphrase : "", M(maintense));
 */
  return(NULL);
}

void TenseParseCompTenseSetAdv(PNode *adv, PNode **adv1, PNode **adv2,
                               PNode **adv3)
{
  if (!*adv1) *adv1 = adv;
  else if (!*adv2) *adv2 = adv;
  else if (!*adv3) *adv3 = adv;
  else Dbg(DBGTENSE, DBGBAD, "too many adverbs <%s>",
           adv->lexitem->le->srcphrase);
}

void TenseParseCompTense_Adv(PNode *vp, PNode *adv, int lang, /* RESULTS */
                             PNode **pn_mainverb, LexEntry **mainverb,
                             PNode **pn_agreeverb, Obj **tense, Obj **mood,
                             PNode **adv1, PNode **adv2, PNode **adv3)
{
  TenseParseCompTense1(vp, lang, pn_mainverb, mainverb, pn_agreeverb,
                       tense, mood, adv1, adv2, adv3);
  if (lang == F_ENGLISH && adv->lexitem && adv->lexitem->le &&
      streq("just", adv->lexitem->le->srcphrase) &&
      (*tense == N("preterit-indicative"))) {
    *tense = N("past-recent");
  } else {
    TenseParseCompTenseSetAdv(adv, adv1, adv2, adv3);
  }
}

/* This is a faster compound tense parser for use during Syn_Parse
 * and Translate. It should be maintained consistent with Sem_ParseVP
 * for the compound tense cases.
 */
void TenseParseCompTense1(PNode *pn, int lang,
                          /* RESULTS */ PNode **pn_mainverb,
                          LexEntry **mainverb, PNode **pn_agreeverb,
                          Obj **tense, Obj **mood, PNode **adv1,
                          PNode **adv2, PNode **adv3)
{
  Obj		*tense1, *tense2, *mood1, *mood2;
  LexEntry	*auxverb;
  PNode		*pn_auxverb, *dummy;
  /************************************************************************/
  if (pn->feature == F_VERB) {
    *pn_mainverb = *pn_agreeverb = pn;
    *mainverb = pn->lexitem->le;
    *mood = FeatToCon(FeatureGet(pn->lexitem->features, FT_MOOD));
    *tense = TenseFindInflTense(lang,
               FeatToCon(FeatureGet(pn->lexitem->features, FT_TENSE)),
                                *mood);
  /************************************************************************/
  } else if (pn->pn1 && pn->pn1->feature == F_VERB && pn->pn2 == NULL) {
    TenseParseCompTense1(pn->pn1, lang, pn_mainverb, mainverb, pn_agreeverb,
                         tense, mood, adv1, adv2, adv3);
  /************************************************************************/
  } else if (pn->pn1 && pn->pn1->feature == F_VP &&
             pn->pn2 && pn->pn2->feature == F_VERB) {
    TenseParseCompTense1(pn->pn1, lang, &pn_auxverb, &auxverb, &dummy,
                  &tense1, &mood1, adv1, adv2, adv3);
    *pn_agreeverb = pn_auxverb;
    TenseParseCompTense1(pn->pn2, lang, pn_mainverb, mainverb, &dummy,
                  &tense2, &mood2, adv1, adv2, adv3);
    *tense = TenseFindMainAux(lang, tense1, auxverb, tense2);
    *mood = mood1;
  /************************************************************************/
  } else if (pn->pn1 && pn->pn1->feature == F_VP &&
             pn->pn2 && pn->pn2->feature == F_VP) {
    TenseParseCompTense1(pn->pn1, lang, &pn_auxverb, &auxverb, &dummy,
                  &tense1, &mood1, adv1, adv2, adv3);
    *pn_agreeverb = pn_auxverb;
    TenseParseCompTense1(pn->pn2, lang, pn_mainverb, mainverb, &dummy,
                  &tense2, &mood2, adv1, adv2, adv3);
    *tense = TenseFindMainAux(lang, tense1, auxverb, tense2);
    *mood = mood1;
  /************************************************************************/
  } else if (pn->pn2 && pn->pn1->feature == F_VP &&
             pn->pn2 && pn->pn2->feature == F_ADVERB) {
    TenseParseCompTense_Adv(pn->pn1, pn->pn2, lang, pn_mainverb,
                            mainverb, pn_agreeverb, tense, mood, adv1, adv2,
                            adv3);
  /************************************************************************/
  } else if (pn->pn1 && pn->pn1->feature == F_ADVERB &&
             pn->pn2 && pn->pn2->feature == F_VP) {
    TenseParseCompTense_Adv(pn->pn2, pn->pn1, lang, pn_mainverb,
                            mainverb, pn_agreeverb, tense, mood, adv1, adv2,
                            adv3);
  /************************************************************************/
  } else if (pn->pn1 && pn->pn1->feature == F_VP &&
             pn->pn2 && pn->pn2->feature == F_PRONOUN) {
  /* todo: Check pronoun well-formedness and capture pronouns. */
    TenseParseCompTense1(pn->pn1, lang, pn_mainverb, mainverb,
                         pn_agreeverb, tense, mood, adv1, adv2, adv3);
  /************************************************************************/
  } else if (pn->pn1 && pn->pn1->feature == F_PRONOUN &&
             pn->pn2 && pn->pn2->feature == F_VP) {
    TenseParseCompTense1(pn->pn2, lang, pn_mainverb, mainverb,
                         pn_agreeverb, tense, mood, adv1, adv2, adv3);
  /************************************************************************/
  } else if (pn->pn1 && pn->pn1->feature == F_VP &&
             pn->pn2 && pn->pn2->feature == F_EXPLETIVE) {
    TenseParseCompTense1(pn->pn2, lang, pn_mainverb, mainverb,
                         pn_agreeverb, tense, mood, adv1, adv2, adv3);
  /************************************************************************/
  } else if (pn->pn1 && pn->pn1->feature == F_PREPOSITION &&
             pn->pn2 && pn->pn2->feature == F_VP) {
    TenseParseCompTense1(pn->pn2, lang, pn_mainverb, mainverb,
                         pn_agreeverb, tense, mood, adv1, adv2, adv3);
    if (*tense == N("infinitive") && lang == F_ENGLISH &&
        streq("to", pn->pn1->lexitem->le->srcphrase)) {
      *tense = N("infinitive-with-to");
    }
  /************************************************************************/
  } else {
    if (DbgOn(DBGSEMPAR, DBGHYPER)) {
      Dbg(DBGTENSE, DBGHYPER, "TenseParseCompTense1: compound tense rejected:");
      PNodePrint(Log, NULL, pn);
    }
    *pn_mainverb = NULL;
    *pn_agreeverb = NULL;
    *mainverb = NULL;
    *tense = NULL;
    *mood = NULL;
  }
  /************************************************************************/
}

void TenseParseCompTense(PNode *pn, int lang, /* RESULTS */ PNode **pn_mainverb,
                         LexEntry **mainverb, PNode **pn_agreeverb, Obj **tense,
                         Obj **mood, PNode **adv1, PNode **adv2, PNode **adv3)
{
  *adv1 = *adv2 = *adv3 = NULL;
  TenseParseCompTense1(pn, lang, pn_mainverb, mainverb, pn_agreeverb,
                       tense, mood, adv1, adv2, adv3);
}

Bool TenseParse_IsOnlyAnAuxiliary(LexEntry *le)
{
  LexEntryToObj	*leo;
  for (leo = le->leo; leo; leo = leo->next) {
    if (!ISA(N("auxiliary-verb"), leo->obj)) return(0);
  }
  return(1);
}

Bool TenseParseIsTopCompTenseOK(PNode *pn, int lang)
{
  PNode		*pn_mainverb, *pn_agreeverb, *adv1, *adv2, *adv3;
  Obj		*tense, *mood;
  LexEntry	*mainverb;
  TenseParseCompTense(pn, lang, &pn_mainverb, &mainverb, &pn_agreeverb, &tense,
                      &mood, &adv1, &adv2, &adv3);
  return(pn_mainverb && mainverb && tense &&
         (!TenseParse_IsOnlyAnAuxiliary(mainverb)));
}

Bool TenseParseIsSubCompTenseOK(PNode *pn, int lang)
{
  PNode		*pn_mainverb, *pn_agreeverb, *adv1, *adv2, *adv3;
  Obj		*tense, *mood;
  LexEntry	*mainverb;
  TenseParseCompTense(pn, lang, &pn_mainverb, &mainverb, &pn_agreeverb, &tense,
                      &mood, &adv1, &adv2, &adv3);
  return(pn_mainverb && mainverb && tense);
}

/* TENSE GENERATION */

PNode *TenseMakeWord(char *word, int pos, HashTable *ht, Discourse *dc)
{
  LexEntry	*le;
  Word		*infl;
  char		features[2];
  features[0] = pos;
  features[1] = TERM;
  if (!(le = LexEntryFind(word, features, ht))) {
    Dbg(DBGTENSE, DBGBAD, "TenseMakeWord 1");
    /* todo: return dummy */
    return(NULL);
  }
  if (!(infl = LexEntryGetInfl(le, dc))) {
    Dbg(DBGTENSE, DBGBAD, "TenseMakeWord 2");
    /* todo: return dummy */
    return(NULL);
  }
  return(PNodeWord(pos, infl->word, infl->features, le, NULL));
}

/* todo: Imperative 1P should use "let us". */
PNode *TenseGenVerbEnglish1(LexEntry *le, LexEntry *advle, LexEntry *notle,
                            LexEntry *justle, Obj *parent_tense, int tense,
                            int number, int person, int mood, int isaux,
                            Discourse *dc)
{
  Bool		infin, not_imper;
  LexEntry	*dole;
  PNode		*pn;
  Word		*vinfl, *doinfl, *advinfl, *notinfl, *justinfl;
  if (!(vinfl = LexEntryGetInflection(le, tense, F_NULL, number, person, mood,
                                      F_NULL, 1, dc))) {
    return(NULL);
  }
  not_imper = parent_tense != N("imperative");
  infin = (not_imper && (tense == F_INFINITIVE ||
                         tense == F_PRESENT_PARTICIPLE ||
                         tense == F_PAST_PARTICIPLE));
  if (not_imper &&
      (isaux || StringIn(F_MODAL, vinfl->features) ||
       streq(le->srcphrase, "be"))) {
  /* modal (adv) (not) (just) */
    pn = PNodeConstit(F_VP, PNodeWord(F_VERB, vinfl->word, vinfl->features,
                      le, NULL), NULL);
    if (advle) {
      if (!(advinfl = LexEntryGetInflection(advle, F_NULL, F_NULL, F_NULL,
                                            F_NULL, F_NULL, F_POSITIVE, 1, dc)))
        return(NULL);
      pn = PNodeConstit(F_VP, pn, PNodeWord(F_ADVERB, advinfl->word,
                                            advinfl->features, advle, NULL));
    }
    if (notle) { if (!(notinfl = LexEntryGetInfl(notle, dc))) return(NULL);
      pn = PNodeConstit(F_VP, pn, PNodeWord(F_ADVERB, notinfl->word,
                                            notinfl->features, notle, NULL));
    }
    if (justle) { if (!(justinfl = LexEntryGetInfl(justle, dc))) return(NULL);
      pn = PNodeConstit(F_VP, pn, PNodeWord(F_ADVERB, justinfl->word,
                                            justinfl->features, justle, NULL));
    }
  } else {
  /* (DO) (not) (adv) (just) verb */
    if (notle && !infin) {
      if (!(vinfl = LexEntryGetInflection(le, F_INFINITIVE, F_NULL, F_NULL,
                                           F_NULL, F_NULL, F_NULL, 1, dc)))
        return(NULL);
    }
    pn = PNodeConstit(F_VP,
                      PNodeWord(F_VERB, vinfl->word, vinfl->features, le, NULL),
                      NULL);
    if (justle) { if (!(justinfl = LexEntryGetInfl(justle, dc))) return(NULL);
      pn = PNodeConstit(F_VP, PNodeWord(F_ADVERB, justinfl->word,
                                        justinfl->features, justle, NULL), pn);
    }
    if (advle) {
      if (!(advinfl = LexEntryGetInflection(advle, F_NULL, F_NULL, F_NULL,
                                            F_NULL, F_NULL, F_POSITIVE,
                                            1, dc))) {
        return(NULL);
      }
      if (pn) {
        pn = PNodeConstit(F_VP, PNodeWord(F_ADVERB, advinfl->word,
                                          advinfl->features, advle, NULL), pn);
      }
    }
    if (notle) {
      if (!(notinfl = LexEntryGetInfl(notle, dc))) return(NULL);
      if (infin) {
        pn = PNodeConstit(F_VP, PNodeWord(F_ADVERB, notinfl->word,
                                          notinfl->features,
                                          notle, NULL), pn);
      } else {
        if (!(dole = LexEntryFind("do", "V", EnglishIndex))) return(NULL);
        if (!(doinfl = LexEntryGetInflection(dole, tense, F_NULL, number,
                                             person, mood, F_NULL, 1, dc))) {
          return(NULL);
        }
        pn = PNodeConstit(F_VP,
              PNodeConstit(F_VP,
              PNodeConstit(F_VP, PNodeWord(F_VERB, doinfl->word,
                                           doinfl->features, dole, NULL),
                           NULL),
              PNodeWord(F_ADVERB, notinfl->word, notinfl->features, notle,
                        NULL)),
                          pn);
      }
    }
  }
  return(pn);
}

PNode *TenseGenVerbEnglish(LexEntry *mainle, LexEntry *advle, LexEntry *notle,
                           Obj *tense, int subjnumber, int subjperson,
                           int isaux, Discourse *dc)
{
  Obj		*auxtense, *auxverb, *maintense, *infltense, *inflmood;
  PNode		*mainpn, *auxpn;
  LexEntry	*auxle, *justle;
  if ((infltense = R1EI(2, &TsNA,
                        L(N("eng-infl-tense-of"), tense, ObjWild, E)))) {
    inflmood = R1EI(2, &TsNA, L(N("eng-infl-mood-of"), tense, ObjWild, E));
    justle = ObjToLexEntryGet(R1EI(2, &TsNA, L(N("eng-infl-adverb-of"),
                                               tense, ObjWild, E)),
                                    F_ADVERB, F_NULL, dc);
    if (tense == N("preterit-subjunctive") && !streq(mainle->srcphrase, "be")) {
      inflmood = N("F71");
    }
    if (!(mainpn = TenseGenVerbEnglish1(mainle, advle, notle, justle,
                                        tense, ConToFeat(infltense),
                                        subjnumber, subjperson,
                                        ConToFeat(inflmood), isaux, dc))) {
      return(NULL);
    }
    if (tense == N("infinitive-with-to")) {
      mainpn = PNodeConstit(F_VP, TenseMakeWord("to", F_PREPOSITION,
                                                EnglishIndex, dc), mainpn);
    }
    return(mainpn);
  }
  if (!(auxtense =
        R1EI(2, &TsNA, L(N("eng-aux-tense-of"), tense, ObjWild, E)))) {
    return(NULL);
  }
  if (!(auxverb =
        R1EI(2, &TsNA, L(N("eng-aux-verb-of"), tense, ObjWild, E)))) {
    return(NULL);
  }
  if (!(maintense =
        R1EI(2, &TsNA, L(N("eng-main-tense-of"), tense, ObjWild, E)))) {
    return(NULL);
  }
  if (!(auxle = ObjToLexEntryGet(auxverb, F_VERB, F_NULL, dc))) return(NULL);
  if (!(auxpn = TenseGenVerbEnglish(auxle, advle, notle, auxtense, subjnumber,
                                    subjperson, 1, dc))) {
    return(NULL);
  }
  if (!(mainpn = TenseGenVerbEnglish(mainle, NULL, NULL, maintense, F_NULL,
                                     F_NULL, isaux, dc))) {
    return(NULL);
  }
  if (mainpn->feature == F_VP && mainpn->pn1 &&
      mainpn->pn1->feature == F_VERB) {
    return(PNodeConstit(F_VP, auxpn, mainpn->pn1));
  } else {
    return(PNodeConstit(F_VP, auxpn, mainpn));
  }
}

/* todo: Adverb before infinitive. */
PNode *TenseGenVerbFrench1(LexEntry *le, LexEntry *nele, LexEntry *pasle,
                           LexEntry *advle, int tense, int gender,
                           int number, int person, int mood, int degree,
                           Discourse *dc)
{
  PNode	*pn;
  Word	*vinfl, *neinfl, *pasinfl, *advinfl;
  if (!(vinfl = LexEntryGetInflection(le, tense, gender, number, person, mood,
                                      degree, 1, dc))) {
    return(NULL);
  }
  pn = PNodeConstit(F_VP,
                    PNodeWord(F_VERB, vinfl->word, vinfl->features, le, NULL),
                    NULL);
  if (nele && pasle && F_LITERARY != DC(dc).style &&
      (tense == F_INFINITIVE || tense == F_PRESENT_PARTICIPLE)) {
    if (!(pasinfl = LexEntryGetInfl(pasle, dc))) return(NULL);
    pn = PNodeConstit(F_VP, PNodeWord(F_ADVERB, pasinfl->word,
                                      pasinfl->features, pasle, NULL), pn);
  } else if (pasle) {
    if (!(pasinfl = LexEntryGetInfl(pasle, dc))) return(NULL);
    pn = PNodeConstit(F_VP, pn, PNodeWord(F_ADVERB, pasinfl->word,
                                          pasinfl->features, pasle, NULL));
  }
  if (nele) {
    if (!(neinfl = LexEntryGetInfl(nele, dc))) return(NULL);
    pn = PNodeConstit(F_VP, PNodeWord(F_ADVERB, neinfl->word, neinfl->features,
                                      nele, NULL), pn);
  }
  if (advle) {
    if (!(advinfl = LexEntryGetInflection(advle, F_NULL, F_NULL, F_NULL,
                                          F_NULL, F_NULL, F_POSITIVE, 1, dc))) {
      return(NULL);
    }
    pn = PNodeConstit(F_VP, pn, PNodeWord(F_ADVERB, advinfl->word,
                                          advinfl->features, advle, NULL));
  }
  return(pn);
}

PNode *TenseGenVerbFrench(LexEntry *mainle, char *usagefeat, LexEntry *nele,
                          LexEntry *pasle, LexEntry *advle, Obj *tense,
                          int subjgender, int subjnumber, int subjperson,
                          int objgender, int objnumber, int objperson,
                          int isaux, Discourse *dc)
{
  Obj		*auxtense, *auxverb, *maintense, *infltense, *inflmood;
  PNode		*mainpn, *auxpn;
  LexEntry	*auxle;
  if ((infltense = R1EI(2, &TsNA, L(N("fr-infl-tense-of"), tense, ObjWild,
                        E)))) {
    inflmood = R1EI(2, &TsNA, L(N("fr-infl-mood-of"), tense, ObjWild, E));
    return(TenseGenVerbFrench1(mainle, nele, pasle, advle, ConToFeat(infltense),
                               subjgender, subjnumber, subjperson,
                               ConToFeat(inflmood), F_NULL, dc));
  }
  if (!(auxtense = R1EI(2, &TsNA, L(N("fr-aux-tense-of"),tense,ObjWild, E)))) {
    return(NULL);
  }
  if (!(auxverb = R1EI(2, &TsNA, L(N("fr-aux-verb-of"), tense, ObjWild, E)))) {
    return(NULL);
  }
  if (!(maintense = R1EI(2, &TsNA,L(N("fr-main-tense-of"),tense,ObjWild, E)))){
    return(NULL);
  }
  if (auxverb == N("aux-be-have")) {
    if (StringIn(F_ETRE, usagefeat)) {
      auxverb = N("aux-be");
      objgender = subjgender; objnumber = subjnumber; objperson = subjperson;
    } else {
      auxverb = N("aux-have");
      if (objgender == F_NULL) objgender = F_MASCULINE;
      if (objnumber == F_NULL) objnumber = F_SINGULAR;
    }
  } else {
    objgender = F_NULL; objnumber = F_NULL; objperson = F_NULL;
  }
  if (!(auxle = ObjToLexEntryGet(auxverb, F_VERB, F_NULL, dc))) return(NULL);
  if (!(auxpn = TenseGenVerbFrench(auxle, "", nele, pasle, advle, auxtense,
                                   subjgender, subjnumber, subjperson,
                                   objgender, objnumber, objperson, 1, dc))) {
    return(NULL);
  }
  if (!(mainpn = TenseGenVerbFrench(mainle, usagefeat, NULL, NULL, NULL,
                                    maintense, objgender, objnumber, objperson,
                                    F_NULL, F_NULL, F_NULL, isaux, dc))) {
    return(NULL);
  }
  if (auxverb == N("aux-come")) {
    mainpn = PNodeConstit(F_VP, TenseMakeWord("de", F_PREPOSITION, FrenchIndex,
                                              dc),
                          mainpn);
  }
  if (mainpn->feature == F_VP && mainpn->pn1 &&
      mainpn->pn1->feature == F_VERB) {
    return(PNodeConstit(F_VP, auxpn, mainpn->pn1));
  } else {
    return(PNodeConstit(F_VP, auxpn, mainpn));
  }
}

/* SUBJUNCTIVIZATION */

Obj *TenseSubjRel(int style, int dialect, int lang)
{
  if (lang == F_FRENCH) {
    if (style == F_LITERARY) return(N("fr-literary-subjunctive-of"));
    else return(N("fr-subjunctive-of"));
  }
  if (dialect == F_AMERICAN) return(N("US-eng-subjunctive-of"));
  return(N("UK-eng-subjunctive-of"));
}

Obj *TenseSubjunctivize(Obj *tense, int style, int dialect, int lang)
{
  Obj	*r;
  if ((r = DbGetRelationValue(&TsNA, NULL, TenseSubjRel(style, dialect, lang),
                              tense, NULL))) {
    return(r);
  }
  return(tense);
}

Obj *TenseIndicativize(Obj *tense, int style, int dialect, int lang)
{
  Obj	*r;
  if ((r = DbGetRelationValue1(&TsNA, NULL, TenseSubjRel(style, dialect, lang),
                               tense, NULL))) {
    return(r);
  }
  return(tense);
}

/* PROGRESSIVIZATION */

Obj *TenseProgressivize1(Obj *tense, Obj *rel)
{
  Obj	*r;
  if ((r = DbGetRelationValue(&TsNA, NULL, rel, tense, NULL))) {
    return(r);
  }
  /* Tense is already progressive OR would require yet more compound tenses. */
  return(tense);
}

Obj *TenseProgressivize(Obj *tense, int lang, Obj *obj)
{
  if (lang == F_FRENCH) {
    return(TenseProgressivize1(tense, N("fr-progressive-of")));
  } else {
    /* French uses imperfect for continuous states and actions;
     * English only uses progressive for actions.
     * But Aspect takes care of this?
     */
    if (ISA(N("action"), I(obj, 0))) {
      return(TenseProgressivize1(tense, N("eng-progressive-of")));
    } else {
      return(tense);
    }
  }
}

/* INFINITIVIZATION */

Obj *TenseInfinitivize(Obj *tense, int lang)
{
  if (lang == F_ENGLISH) {
    /*
    if (ISA(N("past-tense"), tense)) return(N("past-infinitive-with-to"));
     */
    return(N("infinitive-with-to"));
  } else {
    /*
    if (ISA(N("past-tense"), tense)) return(N("past-infinitive"));
     */
    return(N("infinitive"));
  }
}

/* GERUNDIZATION */

Obj *TenseGerundize(Obj *tense)
{
  return(N("present-participle"));
/*
  if (ISA(N("past-tense"), tense)) {
    return(N("past-present-participle"));
  } else {
    return(N("present-participle"));
  }
 */
}

/* TENSE CONCORDANCE
 * References:
 * (Battye and Hintze, 1992, p. 285)
 * (Girodet, 1988, p. 862)
 */

int TenseGetTenseStep(Ts *ts1, Ts *ts2, Float spread)
{
  Float	ratio;
  if (spread < TENSEMINSPREAD) spread = TENSEMINSPREAD;
  if (!TsIsSpecific(ts1)) return(TENSESTEP_PAST);
  if (!TsIsSpecific(ts2)) return(TENSESTEP_FUTURE);  
  ratio = TsFloatMinus(ts1, ts2)/spread;
  if (FloatAbs(ratio) < TENSEEQUALREL) return(TENSESTEP_PRESENT);
  if (ratio < 0.0) {
    if ((-ratio) < TENSENEARREL) return(TENSESTEP_PAST_RECENT);
    else return(TENSESTEP_PAST);
  }
  if (ratio < TENSENEARREL) return(TENSESTEP_FUTURE_NEAR);
  else return(TENSESTEP_FUTURE);
}

int TenseGetTenseStepDistance(Ts *ts1, Ts *ts2, Float spread)
{
  Float	ratio;
  if (spread < TENSEMINSPREAD) spread = TENSEMINSPREAD;
  if (!TsIsSpecific(ts1)) return(-2);
  if (!TsIsSpecific(ts2)) return(2);  
  ratio = TsFloatMinus(ts1, ts2)/spread;
  if (FloatAbs(ratio) < TENSEEQUALREL) return(0);
  if (ratio < 0.0) {
    if ((-ratio) < TENSENEARREL) return(-1);
    else return(-2);
  }
  if (ratio < TENSENEARREL) return(1);
  else return(2);
}

Dur TenseStepDefaultDistance(TenseStep tensestep1, TenseStep tensestep2)
{
  if ((tensestep1 == -5 && tensestep2 == -4) ||
      (tensestep1 == -4 && tensestep2 == -3) ||
      (tensestep1 == -1 && tensestep2 == 0) ||
      (tensestep1 == 0 && tensestep2 == 1)) {
    return(TENSEDEFNEARABS);
  } else if ((tensestep2 == -5 && tensestep1 == -4) ||
      (tensestep2 == -4 && tensestep1 == -3) ||
      (tensestep2 == -1 && tensestep1 == 0) ||
      (tensestep2 == 0 && tensestep1 == 1)) {
    return(-TENSEDEFNEARABS);
  } else {
    return(TENSEDEFSTEPDIST*(tensestep2-tensestep1));
  }
}

TenseStep TenseToTenseStep(Obj *obj)
{
  Obj	*tensestep;
  if (obj == NULL) return((TenseStep)0);
  if ((tensestep = DbGetRelationValue(&TsNA, NULL, N("value-of"), obj, NULL))) {
    return((TenseStep)ObjToNumber(tensestep));
  }
  return((TenseStep)0);
}

/* todo: Repetitive events? */
Dur TenseDuration(TsRange *tsr)
{
  Dur	dur;
  if ((!TsIsSpecific(&tsr->startts)) || (!TsIsSpecific(&tsr->stopts))) {
    return(1L); /* todo */
  }
  dur = TsMinus(&tsr->stopts, &tsr->startts);
  if (dur > 0L) return(dur);
  else return(1L);
}

/* Get the tense of two concepts: conjunction tsr1, tsr2. */
void TenseGenConcord(Obj *obj1, Obj *obj2, TsRange *tsr1, TsRange *tsr2,
                     Ts *now, int lang, int literary,
                     Obj *rel, /* RESULTS */ Obj **t1, Obj **t2)
{
  int		simul, tensestep2, tensestep_dist;
  Float		spread;
  Obj		*aspect1, *aspect2;

  AspectDetermineRelatedPair(rel, obj1, obj2, &aspect1, &aspect2);

  /* Spread is the maximum minus the minimum timestamp. */
  spread = TsFloatSpread(&tsr1->startts, &tsr1->stopts, &tsr2->startts,
                         &tsr2->stopts, now);

  /* Situate matrix clause tense relative to now. */
  if (rel == N("persistent-posteriority")) {
    tensestep2 = TENSESTEP_PAST;
  } else if (TsRangeMatch(now, tsr2) && TsRangeMatch(now, tsr1)) { /* todo */
    tensestep2 = TENSESTEP_PRESENT;
  } else {
    tensestep2 = TenseGetTenseStep(&tsr2->startts, now, spread);
  }
  *t2 = AspectToTense(aspect2, tensestep2, literary, lang);

  simul = ISA(N("simultaneity"), rel);

  /* Situate subordinate clause tense relative to matrix clause. */
  if (simul || rel == N("adjacent-posteriority") ||
      rel == N("adjacent-priority") || rel == N("persistent-posteriority") ||
      TsRangeIsContained(tsr2, tsr1) || TsRangeIsContained(tsr1, tsr2)) {
    tensestep_dist = 0;
  } else {
    tensestep_dist = TenseGetTenseStepDistance(&tsr1->startts, &tsr2->startts,
                                             spread);
  }
  *t1 = AspectToTense(aspect1, tensestep2 + tensestep_dist, literary, lang);

#ifdef notdef
  /* todo: Aspect should handle the below. */
  if (simul) {
  /* Convert relatively continuous concepts to French imperfect/English
   * progressive.
   */
    if (rel == N("equally-long-simultaneity")) {
      *t1 = TenseProgressivize(*t1, lang, obj1);
      *t2 = TenseProgressivize(*t2, lang, obj2);
    } else {
      TsRangeDefault(tsr1, tsr2, &r1);
      TsRangeDefault(tsr2, tsr1, &r2);
      dur1 = (Float)TenseDuration(&r1);
      dur2 = (Float)TenseDuration(&r2);
      if ((dur1/dur2) < TENSEIMPERFECTREL) {
        *t2 = TenseProgressivize(*t2, lang, obj2);
      } else if ((dur2/dur1) < TENSEIMPERFECTREL) {
        *t1 = TenseProgressivize(*t1, lang, obj1);
      }
    }
  }
#endif
}

/* Get the tense of one concept in isolation. */
Obj *TenseGenOne(Obj *obj, TsRange *tsr, Ts *now, int lang, int literary,
                 /* RESULTS */ Obj **aspect)
{
  int	tensestep;
  Obj	*t;
  Float	spread;
  *aspect = AspectDetermineSingleton(obj);
  if (TsRangeMatch(now, tsr)) {
    tensestep = TENSESTEP_PRESENT;
  } else {
    spread = TsFloatSpread(&tsr->startts, &tsr->stopts, now, NULL, NULL);
    tensestep = TenseGetTenseStep(&tsr->startts, now, spread);
  }
  t = AspectToTense(*aspect, tensestep, literary, lang);
#ifdef notdef
  /* todo: Aspect should handle the below. */
  if (TenseDuration(tsr) >= TENSEIMPERFECTABS) {
    t = TenseProgressivize(t, lang, obj);
  }
#endif
  return(t);
}

/* End of file. */
