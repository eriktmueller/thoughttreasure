/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19941208: begun
 * 19941217: more work
 * 19941222: names
 * 19950202: more
 * 19950206: polities, media objects, redid names
 * 19950207: more name work
 * 19950209: TA_Scan
 * 19950726: TA_Product
 * 19950802: TA_Product redone
 *
 * todo:
 * - Parse and generate numbers: "fifty five" <=> 55, "42nd", "82ème".
 *   Convert into canonical units.
 * - Cope with _pseudo-underlining_, *asterisks emphasis*, and SHOUTING.
 *   (SHOUTING may already be handled by spell correction mechanism.)
 * - Parse .sig lines and mailing addresses.
 * - Parse parentheticals which can be relative clauses or separate
 *   sentences.
 * - Recognize bullet (or dot or dash) lists.
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

/*
#define TASLOW
*/

/* Attribution */

Attribution *AttributionCreate(Obj *speaker, Ts *ts, char *emmn)
{
  Attribution	*a;
  a = CREATE(Attribution);
  a->speaker = speaker;
  a->ts = *ts;
  if (emmn) {
    StringCpy(a->emmn, emmn, EMMN_LEN);
  } else {
    a->emmn[0] = TERM;
  }
  return(a);
}

void AttributionPrint(FILE *stream, Attribution *a)
{
  fputc('<', stream);
  TsPrint(stream, &a->ts);
  fprintf(stream, ">SPEAKER<%s>", M(a->speaker));
}

/******************************************************************************
 * Utilities
 ******************************************************************************/

Bool TA_ValueName1(char *phrase, Obj *class, HashTable *ht,
                   /* RESULTS */ Float *value)
{
  int		freeme;
  IndexEntry	*ie, *orig_ie;
  LexEntryToObj	*leo;
  Float		val;
  if (!(ie = orig_ie = LexEntryFindPhrase(ht, phrase, 2, 0, 0, &freeme))) {
    return(0);
  }
  for (; ie; ie = ie->next) {
    for (leo = ie->lexentry->leo; leo; leo = leo->next) {
      if (ISA(class, leo->obj)) {
        if (FLOATNA !=
            (val = ObjToNumber(R1EI(2, &TsNA, L(N("value-of"), leo->obj,
                                    ObjWild, E))))) {
          *value = val;
          if (freeme) IndexEntryFree(orig_ie);
          return(1);
        }
      }
    }
  }
  if (freeme) IndexEntryFree(orig_ie);
  return(0);
}

Bool TA_ValueName(char *phrase, Obj *class, /* RESULTS */ Float *value)
{
  if (TA_ValueName1(phrase, class, EnglishIndex, value)) return(1);
  if (TA_ValueName1(phrase, class, FrenchIndex, value)) return(1);
  return(0);
}

Bool TA_ValueNameInt(char *phrase, Obj *class, /* RESULTS */ int *value)
{
  Float	value1;
  if (TA_ValueName1(phrase, class, EnglishIndex, &value1)) {
    *value = (int)value1;
    return(1);
  }
  if (TA_ValueName1(phrase, class, FrenchIndex, &value1)) {
    *value = (int)value1;
    return(1);
  }
  return(0);
}

Bool TA_ValueNameTod(char *phrase, Obj *class, /* RESULTS */ Tod *value)
{
  Float	value1;
  if (TA_ValueName1(phrase, class, EnglishIndex, &value1)) {
    *value = (Tod)value1;
    return(1);
  }
  if (TA_ValueName1(phrase, class, FrenchIndex, &value1)) {
    *value = (Tod)value1;
    return(1);
  }
  return(0);
}

Bool TA_ValueRangeName1(char *phrase, Obj *class, HashTable *ht,
                        /* RESULTS */ Float *min_value, Float *max_value)
{
  int		freeme;
  IndexEntry	*ie, *orig_ie;
  LexEntryToObj	*leo;
  Float		minval, maxval;
  if (!(ie = orig_ie = LexEntryFindPhrase(ht, phrase, 2, 0, 0, &freeme))) {
    return(0);
  }
  for (; ie; ie = ie->next) {
    for (leo = ie->lexentry->leo; leo; leo = leo->next) {
      if (ISA(class, leo->obj)) {
        minval = ObjToNumber(R1EI(2, &TsNA,
                             L(N("min-value-of"), leo->obj, ObjWild, E)));
        maxval = ObjToNumber(R1EI(2, &TsNA,
                             L(N("max-value-of"), leo->obj, ObjWild, E)));
        if (FLOATNA != minval && FLOATNA != maxval) {
          *min_value = minval;
          *max_value = maxval;
          if (freeme) IndexEntryFree(orig_ie);
          return(1);
        }
      }
    }
  }
  if (freeme) IndexEntryFree(orig_ie);
  return(0);
}

#ifdef notdef
/* Not yet used. Intended for use by TA_Time. */
Bool TA_DualValueRangeName1(char *phrase, Obj *class, HashTable *ht,
                            /* RESULTS */ Float *min_value1, Float *max_value1,
                            Float *min_value2, Float *max_value2)
{
  int		freeme;
  IndexEntry	*ie, *orig_ie;
  LexEntryToObj	*leo;
  Float		minval1, maxval1, minval2, maxval2;
  if (!(ie = orig_ie = LexEntryFindPhrase(ht, phrase, 2, 0, 0, &freeme))) {
    return(0);
  }
  for (; ie; ie = ie->next) {
    for (leo = ie->lexentry->leo; leo; leo = leo->next) {
      if (ISA(class, leo->obj)) {
        minval1 = ObjToNumber(R1EI(2, &TsNA,
                                L(N("min-value1-of"), leo->obj, ObjWild, E)));
        maxval1 = ObjToNumber(R1EI(2, &TsNA,
                                L(N("max-value1-of"), leo->obj, ObjWild, E)));
        minval2 = ObjToNumber(R1EI(2, &TsNA,
                                L(N("min-value2-of"), leo->obj, ObjWild, E)));
        maxval2 = ObjToNumber(R1EI(2, &TsNA,
                                L(N("max-value2-of"), leo->obj, ObjWild, E)));
        if (FLOATNA != minval1 && FLOATNA != maxval1 &&
            FLOATNA != minval2 && FLOATNA != maxval2) {
          *min_value1 = minval1;
          *max_value1 = maxval1;
          *min_value2 = minval2;
          *max_value2 = maxval2;
          if (freeme) IndexEntryFree(orig_ie);
          return(1);
        }
      }
    }
  }
  if (freeme) IndexEntryFree(orig_ie);
  return(0);
}
#endif

/* Indicates whether a LexEntryGetLeo concept should be filtered out
 * because it is handled by other than LexEntry (word) TAs.
 * So far, this seems only to make sense (or matter) for nouns.
 */
Bool TA_FilterOut(Obj *obj, char *feat)
{
  return((!FeatureTaskOK(feat, N("parse"))) ||
         ISA(N("title"), obj) ||
         ISA(N("human-name"), obj));
  /* Was || (ObjIsConcrete(obj) && ISA(N("media-object"), obj))
   * but PNodeListOverrideLower(ch->pnf, PNTYPE_MEDIA_OBJ)
   * should do the trick and also not have the problem of eliminating
   * media objects which are not enclosed in {} .
   */
}

Bool LexEntryFilterOut(LexEntry *le)
{
  LexEntryToObj	*leo;
  if (StringIn(F_EXPLETIVE, le->features)) return(0);

  /* Permit recognition of new words: */
  if (le->leo == NULL) return(0);

  for (leo = le->leo; leo; leo = leo->next) {
    if (!TA_FilterOut(leo->obj, leo->features)) return(0);
  }
  return(1);
}

Bool IndexEntryAllFilterOut(IndexEntry *ie)
{
  for (; ie; ie = ie->next) {
    if (ie->lexentry) {
      if (!LexEntryFilterOut(ie->lexentry)) return(0);
    }
  }
  return(1);
}

ObjList *TA_FindPhrase(HashTable *ht, char *phrase, Obj *class)
{
  int			freeme;
  IndexEntry	*ie, *orig_ie;
  ObjList		*r;
  LexEntryToObj	*leo;
  r = NULL;
  for (ie = orig_ie = LexEntryFindPhrase(ht, phrase, INTPOSINF, 0, 0, &freeme);
       ie; ie = ie->next) {
    for (leo = ie->lexentry->leo; leo; leo = leo->next) {
      if (!ISA(class, leo->obj)) continue;
      if (!ObjListIn(leo->obj, r)) r = ObjListCreate(leo->obj, r);
    }
  }
  if (freeme) IndexEntryFree(orig_ie);
  return(r);
}

ObjList *TA_FindPhraseTryBoth(HashTable *ht, char *phrase, Obj *class)
{
  ObjList	*r;
  if ((r = TA_FindPhrase(ht, phrase, class))) return(r);
  if ((r = TA_FindPhrase(LexEntryHtFlip(ht), phrase, class))) return(r);
  return(NULL);
}

ObjList *TA_FindPhraseBoth(char *phrase, Obj *class)
{
  ObjList	*r;
  if ((r = TA_FindPhrase(FrenchIndex, phrase, class))) return(r);
  if ((r = TA_FindPhrase(EnglishIndex, phrase, class))) return(r);
  return(NULL);
}

ObjList *TA_FindPhraseForwards(char *s, Obj *superclass, int numwords,
                               Discourse *dc, /* RESULTS */ char **nextp)
{
  char		phrase[PHRASELEN], punc[PUNCLEN], *rest;
  ObjList	*classes;
  if (StringGetNWords_LeNonwhite(phrase, punc, s, PHRASELEN, numwords, &rest)) {
    StringElims(phrase, LE_NONWHITESPACE, NULL);
      /* todo: Inelegant. To remove '.'. */
    if ((classes = TA_FindPhrase(DC(dc).ht, phrase, superclass))) {
      if (nextp) *nextp = rest;
      return(classes);
    }
  }
  return(NULL);
}

Obj *TA_FindPhraseForward(char *s, Obj *superclass, int numwords,
                          Discourse *dc, /* RESULTS */ char **nextp)
{
  ObjList	*classes;
  Obj		*class;
  if ((classes = TA_FindPhraseForwards(s, superclass, numwords, dc, nextp))) {
    class = classes->obj;
    ObjListFree(classes);
    return(class);
  }
  return(NULL);
}

Obj *TA_FindPhraseBackward(char *start, char *base,
                           Obj *superclass, int numwords, Discourse *dc)
{
  char		phrase[PHRASELEN], *dummy;
  ObjList	*classes;
  Obj		*class;
  if (StringGetNWords_LeN_Back(phrase, start-1, base, PHRASELEN, numwords,
                               &dummy)) {
    StringElims(phrase, LE_NONWHITESPACE, NULL);
      /* todo: Inelegant. To remove '.'. */
    if ((classes = TA_FindPhrase(DC(dc).ht, phrase, superclass))) {
      class = classes->obj;
      ObjListFree(classes);
      return(class);
    }
  }
  return(NULL);
}

Obj *TA_FindNearbyClassClue(char *start, char *end, char *base,
                            Obj *superclass, Discourse *dc)
{
  Obj	*class;
  if ((class = TA_FindPhraseBackward(start, base, superclass, 2, dc))) {
    return(class);
  }
  if ((class = TA_FindPhraseBackward(start, base, superclass, 1, dc))) {
    return(class);
  }
  if ((class = TA_FindPhraseForward(end, superclass, 2, dc, NULL))) {
    return(class);
  }
  if ((class = TA_FindPhraseForward(end, superclass, 1, dc, NULL))) {
    return(class);
  }
  return(NULL);
}

/******************************************************************************
 * Apply TAs to Channel.
 ******************************************************************************/

/* This function only includes the TAs relevant to corpus articles. */
Bool TA_StartLineArticle(char *in, Discourse *dc, /* RESULTS */ Channel *ch,
                         char **nextp)
{
#ifdef TASLOW
  /* This is slow. */
  if (TA_EmailParseHeader(in, dc, ch, nextp)) return(1);
  else if (TA_EmailParseAttribution(in, dc, ch, nextp)) return(1);
    /* todo: Only parse attributions if inside email? */
#endif
  return(0);
}

void TA_ScanLineBegin(Channel *ch, Discourse *dc)
{
  char	*p, *rest;
  p = (char *)ch->buf;
  while (*p) {
    if (p == (char *)ch->buf || *(p-1) == NEWLINE) {
/*
   This does not work. The code is not set up to deal with more
   than one PNTYPE_END_OF_SENT per sentence.
      if (TA_BlankLine(p, ch, &rest)) p = rest;
 */
      if (TA_EmailParseHeader(p, dc, ch, &rest)) p = rest;
      else if (TA_EmailParseAttribution(p, dc, ch, &rest)) p = rest;
        /* todo: Only parse attributions if inside email? */
      else if (TA_TableParse(p, TABSIZE_UNIX, dc, ch, &rest)) p = rest;
      else p++;
    } else {
      p++;
    }
  }
}

void TA_ScanWordBegin(Channel *ch, Discourse *dc)
{
  char	*p, *rest;
  int	cur, prev;
  prev = TERM;
  p = (char *)ch->buf;
  while (*p) {
    cur = *((uc *)p);
    if (LexEntryNonwhite(cur) && (!LexEntryNonwhite(prev))) {
      if (TA_Name(p, dc, ch, &rest)) {
        prev = ' ';
        p = rest;
      } else if (TA_TsRange(p, dc, ch, &rest)) {
        prev = ' ';
        p = rest;
      } else if (TA_Product(p, (char *)ch->buf, dc, ch, &rest)) {
        prev = ' ';
        p = rest;
      } else {
        prev = cur;
        p++;
      }
    } else {
      prev = cur;
      p++;
    }
  }
}

void TA_ScanAnywhere(Channel *ch, Discourse *dc)
{
  char	*p, *rest;
  for (p = (char *)ch->buf; *p; ) {
    if (TA_MediaObject(p, (char *)ch->buf, dc, ch, &rest)) p = rest;
    else p++;
  }
  for (p = (char *)ch->buf; *p; ) {
    if (TA_Price(p, (char *)ch->buf, dc, ch, &rest)) p = rest;
    else p++;
  }
  for (p = (char *)ch->buf; *p; ) {
    if (TA_Telno(p, dc, ch, &rest)) p = rest;
    else p++;
  }
/* MRBEGIN */
  for (p = (char *)ch->buf; *p; ) {
    if (TA_StarRating(p, dc, ch, &rest)) p = rest;
    else p++;
  }
/* MREND */
#ifdef TASLOW
  /* This is very slow. */
  for (p = (char *)ch->buf; *p; ) {
    if (TA_Communicon(p, dc, ch, &rest)) p = rest;
    else p++;
  }
#endif
#ifndef TASLOW
  if (DC(dc).lang == F_FRENCH) {
#endif
    for (p = (char *)ch->buf; *p; ) {
      if (TA_FrenchPolity(p, dc, ch, &rest)) p = rest;
      else p++;
    }
#ifndef TASLOW
  }
#endif
}

/* "8 o'clock" "5 seconds" */
void TA_ScanPNodePatterns1(Channel *ch, Discourse *dc)
{
  PNode		*p;
  size_t	lowerb, stoppos;
  for (p = ch->pnf->first; p; ) {
    if (TA_Time_Tod(p, p->lowerb, dc, ch, &stoppos)) {
    /* Experimental skipping past. cf "half past seven" locks out "seven" 
     * Perhaps also incorporate this into TA_Time_Dur (and others?).
     */
      while (p && p->lowerb < stoppos) {
        p = p->next;
      }
      continue;
    }
    TA_Time_Dur(p, p->lowerb, dc, ch);
    /* skip <p> to a higher lowerb */
    lowerb = p->lowerb;
    p = p->next;
    while (p && lowerb == p->lowerb) {
      p = p->next;
    }
  }
}

/* "about 8 o'clock" */
void TA_ScanPNodePatterns2(Channel *ch, Discourse *dc)
{
  PNode		*p;
  size_t	lowerb;
  for (p = ch->pnf->first; p; ) {
    TA_Time_Abouts(p, p->lowerb, dc, ch);
    /* skip <p> to a higher lowerb */
    lowerb = p->lowerb;
    p = p->next;
    while (p && lowerb == p->lowerb) {
      p = p->next;
    }
  }
}

/* "January 8, 1988 at about 8 o'clock" */
void TA_ScanPNodePatterns3(Channel *ch, Discourse *dc)
{
  PNode		*p;
  size_t	lowerb;
  for (p = ch->pnf->first; p; ) {
    TA_Time_MergeDateTod(p, p->lowerb, dc, ch);
    /* skip <p> to a higher lowerb */
    lowerb = p->lowerb;
    p = p->next;
    while (p && lowerb == p->lowerb) {
      p = p->next;
    }
  }
}

/* "between January 8, 1988 at about 8 o'clock and January 12, 1988 at
 * 9 o'clock".
 */
void TA_ScanPNodePatterns4(Channel *ch, Discourse *dc)
{
  PNode		*p;
  size_t	lowerb;
  for (p = ch->pnf->first; p; ) {
    TA_Time_Combinations(p, p->lowerb, dc, ch);
    /* skip <p> to a higher lowerb */
    lowerb = p->lowerb;
    p = p->next;
    while (p && lowerb == p->lowerb) {
      p = p->next;
    }
  }
}

void TA_Scan(Channel *ch, Discourse *dc)
{
  char	*p;
  p = (char *)ch->buf;
  ch->buf[ch->len] = TERM;
  TA_ScanAnywhere(ch, dc);
  TA_LexEntry(ch, dc);
  TA_ScanWordBegin(ch, dc);
  TA_ScanLineBegin(ch, dc);
  TA_EndOfSentence(ch, dc);
  TA_ScanPNodePatterns1(ch, dc);
  TA_ScanPNodePatterns2(ch, dc);
  TA_ScanPNodePatterns3(ch, dc);
  TA_ScanPNodePatterns4(ch, dc);
/*
  ChannelPrintPNodes(Log, ch);
 */
  if (DbgOn(DBGGEN, DBGDETAIL)) {
    Dbg(DBGGEN, DBGDETAIL, "BEFORE OVERRIDES:");
    ChannelPrintHighlights(Log, ch);
  }

  PNodeListOverrideLower(ch->pnf, PNTYPE_TABLE);
  PNodeListOverrideLower(ch->pnf, PNTYPE_EMAILHEADER);
  PNodeListOverrideLower(ch->pnf, PNTYPE_ATTRIBUTION);
  PNodeListOverrideLower(ch->pnf, PNTYPE_MEDIA_OBJ);
  PNodeListOverrideLower(ch->pnf, PNTYPE_PRODUCT);
  PNodeListOverrideLower(ch->pnf, PNTYPE_TSRANGE);

  PNodeListOverride(ch->pnf, PNTYPE_NAME, PNTYPE_END_OF_SENT);
    /* So that "Mrs." is not considered end of sentence. */

  if (!(dc->mode & DC_MODE_COMPOUND_NOUN)) {
    PNodeListPhraseOverride(ch->pnf);	/* todoSCORE */
  }

  TA_EmailProcessPostHeaders(ch);

  /* todo: Optional. */
  if (DC(dc).lang == F_ENGLISH) {
/*
    TA_TaggerPrune(ch);
 */
  }

  if (DbgOn(DBGGEN, DBGDETAIL)) {
    Dbg(DBGGEN, DBGDETAIL, "AFTER OVERRIDES:");
    ChannelPrintHighlights(Log, ch);
  }
/*
  ChannelPrintPNodesOfType(Log, ch, PNTYPE_MEDIA_OBJ);
  ChannelPrintPNodesOfType(Log, ch, PNTYPE_PRODUCT);
  ChannelPrintPNodesOfType(Log, ch, PNTYPE_NUMBER);
  ChannelPrintPNodesOfType(Log, ch, PNTYPE_EMAILHEADER);
 */

  TA_ProductTrade(ch->pnf->first);
}

/* End of file. */
