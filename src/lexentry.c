/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Lexical entries (words and phrases).
 *
 * 19931225: begun
 * 19931226: basic lexicon parsing
 * 19940114: added indexing of phrases by all inflections
 * 19940609: new phrase/word indexing begun
 * 19940610: worked on automatic word inflecting and phrase inflecting
 * 19950428: new phrasal verb parsing mechanism
 * 19981122T092001: STATS
 *
 * todo:
 * - How to handle plurals of English abbreviations? "'s" left in inflection
 *   files for now.
 */

#include "tt.h"
#include "lexanam.h"
#include "lexentry.h"
#include "lexmorph.h"
#include "lexobjle.h"
#include "lextheta.h"
#include "lexwf2.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptext.h"
#include "reptime.h"
#include "semdisc.h"
#include "taname.h"
#include "toolcorp.h"
#include "toolrpt.h"
#include "uaquest.h"
#include "utilchar.h"
#include "utildbg.h"
#include "utillrn.h"

/*
#define STATS
 */

/* Don't forget to turn on French when running statistics. */

#ifdef STATS
int StatsLePhraseFeat, StatsLeWordFeat;
int StatsInflSrcPhraseFeat, StatsInflFeat;
#endif

void LexEntryStatsBegin()
{
#ifdef STATS
  StatsLePhraseFeat = 0L;
  StatsLeWordFeat = 0L;
  StatsInflSrcPhraseFeat = 0L;
  StatsInflFeat = 0L;
#endif
}

void LexEntryStatsEnd()
{
#ifdef STATS
  Dbg(DBGLEX, DBGOK, "LEXICAL:FEATURE:CITATIONFORM:ENTIREPHRASE %ld",
      StatsLePhraseFeat);
  Dbg(DBGLEX, DBGOK, "LEXICAL:FEATURE:CITATIONFORM:WORDSINPHRASE %ld",
      StatsLeWordFeat);
  Dbg(DBGLEX, DBGOK, "LEXICAL:FEATURE:INFLECTION:CITATIONFORM %ld",
      StatsInflSrcPhraseFeat);
  Dbg(DBGLEX, DBGOK, "LEXICAL:FEATURE:INFLECTION:INFLECTION %ld",
      StatsInflFeat);
  Dbg(DBGLEX, DBGOK, "LEXICAL:FEATURE:CITATIONFORM %ld",
      StatsLePhraseFeat + StatsLeWordFeat);
  Dbg(DBGLEX, DBGOK, "LEXICAL:FEATURE:INFLECTION %ld",
      StatsInflSrcPhraseFeat + StatsInflFeat);
#endif
}

/* Feature */

int FeatureGet(register char *features, register char *ft)
{
  register char	*p, c;
  while ((c = *features++)) {
    for (p = ft; *p;) if (c == *p++) return((unsigned char)c);
  }
  return(F_NULL);
}

int FeatureGetDefault(char *features, char *ft, int def)
{
  char	*p;
  char	c;
  while ((c = *features++)) {
    for (p = ft; *p;) if (c == *p++) return((uc)c);
  }
  return(def);
}

int FeatureGetRequired(char *errstring, char *features, char *ft)
{
  while (*features) {
    if (StringIn(*((uc *)features), ft)) return((uc)*features);
    features++;
  }
  Dbg(DBGLEX, DBGBAD,
      "feature type <%s> missing from <%s> <%s> -- assumed <%c>",
      ft, errstring, features, *((uc *)ft));
  return(*((uc *)ft));
}

Bool FeatureMatch(int f1, int f2)
{
  return(f1 == F_NULL || f2 == F_NULL || f1 == f2);
}

Bool FeatureDialectMatch(int desired, int candidate)
{
  return(desired == candidate || candidate == F_NULL);
}

Bool FeatureMatch1(int f1, int f2)
{
  return(f2 == F_NULL || f1 == f2);
}

Bool FeatureFtMatch(char *f1, char *f2, char *ft)
{
  return(FeatureMatch(FeatureGet(f1, ft), FeatureGet(f2, ft)));
}

Bool FeatureFtEqual(char *f1, char *f2, char *ft)
{
  return(FeatureGet(f1, ft) == FeatureGet(f2, ft));
}

Bool FeatureCompatInflect(char *f1, char *f2)
{
  return(FeatureFtMatch(f1, f2, FT_POS) &&
         FeatureFtMatch(f1, f2, FT_TENSE) &&
         FeatureFtMatch(f1, f2, FT_GENDER) &&
         FeatureFtMatch(f1, f2, FT_NUMBER) &&
         FeatureFtMatch(f1, f2, FT_PERSON) &&
         FeatureFtMatch(f1, f2, FT_MOOD));
}

Bool FeatureCompatLexEntry(char *f1, char *f2)
{
  return(FeatureFtEqual(f1, f2, FT_POS) &&
         FeatureFtEqual(f1, f2, FT_GENDER));
}

Bool FeatureDefaultTo(char *features, int maxlen, char *ft, unsigned int def)
{
  int	actual;
  if ((actual = FeatureGet(features, ft)) == F_NULL) {
    actual = def;
    StringAppendChar(features, maxlen, def);
  }
  return(actual);
}

int FeatureFlipLanguage(int lang)
{
  int newlang;
  if (lang == F_FRENCH) newlang = F_ENGLISH;
  else newlang = F_FRENCH;
  if (StringIn(newlang, StdDiscourse->langs)) return(newlang);
  return(F_NULL);
}

void FeatureCopyIn(char *in, char *set, char *out)
{
  while (*set) {
    if (StringIn(*set, in)) {
      *out = *set;
      out++;
    }
    set++;
  }
  *out = TERM;
}

void FeatureCheck(char *features, char *set, char *context)
{
  char *origfeat;
  origfeat = features;
  while (*features) {
    if (!StringIn(*features, set)) {
      Dbg(DBGLEX, DBGBAD, "<%c> in <%s> unexpected in %s", *features,
          origfeat, context);
    }
    features++;
  }
}

Obj *FeatToCon1(int feature)
{
  char	name[WORDLEN];
  sprintf(name, "F%d", CharsetISO_8859_1ToMac(feature));
  /* todo: Replace F with relations mac-code-of and iso-code-of.
   * Change object names to more intuitive names and fix references
   * to those F objects in code and db. Cache a table to make these
   * functions (called in parsing) fast.
   */
  return(NameToObj(name, OBJ_NO_CREATE));
}

Obj *FeatToCon(int feature)
{
  int	macfeat;
  Obj	*obj;
  if (!(obj = FeatToCon1(feature))) {
    macfeat = CharsetISO_8859_1ToMac(feature);
    Dbg(DBGLEX, DBGBAD,
"feature <%c> ISO %d (0x%x) MAC F%d (0x%x) missing from Linguistics file",
        (char)feature, feature, feature, macfeat, macfeat);
    return(NULL);
  }
  return(obj);
}

int ConToFeat(Obj *obj)
{
  char	*name;
  name = M(obj);
  if (name[0] != 'F') {
    Dbg(DBGLEX, DBGBAD, "cannot convert concept <%s> to feature", name);
    Stop();
    return(F_NULL);
  }
  return(CharsetMacToISO_8859_1(atoi(name+1))); /* todo */
}

void FeatPrintUnused(FILE *stream)
{
  int	i;
  for (i = '!'; i < 256; i++) {
    if (NULL == FeatToCon1(i)) fputc(i, stream);
  }
  fputc(NEWLINE, stream);
}

void FeaturesForDbFile(char *feat)
{
  StringElimChar(feat, F_NOUN);
  StringElimChar(feat, F_SINGULAR);
}

/* This returns junk usable only for sorting. */
void FeatCanonCmp(char *in, char *out)
{
  char	*p;
  out[0] = '0';
  out[1] = '0';
  out[2] = '0';
  out[3] = '0';
  out[4] = '0';
  p = out + 5;
  while (*in) {
    if (StringIn(*in, FT_MOOD)) {
      out[0] = '0'+StringFirstLoc(*in, FT_MOOD);
    } else if (StringIn(*in, FT_TENSE)) {
      out[1] = '0'+StringFirstLoc(*in, FT_TENSE);
    } else if (StringIn(*in, FT_GENDER)) {
      out[2] = '0'+StringFirstLoc(*in, FT_GENDER);
    } else if (StringIn(*in, FT_NUMBER)) {
      out[3] = '0'+StringFirstLoc(*in, FT_NUMBER);
    } else if (StringIn(*in, FT_PERSON)) {
      out[4] = '0'+StringFirstLoc(*in, FT_PERSON);
    } else {
      *p = *in;
      p++;
    } 
    in++;
  }
  *p = TERM;
}

/* This returns usable features. */
void FeatCanon(char *in, char *out)
{
  char	*p;
  for (p = FT_ALL; *p; p++) {
    if (StringIn(*(uc *)p, in)) {
      *out = *p;
      out++;
    }
  }
  *out = TERM;
}

int FeatCmp(char *f1, char *f2)
{
  char	ff1[FEATLEN], ff2[FEATLEN];
  FeatCanonCmp(f1, ff1);
  FeatCanonCmp(f2, ff2);
  return(strcmp(ff1, ff2));
}

/* Word */

Word *WordCreate(char *srcphrase, char *features, Word *rest, HashTable *ht)
{
  Word *word;
  word = CREATE(Word);
  word->word = HashTableIntern(ht, srcphrase);
  word->features = HashTableIntern(ht, features);
  word->next = rest;
  return(word);
}

void WordFreeList(Word *words)
{
  Word *n;
  while (words) {
    n = words->next;
    MemFree(words, "Word");
    words = n;
  }
}

Word *WordLast(Word *words)
{
  while (words->next) words = words->next;
  return(words);
}

Word *WordInsertCanonOrder(Word *words, Word *word)
{
  Word	*w, *prev;
  prev = NULL;
  for (w = words; w; w = w->next) {
    if (FeatCmp(w->features, word->features) >= 0) break;
    prev = w;
  }
  if (prev) {
    word->next = prev->next;
    prev->next = word;
    return(words);
  } else {
    word->next = words;
    return(word);
  }
}

int WordGetUniqueGender(Word *infl)
{
  int	gender, igender;
  gender = F_NULL;
  for (; infl; infl = infl->next) {
    if (F_NULL != (igender = FeatureGet(infl->features, FT_GENDER))) {
      if (gender != F_NULL && gender != igender) return(F_NULL);
      gender = igender;
    }
  }
  return(gender);
}

Bool WordListEqual(Word *infl1, Word *infl2)
{
  for (; infl1; infl1 = infl1->next, infl2 = infl2->next) {
    if (!infl2) return(0);
    if ((!streq(infl1->word, infl2->word)) ||
        (!streq(infl1->features, infl2->features))) return(0);
  }
  if (infl2) return(0);
  return(1);
}

void WordPrint(FILE *stream, Word *word)
{
  fprintf(stream, "%s.%s/", word->word, word->features);
}

void WordPrintAll(FILE *stream, Word *word)
{
  int	i;
  for (i = 0; word; i++, word = word->next) {
    WordPrint(stream, word);
    if (i > 6) {
      fputc(NEWLINE, stream);
      i = -1;
    }
  }
  fputc(NEWLINE, stream);
}

void WordPrintText(Text *text, Word *word)
{
  TextPuts(word->word, text);
  TextPutc('.', text);
  TextPuts(word->features, text);
  TextPutc('/', text);
}

void WordPrintTextAll(Text *text, Word *word)
{
  int	i;
  for (i = 0; word; i++, word = word->next) {
    WordPrintText(text, word);
    if (i > 6) {
      TextPutc(NEWLINE, text);
      i = -1;
    }
  }
}

/* IndexEntrySpell */

IndexEntrySpell *IndexEntrySpellCreate(LexEntry *lexentry, char *key,
                                       char *features, IndexEntrySpell *next)
{
  IndexEntrySpell	*ie;
  ie = CREATE(IndexEntrySpell);
  ie->lexentry = lexentry;
  ie->word = key;
  ie->features = features;
  ie->next = next;
  return(ie);
}

void IndexEntrySpellIndexInfl(char *key0, LexEntry *lexentry, char *features,
                              HashTable *ht)
{
  IndexEntrySpell	*previe, *ie;
  char	key1[PHRASELEN], *key;
  StringCpy(key1, key0, PHRASELEN);
  StringReduceTotal(key1);
  key = HashTableIntern(ht, key1);
  previe = (IndexEntrySpell *)HashTableGet(ht, key);
  ie = IndexEntrySpellCreate(lexentry, key0, features, previe);
  HashTableSet(ht, key, ie);
}

/* todo: The case where partial=total could be optimized. */
IndexEntry *LexEntryFindPhraseRelaxed2(char *src, char *tot_red_src,
                                       int srclang, int lang,
                                       void (*redfn)(char *), int level,
                                       int maxlevel)
{
  IndexEntry		*ie;
  IndexEntrySpell	*ies, *p;
  char			part_red_src[PHRASELEN], part_red_tgt[PHRASELEN];
  ie = NULL;
  if (level > maxlevel) return(NULL);
  StringCpy(part_red_src, src, PHRASELEN);
  redfn(part_red_src);
  if (!(ies = (IndexEntrySpell *)HashTableGet(SpellIndex, tot_red_src))) {
    return(NULL);
  }
  for (p = ies; p; p = p->next) {
    if (lang != FeatureGet(p->lexentry->features, FT_LANG)) continue;
    StringCpy(part_red_tgt, p->word, PHRASELEN);
    redfn(part_red_tgt);
    if (streq(part_red_src, part_red_tgt)) {
      if (level > 1) {
        Dbg(DBGGEN, DBGDETAIL, "spelling <%s.%c> ¦¦> <%s.%s>", src,
            (char)srclang, p->word, p->features);
      }
      ie = IndexEntryCreate(ies->lexentry, ies->features, ie);
    }
  }
  return(ie);
}

IndexEntry *LexEntryFindPhraseRelaxed1(char *src, int srclang, int lang,
                                       int maxlevel, int leveloffset)
{
  char		tot_red_src[PHRASELEN];
  IndexEntry	*ie;
  StringCpy(tot_red_src, src, PHRASELEN);
  StringReduceTotal(tot_red_src);
  if ((ie = LexEntryFindPhraseRelaxed2(src, tot_red_src, srclang, lang,
                                       StringReduce1, leveloffset+1,
                                       maxlevel))) {
    return(ie);
  }
  if ((ie = LexEntryFindPhraseRelaxed2(src, tot_red_src, srclang, lang,
                                       StringReduce2, leveloffset+2,
                                       maxlevel))) {
    return(ie);
  }
  if ((ie = LexEntryFindPhraseRelaxed2(src, tot_red_src, srclang, lang,
                                       StringReduce3, leveloffset+3,
                                       maxlevel))) {
    return(ie);
  }
  if ((ie = LexEntryFindPhraseRelaxed2(src, tot_red_src, srclang, lang,
                                       StringReduce4, leveloffset+4,
                                       maxlevel))) {
    return(ie);
  }
  if ((ie = LexEntryFindPhraseRelaxed2(src, tot_red_src, srclang, lang,
                                       StringReduce5, leveloffset+5,
                                       maxlevel))) {
    return(ie);
  }
  if ((ie = LexEntryFindPhraseRelaxed2(src, tot_red_src, srclang, lang,
                                       StringReduce6, leveloffset+6,
                                       maxlevel))) {
    return(ie);
  }
  return(NULL);
}

IndexEntry *LexEntryFindPhraseRelaxed(HashTable *ht, char *src, int lang,
                                      int maxlevel, /* RESULTS */ int *freeme)
{
  char			phrase1[PHRASELEN];
  IndexEntry	*ie;
  if (SpellIndex == NULL) {
  /* SpellIndex is disabled to enable faster debugging;
   * this code enables at least some common forms of relaxation.
   * todoSCORE
   */
    if (maxlevel < 1) return(NULL);
    if (StringIsAllUpper(src)) {
      StringCpy(phrase1, src, PHRASELEN);
      StringToLowerDestructive(phrase1+1);
      if ((ie = IndexEntryGet(phrase1, ht))) {
        *freeme = 0;
        return(ie);
      }
      phrase1[0] = CharToLower(*((uc *)phrase1));
      *freeme = 0;
      return(IndexEntryGet(phrase1, ht));
    }
    if (CharIsUpper(*((uc *)src))) {
      StringCpy(phrase1, src, PHRASELEN);
      phrase1[0] = CharToLower(*((uc *)phrase1));
      *freeme = 0;
      return(IndexEntryGet(phrase1, ht));
    }
    return(NULL);
  }
  if (freeme == NULL) return(NULL);
  if ((ie = LexEntryFindPhraseRelaxed1(src, lang, lang, maxlevel, 0))) {
    *freeme = 1;
    return(ie);
  }
  if ((ie = LexEntryFindPhraseRelaxed1(src, lang, FeatureFlipLanguage(lang),
                                       maxlevel, 6))) {
    *freeme = 1;
    return(ie);
  }
  return(NULL);
}

#ifdef notdef
Bool IndexEntryNoConcepts(IndexEntry *ie)
{
  for (; ie; ie = ie->next) {
    if (ie->lexentry && ie->lexentry->leo) return(0);
  }
  return(1);
}
#endif

/* If returned *freeme = 1, caller must free result with IndexEntryFree.
 * If caller does not want to free, it should pass in NULL <freeme>
 * (only when maxlevel == 0).
 */
IndexEntry *LexEntryFindPhrase1(HashTable *ht, char *phrase, int maxlevel,
                                int derive_ok, int derive_depth,
                                /* RESULTS */ int *freeme)
{
  IndexEntry	*ie, *ie1;
  char		phrase1[PHRASELEN];
  if (freeme) *freeme = 0;
  /* todo: Given phrase = "ford", lowercase LexEntry "ford" locks out
   * uppercase LexEntry "Ford". Return all possibilities. todoSCORE.
   */
  if (CharIsUpper(*((uc *)phrase))) {
    ie = IndexEntryGet(phrase, ht);
    /* In "J'étais", the "J" must not lock out the "j'". Also "Le voyage",
     * "Vendredi".
     */
    StringCpy(phrase1, phrase, PHRASELEN);
    phrase1[0] = CharToLower(*((uc *)phrase1));
    if ((ie1 = IndexEntryGet(phrase1, ht))) {
      if (ie) {
        ie = IndexEntryAppendDestructive(IndexEntryCopy(ie),
                                         IndexEntryCopy(ie1));
        if (freeme == NULL) return(NULL);
        *freeme = 1;
      } else {
        ie = ie1;
        if (freeme) *freeme = 0;
      }
      return(ie);
    } else if (ie) {
      if (freeme) *freeme = 0;
      return(ie);
    }
  } else if ((ie = IndexEntryGet(phrase, ht))) {
  /* && !IndexEntryNoConcepts(ie) -- causes lots of junk */
    if (freeme) *freeme = 0;
    return(ie);
  }
  if (derive_ok && (!IsPhrase(phrase)) &&
      (!StringIsNumber(phrase)) &&
      (!StringIn('.', phrase)) && /* todo: end of sentence problems */
      (ie = Lex_WordForm2Derive(ht, phrase, maxlevel, derive_depth))) {
    if (freeme == NULL) return(NULL);
    *freeme = 1;
    return(ie);
  }
  return(LexEntryFindPhraseRelaxed(ht, phrase, LexEntryHtLang(ht), maxlevel,
                                   freeme));
}

IndexEntry *LexEntryFindPhrase(HashTable *ht, char *phrase, int maxlevel,
                               int derive_ok, int nofail,
                               /* RESULTS */ int *freeme)
{
  IndexEntry *ie;
  if ((ie = LexEntryFindPhrase1(ht, phrase, maxlevel, derive_ok, 0, freeme))) {
    return ie;
  }
  if (nofail && StringLastChar(phrase) != '.') {
    /* The word has been derived. */
/*
    sprintf(feat, "%c%c", F_NOUN, LexEntryHtLang(ht));
    le = LexEntryInflectWord1(phrase, LexEntryHtLang(ht), F_NOUN, feat, ht);
    Dbg(DBGLEX, DBGOK, "NEW WORD <%s.%s>", le->srcphrase, le->features);

    sprintf(feat, "%c%c", F_VERB, LexEntryHtLang(ht));
    le = LexEntryInflectWord1(phrase, LexEntryHtLang(ht), F_VERB, feat, ht);
    Dbg(DBGLEX, DBGOK, "NEW WORD <%s.%s>", le->srcphrase, le->features);

    sprintf(feat, "%c%c", F_ADJECTIVE, LexEntryHtLang(ht));
    le = LexEntryInflectWord1(phrase, LexEntryHtLang(ht), F_ADJECTIVE, feat,ht);
    Dbg(DBGLEX, DBGOK, "NEW WORD <%s.%s>", le->srcphrase, le->features);

    sprintf(feat, "%c%c", F_ADVERB, LexEntryHtLang(ht));
    le = LexEntryInflectWord1(phrase, LexEntryHtLang(ht), F_ADVERB, feat, ht);
    Dbg(DBGLEX, DBGOK, "NEW WORD <%s.%s>", le->srcphrase, le->features);
 */
    ie = IndexEntryGet(phrase, ht);
    if (freeme) *freeme = 0;
    return ie;
  }
  return NULL;
}

/* IndexEntry */

IndexEntry *IndexEntryCreate(LexEntry *lexentry, char *features,
                             IndexEntry *next)
{
  IndexEntry	*ie;
  ie = CREATE(IndexEntry);
  ie->lexentry = lexentry;
  ie->features = features;
  ie->next = next;
  return(ie);
}

/* Appends each element of ie2 destructively onto ie1 and returns the new ie1.
 */
IndexEntry *IndexEntryAppendDestructive(IndexEntry *ie1, IndexEntry *ie2)
{
  while (ie2) {
    ie1 = IndexEntryCreate(ie2->lexentry, ie2->features, ie1);
    ie2 = ie2->next;
  }
  return(ie1);
}

IndexEntry *IndexEntryCopy(IndexEntry *ie)
{
  return(IndexEntryAppendDestructive(NULL, ie));
}

void IndexEntryFree(IndexEntry *ie)
{
  IndexEntry *n;
  Dbg(DBGOBJ, DBGHYPER, "IndexEntryFree", E);
  while (ie) {
    n = ie->next;
    MemFree(ie, "IndexEntry");
    ie = n;
  }
}

IndexEntry *IndexEntryGet(char *srcphrase, HashTable *ht)
{
  return((IndexEntry *)HashTableGet(ht, srcphrase));
}

/* Return value for the curious only. */
IndexEntry *IndexEntryIndexInfl(char *key, LexEntry *lexentry, char *features,
                                HashTable *ht)
{
  IndexEntry *previe, *ie;
  if (!SaveTime) {
    IndexEntrySpellIndexInfl(key, lexentry, features, SpellIndex);
  }
  previe = (IndexEntry *)HashTableGet(ht, key);
  for (ie = previe; ie; ie = ie->next) {
    if (lexentry == ie->lexentry && streq(features, ie->features)) {
      Dbg(DBGLEX, DBGBAD, "Duplicate <%s> not indexed", key);
      return(NULL);
    }
  }
  Dbg(DBGLEX, DBGHYPER, "Indexing <%s>\n", key);
  ie = IndexEntryCreate(lexentry, features, previe);
  HashTableSet(ht, key, ie);
  return(ie);
}

/* Return value for the curious only. */
IndexEntry *IndexEntryIndex(LexEntry *le, HashTable *ht)
{
  Word			*infl;
  IndexEntry	*ie;
  for (infl = le->infl; infl; infl = infl->next) {
    ie = IndexEntryIndexInfl(infl->word, le, infl->features, ht);
  }
#ifdef notdef
  LexEntryCheckSrcphrase(le);
#endif
  return(ie);
}

/* LexEntry */

int		MaxWordsInPhrase;
HashTable	*FrenchIndex;
HashTable	*EnglishIndex;
HashTable	*SpellIndex;
LexEntry	*AllLexEntries;
Bool		LexEntryOff, LexEntryInsideName;

void LexEntryInit()
{
  MaxWordsInPhrase = 0;
  AllLexEntries = NULL;
  LexEntryInsideName = 0;
  FrenchIndex = HashTableCreate(10037L);
  EnglishIndex = HashTableCreate(10037L);
  if (!SaveTime) SpellIndex = HashTableCreate(10037L);
  else SpellIndex = NULL;
}

/* Invariants:
 * 1) <le->srcphrase, gender, pos, language> is a unique lexentry identifier.
 * 2) le->srcphrase must be equal to one of its inflections, so it can
 *    be found (because srcphrases aren't hashed anywhere).
 */

void LexEntryCheckSrcphrase(LexEntry *le)
{
  Word			*infl;
  for (infl = le->infl; infl; infl = infl->next) {
    if (streq(infl->word, le->srcphrase)) return;
  }
  Dbg(DBGLEX, DBGBAD, "no srcphrase inflection for <%s>.<%s>",
      le->srcphrase, le->features);
}

LexEntry *LexEntryGetSrcphrase(char *srcphrase, int gender, int pos,
                               HashTable *ht)
{
  IndexEntry	*ie;
  for (ie = IndexEntryGet(srcphrase, ht); ie; ie = ie->next) {
    if (streq(srcphrase, ie->lexentry->srcphrase) &&
        pos == FeatureGet(ie->lexentry->features, FT_POS) &&
        gender == FeatureGet(ie->lexentry->features, FT_GENDER)) {
      return(ie->lexentry);
    }
  }
  return(NULL);
}

LexEntryToObj *LexEntryGetLeo(LexEntry *le)
{
  return(le->leo);
#ifdef notdef
  /* Old training-based word formation: */
  if (le->leo) return(le->leo);
  Dbg(DBGLEX, DBGBAD, "no meaning for <%s>.<%s>, attempting to derive",
      le->srcphrase, le->features);
  if (WordFormUnderstand(le)) {
    return(le->leo);
  } else {
    Dbg(DBGLEX, DBGBAD, "unable to derive");
    return(NULL);
  }
#endif
}

ObjList *LexEntryGetLeoObjs(LexEntry *le)
{
  ObjList		*r;
  LexEntryToObj	*leo;
  r = NULL;
  for (leo = le->leo; leo; leo = leo->next) {
    r = ObjListCreate(leo->obj, r);
  }
  return(r);
}

/* For database checking. */
void LexEntryCheck2(char *srcphrase, char *features, int pos, int really,
                    HashTable *ht, int checkpos)
{
  char		posfeat[FEATLEN];
  int		oldpos;
  IndexEntry *ie;
  if (StringIn(F_EXPLETIVE, features)) return;
  posfeat[0] = pos;
  posfeat[1] = TERM;
  if (pos != checkpos) {
    for (ie = IndexEntryGet(srcphrase, ht); ie; ie = ie->next) {
      if (really && StringIn(F_REALLY, ie->lexentry->features)) continue;
      oldpos = FeatureGet(ie->lexentry->features, FT_POS);
      if (checkpos == oldpos && pos != oldpos) {
        Dbg(DBGLEX, DBGBAD, "<%s>.<%s> similar to <%s>.<%s>",
            srcphrase, features, ie->lexentry->srcphrase,
            ie->lexentry->features);
      }
    }
  }
}

/* For database checking. */
void LexEntryCheck1(char *srcphrase, char *features, int pos, int really,
                    HashTable *ht)
{
  char		posfeat[FEATLEN];
  IndexEntry	*ie;
  if (StringIn(F_EXPLETIVE, features)) return;
  posfeat[0] = pos;
  posfeat[1] = TERM;
  for (ie = IndexEntryGet(srcphrase, ht); ie; ie = ie->next) {
    if (FeatureMatch(pos, FeatureGet(ie->lexentry->features, FT_POS))) {
      if (really && StringIn(F_REALLY, ie->lexentry->features)) continue;
      if (StringIn(F_BORROWING, ie->lexentry->features)) {
      /* For now, don't attempt proper inflection of, say, Italian adjectives.
       */
        continue;
      }
      Dbg(DBGLEX, LexEntryInsideName ? DBGDETAIL : DBGBAD,
          "<%s>.<%s> is similar to existing <%s>.<%s>",
          srcphrase, features, ie->lexentry->srcphrase, ie->lexentry->features);
    }
  }
}

LexEntry *LexEntryCreate(char *srcphrase, char *features, HashTable *ht)
{
  LexEntry	*lexentry;
#ifdef maxchecking
  int		pos, really;
  pos = FeatureGetRequired("lexentry features", features, FT_POS);
  really = StringIn(F_REALLY, features);
  LexEntryCheck1(srcphrase, features, pos, really, ht);
  LexEntryCheck2(srcphrase, features, pos, really, ht, F_ADVERB);
  LexEntryCheck2(srcphrase, features, pos, really, ht, F_CONJUNCTION);
  LexEntryCheck2(srcphrase, features, pos, really, ht, F_DETERMINER);
  LexEntryCheck2(srcphrase, features, pos, really, ht, F_PREPOSITION);
  LexEntryCheck2(srcphrase, features, pos, really, ht, F_PRONOUN);
#endif
#ifdef notdef
  if (LexEntryGetSrcphrase(srcphrase, FeatureGet(features, FT_GENDER), pos,
                           ht)) {
    Dbg(DBGLEX, DBGBAD, "nonunique srcphrase <%s>.<%s>", srcphrase, features);
    /* In this case other problems may result; therefore, these must be fixed
     * ASAP.
     */
  }
#endif
  lexentry = CREATE(LexEntry);
  lexentry->srcphrase = HashTableIntern(ht, srcphrase);
  lexentry->features = HashTableIntern(ht, features);
  lexentry->infl = NULL;
  lexentry->phrase_seps = NULL;
  lexentry->leo = NULL;
  lexentry->next = AllLexEntries;
  AllLexEntries = lexentry;
  return(lexentry);
}

void LexEntryAddInfl(LexEntry *le, Word *word)
{
  if (AnaMorphOn) {
    /* AnaMorph requires sorted inflections. Otherwise it is nice but takes
     * longer.
     */
    le->infl = WordInsertCanonOrder(le->infl, word);
  } else {
    word->next = le->infl;
    le->infl = word;
  }
}

void LexEntryCreateNumber(char *number)
{
  LexEntry	*le;
  char		plural[WORDLEN];
  if (StringIn(F_FRENCH, StdDiscourse->langs)) {
    le = LexEntryCreate(number, "Ny", FrenchIndex);
    LexEntryAddInfl(le, WordCreate(number, "Ny", NULL, FrenchIndex));
    IndexEntryIndex(le, FrenchIndex);
    le = LexEntryCreate(number, "Ay", FrenchIndex);
    LexEntryAddInfl(le, WordCreate(number, "Ay", NULL, FrenchIndex));
    IndexEntryIndex(le, FrenchIndex);
  }
  if (StringIn(F_ENGLISH, StdDiscourse->langs)) {
    StringCpy(plural, number, WORDLEN);
    StringAppendChar(plural, WORDLEN, 's');
    le = LexEntryCreate(number, "Nz", EnglishIndex);
    LexEntryAddInfl(le, WordCreate(number, "SNz", NULL, EnglishIndex));
    LexEntryAddInfl(le, WordCreate(plural, "PNz", NULL, EnglishIndex));
    IndexEntryIndex(le, EnglishIndex);
    le = LexEntryCreate(number, "Az", EnglishIndex);
    LexEntryAddInfl(le, WordCreate(number, "Az", NULL, EnglishIndex));
    IndexEntryIndex(le, EnglishIndex);
  }
}

LexEntry *LexEntryFind(char *srcphrase, char *features, HashTable *ht)
{
  IndexEntry	*ie;
  for (ie = IndexEntryGet(srcphrase, ht); ie; ie = ie->next) {
    if (FeatureCompatLexEntry(ie->lexentry->features, features))
      return(ie->lexentry);
  }
  return(NULL);
}

HashTable *LexEntryLangHt(char *features)
{
  if (F_FRENCH == FeatureGet(features, FT_LANG)) return(FrenchIndex);
  else return(EnglishIndex);
}

HashTable *LexEntryLangHt1(int lang)
{
  if (F_FRENCH == lang) return(FrenchIndex);
  else return(EnglishIndex);
}

int LexEntryHtLang(HashTable *ht)
{
  if (FrenchIndex == ht) return(F_FRENCH);
  else return(F_ENGLISH);
}

HashTable *LexEntryHtFlip(HashTable *ht)
{
  if (EnglishIndex == ht) return(FrenchIndex);
  else return(EnglishIndex);
}

LexEntry *LexEntryFindInfl1(char *word, char *features, HashTable *ht)
{
#ifdef notdef
  LexEntry	*r;
  IndexEntry	*ie;
  r = NULL;
  for (ie = IndexEntryGet(word, ht); ie; ie = ie->next) {
    if (r == ie->lexentry) continue;
    if (FeatureCompatInflect(ie->features, features)) {
      if (r) {
        Dbg(DBGLEX, DBGBAD, "<%s>.<%s> ambiguous", word, features);
        return(r);   
      }
      r = ie->lexentry;
    }
  }
  return(r);
#else
  IndexEntry	*ie;
  for (ie = IndexEntryGet(word, ht); ie; ie = ie->next) {
    if (FeatureCompatInflect(ie->features, features)) {
      return(ie->lexentry);
    }
  }
  return(NULL);
#endif
}

/* This function is used only in reading lexical item (database) words in
 * a phrase. Therefore, initial-letter uppercase lowering is allowed.
 * This is especially intended to cope with determiners in MediaObject
 * names. Otherwise we have to store uppercase versions of lexical items
 * in the inflection file. This seems to lead to the need to them store
 * the uppercase versions in the Linguistics file. Which leads to problems
 * in parsing non-MediaObjects.
 */
LexEntry *LexEntryFindInfl(char *word, char *features, HashTable *ht,
                           int lowering_ok)
{
  int		pos;
  LexEntry	*le;
  char		word1[PHRASELEN];
  if ((le = LexEntryFindInfl1(word, features, ht))) return(le);
  if (StringIsNumber(word)) {
    if ((F_NOUN == (pos = FeatureGetRequired("find", features, FT_POS))) ||
        pos == F_ADJECTIVE) {
      LexEntryCreateNumber(word);
      return(LexEntryFindInfl1(word, features, ht));
    } else Dbg(DBGLEX, DBGBAD, "number <%s> not N or A", word);
  }
  if (lowering_ok && CharIsUpper(*((uc *)word))) {
    StringCpy(word1, word, PHRASELEN);
    word1[0] = CharToLower(*((uc *)word1));
    if ((le = LexEntryFindInfl1(word1, features, ht))) return(le);
  }
  return(NULL);
}

/* For now, always generate inflections in formal style. */
Word *LexEntryGetInflection1(LexEntry *le, int tense, int gender, int number,
                             int person, int mood, int degree, Discourse *dc)
{
  Word	*infl;
  if (DC(dc).dialect != F_NULL) {
    for (infl = le->infl; infl; infl = infl->next) {
      if (F_INFREQUENT != FeatureGet(infl->features, FT_FREQ) &&
          DC(dc).dialect == FeatureGet(infl->features, FT_DIALECT) &&
          F_NULL == FeatureGet(infl->features, FT_ALTER) &&
          FeatureMatch(tense, FeatureGet(infl->features, FT_TENSE)) &&
          FeatureMatch(gender, FeatureGet(infl->features, FT_GENDER)) &&
          FeatureMatch(number, FeatureGet(infl->features, FT_NUMBER)) &&
          FeatureMatch(person, FeatureGet(infl->features, FT_PERSON)) &&
          FeatureMatch(mood, FeatureGetDefault(infl->features, FT_MOOD,
                                               F_INDICATIVE)) &&
          FeatureMatch(degree, FeatureGetDefault(infl->features, FT_DEGREE,
                                                 F_POSITIVE)) &&
          F_NULL == FeatureGet(infl->features, FT_STYLE)) {
        return(infl);
      }
    }
  }
  for (infl = le->infl; infl; infl = infl->next) {
    if (F_INFREQUENT != FeatureGet(infl->features, FT_FREQ) &&
        F_NULL == FeatureGet(infl->features, FT_DIALECT) &&
        F_NULL == FeatureGet(infl->features, FT_ALTER) &&
        FeatureMatch(tense, FeatureGet(infl->features, FT_TENSE)) &&
        FeatureMatch(gender, FeatureGet(infl->features, FT_GENDER)) &&
        FeatureMatch(number, FeatureGet(infl->features, FT_NUMBER)) &&
        FeatureMatch(person, FeatureGet(infl->features, FT_PERSON)) &&
        FeatureMatch(mood, FeatureGetDefault(infl->features, FT_MOOD,
                                             F_INDICATIVE)) &&
        FeatureMatch(degree, FeatureGetDefault(infl->features, FT_DEGREE,
                                               F_POSITIVE)) &&
        F_NULL == FeatureGet(infl->features, FT_STYLE)) {
      return(infl);
    }
  }
  return(NULL);
}

Word *LexEntryGetInflection(LexEntry *le, int tense, int gender, int number,
                            int person, int mood, int degree, int complain,
                            Discourse *dc)
{
  Word *infl;

  if (DC(dc).lang == F_FRENCH && gender == F_NULL) {
    if ((infl = LexEntryGetInflection1(le, tense, F_MASCULINE, number, person,
                                       mood, degree, dc))) {
      return(infl);
    }
  }
  if ((infl = LexEntryGetInflection1(le, tense, gender, number, person, mood,
                                     degree, dc))) {
    return(infl);
  }
  if (FeatureGet(le->features, FT_POS) != F_NOUN) goto failure;
  if (number != F_NULL) {
    /* Try flipping number. */
    if ((infl = LexEntryGetInflection1(le, tense, gender,
                                       number == F_SINGULAR ? F_PLURAL :
                                                              F_SINGULAR,
                                       person, mood, degree,
                                       dc))) {
      return(infl);
    }
  }
  if (gender != F_NULL) {
    if ((infl = LexEntryGetInflection1(le, tense, F_NULL, number, person, mood,
                                       degree, dc))) {
      return(infl);
    }
  }
failure:
  if (complain) {
    Dbg(DBGGENER, DBGBAD,
        "no inflection for <%s>.<%s> %c%c%c%c%c%c", le->srcphrase,
        le->features, (char)tense, (char)gender, (char)number,
        (char)person, (char)mood, (char)degree);
  }
  return(NULL);
}

Word *LexEntryGetInfl(LexEntry *le, Discourse *dc)
{
  return(LexEntryGetInflection(le, F_NULL, F_NULL, F_NULL, F_NULL, F_NULL,
                               F_NULL, 1, dc));
}

Word *LexEntryGetAlter(LexEntry *le, char *features, int alter)
{
  Word	*w;
  if (!le) return(NULL);
  if (StringIn(alter, features)) return(NULL);	/* Already altered. */
  for (w = le->infl; w; w = w->next) {
    if (features == w->features) continue;
    if (StringIn(F_INFREQUENT, w->features)) continue;
    if (StringIn(F_OLD, w->features)) continue;
    if (StringIn(F_OTHER_DIALECT, w->features)) continue;
    if (StringIn(F_INFORMAL, w->features)) continue;
      /* todo: DC(dc).style */
    if (StringAllIn(features, w->features) &&
        StringIn(alter, w->features)) {
      return(w);
    }
  }
  return(NULL);
}

LexEntry *LexEntryGetFrenchAux(int etre)
{
  if (etre) return(LexEntryFind("être", "V", FrenchIndex));
  else return(LexEntryFind("avoir", "V", FrenchIndex));
}

/* todo: Do not create link if one already exists with the same features. */
ObjToLexEntry *LexEntryLinkToObj(LexEntry *le, Obj *obj, char *features,
                                 HashTable *ht, LexEntry **les, int lelen,
                                 int *delims, int *subcats,
                                 ThetaRole *theta_roles_expl)
{
  char		feat[FEATLEN], *p;
  ThetaRole	*theta_roles;
  LexEntryBuildLinkFeatures(features, feat);
  p = HashTableIntern(ht, feat);
  theta_roles =
    ThetaRoleBuild(features, les, lelen, delims, subcats, obj,
                   theta_roles_expl);
  obj->ole = ObjToLexEntryCreate(p, le, theta_roles, obj->ole);
  le->leo = LexEntryToObjCreate(p, obj, theta_roles, le->leo);
  return(obj->ole);
}

Bool LexEntryConceptIsAncestor(Obj *con, LexEntry *le)
{
  LexEntryToObj	*leo;
  if (!le) return(0);
  for (leo = le->leo; leo; leo = leo->next) {
    if (ISA(con, leo->obj)) return(1);
  }
  return(0);
}

Bool LexEntryAllConceptsFeat(int usagefeat, LexEntry *le)
{
  LexEntryToObj	*leo;
  if (le == NULL) return(0);
  if (le->leo == NULL) return(0);
  for (leo = le->leo; leo; leo = leo->next) {
    if (!StringIn(usagefeat, leo->features)) return(0);
  }
  return(1);
}

Bool LexEntryAnyConceptsFeat(int usagefeat, LexEntry *le)
{
  LexEntryToObj	*leo;
  if (le == NULL) return(0);
  if (le->leo == NULL) return(0);
  for (leo = le->leo; leo; leo = leo->next) {
    if (StringIn(usagefeat, leo->features)) return(1);
  }
  return(0);
}

Obj *LexEntryGetConIsAncestor(Obj *con, LexEntry *le)
{
  LexEntryToObj	*leo;
  if (!le) return(NULL);
  for (leo = le->leo; leo; leo = leo->next) {
    if (ISA(con, leo->obj)) return(leo->obj);
  }
  return(NULL);
}

Bool LexEntrySatisfiesCaseFilter(Obj *cas, LexEntry *le, int lang)
{
  int			yes, no;
  Obj			*cas1;
  LexEntryToObj	*leo;
  if (!le) return(1);
  if (!le->leo) return(1);
  /* This is coded to allow:
   *   "him" which is both of case N("iobj") and N("obj").
   *   "mother" which is not marked for case in English.
   */
  yes = no = 0;
  for (leo = le->leo; leo; leo = leo->next) {
    if (lang == F_FRENCH && ISA(N("verb-phrase-pronoun"), leo->obj)) return(0);
    if ((cas1 = DbGetRelationValue(&TsNA, NULL, N("case-of"), leo->obj,
         NULL))) {
      if (cas1 == cas) yes++;
      else no++;
    }
  }
  if (yes > 0) return(1);
  if (no == 0) return(1);
  return(0);
}

Bool IEConceptIs(IndexEntry *ie, Obj *class)
{
  LexEntryToObj	*leo;
  for (; ie; ie = ie->next) {
    for (leo = ie->lexentry->leo; leo; leo = leo->next) {
      if (ISA(class, leo->obj)) return(1);
    }
  }
  return(0);
}

/* Read inflection files. */

Word *LexEntryReadInflWord(FILE *stream, char *lefeat, HashTable *ht)
{
  char	word[PHRASELEN], features[FEATLEN];
  if (StreamPeekc(stream) == TREE_SLOT_SEP) {
    StreamReadc(stream);
    Dbg(DBGLEX, DBGHYPER, "LexEntryReadInflWord: done", E);
    return(NULL);
  }
  StreamReadTo(stream, 1, word, PHRASELEN, LE_FEATURE_SEP, TERM, TERM);
  StringCopyExcept(word, TREE_ESCAPE, PHRASELEN, word);
  StreamReadTo(stream, 1, features, FEATLEN, LE_SEP, TERM, TERM);
#ifdef maxchecking
  FeatureCheck(features, FT_INFL_FILE, "inflection in inflection file");
  if (ht == EnglishIndex && F_NULL != FeatureGet(features, FT_MOOD) &&
      (!streq(word, "were")) && (!streq(word, "was")) &&
      (!streq(word, "Were")) && (!streq(word, "Was"))) {
    Dbg(DBGLEX, DBGBAD, "English verb mood <%s>.<%s>", word, features);
    /* English verb moods are generated algorithmically and should not
     * be specified in the inflection file.
     */
  }
#endif
#ifdef STATS
  StatsInflFeat += strlen(features);
#endif
  StringCat(features, lefeat, FEATLEN);
  return(WordCreate(word, features, NULL, ht));
}

LexEntry *LexEntryReadInfl(FILE *stream, HashTable **htp)
{
  char		srcphrase[PHRASELEN], features[FEATLEN];
  Word		*word;
  LexEntry	*lexentry;
  HashTable	*ht;
#ifdef maxchecking
  int		lang, pos;
#endif

  if (StreamPeekc(stream) == EOF) {
    Dbg(DBGLEX, DBGHYPER, "LexEntryRead: done", E);
    return(NULL);
  }
  StreamReadTo(stream, 1, srcphrase, PHRASELEN, LE_FEATURE_SEP, TERM, TERM);
  StringCopyExcept(srcphrase, TREE_ESCAPE, PHRASELEN, srcphrase);
  Dbg(DBGLEX, DBGHYPER, "<%s>", srcphrase);
  StreamReadTo(stream, 1, features, FEATLEN, LE_SEP, TERM, TERM);
#ifdef maxchecking
  if (StringIn(F_ADJECTIVE, features))
    FeatureCheck(features, FT_ADJLEXENTRY, "adj lexentry in inflection file");
  else FeatureCheck(features, FT_LEXENTRY, "lexentry in inflection file");
  pos = FeatureGetRequired(srcphrase, features, FT_POS);
  lang = FeatureGetRequired(srcphrase, features, FT_LANG);
#endif
#ifdef STATS
  StatsInflSrcPhraseFeat += strlen(features);
#endif
  StringAppendChar(features, FEATLEN, F_INFL_CHECKED);
  ht = LexEntryLangHt(features);
  lexentry = LexEntryCreate(srcphrase, features, ht);
  while ((word = LexEntryReadInflWord(stream, lexentry->features, ht))) {
    LexEntryAddInfl(lexentry, word);
  }
  if (lexentry->infl == NULL) {
  /* add one inflection if none provided */
#ifdef maxchecking
    if (pos == F_NOUN || pos == F_VERB ||
        (lang == F_FRENCH && pos == F_ADJECTIVE))
      Dbg(DBGLEX, DBGBAD, "no inflections defined for <%s>.<%s>",
          srcphrase, features);
#endif
    LexEntryAddInfl(lexentry,
                    WordCreate(lexentry->srcphrase, lexentry->features,
                               NULL, ht));
  }
  *htp = ht;
  return(lexentry);
}

void LexEntryReadInflStream(FILE *stream)
{
  LexEntry *lexentry;
  HashTable	*ht;
  while ((lexentry = LexEntryReadInfl(stream, &ht))) {
    IndexEntryIndex(lexentry, ht);
    if (AnaMorphOn) AnaMorphTrain(lexentry);
  }
  StreamClose(stream);
}

void LexEntryReadInflFile(char *filename)
{
  FILE	*stream;
  if (LexEntryOff) return;
  if (NULL == (stream = StreamOpenEnv(filename, "r"))) return;
  LexEntryReadInflStream(stream);
}

/* Reading of lexical entries in a database file. */

int LexEntryObjNameFix(int c, int report_fixes)
{
  if (Char_isalpha(c) || Char_isdigit(c)) {
    return(c);
  } else {
    if (report_fixes && c != '.') {
      Dbg(DBGLEX, DBGBAD, "illegal Obj name character %c", (char)c);
    }
    c = CharToLowerNoAccents(c);
    if (Char_isalpha(c) || Char_isdigit(c)) {
      return(c);
    }
    return('-');
  }
}

void LexEntryToObjName(char *in, char *out, int feature_sep, int report_fixes)
{
  while (*in) {
    if (*in == TREE_ESCAPE) {
      in++;
      if (in[0] == TERM) break;
      *out++ = LexEntryObjNameFix(*in, report_fixes);
      in++;
      continue;
    }
    if (feature_sep && *in == LE_FEATURE_SEP) break;
    if (*in == LE_PHRASE_INFLECT || *in == LE_PHRASE_NO_INFLECT) {
      in++;
      while (*in && *in != LE_FEATURE_SEP && !LexEntryWhitespace(*in)) in++;
    } else {
      if (LexEntryWhitespace(*in) || *in == LE_FEATURE_SEP) {
        *out++ = '-';
        in++;
        while (*in && LexEntryWhitespace(*in)) in++;
      } else {
        *out++ = LexEntryObjNameFix(*in, report_fixes);
        in++;
      }
    }
  }
  *out = TERM;
}

Bool LexEntryRead(FILE *stream, Obj *concept)
{
  char	lextext[LINELEN];
  int	c;
  if (((c = StreamPeekc(stream)) == TREE_LEVEL) || c == TREE_SLOT_SEP ||
      c == EOF) {
    return(0);
  }
  StreamReadTo(stream, 1, lextext, LINELEN, LE_SEP, TERM, TERM);
  if (lextext[0] == TERM) {
    Dbg(DBGLEX, DBGBAD, "empty lexentry text for <%s>", M(concept));
    return(1);
  }
  LexEntryReadString(lextext, concept, 0);
  return(1);
}

/* Example: lextext = "test# entry*,another#A test# entry*.Az"
 * Steps on lextext.
 */
void LexEntryReadString(char *lextext, Obj *concept, int assume_freq)
{
  char	*features;
  if (LexEntryOff) return;
  features = StringSkipTo(lextext, LE_FEATURE_SEP, TERM, TERM);
  if (features[0] == TERM) {
    Dbg(DBGLEX, DBGBAD, "features missing for <%s> concept <%s>", lextext,
        M(concept));
    return;
  }
#ifdef STATS
  StatsLePhraseFeat += strlen(features);
#endif
#ifdef maxchecking
  if (StringIn(LE_FEATURE_SEP, features)) {
    Dbg(DBGLEX, DBGBAD, "redundant feature separator in <%s>.<%s> object <%s>",
        lextext, features, M(concept));
  }
  if (features <= lextext) {
    Dbg(DBGLEX, DBGBAD, "LexEntryReadString");
    return;
  }
#endif
  *(features-1) = TERM;
  if (assume_freq && !StringIn(F_INFREQUENT, features)) {
    StringAppendChar(features, FEATLEN, F_FREQUENT);
  }
  if (StringIn(F_NAME, features)) {
    LexEntryReadName(lextext, concept, features);
  } else if (StringIn(FeatureGetRequired("LexEntryReadString",
                                         features, FT_LANG),
                      StdDiscourse->langs)) {
    LexEntryReadString1(lextext, concept, features);
  }
}

LexEntry *LexEntryInflectWord1(char *word, int lang, int pos,
                               char *known_features, HashTable *ht)
{
  LexEntry	*le;
  if ((le = MorphInflectWord(word, lang, pos, known_features, ht))) {
    return(le);
  }
  le = LexEntryCreate(word, known_features, ht);
  LexEntryAddInfl(le, WordCreate(word, known_features, NULL, ht));
  IndexEntryIndex(le, ht);
  return(le);
}

/* Word is assumed not to exist already. */
void LexEntryInflectWord0(char *word, int lang, char *known_features,
                          HashTable *ht, /* RESULTS */ LexEntry **le1,
                          LexEntry **le2, LexEntry **le3, LexEntry **le4,
                          LexEntry **le5)
{
  int			pos;
  char			pos_restrict[FEATLEN], feat[FEATLEN];
  LexEntry		*le;
  pos = FeatureGet(known_features, FT_POS);
  if (pos == F_NULL) {
    *le1 = *le2 = *le3 = *le4 = *le5 = NULL;
    MorphRestrictPOS(word, lang, pos_restrict);
    if (StringIn(F_NOUN, pos_restrict)) {
      StringCpy(feat, known_features, FEATLEN);
      StringAppendChar(feat, FEATLEN, F_NOUN);
      if ((le = LexEntryInflectWord1(word, lang, F_NOUN, feat, ht))) {
        *le1 = le;
      }
    }
    if (StringIn(F_VERB, pos_restrict)) {
      StringCpy(feat, known_features, FEATLEN);
      StringAppendChar(feat, FEATLEN, F_VERB);
      if ((le = LexEntryInflectWord1(word, lang, F_VERB, feat, ht))) {
        *le2 = le;
      }
    }
    if (StringIn(F_ADJECTIVE, pos_restrict)) {
      StringCpy(feat, known_features, FEATLEN);
      StringAppendChar(feat, FEATLEN, F_ADJECTIVE);
      if ((le = LexEntryInflectWord1(word, lang, F_ADJECTIVE, feat, ht))) {
        *le3 = le;
      }
    }
    if (StringIn(F_ADVERB, pos_restrict)) {
      StringCpy(feat, known_features, FEATLEN);
      StringAppendChar(feat, FEATLEN, F_ADVERB);
      if ((le = LexEntryInflectWord1(word, lang, F_ADVERB, feat, ht))) {
        *le4 = le;
      }
    }
    if (StringIn(F_INTERJECTION, pos_restrict)) {
      StringCpy(feat, known_features, FEATLEN);
      StringAppendChar(feat, FEATLEN, F_INTERJECTION);
      if ((le = LexEntryInflectWord1(word, lang, F_INTERJECTION, feat, ht))) {
        *le5 = le;
      }
    }
  } else {
    *le1 = LexEntryInflectWord1(word, lang, pos, known_features, ht);
    *le2 = *le3 = *le4 = *le5 = NULL;
  }
}

/* One LexEntry returned at "random". Should only be used for success code. */
LexEntry *LexEntryInflectWord(char *word, int lang, char *known_features,
                              HashTable *ht)
{
  LexEntry	*le1, *le2, *le3, *le4, *le5;
  LexEntryInflectWord0(word, lang, known_features, ht, &le1, &le2, &le3, &le4,
                       &le5);
  if (le1) return(le1);
  if (le2) return(le2);
  if (le3) return(le3);
  if (le4) return(le4);
  if (le5) return(le5);
  return(NULL);
}

void LexEntryPrintInvariantMFSP(FILE *stream, char *word, char *features,
                                char *comment, char *known_features)
{
  StreamPrintf(stream, "%s.%s/%s %s\n%s.MS/%s.MP/%s.FS/%s.FP/\n|\n",
               word, features, comment, known_features, word, word, word, word);
}

void LexEntryPrintInvariantSP(FILE *stream, char *word, char *features,
                              char *comment, char *known_features)
{
  StreamPrintf(stream, "%s.%s/%s %s\n%s.S/%s.P/\n|\n",
               word, features, comment, known_features, word, word);
}

void LexEntryPrintEmpty(FILE *stream, char *word, char *features, char *comment,
                        char *known_features)
{
  StreamPrintf(stream, "%s.%s/%s %s\n|\n", word, features, comment,
               known_features);
}

LexEntry *LexEntryNewWord(char *word, char *features, Obj *concept,
                          HashTable *ht)
{
  int		lang, pos;
  char		lefeatures[FEATLEN], inflfeatures[FEATLEN], *comment;
  LexEntry	*le;
  lang = LexEntryHtLang(ht);
  pos = FeatureGetRequired(word, features, FT_POS);
  if (MorphIsInvariant(word, features, concept)) {
    le = NULL;
    comment = "; inv";
  } else {
    le = MorphInflectWord(word, lang, pos, features, ht);
    comment = "; fail";
  }
  if (le) return(le);
  LexEntryBuildLeFeatures(features, pos, lefeatures);
  if (lang == F_FRENCH) {
    if (pos == F_NOUN) {
      if (StringIn(F_MASC_FEM, features)) {
        LexEntryPrintInvariantMFSP(StreamSuggFrenchInfl, word, lefeatures,
                                   comment, features);
      } else {
        LexEntryPrintInvariantSP(StreamSuggFrenchInfl, word, lefeatures,
                                 comment, features);
      }
    } else if (pos == F_ADJECTIVE) {
      LexEntryPrintInvariantMFSP(StreamSuggFrenchInfl, word, lefeatures,
                                 comment, features);
    } else {
      LexEntryPrintEmpty(StreamSuggFrenchInfl, word, lefeatures, comment,
                         features);
    }
  } else {
    if (pos == F_NOUN) {
      LexEntryPrintInvariantSP(StreamSuggEnglishInfl, word, lefeatures,
                               comment, features);
    } else {
      LexEntryPrintEmpty(StreamSuggEnglishInfl, word, lefeatures, comment,
                         features);
    }
  }

  le = LexEntryCreate(word, lefeatures, ht);

  if (pos == F_NOUN) {
    StringCpy(inflfeatures+1, lefeatures, FEATLEN-1);
    inflfeatures[0] = F_SINGULAR;
    LexEntryAddInfl(le, WordCreate(word, inflfeatures, NULL, ht));
    inflfeatures[0] = F_PLURAL;
    LexEntryAddInfl(le, WordCreate(word, inflfeatures, NULL, ht));
  } else {
  /* This is new. It used to be all parts of speech got S and P. Why?
   * todo: Also, why bother storing S and P for an invariant N?
   */
    LexEntryAddInfl(le, WordCreate(word, lefeatures, NULL, ht));
  }

  IndexEntryIndex(le, ht);
  return(le);
}

/* Associate <nm> with <human>. */
void LexEntryNameForHuman(Obj *human, int gender, Name *nm)
{
  ObjList	*p;
  LearnNameLeUPDATE(nm, gender, human);
  /* Call first so assertions will gen properly. */
  for (p = nm->pretitles; p; p = p->next) {
    DbAssert(&TsRangeAlways, L(N("pre-title-of"), human, p->obj, E));
  }
  if (nm->givennames1) {
    DbAssert(&TsRangeAlways,
             L(N("first-name-of"), human, nm->givennames1->obj, E));
  }
  if (nm->givennames2) {
    DbAssert(&TsRangeAlways, L(N("second-name-of"), human,
             nm->givennames2->obj, E));
  }
  if (nm->givennames3) {
    DbAssert(&TsRangeAlways, L(N("third-name-of"), human,
             nm->givennames3->obj, E));
  }
  if (nm->surnames1) {
    DbAssert(&TsRangeAlways, L(N("first-surname-of"), human,
             nm->surnames1->obj, E));
  }
  if (nm->surnames2) {
    DbAssert(&TsRangeAlways, L(N("second-surname-of"), human,
             nm->surnames2->obj, E));
  }
  for (p = nm->posttitles; p; p = p->next) {
    DbAssert(&TsRangeAlways, L(N("post-title-of"), human, p->obj, E));
  }
}

void LexEntryReadNameA(char *lextext, Obj *concept, char *features)
{
  int	gender;
  Name	*nm;
  gender = FeatureGet(features, FT_GENDERPLUS);
  StringElimChar(lextext, TREE_ESCAPE);
  if ((nm = TA_NameParseKnown(lextext, gender, 0))) {
    LexEntryNameForHuman(concept, gender, nm);
    NameFree(nm);
  }
}

void LexEntryReadName(char *lextext, Obj *concept, char *features)
{
  if (!AnaMorphOn) LexEntryInsideName = 1;
  LexEntryReadNameA(lextext, concept, features);
  LexEntryInsideName = 0;
}

/* Example: lextext = "test# entry*,another#A test# entry*"
 *          features = "Az"
 */
void LexEntryReadString1(char *lextext, Obj *concept, char *features)
{
  int	gender, pos, number, lang;
  char	phrase[PHRASELEN];
  HashTable	*ht;
#ifdef maxchecking
  FeatureCheck(features, FT_ALL, "database file");
#endif
  gender = FeatureGet(features, FT_GENDERPLUS);
  pos = FeatureDefaultTo(features, FEATLEN, FT_POS, F_NOUN);
  number = FeatureDefaultTo(features, FEATLEN, FT_NUMBER, F_SINGULAR);
  lang = FeatureGetRequired(lextext, features, FT_LANG);
  ht = LexEntryLangHt(features);
  while (lextext[0]) {
    lextext = StringReadTo(lextext, phrase, PHRASELEN, TREE_VALUE_SEP, TERM,
                           TERM);
    if (pos == F_PREFIX || pos == F_SUFFIX) {
      Lex_WordForm2Enter(phrase, features, pos, lang, concept);
    } else {
      LexEntryPhraseRead(phrase, features, concept, gender, pos, number, lang,
                         ht);
    }
  }
}

#define MAXWORDSINPHRASE	20

Word *NewInflections;

void LexEntryBuildLeFeatures(char *in, int pos, /* RESULT */ char *out)
{
  if (pos == F_ADJECTIVE) {
    FeatureCopyIn(in, FT_ADJLEXENTRY, out);
  } else FeatureCopyIn(in, FT_LEXENTRY, out);
}

void LexEntryBuildLinkFeatures(char *in, /* RESULT */ char *out)
{
  if (StringIn(F_COMMON_INFL, in)) {
    FeatureCopyIn(in, FT_INFL_LINK, out);
  } else FeatureCopyIn(in, FT_LINK, out);
}

/* Example: in_word = "anotherÝanother" (inflected)
 *          features = "Nz"
 */
LexEntry *LexEntryReadWord(char *in_word, char *features, int nocaseadjust,
                           Obj *concept, HashTable *ht,
                           int inside_phrase,
                           /* RESULTS */ char **wordp)
{
  char		*word, *srcphrase, tmp[DWORDLEN];
  int		savechar;
  LexEntry	*le;
 
  /* todo: If INVARIANT, check that lexicon agrees about this. */
  if (StringIn(F_NOUN, features) && StringIn(F_ADJECTIVE, features)) {
    LexEntryReadNounAdj(in_word, features, concept, ht, inside_phrase, wordp);
    return(NULL);
      /* To indicate something is wrong if NA appears inside phrase. */
  }
  savechar = 0;
  Dbg(DBGLEX, DBGHYPER, "in_word <%s>.<%s>", in_word, features);
  if (StringIn(LE_WORD_SRCPHRASE, in_word)) {
    srcphrase = StringReadTo(in_word, tmp, DWORDLEN, LE_WORD_SRCPHRASE, TERM,
                             TERM);
    word = HashTableIntern(ht, tmp);
    Dbg(DBGLEX, DBGHYPER, "parsed <%s>.<%s> as <%s> <%s>",
        in_word, features, word, srcphrase);
    if (!(le = LexEntryGetSrcphrase(srcphrase,
                                    FeatureGet(features, FT_GENDER),
                                    FeatureGet(features, FT_POS),
                                    ht))) {
      Dbg(DBGLEX, DBGBAD, "nonexistent srcphrase <%s> in <%s>.<%s>",
          srcphrase, word, features);
    }
  } else {
    word = HashTableIntern(ht, in_word);
    le = LexEntryFindInfl(word, features, ht, inside_phrase);
  }
#ifdef notdef
  /* todo: This was removed to allow "Art" to be different from "art".
   * Is there any reason why this is needed anymore? The philosophy is
   * that case is significant. In production, relaxed matching will attempt
   * to find things when there is a failure. todoSCORE
   */
  if (!le) {
    if (CharIsUpper((uc)(word[0])) && !nocaseadjust) {
      savechar = word[0];
      word[0] = CharToLower((uc)(word[0])); /* todo: not a good idea */
      le = LexEntryFindInfl(word, features, ht, inside_phrase);
      word[0] = savechar;
      savechar = 0;
    }
  }
#endif
  if (!le) le = LexEntryNewWord(word, features, concept, ht);
  if (concept) {
    LexEntryLinkToObj(le, concept, features, ht, NULL, 0, NULL, NULL, NULL);
  }
  if (savechar) word[0] = savechar;
  if (wordp) *wordp = word;
  return(le);
}

void LexEntryBuildNAFeatures(char *features, Obj *concept,
                             /* RESULTS */ char *nfeatures, char *afeatures)
{
  StringCopyExcept(features, F_NOUN, FEATLEN, afeatures);
  StringCopyExcept(features, F_ADJECTIVE, FEATLEN, nfeatures);
}

/* Example: word = "Parisien"
 *          features = "NAy"
 */
void LexEntryReadNounAdj(char *word, char *features, Obj *concept,
                         HashTable *ht, int inside_phrase,
                         /* RESULTS */ char **wordp)
{
  int		savechar;
  char		nfeatures[FEATLEN];
  char		afeatures[FEATLEN];
  LexEntryBuildNAFeatures(features, concept, nfeatures, afeatures);
  LexEntryReadWord(word, nfeatures, 1, concept, ht, inside_phrase, wordp);
  if (F_FRENCH == FeatureGet(features, FT_LANG)) {
    if (!CharIsUpper((uc)(word[0]))) {
      Dbg(DBGLEX, DBGDETAIL, "non uppercase §NA <%s>.<%s>", word, features);
    }
    savechar = word[0];
    word[0] = CharToLower((uc)(word[0]));
    LexEntryReadWord(word, afeatures, 1, concept, ht, inside_phrase, wordp);
    word[0] = savechar;
  } else {
    LexEntryReadWord(word, afeatures, 1, concept, ht, inside_phrase, wordp);
  }
}

/* Example: phraseword = "another#A"
 */
LexEntry *LexEntryReadPhraseWord(char *phraseword, int gender, int pos,
                                 int number, int lang, Obj *concept,
                                 HashTable *ht, int noinflect,
                                 /* RESULTS */
                                 char **wordp, int *delimp, int *subcatp)
{
  char		*features, in_word[DWORDLEN];
  int		delim, subcat, wordgender;
  features = StringReadTos(phraseword, 1, in_word, DWORDLEN, LE_PHRASE_FEATSEP);
#ifdef maxchecking
  FeatureCheck(features, FT_ALL, "phrase word");
#endif
#ifdef maxchecking
  if (features <= phraseword) {
    Dbg(DBGLEX, DBGBAD, "features <= phraseword <%s> -- comma not escaped?",
        phraseword);
  }
#endif
#ifdef STATS
  StatsLeWordFeat += strlen(features);
#endif
  subcat = F_NULL;
  delim = *(features-1);
  if (delim != LE_PHRASE_INFLECT && delim != LE_PHRASE_NO_INFLECT &&
      delim != LE_PHRASE_PREP && delim != LE_PHRASE_OPT_PREP) {
    if (streq(in_word, LE_NULLNOUN) || streq(in_word, "½s")) {
      delim = LE_PHRASE_INFLECT;
    } else if (streq(in_word, "s") || streq(in_word, LE_PHRASE_OBJECT)) {
      delim = LE_PHRASE_NO_INFLECT;
    } else if (noinflect) {
      delim = LE_PHRASE_NO_INFLECT;
    } else {
      Dbg(DBGLEX, DBGBAD,
          "<%s> is missing inflection instructions", phraseword);
      delim = LE_PHRASE_NO_INFLECT;
    }
  }
  if (delim == LE_PHRASE_PREP || delim == LE_PHRASE_OPT_PREP) {
    StringAppendChar(features, WORDLEN, F_PREPOSITION);
    subcat = FeatureGet(features, FT_SUBCAT);
      /* Note this is more general than LexEntryPhrasalVerbReadNext
       * which only allows 1 character after the LE_PHRASE_(OPT_)PREP.
       */
  }
  if (streq(in_word, "s")) {
    pos = FeatureDefaultTo(features, FEATLEN, FT_POS, F_ELEMENT);
  } else {
    pos = FeatureDefaultTo(features, FEATLEN, FT_POS, pos);
  }
  FeatureDefaultTo(features, FEATLEN, FT_NUMBER, number);
  if (F_NEUTER == (wordgender = FeatureGet(features, FT_GENDER))) {
    StringElimChar(features, F_NEUTER);
  } else if ((gender != F_NULL) && (wordgender == F_NULL) && 
      (pos == F_NOUN || pos == F_ADJECTIVE)) {
    StringAppendChar(features, FEATLEN, gender);
  }
  StringAppendChar(features, FEATLEN, lang);
  *delimp = delim;
  *subcatp = subcat;
  return(LexEntryReadWord(in_word, features, 0, NULL, ht, 1, wordp));
}

void LexEntryInflectPhraseProb(char *srcphrase, char *inflfeatures)
{
  if (!((streq(srcphrase, "clothing") && streq(inflfeatures, "P")) ||
        (streq(srcphrase, "news") && streq(inflfeatures, "P")) ||
        (streq(srcphrase, "News") && streq(inflfeatures, "P")) ||
        (streq(srcphrase, "Pyrénées") && streq(inflfeatures, "FS")) ||
        (streq(srcphrase, "clothes") && streq(inflfeatures, "S")) ||
        (streq(srcphrase, "will") && streq(inflfeatures, "f")) ||
        (streq(srcphrase, "will") && streq(inflfeatures, "d")) ||
        (streq(srcphrase, "will") && streq(inflfeatures, "e")) ||
        (streq(srcphrase, "breeches") && streq(inflfeatures, "S")) ||
        (streq(srcphrase, "gens") && streq(inflfeatures, "MS")) ||
        (streq(srcphrase, "foutre")))) {
    Dbg(DBGLEX, DBGBAD, "no compatible inflection for <%s>.<%s>",
        srcphrase, inflfeatures);
  }
}

/* Example:
 * inflfeatures: "1S"
 * les: ("be" "here")
 * result: Word "is here"
 */
Word *LexEntryInflectPhrase2(LexEntry **les, int lelen, char **words,
                             int *delims, char *lefeat, char *inflfeatures,
                             char *matchfeat, Word *rest, int forcelower,
                             HashTable *ht)
{
  int		i, found, len;
  char		inflphrase[PHRASELEN], inflfeat[FEATLEN], *p;
  LexEntry	*le;
  Word		*infl;
  inflphrase[0] = TERM;
  for (i = 0; i < lelen; i++) {
    p = StringEndOf(inflphrase);
    if (delims[i] == LE_PHRASE_PREP || delims[i] == LE_PHRASE_OPT_PREP) {
    /* do not include this in result phrase */
    } else if (delims[i] != LE_PHRASE_INFLECT) {
      StringCpy(p, words[i], PHRASELEN-(p-inflphrase));
      StringAppendChar(inflphrase, PHRASELEN, SPACE);
    } else if (!(le = les[i])) {
      Dbg(DBGLEX, DBGBAD, "NA inside phrase is not permitted");
      StringCpy(p, words[i], PHRASELEN-(p-inflphrase));
      StringAppendChar(inflphrase, PHRASELEN, SPACE);
    } else {
      found = 0;
      for (infl = le->infl; infl; infl = infl->next) {
        /* todo: Generate phrase inflections for dialect/frequency/style/...
         * combos, but only when necessary.
         * Generate phrase inflections for FT_ALTER forms (only when
         * F_CONTRACTION at i=0 and F_ELISION at i=n?).
         */
        if (F_NULL == FeatureGet(infl->features, FT_STYLE) &&
            F_NULL == FeatureGet(infl->features, FT_ALTER) &&
            FeatureMatch(F_AMERICAN, FeatureGet(infl->features, FT_DIALECT)) &&
            F_NULL == FeatureGet(infl->features, FT_FREQ) &&
            FeatureMatch(F_POSITIVE, FeatureGet(infl->features, FT_DEGREE)) &&
            FeatureCompatInflect(infl->features, matchfeat) &&
            FeatureCompatInflect(infl->features, inflfeatures)) {
          found = 1;
          if (CharIsUpper((uc)((words[i])[0]))) {
            *p = CharToUpper((uc)(infl->word[0]));
            *(p+1) = TERM;
            StringCat(inflphrase, infl->word+1, PHRASELEN);
          } else {
            StringCpy(p, infl->word, PHRASELEN-(p-inflphrase));
          }
          StringAppendChar(inflphrase, PHRASELEN, SPACE);
          break;
        }
      }
      if (!found) {
        LexEntryInflectPhraseProb(le->srcphrase, inflfeatures);
        /*
        StringCpy(p, words[i], PHRASELEN-(p-inflphrase)););
        StringAppendChar(inflphrase, PHRASELEN, SPACE);
        */
        return(rest);
      }
    }
    if (forcelower) {
      if (!CharIsUpper((uc)(p[0]))) {
        Dbg(DBGLEX, DBGDETAIL, "non uppercase §NA in phrase <%s> <%s¯>",
            p, inflphrase);
      }
      p[0] = CharToLower((uc)(p[0]));
    }
  }
  len = strlen(inflphrase);
  if (len > 0) {
    inflphrase[len-1] = TERM;
  } else {
    Dbg(DBGLEX, DBGBAD, "empty phrase");
  }
  StringCpy(inflfeat, inflfeatures, FEATLEN);
  StringAppendIfNotAlreadyIns(lefeat, FEATLEN, inflfeat);
  if (1) { /* Can set this to 0 to help debugging. */
    StringSearchReplace(inflphrase, "eau ½s", "eaux");
    StringSearchReplace(inflphrase, " ½", "");
    StringSearchReplace(inflphrase, " ø", "");
  }
  Dbg(DBGLEX, DBGHYPER, "phrasal inflection <%s>.<%s>", inflphrase, inflfeat);
  return(WordCreate(inflphrase, inflfeat, rest, ht));
}

int LexEntryNumberOfWords(LexEntry *le)
{
  return(1+StringCountOccurrences(le->srcphrase, SPACE));
}

Bool LexEntryIsPhrase(LexEntry *le)
{
  return(StringIn(SPACE, le->srcphrase));
}

Bool LexEntryIsPhraseOrExpl(LexEntry *le)
{
  ThetaRole	*tr;
  if (LexEntryIsPhrase(le)) return(1);
  if (le && le->leo && le->leo->theta_roles) {
    for (tr = le->leo->theta_roles; tr; tr = tr->next) {
      if (tr->cas == N("expl")) return(1);
    }
  }
  return(0);
}

Bool WordIsPhrase(Word *w)
{
  return(StringIn(SPACE, w->word));
}

Bool IsPhrase(char *s)
{
  return(StringIn(SPACE, s));
}

Bool LexEntryFormPhraseSeps(int lelen, char **seps, int maxlen,
                            /* RESULTS */ char *phrase_seps)
{
  int	i, any_non_space;
  any_non_space = 0;
  phrase_seps[0] = TERM;
  for (i = 0; i < lelen; i++) {
    StringCat(phrase_seps, seps[i], maxlen);
    StringAppendChar(phrase_seps, maxlen, NEWLINE);
    if (!StringAllEqual(seps[i], SPACE)) any_non_space = 1;
  }
  return(any_non_space);
}

void LexEntryAddSep(LexEntry *le, char *inphrase, int maxlen,
                    /* RESULTS */ char *outphrase)
{
  char	*seps;
  if ((!le) || (!(seps = le->phrase_seps))) {
    StringCpy(outphrase, inphrase, maxlen);
    return;
  }
  while (*inphrase) {
    if (*inphrase == SPACE) {
      while (*seps && *seps != NEWLINE) {
        *outphrase = *seps;
        outphrase++;
        seps++;
      }
      if (*seps) seps++;
      inphrase++;
    } else {
      *outphrase = *inphrase;
      outphrase++;
      inphrase++;
    }
  }
  *outphrase = TERM;
}

/* todo: Make this less longwinded. */
LexEntry *LexEntryInflectPhrase1(LexEntry **les, int lelen, char **words,
                                 char **seps, int *delims, int *subcats,
                                 char *features, int gender, int pos,
                                 int number, int lang, Obj *concept,
                                 int forcelower, HashTable *ht)
{
  char		lefeat[FEATLEN], phrase_seps[PHRASELEN];
  LexEntry	*le;
  Word		*infl, *rootinfl;
  LexEntryBuildLeFeatures(features, pos, lefeat);
  infl = rootinfl = NULL;
  if (lang == F_FRENCH) {
  /* todo: If phrase contains adjective, then we need to generate 4 forms
   * of everything?!
   */
    if (pos == F_ADJECTIVE || pos == F_DETERMINER) {
      infl = rootinfl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "MS",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "MP",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "FS",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "FP",
                               "", infl, forcelower, ht);
    } else if (pos == F_EXPLETIVE) {
      infl = rootinfl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "",
                               "", infl, forcelower, ht);
    } else if (pos == F_ADVERB) {
      infl = rootinfl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "",
                               "", infl, forcelower, ht);
    } else if (pos == F_NOUN && gender == F_MASCULINE) {
      infl = rootinfl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "MS",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "MP",
          "", infl, forcelower, ht);
    } else if (pos == F_NOUN && gender == F_FEMININE) {
      infl = rootinfl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "FS",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "FP",
                               "", infl, forcelower, ht);
    } else if (pos == F_NOUN && gender == F_MASC_FEM) {
      infl = rootinfl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "MS",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "MP",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "FS",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "FP",
                               "", infl, forcelower, ht);
    } else if (pos == F_NOUN) {
      infl = rootinfl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "P",
                               "", infl, forcelower, ht);
    } else if (pos == F_VERB) {
      infl = rootinfl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "f",
                                "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pG1S",
                                "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pG2S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pG3S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pG1P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pG2P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pG3P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "iG1S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "iG2S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "iG3S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "iG1P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "iG2P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "iG3P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "sG1S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "sG2S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "sG3S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "sG1P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "sG2P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "sG3P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "uG1S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "uG2S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "uG3S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "uG1P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "uG2P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "uG3P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "cG1S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "cG2S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "cG3S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "cG1P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "cG2P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "cG3P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pJ1S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pJ2S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pJ3S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pJ1P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pJ2P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pJ3P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "iJ1S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "iJ2S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "iJ3S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "iJ1P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "iJ2P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "iJ3P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "dSM",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "dSF",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "dPM",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "dPF",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pI2S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pI1P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pI2P",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "e",
                               "", infl, forcelower, ht);
    }
  } else if (lang == F_ENGLISH) {
    if (pos == F_ADJECTIVE || pos == F_ADVERB) {
      infl = rootinfl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "",
                               "S", infl, forcelower, ht);
    } else if (pos == F_EXPLETIVE) {
      infl = rootinfl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "",
                               "S", infl, forcelower, ht);
    } else if (pos == F_NOUN) {
      infl = rootinfl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "P",
                               "", infl, forcelower, ht);
    } else if (pos == F_VERB) {
      infl = rootinfl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "f",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "p1S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "p3S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "pP",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "i1S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "i3S",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "iP",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "d",
                               "", infl, forcelower, ht);
      infl =
        LexEntryInflectPhrase2(les, lelen, words, delims, lefeat, "e",
                               "", infl, forcelower, ht);
    }
  }
  if (infl == NULL) {
  /* Default case: only one inflection for phrase. */
    rootinfl = infl = LexEntryInflectPhrase2(les, lelen, words, delims, lefeat,
                                             "", "", infl, forcelower, ht);
  }
  if (infl == NULL) {
    Dbg(DBGLEX, DBGBAD, "no phrase inflections <%s>", lefeat);
    return(NULL);
  }
  if (rootinfl == NULL) rootinfl = infl;
  if ((le = LexEntryFind(rootinfl->word, lefeat, ht))) {
    /* If the inflection instructions differ, there might be problems. */
    Dbg(DBGLEX, DBGDETAIL, "polysemous phrase <%s>.<%s>", rootinfl->word,
        lefeat);
    WordFreeList(infl);
  } else {
    Dbg(DBGLEX, DBGHYPER, "root phrase <%s>.<%s>", rootinfl->word, lefeat);
    le = LexEntryCreate(rootinfl->word, lefeat, ht);
    if (LexEntryFormPhraseSeps(lelen, seps, PHRASELEN, phrase_seps)) {
      le->phrase_seps = StringCopy(phrase_seps, "char phrase_seps");
    }
    le->infl = infl;	/* todo: Store in sorted order here too? */
    IndexEntryIndex(le, ht);
  }
  if (concept) {
  /* todo: Right? In expletive case there is no concept. */
    LexEntryLinkToObj(le, concept, features, ht, les, lelen, delims, subcats,
                      NULL);
  }
  return(le);
}

LexEntry *LexEntryInflectPhrase(LexEntry **les, int lelen, char **words,
                                char **seps, int *delims, int *subcats,
                                char *features, int gender, int pos,
                                int number, int lang, Obj *concept,
                                HashTable *ht)
{
  char	nfeatures[FEATLEN];
  char	afeatures[FEATLEN];
  if (StringIn(F_NOUN, features) && StringIn(F_ADJECTIVE, features)) {
    LexEntryBuildNAFeatures(features, concept, nfeatures, afeatures);
    LexEntryInflectPhrase1(les, lelen, words, seps, delims, subcats, afeatures,
                           gender, F_ADJECTIVE, number, lang, concept,
                           (F_FRENCH == FeatureGet(features, FT_LANG)), ht);
    return(LexEntryInflectPhrase1(les, lelen, words, seps, delims, subcats,
                                  nfeatures, gender, F_NOUN, number, lang,
                                  concept, 0, ht));
  } else {
    return(LexEntryInflectPhrase1(les, lelen, words, seps, delims, subcats,
                                  features, gender, pos, number, lang,
                                  concept, 0, ht));
  }
}

ThetaRole *LexEntryPhrasalVerbReadPre(char *phrase, int lang, HashTable *ht,
                                      /* RESULTS */ char **nextp,
                                      char *morefeat)
{
  int		more;
  ThetaRole	*theta_roles_expl;
  theta_roles_expl = NULL;
  morefeat[0] = TERM;
  if (lang == F_FRENCH) {
    more = 1;
    while (more) {
      if (StringHeadEqualAdvance("ne pas ", phrase, &phrase)) {
        theta_roles_expl =
          ThetaRoleAddExplPreWord("pas", F_ADVERB, ht, theta_roles_expl,
                                  TRPOS_PRE_VERB);
      } else if (StringHeadEqualAdvance("ne point ", phrase, &phrase)) {
        theta_roles_expl =
          ThetaRoleAddExplPreWord("point", F_ADVERB, ht, theta_roles_expl,
                                  TRPOS_PRE_VERB);
      } else if (StringHeadEqualAdvance("ne plus ", phrase, &phrase)) {
        theta_roles_expl =
          ThetaRoleAddExplPreWord("plus", F_ADVERB, ht, theta_roles_expl,
                                  TRPOS_PRE_VERB);
      } else if (StringHeadEqualAdvance("ne rien ", phrase, &phrase)) {
        theta_roles_expl =
          ThetaRoleAddExplPreWord("rien", F_ADVERB, ht, theta_roles_expl,
                                  TRPOS_PRE_VERB);
      } else if (StringHeadEqualAdvance("jamais ", phrase, &phrase)) {
        theta_roles_expl =
          ThetaRoleAddExplPreWord("jamais", F_ADVERB, ht, theta_roles_expl,
                                  TRPOS_PRE_VERB);
      } else if (StringHeadEqualAdvance("se ", phrase, &phrase) ||
                 StringHeadEqualAdvance("s'", phrase, &phrase)) {
        theta_roles_expl =
          ThetaRoleAddExplPreWord("se", F_PRONOUN, ht, theta_roles_expl,
                                  TRPOS_PRE_VERB);
        StringAppendChar(morefeat, FEATLEN, F_ETRE);
      } else if (StringHeadEqualAdvance("en ", phrase, &phrase)) {
        theta_roles_expl =
          ThetaRoleAddExplPreWord("en", F_PRONOUN, ht, theta_roles_expl,
                                  TRPOS_PRE_VERB);
      } else if (StringHeadEqualAdvance("s'en ", phrase, &phrase)) {
        StringAppendChar(morefeat, FEATLEN, F_ETRE);
        theta_roles_expl =
          ThetaRoleAddExplPreWord("se", F_PRONOUN, ht, theta_roles_expl,
                                  TRPOS_PRE_VERB);
        theta_roles_expl =
          ThetaRoleAddExplPreWord("en", F_PRONOUN, ht, theta_roles_expl,
                                  TRPOS_PRE_VERB);
      } else if (StringHeadEqualAdvance("s'y ", phrase, &phrase)) {
        StringAppendChar(morefeat, FEATLEN, F_ETRE);
        theta_roles_expl =
          ThetaRoleAddExplPreWord("se", F_PRONOUN, ht, theta_roles_expl,
                                  TRPOS_PRE_VERB);
        theta_roles_expl =
          ThetaRoleAddExplPreWord("y", F_PRONOUN, ht, theta_roles_expl,
                                  TRPOS_PRE_VERB);
      } else more = 0;
    }
  } else if (lang == F_ENGLISH) {
    more = 1;
    while (more) {
      if (StringHeadEqualAdvance("not ", phrase, &phrase)) {
        theta_roles_expl =
          ThetaRoleAddExplPreWord("not", F_ADVERB, ht, theta_roles_expl,
                                  TRPOS_PRE_VERB);
      } else if (StringHeadEqualAdvance("himself ", phrase, &phrase)) {
        theta_roles_expl =
          ThetaRoleAddExplPreWord("himself", F_PRONOUN, ht, theta_roles_expl,
                                  TRPOS_POST_VERB_PRE_OBJ);
      } else more = 0;
    }
  }
  *nextp = phrase;
  return(theta_roles_expl);
}

Bool LexEntryPhraseRead_LelenOvf(int lelen)
{
  if (lelen >= MAXWORDSINPHRASE) {
    Dbg(DBGLEX, DBGBAD, "increase MAXWORDSINPHRASE");
    return(1);
  }
  if (lelen > MaxWordsInPhrase) MaxWordsInPhrase = lelen;
  return(0);
}

/* Returns pointer to next non-white character (possibly TERM). */
char *LexEntryPhrasalVerbReadNext(char *in, /* RESULTS */ char *out, int *delim,
                                  int *subcat, int *position)
{
  char	*orig_in, *orig_out;
  orig_in = in;
  orig_out = out;
  while (*in) {
    if ((*((uc *)in)) == LE_EXPL_SEP) {
      in++;
      while (StringIn((*((uc *)in)), LE_WHITESPACE)) in++;
      *out = TERM;
      StringElimTrailingBlanks(orig_out);
      *delim = LE_PHRASE_INFLECT;
      *subcat = F_NULL;
      *position = TRPOS_POST_VERB_POST_OBJ;
      return(in);
    } else if (*in == LE_PHRASE_PREP || *in == LE_PHRASE_OPT_PREP) {
      *out = TERM;
      if (StringIn(LE_PHRASE_INFLECT, orig_out) ||
          StringIn(LE_PHRASE_NO_INFLECT, orig_out)) {
        /* away#B in^_: back up to before preposition */
        out--;
        while (out > orig_out &&
               (!StringIn((*((uc *)out)), LE_WHITESPACE))) {
          out--;
        }
        *out = TERM;
        *delim = LE_PHRASE_INFLECT;
        *subcat = F_NULL;
        in--;
        while (in > orig_in && (!StringIn((*((uc *)in)), LE_WHITESPACE))) in--;
        while (StringIn((*((uc *)in)), LE_WHITESPACE)) in++;
        return(in);
      } else {
        /* in^_: preposition */
        *delim = *in;
        in++;
        if (StringIn(*((uc *)in), FT_SUBCAT)) {
          *subcat = *((uc *)in);
           in++;
        } else {
          *subcat = F_NULL;
        }
        while (StringIn((*((uc *)in)), LE_WHITESPACE)) in++;
        return(in);
      }
    }
    *out = *in;
    out++;
    in++;
  }
  *delim = LE_PHRASE_INFLECT;
  *subcat = F_NULL;
  *out = TERM;
  return(in);
}

/* Example:
 * in: "clubbing#N" "clubbing"
 * out: "clubbing"  "clubbing"
 */
void LexEntryPhrase_ElimInfl(/* INPUT AND RESULTS */ char *out)
{
  char	*p;
  if (*out == TERM) return;
  for (p = out; *p; p++);
  p--;
  while ((p > out) &&
         (*p != LE_PHRASE_INFLECT) &&
         (*p != LE_PHRASE_NO_INFLECT)) {
    p--;
  }
  if ((*p == LE_PHRASE_INFLECT) ||
      (*p == LE_PHRASE_NO_INFLECT)) {
    *p = TERM;
  }
}

ThetaRole *LexEntryPhrasalVerb_ExplAdd(char *element, int position, int lang,
                                       HashTable *ht,
                                       ThetaRole *theta_roles_expl)
{
  char		feat[FEATLEN];
  LexEntry	*le;
  ThetaRole	*theta_roles;
  feat[0] = F_EXPLETIVE;
  feat[1] = lang;
  feat[2] = TERM;
  if (!StringAnyIn(LE_WHITESPACE, element)) {
    LexEntryPhrase_ElimInfl(element);
  }
  if ((le = LexEntryPhraseRead(element, feat, NULL, F_NULL, F_EXPLETIVE,
                               F_NULL, lang, ht))) {
    /* Note that this order is later reversed. */
    theta_roles = ThetaRoleCreate(0, N("expl"), le, F_NULL, theta_roles_expl);
    theta_roles->position = position;
    return(theta_roles);
  } else {
    Dbg(DBGLEX, DBGBAD, "LexEntryPhrasalVerb_ExplAdd");
    return(theta_roles_expl);
  }
}

/* Examples:
 * se droitiser*.qêVy
 * practice* at#R the#D bar#N.þVz
 * worship* the#D ground#N ø walks# on#R.Véz
 * put* ø away#B in_.Véz
 * ne pas avoir* son*D pareil#NM.Vy
 * not care* about_.VÔzS
 *
 * todo: Shouldn't this work for phrasal adjectives, phrasal X-MAXes, too?
 */
LexEntry *LexEntryPhrasalVerbRead(char *phrase, char *features, Obj *concept,
                                  int gender, int pos, int number, int lang,
                                  HashTable *ht)
{
  int	lelen, delim, subcat, position;
  int	delims[MAXWORDSINPHRASE], subcats[MAXWORDSINPHRASE];
  char	*p, prebuf[WORDLEN], element[PHRASELEN], feat[FEATLEN];
  char	morefeat[FEATLEN], *q;
  char	*verb;
  ThetaRole	*theta_roles_expl;
  LexEntry	*les[MAXWORDSINPHRASE], *verb_le, *le;
  /* Read stuff before verb. */
  theta_roles_expl = LexEntryPhrasalVerbReadPre(phrase, lang, ht, &p, morefeat);
  p = StringReadTo(p, prebuf, DWORDLEN, LE_PHRASE_INFLECT, TERM, TERM);
  if (StringAnyIn(LE_WHITESPACE, prebuf)) {
    for (q = prebuf; *q; q++);
    q--;
    while (q > prebuf && (!StringIn(*q, LE_WHITESPACE))) {
      q--;
    }
    *q = TERM;
    verb = q+1;
    StringElimTrailingBlanks(prebuf);
    theta_roles_expl = LexEntryPhrasalVerb_ExplAdd(prebuf, TRPOS_PRE_VERB,
                                                   lang, ht, theta_roles_expl);
  } else {
    verb = prebuf;
  }
  /* Read verb. */
  if (!(verb_le = LexEntryReadWord(verb, features, 0, NULL, ht, 1, NULL))) {
    Dbg(DBGLEX, DBGBAD, "LexEntryPhrasalVerbRead 1");
    return NULL;
  }
  if (*p == SPACE) {
    p++;
  } else if (*p != TERM) {
    Dbg(DBGLEX, DBGBAD, "LexEntryPhrasalVerbRead 2 <%s> <%s>", phrase, p);
    return NULL;
  }
  /* Read expletive arguments and prepositions. */
  lelen = 0;
  position = TRPOS_POST_VERB_PRE_OBJ;
  while (*p) {
    if (LexEntryPhraseRead_LelenOvf(lelen)) break;
    p = LexEntryPhrasalVerbReadNext(p, element, &delim, &subcat, &position);
    if (element[0] == TERM) continue;
    if (delim == LE_PHRASE_INFLECT) {
    /* Expletive element. */
      theta_roles_expl = LexEntryPhrasalVerb_ExplAdd(element, position, lang,
                                                     ht, theta_roles_expl);
    } else {
    /* Preposition. */
      feat[0] = F_PREPOSITION;
      feat[1] = lang;
      feat[2] = TERM;
      if ((le = LexEntryPhraseRead(element, feat, NULL, F_NULL,
                                   F_PREPOSITION, F_NULL, lang, ht))) {
        les[lelen] = le;
        delims[lelen] = delim;
        subcats[lelen] = subcat;
        lelen++;
      } else {
        Dbg(DBGLEX, DBGBAD, "LexEntryPhrasalVerbRead");
      }
    }
  }
  if (morefeat[0]) StringCat(features, morefeat, FEATLEN);
  LexEntryLinkToObj(verb_le, concept, features, ht, les, lelen, delims, subcats,
                    ThetaRoleReverseDestructive(theta_roles_expl));
  return(verb_le);
}

LexEntry *LexEntryPhraseRead1(char *phrase, char *features, Obj *concept,
                              int gender, int pos, int number, int lang,
                              HashTable *ht)
{
  int		i, lelen, noinflect;
  int		delims[MAXWORDSINPHRASE], subcats[MAXWORDSINPHRASE];
  char		word[WORDLEN], *words[MAXWORDSINPHRASE];
  char		*seps[MAXWORDSINPHRASE], *p, deriv[DWORDLEN];
  LexEntry	*les[MAXWORDSINPHRASE], *le;
  if (!SaveTime) {
  /* todo: Duplicate this in LexEntryPhrasalVerbRead? */
    deriv[0] = lang;
    deriv[1] = pos;
  }
  noinflect = StringIn(F_NO_INFLECT, features);
  lelen = 0;
  p = phrase;
  while (*p) {
    if (LexEntryPhraseRead_LelenOvf(lelen)) break;
    p = StringReadTos(p, 0, word, WORDLEN, LE_WHITESPACE);
    seps[lelen] = MemAlloc(WORDLEN, "char LexEntryPhraseRead");
    p = StringReadToNons(p, 0, seps[lelen], WORDLEN, LE_WHITESPACE);
    if (word[0]) {	/* this condition for "(from#R City# of#R Angels#P)" */
      les[lelen] = LexEntryReadPhraseWord(word, gender, pos, number, lang,
                                          concept, ht, noinflect,
                                          &words[lelen], &delims[lelen],
                                          &subcats[lelen]);
      if (!SaveTime) {
        deriv[lelen+2] = FeatureGet(les[lelen]->features, FT_POS);
      }
      lelen++;
    }
  }
  if (lelen <= 0) {
  /* We've most likely been fed garbage in Learning. */
    Dbg(DBGLEX, DBGDETAIL, "zero lelen");
    return(NULL);
  }
  if (!SaveTime) {
    deriv[lelen+2] = TERM;
    ReportAddPhraseDerive(deriv);
  }
  le = LexEntryInflectPhrase(les, lelen, words, seps, delims, subcats,
                             features, gender, pos, number, lang, concept, ht);
  for (i = 0; i < lelen; i++) {
    MemFree(seps[i], "char LexEntryPhraseRead");
  }
  return(le);
}

/* Example: phrase = "another#A test# entry*"
 *          features = "Nz"
 * Steps on phrase.
 */
LexEntry *LexEntryPhraseRead(char *phrase, char *features, Obj *concept,
                             int gender, int pos, int number, int lang,
                             HashTable *ht)
{
  Dbg(DBGLEX, DBGHYPER, "<%s> lexentry: <%s>.<%s>",
      M(concept), phrase, features);
  StringCopyExcept(phrase, TREE_ESCAPE, PHRASELEN, phrase);
  if (StringAnyIn(LE_WHITESPACE, phrase)) {
    if (pos == F_VERB && (!StringIn(F_FUSED, features))) {
      return(LexEntryPhrasalVerbRead(phrase, features, concept, gender, pos,
                                     number, lang, ht));
    } else {
      return(LexEntryPhraseRead1(phrase, features, concept, gender, pos, number,
                                 lang, ht));
    }
  } else {
    return(LexEntryReadWord(phrase, features, 0, concept, ht, 0, NULL));
  }
}

Bool IndexEntryIsPos(IndexEntry *ie, int pos)
{
  for (; ie; ie = ie->next) {
    if (StringIn(pos, ie->features)) return(1);
  }
  return(0);
}

void LexEntryGuessFeatures(char *word, int lang, HashTable *ht, /* RESULTS */
                           int *gender, int *pos, int *number)
{
  int			freeme;
  IndexEntry	*ie, *orig_ie;
  ie = orig_ie = LexEntryFindPhrase(ht, word, 1, 1, 0, &freeme);
  if (IndexEntryIsPos(ie, F_PREPOSITION)) {
    *gender = FeatureGet(ie->features, FT_GENDER);
    *pos = F_PREPOSITION;
    *number = FeatureGet(ie->features, FT_NUMBER);
    goto done;
  }
  if (IndexEntryIsPos(ie, F_DETERMINER)) {
    *gender = FeatureGet(ie->features, FT_GENDER);
    *pos = F_DETERMINER;
    *number = FeatureGet(ie->features, FT_NUMBER);
    goto done;
  }
  if (ie) { /* was a loop */
    *gender = FeatureGet(ie->features, FT_GENDER);
    *pos = FeatureGet(ie->features, FT_POS);
    *number = FeatureGet(ie->features, FT_NUMBER);
    goto done;
  }
  if (StringTailEq(word, "s")) {
    *gender = F_NULL;
    *pos = F_NOUN;
    *number = F_PLURAL;
    goto done;
  }
  *gender = F_NULL;
  *pos = F_NOUN;
  *number = F_SINGULAR;
done:
  if (freeme) IndexEntryFree(orig_ie);
}

/* Example:
 * gender, pos, number: known features of <in> if available; must generate
 * differences.
 * in: "Principles of A.I."
 * out: "Principles#P of#R A\.I\.#"
 * ht: can be NULL in which case no features are appended to #
 */
void LexEntryPhraseToDbFile(char *in, int gender, int pos, int number, int lang,
                            HashTable *ht, int maxlen, /* RESULTS */ char *out)
{
  int	inword, numwords, w_gender, w_pos, w_number;
  char	word[DWORDLEN], *w;
  inword = numwords = 0;
  w = word;
  maxlen--;
  while (*in) {
    if (LexEntryNonwhite(*((uc *)in))) {
      inword = 1;
      *w++ = *((uc *)in);
    } else {
      if (inword) {
        *w = TERM;
        if (maxlen-- <= 0) goto ovf;
        *out++ = '#';
        if (ht) {
          LexEntryGuessFeatures(word, lang, ht, &w_gender, &w_pos, &w_number);
          if (w_gender != F_NULL && w_gender != gender) {
            if (maxlen-- <= 0) goto ovf;
            *out++ = w_gender;
          }
          if (w_number != F_NULL && w_number != number) {
            if (maxlen-- <= 0) goto ovf;
            *out++ = w_number;
          }
          if (w_pos != F_NULL && w_pos != pos) {
            if (maxlen-- <= 0) goto ovf;
            *out++ = w_pos;
          }
        }
        inword = 0;
        numwords++;
        w = word;
      }
    }
    if (StringIn(*((uc *)in), LE_ESCAPE_CHARS)) {
      if (maxlen-- <= 0) goto ovf;
      *out++ = TREE_ESCAPE;
    }
    if (StringIn(*in, "\n<>")) {
      if (maxlen-- <= 0) goto ovf;
      *out++ = SPACE;
      in++;
    } else {
      if (maxlen-- <= 0) goto ovf;
      *out++ = *in++;
    }
  }
  if (inword && (numwords > 0)) {
    if (maxlen-- <= 0) goto ovf;
    *out++ = '#';
  }
  *out = TERM;
  return;
ovf:
  Dbg(DBGLEX, DBGBAD, "LexEntryPhraseToDbFile ovf");
  *out = TERM;
  return;
}

/* A string version of ChannelRead1 used only on phrases. */
void LexEntryDbFileToPhrase(char *in, /* RESULTS */ char *out)
{
  int	inword;
  inword = 0;
  while (*in) {
    if (LexEntryNonwhite(*in)) {
      *out++ = *in++;
      inword = 1;
    } else {
      if (inword) {
        *out++ = SPACE;
        inword = 0;
      }
      in++;
    }
  }
  *out = TERM;
}

/* LexEntryTool */

void LexEntryTool2(FILE *stream, LexEntry *le)
{
  LexEntryToObj	*leo;
  if (le->leo == NULL) {
    fprintf(stream, "<%s>.<%s> empty leo\n", le->srcphrase, le->features);
    return;
  }
  for (leo = le->leo; leo; leo = leo->next) {
    fprintf(stream, "<%s>.<%s> <%s> <%s>\n", le->srcphrase, le->features,
            leo->features, M(leo->obj));
    ThetaRolePrint(stream, leo->theta_roles, leo->obj);
  }
}

void LexEntryTool1(FILE *stream, HashTable *ht, char *phrase)
{
  IndexEntry    *ie, *origie;
  int		freeme;
  for (ie = origie = LexEntryFindPhrase(ht, phrase, INTPOSINF, 0, 0, &freeme);
       ie; ie = ie->next) {
    if (ie->lexentry) LexEntryTool2(stream, ie->lexentry);
  }
  if (freeme) IndexEntryFree(origie);
}

void LexEntryTool()
{
  char phrase[PHRASELEN];
  printf("Welcome to the lexical entry tool.\n");
  while (1) {
    if (!StreamAskStd("Enter word or phrase: ", PHRASELEN, phrase)) return;
    LexEntryTool1(stdout, EnglishIndex, phrase);
    LexEntryTool1(stdout, FrenchIndex, phrase);
  }
}

/* LexiconDumper */

void LexiconDumper()
{
  char buf[DWORDLEN], fn[FILENAMELEN];
  Obj  *obj;
  FILE *stream;
  printf("Welcome to the lexical entry dumper.\n");
  while (1) {
    if (!StreamAskStd("Enter top concept: ", DWORDLEN, buf)) return;
    if (buf[0] && (obj = NameToObj(buf, OBJ_NO_CREATE))) {
      sprintf(fn, "%s/lexicon.txt", TT_Report_Dir);
      if (NULL == (stream = StreamOpen(fn, "w+"))) stream = stdout;
      LexiconDumper1(stream, obj);
      StreamClose(stream);
    }
  }
}

void LexiconDumper1(FILE *stream, Obj *typ)
{
  int  number;
  char buf[DWORDLEN];
  Obj  *obj;
  ObjToLexEntry *ole;
  Word *dinfl, *infl, *infl1;
  fputs("Name|FullName|Concept|Similarity\n", stream);
  for (obj = Objs; obj; obj = obj->next) {
    if (!ISA(typ, obj)) continue;
    /* Find definitive lexical entry. */
    dinfl = NULL;
    for (ole = obj->ole; ole; ole = ole->next) {
      if (StringIn(F_ISM, ole->features)) continue;
      if (!StringIn(F_FREQUENT, ole->features)) continue;
      if (StringIn(F_COMMON_INFL, ole->features)) {
        number = FeatureGet(ole->features, FT_NUMBER);
      } else {
        number = F_SINGULAR;
      }
      if ((dinfl = LexEntryGetInflection(ole->le, F_NULL, F_NULL, number,
                                         F_NULL, F_NULL, F_NULL, 1,
                                         StdDiscourse))) {
        break;
      }
    }
    if (!dinfl) {
      /* Get last inflection (which is first in database file). */
      for (ole = obj->ole; ole; ole = ole->next) {
        if (StringIn(F_ISM, ole->features)) continue;
        if (StringIn(F_COMMON_INFL, ole->features)) {
          number = FeatureGet(ole->features, FT_NUMBER);
        } else {
          number = F_SINGULAR;
        }
        if ((infl1 = LexEntryGetInflection(ole->le, F_NULL, F_NULL, number,
                                           F_NULL, F_NULL, F_NULL, 1,
                                           StdDiscourse))) {
          dinfl=infl1;
        }
      }
    }

    /* Dump all lexical entries. */
    for (ole = obj->ole; ole; ole = ole->next) {
      if (StringIn(F_ISM, ole->features)) continue;
      if (StringIn(F_COMMON_INFL, ole->features)) {
        number = FeatureGet(ole->features, FT_NUMBER);
      } else {
        number = F_SINGULAR;
      }
      if ((infl = LexEntryGetInflection(ole->le, F_NULL, F_NULL, number, F_NULL,
                                        F_NULL, F_NULL, 1, StdDiscourse))) {
        StringCpy(buf, infl->word, DWORDLEN);
        StringElimChar(buf, '.');
        fputs(buf, stream);
        fputc('|', stream);
        if (dinfl) {
          fputs(dinfl->word, stream);
        } else {
          fputs(infl->word, stream);
        }
        fputc('|', stream);
        fputs(ObjToName(obj), stream);
        fputc('|', stream);
        if (streq(infl->word, dinfl->word) ||
            StringIn(F_FREQUENT, ole->features)) {
          fputs("1.0", stream);
        } else if (StringIn(F_INFREQUENT, ole->features)) {
          fputs("0.7", stream);
        } else {
          fputs("0.99", stream);
        }
        fputc(NEWLINE, stream);
      }
    }
  }
}

/* LexEntryScan */

void LexEntryScan1(FILE *stream, char *features)
{
  LexEntry		*le;
  LexEntryToObj	*leo;
  for (le = AllLexEntries; le; le = le->next) {
    if (StringAllIn(features, le->features)) {
      fprintf(stream, "%s.%s/\n", le->srcphrase, le->features);
    }
    for (leo = le->leo; leo; leo = leo->next) {
      if (StringAllIn(features, leo->features)) {
        fprintf(stream, "==%s//%s.%s.%s/\n", M(leo->obj),
                le->srcphrase, leo->features, le->features);
      }
    }
  }
  fputc(NEWLINE, stream);
}

void LexEntryScan()
{
  char features[FEATLEN];
  printf("[Lexical entry scanner]\n");
  while (1) {
    if (!StreamAskStd("Enter features: ", FEATLEN, features)) return;
    LexEntryScan1(stdout, features);
  }
}

/* LexEntryInflScan */

void LexEntryInflScan1(FILE *stream, char *features)
{
  LexEntry	*le;
  Word		*word;
  for (le = AllLexEntries; le; le = le->next) {
    for (word = le->infl; word; word = word->next) {
      if (StringAllIn(features, word->features)) {
        fprintf(stream, "%s.%s/", word->word, word->features);
      }
    }
  }
  fputc(NEWLINE, stream);
}

void LexEntryInflScan()
{
  char features[FEATLEN];
  printf("[Inflection scanner]\n");
  while (1) {
    if (!StreamAskStd("Enter features: ", FEATLEN, features)) return;
    LexEntryInflScan1(stdout, features);
  }
}

/* LexEntrySuffixGender */

void LexEntrySuffixGender1(FILE *stream, char *suffix)
{
  int		gender, majority_gender;
  long		m, f;
  LexEntry	*le;
  Word		*word;
  m = f = 0;
  for (le = AllLexEntries; le; le = le->next) {
    if (F_FRENCH != FeatureGet(le->features, FT_LANG)) continue;
    if (F_NOUN != FeatureGet(le->features, FT_POS)) continue;
    if (F_NULL == (gender = FeatureGet(le->features, FT_GENDER))) continue;
    if (LexEntryIsPhrase(le) || !MorphIsWord(le->srcphrase)) continue;
    for (word = le->infl; word; word = word->next) {
      if (F_SINGULAR != FeatureGet(word->features, FT_NUMBER)) continue;
      if (StringTailEq(word->word, suffix)) {
        if (gender == F_MASCULINE) m++;
        else if (gender == F_FEMININE) f++;
      }
    }
  }
  fprintf(stream, "%ld M %ld F\n", m, f);
  if (m == 0L || f == 0L) return;
  if (m > f) {
    majority_gender = F_MASCULINE;
  } else {
    majority_gender = F_FEMININE;
  }
  fprintf(stream, "minority items: %ld %ld\n", m, f);
  for (le = AllLexEntries; le; le = le->next) {
    if (F_FRENCH != FeatureGet(le->features, FT_LANG)) continue;
    if (F_NOUN != FeatureGet(le->features, FT_POS)) continue;
    if (F_NULL == (gender = FeatureGet(le->features, FT_GENDER))) continue;
    if (majority_gender == gender) continue;
    if (LexEntryIsPhrase(le) || !MorphIsWord(le->srcphrase)) continue;
    for (word = le->infl; word; word = word->next) {
      if (F_SINGULAR != FeatureGet(word->features, FT_NUMBER)) continue;
      if (StringTailEq(word->word, suffix)) {
        fprintf(stream, "<%s.%s>\n", word->word, word->features);
      }
    }
  }
  fputc(NEWLINE, stream);
}

void LexEntrySuffixGender()
{
  char suffix[FEATLEN];
  printf("Welcome to the suffix gender tool.\n");
  while (1) {
    if (!StreamAskStd("Enter suffix: ", FEATLEN, suffix)) return;
    LexEntrySuffixGender1(stdout, suffix);
    LexEntrySuffixGender1(Log, suffix);
  }
}

/* LexEntryPrintPolysemous */

void LexEntryPrintPolysemous(FILE *stream, int lang)
{
  long		cnt, total;
  LexEntry	*le;
  LexEntryToObj	*leo;
  cnt = total = 0;
  for (le = AllLexEntries; le; le = le->next) {
    total++;
    if (lang != FeatureGet(le->features, FT_LANG)) continue;
    if (le->leo && le->leo->next) {
      StreamSep(stream);
      cnt++;
      fprintf(stream, "<%s.%s> ", le->srcphrase, le->features);
      for (leo = le->leo; leo; leo = leo->next) {
        fprintf(stream, "%s ", M(leo->obj));
      }
      fputc(NEWLINE, stream);
    }
  }
  fprintf(Log, "%ld polysemous out of %ld words\n", cnt, total);
}

/* Returns ObjListList of ObjList of Obj */
ObjListList *LexEntryPolysemous(int lang)
{
  ObjListList   *r, *r0;
  LexEntry	*le;
  LexEntryToObj	*leo;
  r = NULL;
  for (le = AllLexEntries; le; le = le->next) {
    if (lang != FeatureGet(le->features, FT_LANG)) continue;
    if (strlen(le->srcphrase) == 1) continue;
    if (le->leo && le->leo->next) {
      r0 = NULL;
      for (leo = le->leo; leo; leo = leo->next) {
        r0 = ObjListCreate(leo->obj, r0);
      }
      r = ObjListListCreate(r0, r);
      r->obj = N(le->srcphrase); /* todo: Inelegant */
    }
  }
  return r;
}

/* LexEntryPrintPOSAmbiguous */

long LexEntryPrintPOSAmbiguousCnt, LexEntryPrintPOSAmbiguousTotal;

void LexEntryPrintPOSAmbiguous1(char *word, IndexEntry *ie)
{
  char		buf[WORDLEN];
  LexEntryToObj	*leo;
  IndexEntry	*ie1;
  if (ie) {
    LexEntryPrintPOSAmbiguousTotal++;
    if (ie->next) {
/*
      if (strlen(word) != 1) return;
 */
      buf[0] = TERM;
      for (ie1 = ie; ie1; ie1 = ie1->next) {
        StringAppendIfNotAlreadyIn(FeatureGet(ie1->lexentry->features, FT_POS),
                                   WORDLEN, buf);
      }
      if (buf[0] != TERM && buf[1] != TERM) {
        LexEntryPrintPOSAmbiguousCnt++;
        fprintf(Log, "<%s.%s>", word, buf);
        for (ie1 = ie; ie1; ie1 = ie1->next) {
          for (leo = ie1->lexentry->leo; leo; leo = leo->next) {
            fprintf(Log, "\n  %s %s %s", ie1->lexentry->srcphrase,
                    ie1->features, M(leo->obj));
          }
        }
        fputc(NEWLINE, Log);
      }
    }
  }
}

void LexEntryPrintPOSAmbiguous(HashTable *ht)
{
  LexEntryPrintPOSAmbiguousCnt = LexEntryPrintPOSAmbiguousTotal = 0L;
  HashTableForeach(ht, LexEntryPrintPOSAmbiguous1);
  fprintf(Log, "%ld ambiguous out of %ld words\n",
          LexEntryPrintPOSAmbiguousCnt, LexEntryPrintPOSAmbiguousTotal);
}

/* LexEntryCoverageCheck */

void LexEntryCoverageCheck(char *fn, HashTable *ht, int print_missing)
{
  char			word[DWORDLEN], *p, *s;
  int			freeme;
  long			total, found;
  IndexEntry	*ie;
  if (NULL == (s = StringReadFile(fn, 0))) return;
  p = s;
  total = found = 0L;
  StreamSep(Log);
  while (p[0]) {
    StringGetWord_LeNonwhite(word, p, DWORDLEN, &p, NULL);
    total++;
    ie = LexEntryFindPhrase(ht, word, 1, 0, 0, &freeme);
    if (UnsignedIntIsString(word) || ie) {
      found++;
/* To print words which ARE found:
      if (ie && ie->lexentry) {
        fprintf(Log, "%s.%s/\n",
                ie->lexentry->srcphrase, ie->lexentry->features);
      }
 */
    } else if (print_missing) {
      fputs(word, Log);
      fputc(NEWLINE, Log);
    }
    if (freeme) IndexEntryFree(ie);
  }
  fprintf(Log, "%ld of %ld words found (%ld percent)\n", found, total,
          (found*100L)/total);
  StreamSep(Log);
}

/* LexEntryValidateAgainstCorpus */

void LexEntryValidateInflAgainstCorpus(Corpus *corpus, Word *word, int lang)
{
  int	gender_corpus, gender_tt;
  char	word1[DWORDLEN];
  if (lang == F_FRENCH) {
    if (StringIn(F_NOUN, word->features) &&
        !WordIsPhrase(word)) {
      StringToLower(word->word, DWORDLEN, word1);
      if (F_NULL != (gender_corpus = CorpusFrenchGender(corpus, word1))) {
        gender_tt = FeatureGet(word->features, FT_GENDER);
        if (gender_corpus != gender_tt) {
          fprintf(Log, "<%s>.<%s> corpus=<%c>\n", word->word, word->features,
                  (char)gender_corpus);
        }
      }
    }
  }
}

Bool LexEntryIsMascFem(LexEntry *le)
{
  int		m, f;
  Word		*word;
  m = f = 0;
  for (word = le->infl; word; word = word->next) {
    if (StringIn(F_MASCULINE, word->features)) m = 1;
    if (StringIn(F_FEMININE, word->features)) f = 1;
  }
  return(m && f);
}

/* todo: Other features which could be validated/inferred:
 * F_DEFINITE_ART
 * F_NO_PROGRESSIVE
 * ...
 */
void LexEntryValidateAgainstCorpus(Corpus *corpus, int lang)
{
  LexEntry	*le;
  Word		*word;
  if (corpus == NULL) {
    Dbg(DBGLEX, DBGBAD, "corpus not loaded; use corpusload");
    return;
  }
  for (le = AllLexEntries; le; le = le->next) {
    if (lang != FeatureGet(le->features, FT_LANG)) continue;
    if (StringIn(F_REALLY, le->features)) continue;
    if (LexEntryIsMascFem(le)) continue;
    for (word = le->infl; word; word = word->next) {
      LexEntryValidateInflAgainstCorpus(corpus, word, lang);
    }
  }
}

/* LexEntryJuxtapose */

/* todo: Returned answers should have a question mark? Then follow up with
 * user as to which one is meant. Or disambiguate using discourse context.
 */
Answer *LexEntryJuxtapose_ObjHypoth(Obj *q, Obj *obj1, Obj *obj2, Answer *an)
{
  ObjList	*answer;
  if (ISA(N("attribute"), obj1) || ISA(N("action"), obj1)) {
    if (DbRestrictValidate1(NULL, obj1, 1, obj2, 0, DBGBAD)) {
    /* <obj1>:N("red") <obj2>:N("ball")
     * <obj1>:N("spin") <obj2>:N("dryer")
     */
      answer = ObjListCreate(L(obj1, obj2, E), NULL);
      an = AnswerCreate(N("LexEntryJuxtapose_ObjHypoth"), q, NULL,
                        SENSE_SOME, answer, 0, 0, 0, NULL, an);
    }
  }
#ifdef notdef
  /* todo: relation [(mother-of) Mary Tim] [(on) calculator table]
   * There may be too many of these? But that's daydreaming for you.
   * Try all possibilities.
   */
  for (rel = Objs; rel; rel = rel->next) {
    if (!ISA(N("relation"), rel)) continue;
    if ((1 == DbRestrictValidate1(NULL, rel, 1, obj1, 0, DBGBAD)) &&
        (1 == DbRestrictValidate1(NULL, rel, 2, obj2, 0, DBGBAD))) {
      answer = ObjListCreate(L(rel, obj1, obj2, E), NULL);
      an = AnswerCreate(q, NULL, NULL, answer, N("short-answer"), 0, 0, 0,
                        NULL, an);
    }
  }
#endif
  return(an);
}

Answer *LexEntryJuxtapose_Obj(Obj *obj1, Obj *obj2, Answer *an)
{
  ObjList	*answer;
  Obj		*q;
  q = ObjMakeAnd(obj1, obj2);
  /* Find known assertions involving <obj1> and <obj2> */
  if ((answer = DbRetrieveInvolving2AncDesc(&TsNA, NULL, obj1, obj2, NULL))) {
    an = AnswerCreate(N("LexEntryJuxtapose_Obj"), q, NULL, SENSE_MOSTLY,
                      answer, 0, 0, 0, NULL, an);
  }
  if (ISAP(obj1, obj2)) {
    answer = ObjListCreate(ObjISA(obj1, obj2), NULL);
    an = AnswerCreate(N("LexEntryJuxtapose_Obj"), q, NULL, SENSE_MOSTLY,
                      answer, 0, 0, 0, NULL, an);
  }
  if (ISAP(obj2, obj1)) {
    answer = ObjListCreate(ObjISA(obj2, obj1), NULL);
    an = AnswerCreate(N("LexEntryJuxtapose_Obj"), q, NULL, SENSE_MOSTLY,
                      answer, 0, 0, 0, NULL, an);
  }
  /* Hypothesize possible assertions involving <obj1> and <obj2> */
  an = LexEntryJuxtapose_ObjHypoth(q, obj1, obj2, an);
  an = LexEntryJuxtapose_ObjHypoth(q, obj2, obj1, an);
  /* todo: find objects described
   * blue ball => Earth
   * red planet => Mars
   */
  return(an);
}

ObjList *LexEntryToConcepts(char *phrase, HashTable *ht)
{
  int			freeme;
  ObjList		*r;
  IndexEntry	*ie, *origie;
  LexEntryToObj	*leo;
  r = NULL;
  origie = LexEntryFindPhrase(ht, phrase, INTPOSINF, 0, 0, &freeme);
  for (ie = origie; ie; ie = ie->next) {
    if (ie->lexentry == NULL) continue;
    for (leo = ie->lexentry->leo; leo; leo = leo->next) {
      if (!ObjListIn(leo->obj, r)) r = ObjListCreate(leo->obj, r);
    }
  }
  if (freeme) IndexEntryFree(origie);
  return(r);
}

Answer *LexEntryJuxtapose_Phrase(char *phrase1, char *phrase2, Answer *an)
{
  long		len1, len2;
  ObjList	*objs1, *objs2, *p, *q;
  objs1 = LexEntryToConcepts(phrase1, EnglishIndex);
  objs2 = LexEntryToConcepts(phrase2, EnglishIndex);
  len1 = ObjListLen(objs1);
  len2 = ObjListLen(objs2);
  Dbg(DBGUA, DBGOK, "<%s> <%s> %ld x %ld = %ld possibilities", phrase1, phrase2,
      len1, len2, len1*len2);
  for (p = objs1; p; p = p->next) {
    for (q = objs2; q; q = q->next) {
      an = LexEntryJuxtapose_Obj(p->obj, q->obj, an);
    }
  }
  ObjListFree(objs1);
  ObjListFree(objs2);
  return(an);
}

void LexEntryJuxtapose(FILE *in, FILE *out, FILE *err)
{
  char		phrase1[PHRASELEN], phrase2[PHRASELEN];
  Answer	*an;
  Ts		ts;

  TsSetNow(&ts);
  DiscourseContextInit(StdDiscourse, &ts);

  fprintf(out, "[LexEntryJuxtapose]\n");
  while (1) {
    if (!StreamAsk(in, out, "Enter word or phrase 1: ", 
        PHRASELEN, phrase1)) {
      return;
    }
    if (!StreamAsk(in, out, "Enter word or phrase 2: ", 
        PHRASELEN, phrase2)) {
      return;
    }
    AnswerGenInit();
    an = LexEntryJuxtapose_Phrase(phrase1, phrase2, NULL);
    AnswerGenAll(an, StdDiscourse);
    AnswerGenEnd();
  }
}

/* UTILITIES */

Bool LexEntryIsFrenchVowel(int c)
{
  return(StringIn(c, "aàâäeéèêiîïoòôöuü"));
}

Bool LexEntryIsFrenchVowel1(int c)
{
  return(StringIn(c, "aàâäeéèêiîïoòôöuüÀAÉEI‹OÒUÜ"));
}

int LexEntryFrenchInitialSound(int c)
{
  if (StringIn(c, "aàâäeéèêhiîïoòôöuüÀAÉEHI‹OÒUÜ")) {
    return(F_VOCALIC);
  } else {
    return(F_ASPIRE);
  }
}

Bool LexEntryIsEnglishVowel(int c)
{
  return(StringIn(c, "aeiouy"));
}

int LexEntryEnglishInitialSound(int c)
{
  if (StringIn(c, "aeiouAEIOU")) {
    return(F_VOCALIC);
  } else {
    return(F_ASPIRE);
  }
}

Obj *LexEntrySuperlativeClass(char *word, int lang)
{
  if (lang == F_FRENCH) {
    if (streq(word, "plus")) return(N("maximal"));
    if (streq(word, "moins")) return(N("minimal"));
  } else if (lang == F_ENGLISH) {
    if (streq(word, "most")) return(N("maximal"));
    if (streq(word, "least")) return(N("minimal"));
  }
  return(NULL);
}

LexEntry *LexEntrySuperlativeAdverb(Obj *class, Discourse *dc)
{
  if (class == N("maximal") || class == N("minimal")) {
    return(ObjToLexEntryGet(class, F_ADVERB, F_NULL, dc));
  } else return(NULL);
}

Bool LexEntryIsPreposedAdj(char *usagefeat, int pos, Discourse *dc)
{
  if (pos == F_DETERMINER) return(1);
  if (DC(dc).lang == F_ENGLISH) {
    if (StringIn(F_PREPOSED_ADJ, usagefeat)) return(0);
    else return(1);
  } else {
    if (StringIn(F_PREPOSED_ADJ, usagefeat)) return(1);
    else return(0);
  }
}

Bool LexEntryIsEnglishInvertibleVerb(LexEntry *le, char *features)
{
  return(StringIn(F_MODAL, features) ||
         streq(le->srcphrase, "be") ||
         streq(le->srcphrase, "do") ||
         streq(le->srcphrase, "have"));
}

Bool LexEntryWordIs(LexEntry *le, char *srcphrase, int pos, int lang)
{
  return(le && streq(srcphrase, le->srcphrase) &&
         pos == FeatureGet(le->features, FT_POS) &&
         lang == FeatureGet(le->features, FT_LANG));
}

int LexEntryHasAnyExpl(LexEntry *le)
{
  ThetaRole *theta_roles;
  for (theta_roles = le->leo->theta_roles;
       theta_roles;
       theta_roles = theta_roles->next) {
    if (theta_roles->cas == N("expl")) return 1;
  }
  return 0;
}

/* End of file. */

