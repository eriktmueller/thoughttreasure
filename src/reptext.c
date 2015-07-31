/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940712: begun
 * 19940916: restructured
 */

#include "tt.h"
#include "lextheta.h"
#include "reptext.h"
#include "repbasic.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "repstr.h"
#include "reptime.h"
#include "semgen1.h"
#include "semgen2.h"
#include "semdisc.h"
#include "synpnode.h"
#include "utildbg.h"
#include "utilhtml.h"
#include "utiltype.h"

Text *TextCreate(long sizeguess, int linelen, Discourse *dc)
{
  Text	*text;
  text = CREATE(Text);
  text->s = (char *)MemAlloc((size_t)sizeguess, "Text* s");
  text->s[0] = TERM;
  text->len = 0L;
  text->maxlen = sizeguess;
  text->lang = DC(dc).lang;
  text->pos = 0;
  text->linelen = linelen;
  text->attach_next_word = 0;
  text->dc = dc;
  return(text);
}

Text *TextCreat(Discourse *dc)
{
  return(TextCreate(PHRASELEN, DiscourseCurrentChannelLineLen(dc), dc));
}

void TextFree(Text *text)
{
  MemFree(text->s, "Text* s");
  MemFree(text, "Text");
}

void TextPutc(int c, Text *text)
{
  if ((text->len + 1) >= text->maxlen) {
    text->maxlen = text->maxlen*2;
    text->s = (char *)MemRealloc(text->s, (size_t)text->maxlen, "Text* s");
  }
  text->s[text->len] = c;
  text->len++;
  if (c == NEWLINE) text->pos = 0;
  else text->pos++;
}

void TextTabTo(int pos, Text *text)
{
  while (text->pos < pos) TextPutc(SPACE, text);
}

void TextNewlineIfNecessary(Text *text)
{
  if (text->pos > 0) TextPutc(NEWLINE, text);
}

void TextNextParagraph(Text *text)
{
  TextNewlineIfNecessary(text);
  TextPutc(NEWLINE, text);
}

void TextPuts(char *s, Text *text)
{
  while (*s) {
    TextPutc(*s, text);
    s++;
  }
}

void TextPutsIndented(char *s, Text *text, char *indent_string)
{
  while (*s) {
    if (*s == NEWLINE) {
      TextPutc(NEWLINE, text);
      TextPuts(indent_string, text);
    } else {
      TextPutc(*s, text);
    }
    s++;
  }
}

void TextAttachNextWord(Text *text)
{
  text->attach_next_word = 1;
}

void TextPutword(char *s, int post, Text *text)
{
  int	len;
  if (text->attach_next_word) {
  /* todo: Possibly rebreak line if too long. */
  } else {
    len = strlen(s);
    if ((len + text->pos + (post != TERM)) >= text->linelen) {
      TextPutc(NEWLINE, text);
    }
    if (text->pos != 0) TextPutc(SPACE, text);
  }
  TextPuts(s, text);
  if (post != TERM) TextPutc(post, text);
  text->attach_next_word = 0;
}

void TextPrintf(Text *text, char *fmt, ...)
{
  va_list	args;
  char		buf[LINELEN];
#ifdef SUNOS
  va_start(args);
#else
  va_start(args, fmt);
#endif
  vsprintf(buf, fmt, args);
  TextPuts(buf, text);
  va_end(args);
}

void TextPrintfWord(Text *text, int post, char *fmt, ...)
{
  va_list	args;
  char		buf[LINELEN];
#ifdef SUNOS
  va_start(args);
#else
  va_start(args, fmt);
#endif
  vsprintf(buf, fmt, args);
  TextPutword(buf, post, text);
  va_end(args);
}

void TextPrintSpaces(Text *text, int spaces)
{
  int	i;
  for (i = 0; i < spaces; i++) TextPutc(SPACE, text);
}

void TextPrintFeatAbbrev(Text *text, int feature, int force, int post,
                         Discourse *dc)
{
  char	*s;
  if ((s = GenFeatAbbrevString(feature, force, dc))) {
    TextPutword(s, post, text);
  }
}

void TextPrintConceptAbbrev(Text *text, Obj *con, int force, int post,
                            Discourse *dc)
{
  char	*s;
  s = GenConceptAbbrevString(con, force, dc);
  TextPutword(s, post, text);
}

void TextPrintFeaturesAbbrev(Text *text, char *features, int force,
                             char *except, Discourse *dc)
{
  char	buf[PHRASELEN];
  GenFeaturesAbbrevString(features, 1, force, except, dc, PHRASELEN, buf);
  if (buf[0]) {
    TextPrintPhrase(text, TERM, buf);
  }
}

/* Used in printing related words.
 * This is the short form. Contrast with ThetaRoleTextPrint.
 */
void TextPrintWordAndFeatures(Text *text, int html, int lang, char *word,
                              char *features, ThetaRole *theta_roles,
                              int force, int post, char *except, Discourse *dc)
{
  char	buf_preverb[PHRASELEN], buf_postverb1[PHRASELEN];
  char	buf_postverb2[PHRASELEN], buf_iobj[PHRASELEN], buf_feat[PHRASELEN];
  char	buf[PHRASELEN];
  if (html) {
    HTML_TextPrintBoldBeginIfEnglish(text, lang);
  }
  ThetaRoleShortPrint(theta_roles, PHRASELEN, TRPOS_PRE_VERB,
                      N("expl"), buf_preverb);
  ThetaRoleShortPrint(theta_roles, PHRASELEN, TRPOS_POST_VERB_PRE_OBJ,
                      N("expl"), buf_postverb1);
  ThetaRoleShortPrint(theta_roles, PHRASELEN, TRPOS_POST_VERB_POST_OBJ,
                      N("expl"), buf_postverb2);
  ThetaRoleShortPrint(theta_roles, PHRASELEN, 0, N("iobj"), buf_iobj);
  GenFeaturesAbbrevString(features, 1, force, except, dc, PHRASELEN, buf_feat);
  buf[0] = TERM;
  if (buf_preverb[0]) {
    StringCat(buf, buf_preverb, PHRASELEN);
  }
  if (word[0]) {
    if (buf[0]) StringCat(buf, " ", PHRASELEN);
    StringCat(buf, word, PHRASELEN);
  }
  if (buf_postverb1[0]) {
    if (buf[0]) StringCat(buf, " ", PHRASELEN);
    StringCat(buf, buf_postverb1, PHRASELEN);
  }
  if (buf_postverb2[0]) {
    if (buf[0]) StringCat(buf, " ", PHRASELEN);
    StringCat(buf, buf_postverb2, PHRASELEN);
  }
  if (buf_iobj[0]) {
    if (buf[0]) StringCat(buf, " ", PHRASELEN);
    StringCat(buf, buf_iobj, PHRASELEN);
  }
  if (html) {
    HTML_TextPrint(text, buf);
    HTML_TextPrintBoldEndIfEnglish(text, lang);
    TextPutc(SPACE, text);
    TextPuts(buf_feat, text);
    TextPutc(SPACE, text);
  } else {
    if (buf_feat[0]) {
      if (buf[0]) StringCat(buf, " ", PHRASELEN);
      StringCat(buf, buf_feat, PHRASELEN);
    }
    StringAppendChar(buf, PHRASELEN, post);
    TextPrintPhrase(text, TERM, buf);
  }
}

void TextPrintTTObject(Text *text, Obj *obj, Discourse *dc)
{
  char	buf[PHRASELEN];
  buf[0] = GR_OBJNAME;
  StringCpy(buf+1, ObjToName(obj), PHRASELEN-1);
  TextPutword(buf, TERM, text);
}

void TextPrintConcept(Text *text, Obj *obj, int pos, int paruniv, int gender,
                      int number, int person, int post, int toupper,
                      int ancestor_ok, int showgender, Discourse *dc)
{
  char	buf[PHRASELEN];
  GenConceptString(obj, N("empty-article"), pos, F_NULL, DC(dc).lang, gender,
                   number, person, PHRASELEN, ancestor_ok, showgender, dc, buf);
  TextPrintPhrase(text, post, buf);
}

/* todo: Post seems like a bogus concept. Why doesn't the caller just add it
 * to phrase?
 */
void TextPrintPhrase(Text *text, int post, char *phrase)
{
  char	word[PHRASELEN], phrase1[SENTLEN], *p;
  StringCpy(phrase1, phrase, PHRASELEN);
  if (post) StringAppendChar(phrase1, PHRASELEN, post);
  p = phrase1;
  while (1) {
    while (*p == SPACE) p++;
    p = StringReadTo(p, word, PHRASELEN, SPACE, NEWLINE, TAB);
    if (word[0] == TERM) return;
    TextPutword(word, TERM, text);
  }
}

void TextPrintIsa(Text *text, Obj *obj, Obj *parent, Discourse *dc)
{
  PNode		*pn;
  if ((pn = GenIsa(obj, parent, N("present-indicative"), dc))) {
    PNodeTextPrint(text, pn, '.', 0, 1, dc);
    PNodeFreeTree(pn);
  }
}

int TextLastChar(Text *text)
{
  if (text->len > 0) return(text->s[text->len-1]);
  else return(TERM);
}

void TextFlush(Text *text)
{
  text->s[text->len] = TERM;
}

void TextPrint1(FILE *stream, Text *text)
{
  TextFlush(text);
  fputs(text->s, stream);
  fflush(stream);
}

void TextPrint(FILE *stream, Text *text)
{
  if (stream == stdout && (text->dc->mode & DC_MODE_TURING)) {
    fputc(NEWLINE, stream);
    TypingPrint(stream, TextString(text), text->dc);
  } else{
    TextPrint1(stream, text);
  }
}

void TextPrintWithPreface1(FILE *stream, char *preface, Text *text)
{
  char	*p;
  if (preface == NULL || preface[0] == TERM) {
    TextPrint(stream, text);
    return;
  }
  TextFlush(text);
  p = text->s;
  if (p[0] == TERM) return;
  fputs(preface, stream);
  while (*p) {
    fputc(*p, stream);
    if (*p == NEWLINE && *(p+1) != TERM) fputs(preface, stream);
    p++;
  }
  fflush(stream);
}

void TextPrintWithPreface(FILE *stream, char *preface, Text *text)
{
  if (stream == stdout && (text->dc->mode & DC_MODE_TURING)) {
    fputc(NEWLINE, stream);
    TypingPrint(stream, TextString(text), text->dc);
  } else{
    TextPrintWithPreface1(stream, preface, text);
  }
}

void TextToString(Text *text, int maxlen, /* RESULTS */ char *s)
{
  TextFlush(text);
  StringCpy(s, text->s, maxlen);
}

char *TextString(Text *text)
{
  TextFlush(text);
  return(text->s);
}

void TextPutPNode(Text *text, PNode *pn, int eos, Discourse *dc)
{
  if (eos == EOSNA) eos = pn->eos;
  if (eos == EOSNA) eos = '.';
  PNodeTextPrint(text, pn, eos, 0, 1, dc);
  if (DbgOn(DBGGENER, DBGHYPER)) {
    PNodePrint(Log, DiscourseGetInputChannel(dc), pn);
  }
}

void TextPutPNodes(Text *text, ObjList *pns, Discourse *dc)
{
  ObjList	*p;
  for (p = pns; p; p = p->next) {
    TextPutPNode(text, p->u.sp.pn, EOSNA, dc);
  }
}

void TextGen(Text *text, Obj *obj, int eos, Discourse *dc)
{
  PNode	*pn;
  DiscourseFlipListenerSpeaker(dc);
  if (DCREALTIME(dc)) TsSetNow(DCNOW(dc));
  TsRangeSetNever(&DC(dc).last_tsr);
  if ((pn = Gen(F_S, obj, NULL, dc))) {
    TextPutPNode(text, pn, eos, dc);
    PNodeFreeTree(pn);
  }
  DiscourseFlipListenerSpeaker(dc);
}

void TextGenParagraph(Text *text, ObjList *in_objs, int sort_by_ts,
                      int gen_pairs, Discourse *dc)
{
  ObjList	*pns;
  DiscourseFlipListenerSpeaker(dc);
  if (DCREALTIME(dc)) TsSetNow(DCNOW(dc));
  TsRangeSetNever(&DC(dc).last_tsr);
  if ((pns = GenParagraph(in_objs, sort_by_ts, gen_pairs, dc))) {
    TextPutPNodes(text, pns, dc);
    ObjListFreePNodes(pns);
  }
  DiscourseFlipListenerSpeaker(dc);
}

/* End of file. */
