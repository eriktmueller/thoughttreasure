/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "lexentry.h"
#include "lexmorph.h"
#include "lextheta.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "semdisc.h"
#include "synpnode.h"
#include "utildbg.h"

ObjToLexEntry *ObjToLexEntryCreate(char *features, LexEntry *le,
                                   ThetaRole *theta_roles,
                                   ObjToLexEntry *next)
{
  ObjToLexEntry *ole;
  ole = CREATE(ObjToLexEntry);
  ole->features = features;
  ole->le = le;
  ole->next = next;
  ole->theta_roles = theta_roles;
  ole->lastused = UNIXTSNA;
  return(ole);
}

#define MAXOLELEN	500		/* todo */

Bool	consider[MAXOLELEN], stylematch[MAXOLELEN], dialectmatch[MAXOLELEN];
Bool	styledialectmatch[MAXOLELEN];

void ObjToLexEntryClearLastused(ObjToLexEntry *ole, Discourse *dc)
{
  int	i;
  for (i = 0; ole; i++, ole = ole->next) {
  /*
   * "if (consider[i])" was a nice idea but it doesn't work well when doing
   * generation to multiple dialects: the dialect-common items get reset
   * whenever a given dialect finishes the list of lexical items and the
   * result is that the frequent items for other dialects aren't generated.
   */
    if (DC(dc).lang == FeatureGet(ole->le->features, FT_LANG)) {
      ole->lastused = UNIXTSNA;
    }
  }
}

Bool FeatureTaskOK(char *features, Obj *task)
{
  if (task == N("parse")) {
    return(!StringIn(F_TRANS_ONLY, features));
  } else if (task == N("generate")) {
    return(!StringIn(F_TRANS_ONLY, features));
  } else if (task == N("translate")) {
    return(1);
  }
  return(1);
}

void ConToThetaFilled(Obj *obj, /* RESULTS */ int *theta_filled)
{
  int	i;
  for (i = 0; i < MAXTHETAFILLED; i++) {
    theta_filled[i] = I(obj, i) != NULL &&
                      I(obj, i) != ObjNA;
  }
}

void ThetaRoleToThetaFilled(ThetaRole *theta_roles,
                            /* RESULTS */ int *theta_filled)
{
  int	i;
  for (i = 0; i < MAXTHETAFILLED; i++) {
    theta_filled[i] = 0;
  }
  for (i = 0;
       theta_roles && i < MAXTHETAFILLED;
       i++, theta_roles = theta_roles->next) {
    theta_filled[i] = theta_roles->cas != ObjNA && (!theta_roles->isoptional);
  }
}

Bool ThetaRoleMatch(int *theta_filled, ThetaRole *theta_roles, int relaxed)
{
  int	i;
  if (theta_filled == NULL) return(1);
  for (i = 1; i < MAXTHETAFILLED; i++) {
    if (theta_filled[i]) {
      if (theta_roles == NULL || theta_roles->cas == ObjNA) return(0);
    } else {
      if (!relaxed) {
        if (theta_roles &&
            (theta_roles->isoptional == 0 &&
             theta_roles->cas != ObjNA)) {
          return(0);
        }
      }
    }
    if (theta_roles) theta_roles = theta_roles->next;
  }
  return(1);
}

/* todo: Something compatible with the dialect is chosen.
 * Alter this to PREFER dialect-specific choices?
 * <value_prop> can also just be value number.
 */
ObjToLexEntry *ObjToLexEntryGet3(Obj *obj, Obj *value_prop, char *features,
                                 char *not_usagefeat, int subcat, int paruniv,
                                 int *theta_filled, int pass_two,
                                 Discourse *dc)
{
  int		i, pos, address;
  char		features1[FEATLEN];
  ObjToLexEntry *ole, *p;
  ole = obj->ole;
  address = DiscourseAddress(dc);
  if (dc->ga.qwq &&
      (ISA(N("question-word"), obj) || ISA(N("copula"), obj))) {
    StringCpy(features1, features, FEATLEN);
    StringElimChar(features1, F_NOUN);
    features = features1;
  }
  for (i = 0, p = ole; p; i++, p = p->next) {
    if (i >= MAXOLELEN) {
      Dbg(DBGGENER, DBGBAD, "increase MAXOLELEN");
      break;
    }
    pos = FeatureGet(p->le->features, FT_POS);

    /* todo: For super debugging, if we return NULL from this function,
     * print out list of why each lexical entry failed.
     */
    consider[i] = ((DC(dc).lang == FeatureGetRequired("ObjToLexEntryGet",
                                                      p->le->features,
                                                      FT_LANG))
                   && StringAnyIn(features, p->le->features)
                   && paruniv == FeatureGet(p->features, FT_PARUNIV)
                   && (subcat == F_NULL ||
                       subcat == ThetaRoleGetAnySubcat(p->theta_roles))
                   && F_NULL == FeatureGet(p->features, FT_CONNOTE) /* todo */
                   && FeatureDialectMatch(DC(dc).dialect,
                                          FeatureGet(p->features, FT_DIALECT))
                   && DC(dc).style == FeatureGet(p->features, FT_STYLE)
                   && FeatureTaskOK(p->features, dc->task)
                   && FeatureMatch(address, FeatureGet(p->features, FT_ADDRESS))
                   && (not_usagefeat == NULL ||
                       (!StringAnyIn(not_usagefeat, p->features)))
                   && ((pos != F_NOUN) || ISA(N("relation"), obj) ||
                       ThetaRoleMatch(theta_filled, p->theta_roles, pass_two)));
    Nop(); /* Debugging anchor. */
  }
  if (!dc->ga.consistent) {
    for (i = 0, p = ole; p && i < MAXOLELEN; i++, p = p->next) {
      if (consider[i] && F_FREQUENT == ((uc)FeatureGet(p->features, FT_FREQ)) &&
          p->lastused == UNIXTSNA) {
        p->lastused = time(NULL); return(p);
      }
    }
    for (i = 0, p = ole; p && i < MAXOLELEN; i++, p = p->next) {
      if (consider[i] && F_NULL == FeatureGet(p->features, FT_FREQ) &&
          p->lastused == UNIXTSNA) {
        p->lastused = time(NULL); return(p);
      }
    }
    if (DC(dc).infrequent_ok) {
      for (i = 0, p = ole; p && i < MAXOLELEN; i++, p = p->next) {
        if (consider[i] && F_INFREQUENT == FeatureGet(p->features, FT_FREQ) &&
            p->lastused == UNIXTSNA) {
          p->lastused = time(NULL); return(p);
        }
      }
    }
  }
  if (pass_two) {
    /* In this case ObjToLexEntryClearLastused has already been done. */
    return(NULL);
  }
  ObjToLexEntryClearLastused(ole, dc);
  for (i = 0, p = ole; p && i < MAXOLELEN; i++, p = p->next) {
    if (consider[i] && F_FREQUENT == FeatureGet(p->features, FT_FREQ)) {
      p->lastused = time(NULL); return(p);
    }
  }
  for (i = 0, p = ole; p && i < MAXOLELEN; i++, p = p->next) {
    if (consider[i] && F_NULL == FeatureGet(p->features, FT_FREQ)) {
      p->lastused = time(NULL); return(p);
    }
  }
  /* Note in this case we do permit the generation of infrequent. */
  for (i = 0, p = ole; p && i < MAXOLELEN; i++, p = p->next) {
    if (consider[i] && F_INFREQUENT == FeatureGet(p->features, FT_FREQ)) {
      p->lastused = time(NULL); return(p);
    }
  }
  return(NULL);
}

ObjToLexEntry *ObjToLexEntryGet2(Obj *obj, Obj *value_prop, char *features,
                                 char *not_usagefeat, int subcat, int paruniv,
                                 int *theta_filled, Discourse *dc)
{
  ObjToLexEntry	*ole;
  if (!obj) return(NULL);
  if ((ole = ObjToLexEntryGet3(obj, value_prop, features, not_usagefeat,
                               subcat, paruniv, theta_filled, 0,
                               dc))) {
    return(ole);
  }
  ObjToLexEntryClearLastused(obj->ole, dc);
  if (theta_filled) {
    return(ObjToLexEntryGet3(obj, value_prop, features, not_usagefeat, subcat,
                             paruniv, theta_filled, 1, dc));
  }
  return(NULL);
}

ObjToLexEntry *ObjToLexEntryGet1B(Obj *obj, Obj *value_prop, char *features,
                                  char *not_usagefeat, int subcat, int paruniv,
                                  int *theta_filled, Discourse *dc)
{
  ObjToLexEntry	*ole;
  if (!obj) return(NULL);
  if ((ole = ObjToLexEntryGet2(obj, value_prop, features, not_usagefeat, subcat,
                               paruniv, theta_filled, dc))) {
    return(ole);
  }
  if (value_prop) {
    return(ObjToLexEntryGet2(obj, NULL, features, not_usagefeat, subcat,
                             paruniv, theta_filled, dc));
  }
  return(NULL);
}

ObjToLexEntry *ObjToLexEntryGet1A(Obj *obj, Obj *value_prop, char *features,
                                  char *not_usagefeat, int subcat, int paruniv,
                                  int *theta_filled, Discourse *dc)
{
  int		save_style;
  char		*p;
  ObjToLexEntry	*ole;

  if ((ole = ObjToLexEntryGet1B(obj, value_prop, features, not_usagefeat,
                                subcat, paruniv, theta_filled, dc))) {
    return(ole);
  }
  /* Try other styles. */
  save_style = DC(dc).style;
  for (p = FT_STYLE FS_NULL; *p; p++) {
    if (*p == save_style) continue;
    DC(dc).style = *p;
    if ((ole = ObjToLexEntryGet1B(obj, value_prop, features, not_usagefeat,
                                  subcat, paruniv, theta_filled,
                                  dc))) {
      return(ole);
    }
  }

  DC(dc).style = save_style;
  return(NULL);
}

ObjToLexEntry *ObjToLexEntryGet1(Obj *obj, Obj *value_prop, char *features,
                                 int subcat, int paruniv,
                                 int *theta_filled, Discourse *dc)
{
  return(ObjToLexEntryGet1A(obj, value_prop, features, NULL, subcat, paruniv,
                            theta_filled, dc));
}

LexEntry *ObjToLexEntryGet(Obj *obj, int pos, int paruniv, Discourse *dc)
{
  ObjToLexEntry	*ole;
  char	features[2];
  features[0] = pos;
  features[1] = TERM;
  if ((ole = ObjToLexEntryGet1(obj, NULL, features, F_NULL, paruniv,
                               NULL, dc))) {
    return(ole->le);
  }
  return(NULL);
}

LexEntryToObj *LexEntryToObjCreate(char *features, Obj *obj,
                                   ThetaRole *theta_roles, LexEntryToObj *next)
{
  LexEntryToObj *leo;
  leo = CREATE(LexEntryToObj);
  leo->features = features;
  leo->obj = obj;
  leo->next = next;
  leo->theta_roles = theta_roles;
  return(leo);
}

int StyleToInt(int style)
{
  switch (style) {
    case F_LITERARY: return(4);
    case F_INFORMAL: return(2);
    case F_SLANG: return(1);
    default: break;
  }
  return(3);
}

Float StyleScore(int consider_style, int expected_style)
{
  /* todoSCORE */
  return(0.5+
         (0.5*
          (1.0-((IntAbs(StyleToInt(consider_style) -
                        StyleToInt(expected_style)))/3.0))));
}

Float DialectScore(int consider_dialect, int expected_dialect)
{
  /* todoSCORE */
  if (consider_dialect == F_NULL ||
      expected_dialect == F_NULL) return(1.0);
  if (consider_dialect == expected_dialect) return(1.0);
  if (consider_dialect == F_OTHER_DIALECT) return(0.7);
  if (consider_dialect == F_OLD) return(0.5);
  return(0.9);	/* Don't penalize F_AMERICAN != F_BRITISH much. */
}

/* Assumes dc is set according to type of speaker. (Generation would normally
 * be set the same way: generate in American English when talking to an
 * American...)
 * todo: Add FT_ADDRESS (useful only for French, to reduce probability of
 * singular "vous" interpretation when "tu" is expected)
 * todo: If a parse is accepted using a noncongruent lexical item, consider
 * updating the speaker model.
 */
Float LexEntryToObjScore(LexEntryToObj *leo, Discourse *dc)
{
  Float	score;
  if (leo == NULL) return(SCORE_MAX);
  switch (FeatureGet(leo->features, FT_FREQ)) {
  /* todoSCORE */
    case F_FREQUENT:
      score = 1.0;
      break;
    case F_INFREQUENT:
      score = 0.2;
      break;
    default:
      score = 0.9;
      break;
  }
  score = ScoreCombine(score,
                       StyleScore(FeatureGet(leo->features, FT_STYLE),
                                  DC(dc).style));
  score = ScoreCombine(score,
                       DialectScore(FeatureGet(leo->features, FT_DIALECT),
                                    DC(dc).dialect));
  return(score);
}

LexEntry *ObjToAbbrev(Obj *obj, int force, Discourse *dc)
{
  int	pos;
  ObjToLexEntry	*ole;
  if (!obj) return(NULL);
  for (ole = obj->ole; ole; ole = ole->next) {
    if (F_NULL != FeatureGet(ole->features, FT_PARUNIV)) continue;
    if (DC(dc).lang != FeatureGet(ole->le->features, FT_LANG)) continue;
    pos = FeatureGet(ole->le->features, FT_POS);
    if (pos != F_NOUN && pos != F_ADJECTIVE) continue;
    if (F_LITERARY == FeatureGet(ole->features, FT_STYLE) &&
        !MorphIsWord(ole->le->srcphrase)) {
      return(ole->le);
    }
  }
  if (force) {
    if ((ole = ObjToLexEntryGet1(obj, NULL, "NA", F_NULL, F_NULL, NULL, dc))) {
      return(ole->le);
    }
  }
  return(NULL);
}

void ObjToLexEntryPrint(FILE *stream, ObjToLexEntry *ole)
{
  for (; ole; ole = ole->next) {
    if (ole->le) {
      fprintf(stream, "%s.%s/\n", ole->le->srcphrase, ole->le->features);
    }
  }
}

/* End of file. */
