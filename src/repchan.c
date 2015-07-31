/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Communications (I/O) channel.
 *
 * 19940108: begun
 * 19940109: better algorithm developed while doing laundry
 * 19940702: objectized for fun
 * 19941209: redid the buffering mechanism to enable text agents to work
 * 19941212: merged previous SentenceStream with Channels
 * 19950202: blank lines result in PNodes; since any message causes end
 *           of sentence, blank lines now cause end of sentence (to deal
 *           with wire feed headlines having no period).
 * 19950209: converted to new parsing scheme
 * 19980630: ChannelReadLine
 * 19981115: slight allocation mods
 */

#include "tt.h"
#include "lexentry.h"
#include "repbasic.h"
#include "repchan.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptime.h"
#include "synpnode.h"
#include "utildbg.h"

void ChannelInit(Channel *ch)
{
  ch->stream = NULL;
  ch->linelen = DSPLINELEN;
  ch->realtime = 0;
  TsSetNa(&ch->last_activity);
  ch->activity = 0;
  ch->fn[0] = TERM;
  ch->mode[0] = TERM;
  ch->output_reps = 0;
  ch->lang = F_NULL;
  ch->dialect = F_NULL;
  ch->style = F_NULL;
  ch->infrequent_ok = 0;
  ch->ht = NULL;
  TsRangeSetNever(&ch->last_tsr);
  ch->buf = NULL;
  ch->len = 0L;
  ch->maxlen = 0L;
  ch->pos = 0L;
  ch->paraphrase_input = 0;
  ch->echo_input = 0;
  ch->echo_input_last = NULL;
  ch->pnf = NULL;
  ch->pnf_holding_area = NULL;
  ch->synparse_pns = NULL;
  ch->synparse_pnnnext = 0;
  ch->synparse_lowerb = 0L;
  ch->synparse_upperb = 0L;
  ch->synparse_sentences = 0;
  ch->translations = NULL;
  ch->input_text = NULL;
}

void ChannelOpen(Channel *ch, FILE *stream, char *fn, int linelen,
                 char *mode, int output_reps, int lang, int dialect,
                 int style, int paraphrase_input, int echo_input,
                 int batch_ok, char *s)
{
  ChannelClose(ch);
  ch->stream = stream;
  if (stream == stdin || stream == stdout) {
    ch->realtime = CH_REALTIME_LINE_BASED;
  }
  ch->linelen = linelen;
  StringCpy(ch->mode, mode, WORDLEN);
  StringCpy(ch->fn, fn, FILENAMELEN);
  ch->output_reps = output_reps;
  ch->lang = lang;
  ch->dialect = dialect;
  ch->style = style;
  ch->infrequent_ok = 0;
  ch->ht = LexEntryLangHt1(lang);
  if (mode[0] == 'r') {
  /* Input channel. */
    ChannelBufferClear(ch);
    if (s != NULL) {
      ChannelAddToBuffer(ch, s, (size_t)strlen(s));
    } else if ((!ch->realtime) && batch_ok) {
      ChannelReadBatch(ch);
    }
  } else {
    ch->maxlen = 0L;
    ch->len = 0L;
    ch->pos = 0L;
    ch->buf = NULL;
  }
  TsRangeSetNever(&ch->last_tsr);
  TsSetNow(&ch->last_activity);
  ch->activity = 0;
  ch->paraphrase_input = paraphrase_input;
  ch->echo_input = echo_input;
}

void ChannelBufferClear(Channel *ch)
{
  if (ch->buf) MemFree(ch->buf, "char Channel");
  ch->maxlen = 256L;
  ch->len = 0L;
  ch->pos = 0L;
  ch->buf = (unsigned char *)MemAlloc(ch->maxlen, "char Channel");
  ch->pnf = PNodeListCreate(); /* todoFREE: previous */
}

/* This can be called multiple times without harm. */
void ChannelClose(Channel *ch)
{
  if (ch->stream) {
    if (ch->stream == stdin) {
#ifdef MACOS
      csetmode(C_ECHO, ch->stream);
#else
#endif
    } else {
      StreamClose(ch->stream);
    }
    ch->stream = NULL;
  }
  if (ch->buf) MemFree(ch->buf, "char Channel");
  /* if (ch->pnf) PNodeListFree(ch->pnf); */ /* todoFREE */
  ChannelInit(ch);
}

Dur ChannelIdleTime(Channel *ch)
{
  Ts		now;
  TsSetNow(&now);
  return(TsMinus(&now, &ch->last_activity));
}

void ChannelUnget(Channel *ch)
{
  if (ch->pos > 0) ch->pos--;
}

void ChannelResetActivityTime(Channel *ch)
{
  TsSetNow(&ch->last_activity);
}

void ChannelNoteActivity(Channel *ch)
{
  ch->activity = 1;
  TsSetNow(&ch->last_activity);
}

void ChannelAddToBuffer(Channel *ch, char *s, size_t len)
{
  while ((len + ch->len + 1L) > ch->maxlen) {
    ch->maxlen =  ch->maxlen * 2L;
    ch->buf = (unsigned char *)MemRealloc(ch->buf, ch->maxlen, "char Channel");
  }
  memcpy(ch->buf + ch->len, s, len);
  ch->len += len;
  ch->buf[ch->len] = TERM;
  ChannelNoteActivity(ch);
}

void ChannelTruncate(Channel *ch, size_t len)
{
  ch->len = len;
  ch->buf[ch->len] = TERM;
  ChannelNoteActivity(ch);
}

void ChannelBufferDeleteLast(Channel *ch)
{
  if (ch->len > ch->pos) {
    if (ch->stream == stdin) {
      fputc(BS, stdout);
      fputc(SPACE, stdout);
      fputc(BS, stdout);
    }
    ch->len--;
    ChannelNoteActivity(ch);
  }
}

int ChannelReadRealtime(Channel *ch)
{
  int	c, success;
  char	buf[2];
  success = 0;
  while (1) {
    if (EOF == (c = getc(ch->stream))) {
      return(success);
    }
    if (c == BS) {
      ChannelBufferDeleteLast(ch);
    } else {
      fputc(c, stdout);
      if (c == CR) fputc(NEWLINE, stdout);
      buf[0] = c;
      buf[1] = TERM;	/* For debugging. */
      ChannelAddToBuffer(ch, buf, 1L);
      success = 1;
    }
  }
}

int ChannelReadPrompt(Channel *ch)
{
  int	success;
  char	line[LINELEN];
  success = 0;
  while (1) {
    if (ch->stream == stdin) fputs("> ", stdout);
    if (NULL == fgets(line, LINELEN, ch->stream)) {
      break;
    }
    if (line[0] == NEWLINE && line[1] == TERM) break;
    if (line[0] == '.' && line[1] == NEWLINE && line[2] == TERM) break;
    success = 1;
    ChannelAddToBuffer(ch, line, (size_t)strlen(line));
  }
  return(success);
}

int ChannelReadLine(Channel *ch)
{
  char	line[LINELEN];
  if (ch->stream == stdin) fputs("> ", stdout);
  if (NULL == fgets(line, LINELEN, ch->stream)) return(0);
  if (line[0] == NEWLINE && line[1] == TERM) return(0);
  if (line[0] == '.' && line[1] == NEWLINE && line[2] == TERM) return(0);
  ChannelBufferClear(ch);
  ChannelAddToBuffer(ch, line, (size_t)strlen(line));
  return(1);
}

int ChannelReadBatch(Channel *ch)
{
  int	success;
  char	line[LINELEN];
  success = 0;
  while (1) {
    /* todo: Use fread instead. */
    if (NULL == fgets(line, LINELEN, ch->stream)) {
      break;
    }
    success = 1;
    ChannelAddToBuffer(ch, line, (size_t)strlen(line));
  }
  return(success);
}

/* Return value only for those who need to add more onto the PNode. */
PNode *ChannelAddPNode(Channel *ch, int type, Float score, void *any,
                       char *punc, char *begin, char *rest)
{
  PNode			*pn;
  char			*features;

  pn = PNodeCreate1();
  pn->type = type;
  pn->score = score;
  if (punc) StringCpy(pn->punc, punc, PUNCLEN);
  switch (type) {
    case PNTYPE_LEXITEM:
      features = ((Lexitem *)any)->features;
      pn->feature = FeatureGet(features, FT_POS);
      pn->lexitem = any;
      if (StringIn(F_ELISION, features)) pn->punc[0] = TERM;
      break;
    case PNTYPE_NAME:
    case PNTYPE_ORG_NAME:
    case PNTYPE_POLITY:
    case PNTYPE_TELNO:
    case PNTYPE_MEDIA_OBJ:
    case PNTYPE_PRODUCT:
    case PNTYPE_NUMBER:
      pn->feature = F_NOUN;
      pn->u.any = any;
      break;
    case PNTYPE_TSRANGE:
      pn->feature = F_ADVERB;
      pn->u.any = any;
      break;
    default:
      pn->feature = F_NULL;
      pn->u.any = any;
      break;
  }
  pn->lowerb = begin - (char *)ch->buf;
  pn->upperb = (rest - (char *)ch->buf) - 1;
  if (ch->pnf_holding_area) {
    PNodeListAdd(ch->pnf_holding_area, ch, pn, 1);
  } else {
    PNodeListAdd(ch->pnf, ch, pn, 1);
  }
  return(pn);
}

char *ChannelGetSubstring(Channel *ch, size_t lowerb, size_t upperb)
{
  return(StringCopyLenLong(((char *)ch->buf) + lowerb,
                           (1L + upperb - lowerb),
                           "char * Channel"));
}

void ChannelPrintPNodes(FILE *stream, Channel *ch)
{
  PNode	*pn;
  for (pn = ch->pnf->first; pn; pn = pn->next) {
    PNodePrint(stream, ch, pn);
  }
}

void ChannelPrintPNodesOfType(FILE *stream, Channel *ch, int type)
{
  PNode	*pn;
  for (pn = ch->pnf->first; pn; pn = pn->next) {
    if (pn->type == type) {
      PNodePrint(stream, ch, pn);
    }
  }
}

void ChannelPrintHighlight(FILE *stream, Channel *ch, int pntype, int phrase)
{
  size_t	pos;
  char		*delim;
  PNode		*pn;
  int		found;
  found = 0;
  delim = (char *)MemAlloc(ch->len+1L, "char * ChannelPrint");
  memset(delim, SPACE, (long)(ch->len+1L));
  for (pn = ch->pnf->first; pn; pn = pn->next) {
    if (pn->type != pntype) continue;
    if (pntype == PNTYPE_LEXITEM &&
        (pn->lexitem == NULL ||
         phrase != IsPhrase(pn->lexitem->word))) {
      continue;
    }
    found = 1;
    delim[pn->lowerb] = '<';
    delim[pn->upperb] = '>';
  }
  if (!found) return;
  StreamSep(stream);
  if (pntype == PNTYPE_LEXITEM) {
    if (phrase) {
      fprintf(stream, "%s phrases:\n", PNodeTypeToString(pntype));
    } else {
      fprintf(stream, "%s words:\n", PNodeTypeToString(pntype));
    }
  } else {
    fprintf(stream, "%s:\n", PNodeTypeToString(pntype));
  }
  for (pos = 0L; pos < ch->len; pos++) {
    if (delim[pos] == '<') fputs("[[", stream);
    fputc(ch->buf[pos], stream);
    if (delim[pos] == '>') fputs("]]", stream);
  }
  fputc(NEWLINE, stream);
  MemFree(delim, "char * ChannelPrint");
}

void ChannelPrintHighlights(FILE *stream, Channel *ch)
{
  int	i;
  for (i = PNTYPE_MIN; i <= PNTYPE_MAX; i++) {
    if (i == PNTYPE_LEXITEM) {
      ChannelPrintHighlight(stream, ch, i, 0);
      ChannelPrintHighlight(stream, ch, i, 1);
    } else {
      ChannelPrintHighlight(stream, ch, i, 0);
    }
  }
}

/* End of file. */
