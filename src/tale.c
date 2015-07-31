/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Words and phrases.
 *
 * 19981022T081913: minor mods
 */

#include "tt.h"
#include "lexentry.h"
#include "lexitem.h"
#include "lexobjle.h"
#include "repbasic.h"
#include "repchan.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
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

void TA_LELAdd(LexEntry *le, char *features, int maxlen,
               /* RESULTS */ LexEntry **le_le, char **le_feat, int *le_len)
{
  int	i, len;
  char	buf[FEATLEN];
  len = *le_len;
  if (!StringIn(F_VERB, features)) {
  /* This condition allows "Qui a peint Le Déjeuner sur l herbe?" to parse.
   * But is collapsing needed for verbs for other reasons?
   * It would be nice to collapse verbs to reduce number of parses, but we
   * can't reduce them because tense (and the resulting aspect)
   * information is significant.
   */
    for (i = 0; i < len; i++) {
      if (le == le_le[i]) {
        StringIntersect(le_feat[i], features, buf);
        StringCpy(le_feat[i], buf, FEATLEN);
        return;
      }
    }
  }
  if (len < maxlen) {
    le_le[len] = le;
    le_feat[len] = MemAlloc(FEATLEN, "char TA_LELAdd");
    StringCpy(le_feat[len], features, FEATLEN);
    len++;
    *le_len = len;
  } else {
    Dbg(DBGGEN, DBGBAD, "TA_LELAdd: increase maxlen <%s.%s>",
        le->srcphrase, le->features);
  }
}

#define MAXLES 10

void FeatSubstPOS(char *in, int newpos, /* RESULTS */ char *out)
{
  while (*in) {
    if (StringIn(*in, FT_POS)) {
      *out = newpos;
    } else {
      *out = *in;
    }
    in++;
    out++;
  }
  *out = TERM;
}

/* todo: Avoid enumeration of subjunctive, indicative, and imperative cases?
 * But these have distinct meanings.
 */
void TA_LexEntryEnter(Channel *ch, IndexEntry *ie, int prepunc, char *postpunc,
                      char *phrase, char *begin, char *rest, HashTable *ht)
{
  int		i, phrase_len, pos, le_len, elision_infl, elision_in;
  int		contraction_infl, contraction_in, noun_added;
  Float		score;
  Lexitem	*lexitem;
  LexEntry	*le_le[MAXLES];
  char		*le_feat[MAXLES], feat[FEATLEN];
  noun_added = 0;
  le_len = 0;
  phrase_len = strlen(phrase);
  /* Inflection collapsing and elimination of noncompatible
   * contractions/elisions:
   */
  for (; ie; ie = ie->next) {
    if (ie->lexentry == NULL) continue;
    if (LexEntryFilterOut(ie->lexentry)) continue;
    pos = FeatureGet(ie->lexentry->features, FT_POS);
    if (pos == F_NOUN) noun_added = 1;
    if (phrase_len == 1 &&
        (pos == F_NOUN || pos == F_ADJECTIVE)) {
      /* Noun or adjective meanings of single characters are ruled out
       * here because they should be handled by specialized TAs such as
       * TA_MediaObject, TA_Name, TA_Product, a formula TA. todoSCORE
       * todo: Some TA's assume lexical PNodes have been generated here.
       * If so, they could be marked as "to be ignored by Syn_Parse".
       * todoSCORE: An adjective followed by single quote (as in I')
       * is rare?
       * MRBEGIN
       * Note this prevents R (MPAA rating) from being parsed.
       * MREND
       */
      continue;
    }
    contraction_infl = StringIn(F_CONTRACTION, ie->features);
    contraction_in = ((prepunc == SQUOTE) || (prepunc == SQUOTE2));
    elision_infl = StringIn(F_ELISION, ie->features);
    elision_in = ((postpunc[1] == TERM) &&
                ((postpunc[0] == SQUOTE) || (((uc *)postpunc)[0] == SQUOTE2)));
    if ((elision_infl ? elision_in : 1) &&
        (contraction_infl ? contraction_in : 1)) {
    /* If an inflection is marked for elision or contraction, then
     * the input must have a contraction mark. cf "I'm", "isn't" todoSCORE
     */
      TA_LELAdd(ie->lexentry, ie->features, MAXLES, le_le, le_feat, &le_len);
    }
  }
  for (i = 0; i < le_len; i++) {
    lexitem = LexitemCreate(phrase, le_le[i], HashTableIntern(ht, le_feat[i]));
    score = 1.0;
    /* todoSCORE: LexEntryFind phrase should return a score that
     * indicates the degree of relaxation.
     */
    ChannelAddPNode(ch, PNTYPE_LEXITEM, score, lexitem, postpunc, begin, rest);
  }

  if ((!noun_added) &&
      ((prepunc == SQUOTE && StringIn(SQUOTE, postpunc)) ||
       (prepunc == SQUOTE2 && StringIn(SQUOTE2, postpunc)) ||
       (prepunc == LGUILLEMETS && StringIn(RGUILLEMETS, postpunc)) ||
       (prepunc == DQUOTE && StringIn(DQUOTE, postpunc)))) {
    /* Duplicate quoted lexical items (metalinguistic) as nouns.
     * Example: What does (the word) "stupid" mean?
     */
    for (i = 0; i < le_len; i++) {
      FeatSubstPOS(le_feat[i], F_NOUN, feat);
      lexitem = LexitemCreate(phrase, le_le[i], HashTableIntern(ht, feat));
      score = 1.0;
      ChannelAddPNode(ch, PNTYPE_LEXITEM, score, lexitem, postpunc, begin,
                      rest);
    }
  }

  for (i = 0; i < le_len; i++) {
    MemFree(le_feat[i], "char TA_LELAdd");
  }
}

Bool TA_LexEntry1(char *phrase, char *postpunc, char *p,
                  char *rest, int prev, HashTable *ht, Channel *ch,
                  Discourse *dc, int nofail)
{
  int		found, freeme;
  IndexEntry	*ie;
  found = 0;
  if ((ie = LexEntryFindPhrase(ht, phrase, INTPOSINF, 1, nofail, &freeme))) {
    if (!IndexEntryAllFilterOut(ie)) {
      found = 1;
      TA_LexEntryEnter(ch, ie, prev, postpunc, HashTableIntern(ht, phrase),
                       p, rest, ht);
    }
    if (freeme) IndexEntryFree(ie);
  }
  return(found);
}

void TA_LexEntry(Channel *ch, Discourse *dc)
{
  int		numwords, cur, prev, mods, max_words;
  char		*p, *rest, phrase[PHRASELEN], postpunc[PUNCLEN];
  HashTable	*ht;
  ht = DC(dc).ht;
  prev = TERM;
  p = (char *)ch->buf;

  if (dc->mode & DC_MODE_COMPOUND_NOUN) max_words = 1;
  else max_words = MaxWordsInPhrase+1;

  while (*p) {
    cur = *((uc *)p);
    if (LexEntryNonwhite(cur) && (!LexEntryNonwhite(prev))) {
    /* At start of word/phrase. */
      for (numwords = 1; numwords <= max_words; numwords++) {
        if (StringGetNWords_LeNonwhite(phrase, postpunc, p, PHRASELEN, numwords,
                                       &rest)) {
          if (!TA_LexEntry1(phrase, postpunc, p, rest, prev, ht, ch, dc,
                            numwords == 1)) {
            /* todo: Inelegant. This is required to deal with "." at end of
             * sentence. This may eliminate needed dots. A better solution
             * is to eliminate dots specifically where EOS is detected.
             */
            StringElims(phrase, LE_NONWHITESPACE, &mods);
            if (mods) {
              TA_LexEntry1(phrase, postpunc, p, rest, prev, ht, ch, dc,
                           numwords == 1);
            }
          }
        }
      }
    }
    prev = cur;
    p++;
  }
}

void TA_EndOfSentence(Channel *ch, Discourse *dc)
{
  int	c, eos, inword;
  char	*p, *begin;
  eos = inword = 0;
  p = (char *)ch->buf;
  begin = NULL;
  while ((c = *((uc *)p))) {
    if (c == ';' || c == ':') {
      ChannelAddPNode(ch, PNTYPE_END_OF_SENT, 1.0, NULL, NULL, p, p+1);
    } else if (c == '.' || c == '!' || c == '?') {
      if (eos == 0) {
        if (c == '.') eos = 1;
        else eos = 2;
        begin = p;
      }
    } else if (LexEntryNonwhite(c)) {
      if (eos == 2 && CharSentenceStart(c)) {
        ChannelAddPNode(ch, PNTYPE_END_OF_SENT, 1.0, NULL, NULL, begin, p);
      }
      eos = 0;
      begin = NULL;
      inword = 1;
    } else {
      inword = 0;
      if (eos) eos = 2;
    }
    p++;
  }
  if (eos) {
    ChannelAddPNode(ch, PNTYPE_END_OF_SENT, 1.0, NULL, NULL, begin, p);
  }
}

Bool TA_BlankLine(char *in, /* RESULTS */ Channel *ch, char **nextp)
{
  char		*orig_in;
  orig_in = in;
  if (StringLineIsAll(in, " \t=¦-_", &in)) {
    ChannelAddPNode(ch, PNTYPE_END_OF_SENT, 1.0, NULL, NULL, orig_in, in);
    *nextp = in;
    return(1);
  }
  return(0);
}

/* Detects separator lines other than an empty line.
 * (Used for detecting end of table.)
 * Examples:
 * =====================================================================
 *                =====================================
 * --------------------------------CUT--HERE--------------------------------
 * ¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦
 * ______________________________
 */
Bool TA_SeparatorLine(char *s, /* RESULTS */ char **table_end)
{
  if (s[0] == NEWLINE) return(0);
  if (StringLineIsAll(s, "=¦-_ CUTHER", table_end)) return(1);
  return(0);
}

/* End of file. */
