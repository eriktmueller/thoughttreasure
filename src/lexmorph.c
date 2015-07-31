/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Common morphology routines.
 *
 * 19940903: begun
 * 19940904: broke away from AnaMorph
 *
 * If AnaMorphOn = 0, morphology is done algorithmically by Lex_AlgMorph.c.
 * If AnaMorphOn = 1, morphology is done by analogy to existing words by
 * Lex_AnaMorph.c.
 */

#include "tt.h"
#include "lexalgm.h"
#include "lexanam.h"
#include "lexentry.h"
#include "lexmorph.h"
#include "repbasic.h"
#include "repobj.h"
#include "repstr.h"

Bool AnaMorphOn;

void MorphInit(Bool flag)
{
  AnaMorphOn = flag;
  if (flag) {
    AnaMorphInit();
  }
}

LexEntry *MorphInflectWord(char *word, int lang, int pos, char *known_features,
                           HashTable *ht)
{
  if (AnaMorphOn) {
    return(AnaMorphInflectWord(word, lang, pos, known_features, ht));
  } else {
    return(AlgMorphInflectWord(word, lang, pos, known_features, ht));
  }
}

void MorphRestrictPOS(char *word, int lang, /* RESULTS */ char *features)
{
  int	len, last;
  char	*last2, *last3;
  StringCpy(features, FT_POS, FEATLEN);
  len = strlen(word);
  if (lang == F_FRENCH) {
    last = word[len-1];
    if (len >= 2) last2 = &word[len-2]; else last2 = "";
    if (len >= 3) last3 = &word[len-3]; else last3 = "";
    if (StringIn(last, "àâbèêëfghîjklmnoôqùvwy") ||
        streq(last2, "ct") || streq(last2, "dt") || streq(last2, "ft") ||
        streq(last2, "gt") || streq(last2, "ht") || streq(last2, "lt") ||
        streq(last2, "ôt") || streq(last2, "pt") || streq(last2, "tt") ||
        streq(last3, "unt")) {
      /* From Guillet (1990, pp. 53-55). */
      StringElimChar(features, 'V');
    }
    if (streq(last3, "ait")) {
      /* From Guillet (1990, pp. 57-58).
       * All exceptions have been entered into FrenchInflections on 19940831.
       * But does this hold for new words? This certainly fails for new words
       * derived from the 25 exceptions.
       */
      StringCpy(features, "V", FEATLEN);
    }
  }
}

Bool MorphIsProper(char *word)
{
  return(CharIsUpper((uc)(word[0])) && CharIsLower((uc)(word[1])));
}

Bool MorphIsAcronym(char *word)
{
  if (StringIn('.', word)) return(1);
  while (*word) {
    if ((!CharIsUpper((uc)(*word))) &&
        (!Char_isdigit(*word)) &&
        *word != SPACE &&
        *word != '+' &&
        *word != '-') {
      return(0);
    }
    word++;
  }
  return(1);
}

Bool MorphIsWord(char *word)
{
  return((!MorphIsProper(word)) && (!MorphIsAcronym(word)));
}

/* Ignores degrees of adjectives and adverbs. */
Bool MorphIsInvariant(char *word, char *features, Obj *obj)
{
  int	pos;
  if (StringIn(F_INVARIANT, features)) return(1);
  pos = FeatureGet(features, FT_POS);
  if (FeatureGet(features, FT_LANG) == F_FRENCH) {
    if (StringIn(F_MASC_FEM, features)) return(0);
    if (pos != F_NOUN && pos != F_VERB && pos != F_ADJECTIVE) return(1);
    if (StringIn(F_TRADEMARK, features)) return(1);
    if (MorphIsAcronym(word)) return(1);
    if (MorphIsProper(word)) {
      if (ISA(N("polity"), obj)) return(0);
      else return(1);
    }
  } else {
    /* F_ENGLISH */
    if (pos != F_NOUN && pos != F_VERB) {
      return(1);
    }
  }
  return(0);
}

/* End of file. */
