/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19950121: begun
 * 19950122: more work -- gender induction
 * 19950505: added word duplication indexing (under word "dupdup").
 * 19951023: added adverbial finder
 */

#include "tt.h"
#include "toolcorp.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "repbasic.h"
#include "repchan.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptime.h"
#include "synpnode.h"
#include "ta.h"
#include "taemail.h"
#include "utildbg.h"
#include "utillrn.h"

/* BASIC ROUTINES */

Corpus *CorpusCreate()
{
  Corpus	*corpus;
  corpus = CREATE(Corpus);
  corpus->ht = HashTableCreate(30021L);
  return(corpus);
}

Article *ArticleCreate(char *filename, size_t startpos, size_t stoppos,
                       Ts *ts, ObjList *speakers, ObjList *listeners,
                       char *text)
{
  Article	*article;
  article = CREATE(Article);
  article->filename = filename;
  article->startpos = startpos;
  article->stoppos = stoppos;
  if (ts) article->ts = *ts;
  else TsSetNa(&article->ts);
  article->speakers = speakers;
  article->listeners = listeners;
  article->text = text;
  article->emmn = NULL;
  return(article);
}

/* CORPUS TRAINING */

char *CorpusPrevWord;

char *CorpusIndexWord1(Corpus *corpus, char *word0, long offset,
                       Article *article)
{
  char	word1[DWORDLEN], *word2;
  CorpusWordList *prevcwl, *cwl;
  StringToLowerNoAccents(word0, DWORDLEN, word1);
  word2 = HashTableIntern(corpus->ht, word1);
  prevcwl = (CorpusWordList *)HashTableGet(corpus->ht, word2);
  cwl = CREATE(CorpusWordList);
  cwl->offset = offset;
  cwl->article = article;
  cwl->next = prevcwl;
  HashTableSet(corpus->ht, word2, cwl);
  return(word2);
}

void CorpusIndexWord(Corpus *corpus, char *word0, long offset, Article *article)
{
  char	*word2;
  word2 = CorpusIndexWord1(corpus, word0, offset, article);
  if (word2 == CorpusPrevWord) {
    CorpusIndexWord1(corpus, "dupdup", offset, article);
  }
  CorpusPrevWord = word2;
}

void CorpusIndexWords(Corpus *corpus, Article *article)
{
  char		word[DWORDLEN], *p;
  long		offset;
  CorpusPrevWord = NULL;
  p = article->text;
  while (p[0]) {
    offset = (long)(p - article->text);
    if (!StringGetWord_LeNonwhite(word, p, DWORDLEN, &p, NULL)) break;
    if (word[0]) CorpusIndexWord(corpus, word, offset, article);
  }
}

void CorpusIndexArticle(Corpus *corpus, Article *article)
{
  CorpusIndexWords(corpus, article);
}

void CorpusIndexPNodeList(Corpus *corpus, Channel *ch, Ts *ts)
{
  ObjList		*speakers, *listeners;
  Ts			art_ts, *ts1;
  char			*art_emmn;
  PNode			*pn;
  Article		*article;
  speakers = listeners = NULL;
  art_ts = *ts;
  art_emmn = NULL;
  while ((pn = PNodeListRemove(ch->pnf))) {
    if (pn->type == PNTYPE_EMAILHEADER) {
      speakers = ObjListCreate(pn->u.emh->from_obj, NULL);
      listeners = ObjListAppendDestructive(pn->u.emh->to_objs,
                                           pn->u.emh->cc_objs);
      if ((ts1 = EmailHeaderTs(pn->u.emh))) {
        art_ts = *ts1;
        art_emmn = NULL;
      } else {
        art_ts = *ts;
      }
    } else if (pn->type == PNTYPE_ARTICLE) {
      article = pn->u.article;
      article->ts = art_ts;
      article->speakers = speakers;
      article->listeners = listeners;
      article->text = ChannelGetSubstring(ch, pn->lowerb, pn->upperb);
      article->emmn = art_emmn;
      CorpusIndexArticle(corpus, article);
    } else if (pn->type == PNTYPE_ATTRIBUTION) {
      speakers = ObjListCreate(pn->u.attribution->speaker, NULL);
      listeners = NULL;
      if (TsIsSpecific(&pn->u.attribution->ts)) {
        art_ts = pn->u.attribution->ts;
        if (pn->u.attribution->emmn[0]) {
          art_emmn = pn->u.attribution->emmn;
        }
      } else art_ts = *ts;
    }
  }
}

void CorpusPNodeListAddArticle(Channel *ch, char *filename, char *base,
                               char *start, char *end)
{
  ChannelAddPNode(ch, PNTYPE_ARTICLE, 1.0,
                  ArticleCreate(filename, start-base, end-base, NULL, NULL,
                                NULL, NULL),
                  NULL, start, end);
}

void CorpusIndexFileString(Corpus *corpus, Channel *ch, char *filename,
                           char *s, Ts *ts)
{
  char		*start, *end, *p;
  p = start = s;
  while (p[0]) {
    if (p == s || p[-1] == NEWLINE) {
      end = p;
      ch->pnf_holding_area = PNodeListCreate();
      if (TA_StartLineArticle(p, StdDiscourse, ch, &p)) {
        CorpusPNodeListAddArticle(ch, filename, s, start, end);
        PNodeListAppend(ch->pnf, ch->pnf_holding_area, ch);
        start = p;
      }
      PNodeListFree(ch->pnf_holding_area);
      ch->pnf_holding_area = NULL;
    }
    p++;
  }
  CorpusPNodeListAddArticle(ch, filename, s, start, p);
  CorpusIndexPNodeList(corpus, ch, ts);
}

Bool CorpusIndexFile(Corpus *corpus, char *fn, Ts *ts0)
{
  FILE		*stream;
  Ts		ts;
  Channel	ch;
  if (ts0) {
    ts = *ts0;
  } else {
    if (!StreamFileModtime(fn, &ts)) return(0);
  }
  if (NULL == (stream = StreamOpen(fn, "r"))) return(0);
  ChannelInit(&ch);
  ChannelOpen(&ch, stream, fn, DSPLINELEN, "r", 0, F_NULL, F_NULL, F_NULL,
              0, 0, 1, NULL);
  CorpusIndexFileString(corpus, &ch, HashTableIntern(corpus->ht, fn),
                        (char *)ch.buf, &ts);
  ChannelClose(&ch);
  return(1);
}

void CorpusIndexDirectory(Corpus *corpus, char *dirfn)
{
  Directory *dir, *p;
  if (!(dir = DirectoryRead(dirfn))) return;
  for (p = dir; p; p = p->next) {
    CorpusIndexFile(corpus, p->fn, &p->ts);
  }
  DirectoryFree(dir);
}

Corpus *CorpusFrench, *CorpusEnglish;

void CorpusInit()
{
  if (CorpusFrench == NULL) CorpusFrench = CorpusCreate();
  if (CorpusEnglish == NULL) CorpusEnglish = CorpusCreate();
}

Corpus *LangToCorpus(int lang)
{
  if (lang == F_ENGLISH) return(CorpusEnglish);
  else return(CorpusFrench);
}

/* CORPUS APPLICATIONS */

/* Example:
 *
 * 00000000001111111
 * 01234567890123456
 * Crème de la crème.
 * offset = 12
 * s = "la "
 * len = 3
 *
 * 0123
 * La crème
 * offset = 3
 * len = 3
 */
Bool CorpusFrenchGender2(char *s, CorpusWordList *cwl)
{
  int	len;
  len = strlen(s);
  if (len <= cwl->offset) {
    return(0 == memcmp(cwl->article->text + cwl->offset - len, s, len));
  } else {
    return(0);
  }
}

void CorpusFrenchGender1(CorpusWordList *cwl, /* RESULTS */ long *m, long *f)
{
  if (CorpusFrenchGender2(" le ", cwl) ||
      CorpusFrenchGender2(" Le ", cwl) ||
      CorpusFrenchGender2(" un ", cwl) ||
      CorpusFrenchGender2(" Un ", cwl) ||
      CorpusFrenchGender2(" aucun ", cwl) ||
      CorpusFrenchGender2(" Aucun ", cwl) ||
      CorpusFrenchGender2(" ce ", cwl) ||
      CorpusFrenchGender2(" Ce ", cwl) ||
      CorpusFrenchGender2(" cet ", cwl) ||
      CorpusFrenchGender2(" Cet ", cwl) ||
      CorpusFrenchGender2(" du ", cwl) ||
      CorpusFrenchGender2(" Du ", cwl) ||
      CorpusFrenchGender2(" au ", cwl) ||
      CorpusFrenchGender2(" Au ", cwl)) {
    (*m)++;
    return;
  }
  if (CorpusFrenchGender2(" la ", cwl) ||
      CorpusFrenchGender2(" La ", cwl) ||
      CorpusFrenchGender2(" ma ", cwl) ||
      CorpusFrenchGender2(" Ma ", cwl) ||
      CorpusFrenchGender2(" ta ", cwl) ||
      CorpusFrenchGender2(" Ta ", cwl) ||
      CorpusFrenchGender2(" sa ", cwl) ||
      CorpusFrenchGender2(" Sa ", cwl) ||
      CorpusFrenchGender2(" cette ", cwl) ||
      CorpusFrenchGender2(" Cette ", cwl) ||
      CorpusFrenchGender2(" aucune ", cwl) ||
      CorpusFrenchGender2(" Aucune ", cwl) ||
      CorpusFrenchGender2(" une ", cwl) ||
      CorpusFrenchGender2(" Une ", cwl)) {
    (*f)++;
    return;
  }
}

int CorpusFrenchGender(Corpus *corpus, char *word)
{
  int			len;
  long			m, f;
  char			buf[DWORDLEN];
  CorpusWordList	*cwl;
  StringToLowerNoAccents(word, DWORDLEN, buf);
  len = strlen(word);
  m = f = 0L;
  for (cwl = (CorpusWordList *)HashTableGet(corpus->ht, word);
       cwl; cwl = cwl->next) {
    if (0 != strncmp(word, cwl->article->text + cwl->offset, len)) continue;
    CorpusFrenchGender1(cwl, &m, &f);
  }
  /*
  fprintf(stdout, "<%s> M=%ld F=%ld\n", word, m, f);
   */
  if (m == 0L && f == 0L) {
    return(F_NULL);
  } else if (m > f) {
    return(F_MASCULINE);
  } else {
    return(F_FEMININE);
  }
}

void CorpusPrintWordList1(char *buf, CorpusWordList *cwl, int halflinelen,
                          int prelen, int wordlen, int longfmt)
{
  char	ts_buf[DWORDLEN];
  if (longfmt) {
    TsToStringDate(&cwl->article->ts, 1, DWORDLEN, ts_buf);
    sprintf(buf, "%-8.8s %8s %-8.8s ",
            StringNthTail(cwl->article->filename, 8), ts_buf,
            cwl->article->speakers ? M(cwl->article->speakers->obj) : "");
  } else {
    sprintf(buf, "%-8.8s ", StringNthTail(cwl->article->filename, 8));
  }
  StringMakeCorpusLine(cwl->article->text, cwl->offset, halflinelen, wordlen,
                       buf+prelen);
}

void CorpusPrintWordList(FILE *stream, CorpusWordList *cwl, char *word,
                         int linelen, int longfmt)
{
  char			buf[LINELEN];
  int			wordlen;
  int			prelen, halflinelen;
  StringArray	*sa;
  wordlen = strlen(word);
  if (longfmt) {
    prelen = 27;
  } else {
    prelen = 9;
  }
  halflinelen = (((linelen - prelen) - wordlen))/2;
  sa = StringArrayCreate();
  for (; cwl; cwl = cwl->next) {
    CorpusPrintWordList1(buf, cwl, halflinelen, prelen, wordlen, longfmt);
    StringArrayAddCopy(sa, buf, 0);
  }
  /* Sort by filename.
  StringArraySortCol(sa, 0);
  StreamSep(stream);
  StringArrayPrint(stream, sa, 0, 1);
  */

  /* Sort by ts.
  StringArraySortCol(sa, 9);
  StreamSep(stream);
  StringArrayPrint(stream, sa, 0, 1);
  */

  /* Sort by text following word. */
  StringArraySortCol(sa, prelen+halflinelen);
  StringArrayPrint(stream, sa, 0, 1);
  StringArrayPrint(Log, sa, 0, 1);
  StringArrayFreeCopy(sa);
}

void CorpusPrintWord(FILE *stream, Corpus *corpus, char *word0, int linelen)
{
  char	word1[DWORDLEN];
  StringToLowerNoAccents(word0, DWORDLEN, word1);
  CorpusPrintWordList(stream,
                      (CorpusWordList *)HashTableGet(corpus->ht, word1),
                      word1,
                      linelen,
                      0);
}

/* ADVERBIAL FINDER */

void Corpus_AdverbialFinder3(FILE *outstream, char *begin, char *end)
{
  char	*p;
  for (p = begin; p <= end; p++) {
    fputc(*p, outstream);
  }
  fputc(NEWLINE, outstream);
}

void Corpus_AdverbialFinder2(FILE *outstream, char *begin, char *end,
                             int numword_thresh)
{
  int	c, numwords;
  char	*p;
  numwords = 1;
  for (p = begin; p < end; p++) {
    c = *((uc *)p);
    if (c == ',') {
      if (numwords <= numword_thresh) {
        Corpus_AdverbialFinder3(outstream, begin, p);
      }
      break;
    }
    if (!LexEntryNonwhite(c)) numwords++;
  }
}

void Corpus_AdverbialFinder1(FILE *outstream, char *p, int numword_thresh)
{
  int	c, eos, inword;
  char	*begin;
  eos = inword = 0;
  begin = p;
  while ((c = *((uc *)p))) {
    if (c == '.' || c == '!' || c == '?') {
      if (eos == 0) {
        if (c == '.') eos = 1;
        else eos = 2;
      }
    } else if (LexEntryNonwhite(c)) {
      if (eos == 2 && CharSentenceStart(c)) {
        Corpus_AdverbialFinder2(outstream, begin, p, numword_thresh);
        begin = p;
      }
      eos = 0;
      inword = 1;
    } else {
      inword = 0;
      if (eos) eos = 2;
    }
    p++;
  }
}

/* It is intended for the user of this first to clear
 * outadv.txt, then to run the output through
 * sort|uniq.
 */
Bool Corpus_AdverbialFinderFile(char *in_fn, char *out_fn)
{
  char	*in;
  FILE	*outstream;
  if (NULL == (in = StringReadFile(in_fn, 0))) return(0);
  if (NULL == (outstream = StreamOpen(out_fn, "a"))) return(0);
  Corpus_AdverbialFinder1(outstream, in, 6);
  StreamClose(outstream);
  MemFree(in, "char StringReadFile");
  return(1);
}

Bool Corpus_AdverbialFinderDirectory(char *dirfn, char *out_fn)
{
  Directory *dir, *p;
  if (!(dir = DirectoryRead(dirfn))) return(0);
  for (p = dir; p; p = p->next) {
    Corpus_AdverbialFinderFile(p->fn, out_fn);
  }
  DirectoryFree(dir);
  return(1);
}

/* End of file. */
