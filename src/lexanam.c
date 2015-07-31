/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Analogical inflectional morphology.
 *
 * 19940903: begun
 * 19940904: finishing touches
 *
 * This uses up lots of memory because of all the combinations that are stored.
 * But it works much better than AlgMorph.
 */

#include "tt.h"
#include "lexanam.h"
#include "lexentry.h"
#include "repbasic.h"
#include "repobj.h"
#include "repstr.h"
#include "reptext.h"
#include "utildbg.h"
#include "utilrpt.h"

HashTable	*AnaMorphHt;
AnaMorphClass	*AnaMorphClassAll;

/* AnaMorphClassList */

AnaMorphClassList *AnaMorphClassListCreate(AnaMorphClass *mc,
                                           AnaMorphClassList *next)
{
  AnaMorphClassList	*mcs;
  mcs = CREATE(AnaMorphClassList);
  mcs->mc = mc;
  mcs->next = next;
  return(mcs);
}

AnaMorphClassList *AnaMorphClassListGet(char *suffix)
{
  return((AnaMorphClassList *)HashTableGet(AnaMorphHt, suffix));
}

void AnaMorphClassListSet(char *suffix, AnaMorphClassList *mcs)
{
  HashTableSet(AnaMorphHt, suffix, mcs);
}

/* AnaMorphClass */

void AnaMorphInit()
{
  AnaMorphHt = HashTableCreate(30001L);
  AnaMorphClassAll = NULL;
}

AnaMorphClass *AnaMorphClassCreate(int lang, int pos, int gender, LexEntry *le)
{
  AnaMorphClass	*mc;
  mc = CREATE(AnaMorphClass);
  mc->count = 0;
  mc->lang = lang;
  mc->pos = pos;
  mc->gender = gender;
  mc->example = le;
  mc->infls = NULL;
  mc->next = NULL;
  return(mc);
}

void AnaMorphClassFree(AnaMorphClass *mc)
{
  MemFree(mc, "AnaMorphClass");
}

void AnaMorphClassIndex1(char *suffix, AnaMorphClass *mc)
{
  AnaMorphClassListSet(suffix,
                       AnaMorphClassListCreate(mc,
                                               AnaMorphClassListGet(suffix)));
}

void AnaMorphClassIndex(AnaMorphClass *mc)
{
  Word	*infl;
  for (infl = mc->infls; infl; infl = infl->next) {
    AnaMorphClassIndex1(infl->word, mc);
  }
}

void ReportAnaMorphHeader(Rpt *rpt, Discourse *dc)
{
  RptStartLine(rpt);
  RptAddConcept(rpt, N("example"), dc);
  RptAddConcept(rpt, N("quantity"), dc);
  RptAddConcept(rpt, N("language"), dc);
  RptAdd(rpt, "", RPT_JUST_LEFT);
  RptAddConcept(rpt, N("grammatical-gender"), dc);
  RptAdd(rpt, "", RPT_JUST_LEFT);
}

void ReportAnaMorphLine(Rpt *rpt, AnaMorphClass *mc, Discourse *dc)
{
  char	*s;
  Text	*text;
  RptStartLine(rpt);
  RptAdd(rpt, mc->example->srcphrase, RPT_JUST_LEFT);
  RptAddInt(rpt, mc->count, dc);
  RptAddFeatAbbrev(rpt, mc->lang, 1, dc);
  RptAddFeatAbbrev(rpt, mc->pos, 1, dc);
  RptAddFeatAbbrev(rpt, mc->gender, 1, dc);
  text = TextCreat(dc);
  WordPrintTextAll(text,  mc->infls);
  s = TextString(text);
  RptAdd(rpt, s, RPT_JUST_LEFT);
  TextFree(text);
}

void ReportAnaMorph1(Rpt *rpt, int countthresh, Discourse *dc)
{
  AnaMorphClass	*mc;
  for (mc = AnaMorphClassAll; mc; mc = mc->next) {
    if (mc->count < countthresh) continue;
    ReportAnaMorphLine(rpt, mc, dc);
  }
  ReportAnaMorphHeader(rpt, dc);
}

void ReportAnaMorph(Text *text, Discourse *dc)
{
  Rpt	*rpt;
  rpt = RptCreate();
  ReportAnaMorph1(rpt, 2, dc);
  RptTextPrint(text, rpt, 1, 1, -1, 45);
  RptFree(rpt);
}

/* TRAINING */

AnaMorphClass *AnaMorphClassFindEquiv(AnaMorphClass *mc)
{
  AnaMorphClassList	*mcs;
#ifdef maxchecking
  if (!mc->infls) {
    Dbg(DBGGEN, DBGBAD, "AnaMorphClassFindEquiv: null infls");
    return(NULL);
  }
#endif
  mcs = AnaMorphClassListGet(mc->infls->word);
  for (; mcs; mcs = mcs->next) {
    if (mcs->mc->lang != mc->lang || mcs->mc->pos != mc->pos ||
        mcs->mc->gender != mc->gender) continue;
    if (WordListEqual(mcs->mc->infls, mc->infls)) return(mcs->mc);
  }
  return(NULL);
}

void AnaMorphTrain2(AnaMorphClass *mc)
{
  AnaMorphClass	*existing;
  if ((existing = AnaMorphClassFindEquiv(mc))) {
    AnaMorphClassFree(mc);
    existing->count++;
  } else {
    mc->count = 1;
    mc->next = AnaMorphClassAll;
    AnaMorphClassAll = mc;
    AnaMorphClassIndex(mc);
  }
}

void AnaMorphTrain1(Word *infl, int stemlen, int lang, int pos, int gender,
                    LexEntry *le)
{
  int			len;
  char			suffix[DWORDLEN], features[FEATLEN];
  AnaMorphClass	*mc;
  mc = AnaMorphClassCreate(lang, pos, gender, le);
  for (; infl; infl = infl->next) {
    len = strlen(infl->word)-stemlen;
    memcpy(suffix, infl->word+stemlen, len);
    suffix[len] = TERM;
    FeatureCopyIn(infl->features, FT_INFL_FILE, features);
    mc->infls = WordCreate(suffix, features, mc->infls, AnaMorphHt);
  }
  AnaMorphTrain2(mc);
}

void AnaMorphTrain(LexEntry *le)
{
  int	stemlen, lang, pos, gender;
  char	stem[DWORDLEN];
  Word	*infl;
  if (!le->infl) return;
  if (StringIn(F_BORROWING, le->features)) return;
  if (CharIsUpper((uc)(le->srcphrase[0]))) return;
  lang = FeatureGetRequired("AnaMorphTrain 1", le->features, FT_LANG);
  pos = FeatureGetRequired("AnaMorphTrain 2", le->features, FT_POS);
  gender = FeatureGet(le->features, FT_GENDER);
  if (lang == F_ENGLISH) {
    if (!StringIn(pos, "NV")) return;
  } else {
    if (!StringIn(pos, "ANV")) return;
  }
  for (stemlen = 0; ; stemlen++) {
    infl = le->infl;
    if (strlen(infl->word) < stemlen) return;
    memcpy(stem, infl->word, stemlen);
    stem[stemlen] = TERM;
    for (infl = infl->next; infl; infl = infl->next) {
      if (strlen(infl->word) < stemlen) return;
      if (0 != memcmp(stem, infl->word, stemlen)) return;
    }
    AnaMorphTrain1(le->infl, stemlen, lang, pos, gender, le);
  }
}

/* USE */

LexEntry *AnaMorphInflectWord2(char *word, int lang, int pos, int gender,
                               char *known_features, char *stem, char *suffix,
                               AnaMorphClass *mc, char *features,
                               int unique_gender, int mascfem, HashTable *ht)
{
  int		i;
  char		leword[DWORDLEN], lefeatures[FEATLEN], *p;
  char		inflword[DWORDLEN], inflfeat[FEATLEN];
  Word		*infl, *newinfl;
  FILE		*sugg;
  LexEntry	*le;
#ifdef maxchecking
  if (!mc->infls) {
    Dbg(DBGGEN, DBGBAD, "AnaMorphInflectWord4: null infls");
    return(NULL);
  }
#endif
  StringCpy(leword, stem, DWORDLEN);
  StringCat(leword, WordLast(mc->infls)->word, DWORDLEN);
  /* le->infls are stored in canonical order (e.g., singular, plural)
   * mc->infls are stored in reverse canonicalorder.
   */
  if (lang == F_FRENCH) {
    sugg = StreamSuggFrenchInfl;
  } else {
    sugg = StreamSuggEnglishInfl;
  }
  if (pos == F_NOUN && lang != F_ENGLISH) {
    if (gender == F_NULL) {
      gender = unique_gender;
      if (gender != F_NULL && mascfem) {
        Dbg(DBGGEN, DBGBAD, "§ inflection not found for <%s>.<%s>",
            word, known_features);
      }
    } else {
#ifdef maxchecking
      if (mascfem) {
        Dbg(DBGGEN, DBGBAD, "AnaMorphInflectWord4: <%s>.<%s> ?!",
            word, known_features);
      }
#endif
    }
    if (gender != F_NULL) {
      lefeatures[0] = gender;
      lefeatures[1] = pos;
      lefeatures[2] = lang;
      lefeatures[3] = TERM;
      p = &lefeatures[3];
    } else {
      lefeatures[0] = pos;
      lefeatures[1] = lang;
      lefeatures[2] = TERM;
      gender = F_NULL;
      p = &lefeatures[2];
    }
  } else {
    lefeatures[0] = pos;
    lefeatures[1] = lang;
    lefeatures[2] = TERM;
    gender = F_NULL;
    p = &lefeatures[2];
  }
  FeatureCopyIn(known_features, FT_LE_MINUS, p);
  newinfl = NULL;
  StreamPrintf(sugg, "%s.%s/; %s+%s.%s <- %s.%s.%s %d\n",
               leword, lefeatures, stem, suffix, known_features,
               mc->example->srcphrase, mc->example->features, features,
               mc->count);
  for (i = 0, infl = mc->infls; infl; i++, infl = infl->next) {
    StringCpy(inflword, stem, DWORDLEN);
    StringCat(inflword, infl->word, DWORDLEN);
    if (gender == F_NULL) {
      StringCpy(inflfeat, infl->features, FEATLEN);
    } else {
      StringCopyExcept(infl->features, gender, FEATLEN, inflfeat);
    }
    StreamPrintf(sugg, "%s.%s/", inflword, inflfeat);
    if (i > 4) {
      StreamPutc(NEWLINE, sugg);
      i = -1;
    }
    StringCat(inflfeat, lefeatures, FEATLEN);
    newinfl = WordCreate(inflword, inflfeat, newinfl, ht);
  }
  StreamPuts("\n|\n", sugg);
  le = LexEntryCreate(leword, lefeatures, ht);
  le->infl = newinfl;
  IndexEntryIndex(le, ht);
  return(le);
}

LexEntry *AnaMorphInflectWord1(char *word, int lang, int pos, int gender,
                               char *known_features, int stemlen,
                               AnaMorphClassList *mcs, char *suffix,
                               int mascfem, HashTable *ht)
{
  int			maxcount, unique_gender, maxunique_gender;
  char			stem[DWORDLEN], *maxfeat;
  Word			*infl;
  AnaMorphClass		*maxmc;
  AnaMorphClassList	*mcs1;
  maxmc = NULL;
  maxcount = INTNEGINF;
  maxfeat = NULL;
  maxunique_gender = F_NULL;
  for (mcs1 = mcs; mcs1; mcs1 = mcs1->next) {
    if (!FeatureMatch(lang, mcs1->mc->lang)) continue;
    if (!FeatureMatch(pos, mcs1->mc->pos)) continue;
    if (!FeatureMatch(gender, mcs1->mc->gender)) continue;
    if (lang == F_FRENCH && pos == F_NOUN) {
      unique_gender = WordGetUniqueGender(mcs1->mc->infls);
      if (mascfem != (F_NULL == unique_gender)) {
        continue;
      }
    } else unique_gender = F_NULL;
    for (infl = mcs1->mc->infls; infl; infl = infl->next) {
      if (streq(infl->word, suffix) &&
          FeatureCompatInflect(infl->features, known_features)) {
        if (mcs1->mc->count > maxcount) {
        /* todoSCORE: Heuristic: largest count locks out other possibilities.
         * With same count, arbitrary inflection is picked.
         */
          maxcount = mcs1->mc->count;
          maxmc = mcs1->mc;
          maxfeat = infl->features;
          maxunique_gender = unique_gender;
        }
      }
    }
  }
  if (maxmc) {
    memcpy(stem, word, stemlen);
    stem[stemlen] = TERM;
    return(AnaMorphInflectWord2(word, lang, pos, gender, known_features, stem,
                                suffix, maxmc, maxfeat, maxunique_gender,
                                mascfem, ht));
  }
  return(NULL);
}

LexEntry *AnaMorphInflectWord(char *word, int lang, int pos,
                              char *known_features, HashTable *ht)
{
  char			suffix[DWORDLEN];
  int			len, stemlen, mascfem, gender;
  AnaMorphClassList	*mcs;
  LexEntry		*le;
  len = strlen(word);
  mascfem = StringIn(F_MASC_FEM, known_features);
  gender = FeatureGet(known_features, FT_GENDER);
  for (stemlen = 0; stemlen < len; stemlen++) {
    memcpy(suffix, word+stemlen, len-stemlen);
    suffix[len-stemlen] = TERM;
    if (!(mcs = AnaMorphClassListGet(suffix))) continue;
    if ((le = AnaMorphInflectWord1(word, lang, pos, gender, known_features,
                                   stemlen, mcs, suffix, mascfem, ht))) {
    /* todoSCORE: Heuristic: longest suffix locks out other possibilities. */
      return(le);
    }
  }
  return(NULL);
}

/* TESTING */

void AnaMorphTool()
{
  char		word[DWORDLEN], features[FEATLEN];
  printf("Welcome to the morphology tool.\n");
  while (1) {
    if (!StreamAskStd("Enter word: ", DWORDLEN, word)) return;
    if (!StreamAskStd("Enter language and other features: ", FEATLEN,
                      features)) {
      return;
    }
    if (StringIn(F_FRENCH, features)) {
      LexEntryInflectWord(word, F_FRENCH, features, FrenchIndex);
    } else if (StringIn(F_ENGLISH, features)) {
      LexEntryInflectWord(word, F_ENGLISH, features, EnglishIndex);
    }
  }
}

/* End of file */
