/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Prune parse nodes by calling an external Penn Treebank compatible
 * part-of-speech tagger.
 *
 * 19960201: begun
 * 19960202: more work
 *
 * Approximate Penn Treebank to ThoughtTreasure correspondences:
 * 
 * PTB  TT      description
 * --------------------------------------------------------------------------
 * CC	K	coordinating conjunction
 * CD	N	cardinal number
 * DT	D	determiner
 * EX	H	existential there
 * FW	9j	foreign word
 * IN	R/K	preposition or subordinating conjunction
 * JJ	A	adjective
 * JJR	A7	comparative adjective
 * JJS	A8	superlative adjective
 * LS	?	list item marker
 * MD	Vµ	modal
 * NN	NS	noun, singular or mass
 * NNS	NP	noun, plural
 * NNP	NS	proper noun, singular
 * NNPS	NP	proper noun, plural
 * PDT	D	predeterminer
 * POS	?	possessive ending
 * PP	H	personal pronoun
 * PRP	H	personal pronoun (including possessive pronouns mine, yours)
 * PP$	H	possessive pronoun = determiner (my, your, his)
 * PRP$	H	possessive pronoun = determiner (my, your, his)
 * RB	B	adverb
 * RBR	B7	comparative adverb
 * RBS	B8	superlative adverb
 * RP	9	particle
 * SYM	?	symbol (math or scientific)
 * TO	R	to
 * UH	U	interjection
 * VB	V	verb, base form
 * VBD	Vi	verb, past tense
 * VBG	Ve	verb, gerund or present participle
 * VBN	Vd	verb, past participle
 * VBP	Vp1	verb, present, non-third-person singular
 * VBZ	Vp3S	verb, present, third-person singular
 * WDT	D	wh-determiner
 * WP	H	wh-pronoun
 * WPZ	H	possessive wh-pronoun
 * WP$	H	possessive wh-pronoun
 * WRB	B	wh-adverb
 */

#include "tt.h"
#include "lexentry.h"
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

TaggerWords *TaggerWordsCreate()
{
  TaggerWords	*tw;
  tw = CREATE(TaggerWords);
  tw->maxlen = 128L;
  tw->len = 0L;
  tw->positions =
    (size_t *)MemAlloc(tw->maxlen*sizeof(size_t), "TaggerWords* positions");
  tw->firstchars =
    (char *)MemAlloc(tw->maxlen*sizeof(char), "TaggerWords* firstchars");
  return(tw);
}

void TaggerWordsFree(TaggerWords *tw)
{
  MemFree(tw->positions, "TaggerWords* positions");
  MemFree(tw, "TaggerWords");
}

void TaggerWordsAdd(TaggerWords *tw, size_t position, int firstchar)
{
  if (tw->len >= tw->maxlen) {
    tw->maxlen = tw->maxlen*2;
    tw->positions = (size_t *)MemRealloc(tw->positions,
                                         tw->maxlen*sizeof(size_t),
                                         "TaggerWords* positions");
    tw->firstchars = (char *)MemRealloc(tw->firstchars,
                                        tw->maxlen*sizeof(char),
                                        "TaggerWords* firstchars");
  }
  tw->positions[tw->len] = position;
  tw->firstchars[tw->len] = firstchar;
  tw->len++;
}

void TA_TaggerWriteSentence(FILE *stream, TaggerWords *tw, Channel *ch,
                            size_t lowerb, size_t upperb, int eoschar)
{
  int		c, prev, inword, first;
  size_t        pos;
  prev = TERM;
  inword = 0;
  first = 1;
  for (pos = lowerb; pos <= upperb; pos++) {
    c = *(((char *)ch->buf) + pos);
    if (LexEntryNonwhite(c)) {
      if (!inword) {
        if (first) {
          first = 0;
        } else {
          fputc(' ', stream);
        }
        if (prev == '\'') fputc(prev, stream);
        inword = 1;
        TaggerWordsAdd(tw, pos, c);
      }
      fputc(c, stream);
    } else {
      inword = 0;
      if (c == ',') fputs(" ,", stream);
    }
    prev = c;
  }
  fputc(' ', stream);
  fputc(eoschar, stream);
  TaggerWordsAdd(tw, pos, eoschar);
  fputc(NEWLINE, stream);
}

int TA_TaggerWriteInput(char *fn, TaggerWords *tw, Channel *ch)
{
  int		eoschar;
  size_t	pos;
  PNode		*pn;
  FILE		*stream;

  if (NULL == (stream = StreamOpen(fn, "w+"))) {
    return(0);
  }
  pos = 0;
  pn = ch->pnf->first;
  while (pos < ch->len) {
    if (NULL == (pn = PNodeGetNext(pn, pos))) break;
    if (pn->type == PNTYPE_END_OF_SENT) {
      eoschar = ch->buf[pn->lowerb];
      TA_TaggerWriteSentence(stream, tw, ch, pos, pn->lowerb, eoschar);
    }
    pos = pn->upperb+1;
  }
  StreamClose(stream);
  return(1);
}

int TA_TaggedWordRead(FILE *stream, /* RESULTS */ char *word, char *tag)
{
  int   c, pos;
  pos = 0;
  while (('/' != (c = getc(stream))) && (c != EOF)) {
    if (c == NEWLINE) continue;
    word[pos++] = c;
  }
  if (c == EOF) return(0);
  word[pos] = TERM;
  pos = 0;
  while (' ' != (c = getc(stream))) {
    tag[pos++] = c;
  }
  tag[pos] = TERM;
  return(1);
}


int PennIsPunct(char *tag)
{
  return(streq(tag, ":") || streq(tag, "."));
}

/* Only returns a non-F_NULL part of speech when it is unambiguous.
 * (cf IN which can be a preposition or a conjunction)
 */
int PennPOS(char *tag)
{
  if (streq(tag, "CC")) return(F_CONJUNCTION);
  else if (streq(tag, "CD")) return(F_NULL);
  else if (streq(tag, "DT")) return(F_DETERMINER);
  else if (streq(tag, "EX")) return(F_PRONOUN);
  else if (streq(tag, "FW")) return(F_NULL);
  else if (streq(tag, "IN")) return(F_NULL);
  else if (streq(tag, "JJ")) return(F_ADJECTIVE);
  else if (streq(tag, "JJR")) return(F_ADJECTIVE);
  else if (streq(tag, "JJS")) return(F_ADJECTIVE);
  else if (streq(tag, "LS")) return(F_NULL);
  else if (streq(tag, "MD")) return(F_VERB);
  else if (streq(tag, "NN")) return(F_NOUN);
  else if (streq(tag, "NNS")) return(F_NOUN);
  else if (streq(tag, "NNP")) return(F_NOUN);
  else if (streq(tag, "NNPS")) return(F_NOUN);
  else if (streq(tag, "PDT")) return(F_DETERMINER);
  else if (streq(tag, "POS")) return(F_ELEMENT);
    /* possessive 's */
  else if (streq(tag, "PP")) return(F_PRONOUN);
  else if (streq(tag, "PRP")) return(F_PRONOUN);
  else if (streq(tag, "PP$")) return(F_DETERMINER);
    /* e.g. my, your, his */
  else if (streq(tag, "PRP$")) return(F_DETERMINER);
    /* e.g. my, your, his */
  else if (streq(tag, "RB")) return(F_ADVERB);
  else if (streq(tag, "RBR")) return(F_ADVERB);
  else if (streq(tag, "RBS")) return(F_ADVERB);
  else if (streq(tag, "RP")) return(F_PREPOSITION);
    /* Yes, verb particles are prepositions (not adverbs) in ThoughtTreasure. */
  else if (streq(tag, "SYM")) return(F_NULL);
  else if (streq(tag, "TO")) return(F_PREPOSITION);
  else if (streq(tag, "UH")) return(F_INTERJECTION);
  else if (streq(tag, "VB")) return(F_VERB);
  else if (streq(tag, "VBD")) return(F_VERB);
  else if (streq(tag, "VBG")) return(F_VERB);
  else if (streq(tag, "VBN")) return(F_VERB);
  else if (streq(tag, "VBP")) return(F_VERB);
  else if (streq(tag, "VBZ")) return(F_VERB);
  else if (streq(tag, "WDT")) return(F_DETERMINER);
  else if (streq(tag, "WP")) return(F_PRONOUN);
  else if (streq(tag, "WPZ")) return(F_PRONOUN);
  else if (streq(tag, "WP$")) return(F_PRONOUN);
  else if (streq(tag, "WRB")) return(F_ADVERB);
  else if (streq(tag, ":")) return(F_NULL);
  else if (streq(tag, ".")) return(F_NULL);
  else {
    Dbg(DBGGEN, DBGBAD, "unknown Penn Treebank tag <%s>", tag);
    return(F_NULL);
  }
}

int PennTreebankTTCompat(char *tag, char *feat)
{
  int	pos, number, tense;
  pos = FeatureGet(feat, FT_POS);
  if (!FeatureMatch(PennPOS(tag), pos)) {
    if (streq(tag, "WRB") && pos == F_INTERJECTION) {
    /* This can be OK. cf <WRB> <Why>.<·Uz¸> (explanation-request) */
    } else {
      return(0);
    }
  }
  if (streq(tag, "CD")) {
    if (pos != F_NOUN && pos != F_ADJECTIVE) return(0);
  } else if (streq(tag, "IN")) {
    if (pos != F_PREPOSITION && pos != F_CONJUNCTION) return(0);
  }
  number = FeatureGet(feat, FT_NUMBER);
  if (streq(tag, "NN") && number != F_SINGULAR) return(0);
  if (streq(tag, "NNS") && number != F_PLURAL) return(0);
  if (streq(tag, "NNP") && number != F_SINGULAR) return(0);
  if (streq(tag, "NNPS") && number != F_PLURAL) return(0);
  tense = FeatureGet(feat, FT_TENSE);
  if (streq(tag, "VB") && tense != F_INFINITIVE) return(0);
  if (streq(tag, "VBD") && tense != F_IMPERFECT) return(0);
  if (streq(tag, "VBG") && tense != F_PRESENT_PARTICIPLE) return(0);
  if (streq(tag, "VBN") && tense != F_PAST_PARTICIPLE) return(0);
  if (streq(tag, "VBP") && tense != F_PRESENT) return(0);
  if (streq(tag, "VBZ") && tense != F_PRESENT) return(0);
  /* The VBP/VBZ distinction appears to be error-prone */
  /* comparative/superlative not used yet */
  return(1);
}

/* <nbest> is a sequence of Penn Treebank tags TAG_TAG_...TAG, where
 * the first tag is the most likely and the remaining tags are
 * other possible tags in no particular order.
 * <tt> are ThoughtTreasure features.
 * Returns a score multiplier. If no tags are compatible, returns 0.0.
 */
Float PennTreebankTTCompatNBest(char *nbest, char *tt)
{
  int	pos, tagnum;
  char	tagitem[16], *orig_nbest;
  orig_nbest = nbest;
  tagnum = 1;
  while (1) {
    pos = 0;
    while (*nbest && '_' != *nbest) {
      tagitem[pos++] = *nbest;
      nbest++;
    }
    tagitem[pos] = TERM;
    if (PennTreebankTTCompat(tagitem, tt)) {
/*
      Dbg(DBGGEN, DBGHYPER, "<%s> compatible with <%s>", orig_nbest, tt);
 */
      if (tagnum == 1) return(1.0);
      else return(0.75);
    }
    if (*nbest == TERM) break;
    nbest++;
    tagnum++;
  }
  Dbg(DBGGEN, DBGHYPER, "<%s> incompatible with <%s>", orig_nbest, tt);
  return(0.0);
}
 
/* Returns how many matching pns were found <r_cnt>, and how many of those
 * would be spliced out <r_spliced>.
 */
void TA_TaggerPrune2(PNode *pn, PNode *prev, int dosplice, size_t pos,
                     int firstchar, Channel *ch, char *word, char *tag,
                     /* RESULTS */ PNode **r_pn, PNode **r_prev,
                     int *r_cnt, int *r_spliced)
{
  int	cnt, spliced;
  Float	score;

  cnt = spliced = 0;
  for (; pn && pn->lowerb <= pos; pn = pn->next) {
/*
    Dbg(DBGGEN, DBGHYPER, "%ld %ld\n", pn->lowerb, pos);
 */
    if (pn->lowerb == pos &&
        pn->type == PNTYPE_LEXITEM &&
        pn->lexitem &&
        pn->lexitem->word &&
        (!PNodeIsPhrase(pn))) {
      if (firstchar != pn->lexitem->word[0]) {
        if (!dosplice) {
          Dbg(DBGGEN, DBGBAD, "TA_TaggerPrune1 firstchar mismatch <%c> <%s>",
              (char)firstchar, pn->lexitem->word);
        }
        prev = pn;
      } else if (0.0 ==
                 (score = PennTreebankTTCompatNBest(tag,
                            pn->lexitem->features))) {
        /* Splice out. */
        cnt++;
        spliced++;
        if (dosplice) {
          Dbg(DBGGEN, DBGDETAIL, "tag <%s> rules out <%s>.<%s>",
              tag, pn->lexitem->word, pn->lexitem->features);
          if (prev) {
            prev->next = pn->next;
            if (prev->next == NULL) ch->pnf->last = prev;
            /* todoFREE */
          } else {
            ch->pnf->first = pn->next;
            if (ch->pnf->first == NULL) ch->pnf->last = NULL;
          }  
        }
      } else {
        cnt++;
        if (dosplice && pn->score < 1.0) {
          pn->score = ScoreCombine(pn->score, score);
          Dbg(DBGGEN, DBGDETAIL,
              "nonbest tag <%s> modifies score of <%s>.<%s> to %d",
              tag, pn->lexitem->word, pn->lexitem->features, pn->score);
        }
        prev = pn;
      }
    } else {
      prev = pn;
    }
  }
  if (r_prev) *r_prev = prev;
  if (r_pn) *r_pn = pn;
  if (r_cnt) *r_cnt = cnt;
  if (r_spliced) *r_spliced = spliced;
}

int TA_TaggerPrune1(char *fn, TaggerWords *tw, Channel *ch)
{
  int		cnt, spliced;
  long		wordnum;
  char		word[PHRASELEN], tag[FEATLEN];
  size_t	pos;
  int		firstchar;
  FILE		*stream;
  PNode		*pn, *prev;

  if (NULL == (stream = StreamOpen(fn, "r"))) {
    return(0);
  }
  wordnum = 0L;
  pn = ch->pnf->first;
  prev = NULL;
  while (pn) {
    if (!TA_TaggedWordRead(stream, word, tag)) break;
    Dbg(DBGGEN, DBGHYPER, "<%s>.<%s>", word, tag);
    if (wordnum >= tw->len) {
      Dbg(DBGGEN, DBGBAD, "TA_TaggerPrune1 length mismatch");
      break;
    }
    if (!PennIsPunct(tag)) {
      pos = tw->positions[wordnum];
      firstchar = tw->firstchars[wordnum];
      TA_TaggerPrune2(pn, prev, 0, pos, firstchar, ch, word, tag, NULL, NULL,
                      &cnt, &spliced);
      if ((spliced > 0) &&
          (spliced < cnt) && /* Don't rule out all words. */
          (cnt > 1)) { /* Don't rule out the only instance of a word. */
        TA_TaggerPrune2(pn, prev, 1, pos, firstchar, ch, word, tag, &pn, &prev,
                        NULL, NULL);
      }
    }
    wordnum++;
  }

  StreamClose(stream);
  return(1);
}

/* Top-level function of this file. */
void TA_TaggerPrune(Channel *ch)
{
  TaggerWords	*tw;
  char		infn[FILENAMELEN], outfn[FILENAMELEN], cmd[96];

  Dbg(DBGGEN, DBGDETAIL, "TAGGER PRUNE BEGIN");

  tw = TaggerWordsCreate();

  FileTemp("/tmp/Tag.In", FILENAMELEN, infn);
  FileTemp("/tmp/Tag.Out", FILENAMELEN, outfn);

  /* (1) Write all tokens in <ch->buf> to file in Penn Treebank format. */
  if (!TA_TaggerWriteInput(infn, tw, ch)) {
    goto done1;
  }

  /* (2) Run the tagger. */
  sprintf(cmd, "./tagger %s %s", infn, outfn);
  Dbg(DBGGEN, DBGDETAIL, "invoking: %s", cmd);
  if (0 > system(cmd)) {
    Dbg(DBGGEN, DBGBAD, "system: <%s> failed", cmd);
    goto done;
  }
  Dbg(DBGGEN, DBGDETAIL, "done running: %s", cmd);

  /* (3) Use the output of the tagger to prune <ch->pnf> */
  if (!TA_TaggerPrune1(outfn, tw, ch)) {
    goto done;
  }

done:
  unlink(infn);
  unlink(outfn);
done1:
  TaggerWordsFree(tw);

  Dbg(DBGGEN, DBGDETAIL, "TAGGER PRUNE END");
}

/* End of file. */
