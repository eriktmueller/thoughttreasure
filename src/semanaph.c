/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Anaphoric parser.
 *
 * 19951025: begun
 * 19951026: more work
 * 19951027: debugging output
 * 19951120: moved all intension evaluations out of Sem_Parse into
 *           Sem_Anaphora (genitives, names)
 *
 * todo:
 * - Nonanaphoric pronouns such as "it" in "take it upon oneself".
 *   (Handle this example with a phrasal verb including fixed "it"?)
 */

#include "tt.h"
#include "semanaph.h"
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
#include "synpnode.h"
#include "synxbar.h"
#include "utildbg.h"
#include "utillrn.h"

/* Anaphor */

Anaphor *AnaphorCreate(Obj *con, Obj *source_con, PNode *pn, int salience,
                       Anaphor *next)
{
  Anaphor	*an;
  an = CREATE(Anaphor);
  an->con = con;
  an->source_con = source_con;
#ifdef maxchecking
  if (pn && PNodeIsOK(pn)) {
    an->pn = pn;
  } else {
    an->pn = NULL;
  }
#else
  an->pn = pn;
#endif
  an->salience = salience;
  an->gender = F_NULL;
  an->number = F_NULL;
  an->person = F_NULL;
  an->next = next;
  return(an);
}

void AnaphorFree(Anaphor *an)
{
  MemFree(an, "Anaphor");
}

void AnaphorFreeAll(Anaphor *anaphors)
{
  Anaphor *n;
  while (anaphors) {
    n = anaphors->next;
    AnaphorFree(anaphors);
    anaphors = n;
  }
}

Anaphor *AnaphorCopyAll(Anaphor *anaphors)
{
  Anaphor	*r, *an;
  r = NULL;
  for (an = anaphors; an; an = an->next) {
    r = AnaphorCreate(an->con, an->source_con, an->pn, an->salience, r);
  }
  return(r);
}

/* Copies both <an1> and <an2>. */
Anaphor *AnaphorAppend(Anaphor *an1, Anaphor *an2)
{
  Anaphor	*r, *an;
  r = NULL;
  for (an = an1; an; an = an->next) {
    r = AnaphorCreate(an->con, an->source_con, an->pn, an->salience, r);
  }
  for (an = an2; an; an = an->next) {
    r = AnaphorCreate(an->con, an->source_con, an->pn, an->salience, r);
  }
  return(r);
}

Anaphor *AnaphorAppend3(Anaphor *an1, Anaphor *an2, Anaphor *an3)
{
  Anaphor	*r, *an;
  r = NULL;
  for (an = an1; an; an = an->next) {
    r = AnaphorCreate(an->con, an->source_con, an->pn, an->salience, r);
  }
  for (an = an2; an; an = an->next) {
    r = AnaphorCreate(an->con, an->source_con, an->pn, an->salience, r);
  }
  for (an = an3; an; an = an->next) {
    r = AnaphorCreate(an->con, an->source_con, an->pn, an->salience, r);
  }
  return(r);
}

Anaphor *AnaphorLast(Anaphor *an)
{
  while (an->next) an = an->next;
  return(an);
}

Anaphor *AnaphorAppendDestructive(Anaphor *an1, Anaphor *an2)
{
  if (an1) {
    AnaphorLast(an1)->next = an2;
    return(an1);
  } else return(an2);
}

void AnaphorPrint(FILE *stream, Anaphor *an, int show_pn)
{
  fprintf(stream, "anaphor <%s> <%s> %d",
          M(an->con), M(an->source_con), an->salience);
  if (an->pn) {
    fputc(SPACE, stream);
    PNodePrint1(stream, NULL, an->pn, 0);
  }
}

void AnaphorPrintAll(FILE *stream, Anaphor *anaphors)
{
  Anaphor	*an;
  for (an = anaphors; an; an = an->next) {
    AnaphorPrint(stream, an, 1);
    fputc(NEWLINE, stream);
  }
}

void AnaphorPrintAllLine(FILE *stream, Anaphor *anaphors)
{
  Anaphor	*an;
  for (an = anaphors; an; an = an->next) {
    AnaphorPrint(stream, an, 0);
    fputc(SPACE, stream);
  }
}

ObjList *Sem_AnaphoraObject(Obj *con, Obj *source_con, PNode *pn, ObjList *r)
{
  Anaphor	*anaphors;
  anaphors = AnaphorCreate(con, source_con, pn, SALIENCE_MINIMAL, NULL);
  r = ObjListCreateSP(con, SCORE_MAX, NULL, pn, anaphors, r);
  return(r);
}

void Sem_AnaphoraObjListSet(ObjList *objs, Obj *source_con, PNode *pn)
{
  ObjList	*p;
  Anaphor	*anaphors;
  for (p = objs; p; p = p->next) {
    anaphors = AnaphorCreate(p->obj, source_con, pn, SALIENCE_MINIMAL, NULL);
    p->u.sp.anaphors = anaphors;
  }
}

Anaphor *ObjAnaphorBuild(Obj *obj, PNode *obj_pn, Anaphor *r)
{
  int	i, len;
  if (!ObjIsList(obj)) {
    return(AnaphorCreate(obj, obj, obj_pn, SALIENCE_MINIMAL, r));
  }
  for (i = 0, len = ObjLen(obj); i < len; i++) {
    r = ObjAnaphorBuild(I(obj, i), PNI(obj, i), r);
  }
  return(r);
}

void ObjListAnaphorAdd(ObjList *objs)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    p->u.sp.anaphors = ObjAnaphorBuild(p->obj, NULL, NULL);
  }
}

/******************************************************************************
 * DEICTICS
 *
 * See (Lyons, 1977, pp. 636-690)
 * and (Moeschler and Reboul, 1994, p. 359).
 *
 * todo: Should deictics be moved into the Context, since (despite their
 * definition as being an obvious, immediate part of the context) they
 * too are subject to alternative interpretations (especially across
 * electronic communication channels)?
 ******************************************************************************/

ObjList *Sem_AnaphoraDeictic(PNode *pn, ObjList *in, int gender, int number,
                             ObjList *r)
{
  int		len;
  ObjList	*p;
  Obj		*elems[MAXLISTLEN];
  if (number == F_SINGULAR) {
    for (p = in; p; p = p->next) {
      /* REDUNDANT: Deictic pronouns do not have gender in English or French. */
      if (FeatureMatch(gender, p->u.sl.gender)) {
        r = ObjListCreateSP(p->obj, SCORE_MAX, NULL, pn, NULL, r);
      }
    }
  } else {
    len = 1;
    elems[0] = N("and");
    for (p = in; p; p = p->next) {
      /* REDUNDANT: Deictic pronouns do not have gender in English or French. */
      if (FeatureMatch(gender, p->u.sl.gender)) {
        if (len < MAXLISTLEN) {
          elems[len] = p->obj;
          len++;
        } else {
          Dbg(DBGGEN, DBGBAD, "Sem_AnaphoraDeictic: increase MAXLISTLEN");
        }
      }
    }
    if (len == 2) {
      r = ObjListCreateSP(elems[1], SCORE_MAX, NULL, pn, NULL, r);
    } else {
      r = ObjListCreateSP(ObjCreateList1(elems, len), SCORE_MAX, NULL, pn,
                          NULL, r);
    }
  }
  return(r);
}

/******************************************************************************
 * PRONOUNS
 ******************************************************************************/

Bool Sem_AnaphoraMatch(int ant_salience,
                       int anaphor_gender, int ant_gender,
                       int anaphor_number, int ant_number,
                       int anaphor_person, int ant_person)
{
  return(ant_salience > 0 &&
         FeatureMatch(anaphor_gender, ant_gender) &&
         FeatureMatch(anaphor_number, ant_number) &&
         FeatureMatch(anaphor_person, ant_person));
}

/* <anaphor_number> assumed F_PLURAL. */
Bool Sem_AnaphoraPluralMatch(int ant_salience,
                             int anaphor_gender, int antecedent_gender,
                             int anaphor_number, int antecedent_number,
                             int anaphor_person, int antecedent_person)
{
  if (F_SINGULAR != antecedent_number) {
  /* We pass on this here because it should have already been matched
   * by Sem_AnaphoraMatch.
   */
    return(0);
  }
  if (anaphor_gender == F_FEMININE && antecedent_gender == F_MASCULINE) {
  /* In French, F_MASCULINE matches F_FEMININE, but F_FEMININE
   * does not match F_MASCULINE. In the speech of young children
   * "ils" is being generalized to mean "they", a plural genderless
   * pronoun which can be applied to a group of women.
   * This will never apply in English since plural pronouns do not
   * have gender.
   * todo: F_MASCULINE matches F_FEMININE only if at least 1 F_MASCULINE.
   */
    return(0);
  }
  return(ant_salience > 0 &&
         FeatureMatch(antecedent_person, anaphor_person));
}

/* "Jim thinks he is really funny."
 * pronoun: subject-pronoun
 * obj: [think Jim [funny Jim 1.0]]
 * todo: But this does not work when intensions such as names have not
 * yet been resolved. (cf "Jim Garnier was in his apartment.")
 */
ObjList *Sem_AnaphoraPronounIntrasentential1(Obj *pronoun, PNode *pronoun_pn,
                                             Obj *class, int pronoun_gender,
                                             int pronoun_number,
                                             int pronoun_person,
                                             Context *cx,
                                             Obj *obj, PNode *obj_pn,
                                             ObjList *r, /* RESULTS */
                                             ObjList **plurals)
{
  int		i, len, obj_gender, obj_number, obj_person;
  Anaphor	*anaphors;
  if (ObjIsNa(obj)) return(r);
  if (ObjIsList(obj)) {
    for (i = 1, len = ObjLen(obj); i < len; i++) {
      r = Sem_AnaphoraPronounIntrasentential1(pronoun, pronoun_pn, class,
                                              pronoun_gender, pronoun_number,
                                              pronoun_person, cx, I(obj, i),
                                              PNI(obj, i), r, plurals);
    }
  } else if (ContextIsAntecedent(obj, cx)) {
    if (ISAP(class, obj)) {
      PNodeGetHeadNounFeatures(obj_pn, 0, &obj_gender, &obj_number,
                               &obj_person);
      if (Sem_AnaphoraMatch(1, pronoun_gender, obj_gender,
                            pronoun_number, obj_number,
                            pronoun_person, obj_person)) {
        anaphors = AnaphorCreate(obj, pronoun, pronoun_pn, SALIENCE_MINIMAL,
                                 NULL);
        r = ObjListCreateSP(obj, SCORE_MAX, NULL, pronoun_pn, anaphors, r);
      }
      if (pronoun_number == F_PLURAL &&
          Sem_AnaphoraPluralMatch(1, pronoun_gender, obj_gender,
                                  pronoun_number, obj_number,
                                  pronoun_person, obj_gender)) {
        *plurals = ObjListCreate(obj, *plurals);
      }
    }
  }
  return(r);
}

/* This routines handles forward-looking or anticipatory anaphora
 * (cataphora) as well as backward-looking anaphora, all within
 * a sentence.
 */
ObjList *Sem_AnaphoraPronounIntrasentential(Obj *pronoun, PNode *pronoun_pn,
                                            Obj *class,
                                            int pronoun_gender,
                                            int pronoun_number,
                                            int pronoun_person,
                                            Context *cx, Obj *obj,
                                            PNode *obj_pn, ObjList *r)
{
  Obj		*and_obj;
  ObjList	*plurals;
  Anaphor	*anaphors;
  plurals = NULL;
  r = Sem_AnaphoraPronounIntrasentential1(pronoun, pronoun_pn, class,
                                          pronoun_gender, pronoun_number,
                                          pronoun_person, cx, obj, obj_pn,
                                          r, &plurals);
  if (plurals) {
    and_obj = ObjListToAndObj(plurals);
    anaphors = AnaphorCreate(and_obj, pronoun, pronoun_pn, SALIENCE_MINIMAL,
                             NULL);
    r = ObjListCreateSP(and_obj, SCORE_MAX, NULL, pronoun_pn, anaphors, r);
  }
  ObjListFree(plurals);
  return(r);
}

/* "Jim is funny. He really is."
 * <cx> contains Actor Jim.
 */
ObjList *Sem_AnaphoraPronounIntersentential(Obj *pronoun, PNode *pn, Obj *class,
                                            int gender, int number, int person,
                                            Context *cx, int max_sent_dist,
                                            ObjList *r)
{ 
  int		salience;
  Obj		*and_obj;
  ObjList	*objs;
  Actor		*ac;
  Antecedent	*ant;
  Anaphor	*anaphors;
  for (ac = cx->actors; ac; ac = ac->next) {
    if (!ISAP(class, ac->actor)) continue;
    ant = &ACTOR_ANTECEDENT(ac, cx);
    salience = SALIENCE(ant);
    if (Sem_AnaphoraMatch(salience, gender, ant->gender,
                          number, ant->number,
                          person, ant->person)) {
/* todoSCORE: Use salience to order processing or favor interpretations
 * if they otherwise have equal sense.
 */
      anaphors = AnaphorCreate(ac->actor, pronoun, pn, salience, NULL);
      r = ObjListCreateSP(ac->actor, salience, NULL, pn, anaphors, r);
    }
  }
  if (number == F_PLURAL) {
    objs = NULL;
    for (ac = cx->actors; ac; ac = ac->next) {
      if (!ISAP(class, ac->actor)) continue;
      ant = &ACTOR_ANTECEDENT(ac, cx);
      salience = SALIENCE(ant);
      if (Sem_AnaphoraPluralMatch(salience, gender, ant->gender,
                                  number, ant->number,
                                  person, ant->gender)) {
        objs = ObjListCreate(ac->actor, objs);
      }
    }
    if (objs) {
      and_obj = ObjListToAndObj(objs);
      anaphors = AnaphorCreate(and_obj, pronoun, pn, salience, NULL);
      r = ObjListCreateSP(and_obj, SCORE_MAX, NULL, pn, anaphors, r);
    }
    ObjListFree(objs);
  }
  return(r);
}

ObjList *Sem_AnaphoraPronoun2(Obj *pronoun, PNode *pn, Obj *class, Context *cx,
                              int gender, int number, int person, Obj *topcon,
                              int max_sent_dist, ObjList *r)
{
  r = Sem_AnaphoraPronounIntersentential(pronoun, pn, class, gender, number,
                                         person, cx, max_sent_dist, r);
  r = Sem_AnaphoraPronounIntrasentential(pronoun, pn, class, gender, number,
                                         person, cx, topcon, NULL, r);
  return(r);
}

ObjList *Sem_AnaphoraPronoun1(Obj *pronoun, PNode *pn, Context *cx,
                              int gender, int number, int person, Obj *topcon)
{
  ObjList	*r;
  r = NULL;
  if (DC(cx->dc).lang == F_FRENCH) {
    r = Sem_AnaphoraPronoun2(pronoun, pn, N("human"), cx, gender, number,
                             person, topcon, 3, r);
    if (!ISA(N("subject-pronoun"), pronoun)) {
    /* todo: For subject pronouns this causes (1) too many parses, (2) reduced
     * use of pronouns in generation. I think we need to treat this as an
     * exception.
     */
      r = Sem_AnaphoraPronoun2(pronoun, pn, N("nonhuman"), cx, gender, number,
                               person, topcon, 1, r);
    }
  } else {
    if (pronoun == N("pronoun-there-location")) {
      r = Sem_AnaphoraPronoun2(pronoun, pn, N("polity"), cx, gender, number,
                               person, topcon, 2, r);
      r = Sem_AnaphoraPronoun2(pronoun, pn, N("educational-institution"),
                               cx, gender, number, person, topcon, 2, r);
    } else if (gender == F_NEUTER && number == F_SINGULAR) {
    /* it, itself */
      r = Sem_AnaphoraPronoun2(pronoun, pn, N("nonhuman"), cx, gender, number,
                               person, topcon, 2, r);
    } else {
    /* he, she */
      r = Sem_AnaphoraPronoun2(pronoun, pn, N("animal"), cx, gender, number,
                               person, topcon, 2, r);
      if (number == F_PLURAL) {
      /* they */
        r = Sem_AnaphoraPronoun2(pronoun, pn, N("nonhuman"), cx, gender, number,
                                 person, topcon, 2, r);
      }
      if (gender == F_FEMININE) {
        r = Sem_AnaphoraPronoun2(pronoun, pn, N("ship"), cx, gender, number,
                                 person, topcon, 1, r);
        r = Sem_AnaphoraPronoun2(pronoun, pn, N("starship"), cx, gender, number,
                                 person, topcon, 1, r);
        r = Sem_AnaphoraPronoun2(pronoun, pn, N("motor-vehicle"), cx, gender,
                                 number, person, topcon, 1, r);
      }
    }
  }
  return(r);
}

/* Note this returns NULL if it finds no antecedents.
 * Includes deictic.
 */
ObjList *Sem_AnaphoraPronoun0(Obj *pronoun, PNode *pn, Context *cx, int gender,
                              int number, int person, Obj *topcon)
{
  if (person == F_FIRST_PERSON) {
  /* todo: Might want to handle groups differently. */
    return(Sem_AnaphoraDeictic(pn, DCSPEAKERS(cx->dc), gender, number, NULL));
  } else if (person == F_SECOND_PERSON) {
    return(Sem_AnaphoraDeictic(pn, DCLISTENERS(cx->dc), gender, number, NULL));
  } else {
  /* person == F_THIRD_PERSON || person == F_NULL */
    if (topcon == NULL) {
#ifdef INTEGSYNSEMANA
    /* Called after Sem_Parse of subtree. Pronouns aren't resolved here.
     * They will be resolved on a later second pass.
     */
      return(ObjListCreateSP(pronoun, SCORE_MAX, NULL, pn, NULL, NULL));
#else
      Dbg(DBGGEN, DBGBAD, "Sem_AnaphoraPronoun0: NULL topcon");
#endif
    }
    return(Sem_AnaphoraPronoun1(pronoun, pn, cx, gender, number, person,
                                topcon));
  }
}

/* Note this doesn't ever return NULL.
 * Includes deictic.
 * todo: look for matching script objects:
 * for (sg = ac->subgoals; sg; sg = sg->next)
 *   if (ContextGetObjectRels(&cx->story_time, sg->obj, obj))
 *     add Anaphor
 */
ObjList *Sem_AnaphoraPronoun(Obj *pronoun, PNode *pn, Context *cx, Obj *topcon)
{
  int		gender, number, person;
  ObjList	*r;
  PNodeGetHeadNounFeatures(pn, 0, &gender, &number, &person);
  if (ISA(N("expletive-pronoun"), pronoun)) {
    return(Sem_AnaphoraObject(pronoun, pronoun, pn, NULL));
  }
  if ((r = Sem_AnaphoraPronoun0(pronoun, pn, cx, gender, number, person,
                                topcon))) {
    return(r);
  } else {
    /* todo: Ask "Who?" or "What?". */
    Dbg(DBGGEN, DBGDETAIL, "can't find antecedents for <%s> <%c%c%c>",
        M(pronoun), gender, number, person);
    return(NULL);
#ifdef notdef
    /* Why return stuff that nobody down the road knows how to use?:
     * That is, pronouns and articles are only used by Sem_Anaphora.
     * such-that's, however, are used by UAs such as UA_Answer.
     */
    return(Sem_AnaphoraObject(pronoun, pronoun, pn, NULL));
#endif
  }
}

/* Note this returns NULL if it finds no antecedents.
 * Includes deictic.
 */
ObjList *Sem_AnaphoraPronoun_AnFree(Obj *pronoun, PNode *pn, Context *cx,
                                    int gender, int number, int person,
                                    Obj *topcon)
{
  ObjList	*r, *p;
  r = Sem_AnaphoraPronoun0(pronoun, pn, cx, gender, number, person, topcon);
  for (p = r; p; p = p->next) {
    AnaphorFreeAll(p->u.sp.anaphors);
  }
  return(r);
}

/******************************************************************************
 * DETERMINERS
 ******************************************************************************/

ObjList *Sem_AnaphoraDefiniteArticle(Obj *det, Obj *det_arg, PNode *det_pn,
                                     PNode *det_arg_pn, Context *cx,
                                     Obj *topcon)
{
  /* We only look in the context for this. Otherwise, we will get way
   * too many matches (e.g., "the American"=->every known American).
   * If not found, we create a new such object.
   */
  if (N("such-that") == I(det_arg, 0)) Stop();
#ifdef notdef
  if (N("such-that") == I(det_arg, 0)) {
    return(Sem_AnaphoraIntensionParse(det_arg, det_arg_pn, cx, 1, 0, topcon));
  }
#endif
  return(Sem_AnaphoraIntensionParse1(det_arg, NULL, det_arg, det_arg_pn,
                                     cx, 1, 0, topcon));
}

ObjList *Sem_AnaphoraIndefiniteArticle(Obj *det, Obj *det_arg, PNode *det_pn,
                                       PNode *det_arg_pn, Context *cx,
                                       Obj *topcon)
{
  ObjList	*r;
  if (N("such-that") == I(det_arg, 0)) Stop();
#ifdef notdef
  if (N("such-that") == I(det_arg, 0)) {
  /* This should always refer to a new entity. Exceptions? */
    return(Sem_AnaphoraIntensionParse(det_arg, det_arg_pn, cx, 1, 1, topcon));
  }
#endif
  if (ObjIsList(det_arg)) {
  /* [SPARCstationfest] */
    return(Sem_AnaphoraObject(det_arg, det_arg, det_pn, NULL));
  }
  r = Sem_AnaphoraIntensionParseNew(det_arg, NULL, det_arg, det_arg_pn,
                                    cx, topcon);
  /* Return extension+intension. "I want to buy a car." */
  return(Sem_AnaphoraObject(det_arg, det_arg, det_pn, r));
}

void PNodeGetGNP_Possessive(PNode *pn, int lang, /* RESULTS */ int *gender,
                            int *number, int *person)
{
  PNodeGetGenderNumberPerson(pn, gender, number, person);
    /* Not PNodeGetHeadNounFeatures because this isn't a noun/pronoun. */
  if (lang == F_FRENCH) *gender = *number = F_NULL; /* todo: number */
}

ObjList *Sem_AnaphoraPossessiveDeterminer(Obj *det, Obj *det_arg, PNode *det_pn,
                                          PNode *det_arg_pn, Context *cx,
                                          Obj *topcon,
                                          /* RESULTS */ int *do_suchthat)
{
  int		accepted, gender, number, person;
  ObjList	*p, *r, *objs;
  PNodeGetGNP_Possessive(det_pn, DC(cx->dc).lang, &gender, &number, &person);
  if (DC(cx->dc).lang == F_FRENCH) gender = number = F_NULL; /* todo: Number. */
  objs = Sem_AnaphoraPronoun_AnFree(det, det_pn, cx, gender, number,
                                    person, topcon);
  if (objs == NULL) {
    Dbg(DBGGEN, DBGDETAIL,
        "can't find possessive antecedents for <%s> <%c%c%c>",
        M(det), gender, number, person);
    return(Sem_AnaphoraParse(det_arg, det_arg_pn, cx, topcon));
  }
  r = NULL;
  for (p = objs; p; p = p->next) {
  /* ... Peter ... "his family"
   * p->obj:  N("Peter")
   * det_arg: N("family")
   */
    *do_suchthat = 1;
    r = Sem_ParseGenitive(det_arg, p->obj, p->u.sp.score, p->u.sp.leo,
                          p->u.sp.pn, p->u.sp.anaphors, cx->dc, r, &accepted);
       /* Note the above dereferences if it can. */
  }
  ObjListFree(objs);
  ObjListAnaphorAdd(r);
  if (r == NULL) {
    Dbg(DBGGEN, DBGDETAIL, "can't parse genitive <%s> of <%s> ...",
        M(det_arg), M(objs->obj));
    return(Sem_AnaphoraParse(det_arg, det_arg_pn, cx, topcon));
  }
  return(r);
}

/*
 * "my brother"  [possessive-determiner brother]
 * "his brother" [possessive-determiner brother]
 * "the cat"     [definite-article cat]
 * "a cat"       [indefinite-article cat]
 */
ObjList *Sem_AnaphoraDeterminer(Obj *det, Obj *det_arg, PNode *det_pn,
                                PNode *det_arg_pn, Context *cx,
                                Obj *topcon,
                                /* RESULTS */ int *do_suchthat)
{
  *do_suchthat = 0;
  if (ISA(N("possessive-determiner"), det)) {
    return(Sem_AnaphoraPossessiveDeterminer(det, det_arg, det_pn, det_arg_pn,
                                            cx, topcon, do_suchthat));
  } else if (ISA(N("definite-article"), det)) {
    return(Sem_AnaphoraDefiniteArticle(det, det_arg, det_pn, det_arg_pn,
                                       cx, topcon));
  } else if (ISA(N("indefinite-article"), det)) {
    return(Sem_AnaphoraIndefiniteArticle(det, det_arg, det_pn, det_arg_pn,
                                         cx, topcon));
  } else {
    Dbg(DBGGEN, DBGBAD, "Sem_AnaphoraDeterminer");
    return(Sem_AnaphoraParse(det_arg, det_arg_pn, cx, topcon));
  }
}

/******************************************************************************
 * ANAPHORA
 ******************************************************************************/

LexEntryToObj *PNodeToLeo(PNode *pn)
{
  if (pn && pn->lexitem && pn->lexitem->le) {
    return(pn->lexitem->le->leo);
  }
  return(NULL);
}

Bool Sem_AnaphoraIsCopulaPossessive(Obj *obj, /* RESULTS */
                                    Obj **det, PNode **det_pn,
                                    Obj **subj, PNode **subj_pn,
                                    Obj **rel, PNode **rel_pn,
                                    LexEntryToObj **copula_leo)
{
  if (ISA(N("copula"), I(obj, 0))) {
    *copula_leo = PNodeToLeo(PNI(obj, 0));
    if (ISA(N("possessive-determiner"), I(I(obj, 2), 0))) {
    /* [standard-copula Peter [possessive-determiner friend-of]] */
      *det = I(I(obj, 2), 0);
      *det_pn = PNI(I(obj, 2), 0);
      *subj = I(obj, 1);
      *subj_pn = PNI(obj, 1);
      *rel = I(I(obj, 2), 1);
      *rel_pn = PNI(I(obj, 2), 1);
      return(1);
    } else if (ISA(N("possessive-determiner"), I(I(obj, 1), 0))) {
    /* [standard-copula [possessive-determiner friend-of] Peter] */
      *det = I(I(obj, 1), 0);
      *det_pn = PNI(I(obj, 1), 0);
      *subj = I(obj, 2);
      *subj_pn = PNI(obj, 2);
      *rel = I(I(obj, 1), 1);
      *rel_pn = PNI(I(obj, 1), 1);
      return(1);
    }
  }
  return(0);
}

/*
 * det:  N("possessive-determiner")
 * subj: N("Clinton")
 * rel:  N("President-of")
 */
ObjList *Sem_AnaphoraParseCopulaPossessive(Obj *det, PNode *det_pn,
                                           Obj *subj, PNode *subj_pn,
                                           Obj *rel, PNode *rel_pn,
                                           LexEntryToObj *copula_leo,
                                           Context *cx, Obj *topcon)
{
  int		gender, number, person;
  ObjList	*iobjs, *p, *r;
  LexEntryToObj *rel_leo;
  SP		subj_sp, rel_sp;

  rel_leo = PNodeToLeo(rel_pn);
  PNodeGetGNP_Possessive(det_pn, DC(cx->dc).lang, &gender, &number, &person);
  iobjs = Sem_AnaphoraPronoun_AnFree(det, det_pn, cx, gender, number,
                                     person, topcon);
  r = NULL;
  for (p = iobjs; p; p = p->next) {
    SPInit(&subj_sp);
    subj_sp.pn = subj_pn;
    SPInit(&rel_sp);
    rel_sp.pn = rel_pn;
    rel_sp.leo = rel_leo;
    r = Sem_ParseS_EquativeRole1(SCORE_MAX, subj, &subj_sp, NULL, rel, &rel_sp,
                                 p->obj, &p->u.sp, copula_leo, cx->dc);
  }
  ObjListAnaphorAdd(r);
  return(r);
}

void Sem_AnaphoraTs(Ts *ts, Context *cx)
{
  Ts	newts;
  if (TsIsNa(ts)) return;
  if (ts->flag & TSFLAG_TOD) {
    if (ts->flag & TSFLAG_NOW) {
      if (!TsIsNa(DCNOW(cx->dc))) {
        TsMidnight(DCNOW(cx->dc), &newts);
        TsIncrement(&newts, (Dur)ts->unixts);
        *ts = newts;
      }
    } else {
    /* Assume story time is being referred to. */
      if (!TsIsNa(&cx->story_time.stopts)) {
        TsMidnight(&cx->story_time.stopts, &newts);
        TsIncrement(&newts, (Dur)ts->unixts);
        *ts = newts;
      }
    }
  }
}

ObjList *Sem_AnaphoraTime(ObjList *objs, Context *cx)
{
  ObjList	*p;
  TsRange	*tsr;
  for (p = objs; p; p = p->next) {
    tsr = ObjToTsRange(p->obj);
    Sem_AnaphoraTs(&tsr->startts, cx);
    Sem_AnaphoraTs(&tsr->stopts, cx);
  }
  return(objs);
}

/* todo: This needs to be Cartesian productified.
   See ObjListListCartesianProduct
ObjList *Sem_AnaphoraGroup(Obj *group, PNode *pn, Context *cx, Obj *topcon)
{
  ObjList	*p, *members;
  members = GroupMembers(&TsNA, group);
  for (p = members; p; p = p->next) {
    r1 = Sem_AnaphoraParse(p->obj, pn, cx, topcon);
  }
  ObjListFree(members);
}
 */

void Sem_AnaphoraIntensionParseNew1(Obj *class, ObjList *props, Obj *newobj,
                                    Context *cx, Obj *skip_this_prop)
{
  Obj		*assertion, *elem;
  ObjList	*p;
  for (p = props; p; p = p->next) {
    if (skip_this_prop == p->obj) continue;
    elem = PropFix(p->obj);
  /* todo: Wait a minute: Are we allowed to assert into the context here?
   * What about alternatives?!
   * todo: if (N("of") == I(elem, 0)) (= genitive), we need to figure out
   * the exact relation.
   */
    assertion = ObjSubst(elem, class, newobj);
    /* todo: Should isa be handled in DbAssert or here? See UA_Relation. */
    DbAssert(&cx->story_time, assertion);
  }
}

Obj *PropFix(Obj *obj)
{
  if (ISA(N("tense"), I(obj, 0))) {
    return(I(obj, 1));
  } else {
    return(obj);
  }
}

Bool PropsIsOK(ObjList *props)
{
  Obj		*elem;
  ObjList	*p;
  for (p = props; p; p = p->next) {
    elem = PropFix(p->obj);
    if (N("isa") == I(elem, 0)) {
      if (ObjIsConcrete(I(elem, 1)) ||
          ObjIsConcrete(I(elem, 2))) {
        return(0);
      }
    }
    if (N("ako") == I(elem, 0)) {
      if (ObjIsAbstract(I(elem, 1)) ||
          ObjIsConcrete(I(elem, 2))) {
        return(0);
      }
    }
  }
  return(1);
}

/* text                  class    props
 * --------------------- -------- ---------------------
 * "Amy Newton's mother" human    [mother-of Amy-Newton human]
 * "Amy Newton"          human    [NAME-of human NAME:"Amy Newton"]
 * "cat"                 cat      NULL
 */
ObjList *Sem_AnaphoraIntensionParseNew(Obj *class, ObjList *props,
                                       Obj *source_con, PNode *pn,
                                       Context *cx, Obj *topcon)
{
  Obj		*skip_this_prop, *newobj;
  Name		*nm;
  if (ObjListInDeep(props, N("such-that"))) {
  /* This is intended to eliminate the creation of new objects as
   * a result of passing through the such-that (for QA).
   * "Jim Garnier's sister daydreams."
   */
    return(NULL);
  }
  if (class == N("string")) {
  /* It is not very useful to create a dummy empty string. */
    return(NULL);
  }
  if (!PropsIsOK(props)) {
  /* "The American who is my sister, ate." */
    return(NULL);
  }

  if ((skip_this_prop = ObjListRelFind(props, N("NAME-of")))) {
    nm = ObjToHumanName(I(skip_this_prop, 2));
    newobj = LearnHuman(nm, nm->fullname, cx->dc, NULL);
  } else if (ISA(N("human"), class) && (!ObjIsConcrete(class))) {
    newobj = LearnHuman(NULL, "<name unknown>", cx->dc, NULL);
  } else {
  /* This is probably not enough information to create an instance.
   * Too many spurious instances are being created.
   * "What is her occupation?"
   * @19960115000000:19960115000001#34435|[occupation-of Jeanne-P-chl
   *                                       occupation2633
   */
    return(NULL);
/*
    newobj = ObjCreateInstance(class, NULL);
 */
  }
  Sem_AnaphoraIntensionParseNew1(class, props, newobj, cx, skip_this_prop);
  return(Sem_AnaphoraObject(newobj, source_con, pn, NULL));
}

/* text                    class   props
 * ----------------------- ------- -----
 * "the store"             store   NULL
 * "a store that X owns"   store   [owner-of store X]
 * "the store that X owns" store   [owner-of store X]
 * "Jim Garnier"           human   [NAME-of human NAME:"Jim Garnier"]
 */
ObjList *Sem_AnaphoraIntensionParse1(Obj *class, ObjList *props,
                                     Obj *source_con, PNode *pn,
                                     Context *cx, int cx_only,
                                     int new_only, Obj *topcon)
{
  ObjList	*r;
  Intension	itn;

  if (new_only) {
    r = Sem_AnaphoraIntensionParseNew(class, props, source_con, pn, cx, topcon);
  } else {
    IntensionInit(&itn);
    itn.class = class;
    itn.props = props;
    itn.cx = cx;
    itn.cx_only = cx_only;

    /* todo: Look in <topcon> too? */
    if (NULL == (r = DbFindIntension(&itn, NULL))) {
    /* The referred to entity is not in the context yet. So add it.
     * Possibly a nonrestrictive relative clause.
     */
      r = Sem_AnaphoraIntensionParseNew(class, props, source_con, pn, cx,
                                       topcon);
    } else {
      Sem_AnaphoraObjListSet(r, source_con, pn);
    }
  }
  return(r);
}

ObjList *Sem_AnaphoraIntensionParse(Obj *obj, PNode *pn, Context *cx,
                                    Obj *topcon)
{
  int		cx_only, new_only;
  Obj		*class;
  ObjList	*props, *r, *r1, *p;
  ObjListList	*props_list, *pp;
  if ((props = ObjIntensionParse(obj, 1, &class))) {
#ifdef INTEGSYNSEMANA
    if (topcon == NULL &&
        (ObjListISADeep(N("F72"), props) ||
         ObjListISADeep(N("possessive-determiner"), props))) {
      return(Sem_AnaphoraObject(obj, obj, pn, NULL));
    }
#else
    if (topcon == NULL) {
      Dbg(DBGGEN, DBGBAD, "Sem_AnaphoraIntensionParse: NULL topcon");
    }
#endif

    if (pn == NULL) {
    /* todo */
      pn = PNI(obj, 1);
    }
    if (ISA(N("indefinite-article"), I(I(obj, 1), 0))) {
      new_only = 1;
      cx_only = 0; /* Don't care. */
    } else if (ISA(N("definite-article"), I(I(obj, 1), 0))) {
      new_only = 0;
      cx_only = 1;
    } else {
      new_only = cx_only = 0;
    }
    /* "Jim Garnier's sister"
     * obj: [such-that
     *       human
     *       [sister-of
     *        [such-that human [NAME-of human NAME:"Jim Garnier"]]
     *        human]]
     * props: [sister-of [such-that human [NAME-of human NAME:"Jim Garnier"]]
     *         human]
     * class: human
     * Recurse on <props> first, to get:
     * props_list: {[sister-of Jim human]}
     *
     * props: [standard-copula [possessive-determiner sister-of] human]
     * props_list: {[standard-copula Karen human]}
     */
    props_list = NULL;
    for (p = props; p; p = p->next) {
      r1 = Sem_AnaphoraParse(p->obj, p->u.sp.pn, cx, topcon);
      props_list = ObjListListCartesianProduct(props_list, r1);
    }
    /* todo: Freeing. */
    r = NULL;
    for (pp = props_list; pp; pp = pp->next) {
      /* todo: This <pn> is somewhat bogus. Should use pp->u.pn, but this
       * is taken.
       */
      r1 = Sem_AnaphoraIntensionParse1(class, pp->u.objs, obj, pn,
                                       cx, cx_only, new_only,
                                       topcon);
      r = ObjListAppendDestructive(r, r1);
    }
    if (ObjInDeep(N("NAME-of"), obj) ||
        ObjInDeep(N("indefinite-article"), obj) ||
        ObjInDeep(N("definite-article"), obj)) {
    /* Return extension only. */
      return(r);
    } else {
    /* Return extension+intension. */
      return(Sem_AnaphoraObject(obj, obj, pn, r));
    }
  }
  return(NULL);
}

ObjList *Sem_AnaphoraIntensionParses(ObjList *objs, PNode *pn, 
                                    Context *cx, Obj *topcon)
{
  ObjList	*p, *r, *r0;
  r = NULL;
  for (p = objs; p; p = p->next) {
    if (ISA(N("such-that"), I(p->obj, 0))) {
      r0 = Sem_AnaphoraParse(p->obj, pn, cx, topcon);
      r = ObjListAppendDestructive(r, r0);
    } else {
      r = ObjListCreateSPFrom(p->obj, p->u.sp.pn, p, r);
    }
  }
  return(r);
}

ObjList *Sem_AnaphoraIntensionIf(int do_suchthat, ObjList *r, PNode *pn,
                                 Context *cx, Obj *topcon)
{
  ObjList	*r1;
  if (do_suchthat) {
    r1 = Sem_AnaphoraIntensionParses(r, pn, cx, topcon);
    ObjListFree(r);
    return(r1);
  } else {
    return(r);
  }
}

ObjList *Sem_AnaphoraParse1(Obj *obj, PNode *pn, Context *cx, Obj *topcon)
{
  int 		i, j, len, len1, len2, do_suchthat;
  Obj		*det, *subj, *rel;
  ObjList	*objs, *p, *q, *r;
  LexEntryToObj	*copula_leo;
  PNode		*det_pn, *subj_pn, *rel_pn;

  if (ISA(N("such-that"), I(obj, 0))) {
    return(Sem_AnaphoraIntensionParse(obj, pn, cx, topcon));
  }

  if (Sem_AnaphoraIsCopulaPossessive(obj, &det, &det_pn, &subj, &subj_pn, &rel,
                                     &rel_pn, &copula_leo)) {
    if ((r = Sem_AnaphoraParseCopulaPossessive(det, det_pn, subj, subj_pn,
                                               rel, rel_pn, copula_leo, cx,
                                               topcon))) {
      return(r);
    }
  }

  if ((ISA(N("F72"), obj) && (!ISA(N("interrogative-pronoun"), obj))) ||
      ISA(N("possessive-determiner"), obj)) {
  /* Stray possessive-determiners can occur from Sem_ParseCopula.
   * "What is her occupation?"
   */
    if (topcon == NULL) {
#ifdef INTEGSYNSEMANA
      return(Sem_AnaphoraObject(obj, obj, pn, NULL));
#else
      Dbg(DBGGEN, DBGBAD, "Sem_AnaphoraParse1: NULL topcon");
      return(Sem_AnaphoraPronoun(obj, pn, cx, topcon));
#endif
    } else {
      return(Sem_AnaphoraPronoun(obj, pn, cx, topcon));
    }
  }

/*
  if (ISA(N("group"), obj)) {
    return(Sem_AnaphoraGroup(obj, pn, cx, topcon));
  }
 */

  if (!ObjIsList(obj)) {
    return(Sem_AnaphoraObject(obj, obj, pn, NULL));
  }

  if (ISA(N("F68"), I(obj, 0))) {
    if (topcon == NULL) {
#ifdef INTEGSYNSEMANA
      return(Sem_AnaphoraObject(obj, obj, pn, NULL));
#else
      Dbg(DBGGEN, DBGBAD, "Sem_AnaphoraIntensionParse: NULL topcon");
#endif
    }
    r = Sem_AnaphoraDeterminer(I(obj, 0), I(obj, 1), PNI(obj, 0),
                               PNI(obj, 1), cx, topcon, &do_suchthat);
    return(Sem_AnaphoraIntensionIf(do_suchthat, r, pn, cx, topcon));
  }

  r = ObjListCreateSP(ObjCopyList(obj), SCORE_MAX, NULL, pn, NULL, NULL);
  /* Note similar Cartesian product code is used in ObjUnroll. */
  len1 = 1;
  for (i = 0, len = ObjLen(obj); i < len; i++) {
    /* I(obj, i): N("subject-pronoun") */
    objs = Sem_AnaphoraParse(I(obj, i), PNI(obj, i), cx, topcon);
    if (i == 0) {
    /* The first element of a proposition is usually something
     * that can't be an anaphor. Examples: tense, copula.
     * But adverbs? "that way"
     */
      AnaphorFreeAll(objs->u.sp.anaphors);
      objs->u.sp.anaphors = NULL;
    }
    /* objs: N("Jim") N("Harry") */
    len2 = ObjListLen(objs);
    if (len2 <= 0) {
    /* Parse failed. todo: in some cases it shouldn't? */
      return(NULL);
    } else if (len2 > 1) {
      r = ObjListReplicateNTimesAnaphors(r, len2); /* Does not copy anaphors. */
    }
    /* objs:       N("Jim")               N("Harry")
     * r:    [talk subject-pronoun] [talk subject-pronoun]
     */
    for (p = objs, q = r; p; p = p->next) {
      for (j = 0; j < len1; j++) {
        ObjSetIth(q->obj, i, p->obj);
        q->u.sp.anaphors = AnaphorAppend(q->u.sp.anaphors, p->u.sp.anaphors);
           /* Copies anaphors. */
        q = q->next;
      }
    }
    len1 = len1*len2;
    ObjListFree(objs);
  }
  return(Sem_AnaphoraTime(r, cx));
}

ObjList *Sem_AnaphoraParse(Obj *obj, PNode *pn, Context *cx, Obj *topcon)
{
  ObjList	*r;
  if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
    IndentUp();
    IndentPrint(Log);
    fputs("AC ", Log);
    ObjPrint1(Log, obj, NULL, 5, 1, 0, 1, 0);
    fputc(NEWLINE, Log);
  }
  if (pn == NULL) Dbg(DBGGEN, DBGHYPER, "pn not supplied");
  r = Sem_AnaphoraParse1(obj, pn, cx, topcon);
  if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
    IndentPrint(Log);
    fputs("AR ", Log);
    ObjListPrint(Log, r);
    IndentDown();
  }
  return(r);
}

ObjList *Sem_AnaphoraParseA(Obj *obj, PNode *pn, Context *cx, Obj *topcon)
{
  if (topcon == NULL) {
#ifdef INTEGSYNSEMANA
#ifdef notdef
    if (ObjIsAscriptiveAttachmentDeep(obj)) {
    /* "Puchl is a grocer"  */
      return(Sem_AnaphoraObject(obj, obj, pn, NULL));
    }
#endif
#else
    Dbg(DBGGEN, DBGBAD, "Sem_AnaphoraParseA: NULL topcon");
#endif
  }
  return(Sem_AnaphoraParse(obj, pn, cx, topcon));
}

#ifdef INTEGSYNSEMANA
ObjList *Sem_AnaphoraParses(ObjList *objs, PNode *pn, Context *cx, Obj *topcon)
{
  ObjList	*p, *q, *r, *r1;
  r = NULL;
  for (p = objs; p; p = p->next) {
    r1 = Sem_AnaphoraParseA(p->obj, pn, cx, topcon);
    for (q = r1; q; q = q->next) {
      q->u.sp.score = ScoreCombine(q->u.sp.score, p->u.sp.score);
      q->u.sp.pn = p->u.sp.pn;
      q->u.sp.anaphors = AnaphorAppend(q->u.sp.anaphors, p->u.sp.anaphors);
      q->u.sp.leo = p->u.sp.leo;
    }
    r = ObjListAppendDestructive(r, r1);
  }
  return(r);
}
#endif

/******************************************************************************
 * COREFERENCE
 *
 * For more information on linguistic coreference constraints, see:
 * Lasnik (1976)
 * Lyons (1977, pp. 77, 661, 664, 667) (no tree-based rules though)
 * Bresnan (1978)
 * Chomsky (1982/1987, pp. 17, 20)
 * Allen (1995, p. 369)
 *
 * todo: Lyons (1977, p. 661) regarding the fact that cataphora does
 * not hold between coordinate clauses in compound sentences.
 ******************************************************************************/

Bool Sem_AnaphoraIsCoreferenceOKSymmetric(Anaphor *an1, Anaphor *an2)
{
  int	gender1, number1, person1;
  int	gender2, number2, person2;
  PNodeGetHeadNounFeatures(an1->pn, 0, &gender1, &number1, &person1);
  PNodeGetHeadNounFeatures(an2->pn, 0, &gender2, &number2, &person2);
  return(FeatureMatch(gender1, gender2) &&
         FeatureMatch(number1, number2) &&
         FeatureMatch(person1, person2));
}

Bool Sem_AnaphoraIsCoreferenceOKAsymmetric(Anaphor *an1, Anaphor *an2,
                                           PNode *root)
{
  PNode	*np1, *np2, *dummy;
  np1 = XBarImproperAncestorNP(an1->pn, root);	/* todo: Go up to X-MAX? */
  np2 = XBarImproperAncestorNP(an2->pn, root);
  if (np1 == NULL || np2 == NULL) {
  /* todo */
    Dbg(DBGGEN, DBGDETAIL, "Sem_AnaphoraIsCoreferenceOKAsymmetric");
    return(1);
  }
  if (ISA(N("F72"), an1->source_con)) {
  /* NP1 is pronoun. */
    if (ISA(N("reflexive-pronoun"), an1->source_con)) {
      return(XBarGoverns(np2, np1, root));
    } else {
      return(!XBarGoverns(np2, np1, root));
    }
  } else {
  /* NP1 is not pronoun. */
    return(!XBarCCommands(np2, np1, root, &dummy));
  }
}

Bool Sem_AnaphoraIsCoreferenceOK(Anaphor *anaphors, PNode *root)
{
  Anaphor	*an1, *an2;
  for (an1 = anaphors; an1; an1 = an1->next) {
    for (an2 = anaphors; an2; an2 = an2->next) {
      if (an1 == an2) continue;
      if (an1->con != an2->con) continue; /* noncoreferential */
      if (!Sem_AnaphoraIsCoreferenceOKSymmetric(an1, an2)) return(0);
      if (!Sem_AnaphoraIsCoreferenceOKAsymmetric(an1, an2, root)) return(0);
      if (!Sem_AnaphoraIsCoreferenceOKAsymmetric(an2, an1, root)) return(0);
    }
  }
  return(1);
}

/******************************************************************************
 * ANAPHORA PRUNING
 ******************************************************************************/

/* Destructively modifies <objs>. */
ObjList *Sem_AnaphoraPrune(ObjList *objs, PNode *root)
{
  ObjList	*p, *prev;
  prev = NULL;
  for (p = objs; p; p = p->next) {
    if (!Sem_AnaphoraIsCoreferenceOK(p->u.sp.anaphors, root)) {
      if (DbgOn(DBGGEN, DBGDETAIL)) {
        fprintf(Log, "Coreference constraint pruned ");
        ObjPrint1(Log, p->obj, NULL, 5, 1, 0, 1, 0);
        fprintf(Log, " with anaphors:\n");
        AnaphorPrintAll(Log, p->u.sp.anaphors);
      }
      /* Splice out. */
      if (prev) {
        prev->next = p->next;
        /* todoFREE */
      } else objs = p->next;
    } else {
      if (DbgOn(DBGGEN, DBGDETAIL)) {
        fprintf(Log, "Accepted ");
        ObjPrint1(Log, p->obj, NULL, 5, 1, 0, 1, 0);
        if (p->u.sp.anaphors) {
          fprintf(Log, " with anaphors:\n");
          AnaphorPrintAll(Log, p->u.sp.anaphors);
        } else {
          fputc(NEWLINE, Log);
        }
      }
      prev = p;
    }
  }
  return(objs);
}

/******************************************************************************
 * ANAPHORA PARSING AND PRUNING
 ******************************************************************************/

ObjList *Sem_Anaphora(Obj *concept, Context *cx, PNode *root)
{
  ObjList	*objs;
  if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
    Dbg(DBGSEMPAR, DBGDETAIL, "**** ANAPHORIC PARSE BEGIN ****");
    IndentInit();
  }
  objs = Sem_AnaphoraParse(concept, NULL, cx, concept);
  objs = Sem_AnaphoraPrune(objs, root);
  if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
    Dbg(DBGSEMPAR, DBGDETAIL, "**** ANAPHORIC PARSE END ****");
  }
  return(objs);
}

/******************************************************************************
 * ANAPHORA COMMITMENT
 ******************************************************************************/

void Sem_AnaphoraCommit1(Anaphor *an, Context *cx)
{
  int	gender, number, person;
  Obj	*sex;
  PNodeGetHeadNounFeatures(an->pn, 0, &gender, &number, &person);
  if (gender == F_NULL) {
    if (ISA(N("animal"), an->con)) {
      if ((sex = DbGetEnumValue(NULL, &cx->story_time, N("sex"), an->con,
                                NULL))) {
        gender = SexToFeat(sex);
      }
    }
  }
  ContextAntecedentRefreshAllCh(cx, an->con, an->pn, cx->sproutpn,
                                gender, number, person);
}

void Sem_AnaphoraCommit(Anaphor *anaphors, Context *cx)
{
  Anaphor	*an;
  ContextAntecedentDecayAllCh(cx);
  for (an = anaphors; an; an = an->next) {
    Sem_AnaphoraCommit1(an, cx);
    an->pn = NULL;	/* Syn_ParseParseDone frees these when UA returns. */
  }
}

/******************************************************************************
 * GENERATION
 ******************************************************************************/

/* todoDATABASE */
Obj *Sem_AnaphoraCaseToPronoun(Discourse *dc, Obj *cas, int same_as_subject)
{
  if (cas == N("subj")) {
    /* He goes/Il va */
    return(N("subject-pronoun"));
  } else if (cas == N("obj")) {
    if (same_as_subject) {
    /* He washes himself/Il lave se => Il se lave */
      return(N("reflexive-pronoun"));
    } else {
    /* He washes her/Il lave la => Il la lave */
      return(N("direct-object-pronoun"));
    }
  } else if (cas == N("iobj")) {
    if (same_as_subject) {
    /* He talks to himself/Il parle à se => Il se parle
     * todo: For French, it would be more symmetrical to generate
     * moi-même here which is transformed to a reflexive pronoun,
     * just as disjunctive pronouns are transformed to indirect
     * object pronouns.
     */
      return(N("reflexive-pronoun"));
    } else {
    /* He talks to her/Il parle à elle => Il lui parle */
      return(N("disjunctive-pronoun"));
    }
  } else if (cas == N("complement")) {
    if (DC(dc).lang == F_ENGLISH && same_as_subject) {
    /* He is himself */
      return(N("reflexive-pronoun"));
    } else if (DC(dc).lang == F_ENGLISH && DC(dc).style == F_LITERARY) {
    /* It is he */
      return(N("subject-pronoun"));
    } else {
    /* It is him/C'est lui */
      return(N("disjunctive-pronoun"));
    }
  }
  return(NULL);
}

void Sem_AnaphoraGetGN(ObjList *objs, Obj *obj,
                       /* RESULTS */ int *gender, int *number)
{
#ifdef notdef
    Not yet used.
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    if (p->obj == obj) {
      *number = p->u.sl.number;
      *gender = p->u.sl.gender;
      return;
    }
  }
#endif

  *number = F_SINGULAR;
  *gender = F_MASCULINE;
}

/* todo: Need to come up with better conditions, as "What is the
 * circumference of there?" doesn't sound right. But if "of there"
 * transforms to "its" we might be OK.
 */
Bool Sem_AnaphoraIsLocation(Discourse *dc, Obj *obj)
{
  return(ISA(N("polity"), obj) ||
         ISA(N("educational-institution"), obj) ||
         ISA(N("building"), obj));
}

Bool Sem_AnaphoraIsGenerallyKnown(Obj *obj)
{
  return(ISA(N("organization"), obj) && obj != N("organization"));
}

/* Returns whether <obj> is the one and only Obj with the highest salience
 * among <objs>. (If <obj> and another Obj both have the highest salience,
 * then 0 is returned.)
 */
Bool Sem_Anaphora_ObjHasHighestSalienceAmong(Obj *obj, ObjList *objs)
{
  int		highest;
  ObjList	*p;
  Anaphor	*an;
  if (objs == NULL) return(0);
  highest = 0;
  for (p = objs; p; p = p->next) {
    for (an = p->u.sp.anaphors; an; an = an->next) {
      if (an->salience > highest) {
        highest = an->salience;
      }
    }
  }
  /* If anyone besides <obj> has the highest salience, return 0. */
  for (p = objs; p; p = p->next) {
    for (an = p->u.sp.anaphors; an; an = an->next) {
      if (an->salience == highest) {
        if (obj != p->obj) return(0);
      }
    }
  }
  /* If <obj> has the highest salience, return 1. (<obj> was never
   * guaranteed to be in <objs>.)
   */
  for (p = objs; p; p = p->next) {
    for (an = p->u.sp.anaphors; an; an = an->next) {
      if (an->salience == highest) {
        if (obj == p->obj) return(1);
      }
    }
  }
  return(0);
}

/* todo: An object is also known if it is nearby? */
void Sem_AnaphoraGenPronoun(Discourse *dc, Obj *obj, Obj *topcon,
                            int same_as_subject, Obj *cas,
                            /* INPUT AND RESULTS */ int *gender, int *number,
                            int *person, /* RESULTS */ Obj **pronoun_con,
                            PNode **pronoun_pn,
                            Bool *isknown, int *actual_number)
{
  Obj		*pronoun_type;
  ObjList	*objs;
  *pronoun_pn = NULL;
  *pronoun_con = NULL;
  *isknown = 0;
  if (ObjListIn(obj, DCSPEAKERS(dc))) {
    Sem_AnaphoraGetGN(DCSPEAKERS(dc), obj, gender, number);
    *actual_number = *number;
    *person = F_FIRST_PERSON;
    *isknown = 1;
    *pronoun_con = Sem_AnaphoraCaseToPronoun(dc, cas, same_as_subject);
    *pronoun_pn =
      GenMakePronoun(*pronoun_con, obj, *gender, *number, *person, dc);
    return;
  } else if (ObjListIn(obj, DCLISTENERS(dc))) {
    Sem_AnaphoraGetGN(DCSPEAKERS(dc), obj, gender, number);
    if (F_TUTOIEMENT == DiscourseAddress(dc)) *number = F_SINGULAR;
    else *number = F_PLURAL;
    *actual_number = F_SINGULAR;
    *person = F_SECOND_PERSON;
    *isknown = 1;
    *pronoun_con = Sem_AnaphoraCaseToPronoun(dc, cas, same_as_subject);
    *pronoun_pn = GenMakePronoun(*pronoun_con, obj, *gender, *number,
                                 *person, dc);
    return;
  }
  *actual_number = *number;
  Dbg(DBGGEN, DBGHYPER, "Sem_AnaphoraGen: <%s>", M(obj));
  if ((cas != N("subj")) && Sem_AnaphoraIsLocation(dc, obj)) {
    pronoun_type = N("pronoun-there-location");
  } else {
    pronoun_type = Sem_AnaphoraCaseToPronoun(dc, cas, same_as_subject);
  }
  objs = Sem_AnaphoraPronoun1(pronoun_type, NULL, dc->cx_best,
                              *gender, *number, *person, topcon);
  /* todo: Could also allow if restrictions make this unambiguous.
   * For example, in "Elle est diplomée ...", "elle" is not a university.
   * Though salience might already handle this.
   */
  if (Sem_Anaphora_ObjHasHighestSalienceAmong(obj, objs)) {
    *person = F_THIRD_PERSON;
    *isknown = 1;
    *pronoun_con = pronoun_type;
    *pronoun_pn = GenMakePronoun(pronoun_type, obj, *gender, *number, *person,
                                 dc);
    ObjListFree(objs);
    return;
  }
  if (ObjListIn(obj, objs)) *isknown = 1;
  ObjListFree(objs);
  if (!*isknown) {
    if (Sem_AnaphoraIsGenerallyKnown(obj)) {
      *isknown = 1;
    }
  }
}

Obj *Sem_AnaphoraGetIndefiniteArticle(Obj *obj, char *usagefeat, Discourse *dc)
{
  if (ISAP(N("human"), obj)) {
  /* Yes, Mrs. Jeanne Püchl is in fact a human. */
    return(N("empty-article")); /* todo */
  }
  if (StringIn(F_MASS_NOUN, usagefeat)) {
    if (DC(dc).lang == F_ENGLISH) {
      /* "a beverage and some Irish soda bread are a type of food". */
      return(N("empty-article"));
    } else {
      return(N("partitive-article"));
    }
  } else {
    return(N("indefinite-article"));
  }
}

/* See Chuquet and Paillard (1989, p. 48) and Lyons (1977, p. 194). */
Obj *Sem_AnaphoraGenArticle(Discourse *dc, Obj *obj, Obj *actual_obj,
                            LexEntry *le, char *usagefeat, Obj *aspect,
                            int number, int pos, int isknown,
                            Obj *sugg_arttype)
{
  int	art_feat;
  Obj	*arttype;

  if (sugg_arttype == N("forced-empty-article")) {
    return(N("empty-article"));
  }
  if (sugg_arttype == N("forced-definite-article")) {
    return(N("definite-article"));
  }

  if (pos != F_NOUN && pos != F_PRONOUN) return(N("empty-article"));

  if (pos == F_NOUN &&
      N("aspect-generality-ascriptive") == aspect &&
      number == F_PLURAL) {
    if (DC(dc).lang == F_ENGLISH) {
    /* Elephants are smart. */
      return(N("empty-article"));
    } else {
    /* Les éléphants sont intelligents. */
      return(N("definite-article"));
    }
  }

  if (pos == F_NOUN &&
       N("aspect-generality-equative") == aspect) {
    if (number == F_SINGULAR) {
      /* An elephant is a mammal. */
      return(Sem_AnaphoraGetIndefiniteArticle(obj, usagefeat, dc));
    } else if (number == F_PLURAL) {
      if (DC(dc).lang == F_ENGLISH) {
      /* Elephants are mammals. */
        return(N("empty-article"));
      } else {
      /* todo: Les éléphants sont des mammifères. */
        return(N("definite-article"));
      }
    }
  }

  arttype = sugg_arttype;
  art_feat = FeatureGet(usagefeat, FT_ARTICLE);
  if (pos == F_PRONOUN && art_feat == F_NULL) art_feat = F_EMPTY_ART;

  /* Conceptual defaulting */
  if (arttype == NULL && /* Added this so that "the Beatles" works. */
      ObjIsConcrete(obj) && (art_feat == F_NULL)) {
    /* todo: Was FT_PARUNIV. Mistake? */
    if (ISA(N("human"), obj) || ISA(N("polity"), obj) ||
        ISA(N("media-object"), obj)) {
      arttype = N("empty-article");
    }
  }
  if (arttype == NULL && art_feat == F_NULL) {
    if (ISA(N("langue"), obj)) {
      arttype = N("empty-article");
    }
  }
  if (ISA(N("human-name"), obj) && (art_feat == F_NULL)) {
    arttype = N("empty-article");
  }
  if (ISA(N("company"), obj) && (art_feat == F_NULL)) {
    arttype = N("empty-article");
  }

  /* Linguistic overrides. */
  if (F_DEFINITE_ART == art_feat) {
    arttype = N("definite-article");
  } else if (F_EMPTY_ART == art_feat) {
    arttype = N("empty-article");
  }

  if (arttype == NULL) {

    if ((!isknown) && ObjIsConcrete(actual_obj)) {
    /* "the street" */
      isknown = ContextIsObjectFound(dc->cx_best, actual_obj);
    }

    if (isknown) {
      arttype = N("definite-article");
    } else {
      arttype = Sem_AnaphoraGetIndefiniteArticle(obj, usagefeat, dc);
    }
  }

  if ((sugg_arttype == N("definite-article") ||
       sugg_arttype == N("indefinite-article")) &&
      DC(dc).lang == F_ENGLISH) {
    /* todo: This is currently intended to be used only for dictionary
     * definitions.
     */
    if (StringIn(F_MASS_NOUN, usagefeat) ||
        (StringIn(F_COMMON_INFL, usagefeat) &&
         (F_PLURAL == FeatureGet(usagefeat, FT_NUMBER)))) {
      arttype = N("empty-article");
    }
  }

  return(arttype);
}

/******************************************************************************
 * POST-ANAPHORA PROCESSING
 ******************************************************************************/

Obj *Sem_AnaphoraPost(Obj *in)
{
  int		i, len;
  Obj		*out;
  ObjList	*members;
  if (N("prep-accompaniment-with") == I(in, 0) &&
      ISA(N("script"), I(I(in, 1), 0)) && I(I(in, 1), 1) &&
      I(in, 2)) {
    out = ObjCopyList(I(in, 1));
    members = AndObjToObjList(I(in, 2));
    members = ObjListCreate(I(out, 1), members);
    ObjSetIth(out, 1, GroupCreate(&TsNA, members));
    ObjListFree(members);
  } else {
    out = in;
  }
  for (i = 0, len = ObjLen(out); i < len; i++) {
    ObjSetIth(out, i, Sem_AnaphoraPost(I(out, i)));
  }
  return(out);
}

/* End of file. */
