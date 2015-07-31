/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Names of humans and sometimes also animals and physical objects.
 *
 * 19950209: created distinct file
 * 19950210: more work
 * 19950211: more work
 * 19950212: more work; doing names is more difficult than one would expect
 *
 * todo:
 * - Nicknames: Jim ``DJ'' Garnier.
 * - A middle initial which stands for no name: Harry S. Truman.
 * - Mme l'inspecteur, Monsieur le Ministre, Madame la Marquise
 * - le président Kennedy.
 * - For British English, generator should produce:
 *   J. A. Garnier, Jim Alfred Garnier, Jim Garnier.
 *   For American English, generator should produce the above, plus:
 *   Jim A. Garnier.
 * - According to Lyons (1977, p. 224, 452),
 *   names can be distinguished from singular countable common nouns:
 *   the latter is always preceded by a determiner (or quantifier):
 *     The boy came yesterday.
 *     James came yesterday.
 *     *Boy came yesterday.
 *     *The James came yesterday.
 *   However, this is no longer true in colloquial American usage, some
 *   examples being:
 *     From Saturday Night Live, "the Mikester".
 *     From Ivana Trump, "the Don".
 *     Also heard: "the Roml".
 *     Also: "introducing the (one and only) Jim Garnier".
 *   Any lexical entries marked as taking no article are further
 *   exceptions.
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "lexwf1.h"
#include "lexwf3.h"
#include "repbasic.h"
#include "repchan.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "repstr.h"
#include "reptime.h"
#include "semdisc.h"
#include "synpnode.h"
#include "ta.h"
#include "taemail.h"
#include "tale.h"
#include "taname.h"
#include "taprod.h"
#include "tatable.h"
#include "tatagger.h"
#include "tatime.h"
#include "utildbg.h"
#include "utillrn.h"

Name *NameCreate()
{
  Name	*nm;
  nm = CREATE(Name);
  nm->fullname[0] = TERM;

  nm->pretitle[0] = TERM;
  nm->pretitles = NULL;

  nm->givenname1[0] = TERM;
  nm->givennames1 = NULL;
  nm->giveninitial1 = TERM;

  nm->givenname12_sep = SPACE;

  nm->givenname2[0] = TERM;
  nm->givennames2 = NULL;
  nm->giveninitial2 = TERM;

  nm->givenname3[0] = TERM;
  nm->givennames3 = NULL;
  nm->giveninitial3 = TERM;

  nm->surname1[0] = TERM;
  nm->surnames1 = NULL;
  nm->surinitial1 = TERM;

  nm->surname12_sep = SPACE;

  nm->surname2[0] = TERM;
  nm->surnames2 = NULL;
  nm->surinitial2 = TERM;

  nm->posttitle[0] = TERM;
  nm->posttitles = NULL;
  return(nm);
}

void NameFree(Name *nm)
{
  ObjListFree(nm->pretitles);
  ObjListFree(nm->givennames1);
  ObjListFree(nm->givennames2);
  ObjListFree(nm->givennames3);
  ObjListFree(nm->surnames1);
  ObjListFree(nm->surnames2);
  ObjListFree(nm->posttitles);
  MemFree(nm, "Name");
}

void NamePrint(FILE *stream, Name *nm)
{
  fprintf(stream, "<%s>:<%s>G<%s:%c>%c<%s:%c>",
          nm->fullname, nm->pretitle, nm->givenname1, (char)nm->giveninitial1,
          (char)nm->givenname12_sep, nm->givenname2, (char)nm->giveninitial2);
  fprintf(stream, "<%s:%c>S<%s:%c>%c<%s:%c>P<%s>",
          nm->givenname3, (char)nm->giveninitial3, nm->surname1,
          (char)nm->surinitial1, (char)nm->surname12_sep, nm->surname2,
          (char)nm->surinitial2,
          nm->posttitle);
}

#define MINAFFIXLEN	3
#define MAXAFFIXLEN	5

HashTable	*NameGivennamePrefixHt, *NameGivennameSuffixHt;
HashTable	*NameSurnamePrefixHt, *NameSurnameSuffixHt;

void TA_NameInit()
{
  NameGivennamePrefixHt = WordFormAffixFindTrain(1, N("given-name"), "S", 0,
                                                 MINAFFIXLEN, MAXAFFIXLEN);
  NameGivennameSuffixHt = WordFormAffixFindTrain(0, N("given-name"), "S", 0,
                                                 MINAFFIXLEN, MAXAFFIXLEN);
  NameSurnamePrefixHt = WordFormAffixFindTrain(1, N("surname"), "S", 0,
                                               MINAFFIXLEN, MAXAFFIXLEN);
  NameSurnameSuffixHt = WordFormAffixFindTrain(0, N("surname"), "S", 0,
                                               MINAFFIXLEN, MAXAFFIXLEN);
}

void ReportAffix(Text *text, Discourse *dc)
{
  ReportWordFormAffix(text, dc, NameGivennamePrefixHt, N("prefix"));
  ReportWordFormAffix(text, dc, NameGivennameSuffixHt, N("suffix"));
  ReportWordFormAffix(text, dc, NameSurnamePrefixHt, N("prefix"));
  ReportWordFormAffix(text, dc, NameSurnameSuffixHt, N("suffix"));
}

void TA_NameParse1(char *phrase, Obj *class, int relax_ok, int always_relax,
                   /* RESULTS */ int *found_exact, int *found_relaxed,
                   ObjList **objs)
{
  char		phrase1[PHRASELEN];
  int		len;
  ObjList	*r;
  *found_exact = 0;
  if (found_relaxed) *found_relaxed = 0;
  *objs = NULL;
  /* (1) Find name in lexicon and use full spell correction. */
  if ((r = TA_FindPhraseBoth(phrase, class))) {
    *found_exact = 1;
    *objs = r;
    return;
  }
  
  if (class == N("post-title") && StringIsDigits(phrase)) {
  /* todo: Save date. Also, make sure digits are preceded by single quote. */
    *objs = ObjListCreate(N("graduation-date"), NULL);
    *found_exact = 1;
    return;
  }

  if (!relax_ok) return;

  /* (2) Find initial name substring. Similar to prefixes below. */
  StringCpy(phrase1, phrase, PHRASELEN);
  len = strlen(phrase1);
  while (len > 5) {
    len--;
    phrase1[len] = TERM;
    if ((r = TA_FindPhraseBoth(phrase1, class))) {
      if (found_relaxed) *found_relaxed = 1;
      *objs = r;
      return;
    }
  }

  if (!CharIsUpper(*((uc *)phrase))) return;

  if (streq("Elle", phrase)) return;

  /* (3) If phrase is not in lexicon exactly, and a common prefix/suffix (such
   *     as Gold+, Fred+, +anda, +ette, +baum) is found, then learn the new
   *     name.
   * todo: Guess its gender. Of course, more information is elsewhere in the
   * text. Pronouns and adjectives in French.
   */
  if (NULL == LexEntryFindPhrase(EnglishIndex, phrase, 0, 0, 0, NULL) &&
      NULL == LexEntryFindPhrase(EnglishIndex, phrase, 0, 0, 0, NULL)) {
    if ((class == N("given-name") &&
         (WordFormAffix(NameGivennameSuffixHt, 0, phrase, MINAFFIXLEN,
                        MAXAFFIXLEN) ||
          WordFormAffix(NameGivennamePrefixHt, 1, phrase, MINAFFIXLEN,
                        MAXAFFIXLEN))) ||
        (class == N("surname") &&
         (WordFormAffix(NameSurnameSuffixHt, 0, phrase, MINAFFIXLEN,
                        MAXAFFIXLEN) ||
          WordFormAffix(NameSurnamePrefixHt, 1, phrase, MINAFFIXLEN,
                        MAXAFFIXLEN)))) {
      *objs = NULL;
      if (found_relaxed) *found_relaxed = 1;
      return;
    }
  }

  if (always_relax) {
    *objs = NULL;
    if (found_relaxed) *found_relaxed = 1;
    return;
  }
}

Bool LenGE(int len, int desired_len, int len_exact, int *IsPosttitle, int o)
{
  if (len_exact) {
    /* 0       1       2
     * Lorenzo Semple, Jr.
     * len = 3
     * desired_len = 2
     */
    if ((desired_len < len) && IsPosttitle[o+desired_len]) {
      return(len == (desired_len+1));
    } else {
      return(len == desired_len);
    }
  } else return(len >= desired_len);
}

#define WH(c)	((c) == SPACE)
#define WD(c)	(((c) == SPACE) || ((c) == '-'))
#define LN(a, b)  ObjListCreate(LearnName(a, b, gender, orig_in, NULL), NULL)

/* Detect every known name-description paradigm. This is a long-winded way
 * of coding and slightly less efficient but more understandable than an
 * FSM (though difficult to modify for changes which affect all the cases).
 * Order of cases in the below code: longer before shorter BEFORE exact before
 * relaxed BEFORE comma format before non-comma format.
 */
Bool TA_NameParse2(Name *nm, int len, int *term, int *IsGivenname,
                   int *IsSurname, int *IsInitial,
                   int *IsGivennameRelaxed, int *IsSurnameRelaxed,
                   int *IsPosttitle,
                   ObjList **GivennameObjs, ObjList **SurnameObjs,
                   char **words, char *orig_in, int gender,
                   int len_exact, int revok, /* RESULTS */ int *offset)
{
  int	o;
  o = *offset;
  if (revok &&
      LenGE(len, 5, len_exact, IsPosttitle, o) &&
      IsSurname[o+0] && term[o+0] == '-' &&
      IsSurname[o+1] && term[o+1] == ',' && IsGivenname[o+2] && WH(term[o+2]) &&
      IsGivenname[o+3] && WH(term[o+3]) && IsGivenname[o+4]) {
  /* Rey-Debove, Karen Anne Marie */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->surname12_sep = term[o+0];
    StringCpy(nm->surname2, words[o+1], PHRASELEN);
    if (IsSurnameRelaxed[o+1]) nm->surnames2 = LN(words[o+1], N("surname"));
    else nm->surnames2 = SurnameObjs[o+1];
    StringCpy(nm->givenname1, words[o+2], PHRASELEN);
    if (IsGivennameRelaxed[o+2]) {
      nm->givennames1 = LN(words[o+2], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+2];
    }
    nm->givenname12_sep = term[o+2];
    StringCpy(nm->givenname2, words[o+3], PHRASELEN);
    if (IsGivennameRelaxed[o+3]) {
      nm->givennames2 = LN(words[o+3], N("given-name"));
    } else {
      nm->givennames2 = GivennameObjs[o+3];
    }
    StringCpy(nm->givenname3, words[o+4], PHRASELEN);
    if (IsGivennameRelaxed[o+4]) {
      nm->givennames3 = LN(words[o+4], N("given-name"));
    } else {
      nm->givennames3 = GivennameObjs[o+4];
    }
    o += 5;
    goto success;
  } else if (revok && LenGE(len, 5, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] &&
             term[o+0] == '-' && IsSurname[o+1] && term[o+1] == ',' &&
             IsGivenname[o+2] && WH(term[o+2]) && IsInitial[o+3] &&
             IsInitial[o+4]) {
  /* Rey-Debove, Karen A. M. */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->surname12_sep = term[o+0];
    StringCpy(nm->surname2, words[o+1], PHRASELEN);
    if (IsSurnameRelaxed[o+1]) nm->surnames2 = LN(words[o+1], N("surname"));
    else nm->surnames2 = SurnameObjs[o+1];
    StringCpy(nm->givenname1, words[o+2], PHRASELEN);
    if (IsGivennameRelaxed[o+2]) {
      nm->givennames1 = LN(words[o+2], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+2];
    }
    nm->giveninitial2 = *words[o+3];
    nm->giveninitial3 = *words[o+4];
    o += 5;
    goto success;
  } else if (revok && LenGE(len, 5, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] &&
             term[o+0] == '-' && IsSurname[o+1] && term[o+1] == ',' &&
             IsInitial[o+2] &&
             IsInitial[o+3] && IsInitial[o+4]) {
  /* Rey-Debove, K. A. M. */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->surname12_sep = term[o+0];
    StringCpy(nm->surname2, words[o+1], PHRASELEN);
    if (IsSurnameRelaxed[o+1]) nm->surnames2 = LN(words[o+1], N("surname"));
    else nm->surnames2 = SurnameObjs[o+1];
    nm->giveninitial1 = *words[o+2];
    nm->givenname12_sep = term[o+2];
    nm->giveninitial2 = *words[o+3];
    nm->giveninitial3 = *words[o+4];
    o += 5;
    goto success;
  } else if (LenGE(len, 5, len_exact, IsPosttitle, o) && IsGivenname[o+0] &&
             WH(term[o+0]) && IsGivenname[o+1] && WH(term[o+1]) &&
             IsGivenname[o+2] && WH(term[o+2]) && IsSurname[o+3] &&
             term[o+3] == '-' && IsSurname[o+4]) {
  /* Karen Anne Marie Rey-Debove */
    StringCpy(nm->givenname1, words[o+0], PHRASELEN);
    if (IsGivennameRelaxed[o+0]) {
      nm->givennames1 = LN(words[o+0], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+0];
    }
    nm->givenname12_sep = term[o+0];
    StringCpy(nm->givenname2, words[o+1], PHRASELEN);
    if (IsGivennameRelaxed[o+1]) {
      nm->givennames2 = LN(words[o+1], N("given-name"));
    } else {
      nm->givennames2 = GivennameObjs[o+1];
    }
    StringCpy(nm->givenname3, words[o+2], PHRASELEN);
    if (IsGivennameRelaxed[o+2]) {
      nm->givennames3 = LN(words[o+2], N("given-name"));
    } else {
      nm->givennames3 = GivennameObjs[o+2];
    }
    StringCpy(nm->surname1, words[o+3], PHRASELEN);
    if (IsSurnameRelaxed[o+3]) nm->surnames1 = LN(words[o+3], N("surname"));
    else nm->surnames1 = SurnameObjs[o+3];
    nm->surname12_sep = term[o+3];
    StringCpy(nm->surname2, words[o+4], PHRASELEN);
    if (IsSurnameRelaxed[o+4]) nm->surnames2 = LN(words[o+4], N("surname"));
    else nm->surnames2 = SurnameObjs[o+4];
    o += 5;
    goto success;
  } else if (LenGE(len, 5, len_exact, IsPosttitle, o) && IsGivenname[o+0] &&
             WH(term[o+0]) && IsInitial[o+1] && IsInitial[o+2] &&
             IsSurname[o+3] && term[o+3] == '-' && IsSurname[o+4]) {
  /* Karen A. M. Rey-Debove */
    StringCpy(nm->givenname1, words[o+0], PHRASELEN);
    if (IsGivennameRelaxed[o+0]) {
      nm->givennames1 = LN(words[o+0], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+0];
    }
    nm->givenname12_sep = term[o+0];
    nm->giveninitial2 = *words[o+1];
    nm->giveninitial3 = *words[o+2];
    StringCpy(nm->surname1, words[o+3], PHRASELEN);
    if (IsSurnameRelaxed[o+3]) nm->surnames1 = LN(words[o+3], N("surname"));
    else nm->surnames1 = SurnameObjs[o+3];
    nm->surname12_sep = term[o+3];
    StringCpy(nm->surname2, words[o+4], PHRASELEN);
    if (IsSurnameRelaxed[o+4]) nm->surnames2 = LN(words[o+4], N("surname"));
    else nm->surnames2 = SurnameObjs[o+4];
    o += 5;
    goto success;
  } else if (LenGE(len, 5, len_exact, IsPosttitle, o) && IsInitial[o+0] &&
             IsInitial[o+1] && IsInitial[o+2] &&
             IsSurname[o+3] && term[o+3] == '-' && IsSurname[o+4]) {
  /* K. A. M. Rey-Debove */
    nm->giveninitial1 = *words[o+0];
    nm->givenname12_sep = term[o+0];
    nm->giveninitial2 = *words[o+1];
    nm->giveninitial3 = *words[o+2];
    StringCpy(nm->surname1, words[o+3], PHRASELEN);
    if (IsSurnameRelaxed[o+3]) nm->surnames1 = LN(words[o+3], N("surname"));
    else nm->surnames1 = SurnameObjs[o+3];
    nm->surname12_sep = term[o+3];
    StringCpy(nm->surname2, words[o+4], PHRASELEN);
    if (IsSurnameRelaxed[o+4]) nm->surnames2 = LN(words[o+4], N("surname"));
    else nm->surnames2 = SurnameObjs[o+4];
    o += 5;
    goto success;
  } else if (revok && LenGE(len, 4, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] &&
             term[o+0] == ',' && IsGivenname[o+1] && WH(term[o+1]) &&
             IsGivenname[o+2] && WH(term[o+2]) && IsGivenname[o+3]) {
  /* Rey, Karen Anne Marie */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    StringCpy(nm->givenname1, words[o+1], PHRASELEN);
    if (IsGivennameRelaxed[o+1]) {
      nm->givennames1 = LN(words[o+1], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+1];
    }
    nm->givenname12_sep = term[o+1];
    StringCpy(nm->givenname2, words[o+2], PHRASELEN);
    if (IsGivennameRelaxed[o+2]) {
      nm->givennames2 = LN(words[o+2], N("given-name"));
    } else {
      nm->givennames2 = GivennameObjs[o+2];
    }
    StringCpy(nm->givenname3, words[o+3], PHRASELEN);
    if (IsGivennameRelaxed[o+3]) {
      nm->givennames3 = LN(words[o+3], N("given-name"));
    } else {
      nm->givennames3 = GivennameObjs[o+3];
    }
    o += 4;
    goto success;
  } else if (revok && LenGE(len, 4, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] &&
             term[o+0] == ',' && IsGivenname[o+1] && WH(term[o+1]) &&
             IsInitial[o+2] &&
             IsInitial[o+3]) {
  /* Rey, Karen A. M. */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    StringCpy(nm->givenname1, words[o+1], PHRASELEN);
    if (IsGivennameRelaxed[o+1]) {
      nm->givennames1 = LN(words[o+1], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+1];
    }
    nm->giveninitial2 = *words[o+2];
    nm->giveninitial3 = *words[o+3];
    o += 4;
    goto success;
  } else if (revok && LenGE(len, 4, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] &&
             term[o+0] == ',' && IsInitial[o+1] && IsInitial[o+2] &&
             IsInitial[o+3]) {
  /* Rey, K. A. M. */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->giveninitial1 = *words[o+1];
    nm->givenname12_sep = term[o+1];
    nm->giveninitial2 = *words[o+2];
    nm->giveninitial3 = *words[o+3];
    o += 4;
    goto success;
  } else if (LenGE(len, 4, len_exact, IsPosttitle, o) && IsGivenname[o+0] &&
             WH(term[o+0]) && IsGivenname[o+1] && WH(term[o+1]) &&
             IsGivenname[o+2] &&
             WH(term[o+2]) && IsSurname[o+3]) {
  /* Karen Anne Marie Rey */
    StringCpy(nm->givenname1, words[o+0], PHRASELEN);
    if (IsGivennameRelaxed[o+0]) {
      nm->givennames1 = LN(words[o+0], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+0];
    }
    nm->givenname12_sep = term[o+0];
    StringCpy(nm->givenname2, words[o+1], PHRASELEN);
    if (IsGivennameRelaxed[o+1]) {
      nm->givennames2 = LN(words[o+1], N("given-name"));
    } else {
      nm->givennames2 = GivennameObjs[o+1];
    }
    StringCpy(nm->givenname3, words[o+2], PHRASELEN);
    if (IsGivennameRelaxed[o+2]) {
      nm->givennames3 = LN(words[o+2], N("given-name"));
    } else {
      nm->givennames3 = GivennameObjs[o+2];
    }
    StringCpy(nm->surname1, words[o+3], PHRASELEN);
    if (IsSurnameRelaxed[o+3]) nm->surnames1 = LN(words[o+3], N("surname"));
    else nm->surnames1 = SurnameObjs[o+3];
    o += 4;
    goto success;
  } else if (LenGE(len, 4, len_exact, IsPosttitle, o) && IsGivenname[o+0] &&
             WH(term[o+0]) && IsInitial[o+1] && IsInitial[o+2] &&
             IsSurname[o+3]) {
  /* Karen A. M. Rey */
    StringCpy(nm->givenname1, words[o+0], PHRASELEN);
    if (IsGivennameRelaxed[o+0]) {
      nm->givennames1 = LN(words[o+0], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+0];
    }
    nm->giveninitial2 = *words[o+1];
    nm->giveninitial3 = *words[o+2];
    StringCpy(nm->surname1, words[o+3], PHRASELEN);
    if (IsSurnameRelaxed[o+3]) nm->surnames1 = LN(words[o+3], N("surname"));
    else nm->surnames1 = SurnameObjs[o+3];
    o += 4;
    goto success;
  } else if (LenGE(len, 4, len_exact, IsPosttitle, o) && IsInitial[o+0] &&
             IsInitial[o+1] && IsInitial[o+2] &&
             IsSurname[o+3]) {
  /* K. A. M. Rey */
    nm->giveninitial1 = *words[o+0];
    nm->givenname12_sep = term[o+0];
    nm->giveninitial2 = *words[o+1];
    nm->giveninitial3 = *words[o+2];
    StringCpy(nm->surname1, words[o+3], PHRASELEN);
    if (IsSurnameRelaxed[o+3]) nm->surnames1 = LN(words[o+3], N("surname"));
    else nm->surnames1 = SurnameObjs[o+3];
    o += 4;
    goto success;
  } else if (revok && LenGE(len, 4, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] &&
             term[o+0] == '-' && IsSurname[o+1] && term[o+1] == ',' &&
             IsGivenname[o+2] && WD(term[o+2]) && IsGivenname[o+3]) {
  /* Rey-Debove, Karen(-)Anne */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->surname12_sep = term[o+0];
    StringCpy(nm->surname2, words[o+1], PHRASELEN);
    if (IsSurnameRelaxed[o+1]) nm->surnames2 = LN(words[o+1], N("surname"));
    else nm->surnames2 = SurnameObjs[o+1];
    StringCpy(nm->givenname1, words[o+2], PHRASELEN);
    if (IsGivennameRelaxed[o+2]) {
      nm->givennames1 = LN(words[o+2], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+2];
    }
    nm->givenname12_sep = term[o+2];
    StringCpy(nm->givenname2, words[o+3], PHRASELEN);
    if (IsGivennameRelaxed[o+3]) {
      nm->givennames2 = LN(words[o+3], N("given-name"));
    } else {
      nm->givennames2 = GivennameObjs[o+3];
    }
    o += 4;
    goto success;
  } else if (revok && LenGE(len, 4, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] &&
             term[o+0] == '-' && IsSurname[o+1] && term[o+1] == ',' &&
             IsGivenname[o+2] && WH(term[o+2]) && IsInitial[o+3]) {
  /* Rey-Debove, Karen A. */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->surname12_sep = term[o+0];
    StringCpy(nm->surname2, words[o+1], PHRASELEN);
    if (IsSurnameRelaxed[o+1]) nm->surnames2 = LN(words[o+1], N("surname"));
    else nm->surnames2 = SurnameObjs[o+1];
    StringCpy(nm->givenname1, words[o+2], PHRASELEN);
    if (IsGivennameRelaxed[o+2]) {
      nm->givennames1 = LN(words[o+2], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+2];
    }
    nm->giveninitial2 = *words[o+3];
    o += 4;
    goto success;
  } else if (revok && LenGE(len, 4, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] &&
             term[o+0] == '-' && IsSurname[o+1] && term[o+1] == ',' &&
             IsInitial[o+2] &&
             IsInitial[o+3]) {
  /* Rey-Debove, K. A. */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->surname12_sep = term[o+0];
    StringCpy(nm->surname2, words[o+1], PHRASELEN);
    if (IsSurnameRelaxed[o+1]) nm->surnames2 = LN(words[o+1], N("surname"));
    else nm->surnames2 = SurnameObjs[o+1];
    nm->giveninitial1 = *words[o+2];
    nm->giveninitial2 = *words[o+3];
    o += 4;
    goto success;
  } else if (LenGE(len, 4, len_exact, IsPosttitle, o) && IsGivenname[o+0] &&
             WD(term[o+0]) && IsGivenname[o+1] && WH(term[o+1]) &&
             IsSurname[o+2] && term[o+2] == '-' && IsSurname[o+3]) {
  /* Karen(-)Anne Rey-Debove */
    StringCpy(nm->givenname1, words[o+0], PHRASELEN);
    if (IsGivennameRelaxed[o+0]) {
      nm->givennames1 = LN(words[o+0], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+0];
    }
    nm->givenname12_sep = term[o+0];
    StringCpy(nm->givenname2, words[o+1], PHRASELEN);
    if (IsGivennameRelaxed[o+1]) {
      nm->givennames2 = LN(words[o+1], N("given-name"));
    } else {
      nm->givennames2 = GivennameObjs[o+1];
    }
    StringCpy(nm->surname1, words[o+2], PHRASELEN);
    if (IsSurnameRelaxed[o+2]) nm->surnames1 = LN(words[o+2], N("surname"));
    else nm->surnames1 = SurnameObjs[o+2];
    nm->surname12_sep = term[o+2];
    StringCpy(nm->surname2, words[o+3], PHRASELEN);
    if (IsSurnameRelaxed[o+3]) nm->surnames2 = LN(words[o+3], N("surname"));
    else nm->surnames2 = SurnameObjs[o+3];
    o += 4;
    goto success;
  } else if (LenGE(len, 4, len_exact, IsPosttitle, o) && IsGivenname[o+0] &&
             WH(term[o+0]) && IsInitial[o+1] && IsSurname[o+2] &&
             term[o+2] == '-' && IsSurname[o+3]) {
  /* Karen A. Rey-Debove */
    StringCpy(nm->givenname1, words[o+0], PHRASELEN);
    if (IsGivennameRelaxed[o+0]) {
      nm->givennames1 = LN(words[o+0], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+0];
    }
    nm->givenname12_sep = term[o+0];
    StringCpy(nm->surname1, words[o+2], PHRASELEN);
    if (IsSurnameRelaxed[o+2]) nm->surnames1 = LN(words[o+2], N("surname"));
    else nm->surnames1 = SurnameObjs[o+2];
    nm->surname12_sep = term[o+2];
    StringCpy(nm->surname2, words[o+3], PHRASELEN);
    if (IsSurnameRelaxed[o+3]) nm->surnames2 = LN(words[o+3], N("surname"));
    else nm->surnames2 = SurnameObjs[o+3];
    o += 4;
    goto success;
  } else if (LenGE(len, 4, len_exact, IsPosttitle, o) && IsInitial[o+0] &&
             IsInitial[o+1] && IsSurname[o+2] &&
             term[o+2] == '-' && IsSurname[o+3]) {
  /* K. A. Rey-Debove */
    nm->giveninitial1 = *words[o+0];
    nm->givenname12_sep = term[o+0];
    nm->giveninitial2 = *words[o+1];
    StringCpy(nm->surname1, words[o+2], PHRASELEN);
    if (IsSurnameRelaxed[o+2]) nm->surnames1 = LN(words[o+2], N("surname"));
    else nm->surnames1 = SurnameObjs[o+2];
    nm->surname12_sep = term[o+2];
    StringCpy(nm->surname2, words[o+3], PHRASELEN);
    if (IsSurnameRelaxed[o+3]) nm->surnames2 = LN(words[o+3], N("surname"));
    else nm->surnames2 = SurnameObjs[o+3];
    o += 4;
    goto success;
  } else if (revok && LenGE(len, 4, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] &&
             term[o+0] == ',' && IsInitial[o+1] && IsGivenname[o+2] &&
             WH(term[o+2]) && IsInitial[o+3]) {
  /* Garnier, J. Alfred B. */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->giveninitial1 = *words[o+1];
    StringCpy(nm->givenname2, words[o+2], PHRASELEN);
    if (IsGivennameRelaxed[o+2]) {
      nm->givennames2 = LN(words[o+2], N("given-name"));
    } else {
      nm->givennames2 = GivennameObjs[o+2];
    }
    nm->giveninitial3 = *words[o+3];
    o += 4;
    goto success;
  } else if (LenGE(len, 4, len_exact, IsPosttitle, o) && IsInitial[o+0] &&
             IsGivenname[o+1] && WH(term[o+1]) && IsInitial[o+2] &&
             IsSurname[o+3]) {
  /* J. Alfred B. Garnier */
    nm->giveninitial1 = *words[o+0];
    StringCpy(nm->givenname2, words[o+1], PHRASELEN);
    if (IsGivennameRelaxed[o+1]) {
      nm->givennames2 = LN(words[o+1], N("given-name"));
    } else {
      nm->givennames2 = GivennameObjs[o+1];
    }
    nm->giveninitial3 = *words[o+2];
    StringCpy(nm->surname1, words[o+3], PHRASELEN);
    if (IsSurnameRelaxed[o+3]) nm->surnames1 = LN(words[o+3], N("surname"));
    else nm->surnames1 = SurnameObjs[o+3];
    o += 4;
    goto success;
  } else if (revok && LenGE(len, 3, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] && term[o+0] == ',' && IsGivenname[o+1] &&
             WD(term[o+1]) && IsGivenname[o+2]) {
  /* Garnier, Jim(-)Alfred */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    StringCpy(nm->givenname1, words[o+1], PHRASELEN);
    if (IsGivennameRelaxed[o+1]) {
      nm->givennames1 = LN(words[o+1], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+1];
    }
    nm->givenname12_sep = term[o+1];
    StringCpy(nm->givenname2, words[o+2], PHRASELEN);
    if (IsGivennameRelaxed[o+2]) {
      nm->givennames2 = LN(words[o+2], N("given-name"));
    } else {
      nm->givennames2 = GivennameObjs[o+2];
    }
    o += 3;
    goto success;
  } else if (revok && LenGE(len, 3, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] && term[o+0] == ',' && IsInitial[o+1] &&
             IsGivenname[o+2]) {
  /* Garnier, J. Alfred */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->giveninitial1 = *words[o+1];
    StringCpy(nm->givenname2, words[o+2], PHRASELEN);
    if (IsGivennameRelaxed[o+2]) {
      nm->givennames2 = LN(words[o+2], N("given-name"));
    } else {
      nm->givennames2 = GivennameObjs[o+2];
    }
    o += 3;
    goto success;
  } else if (revok && LenGE(len, 3, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] && term[o+0] == ',' && IsGivenname[o+1] &&
             WH(term[o+1]) && IsInitial[o+2]) {
  /* Garnier, Jim A. */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    StringCpy(nm->givenname1, words[o+1], PHRASELEN);
    if (IsGivennameRelaxed[o+1]) {
      nm->givennames1 = LN(words[o+1], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+1];
    }
    nm->giveninitial2 = *words[o+2];
    o += 3;
    goto success;
  } else if (revok && LenGE(len, 3, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] && term[o+0] == ',' && IsInitial[o+1] &&
             IsInitial[o+2]) {
  /* Garnier, J. A. */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->giveninitial1 = *words[o+1];
    nm->givenname12_sep = term[o+1];
    nm->giveninitial2 = *words[o+2];
    o += 3;
    goto success;
  } else if (LenGE(len, 3, len_exact, IsPosttitle, o) && IsGivenname[o+0] &&
             WH(term[o+0]) && (IsSurname[o+1] && !IsSurnameRelaxed[o+1]) &&
             WH(term[o+1]) && IsSurname[o+2]) {
  /* Karen Rey Debove */
    StringCpy(nm->givenname1, words[o+0], PHRASELEN);
    if (IsGivennameRelaxed[o+0]) {
      nm->givennames1 = LN(words[o+0], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+0];
    }
    nm->givenname12_sep = term[o+0];
    StringCpy(nm->givenname2, words[o+1], PHRASELEN);
    nm->givennames2 = GivennameObjs[o+1];
    StringCpy(nm->surname1, words[o+2], PHRASELEN);
    if (IsSurnameRelaxed[o+2]) nm->surnames1 = LN(words[o+2], N("surname"));
    else nm->surnames1 = SurnameObjs[o+2];
    o += 3;
    goto success;
  } else if (LenGE(len, 3, len_exact, IsPosttitle, o) && IsGivenname[o+0] &&
             WD(term[o+0]) && IsGivenname[o+1] && WH(term[o+1]) &&
             IsSurname[o+2]) {
  /* Jim(-)Alfred Garnier */
    StringCpy(nm->givenname1, words[o+0], PHRASELEN);
    if (IsGivennameRelaxed[o+0]) {
      nm->givennames1 = LN(words[o+0], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+0];
    }
    nm->givenname12_sep = term[o+0];
    StringCpy(nm->givenname2, words[o+1], PHRASELEN);
    if (IsGivennameRelaxed[o+1]) {
      nm->givennames2 = LN(words[o+1], N("given-name"));
    } else {
      nm->givennames2 = GivennameObjs[o+1];
    }
    StringCpy(nm->surname1, words[o+2], PHRASELEN);
    if (IsSurnameRelaxed[o+2]) nm->surnames1 = LN(words[o+2], N("surname"));
    else nm->surnames1 = SurnameObjs[o+2];
    o += 3;
    goto success;
  } else if (LenGE(len, 3, len_exact, IsPosttitle, o) && IsInitial[o+0] &&
            IsGivenname[o+1] && WH(term[o+1]) && IsSurname[o+2]) {
  /* J. Alfred Garnier */
    nm->giveninitial1 = *words[o+0];
    nm->givenname12_sep = term[o+0];
    StringCpy(nm->givenname2, words[o+1], PHRASELEN);
    if (IsGivennameRelaxed[o+1]) {
      nm->givennames2 = LN(words[o+1], N("given-name"));
    } else {
      nm->givennames2 = GivennameObjs[o+1];
    }
    StringCpy(nm->surname1, words[o+2], PHRASELEN);
    if (IsSurnameRelaxed[o+2]) nm->surnames1 = LN(words[o+2], N("surname"));
    else nm->surnames1 = SurnameObjs[o+2];
    o += 3;
    goto success;
  } else if (LenGE(len, 3, len_exact, IsPosttitle, o) && IsGivenname[o+0] &&
             WH(term[o+0]) && IsInitial[o+1] && IsSurname[o+2]) {
  /* Jim A. Garnier */
    StringCpy(nm->givenname1, words[o+0], PHRASELEN);
    if (IsGivennameRelaxed[o+0]) {
      nm->givennames1 = LN(words[o+0], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+0];
    }
    nm->giveninitial2 = *words[o+1];
    StringCpy(nm->surname1, words[o+2], PHRASELEN);
    if (IsSurnameRelaxed[o+2]) nm->givennames1 = LN(words[o+2], N("surname"));
    else nm->givennames1 = SurnameObjs[o+2];
    o += 3;
    goto success;
  } else if (LenGE(len, 3, len_exact, IsPosttitle, o) && IsInitial[o+0] &&
             IsInitial[o+1] && IsSurname[o+2]) {
  /* J. A. Garnier */
    nm->giveninitial1 = *words[o+0];
    nm->givenname12_sep = term[o+0];
    nm->giveninitial2 = *words[o+1];
    StringCpy(nm->surname1, words[o+2], PHRASELEN);
    if (IsSurnameRelaxed[o+2]) nm->surnames1 = LN(words[o+2], N("surname"));
    else nm->surnames1 = SurnameObjs[o+2];
    o += 3;
    goto success;
  } else if (revok && LenGE(len, 3, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] && term[o+0] == '-' && IsSurname[o+1] &&
             term[o+1] == ',' && IsGivenname[o+2]) {
  /* Rey-Debove, Karen */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->surname12_sep = term[o+0];
    StringCpy(nm->surname2, words[o+1], PHRASELEN);
    if (IsSurnameRelaxed[o+1]) nm->surnames2 = LN(words[o+1], N("surname"));
    else nm->surnames2 = SurnameObjs[o+1];
    StringCpy(nm->givenname1, words[o+2], PHRASELEN);
    if (IsGivennameRelaxed[o+2]) {
      nm->givennames1 = LN(words[o+2], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+2];
    }
    o += 3;
    goto success;
  } else if (revok && LenGE(len, 3, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] && term[o+0] == '-' && IsSurname[o+1] &&
             term[o+1] == ',' && IsInitial[o+2]) {
  /* Rey-Debove, K. */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->surname12_sep = term[o+0];
    StringCpy(nm->surname2, words[o+1], PHRASELEN);
    if (IsSurnameRelaxed[o+1]) nm->surnames2 = LN(words[o+1], N("surname"));
    else nm->surnames2 = SurnameObjs[o+1];
    nm->giveninitial1 = *words[o+2];
    o += 3;
    goto success;
  } else if (LenGE(len, 3, len_exact, IsPosttitle, o) && IsGivenname[o+0] &&
             WH(term[o+0]) && IsSurname[o+1] && term[o+1] == '-' &&
             IsSurname[o+2]) {
  /* Karen Rey-Debove */
    StringCpy(nm->givenname1, words[o+0], PHRASELEN);
    if (IsGivennameRelaxed[o+0]) {
      nm->givennames1 = LN(words[o+0], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+0];
    }
    StringCpy(nm->surname1, words[o+1], PHRASELEN);
    if (IsSurnameRelaxed[o+1]) nm->surnames1 = LN(words[o+1], N("surname"));
    else nm->surnames1 = SurnameObjs[o+1];
    nm->surname12_sep = term[o+1];
    StringCpy(nm->surname2, words[o+2], PHRASELEN);
    if (IsSurnameRelaxed[o+2]) nm->surnames2 = LN(words[o+2], N("surname"));
    else nm->surnames2 = SurnameObjs[o+2];
    o += 3;
    goto success;
  } else if (LenGE(len, 3, len_exact, IsPosttitle, o) && IsInitial[o+0] &&
             IsSurname[o+1] && term[o+1] == '-' && IsSurname[o+2]) {
  /* K. Rey-Debove */
    nm->giveninitial1 = *words[o+0];
    StringCpy(nm->surname1, words[o+1], PHRASELEN);
    if (IsSurnameRelaxed[o+1]) nm->surnames1 = LN(words[o+1], N("surname"));
    else nm->surnames1 = SurnameObjs[o+1];
    nm->surname12_sep = term[o+1];
    StringCpy(nm->surname2, words[o+2], PHRASELEN);
    if (IsSurnameRelaxed[o+2]) nm->surnames2 = LN(words[o+2], N("surname"));
    else nm->surnames2 = SurnameObjs[o+2];
    o += 3;
    goto success;
  } else if (revok && LenGE(len, 2, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] && term[o+0] == ',' && IsGivenname[o+1]) {
  /* Garnier, Jim */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    StringCpy(nm->givenname1, words[o+1], PHRASELEN);
    if (IsGivennameRelaxed[o+1]) {
      nm->givennames1 = LN(words[o+1], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+1];
    }
    o += 2;
    goto success;
  } else if (revok && LenGE(len, 2, len_exact, IsPosttitle, o) &&
             IsSurname[o+0] && term[o+0] == ',' && IsInitial[o+1]) {
  /* Garnier, J. */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->giveninitial1 = *words[o+1];
    o += 2;
    goto success;
  } else if (LenGE(len, 2, len_exact, IsPosttitle, o) && IsGivenname[o+0] &&
             WH(term[o+0]) && IsSurname[o+1]) {
  /* Jim Garnier */
    StringCpy(nm->givenname1, words[o+0], PHRASELEN);
    if (IsGivennameRelaxed[o+0]) {
      nm->givennames1 = LN(words[o+0], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+0];
    }
    StringCpy(nm->surname1, words[o+1], PHRASELEN);
    if (IsSurnameRelaxed[o+1]) nm->surnames1 = LN(words[o+1], N("surname"));
    else nm->surnames1 = SurnameObjs[o+1];
    o += 2;
    goto success;
  } else if (LenGE(len, 2, len_exact, IsPosttitle, o) && IsInitial[o+0] &&
             IsSurname[o+1]) {
  /* J. Garnier */
    nm->giveninitial1 = *words[o+0];
    StringCpy(nm->surname1, words[o+1], PHRASELEN);
    if (IsSurnameRelaxed[o+1]) nm->surnames1 = LN(words[o+1], N("surname"));
    else nm->surnames1 = SurnameObjs[o+1];
    o += 2;
    goto success;
  } else if (LenGE(len, 2, len_exact, IsPosttitle, o) && IsSurname[o+0] &&
             term[o+0] == '-' && IsSurname[o+1]) {
  /* Rey-Debove */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    nm->surname12_sep = term[o+0];
    StringCpy(nm->surname2, words[o+1], PHRASELEN);
    if (IsSurnameRelaxed[o+1]) nm->surnames2 = LN(words[o+1], N("surname"));
    else nm->surnames2 = SurnameObjs[o+1];
    o += 2;
    goto success;
  } else if (LenGE(len, 2, len_exact, IsPosttitle, o) && IsGivenname[o+0] &&
             WD(term[o+0]) && IsGivenname[o+1]) {
  /* Karen(-)Anne */
    StringCpy(nm->givenname1, words[o+0], PHRASELEN);
    if (IsGivennameRelaxed[o+0]) {
      nm->givennames1 = LN(words[o+0], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+0];
    }
    nm->givenname12_sep = term[o+0];
    StringCpy(nm->givenname2, words[o+1], PHRASELEN);
    if (IsGivennameRelaxed[o+1]) {
      nm->givennames2 = LN(words[o+1], N("given-name"));
    } else {
      nm->givennames2 = GivennameObjs[o+1];
    }
    o += 2;
    goto success;
  } else if (LenGE(len, 1, len_exact, IsPosttitle, o) && IsSurname[o+0]) {
  /* Garnier */
    StringCpy(nm->surname1, words[o+0], PHRASELEN);
    if (IsSurnameRelaxed[o+0]) nm->surnames1 = LN(words[o+0], N("surname"));
    else nm->surnames1 = SurnameObjs[o+0];
    o += 1;
    goto success;
  } else if (LenGE(len, 1, len_exact, IsPosttitle, o) && IsGivenname[o+0]) {
  /* Jim */
    StringCpy(nm->givenname1, words[o+0], PHRASELEN);
    if (IsGivennameRelaxed[o+0]) {
      nm->givennames1 = LN(words[o+0], N("given-name"));
    } else {
      nm->givennames1 = GivennameObjs[o+0];
    }
    o += 1;
    goto success;
  }
  return(0);

success:
  *offset = o;
  return(1);
}

/* todoDATABASE: If these are added to the database, we should lock them
 * out of normal parsing. In fact, we could do this for all items used
 * only by text agents.
 */
Bool NameIsParticle(char *out)
{
  return(streq(out, "O") || streq(out, "d") || streq(out, "de") ||
         streq(out, "De") || streq(out, "du") || streq(out, "Du") ||
         streq(out, "le") || streq(out, "Le") || streq(out, "la") ||
         streq(out, "La") || streq(out, "St") || streq(out, "van") ||
         streq(out, "Van") || streq(out, "von") || streq(out, "Von") ||
         streq(out, "Ben"));
}

#define MAXNAMES	10

/* John P. Davis
 * Henri IV
 * John Paul II
 * Jean-Pierre Rampall
 * Martin Peter King, Jr.
 * M. Durand
 * Monsieur Durand
 * Messieurs les jurés
 * Messieurs Durand
 * President Kennedy
 * de Gaulle
 * Garnier, Jim
 */
Name *TA_NameParse(char *in, int gender, int known_to_be_name,
                   int revok, /* RESULTS */ char **nextp, char *trailpunc)
{
  int		i, len, o, prev_was_particle;
  int		IsPretitle[MAXNAMES], IsGivenname[MAXNAMES];
  int		IsGivennameRelaxed[MAXNAMES];
  int		IsGivennameEither[MAXNAMES], IsInitial[MAXNAMES];
  int		IsSurname[MAXNAMES], IsSurnameRelaxed[MAXNAMES];
  int		IsSurnameEither[MAXNAMES];
  int		IsPosttitle[MAXNAMES], term[MAXNAMES];
  char		*orig_in, *rest[MAXNAMES], *words[MAXNAMES], *puncs[MAXNAMES];
  char		word[PHRASELEN], punc[PUNCLEN];
  ObjList	*PretitleObjs[MAXNAMES], *GivennameObjs[MAXNAMES];
  ObjList	*SurnameObjs[MAXNAMES], *PosttitleObjs[MAXNAMES];
  Name		*nm;

  nm = NULL;
  orig_in = in;
  /* (1) Soak up contiguous name-like entities from input stream. */
  i = 0;
  len = 0; /* todoFREE: Freeing on failure before len set to i (words, punc). */
  prev_was_particle = 0;
  while (i < MAXNAMES) {
    if (in[0] == TERM) break;
    if (!StringGetWord_LeNonwhite(word, in, PHRASELEN, &in, punc)) break;
    term[i] = *((uc *)punc);
    puncs[i] = StringCopy(punc, "TA_ParseName char *");
    if (NameIsParticle(word)) {
      prev_was_particle = 1;
      continue;	/* todo: Store particles. */
    }
    if (!CharIsUpper(*((uc *)word))) break;
    if (!StringIsRomanNumeral(word)) {
      StringAllUpperToUL(word); /* GARNIER => Garnier */
    }
    words[i] = StringCopy(word, "TA_ParseName char *");
    rest[i] = in;
    IsPretitle[i] = IsGivenname[i] = IsGivennameRelaxed[i] = 0;
    IsSurname[i] = IsSurnameRelaxed[i] = IsPosttitle[i] = 0;
    if (word[1] == TERM || (word[1] == '.' && word[2] == TERM)) {
      if (prev_was_particle) goto failure; /* todoSCORE: Locks out "de F." */
      IsInitial[i] = 1;
    } else {
      StringElimChar(word, '.');
      IsInitial[i] = 0;
      TA_NameParse1(word, N("post-title"), 0, 0, &IsPosttitle[i], NULL,
                    &PosttitleObjs[i]);
      if (!IsPosttitle[i]) {
        TA_NameParse1(word, N("pre-title"), 0, 0, &IsPretitle[i], NULL,
                      &PretitleObjs[i]);
        TA_NameParse1(word, N("given-name"), 1, known_to_be_name,
                      &IsGivenname[i], &IsGivennameRelaxed[i],
                      &GivennameObjs[i]);
        TA_NameParse1(word, N("surname"), 1, known_to_be_name, &IsSurname[i],
                      &IsSurnameRelaxed[i], &SurnameObjs[i]);
        if (prev_was_particle) {
          if (IsGivenname[i] && (!IsSurname[i])) {
          /* todoSCORE: Locks out "de <given name> which is not also a
           * <surname>".
           */
            goto failure;
          }
        }
      } else {
        if (prev_was_particle) goto failure;
          /* todoSCORE: Locks out "de PhD." */
      }
    }
    IsGivennameEither[i] = IsGivenname[i] || IsGivennameRelaxed[i];
    IsSurnameEither[i] = IsSurname[i] || IsSurnameRelaxed[i];
    i++;
    prev_was_particle = 0;
  }
  if (i == 0) return(NULL);
  len = i;
  nm = NameCreate();

  /* (2) Parse results. */
  o = 0;
  while (o < 2 && o < len && IsPretitle[o]) {
  /* "Dr" "M." "Mme" -- todo: Maintain multiple title text. */
    StringCpy(nm->pretitle, words[o], PHRASELEN);
    nm->pretitles = ObjListAppendDestructive(nm->pretitles, PretitleObjs[o]);
    o++;
  }
  len -= o;

  /* (2A) Try parsing without relaxation. */
  if (!known_to_be_name) {
    if (TA_NameParse2(nm, len, term, IsGivenname, IsSurname, IsInitial,
                      IsGivennameRelaxed, IsSurnameRelaxed, IsPosttitle,
                      GivennameObjs, SurnameObjs, words, orig_in, gender,
                      0, revok, &o)) {
      goto post_titles;
    }
  }
  /* (2B) Allow relaxation. todoSCORE */
  if (TA_NameParse2(nm, len, term, IsGivennameEither, IsSurnameEither,
                    IsInitial, IsGivennameRelaxed, IsSurnameRelaxed,
                    IsPosttitle, GivennameObjs, SurnameObjs, words, orig_in,
                    gender, known_to_be_name, revok, &o)) {
    goto post_titles;
  }

failure:
  if (nm) NameFree(nm);
  nm = NULL;
  goto done;

post_titles:
  while (o < len && IsPosttitle[o]) {
  /* "Esq" "IV" -- todo: Maintain multiple title text. */
    StringCpy(nm->posttitle, words[o], PHRASELEN);
    nm->posttitles = ObjListAppendDestructive(nm->posttitles, PosttitleObjs[o]);
    o++;
  }
  *nextp = rest[o-1];
  if (trailpunc) StringCpy(trailpunc, puncs[o-1], PUNCLEN);
  nm->fullname[0] = TERM;
  if (nm->pretitle[0]) {
    StringCat(nm->fullname, nm->pretitle, PHRASELEN);
    StringAppendChar(nm->fullname, PHRASELEN, SPACE);
  }
  if (nm->givenname1[0]) {
    StringCat(nm->fullname, nm->givenname1, PHRASELEN);
    StringAppendChar(nm->fullname, PHRASELEN, SPACE);
  }
  if (nm->givenname2[0]) {
    StringCat(nm->fullname, nm->givenname2, PHRASELEN);
    StringAppendChar(nm->fullname, PHRASELEN, SPACE);
  }
  if (nm->givenname3[0]) {
    StringCat(nm->fullname, nm->givenname3, PHRASELEN);
    StringAppendChar(nm->fullname, PHRASELEN, SPACE);
  }
  if (nm->surname1[0]) {
    StringCat(nm->fullname, nm->surname1, PHRASELEN);
    StringAppendChar(nm->fullname, PHRASELEN, SPACE);
  }
  if (nm->surname2[0]) {
    StringCat(nm->fullname, nm->surname2, PHRASELEN);
    StringAppendChar(nm->fullname, PHRASELEN, SPACE);
  }
  if (nm->posttitle[0]) {
    StringCat(nm->fullname, nm->posttitle, PHRASELEN);
    StringAppendChar(nm->fullname, PHRASELEN, SPACE);
  }
  StringElimTrailingBlanks(nm->fullname);
  if (nm->givenname12_sep != '-') nm->givenname12_sep = SPACE;
  if (nm->surname12_sep != '-') nm->surname12_sep = SPACE;

done:
  for (i = 0; i < len; i++) {
    MemFree(words[i], "TA_ParseName char *");
    MemFree(puncs[i], "TA_ParseName char *");
  }
  /* todoFREE: Unused ObjLists. */
  return(nm);
}

Name *TA_NameParseKnown(char *name, int gender, int revok)
{
  char	*rest;
  Name	*nm;
  if (NULL == (nm = TA_NameParse(name, gender, 1, 0, &rest, NULL))) {
    Dbg(DBGLEX, DBGBAD, "unable to parse name <%s>", name);
    /* For debugging: */
    TA_NameParse(name, gender, 1, 0, &rest, NULL);
    return(NULL);
  }
  if (*rest != TERM) {
    Dbg(DBGLEX, DBGBAD, "trouble parsing name <%s>", name);
  }
  if (DbgOn(DBGLEX, DBGHYPER)) {
    NamePrint(Log, nm);
    fputc(NEWLINE, Log);
  }
  return(nm);
}

Bool TA_Name(char *in, Discourse *dc, /* RESULTS */ Channel *ch, char **nextp)
{
  char		*orig_in, trailpunc[PUNCLEN];
  Name		*nm;
  orig_in = in;
  if ((nm = TA_NameParse(in, F_NULL, 0, 0, &in, trailpunc))) {
    ChannelAddPNode(ch, PNTYPE_NAME, 1.0, nm, trailpunc, orig_in, in);
    *nextp = in;
    return(1);
  }
  return(0);
}

/* French format polities.
 *
 * Examples:
 * New Haven (Connecticut)
 * Baltimore (Maryland)
 * Bethesda (Maryland)
 * Colombier-Châtelot (Doubs)
 * Bourges (Cher)
 * Saumur (Maine-et-Loire)
 * Cendrieux (Dordogne)
 */
Bool TA_FrenchPolity(char *in, Discourse *dc,
                     /* RESULTS */ Channel *ch, char **nextp)
{
  char		subpolity[PHRASELEN], superpolity[PHRASELEN], *orig_in;
  ObjList	*subpolities, *superpolities, *p, *q, *r;
  int	i;
  orig_in = in;
  i = 0;
  while (*in && (*in == '-' || CharIsLower(*in) || CharIsUpper(*in))) {
    if (i >= (PHRASELEN-1)) return(0);
    subpolity[i] = *in;
    in++;
    i++;
  }
  if (i == 0) return(0);
  subpolity[i] = TERM;
  in = StringSkipWhitespace(in);
  if (*in != LPAREN) return(0);
  in++;
  StringElimLeadingBlanks(subpolity);
  StringElimTrailingBlanks(subpolity);
  StringMapLeWhitespaceToSpace(subpolity);	/* todo */
  i = 0;
  while (*in && (*in == '-' || CharIsLower(*in) || CharIsUpper(*in))) {
    if (i >= 50) return(0); /* todoSCORE */
    if (i >= (PHRASELEN-1)) return(0);
    superpolity[i] = *in;
    in++;
    i++;
  }
  if (i == 0) return(0);
  superpolity[i] = TERM;
  in = StringSkipWhitespace(in);
  if (*in != RPAREN) return(0);
  in++;
  StringElimLeadingBlanks(superpolity);
  StringElimTrailingBlanks(superpolity);
  StringMapLeWhitespaceToSpace(superpolity);	/* todo */
  subpolities = TA_FindPhraseTryBoth(DC(dc).ht, subpolity, N("polity"));
  superpolities = TA_FindPhraseTryBoth(DC(dc).ht, superpolity, N("polity"));
  r = NULL;
  for (p = subpolities; p; p = p->next) {
    for (q = superpolities; q; q = q->next) {
      if (PolityContained(&TsNA, NULL, p->obj, q->obj)) {
        r = ObjListCreate(p->obj, r);
        break;
      }
    }
  }
  ObjListFree(subpolities);
  ObjListFree(superpolities);
  if (r == NULL) {
    /* todo: If superpolities, learn new subpolities. */
    return(0);
  }
  ChannelAddPNode(ch, PNTYPE_POLITY, 1.0, r, NULL, orig_in, in);
  *nextp = in;
  return(1);
}

/* End of file. */
