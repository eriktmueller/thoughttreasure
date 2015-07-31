/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
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
#include "reptext.h"
#include "reptime.h"
#include "semanaph.h"
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

/* NOUN PHRASE GENERATION */

void GenGetNounFeatures(Obj *obj, LexEntry *le, int pos, char *usagefeat,
                        int is_count, Discourse *dc, /* RESULTS */ int *tense,
                        int *degree, /* INPUT AND RESULTS */ int *gender,
                        int *number)
{
  *tense = F_NULL;
  *degree = F_NULL;
  if (pos == F_NOUN) {
    if (*gender == F_NULL) *gender = FeatureGet(le->features, FT_GENDER);
    if (*gender == F_NULL) {
      if (ISA(N("human"), obj)) {
        *gender = DiscourseGenderOf(obj, dc);
      }
    }
    if (StringIn(F_MASS_NOUN, usagefeat) && (!is_count)) {
      *number = F_SINGULAR;
    }
    if (StringIn(F_COMMON_INFL, usagefeat)) {
      *number = FeatureGet(usagefeat, FT_NUMBER);
    }
    if (*number == F_NULL) {
      if (StringIn(F_DEFINITE_ART, usagefeat)) {
        *number = F_SINGULAR; /* "the sky" */
      }
    }
    if (*number == F_NULL) {
      if (ObjIsAbstract(obj) && /* not Jacques-Chirac */
          N("aspect-generality-ascriptive") == dc->ga.aspect) {
      /* Elephants are gray. */
        *number = F_PLURAL;
      } else {
        *number = F_SINGULAR;
      }
    }
  } else if (pos == F_PRONOUN) {
    if (*number == F_NULL) *number = F_SINGULAR;
  } else if (pos == F_VERB) {
    *tense = F_INFINITIVE;
    *gender = F_MASCULINE;
    *number = F_SINGULAR;
  } else if (pos == F_ADJECTIVE) {
    *gender = F_MASCULINE;
    *number = F_SINGULAR;
    *degree = F_POSITIVE;
  } else if (pos == F_ADVERB) {
    *gender = F_NULL;
    *number = F_NULL;
    *degree = F_POSITIVE;
  } else if (pos == F_PREPOSITION) {
    *gender = F_NULL;
    *number = F_SINGULAR;
  } else {
    *gender = F_NULL;
    *number = F_NULL;
  }
}

/* todo: Use definite article if noun is implied by script context. */
PNode *GenObjNP1(Obj *obj, Obj *actual_obj, LexEntry *le, char *usagefeat,
                 Obj *cas, int anaphora_on, Obj *arttype, PNode *agree_np,
                 PNode *subj_np, int gender, int number, int person,
                 /* arguments for optional adjective phrase generation: */
                 LexEntry *adj_le, LexEntry *adv_le, char *adj_usagefeat,
                 int superlative,
                 /* */
                 int is_count, Discourse *dc)
{
  int		actual_gender, actual_number, person1;
  int		pos, tense, degree;
  Bool		isknown;
  Obj		*gened_con;
  PNode		*noun, *pronoun_pn, *np, *det, *adjp;
  Word		*infl;
  pos = FeatureGet(le->features, FT_POS);
  if (agree_np) {
    PNodeGetHeadNounFeatures(agree_np, 1, &gender, &number, &person);
  }
  GenGetNounFeatures(obj, le, pos, usagefeat, is_count, dc,
                     &tense, &degree, &gender, &number);
  if (anaphora_on) {
    /* todo: Pass in top-level concept here to generate intrasentential
     * pronouns.
     */
    Sem_AnaphoraGenPronoun(dc, obj, NULL, PNodeNounConcept(subj_np) == obj,
                           cas, &gender, &number, &person, &gened_con,
                           &pronoun_pn, &isknown, &actual_number);
  } else {
    pronoun_pn = NULL;
    gened_con = obj;
    isknown = 0;
    actual_number = number;
  }

  if (pronoun_pn) {
    if (actual_number != number) {
      if (pronoun_pn->pn1) pronoun_pn->pn1->number = actual_number;
      else Dbg(DBGGENER, DBGBAD, "GenObjNP1: bad pronoun");
    }
    DiscourseAnaphorAdd(dc, gened_con, actual_obj, pronoun_pn, gender,
                        number, person);
    return(pronoun_pn);
  }
  actual_gender = gender;
  if (obj == N("pronoun-it-expletive")) {
    gender = F_NEUTER;
    person1 = F_THIRD_PERSON;
  } else {
    person1 = F_NULL;
  }
  if (!(infl = LexEntryGetInflection(le, tense, gender, number, person1, F_NULL,
                                     degree, 1, dc))) {
    return(NULL);
  }
  number = FeatureGet(infl->features, FT_NUMBER);
  if ((F_NULL == (gender = FeatureGet(infl->features, FT_GENDER))) &&
      (DC(dc).lang == F_FRENCH)) {
    gender = F_MASCULINE;
  }
  noun = PNodeWord(pos, infl->word, infl->features, le, obj);
  if (actual_gender != gender) noun->gender = actual_gender;
  
  /* Construct noun. */
  np = PNodeConstit(F_NP, noun, NULL);

  /* Add optional adjective phrase. */
  if (adj_le) {
    adjp = GenADJP(adj_le, adv_le, superlative, gender, number, dc);
    if (LexEntryIsPreposedAdj(adj_usagefeat,
                             FeatureGet(adj_le->features, FT_POS), dc)) {
      np = PNodeConstit(F_NP, adjp, np);
    } else {
      if (superlative) {
        if (!(det = GenMakeDet(N("definite-article"), gender, number, F_NULL,
                               dc))) {
          return(NULL);
        }
        adjp = PNodeConstit(F_ADJP, det, adjp);
      }
      np = PNodeConstit(F_NP, np, adjp);
    }
  }

  /* Add optional article. */
  arttype = Sem_AnaphoraGenArticle(dc, obj, actual_obj, le, usagefeat,
                                   dc->ga.aspect, actual_number, pos, isknown,
                                   arttype);
  if (arttype && arttype != N("empty-article")) {
    if (!(det = GenMakeDet(arttype, gender, number, F_NULL, dc))) return(NULL);
    np = PNodeConstit(F_NP, det, np);
  }

  if (anaphora_on) {
    DiscourseAnaphorAdd(dc, actual_obj, actual_obj, np, actual_gender,
                        actual_number, person);
  }
  return(np);
}

PNode *GenElimDet(PNode *pn)
{
  if (pn->feature == F_NP &&
      pn->pn1 && pn->pn1->feature == F_DETERMINER &&
      pn->pn2 && pn->pn2->feature == F_NP) {
    return(GenElimDet(pn->pn2));
  }
  return(pn);
}

PNode *GenSwitchArticleToDefinite(PNode *pn, Discourse *dc)
{
  int	gender, number, person;
  PNode	*det;
  if (pn->feature == F_NP &&
      pn->pn1 && pn->pn1->feature == F_DETERMINER &&
      pn->pn2 && pn->pn2->feature == F_NP) {
    gender = FeatureGet(pn->pn1->lexitem->features, FT_GENDER);
    number = FeatureGet(pn->pn1->lexitem->features, FT_NUMBER);
    person = FeatureGet(pn->pn1->lexitem->features, FT_PERSON);
    if ((det = GenMakeDet(N("definite-article"), gender, number, person,
                          dc))) {
      pn->pn1 = det;
      return(pn);
    }
  }
  return(pn);
}

PNode *GenGenitive2(PNode *pn, PNode *of_pn, int of_animate, Discourse *dc)
{
  if (DC(dc).lang == F_ENGLISH &&
      of_animate &&
      PNodeNumberOfWords(of_pn) <= 4) {
    /* Baker (1978, p. 454) puts the first NP inside a determiner.
     * But multiple determiners are not usually allowed, yet
     * multiple genitive NPs are possible.
     * What is the structure of the NP inside the determiner?
     */
    return(PNodeConstit(F_NP,
      PNodeConstit(F_NP, of_pn, GenMakeElement(N("genitive-element"), dc)),
            GenElimDet(pn)));
  } else {
    return(PNodeConstit(F_NP, pn, GenMakePP(N("prep-of"), of_pn, dc)));
  }
}

PNode *GenGenitive1(PNode *pn, PNode *of_pn, Obj *of_obj, Discourse *dc)
{
  return(GenGenitive2(pn, of_pn, ISA(N("animate-object"), of_obj), dc));
}

Bool GenFindInterpersonalRelation(Ts *ts, Obj *human_a,
                                  /* RESULTS */ Obj **human_b, Obj **rel,
                                  int *iobji)
{
  Obj	*obj;
  if ((obj = R1D(ts, L(N("ipr"), ObjWild, human_a, E), 0))) {
    *human_b = I(obj, 1);
    *rel = I(obj, 0);
    *iobji = 1;
    return(1);
  }
  if ((obj = R1D(ts, L(N("ipr"), human_a, ObjWild, E), 0))) {
    *human_b = I(obj, 2);
    *rel = I(obj, 0);
    *iobji = 2;
    return(1);
  }
  return(0);
}

/* See also Sem_ParseGetRoleIndices. */
void GenGetRole(Obj *rel, int iobji, /* RESULTS */ Obj **rel_article)
{
  Obj	*reltype;
  reltype = DbGetEnumValue(&TsNA, NULL, N("relation-type"), rel, NULL);
  switch (iobji) {
    case 1:
      if (reltype == N("many-to-one")) {
        *rel_article = N("definite-article");
      } else if (reltype == N("one-to-many")) {
        *rel_article = N("indefinite-article");
      } else if (reltype == N("one-to-one")) {
        *rel_article = N("definite-article");
      } else {
        *rel_article = N("indefinite-article");
      }
      break;
    case 2:
      if (reltype == N("many-to-one")) {
        *rel_article = N("indefinite-article");
      } else if (reltype == N("one-to-many")) {
        *rel_article = N("definite-article");
      } else if (reltype == N("one-to-one")) {
        *rel_article = N("definite-article");
      } else {
        *rel_article = N("indefinite-article");
      }
      break;
    default:
      Dbg(DBGSEMPAR, DBGBAD, "GenGetCase: unknown iobji");
  }
}

/* Examples:
 * obj               pn
 * ----------------- ------------------
 * foyer23           "foyer"
 * living-room23     "living-room"
 */
PNode *GenArea(Ts *ts, Obj *obj, PNode *pn, Discourse *dc)
{
  ObjList	*objs;
  PNode		*of_pn;
  if (ISA(N("room"), obj)) {
    if ((objs = SpaceFindEnclose(ts, NULL, obj, N("apartment"), NULL))) {
      if ((of_pn = GenObjNPSimple(I(objs->obj, 2), dc))) {
        return(GenGenitive1(pn, of_pn, I(objs->obj, 2), dc));
      }
    }
  }
  return(NULL);
}

/* Examples:
 * rel          obj       pn      i
 * -----------  --------  ------- ----
 * owner-of     Stereo23  stereo  2
 * residence-of apt20rd   apart   1
 */
PNode *GenPseudoOwnership(Ts *ts, Obj *rel, Obj *obj, PNode *pn, int i,
                          Discourse *dc)
{
  ObjList	*of_objs;
  Obj		*of_obj;
  PNode		*of_pn;
  if (i == 1) {
    if ((of_objs = RDI(1, ts, L(rel, ObjWild, obj, E), 0))) {
      of_obj = ObjListToAndObj(of_objs);
      if ((of_pn = GenObjNPSimple(of_obj, dc))) {
        return(GenGenitive1(pn, of_pn, of_obj, dc));
      }
    }
  } else if (i == 2) {
    if ((of_objs = RDI(2, ts, L(rel, obj, ObjWild, E), 0))) {
      of_obj = ObjListToAndObj(of_objs);
      if ((of_pn = GenObjNPSimple(of_obj, dc))) {
        return(GenGenitive1(pn, of_pn, of_obj, dc));
      }
    }
  } else Dbg(DBGSEMPAR, DBGBAD, "GenPseudoOwnership: unknown i");
  return(NULL);
}

/* Example:
 * rel            iobji  result
 * -------------- ------ --------
 * N("sister-of")      1 "sister"
 * N("sister-of")      2 "brother"
 */
PNode *GenRelationNP1(Obj *rel, int iobji, Discourse *dc)
{
  Obj			*rel_article;
  ObjToLexEntry	*ole;
  GenGetRole(rel, iobji, &rel_article);
  if ((ole = ObjToLexEntryGet1(rel, NULL, FS_NOUN, F_NULL, F_NULL,
                               NULL, dc))) {
    return(GenRelationObjNP(rel, ole->le, ole->features, rel_article,
                            NULL, dc));

  }
  return(NULL);
}

/* rel                 of_obj             iobji result
 * --------------      ------------------ ----- ------ 
 * N("sister-of")      N("Jim")               1 "the sister of Jim"/
 *                                              "Jim's sister"
 *  [sister-of Jim ?]
 * N("President-of")   N("country-USA")     1 "the President of the US"
 *  [President-of country-USA ?]
 * N("nationality-of") N("country-USA")     2 "a citizen of the US"
 *  [nationality-of ? country-USA]
 */
PNode *GenRelationNP(Obj *rel, Obj *of_obj, int iobji, Discourse *dc)
{
  PNode	*of_pn, *pn1;
  if ((of_pn = GenObjNPSimple(of_obj, dc))) {
    if ((pn1 = GenRelationNP1(rel, iobji, dc))) {
      return(GenGenitive1(pn1, of_pn, of_obj, dc));
    }
  }
  return(NULL);
}

/* Same cases as Sem_ParseGenitive1:
 *     obj               pn
 * --------------------- ------------------
 * (1) left-foot-of-Jim "left foot"
 * (2) sister-of-Jim    "human"
 * (3) Stereo23          "stereo"
 * (4) foyer23           "foyer"
 *
 */
PNode *GenGenitive(Obj *obj, PNode *pn, Discourse *dc)
{
  int	iobji;
  Ts	*ts;
  Obj	*of_obj, *rel;
  PNode	*of_pn, *pn1;

  /* todo: Need to store outermost concept tsrange so we can use it here. */
  if (dc->cx_best) {
    ts = &(dc->cx_best->story_time.stopts);
  } else {
    ts = &TsNA;
  }

  /* CASE (1) */
  if ((of_obj = DbRetrieveWhole(ts, NULL, N("concept"), obj))) {
    if ((of_pn = GenObjNPSimple(of_obj, dc))) {
      return(GenGenitive1(GenSwitchArticleToDefinite(pn, dc), of_pn, of_obj,
                          dc));
    }
  }

  /* CASE (2): todo: Improve logic. Also this could recurse infinitely. */
  if (GenFindInterpersonalRelation(ts, obj, &of_obj, &rel, &iobji)) {
    if ((pn1 = GenRelationNP(rel, of_obj, iobji, dc))) {
      return(pn1);
    }
  }

  /* CASE (3) */
  if ((pn1 = GenPseudoOwnership(ts, N("owner-of"), obj, pn, 2, dc))) {
    return(pn1);
  }
  if ((pn1 = GenPseudoOwnership(ts, N("residence-of"), obj, pn, 1, dc))) {
    return(pn1);
  }
  if ((pn1 = GenPseudoOwnership(ts, N("headquarters-of"), obj, pn, 1, dc))) {
    return(pn1);
  }

  /* CASE (4) */
  if ((pn1 = GenArea(ts, obj, pn, dc))) {
    return(pn1);
  }

  return(NULL);
}

PNode *GenStringNP(Obj *obj, Discourse *dc)
{
  Obj	*string_type;
  PNode	*pn, *pn1;
  pn = GenLiteral(ObjToString(obj), F_NP, F_NOUN);
  if ((string_type = ObjToStringClass(obj)) &&
      N("email-address") != string_type &&
      N("string") != string_type) {
    if ((pn1 = GenNP(FS_HEADNOUN, string_type, N("subj"), 0, F_NULL,
                     N("empty-article"), NULL, NULL, F_NULL, F_NULL,
                     F_NULL, N("present-indicative"), 0, 0, dc))) {
      pn = PNodeConstit(F_NP, pn1, pn);
    }
  }
  return(pn);
}

PNode *GenObjNP(char *pos_restrict, Obj *obj, Obj *actual_obj, Obj *cas,
                int anaphora_on, int paruniv, Obj *arttype, PNode *agree_np,
                PNode *subj_np, int gender, int number, int person,
                Obj *comptense, int ancestor_ok, int require_noun,
                /* arguments for optional adjective phrase generation: */
                LexEntry *adj_le, LexEntry *adv_le, char *adj_usagefeat,
                int superlative,
                /* */
                int is_count, Discourse *dc)
{
  int		i, nump, savelang, newlang;
  ObjToLexEntry	*ole;
  PNode		*pn, *pn1;
  Float		x;
  if (obj == ObjNA || ObjIsVar(obj)) return(NULL);
  if (ISA(N("intervention"), obj)) {
    if ((pn = GenInterventionObj(F_NP, obj, dc))) {
      return(PNodeConstit(F_NP, pn, NULL));
    } else {
      return(NULL);
    }
  }
  if (FLOATNA != (x = ObjToNumber(obj))) {
  /* todo: Convert to appropriate units depending on country, range. */
    return(PNodeConstit(F_NP,
                        GenNumberWithUnits(x, ObjToNumberClass(obj),
                                           1, F_NOUN, F_MASCULINE, dc), NULL));
  }
  if (ObjIsString(obj)) {
    return(GenStringNP(obj, dc));
  }
  if ((pn = GenPolerina(obj))) {
    return pn;
  }
  if (ObjIsTsRange(obj)) {
    return(GenTsRangeObj(obj, dc));
  }
  if (ObjNumParents(obj) == 0 && IsUnprocessedTail(ObjToName(obj))) {
    return(GenLiteral(ObjToName(obj), F_NP, F_NOUN));
  }
  if ((ole = ObjToLexEntryGet1(obj, NULL, pos_restrict, F_NULL, paruniv,
                               NULL, dc))) {
    if ((pn = GenObjNP1(obj, actual_obj, ole->le, ole->features, cas,
                        anaphora_on, arttype, agree_np, subj_np, gender,
                        number, person, adj_le, adv_le, adj_usagefeat,
                        superlative, is_count, dc))) {
      return(pn);
    }
  }

  /* todo: Perhaps this would be better handled by filling in missing lexentries
   * in advance. The problem with the below is that pronouns cannot be generated
   * since the language is wrong.
   * 
   * Was if (ISA(N("organization"), obj) ||
   *         (ObjIsConcrete(obj) && ISA(N("media-object"), obj))).
   */
  if (dc->ga.flip_lang_ok &&
      (F_NULL != (newlang = FeatureFlipLanguage(DC(dc).lang)))) {
    savelang = DC(dc).lang;
    DiscourseSetLang(dc, newlang);
    ole = ObjToLexEntryGet1(obj, NULL, pos_restrict, F_NULL, paruniv,
                            NULL, dc);
    DiscourseSetLang(dc, savelang);
    if (ole) {
      if ((pn = GenObjNP1(obj, actual_obj, ole->le, ole->features, cas, 0,
                          arttype, agree_np, subj_np, gender, number, person,
                          adj_le, adv_le, adj_usagefeat, superlative, is_count,
                          dc))) {
        return(pn);
      }
    }
  }
  if (!require_noun) {
    /* Allow any part of speech to generate as a NP.
     * todo: Generate verb "expl"s (used in dictionary definitions).
     */
    if ((ole = ObjToLexEntryGet1(obj, NULL, FT_POS, F_NULL, paruniv,
                                 NULL, dc)) &&
        (pn = GenObjNP1(obj, actual_obj, ole->le, ole->features, cas,
                        anaphora_on, arttype, agree_np, subj_np, gender,
                        number, person, adj_le, adv_le, adj_usagefeat,
                        superlative, is_count, dc))) {
      return(pn);
    }
  }

  if (ancestor_ok) {
    for (i = 0, nump = ObjNumParents(obj); i < nump; i++) {
      if ((pn = GenObjNP(pos_restrict, ObjIthParent(obj, i), actual_obj, cas,
                         anaphora_on, paruniv, arttype, agree_np, subj_np,
                         gender, number, person, comptense, ancestor_ok,
                         require_noun, adj_le, adv_le, adj_usagefeat,
                         superlative, is_count, dc))) {
        if ((pn1 = GenGenitive(obj, pn, dc))) {
          return(pn1);
        } else {
          return(pn);
        }
      }
    }
  }
  if (!ObjBarrierISA(obj)) {
    GenTrouble("GenObjNP", obj, NULL, NULL);
  }
  return(NULL);
}

PNode *GenNP1(char *pos_restrict, Obj *obj, Obj *cas, int anaphora_on,
              int paruniv, Obj *arttype, PNode *agree_np, PNode *subj_np,
              int gender, int number, int person, Obj *comptense,
              int ancestor_ok, int require_noun, Discourse *dc)
{
  if (obj == NULL) return(NULL);
  if (ObjIsList(obj)) {
    if (ISA(N("such-that"), I(obj, 0))) {
      return(GenIntension(F_NP, obj, dc));
    } else if (ISA(N("question-element"), I(obj, 0))) {
      return(GenQuestionElement(F_NP, obj, dc));
    } else if (ISA(N("and"), I(obj, 0)) || ISA(N("or"), I(obj, 0))) {
      /* todo: Pass down arttype for use with nouns. */
      return(GenCoord(F_NP, I(obj, 0), obj, comptense, dc));
    } else {
      return(GenerateNoTS(F_NP, obj, comptense, dc));
    }
  }
  return(GenObjNP(pos_restrict, obj, obj, cas, anaphora_on, paruniv, arttype,
                  agree_np, subj_np, gender, number, person, comptense,
                  ancestor_ok, require_noun, NULL, NULL, "", 0, 0, dc));
}

PNode *GenNP(char *pos_restrict, Obj *obj, Obj *cas, int anaphora_on,
             int paruniv, Obj *arttype, PNode *agree_np, PNode *subj_np,
             int gender, int number, int person, Obj *comptense,
             int ancestor_ok, int require_noun, Discourse *dc)
{
  PNode	*pn;
  if (cas == N("subj") &&
      ISA(N("nonfinite-verb"), comptense)) {
    /* Subjects of nonfinite verbs are in objective case. */
    cas = N("obj");
  }
  if (!(pn = GenNP1(pos_restrict, obj, cas, anaphora_on, paruniv, arttype,
                    agree_np, subj_np, gender, number, person, comptense,
                    ancestor_ok, require_noun, dc))) {
    return(NULL);
  }
  if (XBarIs_NP_S(pn)) {
    if (ISA(N("infinitive"), comptense)) {
      if (DC(dc).lang == F_ENGLISH) {
      /* Add "for" in English unless a preposition is already present. */
        pn = GenMakePP(N("prep-grammatical-for"), pn, dc);
      }
    } else if (!ISA(N("participle"), comptense)) {
    /* Add "that". */
      pn = GenMakeSubordConj(F_NP, pn, dc);
    }
  }
  return(pn);
}

/* VERB PHRASE GENERATION */

PNode *GenVP1(LexEntry *le, char *usagefeat, Obj *comptense, int subjgender,
              int subjnumber, int subjperson, PNode *objnp, Discourse *dc)
{
  int	objgender, objnumber, objperson;
  PNode	*pn;
  if (!le) {
    Dbg(DBGGENER, DBGBAD, "GenVP1: empty le");
    return(NULL);
  }
  if (DC(dc).lang == F_FRENCH && objnp) {
    PNodeGetHeadNounFeatures(objnp, 0, &objgender, &objnumber, &objperson);
  } else {
    objgender = objnumber = objperson = F_NULL;
  }
  if (DC(dc).lang == F_FRENCH) {
    pn = TenseGenVerbFrench(le, usagefeat, dc->ga.gen_negle1, dc->ga.gen_negle2,
                            dc->ga.gen_advle, comptense, subjgender, subjnumber,
                            subjperson, objgender, objnumber, objperson,
                            0, dc);
  } else if (DC(dc).lang == F_ENGLISH) {
    pn = TenseGenVerbEnglish(le, dc->ga.gen_advle, dc->ga.gen_negle2, comptense,
                             subjnumber, subjperson, 0, dc);
  } else pn = NULL;
  if (!pn) {
    GenTrouble("GenVP1", comptense, NULL, le);
    if (comptense != N("present-indicative")) {
      return(GenVP1(le, usagefeat, N("present-indicative"), subjgender,
                    subjnumber, subjperson, objnp, dc));
    }
    return(NULL);
  }
  return(pn);
}

PNode *GenVP(LexEntry *le, char *usagefeat, Obj *comptense, PNode *subjnp,
             PNode *objnp, Discourse *dc)
{
  int	subjgender, subjnumber, subjperson;
  if (subjnp) {
    PNodeGetHeadNounFeatures(subjnp, 0, &subjgender, &subjnumber, &subjperson);
    if (subjperson == F_NULL) subjperson = F_THIRD_PERSON;
    if (subjnumber == F_NULL) subjnumber = F_SINGULAR;
  } else {
    if (DC(dc).lang == F_ENGLISH) {
      subjgender = F_NULL;
    } else {
      subjgender = F_MASCULINE;
    }
    subjnumber = F_SINGULAR;
    subjperson = F_THIRD_PERSON;
  }
  return(GenVP1(le, usagefeat, comptense, subjgender, subjnumber, subjperson,
                objnp, dc));
}

PNode *GenVPAdv1(LexEntry *le, char *usagefeat, Obj *comptense, int subjgender,
                 int subjnumber, int subjperson, PNode *objnp, PNode *adv1,
                 PNode *adv2, PNode *adv3, Discourse *dc)
{
  PNode		*r;
  GenAdvice	save_ga;
  save_ga = dc->ga;
  if (adv1) dc->ga.gen_advle = adv1->lexitem->le;
  if (adv2) dc->ga.gen_negle1 = adv2->lexitem->le;
  if (adv3) dc->ga.gen_negle2 = adv3->lexitem->le;
  r = GenVP1(le, usagefeat, comptense, subjgender, subjnumber, subjperson,
             objnp, dc);
  dc->ga = save_ga;
  return(r);
}

PNode *GenVPAdv(LexEntry *le, char *usagefeat, Obj *comptense, PNode *subjnp,
                PNode *objnp, PNode *adv1, PNode *adv2, PNode *adv3,
                Discourse *dc)
{
  PNode	*r;
  GenAdvice	save_ga;
  save_ga = dc->ga;
  if (adv1) dc->ga.gen_advle = adv1->lexitem->le;
  if (adv2) dc->ga.gen_negle1 = adv2->lexitem->le;
  if (adv3) dc->ga.gen_negle2 = adv3->lexitem->le;
  r = GenVP(le, usagefeat, comptense, subjnp, objnp, dc);
  dc->ga = save_ga;
  return(r);
}

/* POLERINA CONCEPT GENERATION */

PNode *GenPolerina(Obj *obj)
{
  char *nm;
  nm = ObjToName(obj);
  if (nm && StringHeadEqual("h-", nm)) {
    return GenPolerinaHuman(obj);
  } else {
    return NULL;
  }
}

PNode *GenPolerinaHuman(Obj *obj)
{
  char *nm, *s, buf[DWORDLEN], *bp, r[PHRASELEN], *pr;
  nm = ObjToName(obj);
  s = nm+2;
  pr = r;
  while (*s) {
    s = StringReadTo(s, buf, DWORDLEN, '-', TERM, TERM);
    if (*buf) {
      bp = buf+1;
      if (*bp) {
        if (pr > r) {
          if ((pr-r)>=PHRASELEN) break;
          *pr++ = ' ';
        }
        while (*bp) {
          if ((pr-r)>=PHRASELEN) break;
          if (*bp == '1') *pr++ = SQUOTE;
          else if (*bp == '8') *pr++ = '-';
          else *pr++ = *bp;
          bp++;
        }
      }
    }
  }
  *pr++ = TERM;
  if (r[0]) return GenLiteral(r, F_NP, F_NOUN);
  return NULL;
}

/* LITERAL GENERATION */

PNode *GenLiteral(char *literal, int constit, int pos)
{
  return(PNodeConstit(constit, PNodeWord(pos, StringCopy(literal, "GenLiteral"),
                                         "S", NULL, NULL), /* todo */
                               NULL));
}

/* NUMBER GENERATION */

/* todo: Words for numbers and fractions. */
PNode *GenNumber(Float val, int always_numeric, int pos, int gender,
                 Discourse *dc)
{
  char		word[DWORDLEN], features[2];
  LexEntry	*le;
  Word		*infl;
  PNode	*pn;
  if (!always_numeric) {
    if ((pn = GenValueName(val, N("cardinal-number"), pos, gender, F_SINGULAR,
                           dc))) {
      return(pn);
    }
    if ((pn = GenValueName(val, N("fraction"), pos, gender, F_SINGULAR, dc))) {
      return(pn);
    }
  }
  if (val > 9999.0) {
    if (val == FloatInt(val)) {
      sprintf(word, "%.0f", val);
    } else {
      sprintf(word, "%f", val);
    }
  } else {
    sprintf(word, "%g", val);
  }
  if (DC(dc).lang == F_FRENCH) StringSearchReplace(word, ".", ",");
  features[0] = pos;
  features[1] = TERM;
  if (!(le = LexEntryFindInfl(word, features, DC(dc).ht, 0))) return(NULL);
  if (!(infl = LexEntryGetInflection(le, F_NULL, gender, F_SINGULAR, F_NULL,
                                     F_NULL, F_NULL, 1, dc))) {
    return(NULL);
  }
  return(PNodeWord(pos, infl->word, infl->features, le, NULL));
}

PNode *GenNumberWithUnits(Float val, Obj *units, int always_numeric, int pos,
                          int gender, Discourse *dc)
{
  PNode	*pn_number, *pn_units;
  pn_number = GenNumber(val, always_numeric, pos, gender, dc);
  if (units && units != N("u")) {
    pn_units = GenMakeNoun(units, (val == 1.0) ? F_SINGULAR : F_PLURAL, dc);
  } else {
    pn_units = NULL;
  }
  if (pn_units) {
    return(PNodeConstit(F_NP, pn_number, pn_units));
  }
  return(pn_number);
}

/* TIMESTAMP RANGE GENERATION */

PNode *GenTsRange(Obj *obj, int norepeat, Discourse *dc)
{
  return(GenTsRange1(ObjToTsRange(obj), obj, norepeat, dc));
}

/* todo: Regular events. */
PNode *GenTsRange1(TsRange *tsr, Obj *obj, int norepeat, Discourse *dc)
{
  PNode	*pnstart, *pnstop, *pnconj, *pnpp;
  if (norepeat && TsRangeEqual(tsr, &DC(dc).last_tsr)) return(NULL);
  DC(dc).last_tsr = *tsr;
  if (TsIsSpecific(&tsr->startts) && TsGT(DCNOW(dc), &tsr->startts) &&
      TsIsNa(&tsr->stopts)) {
    if (!(pnstart = GenTs(&tsr->startts, 0, dc))) return(NULL);
    if ((pnpp = GenMakePP(N("prep-dur-start-alone"), pnstart, dc))) {
      return(pnpp);
    }
    return(pnstart);
  } else if ((obj == NULL || !ISA(N("action"), I(obj, 0))) &&
             TsIsSpecific(&tsr->startts) && TsIsSpecific(&tsr->stopts) &&
             (!TsEQ(&tsr->startts, &tsr->stopts)) &&
             (TsMinus(&tsr->stopts, &tsr->startts) > SECONDSPERMINL*15L)) {
    if (!(pnstart = GenTs(&tsr->startts, 0, dc))) return(NULL);
    if (!(pnstop = GenTs(&tsr->stopts, 0, dc))) return(NULL);
    if (!(pnconj = GenMakeConj(F_NP, N("and"), pnstart, pnstop, dc))) {
      return(NULL);
    }
    return(GenMakePP(N("prep-dur-between"), pnconj, dc));
  } else if (TsIsSpecific(&tsr->startts)) {
    return(GenTs(&tsr->startts, 1, dc));
  } else {
    return(NULL);
  }
}

PNode *GenTsRangeObj(Obj *obj, Discourse *dc)
{
  PNode		*pn;
  GenAdvice	save_ga;
  save_ga = dc->ga;
  dc->ga.gen_tsr = 1;
  TsrGenAdviceSet(&dc->ga.tga);
  pn = GenTsRange1(ObjToTsRange(obj), NULL, 0, dc);
  dc->ga = save_ga;
  return(pn);
}

/* TIMESTAMP GENERATION */

PNode *GenTsEmbed(int constit, PNode *pn1, PNode *pn2)
{
  if (pn1 && pn2) return(PNodeConstit(constit, pn1, pn2));
  else if (pn2) return(pn2);
  else return(pn1);
}

/* todo: Numeric time and date formats: 11/07/90 07.11.90 11:58 11.58 11h58. */
PNode *GenTs(Ts *ts, int not_embedded, Discourse *dc)
{
  int			day_included;
  PNode			*pn, *pndate, *pntod, *pnpp;
  if (dc->ga.tga.attempt_relative &&
      (pn = GenDurRelativeToNow(ts, 1, 0, not_embedded, dc))) {
    return(pn);
  }
  if ((!dc->ga.tga.attempt_day_and_part_of_the_day) ||
      (!(pndate = GenDayAndPartOfTheDay(ts, not_embedded, dc)))) {
    if ((!dc->ga.tga.include_day_of_the_week) &&
        (!dc->ga.tga.include_day) &&
        (!dc->ga.tga.include_month) &&
        (!dc->ga.tga.include_year)) {
     pndate = NULL;
    } else {
      if (!(pndate = GenDate(ts, dc->ga.tga.include_day_of_the_week,
                             dc->ga.tga.include_day,
                             dc->ga.tga.include_month,
                             dc->ga.tga.include_year, dc,
                             &day_included))) {
        return(NULL);
      }
    }
  } else not_embedded = 0;
  if (pndate && dc->ga.tga.prep_ok && not_embedded &&
      (pnpp = GenMakePP(day_included ? N("prep-temporal-position-day") :
                                       N("prep-temporal-position-month-year"),
                        pndate, dc))) {
    pndate = pnpp;
  }
  if (dc->ga.tga.include_time_of_day &&
      (pntod = GenTod(ts, dc))) {
    if (dc->ga.tga.prep_ok &&
        !(pntod = GenMakePP(N("prep-temporal-position-tod"), pntod, dc))) {
      return(NULL);
    }
    return(GenTsEmbed(F_ADVP, pndate, pntod));
  }
  return(pndate);
}

/* todo: A.D./B.C., 18th instead of 18 in English, commas. */
PNode *GenDate(Ts *ts, int include_day_of_the_week, int include_day,
               int include_month, int include_year, Discourse *dc,
               /* RESULTS */ int *day_included)
{
  PNode		*pn, *pnday_of_the_week, *pnyear, *pnmonth, *pnday;
  int		year, month, day, hour, min, sec;
  Float		day_of_the_week;
  day_of_the_week = (Float)TsToDay(ts);
  TsToYMDHMS(ts, &year, &month, &day, &hour, &min, &sec);
  if (day == INTNA) include_day = 0;
  if (month == INTNA) {
    include_month = 0;
    include_day_of_the_week = 0;
    include_day = 0;
    include_year = 1;
  }
  if (day_included) *day_included = include_day;
  if (!include_month) {
    if (include_day_of_the_week) {
      Dbg(DBGGENER, DBGBAD, "GenDate: disallowed case 1");
    }
    if (include_day) {
      Dbg(DBGGENER, DBGBAD, "GenDate: disallowed case 2");
    }
    if (!include_year) {
      Dbg(DBGGENER, DBGBAD, "GenDate: disallowed case 3");
    }
    return(GenNumber(year, 1, F_NOUN, F_MASCULINE, dc));
  }
  if (include_year) {
    if (!(pnyear = GenNumber(year, 1, F_NOUN, F_MASCULINE, dc))) return(NULL);
  } else pnyear = NULL;
  if (!(pnmonth = GenValueName(month, N("month-of-the-year"), F_NOUN, F_NULL,
                               F_SINGULAR, dc))) {
    return(NULL);
  }
  if (!include_day) return(GenTsEmbed(F_NP, pnmonth, pnyear));
  if (include_day_of_the_week) {
    pnday_of_the_week = GenValueName(day_of_the_week, N("day-of-the-week"),
                                     F_NOUN, F_NULL, F_SINGULAR, dc);
  } else pnday_of_the_week = NULL;
  if (!(pnday = GenNumber(day, 1, F_NOUN, F_MASCULINE, dc))) return(NULL);
  if (DC(dc).lang == F_ENGLISH) {
    PNodeAddPuncToEnd(pnday, ",");
    pn = GenTsEmbed(F_NP, PNodeConstit(F_NP, pnmonth, pnday), pnyear);
  } else {
    pn = GenTsEmbed(F_NP, PNodeConstit(F_NP,
                            PNodeConstit(F_NP,
                            GenMakeDet(N("definite-article"), F_MASCULINE,
                                       F_SINGULAR,
                                       F_NULL, dc),
                                         pnday),
                                       pnmonth), pnyear);
  }
  if (pnday_of_the_week) pn = PNodeConstit(F_NP, pnday_of_the_week, pn);
  return(pn);
}

PNode *GenHour(Float hour, Discourse *dc, /* RESULTS */ int *gender)
{
  int	is_pm;
  PNode	*pnhour, *pnhrword;
  if (hour == 24.0) hour = 0.0;
  *gender = F_NULL;
  if ((pnhrword = GenValueName(hour*SECONDSPERHOURF, N("hour-of-the-day"),
                               F_NOUN, F_NULL, F_SINGULAR, dc))) {
    *gender = FeatureGet(pnhrword->lexitem->features, FT_GENDER);
    return(PNodeConstit(F_NP, pnhrword, NULL));
  }
  if (DC(dc).lang == F_ENGLISH) {
    if (hour > 12.0) {
    /* Noon and midnight should have been handled above. */
      hour = hour - 12.0;
      is_pm = 1;
    } else {
      is_pm = 0;
    }
    if (!(pnhour = GenNumber((Float)hour, 0, F_NOUN, F_NULL, dc))) return(NULL);
    if (DC(dc).style == F_INFORMAL || DC(dc).style == F_SLANG) {
      return(PNodeConstit(F_NP, pnhour, NULL));
    }
    if (!(pnhrword = GenMakeAdverb(is_pm ? N("adverb-pm") : N("adverb-am"),
                                   F_POSITIVE, dc))) {
    /* todo: Option for N("adverb-o-clock"). */
      return(NULL);
    }
  } else {
    if (!(pnhrword = GenMakeNoun(N("hour"), (hour == 1.0) ? F_SINGULAR :
                                                            F_PLURAL, dc))) {
      return(NULL);
    }
    *gender = FeatureGet(pnhrword->lexitem->features, FT_GENDER);
    if (!(pnhour = GenNumber(hour, 0, F_ADJECTIVE,
                             FeatureGet(pnhrword->lexitem->features, FT_GENDER),
                             dc))) return(NULL);
  }
  return(PNodeConstit(F_NP, pnhour, pnhrword));
}

PNode *GenTod2(int hour, int min, int sec, Discourse *dc)
{
  int		gender;
  PNode		*pnhour, *pnfrac;
  Float		frac;
  if (hour == INTNAF && min == INTNAF && sec == INTNAF) return(NULL);
  frac = FloatRound(((Float)min)/60.0, .25);
  if (frac == 1.0) {
    frac = 0.0;
    hour++;
  }
  if (frac == 0.00) return(GenHour((Float)hour, dc, &gender));
  if (frac == 0.75) {
    frac = 1.0-frac;
    hour++;
    if (!(pnhour = GenHour((Float)hour, dc, &gender))) return(NULL);
    if (DC(dc).lang == F_FRENCH) {
      if (!(pnfrac = GenNumber(frac, 0, F_NOUN, F_MASCULINE, dc))) return(NULL);
      if (!(pnfrac = GenMakePP(N("prep-time-before-hour-french"), pnfrac, dc)))
        return(NULL);
      return(PNodeConstit(F_NP, pnhour, pnfrac));
    } else {
      if (!(pnfrac = GenNumber(frac, 0, F_NOUN, F_NULL, dc))) return(NULL);
      if (!(pnhour = GenMakePP(N("prep-time-before-hour-english"), pnhour, dc)))
        return(NULL);
      return(PNodeConstit(F_NP, pnfrac, pnhour));
    }
  } else {
    if (!(pnhour = GenHour((Float)hour, dc, &gender))) return(NULL);
    if (DC(dc).lang == F_FRENCH) {
      if (!(pnfrac = GenNumber(frac, 0, F_NOUN,
                               frac == 0.25 ? F_MASCULINE : gender, dc))) {
        return(NULL);
      }
      return(GenMakeConj(F_NP, N("and"), pnhour, pnfrac, dc));
    } else {
      if (!(pnfrac = GenNumber(frac, 0, F_NOUN, F_NULL, dc))) return(NULL);
      if (!(pnhour = GenMakePP(N("prep-time-after-hour"), pnhour, dc))) {
        return(NULL);
      }
      return(PNodeConstit(F_NP, pnfrac, pnhour));
    }
  }
}

PNode *GenTod1(Tod tod, Discourse *dc)
{
  int	hour, min, sec;
  TodToHMS(tod, &hour, &min, &sec);
  return(GenTod2(hour, min, sec, dc));
}

PNode *GenTod(Ts *ts, Discourse *dc)
{
  char	buf[WORDLEN];
  int	year, month, day, hour, min, sec;
  TsToYMDHMS(ts, &year, &month, &day, &hour, &min, &sec);
  if (dc->ga.tga.exactly) {
    if (hour != INTNAF && min != INTNAF && sec == INTNAF) {
      sprintf(buf, "%.2d:%.2d:%.2d", hour, min, sec);
    } else if (hour != INTNAF && min != INTNAF) {
      sprintf(buf, "%.2d:%.2d", hour, min);
    } else {
      return(NULL);
    }
    return(PNodeCreateString(StringCopy(buf, "char * GenTod exact")));
  } else {
    return(GenTod2(hour, min, sec, dc));
  }
}

PNode *GenDayAndPartOfTheDay(Ts *ts, int not_embedded, Discourse *dc)
{
  int		prep_day;
  PNode		*pn, *pnpod, *pndet, *pnday;
  Float		relday, tod;
  relday = (Float)TsDaysBetween(ts, DCNOW(dc));
  if (relday >= 7.0 || relday <= -7.0) return(NULL);
  tod = (Float)TsToTod(ts);
  if ((pn = GenDualValueRangeName(relday, tod,
                                  N("relative-day-and-part-of-the-day"),
                                  F_NOUN, F_NULL, F_SINGULAR, dc))) {
    return(pn);
  }
  if (relday != -2.0 && relday != 2.0) {
    pnpod = GenValueRangeName(tod, N("part-of-the-day"), F_NOUN, F_NULL,
                              F_SINGULAR, F_NULL, dc);
  } else {
    pnpod = NULL;
  }
  if (relday == 0.0 && pnpod) {
    if (!(pndet = GenMakeDet(N("det-this"),
                             FeatureGet(pnpod->lexitem->features, FT_GENDER),
                             FeatureGet(pnpod->lexitem->features, FT_NUMBER),
                             F_NULL, dc))) {
      return(NULL);
    }
    return(PNodeConstit(F_NP, pndet, pnpod));
  }
  if (!(pnday = GenValueName(relday, N("relative-day"), F_NOUN, F_NULL,
                             F_SINGULAR, dc))) {
    if (!(pnday = GenValueName((Float)TsToDay(ts), N("day-of-the-week"), F_NOUN, 
                               F_NULL, F_SINGULAR, dc))) {
      return(NULL);
    }
    prep_day = not_embedded;
  } else prep_day = 0;
  if (pnpod) pnday = PNodeConstit(F_NP, pnday, pnpod);
  if (prep_day &&
      (pn = GenMakePP(N("prep-temporal-position-day"), pnday, dc))) {
    return(pn);
  }
  return(pnday);
}

PNode *GenDurRelativeToNow(Ts *ts, int only_if_near, int use_word,
                           int not_embedded, Discourse *dc)
{
  Float			dur;
  PNode			*pn, *pn1, *pn2;
  dur = TsFloatMinus(ts, DCNOW(dc));
  if (only_if_near &&
      ((dur >= 3.0*SECONDSPERHOURF) || (dur <= -3.0*SECONDSPERHOURF))) {
    return(NULL);
  }
  if ((use_word || dur == 0.0) &&
      (pn = GenValueRangeName(dur, N("duration-relative-to-now"),
                              F_ADVERB, F_NULL,
                              F_NULL, F_NULL, dc))) {
    return(pn);
  }
  if ((pn1 = GenUnitOfMeasure(FloatAbs(dur), 1.0, N("absolute-unit-of-time"),
                              dc))) {
    if (dur > 0L) {
      if (not_embedded) {
        return(GenMakePP(N("prep-dur-relative-to-now"), pn1, dc));
      } else if (DC(dc).lang == F_FRENCH) {
        return(GenMakePP(N("prep-dur-relative-to-now-embedded"), pn1, dc));
      } else {
        if (!(pn2 = GenMakeAdverb(N("adverb-dur-end-embedded"), F_POSITIVE,
                                  dc)))
          return(NULL);
        return(PNodeConstit(F_ADVP, pn1, pn2));
      }
    } else {
      if (!(pn2 = GenMakeAdverb(N("adverb-ago"), F_POSITIVE, dc)))
        return(NULL);
      if (DC(dc).lang == F_FRENCH) return(PNodeConstit(F_ADVP, pn2, pn1));
      else return(PNodeConstit(F_ADVP, pn1, pn2));
    }
  }
  return(NULL);
}

PNode *GenDur1(Float dur, Discourse *dc)
{
  PNode	*pn1;
  if ((pn1 = GenUnitOfMeasure(FloatAbs(dur), 1.0, N("absolute-unit-of-time"),
                              dc))) {
    return(GenMakePP(N("prep-dur-for"), pn1, dc));
  }
  return(NULL);
}

PNode *GenDur(Obj *obj, Discourse *dc)
{
  Dur		dur;
  if (DURNA != (dur = TsRangeDur(ObjToTsRange(obj)))) {
    return(GenDur1((Float)dur, dc));
  }
  return(NULL);
}

/* UNIT OF MEASURE GENERATION */

PNode *GenUnitOfMeasure(Float val, Float round_to_nearest, Obj *meas_type,
                        Discourse *dc)
{
  int		i, len;
  Float		min, max, conv, cval;
  PNode		*pnadj, *pnnoun;
  Obj		*measure;
  for (i = 0, len = ObjNumChildren(meas_type); i < len; i++) {
    measure = ObjIthChild(meas_type, i);
    conv = ObjToNumber(R1EI(2, &TsNA, L(N("canonical-factor-of"), measure,
                                        ObjWild, E)));
    cval = val / conv;
    min = ObjToNumber(R1EI(2, &TsNA, L(N("gen-min-of"), measure, ObjWild, E)));
    if (min != FLOATNA && min > cval) continue;
    max = ObjToNumber(R1EI(2, &TsNA, L(N("gen-max-of"), measure, ObjWild, E)));
    if (max != FLOATNA && max < cval) continue;
    if (round_to_nearest != FLOATNA) cval = FloatRound(cval, round_to_nearest);
    if ((pnnoun = GenMakeNoun(measure, (cval == 1.0) ? F_SINGULAR :
                                                       F_PLURAL, dc)) &&
        (pnadj = GenNumber(cval, 0, F_ADJECTIVE,
                           FeatureGet(pnnoun->lexitem->features, FT_GENDER),
                           dc))) {
      return(PNodeConstit(F_NP, PNodeConstit(F_ADJP, pnadj, NULL), pnnoun));
    }
  }
  return(NULL);
}

/* VALUE NAME GENERATION */

/* todo: Reimplement looping as a single query if and when querying of values
 * becomes possible.
 */
Obj *GenValueObj(Float val, Obj *class)
{
  int		i, len;
  Float		val1;
  Obj		*obj;
  for (i = 0, len = ObjNumChildren(class); i < len; i++) {
    obj = ObjIthChild(class, i);
    val1 = ObjToNumber(R1EI(2, &TsNA, L(N("value-of"), obj, ObjWild, E)));
    if (val1 != val) continue;
    return(obj);
  }
  return(NULL);
}

/* todo: Reimplement looping as a single query if and when querying of values
 * becomes possible.
 */
PNode *GenValueName(Float val, Obj *class, int pos, int gender, int number,
                    Discourse *dc)
{
  int		i, len;
  char		features[2];
  Float		val1;
  Obj		*obj;
  ObjToLexEntry	*ole;
  Word		*infl;
  features[0] = pos;
  features[1] = TERM;
  for (i = 0, len = ObjNumChildren(class); i < len; i++) {
    obj = ObjIthChild(class, i);
    val1 = ObjToNumber(R1EI(2, &TsNA, L(N("value-of"), obj, ObjWild, E)));
    if (val1 != val) continue;
    if (!(ole = ObjToLexEntryGet1(obj, NULL, features, F_NULL, F_NULL,
                                  NULL, dc))) {
      continue;
    }
    if (!(infl = LexEntryGetInflection(ole->le, F_NULL, gender, number,
                                       F_NULL, F_NULL, F_NULL, 1, dc))) {
      continue;
    }
    return(PNodeWord(pos, infl->word, infl->features, ole->le, NULL));
  }
  return(NULL);
}

PNode *GenValueRangeName(Float val, Obj *class, int pos, int gender, int number,
                         int paruniv, Discourse *dc)
{
  int		i, len;
  char		features[2];
  Float		min, max;
  Obj		*obj;
  ObjToLexEntry	*ole;
  Word		*infl;
  features[0] = pos;
  features[1] = TERM;
  for (i = 0, len = ObjNumChildren(class); i < len; i++) {
    obj = ObjIthChild(class, i);
    min = ObjToNumber(R1EI(2, &TsNA, L(N("min-value-of"), obj, ObjWild, E)));
    if (min != FLOATNA && min > val) continue;
    max = ObjToNumber(R1EI(2, &TsNA, L(N("max-value-of"), obj, ObjWild, E)));
    if (max != FLOATNA && max < val) continue;
      /* REALLY: Has to be < so that min == max case produces a result. */
    if (!(ole = ObjToLexEntryGet1(obj, NULL, features, F_NULL, paruniv,
                                  NULL, dc))) {
      continue;
    }
    if (!(infl = LexEntryGetInflection(ole->le, F_NULL, gender, number,
                                       F_NULL, F_NULL, F_NULL, 1, dc))) {
      continue;
    }
    return(PNodeWord(pos, infl->word, infl->features, ole->le, obj));
  }
  return(NULL);
}

PNode *GenDualValueRangeName(Float val1, Float val2, Obj *class,
                             int pos, int gender, int number, Discourse *dc)
{
  int		i, len;
  char		features[2];
  Float		min1, max1, min2, max2;
  Obj		*obj;
  ObjToLexEntry	*ole;
  Word		*infl;
  features[0] = pos;
  features[1] = TERM;
  for (i = 0, len = ObjNumChildren(class); i < len; i++) {
    obj = ObjIthChild(class, i);
    min1 = ObjToNumber(R1EI(2, &TsNA, L(N("min-value1-of"), obj, ObjWild, E)));
    if (min1 != FLOATNA && min1 > val1) continue;
    max1 = ObjToNumber(R1EI(2, &TsNA, L(N("max-value1-of"), obj, ObjWild, E)));
    if (max1 != FLOATNA && max1 < val1) continue;
    min2 = ObjToNumber(R1EI(2, &TsNA, L(N("min-value2-of"), obj, ObjWild, E)));
    if (min2 != FLOATNA && min2 > val2) continue;
    max2 = ObjToNumber(R1EI(2, &TsNA, L(N("max-value2-of"), obj, ObjWild, E)));
    if (max2 != FLOATNA && max2 < val2) continue;
    if (!(ole = ObjToLexEntryGet1(obj, NULL, features, F_NULL, F_NULL,
                                  NULL, dc)))
      continue;
    if (!(infl = LexEntryGetInflection(ole->le, F_NULL, gender, number,
                                       F_NULL, F_NULL, F_NULL, 1, dc))) {
      continue;
    }
    return(PNodeWord(pos, infl->word, infl->features, ole->le, obj));
  }
  return(NULL);
}

/* HELPING FUNCTIONS */

PNode *GenMakeAdverb(Obj *type, int degree, Discourse *dc)
{
  LexEntry	*le;
  Word		*infl;
  if (!(le = ObjToLexEntryGet(type, F_ADVERB, F_NULL, dc))) return(NULL);
  if (!(infl = LexEntryGetInflection(le, F_NULL, F_NULL, F_NULL, F_NULL,
                                     F_NULL, degree, 1, dc))) return(NULL);
  return(PNodeWord(F_ADVERB, infl->word, infl->features, le, type));
}

PNode *GenMakeDet(Obj *type, int gender, int number, int person, Discourse *dc)
{
  LexEntry	*le;
  Word		*infl;
  if (!(le = ObjToLexEntryGet(type, F_DETERMINER, F_NULL, dc))) return(NULL);
  if (!(infl = LexEntryGetInflection(le, F_NULL, gender, number, person,
                                     F_NULL, F_NULL, 1, dc))) return(NULL);
  return(PNodeWord(F_DETERMINER, infl->word, infl->features, le, type));
}

PNode *GenMakeExpl(LexEntry *le, PNode *subjnp, Discourse *dc)
{
  Word		*infl;
  int		gender, number, person;

  gender = number = person = F_NULL;
  if (subjnp && le) {
    if ((streq(le->srcphrase, "se") &&
         F_PRONOUN == FeatureGet(le->features, FT_POS)) ||
        (streq(le->srcphrase, "himself") &&
         F_PRONOUN == FeatureGet(le->features, FT_POS))) {
    /* Inflect reflexives properly. */
      PNodeGetHeadNounFeatures(subjnp, 1, &gender, &number, &person);
      if (person == F_NULL) person = F_THIRD_PERSON;
    }
  }
  if (!(infl = LexEntryGetInflection(le, F_NULL, gender, number, person,
                                     F_NULL, F_NULL, 1, dc))) return(NULL);
  return(PNodeWord(F_EXPLETIVE, infl->word, infl->features, le, NULL));
}

PNode *GenMakeFrenchPossDet1(int owner_number, int owner_person,
                             int modified_gender, int modified_number,
                             Discourse *dc)
{
  Obj		*type;
  LexEntry	*le;
  Word		*infl;
  char		qfeat[FEATLEN];
  type = N("possessive-determiner");
  if (!(le = ObjToLexEntryGet(type, F_DETERMINER, F_NULL, dc))) {
    return(NULL);
  }
  qfeat[0] = owner_person;
  qfeat[1] = owner_number;
  qfeat[2] = modified_gender;
  qfeat[3] = modified_number;
  qfeat[4] = TERM;
  for (infl = le->infl; infl; infl = infl->next) {
    if (0 == strncmp(qfeat, infl->features, 4)) {
      return(PNodeWord(F_DETERMINER, infl->word, infl->features, le, type));
    }
  }
  Dbg(DBGGENER, DBGBAD, "GenMakeFrenchPossDet: no inflection %c%c%c%c",
      (char)owner_number, (char)owner_person, (char)modified_gender,
      (char)modified_number);
  return(NULL);
}

PNode *GenMakeFrenchPossDet(char *owner_feat, char *modified_feat,
                            Discourse *dc)
{
  int	owner_number, owner_person;
  int	modified_gender, modified_number;

  owner_number = FeatureGet(owner_feat, FT_NUMBER);
  owner_person = FeatureGet(owner_feat, FT_PERSON);
  modified_gender = FeatureGet(modified_feat, FT_GENDER);
  modified_number = FeatureGet(modified_feat, FT_NUMBER);
  return(GenMakeFrenchPossDet1(owner_number, owner_person, modified_gender,
                               modified_number, dc));
}

PNode *GenMakeEnglishPossDet(char *owner_feat, Discourse *dc)
{
  int	owner_gender, owner_number, owner_person;
  owner_gender = FeatureGet(owner_feat, FT_GENDER);
  owner_number = FeatureGet(owner_feat, FT_NUMBER);
  owner_person = FeatureGet(owner_feat, FT_PERSON);
  if (owner_gender == F_NULL) owner_gender = F_NEUTER;
  if (owner_number == F_NULL) owner_number = F_SINGULAR;
  if (owner_person == F_NULL) owner_person = F_THIRD_PERSON;
  if (owner_gender == F_NEUTER && owner_number == F_PLURAL) {
    /* There is no plural version of "its" in English. */
    owner_number = F_SINGULAR;
  }
  return(GenMakeDet(N("possessive-determiner"), owner_gender, owner_number,
                    owner_person, dc));
}

PNode *GenMakeNoun(Obj *obj, int number, Discourse *dc)
{
  ObjToLexEntry	*ole;
  Word			*infl;
  if (!(ole = ObjToLexEntryGet1(obj, NULL, "N", F_NULL, F_NULL, NULL,
                                dc))) {
    return(NULL);
  }
  if (StringIn(F_MASS_NOUN, ole->features)) number = F_SINGULAR;
  if (!(infl = LexEntryGetInflection(ole->le, F_NULL, F_NULL, number, F_NULL,
                                     F_NULL, F_NULL, 1, dc))) {
    return(NULL);
  }
  return(PNodeWord(F_NOUN, infl->word, infl->features, ole->le, obj));
}

PNode *GenMakeConj(int constit, Obj *conj, PNode *pn1, PNode *pn2,
                   Discourse *dc)
{
  LexEntry	*le;
  Word		*infl;
  GenAdvice	save_ga;

  save_ga = dc->ga;
  dc->ga.consistent = 1;
  le = ObjToLexEntryGet(conj, F_CONJUNCTION, F_NULL, dc);
  dc->ga = save_ga;

  if (!le) {
    Dbg(DBGGENER, DBGBAD, "GenMakeConj: no lexical entry for <%s>", M(conj));
    goto failure;
  }

  if (!(infl = LexEntryGetInflection(le, F_NULL, F_NULL, F_NULL, F_NULL,
                                     F_NULL, F_NULL, 1, dc))) {
    Dbg(DBGGENER, DBGBAD, "GenMakeConj: no inflection for <%s>", M(conj));
    goto failure;
  }
  return(PNodeConstit(constit, pn1,
              PNodeConstit(constit,
                           PNodeWord(F_CONJUNCTION, infl->word,
                                     infl->features, le, conj),
                           pn2)));
failure:
  return(PNodeConstit(constit, pn1, pn2));
}

PNode *GenMakeSubordConj(int constit, PNode *pn, Discourse *dc)
{
  LexEntry	*le;
  Word		*infl;
  Obj		*obj;
  obj = N("standard-subordinating-conjunction");
  if (!(le = ObjToLexEntryGet(obj, F_CONJUNCTION, F_NULL, dc))) {
    return(NULL);
  }
  if (!(infl = LexEntryGetInflection(le, F_NULL, F_NULL, F_NULL, F_NULL,
                                     F_NULL, F_NULL, 1, dc))) {
    return(NULL);
  }
  return(PNodeConstit(constit,
         PNodeConstit(F_S,
                      PNodeWord(F_CONJUNCTION, infl->word, infl->features,
                                le, obj),
                      pn),
                      NULL));
}

PNode *GenMakePronoun(Obj *type, Obj *concept, int gender, int number,
                      int person, Discourse *dc)
{
  LexEntry	*le;
  Word		*infl;
  if (!(le = ObjToLexEntryGet(type, F_PRONOUN, F_NULL, dc))) return(NULL);
  if (!(infl = LexEntryGetInflection(le, F_NULL, gender, number, person,
                                     F_NULL, F_NULL, 1, dc))) return(NULL);
  return(PNodeConstit(F_NP,
                      PNodeWord(F_PRONOUN, infl->word, infl->features, le,
                                concept),
                      NULL));
}

PNode *GenMakePrep(Obj *type, int gender, int number, Discourse *dc)
{
  LexEntry	*le;
  Word		*infl;
  if (!(le = ObjToLexEntryGet(type, F_PREPOSITION, F_NULL, dc))) return(NULL);
  if (!(infl = LexEntryGetInflection(le, F_NULL, gender, number, F_NULL,
                                     F_NULL, F_NULL, 1, dc))) return(NULL);
  return(PNodeWord(F_PREPOSITION, infl->word, infl->features, le, type));
}

PNode *GenMakeElement(Obj *type, Discourse *dc)
{
  LexEntry	*le;
  Word		*infl;
  if (!(le = ObjToLexEntryGet(type, F_ELEMENT, F_NULL, dc))) return(NULL);
  if (!(infl = le->infl)) return(NULL);
  return(PNodeWord(F_ELEMENT, infl->word, infl->features, le, type));
}

PNode *GenMakePP1(LexEntry *le, PNode *np, Discourse *dc)
{
  Word		*infl;
  if (!le) return(NULL);
  if (!(infl = LexEntryGetInflection(le, F_NULL, F_NULL, F_SINGULAR, F_NULL,
                                     F_NULL, F_NULL, 1, dc))) return(NULL);
  return(PNodeConstit(F_PP,
                      PNodeWord(F_PREPOSITION, infl->word, infl->features,
                                le, NULL),
                      np));
}

PNode *GenMakePP(Obj *type, PNode *np, Discourse *dc)
{
  LexEntry	*le;
  if (DC(dc).lang == F_FRENCH && type == N("prep-temporal-position-day")) {
    return(NULL);
  }
  if (!(le = ObjToLexEntryGet(type, F_PREPOSITION, F_NULL, dc))) return(NULL);
  return(GenMakePP1(le, np, dc));
}

/* HANDY-DANDY GENERATORS */

PNode *GenNounPrepNoun(Obj *det, Obj *obj1, Obj *prep, Obj *obj2, int number1,
                       int number2, Discourse *dc)
{
  PNode	*pn1, *pn2;
  if (!(pn1 = GenNP(FS_HEADNOUN, obj1, N("subj"), 0, F_NULL, det, NULL,
                    NULL, F_NULL, number1, F_NULL, N("present-indicative"),
                    0, 0, dc))) {
    return(NULL);
  }
  if (!(pn2 = GenNP(FS_HEADNOUN, obj2, N("subj"), 0, F_NULL, N("empty-article"),
                    NULL, NULL, F_NULL, number2, F_NULL,
                    N("present-indicative"), 0, 0, dc))) {
    return(NULL);
  }
  return(TransformGen(PNodeConstit(F_NP, pn1, GenMakePP(prep, pn2, dc)), dc));
}

PNode *GenIsa(Obj *obj, Obj *parent, Obj *comptense, Discourse *dc)
{
  PNode	*subjnp, *vp, *comp;
  if (!(subjnp = GenNP(FS_HEADNOUN, obj, N("subj"), 0, F_NULL,
                       DiscourseIsaArticle(dc),
                       NULL, NULL, F_NULL, F_NULL, F_NULL,
                       N("present-indicative"),
                       0, 0, dc))) {
    return(NULL);
  }
  if (!(vp = GenVP(ObjToLexEntryGet(N("standard-copula"), F_VERB, F_NULL, dc),
                   "", comptense, subjnp, NULL, dc))) {
    return(NULL);
  }
  if (ObjIsConcrete(obj)) {
    if (!(comp = GenNP(FS_HEADNOUN, parent, N("complement"), 0, F_NULL,
                       N("indefinite-article"), subjnp, subjnp, F_NULL, F_NULL,
                       F_NULL, N("present-indicative"), 0, 0, dc))) {
      return(NULL);
    }
  } else {
    if (!(comp = GenNounPrepNoun(N("indefinite-article"), N("idea-type"),
                                 N("prep-of"), parent, F_SINGULAR, F_SINGULAR,
                                 dc))) {
      return(NULL);
    }
  }
  return(TransformGen(PNodeConstit(F_S, subjnp, PNodeConstit(F_VP, vp, comp)),
                                   dc));
}

/* STRING GENERATORS */

void GenNounString(Obj *obj, char *pos_restrict, int paruniv, int lang,
                   int gender, int number, int person, int maxlen,
                   Obj *arttype, int ancestor_ok, int showgender,
                   Discourse *dc, /* RESULTS */ char *s)
{
  int		save;
  PNode		*pn;
  Text		*text;
  save = DC(dc).lang;
  DiscourseSetLang(dc, lang);
  pn = GenNP(pos_restrict, obj, N("complement"), 0, paruniv, arttype, NULL,
             NULL, gender, number, person, N("present-indicative"),
             ancestor_ok, 0, dc);
  pn = TransformGen(pn, dc);
  if (!pn) {
    s[0] = TERM;
    DiscourseSetLang(dc, save);
    return;
  }
  text = TextCreate(PHRASELEN, INTPOSINF, dc);
  PNodeTextPrint(text, pn, SPACE, showgender, 1, dc);
  PNodeFreeTree(pn);
  TextToString(text, maxlen, s);
/*
  StringElimLeadingBlanks(s);
*/
  StringElimTrailingBlanks(s);
  TextFree(text);
  DiscourseSetLang(dc, save);
}

void GenConceptString(Obj *obj, Obj *article, int pos, int paruniv, int lang,
                      int gender, int number, int person, int maxlen,
                      int ancestor_ok, int showgender, Discourse *dc,
                      /* RESULTS */ char *s)
{
  char		pos_restrict[2];
  s[0] = TERM;
  pos_restrict[0] = pos;
  pos_restrict[1] = TERM;
                                      /* Was DC(dc).lang. */
  GenNounString(obj, pos_restrict, paruniv, lang, gender, number, person,
                PHRASELEN, article, ancestor_ok, showgender, dc, s);
}

char *GenFeatAbbrevString(int feature, int force, Discourse *dc)
{
  LexEntry *le;
  if ((le = ObjToAbbrev(FeatToCon(feature), force, dc))) {
    return(le->srcphrase);
  } else {
    return("");
  }
} 

char *GenFeatAbbrevStringQuick(int feature)
{
  switch (feature) {
    case F_NP: return("NP");
    case F_VP: return("VP");
    case F_PP: return("PP");
    case F_S: return("S");
    case F_PRONOUN: return("Pro");
    case F_NOUN: return("N");
    case F_VERB: return("V");
    case F_ADJECTIVE: return("Adj");
    case F_PREPOSITION: return("P");
    case F_ELEMENT: return("Elem");
    case F_ADVERB: return("Adv");
    case F_DETERMINER: return("Det");
    case F_CONJUNCTION: return("Conj");
    case F_EXPLETIVE: return("Expl");
    case F_ADJP: return("AdjP");
    case F_ADVP: return("AdvP");
    case F_INTERJECTION: return("Interj");
    case F_S_POS: return("CannedSent");
    case F_PREFIX: return("Prefix");
    case F_SUFFIX: return("Suffix");
    default:
      return(GenFeatAbbrevString(feature, 1, StdDiscourse));
  }
}

char *GenConceptAbbrevString(Obj *con, int force, Discourse *dc)
{
  LexEntry *le;
  if ((le = ObjToAbbrev(con, force, dc))) {
    return(le->srcphrase);
  } else {
    return("");
  }
} 

void GenFeaturesAbbrevString(char *features, int canonize, int force,
                             char *except, Discourse *dc, int maxlen,
                             /* RESULTS */ char *out)
{
  int	notfirst;
  char	*s, *p, canon[FEATLEN];
  out[0] = TERM;
  notfirst = 0;
  if (canonize) {
    FeatCanon(features, canon);
  } else {
    StringCpy(canon, features, FEATLEN);
  }
  for (p = canon; *p; p++) {
    if (*p == F_NULL) continue;
    if (StringIn(*p, except)) continue;
    s = GenFeatAbbrevString(*((uc *)p), force, dc);
    if (s[0]) {
      if (notfirst) {
        StringAppendDots(out, " ", maxlen);
      }
      notfirst = 1;
      StringAppendDots(out, s, maxlen);
    }
  }
}

void GenFeatureName(int feature, int lang, int maxlen, Discourse *dc,
                    /* RESULTS */ char *feature_r, char *parent_r)
{
  Obj		*obj;
  if (!(obj = FeatToCon(feature))) {
    feature_r[0] = TERM;
    parent_r[0] = TERM;
    return;
  }
  GenConceptString(obj, N("empty-article"), F_NOUN, F_NULL, lang, F_NULL,
                   F_NULL, F_NULL, maxlen, 0, 1, dc, feature_r);
  if (!(obj = ObjGetAParent(obj))) {
    parent_r[0] = TERM;
    return;
  }
  GenConceptString(obj, N("empty-article"), F_NOUN, F_NULL, lang, F_NULL,
                   F_NULL, F_NULL, maxlen, 0, 1, dc, parent_r);
} 

void GenAppendNounGender(char *features, /* RESULTS */ char *out)
{
  int	gender;
  /* Hardcoded linguistic output M/F/PL. */
  if (F_NOUN == FeatureGet(features, FT_POS)) {
    if (F_NULL != (gender = FeatureGet(features, FT_GENDER))) {
      StringAppendChar(out, PHRASELEN, SPACE);
      StringAppendChar(out, PHRASELEN, gender);
    }
    if (F_PLURAL == FeatureGet(features, FT_NUMBER)) {
      if (F_NULL == gender) StringAppendChar(out, PHRASELEN, SPACE);
      StringAppendChar(out, PHRASELEN, 'P');
      StringAppendChar(out, PHRASELEN, 'L');
    }
  }
}

/* End of file. */
