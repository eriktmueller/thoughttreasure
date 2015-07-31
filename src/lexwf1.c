/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940907: begun
 * 19940908: completed
 *
 * This attempts to find new instances of N("suffix-synonym") which
 * could be used by Lex_WordForm2. This isn't really needed, as most
 * such suffixes should already be defined in the database.
 *
 * todo:
 * - After training, could free all instances less than WORDFORM_COUNT_THRESH.
 */

#include "tt.h"
#include "lexentry.h"
#include "lexmorph.h"
#include "lexwf1.h"
#include "repbasic.h"
#include "repobj.h"
#include "repstr.h"
#include "utildbg.h"
#include "utilrpt.h"

/* BASICS */

#define WORDFORM_MINSTEM	3
#define WORDFORM_COUNT_THRESH	20

HashTable	*WordFormHt;
Bool		WordFormTrained;

void WordFormInit()
{
  WordFormHt = HashTableCreate(1009L);	/* todo: Increase size? */
  WordFormTrained = 0;
}

void WordFormRuleAdd(char *suffix1, char *lefeat1, char *linkfeat1,
                     char *suffix2, char *lefeat2, char *linkfeat2,
                     LexEntry *le1, LexEntry *le2)                     
{
  char	index[PHRASELEN];
  WordFormRule	*wfr;
  wfr = CREATE(WordFormRule);
  wfr->count = 1;
  wfr->suffix1 = suffix1;
  wfr->lefeat1 = lefeat1;
  wfr->linkfeat1 = linkfeat1;
  wfr->suffix2 = suffix2;
  wfr->lefeat2 = lefeat2;
  wfr->linkfeat2 = linkfeat2;
  StringCpy(index, suffix1, PHRASELEN);
  StringCat(index, lefeat1, PHRASELEN);
  wfr->example1 = le1;
  wfr->example2 = le2;
  wfr->next = (WordFormRule *)HashTableGet(WordFormHt, index);
  HashTableSetDup(WordFormHt, index, wfr);
}

WordFormRule *WordFormGetRules(char *suffix1, char *lefeat1)
{
  char	index[PHRASELEN];
  StringCpy(index, suffix1, PHRASELEN);
  StringCat(index, lefeat1, PHRASELEN);
  return((WordFormRule *)HashTableGet(WordFormHt, index));
}

/* TRAINING */

Bool WordFormSkipLe(char *lefeat)
{
  return(StringIn(F_BORROWING, lefeat));
}

Bool WordFormSkipLink(char *linkfeat)
{
  return(StringIn(F_INFREQUENT, linkfeat));
}

void WordFormBuildLeFeat(char *in, /* RESULTS */ char *out)
{
  int	c;
  if (F_NULL != (c = FeatureGet(in, FT_GENDER))) {
    *out = c;
    out++;
  }
  *out = FeatureGet(in, FT_POS);
  out++;
  *out = FeatureGet(in, FT_LANG);
  out++;
  *out = TERM;
}

void WordFormBuildLinkFeat(char *in, /* RESULTS */ char *out)
{
  int	c;
  if (F_NULL != (c = FeatureGet(in, FT_PARUNIV))) {
    *out = c;
    out++;
  }
  *out = TERM;
}

Bool WordFormLinkIsCompat(char *f1, char *f2)
{
  return(FeatureFtMatch(f1, f2, FT_PARUNIV)); /* Was FT_GRAMMAR. */
}

void WordFormTrain3(char *suffix1, char *lefeat1, char *linkfeat1,
                    char *suffix2, char *lefeat2, char *linkfeat2,
                    LexEntry *le1, LexEntry *le2)
{
  WordFormRule	*wfr;
  HashTable	*ht1, *ht2;
  ht1 = LexEntryLangHt(lefeat1);
  ht2 = LexEntryLangHt(lefeat2);
  suffix1 = HashTableIntern(ht1, suffix1);
  lefeat1 = HashTableIntern(ht1, lefeat1);
  linkfeat1 = HashTableIntern(ht1, linkfeat1);
  suffix2 = HashTableIntern(ht2, suffix2);
  lefeat2 = HashTableIntern(ht2, lefeat2);
  linkfeat2 = HashTableIntern(ht2, linkfeat2);
  if (suffix1 == suffix2 &&
      lefeat1 == lefeat2 &&
      linkfeat1 == linkfeat2) {
    return;
  }
  for (wfr = WordFormGetRules(suffix1, lefeat1); wfr; wfr = wfr->next) {
    if (suffix2 != wfr->suffix2) continue;
    if (lefeat2 != wfr->lefeat2) continue;
    if (linkfeat2 != wfr->linkfeat2) continue;
    if (linkfeat1 != wfr->linkfeat1) continue;
    wfr->count++;
    return;
  }
  WordFormRuleAdd(suffix1, lefeat1, linkfeat1, suffix2, lefeat2, linkfeat2,
                  le1, le2);
}

void WordFormTrain2(char *word1, char *lefeat1, char *linkfeat1,
                    char *word2, char *lefeat2, char *linkfeat2,
                    LexEntry *le1, LexEntry *le2)
{
  int	len1, len2, stemlen;
  len1 = strlen(word1);
  len2 = strlen(word2);
  for (stemlen = WORDFORM_MINSTEM; ; stemlen++) {
    if (stemlen > len1 || stemlen > len2) return;
    if (0 != memcmp(word1, word2, stemlen)) return;
    WordFormTrain3(word1+stemlen, lefeat1, linkfeat1,
                   word2+stemlen, lefeat2, linkfeat2,
                   le1, le2);
  }
}

void WordFormTrain1(ObjToLexEntry *ole1, ObjToLexEntry *ole2)
{
  char	lefeat1[FEATLEN], linkfeat1[FEATLEN], lefeat2[FEATLEN];
  char	linkfeat2[FEATLEN];
  WordFormBuildLeFeat(ole1->le->features, lefeat1);
  WordFormBuildLeFeat(ole2->le->features, lefeat2);
  WordFormBuildLinkFeat(ole1->features, linkfeat1);
  WordFormBuildLinkFeat(ole2->features, linkfeat2);
  WordFormTrain2(ole1->le->srcphrase, lefeat1, linkfeat1,
                 ole2->le->srcphrase, lefeat2, linkfeat2,
                 ole1->le, ole2->le);
}

void WordFormTrain()
{
  Obj	*obj;
  ObjToLexEntry	*ole1, *ole2;
  if (WordFormTrained) return;
  Dbg(DBGLEX, DBGDETAIL, "WordFormTrain starting");
  for (obj = Objs; obj; obj = obj->next) {
    for (ole1 = obj->ole; ole1; ole1 = ole1->next) {
      if (WordFormSkipLink(ole1->features)) continue;
      if (WordFormSkipLe(ole1->le->features)) continue;
      for (ole2 = obj->ole; ole2; ole2 = ole2->next) {
        if (ole1 == ole2) continue;
        if (WordFormSkipLink(ole2->features)) continue;
        if (WordFormSkipLe(ole2->le->features)) continue;
        /* keep this test consistent with WORDFORM_MINSTEM */
        if (LexEntryIsPhrase(ole1->le) || !MorphIsWord(ole1->le->srcphrase)) {
          continue;
        }
        if (LexEntryIsPhrase(ole2->le) || !MorphIsWord(ole2->le->srcphrase)) {
          continue;
        }
        if (ole1->le->srcphrase[0] == ole2->le->srcphrase[0] &&
            ole1->le->srcphrase[1] == ole2->le->srcphrase[1] &&
            ole1->le->srcphrase[2] == ole2->le->srcphrase[2]) {
          WordFormTrain1(ole1, ole2);
        }
      }
    }
  }
  WordFormTrained = 1;
  Dbg(DBGLEX, DBGDETAIL, "WordFormTrain done");
}

/* UNDERSTANDING */

Bool WordFormUnderstand1(LexEntry *le, char *suffix, int stemlen)
{
  int		maxcount;
  Obj		*maxcon;
  char		word2[PHRASELEN], feat[PHRASELEN], *maxlinkfeat1, *maxlefeat1;
  WordFormRule	*wfr, *maxwfr;
  IndexEntry	*ie;
  LexEntryToObj	*leo;

  maxcount = INTNEGINF;
  maxcon = NULL;
  maxlinkfeat1 = maxlefeat1 = NULL;
  maxwfr = NULL;
  memcpy(word2, le->srcphrase, stemlen);
  for (wfr = WordFormGetRules(suffix, le->features); wfr; wfr = wfr->next) {
    word2[stemlen] = TERM;
    StringCat(word2, wfr->suffix2, PHRASELEN);
    for (ie = IndexEntryGet(word2, LexEntryLangHt(wfr->lefeat2)); ie;
         ie = ie->next) {
      if (!FeatureCompatLexEntry(ie->lexentry->features, wfr->lefeat2)) {
        continue;
      }
      for (leo = ie->lexentry->leo; leo; leo = leo->next) {
        if (!WordFormLinkIsCompat(leo->features, wfr->linkfeat2)) continue;
        if (wfr->count > maxcount) {
          maxwfr = wfr;
          maxcount = wfr->count;
          maxcon = leo->obj;
          maxlinkfeat1 = wfr->linkfeat1;
          maxlefeat1 = wfr->lefeat1;
        }
      }
    }
  }
  if (maxcount > WORDFORM_COUNT_THRESH) {
    Dbg(DBGLEX, DBGDETAIL, "%d %s.%s%s -> %s.%s%s",
        maxwfr->count,
        maxwfr->suffix2, maxwfr->linkfeat2, maxwfr->lefeat2,
        maxwfr->suffix1, maxwfr->linkfeat1, maxwfr->lefeat1);
    Dbg(DBGLEX, DBGDETAIL, "====%s//%s.%s%s/",
        M(maxcon), le->srcphrase, maxlinkfeat1, le->features);
    StringCpy(feat, maxlinkfeat1, FEATLEN);
    StringCat(feat, maxlefeat1, FEATLEN);
    LexEntryLinkToObj(le, maxcon, feat, LexEntryLangHt(le->features), NULL,
                      0, NULL, NULL, NULL);
    return(1);
  }
  return(0);
}

Bool WordFormUnderstand(LexEntry *le)
{
  int	stemlen, len;
  if (!WordFormTrained) WordFormTrain();
  len = strlen(le->srcphrase);
  for (stemlen = WORDFORM_MINSTEM; stemlen < len; stemlen++) {
    if (WordFormUnderstand1(le, le->srcphrase+stemlen, stemlen)) {
      return(1);
    }
  }
  return(0);
}

/* GENERATION */

ObjToLexEntry *WordFormGenerate1(LexEntry *le, char *suffix1, char *linkfeat1,
                                 char *lefeat2, char *linkfeat2, int stemlen,
                                 Obj *con)
{
  int		maxcount;
  char		word2[PHRASELEN], feat[FEATLEN];
  WordFormRule	*wfr, *maxwfr;
  HashTable	*ht;
  LexEntry	*new_le;

  maxcount = INTNEGINF;
  maxwfr = NULL;
  for (wfr = WordFormGetRules(suffix1, le->features); wfr; wfr = wfr->next) {
    if (!FeatureCompatLexEntry(lefeat2, wfr->lefeat2)) continue;
    if (!WordFormLinkIsCompat(linkfeat1, wfr->linkfeat1)) continue;
    if (!WordFormLinkIsCompat(linkfeat2, wfr->linkfeat2)) continue;
    if (wfr->count > maxcount) {
      maxcount = wfr->count;
      maxwfr = wfr;
    }
  }
  if (maxwfr && (maxcount > WORDFORM_COUNT_THRESH)) {
    memcpy(word2, le->srcphrase, stemlen);
    word2[stemlen] = TERM;
    StringCat(word2, maxwfr->suffix2, PHRASELEN);

    Dbg(DBGLEX, DBGDETAIL, "rule <%d> <%s>.<%s><%s> -> <%s>.<%s><%s>",
        maxwfr->count,
        maxwfr->suffix1, maxwfr->linkfeat1, maxwfr->lefeat1,
        maxwfr->suffix2, maxwfr->linkfeat2, maxwfr->lefeat2);
    Dbg(DBGLEX, DBGDETAIL, "suggest new word ====%s//%s.%s%s/",
        M(con), word2, maxwfr->linkfeat2, maxwfr->lefeat2);

    ht = LexEntryLangHt(maxwfr->lefeat2);
    new_le = LexEntryNewWord(word2, maxwfr->lefeat2, con, ht);
    StringCpy(feat, maxwfr->linkfeat2, FEATLEN);
    StringCat(feat, maxwfr->lefeat2, FEATLEN);
    return(LexEntryLinkToObj(new_le, con, feat, ht, NULL, 0, NULL, NULL, NULL));
  }
  return(NULL);
}

/* Generate lexical gaps. */
ObjToLexEntry *WordFormGenerate(Obj *con, char *lefeat2, char *linkfeat2)
{
  int			len, stemlen;
  ObjToLexEntry	*ole, *r;
  for (ole = con->ole; ole; ole = ole->next) {
    len = strlen(ole->le->srcphrase);
    for (stemlen = WORDFORM_MINSTEM; stemlen < len; stemlen++) {
      if ((r = WordFormGenerate1(ole->le, ole->le->srcphrase+stemlen,
                                 ole->features, lefeat2, linkfeat2, stemlen,
                                 con))) {
        return(r);
      }
    }
  }
  return(NULL);
}

/* FILLING IN */

void WordFormFillin()
{
  LexEntry	*le;
  Dbg(DBGLEX, DBGDETAIL, "WordFormFillin starting");
  for (le = AllLexEntries; le; le = le->next) {
    if (!le->leo) WordFormUnderstand(le);
  }
/*
  for (obj = Objs; obj; obj = obj->next) {
    for each missing LEFEAT and LINKFEAT, do:
    WordFormGenerate(obj, lefeat, linkfeat);
  }
*/
  Dbg(DBGLEX, DBGDETAIL, "WordFormFillin done");
}

/* REPORT */

Discourse *ReportWordFormDc;
Rpt	*ReportWordFormRpt;

void ReportWordFormHeader(Rpt *rpt, Discourse *dc)
{
  RptStartLine(rpt);
  RptAddConcept(rpt, N("quantity"), dc);
  RptAddConcept(rpt, N("suffix"), dc);
  RptAdd(rpt, "", RPT_JUST_LEFT);
  RptAdd(rpt, "", RPT_JUST_LEFT);
  RptAddConcept(rpt, N("suffix"), dc);
  RptAdd(rpt, "", RPT_JUST_LEFT);
  RptAdd(rpt, "", RPT_JUST_LEFT);
  RptAddConcept(rpt, N("example"), dc);
  RptAddConcept(rpt, N("example"), dc);
}

void ReportWordFormLine1(char *index, WordFormRule *wfr)
{
  if (wfr->count <= WORDFORM_COUNT_THRESH) return;
  RptStartLine(ReportWordFormRpt);
  RptAddInt(ReportWordFormRpt, wfr->count, ReportWordFormDc);
  RptAdd(ReportWordFormRpt, wfr->suffix1, RPT_JUST_LEFT);
  RptAddFeaturesAbbrev(ReportWordFormRpt, wfr->lefeat1, 1, "",
                       ReportWordFormDc);
  RptAddFeaturesAbbrev(ReportWordFormRpt, wfr->linkfeat1, 1, "",
                       ReportWordFormDc);
  RptAdd(ReportWordFormRpt, wfr->suffix2, RPT_JUST_LEFT);
  RptAddFeaturesAbbrev(ReportWordFormRpt, wfr->lefeat2, 1, "",
                       ReportWordFormDc);
  RptAddFeaturesAbbrev(ReportWordFormRpt, wfr->linkfeat2, 1, "",
                       ReportWordFormDc);
  RptAdd(ReportWordFormRpt, wfr->example1->srcphrase, RPT_JUST_LEFT);
  RptAdd(ReportWordFormRpt, wfr->example2->srcphrase, RPT_JUST_LEFT);
}

void ReportWordFormLine(char *index, WordFormRule *wfr)
{
  for (; wfr; wfr = wfr->next) {
    ReportWordFormLine1(index, wfr);
  }
}

void ReportWordForm(Text *text, Discourse *dc)
{
  if (WordFormTrained == 0) {
    WordFormTrain();
  }
  ReportWordFormDc = dc;
  ReportWordFormRpt= RptCreate();
  HashTableForeach(WordFormHt, ReportWordFormLine);
  ReportWordFormHeader(ReportWordFormRpt, dc);
  RptTextPrint(text, ReportWordFormRpt, 1, 0, -1, 20);
  RptFree(ReportWordFormRpt);
}

/* End of file. */
