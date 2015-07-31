/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Common affix finder.
 *
 * 19960213: cleanup
 *
 * Currently used only by TA_Name.
 */

#include "tt.h"
#include "lexentry.h"
#include "lexwf3.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "utildbg.h"
#include "utilrpt.h"

#define AFFIXTHRESH	8L

void WordFormAffixFindTrain1(HashTable *ht, int prefix, char *match_features,
                             char *word, char *word_features, int minlen,
                             int maxlen)
{
  int		i, len;
  char		affixbuf[PHRASELEN], *affix;
  size_t	count;
  if (StringAllIn(match_features, word_features)) {
    len = strlen(word);
    for (i = minlen; i <= maxlen && i <= len-1; i++) {
      if (prefix) {
        StringCpy(affixbuf, word, PHRASELEN);
        affixbuf[i] = TERM;
        affix = affixbuf;
      } else {
        affix = StringNthTail(word, i);
      }
      affix = HashTableIntern(ht, affix);
      count = (size_t)HashTableGet(ht, affix);
      count++;
      HashTableSet(ht, affix, (void *)count);
    }
  }
}

/* todo: More specific affixes are stored unnecessarily. */
HashTable *WordFormAffixFindTrain(int prefix, Obj *class, char *features,
                                  int phrases_ok, int minlen, int maxlen)
{
  LexEntry	*le;
  HashTable	*ht;
  Word		*word;
  ht = HashTableCreate(1021L);
  for (le = AllLexEntries; le; le = le->next) {
    if ((!phrases_ok) && LexEntryIsPhrase(le)) continue;
    if (class && (!LexEntryConceptIsAncestor(class, le))) continue;
    for (word = le->infl; word; word = word->next) {
      WordFormAffixFindTrain1(ht, prefix, features, word->word, word->features,
                              minlen, maxlen);
    }
  }
  return(ht);
}

Bool WordFormAffix(HashTable *ht, int prefix, char *word, int minlen,
                   int maxlen)
{
  int		i, len;
  char		affixbuf[PHRASELEN], *affix;
  if (ht == NULL) return(0);
  len = strlen(word);
  for (i = minlen; i <= maxlen && i <= len-1; i++) {
    if (prefix) {
      StringCpy(affixbuf, word, PHRASELEN);
      affixbuf[i] = TERM;
      affix = affixbuf;
    } else {
      affix = StringNthTail(word, i);
    }
    if (AFFIXTHRESH < (size_t)HashTableGet(ht, affix)) return(1);
  }
  return(0);
}

Discourse	*ReportWordFormAffixDc;
Rpt		*ReportWordFormAffixRpt;
Obj		*WordFormAffixClass;

void ReportWordFormAffixHeader(Rpt *rpt, Discourse *dc)
{
  RptStartLine(rpt);
  RptAddConcept(rpt, N("quantity"), dc);
  RptAddConcept(rpt, WordFormAffixClass, dc);
}

void ReportWordFormAffixLine(char *index, size_t count)
{
  if (count < AFFIXTHRESH) return;
  RptStartLine(ReportWordFormAffixRpt);
  RptAddLong(ReportWordFormAffixRpt, count, ReportWordFormAffixDc);
  RptAdd(ReportWordFormAffixRpt, index, RPT_JUST_LEFT);
}

void ReportWordFormAffix(Text *text, Discourse *dc, HashTable *ht,
                         Obj *affix_class)
{
  if (ht == NULL) return;
  ReportWordFormAffixDc = dc;
  WordFormAffixClass = affix_class;
  ReportWordFormAffixRpt = RptCreate();
  HashTableForeach(ht, ReportWordFormAffixLine);
  ReportWordFormAffixHeader(ReportWordFormAffixRpt, dc);
  RptTextPrint(text, ReportWordFormAffixRpt, 1, 0, -1, 20);
  RptFree(ReportWordFormAffixRpt);
}

/* End of file. */
