/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940906: begun
 * 19940907: more work
 * 19940915: added word lists
 * 19950531: moved output into TT_HTML directory
 *
 * todo:
 * - Find more matches from elimination of accents.
 * - Find more matches from similarly but not identically spelled words.
 * - Nice conjugation printouts.
 */

#include "tt.h"
#include "toolrpt.h"
#include "lexanam.h"
#include "lexentry.h"
#include "lexmorph.h"
#include "lexobjle.h"
#include "lexwf1.h"
#include "lexwf3.h"
#include "repbasic.h"
#include "repchan.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptext.h"
#include "reptime.h"
#include "semdisc.h"
#include "semgen1.h"
#include "semgen2.h"
#include "synpnode.h"
#include "taname.h"
#include "toolstat.h"
#include "utildbg.h"
#include "utilrpt.h"

/* INITIALIZATION */

HashTable *ReportPhraseDeriv;

void ReportInit()
{
  if (!SaveTime) {
    ReportPhraseDeriv = HashTableCreate(521L);
  }
  ReportAnagramInit();
}

/* WORD SEQUENCE */

/* todo: Take into account ranges when processing attributes.
 *       There seem to be repeats, and missing finds such as
 *       cookie->biscuit->scone.
 */

ObjList	*ReportFauxAmiObjs;

void ReportFauxAmiChain3(Text *text, char *source, char *source_feat,
                         Obj *concept, int source_lang, int source_dialect,
                         char *target, char *target_feat, int target_lang,
                         int target_dialect, Discourse *dc)
{
  char	buf[PHRASELEN], *s;
  IndentPrintText(text, " ");
  TextPuts(source, text);
  TextPutc(SPACE, text);
  s = GenFeatAbbrevString(FeatureGet(source_feat, FT_POS), 1, dc);
  TextPuts(s, text);
  GenConceptString(concept, N("empty-article"), F_NOUN, F_NULL, DC(dc).lang,
                   F_NULL, F_NULL, F_NULL, PHRASELEN, 1, 1, dc, buf+2);
  if (buf[2] && 0 != strncmp(buf+2, source, 3)) {
    buf[0] = SPACE;
    buf[1] = LPAREN;
    StringAppendChar(buf, PHRASELEN, RPAREN);
    StringAppendChar(buf, PHRASELEN, SPACE);
    TextPuts(buf, text);
  } else {
    TextPutc(SPACE, text);
  }
  s = GenFeatAbbrevString((uc)source_lang, 1, dc);
  TextPuts(s, text);
  TextPutc(SPACE, text);
  if (source_dialect != F_NULL) {
    s = GenFeatAbbrevString((uc)source_dialect, 1, dc);
    TextPuts(s, text);
    TextPutc(SPACE, text);
  }
  TextPuts("= ", text);
  TextPuts(target, text);
  TextPutc(SPACE, text);
  s = GenFeatAbbrevString(FeatureGet(target_feat, FT_POS), 1, dc);
  TextPuts(s, text);
  TextPutc(SPACE, text);
  s = GenFeatAbbrevString((uc)target_lang, 1, dc);
  TextPuts(s, text);
  TextPutc(SPACE, text);
  if (target_dialect != F_NULL) {
    s = GenFeatAbbrevString((uc)target_dialect, 1, dc);
    TextPuts(s, text);
    TextPutc(SPACE, text);
  }
  TextPutc(NEWLINE, text);
}

void ReportFauxAmiChain2(int paruniv, Text *text, LexEntry *source_le,
                         Obj *obj, int ignore_infrequent, int source_lang,
                         int target_lang, HashTable *source_ht,
                         int source_dialect, int target_dialect, int depth,
                         LexEntry *depth1_source_le,
                         LexEntry *depth1_target_le, Obj *depth1_obj,
                         Discourse *dc)
{
  ObjToLexEntry	*ole;
  LexEntryToObj	*leo;
  LexEntry	*target_le, *source_le2, *prev_le;
  IndexEntry	*ie;
  char		*prev_srcphrase;
  if (depth >= 10) {
    return;
  }
  if (ObjListIn(obj, ReportFauxAmiObjs)) return;
  ReportFauxAmiObjs = ObjListCreate(obj, ReportFauxAmiObjs);
  IndentUp();
  /* obj = a meaning of the word COOKIE in the source language/dialect = OREO */
  for (ole = obj->ole; ole; ole = ole->next) {
    if (ole->le == NULL) continue;
    if (ole->le == source_le) continue;
    target_le = ole->le;
    if (target_lang != FeatureGet(target_le->features, FT_LANG)) continue;
    if (FeatureGet(ole->features, FT_DIALECT) != target_dialect) continue;
    if (streq(source_le->srcphrase, target_le->srcphrase)) continue;
    /* target_le = a word BISCUIT in the target language/dialect for the
     * meaning OREO.
     */
    prev_le = NULL;
    prev_srcphrase = NULL;
    /*
    if (streq(target_le->srcphrase, "country")) Stop();
     */
    for (ie = IndexEntryGet(target_le->srcphrase, source_ht);
         ie;
         ie = ie->next) {
      source_le2 = ie->lexentry;
      if (source_le2 == prev_le) continue;
      if (source_le2->srcphrase == prev_srcphrase) {
      /* Equal because of interning. */
        continue;
      }
      prev_le = source_le2;
      prev_srcphrase = source_le2->srcphrase;
      if (source_lang != FeatureGet(source_le2->features, FT_LANG)) continue;
      if (!MorphIsWord(source_le2->srcphrase)) continue;
      /* source_le2 = the word BISCUIT in the source language */
      for (leo = source_le2->leo; leo; leo = leo->next) {
        if (leo->obj == obj) continue;
        if (ignore_infrequent && StringIn(F_INFREQUENT, leo->features)) {
          continue;
        }
        if (FeatureGet(leo->features, FT_DIALECT) != source_dialect) continue;
        if (paruniv != FeatureGet(leo->features, FT_PARUNIV)) continue;
/*
        if (ObjListIn(leo->obj, ReportFauxAmiObjs)) continue;
*/
        /* leo->obj = meaning of word BISCUIT in source language
         * (CRACKER) != OREO.
         */
        if (depth == 1) {
          IndentDown();
          ReportFauxAmiChain3(text, depth1_source_le->srcphrase,
                                  depth1_source_le->features, depth1_obj,
                                  source_lang, source_dialect,
                                  depth1_target_le->srcphrase,
                                  depth1_target_le->features, target_lang,
                                  target_dialect, dc);
          IndentUp();
        }
        if (depth > 0) {
          ReportFauxAmiChain3(text, source_le->srcphrase, source_le->features,
                              obj, source_lang, source_dialect,
                              target_le->srcphrase, target_le->features,
                              target_lang, target_dialect, dc);
        }
        ReportFauxAmiChain2(paruniv, text, source_le2, leo->obj,
                            ignore_infrequent, source_lang, target_lang,
                            source_ht, source_dialect, target_dialect,
                            depth+1, source_le, target_le, obj, dc);
                                                    
      }
    }
  }
  IndentDown();
}

/* Must set ReportFauxAmiObjs = NULL and do IndentInit() before calling this.
 * Then ObjListFree(ReportFauxAmiObjs) afterwards.
 */
void ReportFauxAmiChain1(Text *text, LexEntry *source_le, int ignore_infrequent,
                         int source_lang, int target_lang, HashTable *source_ht,
                         int source_dialect, int target_dialect, Discourse *dc)
{
  LexEntryToObj	*leo;
  if (!StringIn(source_lang, dc->langs)) return;
  if (!StringIn(target_lang, dc->langs)) return;
  if ((source_dialect != F_NULL) &&
      (!StringIn(source_dialect, dc->dialects))) return;
  if ((target_dialect != F_NULL) &&
      (!StringIn(target_dialect, dc->dialects))) return;
  if (source_lang != FeatureGet(source_le->features, FT_LANG)) return;
  for (leo = source_le->leo; leo; leo = leo->next) {
    if (ignore_infrequent && StringIn(F_INFREQUENT, leo->features)) continue;
    if (FeatureGet(leo->features, FT_DIALECT) != source_dialect) {
      continue;
    }
    if (leo->obj == NULL) continue;
    ReportFauxAmiChain2(FeatureGet(leo->features, FT_PARUNIV),
                       text, source_le, leo->obj, ignore_infrequent,
                       source_lang, target_lang, source_ht, source_dialect,
                       target_dialect, 0, NULL, NULL, NULL, dc);
  }
}

void ReportFauxAmi(Text *text, Discourse *dc)
{
  LexEntry		*le;
  ReportFauxAmiObjs = NULL;
  for (le = AllLexEntries; le; le = le->next) {
    if (!MorphIsWord(le->srcphrase)) continue;
    IndentInit();
    ReportFauxAmiChain1(text, le, 1, F_ENGLISH, F_FRENCH, EnglishIndex,
                       F_NULL, F_NULL, dc);
    IndentInit();
    ReportFauxAmiChain1(text, le, 1, F_FRENCH, F_ENGLISH, FrenchIndex,
                       F_NULL, F_NULL, dc);
    IndentInit();
    ReportFauxAmiChain1(text, le, 1, F_ENGLISH, F_ENGLISH, EnglishIndex,
                       F_AMERICAN, F_BRITISH, dc);
  }
  ObjListFree(ReportFauxAmiObjs);
}

/* FEATURES */

void ReportFeatureSheetHeader(Rpt *rpt, Discourse *dc)
{
  int	save;
  char	*lang;
  RptStartLine(rpt);
  save = DC(dc).lang;
  if (dc->mode & DC_MODE_PROGRAMMER) {
    RptAdd(rpt, "", RPT_JUST_LEFT);
  }
  for (lang = dc->langs; *lang; lang++) {
    DiscourseSetLang(dc, *((uc *)lang));
    RptAddFeatAbbrev(rpt, *((uc *)lang), 0, dc);
  }
  for (lang = dc->langs; *lang; lang++) {
    DiscourseSetLang(dc, *((uc *)lang));
    RptAddConcept(rpt, N("feature"), dc);
    RptAddConcept(rpt, N("part-of-speech"), dc);
  }
  DiscourseSetLang(dc, save);
}

void ReportFeatureSheetLine(Rpt *rpt, int feature, Discourse *dc)
{
  int	save;
  char	*lang;
  char	buf1[PHRASELEN], buf2[PHRASELEN];
  save = DC(dc).lang;
  RptStartLine(rpt);
  if (dc->mode & DC_MODE_PROGRAMMER) {
    buf1[0] = feature;
    buf1[1] = TERM;
    RptAdd(rpt, buf1, RPT_JUST_LEFT);
  }
  for (lang = dc->langs; *lang; lang++) {
    DiscourseSetLang(dc, *((uc *)lang));
    RptAddFeatAbbrev(rpt, (uc)feature, 0, dc);
  }
  DiscourseSetLang(dc, save);
  for (lang = dc->langs; *lang; lang++) {
    GenFeatureName(feature, *((uc *)lang), PHRASELEN, dc, buf1, buf2);
    RptAdd(rpt, buf1, RPT_JUST_RIGHT);
    RptAdd(rpt, buf2, RPT_JUST_LEFT);
  }
}

void ReportFeature(Text *text, Discourse *dc)
{
  int	i;
  char	*all;
  Rpt	*rpt;
  Obj	*obj;
  /* Do a little consistency check first. */
  for (i = 1; i < 256; i++) {
    if ((!StringIn(i, FT_ALL)) && (!StringIn(i, FT_CONSTIT))) {
      if ((obj = FeatToCon1(i))) {
        Dbg(DBGLEX, DBGBAD, "<%s> in database file not in FT_ALL/FT_CONSTIT",
            M(obj));
      }
    }
  }
  rpt = RptCreate();
  all = FT_ALL;
  while (*all) {
    ReportFeatureSheetLine(rpt, *((uc *)all), dc);
    all++;
  }
  ReportFeatureSheetHeader(rpt, dc);
  RptTextPrint(text, rpt, 1, 0, 1, 17);
  RptFree(rpt);
}

/* PHRASE DERIVATION */

Discourse	*ReportPhraseDerivationDc;
Rpt		*ReportPhraseDerivationRpt;

void ReportAddPhraseDerive(char *deriv)
{
  long	cnt;
  if (!SaveTime) {
    cnt = (long)HashTableGet(ReportPhraseDeriv, deriv);
    cnt++;
    HashTableSetDup(ReportPhraseDeriv, deriv, (void *)cnt);
  }
}

void ReportPhraseDerivationHeader(Rpt *rpt, Discourse *dc)
{
  RptStartLine(rpt);
  RptAddConcept(rpt, N("language"), dc);
  RptAdd(rpt, "", RPT_JUST_LEFT);
  RptAddConcept(rpt, N("quantity"), dc);
  RptAddConcept(rpt, N("derivation"), dc);
}

void ReportPhraseDerivationLine(char *deriv, long value)
{
  if (value < 30) return;
  RptStartLine(ReportPhraseDerivationRpt);
  RptAddFeatAbbrev(ReportPhraseDerivationRpt, ((uc *)deriv)[0],
                   1, ReportPhraseDerivationDc);
  RptAddFeatAbbrev(ReportPhraseDerivationRpt, ((uc *)deriv)[1],
                   1, ReportPhraseDerivationDc);
  RptAddLong(ReportPhraseDerivationRpt, value, ReportPhraseDerivationDc);
  RptAddFeaturesAbbrev(ReportPhraseDerivationRpt, deriv+2, 1, "",
                       ReportPhraseDerivationDc);
}

void ReportPhraseDerivation(Text *text, Discourse *dc)
{
  if (!SaveTime) {
    ReportPhraseDerivationDc = dc;
    ReportPhraseDerivationRpt= RptCreate();
    HashTableForeach(ReportPhraseDeriv, ReportPhraseDerivationLine);
    ReportPhraseDerivationHeader(ReportPhraseDerivationRpt, dc);
    RptTextPrint(text, ReportPhraseDerivationRpt, 1, 2, -1, 20);
    RptFree(ReportPhraseDerivationRpt);
  }
}

/* WORD LIST */

void ReportWordListAdd(StringArray *sa, char *match_features,
                       int palindrome_only, char *word, char *word_features)
{
  if (StringAllIn(match_features, word_features) &&
      ((!palindrome_only) || StringIsPalindrome(word))) {
    StringArrayAdd(sa, word, 0);
  }
}

void ReportWordList(FILE *stream_regular, FILE *stream_inverse, Obj *class,
                    char *features, int all_inflections, int palindrome_only,
                    int phrases_ok)
{
  LexEntry		*le;
  Word			*word;
  StringArray	*sa;
  sa = StringArrayCreate();
  for (le = AllLexEntries; le; le = le->next) {
    if ((!phrases_ok) && LexEntryIsPhrase(le)) continue;
    if (class && (!LexEntryConceptIsAncestor(class, le))) continue;
    if (all_inflections) {
      for (word = le->infl; word; word = word->next) {
        ReportWordListAdd(sa, features, palindrome_only, word->word,
                          word->features);
      }
    } else {
      if (!le->infl) {
        Dbg(DBGLEX, DBGBAD, "ReportWordList: empty le->infl");
      } else {
        ReportWordListAdd(sa, features, palindrome_only, le->infl->word,
                          le->features);
      }
    }
  }
  if (stream_regular) {
    StringArraySort(sa);
    StringArrayPrint(stream_regular, sa, 1, 1);
  }
  if (stream_inverse) {
    StringArrayInverseSort(sa);
    StringArrayPrint(stream_inverse, sa, 1, 1);
  }
  StringArrayFree(sa);
}

/* ANAGRAMS */

HashTable *ReportAnagramHt;

void ReportAnagramInit()
{
  ReportAnagramHt = NULL;
}

Bool ReportAnagramIn(char *word, AnagramClass *ac)
{
  for (; ac; ac = ac->next) {
    if (0 == french_strcmp(word, ac->word)) return(1);
  }
  return(0);
}

void ReportAnagramTrain1(LexEntry *le)
{
  Word			*word;
  AnagramClass	*acprev, *ac;
  char	buf[PHRASELEN];
  for (word = le->infl; word; word = word->next) {
    StringAnagramUniquify(word->word, PHRASELEN, buf);
    acprev = (AnagramClass *)HashTableGet(ReportAnagramHt, buf);
    if (!ReportAnagramIn(word->word, acprev)) {
      ac = CREATE(AnagramClass);
      ac->word = word->word;
      ac->features = word->features;
      ac->next = acprev;
      HashTableSetDup(ReportAnagramHt, buf, ac);
    }
  }
}

void ReportAnagramTrain()
{
  LexEntry		*le;
  Dbg(DBGLEX, DBGDETAIL, "ReportAnagramTrain begin");
  ReportAnagramHt = HashTableCreate(5021L);
  for (le = AllLexEntries; le; le = le->next) {
    if (LexEntryIsPhrase(le)) continue;
    ReportAnagramTrain1(le);
  }
  Dbg(DBGLEX, DBGDETAIL, "ReportAnagramTrain end");
}

AnagramClass *ReportGetAnagrams(char *word)
{
  char	buf[PHRASELEN];
  if (!ReportAnagramHt) {
    return(NULL);
#ifdef notdef
    ReportAnagramTrain();
#endif
  }
  StringAnagramUniquify(word, PHRASELEN, buf);
  return((AnagramClass *)HashTableGet(ReportAnagramHt, buf));
}

void ReportAnagramsOf1(Text *text, AnagramClass *ac, char *except,
                       Discourse *dc)
{
  for (; ac; ac = ac->next) {
    if (streq(except, ac->word)) continue;
    TextPrintWordAndFeatures(text, 0, DC(dc).lang, ac->word, ac->features,
                             NULL, 1, ';', FS_CHECKING, dc);
  }
  TextPutc(NEWLINE, text);
}

void ReportAnagramsOf(Text *text, char *word, Discourse *dc)
{
  AnagramClass *ac;
  ac = ReportGetAnagrams(word);
  if (ac && ac->next) {
    TextPrintConcept(text, N("anagram-of"), F_NOUN, F_NULL, F_NULL, F_PLURAL,
                     F_NULL, ':', 0, 0, 0, dc);
    ReportAnagramsOf1(text, ac, word, dc);
  }
}

Text		*ReportAnagramsText;
Discourse	*ReportAnagramsDiscourse;

void ReportAnagrams1(char *canon, AnagramClass *ac)
{
  if (ac && ac->next) {
    ReportAnagramsOf1(ReportAnagramsText, ac, "", ReportAnagramsDiscourse);
  }
}

void ReportAnagram(Text *text, Discourse *dc)
{
  if (!ReportAnagramHt) {
    ReportAnagramTrain();
  }
  ReportAnagramsText = text;
  ReportAnagramsDiscourse = dc;
  HashTableForeach(ReportAnagramHt, ReportAnagrams1);
  TextPutc(NEWLINE, text);
}

/* MENU */

void Tool_Report()
{
  char	buf[WORDLEN], fn[FILENAMELEN];
  Text	*text;
  FILE	*stream1, *stream2;
  printf("[Report]\n");
  printf("anag...anagram\n");
  printf("anam...analogical morphology\n");
  printf("class..classifications\n");
  printf("faux...faux ami\n");
  printf("feat...feature\n");
  printf("i......inflection\n");
  printf("n......name\n");
  printf("pali...palindrome\n");
  printf("phra...phrase derivation\n");
  printf("song...songs\n");
  printf("stat...stats\n");
  printf("wrd1...word formation 1\n");
  printf("wrd3...word formation 3\n");
  while (StreamAskStd("Enter [Report] choice: ", WORDLEN, buf)) {
    text = NULL;
    if (streq(buf, "wrd3")) {
      sprintf(fn, "%s/rptwrd3.txt", TT_Report_Dir);
      text = TextCreat(StdDiscourse);
      ReportAffix(text, StdDiscourse);
    } else if (streq(buf, "anag")) {
      sprintf(fn, "%s/rptanag.txt", TT_Report_Dir);
      text = TextCreat(StdDiscourse);
      ReportAnagram(text, StdDiscourse);
    } else if (streq(buf, "anam")) {
      sprintf(fn, "%s/rptanam.txt", TT_Report_Dir);
      text = TextCreat(StdDiscourse);
      ReportAnaMorph(text, StdDiscourse);
    } else if (streq(buf, "faux")) {
      sprintf(fn, "%s/rptfaux.txt", TT_Report_Dir);
      text = TextCreat(StdDiscourse);
      ReportFauxAmi(text, StdDiscourse);
    } else if (streq(buf, "feat")) {
      sprintf(fn, "%s/rptfeat.txt", TT_Report_Dir);
      text = TextCreat(StdDiscourse);
      ReportFeature(text, StdDiscourse);
    } else if (streq(buf, "i")) {
      sprintf(fn, "%s/rpteinfl.txt", TT_Report_Dir);
      if (NULL == (stream1 = StreamOpen(fn, "w+"))) stream1 = stdout;
      sprintf(fn, "%s/rpteinfi.txt", TT_Report_Dir);
      if (NULL == (stream2 = StreamOpen(fn, "w+"))) stream2 = stdout;
      ReportWordList(stream1, stream2, NULL, "z", 1, 0, 1);
      StreamClose(stream1);
      StreamClose(stream2);

      sprintf(fn, "%s/rptfinfl.txt", TT_Report_Dir);
      if (NULL == (stream1 = StreamOpen(fn, "w+"))) stream1 = stdout;
      sprintf(fn, "%s/rptfinfi.txt", TT_Report_Dir);
      if (NULL == (stream2 = StreamOpen(fn, "w+"))) stream2 = stdout;
      ReportWordList(stream1, stream2, NULL, "y", 1, 0, 1);
      StreamClose(stream1);
      StreamClose(stream2);
    } else if (streq(buf, "n")) {
      sprintf(fn, "%s/rptname.txt", TT_Report_Dir);
      if (NULL == (stream1 = StreamOpen(fn, "w+"))) stream1 = stdout;
      sprintf(fn, "%s/rptnamei.txt", TT_Report_Dir);
      if (NULL == (stream2 = StreamOpen(fn, "w+"))) stream2 = stdout;
      ReportWordList(stream1, stream2, N("human-name"), "", 1, 0, 1);
      StreamClose(stream1);
      StreamClose(stream2);
    } else if (streq(buf, "pali")) {
      sprintf(fn, "%s/rptepali.txt", TT_Report_Dir);
      if (NULL == (stream1 = StreamOpen(fn, "w+"))) stream1 = stdout;
      ReportWordList(stream1, NULL, NULL, "z", 1, 1, 1);
      StreamClose(stream1);

      sprintf(fn, "%s/rptfpali.txt", TT_Report_Dir);
      if (NULL == (stream1 = StreamOpen(fn, "w+"))) stream1 = stdout;
      ReportWordList(stream1, NULL, NULL, "y", 1, 1, 1);
      StreamClose(stream1);
    } else if (streq(buf, "phra")) {
      sprintf(fn, "%s/rptphra.txt", TT_Report_Dir);
      text = TextCreat(StdDiscourse);
      ReportPhraseDerivation(text, StdDiscourse);
    } else if (streq(buf, "stat")) {
      sprintf(fn, "%s/rptstat.txt", TT_Report_Dir);
      if (NULL == (stream1 = StreamOpen(fn, "w+"))) stream1 = Log;
      ReportStatistics(stream1);
      StreamClose(stream1);
    } else if (streq(buf, "class")) {
      sprintf(fn, "%s/rptclass.txt", TT_Report_Dir);
      if (NULL == (stream1 = StreamOpen(fn, "w+"))) stream1 = Log;
      StatClassify(stream1);
      StreamClose(stream1);
    } else if (streq(buf, "wrd1")) {
      sprintf(fn, "%s/rptwrd1.txt", TT_Report_Dir);
      text = TextCreat(StdDiscourse);
      ReportWordForm(text, StdDiscourse);
    } else {
      printf("What?\n");
    }
    if (text) {
      if (NULL == (stream1 = StreamOpen(fn, "w+"))) stream1 = Log;
      TextPrint(stream1, text);
      TextFree(text);
      StreamClose(stream1);
    }
  }
}

/* End of file. */
