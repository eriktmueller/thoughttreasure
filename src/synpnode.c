/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19941213: added old Messages
 * 19950209: merged old Messages into PNodes
 * 19950320: added punctuation
 * 19950414: PNodeList kept sorted
 * 19981116: PNodeSocketPrint
 */

#include "tt.h"
#include "lexentry.h"
#include "lexitem.h"
#include "lexmorph.h"
#include "lextheta.h"
#include "repbasic.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptext.h"
#include "reptime.h"
#include "semdisc.h"
#include "semgen1.h"
#include "semgen2.h"
#include "synfilt.h"
#include "synparse.h"
#include "synpnode.h"
#include "synxbar.h"
#include "ta.h"
#include "taemail.h"
#include "taname.h"
#include "tatable.h"
#include "toolsvr.h"
#include "utildbg.h"

/* PNodeList */

PNodeList *PNodeListCreate()
{
  PNodeList	*pnf;
  pnf = CREATE(PNodeList);
  pnf->first = NULL;
  pnf->last = NULL;
  pnf->cache = NULL;
  return(pnf);
}

void PNodeListFree(PNodeList *pnf)
{
  if (pnf->first) Dbg(DBGGEN, DBGBAD, "PNodeListFree: 1");
  if (pnf->last) Dbg(DBGGEN, DBGBAD, "PNodeListFree: 2");
  MemFree(pnf, "PNodeList");
}

Bool PNodeListIsEmpty(PNodeList *pnf)
{
  return(pnf->first == NULL);
}

Bool PNodeListAddAfter(PNodeList *pnf, PNode *searchfrom, PNode *m)
{
  PNode	*pn, *prev;
  if (!searchfrom) return(0);
  if (m->lowerb < searchfrom->lowerb) {
  /* Example: m = 22, searchfrom = 45->67->77->77->79
   * If searchfrom is pnf->cache, caller should try again with pnf->first.
   * If searchfrom is pnf->first, then caller should add m to head of pnf.
   */
    return(0);
  }
  /* Examples: m = 22, searchfrom = 22->22->22->27->32->45->67
   *           m = 23, searchfrom = 22->27->32->45->67
   *           m = 100, searchfrom = 22->27->32->45->67
   */
  prev = searchfrom;
  for (pn = searchfrom->next; pn; pn = pn->next) {
    if (m->lowerb < pn->lowerb) {
    /* Insert <m> before <pn>:
     *   22(prev)--------->27(pn)
     *            23(m)
     */
      m->next = prev->next;
      prev->next = m;
      pnf->cache = m;
      return(1);
    }
    prev = pn;
  }
  /* Add <m> to tail. */
  pnf->last->next = m;
  pnf->last = m;
  m->next = NULL;
  pnf->cache = m;
  return(1);
}

/* Add <m> to <pnf>. Keep <pnf> sorted by increasing lowerb. */
void PNodeListAdd(PNodeList *pnf, Channel *ch, PNode *m, int dbg)
{
#ifdef maxchecking
  if (pnf == NULL) {
     Dbg(DBGGEN, DBGBAD, "PNodeListAdd: pnf NULL");
     return;
  }
#endif
  if (pnf->first == NULL) {
  /* <pnf> is of length 0. */
    pnf->first = pnf->last = m;
    m->next = NULL;
    pnf->cache = m;
    goto done;
  }
  if (PNodeListAddAfter(pnf, pnf->cache, m)) goto done;
  if (PNodeListAddAfter(pnf, pnf->first, m)) goto done;
  /* Add <m> to head. */
  m->next = pnf->first;
  pnf->first = m;
  pnf->cache = m;
done:
  /* <here> */
  if (dbg && DbgOn(DBGGEN, DBGDETAIL)) {
    PNodePrint(Log, ch, m);
  }
}

/* Used by Corpus. */
PNode *PNodeListRemove(PNodeList *pnf)
{
  PNode	*m;
  pnf->cache = NULL;
  if (pnf->first) {
    m = pnf->first;
    if (pnf->first == pnf->last) {
      pnf->first = pnf->last = NULL;
    } else {
      pnf->first = m->next;
      m->next = NULL;
    }
    return(m);
  }
  return(NULL);
}

Bool PNodeWithin(PNode *pn, size_t lowerb, size_t upperb,
                 size_t lowerb_subj, size_t upperb_subj)
{
  if (lowerb_subj != SIZENA) {
    if (pn->lowerb >= lowerb_subj &&
        pn->lowerb <= upperb_subj &&
        pn->upperb >= lowerb_subj &&
        pn->upperb <= upperb_subj) {
    /* Subject lines of email are exempt from this. */
      return(0);
    }
  }
  return(pn->lowerb >= lowerb &&
         pn->lowerb <= upperb &&
         pn->upperb >= lowerb &&
         pn->upperb <= upperb);
}

void PNodeListSpliceOutWithinLower(PNodeList *pnf, int type, size_t lowerb,
                                   size_t upperb, size_t lowerb_subj,
                                   size_t upperb_subj)
{
  PNode	*p, *prev;
  prev = NULL;
  for (p = pnf->first; p; p = p->next) {
    if (p->type < type && PNodeWithin(p, lowerb, upperb, lowerb_subj,
                                      upperb_subj)) {
      /* Splice out. */
      if (prev) {
        prev->next = p->next;
        if (prev->next == NULL) pnf->last = prev;
        /* todoFREE */
      } else {
        pnf->first = p->next;
        if (pnf->first == NULL) pnf->last = NULL;
      }
    } else prev = p;
  }
}

void PNodeListOverrideLower(PNodeList *pnf, int type)
{
  PNode	*p;
  for (p = pnf->first; p; p = p->next) {
    if (p->type == type) {
      PNodeListSpliceOutWithinLower(pnf, type, p->lowerb, p->upperb,
                                    p->lowerb_subj, p->upperb_subj);
    }
  }
}

void PNodeListSpliceOutType(PNodeList *pnf, int type, size_t lowerb,
                            size_t upperb, size_t lowerb_subj,
                            size_t upperb_subj)
{
  PNode	*p, *prev;
  prev = NULL;
  for (p = pnf->first; p; p = p->next) {
    if (p->type == type && PNodeWithin(p, lowerb, upperb, lowerb_subj,
                                       upperb_subj)) {
      /* Splice out. */
      if (prev) {
        prev->next = p->next;
        if (prev->next == NULL) pnf->last = prev;
        /* todoFREE */
      } else {
        pnf->first = p->next;
        if (pnf->first == NULL) pnf->last = NULL;
      }
    } else prev = p;
  }
}

/* Currently used for names. Proper "within":
 * A period at the beginning or end of a name shouldn't cause EOS
 * to be overrided.
 */
void PNodeListOverride(PNodeList *pnf, int type1, int type2)
{
  PNode	*p;
  for (p = pnf->first; p; p = p->next) {
    if (p->type == type1) {
      PNodeListSpliceOutType(pnf, type2, p->lowerb+1L, p->upperb-1L,
                             p->lowerb_subj, p->upperb_subj);
    }
  }
}

void PNodeListPhraseOverride1(PNodeList *pnf, size_t lowerb, size_t upperb,
                              int len)
{
  PNode	*p, *prev;
  prev = NULL;
  for (p = pnf->first; p; p = p->next) {
    if (p->type == PNTYPE_LEXITEM && p->lexitem &&
        (LexEntryNumberOfWords(p->lexitem->le) < len) &&
        PNodeWithin(p, lowerb, upperb, SIZENA, SIZENA)) {
      /* Splice out. */
      if (prev) {
        prev->next = p->next;
        if (prev->next == NULL) pnf->last = prev;
        /* todoFREE */
      } else {
        pnf->first = p->next;
        if (pnf->first == NULL) pnf->last = NULL;
      }
    } else prev = p;
  }
}

/* For phrases longer than 2 words, longer phrases override shorter ones. */
void PNodeListPhraseOverride(PNodeList *pnf)
{
  int	len;
  PNode	*p;
  for (p = pnf->first; p; p = p->next) {
    if (p->type == PNTYPE_LEXITEM && p->lexitem &&
        ((len = LexEntryNumberOfWords(p->lexitem->le)) > 2)) {
      PNodeListPhraseOverride1(pnf, p->lowerb, p->upperb, len);
    }
  }
}

void PNodeListSpliceOut(PNodeList *pnf, PNode *spliceout_pn)
{
  PNode	*p, *prev;
  prev = NULL;
  for (p = pnf->first; p; p = p->next) {
    if (p == spliceout_pn) {
      /* Splice out. */
      if (prev) {
        prev->next = p->next;
        if (prev->next == NULL) pnf->last = prev;
        /* todoFREE */
      } else {
        pnf->first = p->next;
        if (pnf->first == NULL) pnf->last = NULL;
      }
      break;
    } else prev = p;
  }
}

/* Used by Corpus. */
void PNodeListAppend(PNodeList *pnf, PNodeList *pnf2, Channel *ch)
{
  PNode	*m;
  while ((m = PNodeListRemove(pnf2))) {
    PNodeListAdd(pnf, ch, m, 0);
  }
}

/* Get next sentence or block (communicon, attribution, email header, table). */
PNode *PNodeGetNext(PNode *pn, size_t after)
{
  size_t	lowest_upperb;
  PNode		*lowest_pn;
  lowest_upperb = SIZEPOSINF;
  lowest_pn = NULL;
  for (; pn; pn = pn->next) {
    if (pn->type == PNTYPE_END_OF_SENT ||
        ((pn->type >= PNTYPE_MIN_BLOCK &&
          pn->type <= PNTYPE_MAX_BLOCK))) {
      if (pn->lowerb >= after &&
          pn->upperb < lowest_upperb) {
        lowest_upperb = pn->upperb;
        lowest_pn = pn;
      }
    }
  }
  return(lowest_pn);
}

/* PNode */

PNode *PNodeCreate(char feature, Lexitem *lexitem, PNode *pn1, PNode *pn2,
                   size_t lowerb, size_t upperb, Obj *obj, PNode *next)
{
  PNode *pn;
  pn = CREATE(PNode);
  pn->type = PNTYPE_NA;
  pn->score = SCORE_MAX;
  pn->concepts = NULL;
  pn->concepts_parent = NULL;
  pn->allcons = NULL;
  pn->eos = EOSNA;
  pn->gender = pn->number = pn->person = F_NULL;
  pn->num = PNUMNA;
  pn->feature = feature;
  pn->lexitem = lexitem;
  pn->obj = obj;
  pn->ole = NULL;
  pn->punc[0] = TERM;
  pn->is_coord = 0;
  pn->pn1 = pn1;
  pn->pn2 = pn2;
  pn->u.na = NULL;
  pn->lowerb = lowerb;
  pn->upperb = upperb;
  pn->lowerb_subj = SIZENA;
  pn->upperb_subj = SIZENA;
  pn->versuslen = 0;
  pn->versusmaxlen = 0;
  pn->versus = NULL;
  pn->appliedsingletons = 0;
  pn->next = next;
  pn->next_altern = NULL;
  return(pn);
}

PNode *PNodeCreate1()
{
  return(PNodeCreate(F_NULL, NULL, NULL, NULL, 0L, 0L, NULL, NULL));
}

PNode *PNodeCreateString(char *s)
{
  PNode	*pn;
  pn = PNodeCreate1();
  pn->type = PNTYPE_STRING;
  pn->u.s = s;
  return(pn);
}

PNode *PNodeCopy(PNode *pn, PNode *next)
{
  PNode	*new;
  new = CREATE(PNode);
  *new = *pn;	/* todoFREE */
  new->next = next;
  return(new);
}

Bool PNodeAddPuncToEnd(PNode *pn, char *punc)
{
  if (pn == NULL) return(0);
  if (pn->type != PNTYPE_CONSTITUENT) {
    if (pn->lexitem && StringIn(F_ELISION, pn->lexitem->features) &&
        StringIn(SQUOTE, punc)) {
      return(1);
    }
    StringCpy(pn->punc, punc, PUNCLEN);
    return(1);
  } else if (pn->type == PNTYPE_NAME) {
    StringCpy(pn->punc, punc, PUNCLEN);
    return(1);
  }
  if (pn->pn2 && PNodeAddPuncToEnd(pn->pn2, punc)) return(1);
  return(PNodeAddPuncToEnd(pn->pn1, punc));
}

char *PNodeGetEndPunc(PNode *pn)
{
  char	*s;
  if (pn == NULL) return(NULL);
  if (pn->type != PNTYPE_CONSTITUENT) {
    return(pn->punc);
  }
  if (pn->pn2 && (s = PNodeGetEndPunc(pn->pn2))) return(s);
  return(PNodeGetEndPunc(pn->pn1));
}

Bool PNodeEndGroupingPunc(PNode *pn)
{
  return(StringAnyIn(GROUPPUNC, PNodeGetEndPunc(pn)));
}

PNode *PNodeFlattenAltern1(PNode *pn)
{
  PNode	*pn1, *pn2, *p, *q, *r, *r1;
  if (pn == NULL) return(NULL);
  if (pn->type != PNTYPE_CONSTITUENT) {
    return(PNodeCopy(pn, NULL));
  }
  pn1 = PNodeFlattenAltern(pn->pn1);
  pn2 = PNodeFlattenAltern(pn->pn2);
  if (pn1 && pn2) {
    r = NULL;
    for (p = pn1; p; p = p->next_altern) {
      for (q = pn2; q; q = q->next_altern) {
        r1 = PNodeConstit(pn->feature, p, q);
        r1->next_altern = r;
        r = r1;
      }
    }
    PNodeBreakup(pn1);
    PNodeBreakup(pn2);
    return(r);
  }
  r = NULL;
  for (p = pn1; p; p = p->next_altern) {
    r1 = PNodeConstit(pn->feature, p, NULL);
    r1->next_altern = r;
    r = r1;
  }
  PNodeBreakup(pn1);
  return(r);
}

PNode *PNodeFlattenAltern(PNode *pn)
{
  PNode	*p, *r, *r1;
  if (pn == NULL) return(NULL);
  if (pn->next_altern) {
    r = NULL;
    for (p = pn; p; p = p->next_altern) {
      r1 = PNodeFlattenAltern1(p);
      r1->next_altern = r;
      r = r1;
    }
    return(r);
  }
  return(PNodeFlattenAltern1(pn));
}

PNode *PNodeLast(PNode *pn)
{
  while (pn->next_altern) pn = pn->next_altern;
  return(pn);
}

PNode *PNodeAppendDestructive(/* RESULTS */ PNode *pn1, /* INPUT */ PNode *pn2)
{
  if (pn1) {
    PNodeLast(pn1)->next_altern = pn2;
    return(pn1);
  } else return(pn2);
}

Bool PNodeIsOK(PNode *pn)
{
  if (pn->type >= PNTYPE_MIN &&
      pn->type <= PNTYPE_MAX) {
    return(1);
  } else {
    if (pn->type == PNTYPE_DESTROYED) Dbg(DBGTREE, DBGBAD, "freed PNode");
    else Dbg(DBGTREE, DBGBAD, "bad PNode");
    Stop();
    return(0);
  }
}

Bool PNodeIsPhrase(PNode *pn)
{
  return(pn->lexitem && pn->lexitem->le && LexEntryIsPhrase(pn->lexitem->le));
}

void PNodeFree(PNode *pn)
{
  PNodeIsOK(pn);
  if (pn->versus) MemFree(pn->versus, "PNode* versus");
  /* todoFREE: Free Table, Article, and other types. */
  switch (pn->type) {
    case PNTYPE_COMMUNICON:
      ObjListFree(pn->u.communicons);
      break;
    case PNTYPE_EMAILHEADER:
      MemFree(pn->u.emh, "EmailHeader");
      break;
    default:
      break;
  }
  pn->type = PNTYPE_DESTROYED;
  MemFree(pn, "PNode");
}

char *PNodeTypeToString(int typ)
{
  switch (typ) {
    case PNTYPE_NA: return("NA");
    case PNTYPE_LEXITEM: return("LEXITEM");
    case PNTYPE_NAME: return("NAME");
    case PNTYPE_ORG_NAME: return("ORG_NAME");
    case PNTYPE_POLITY: return("POLITY");
    case PNTYPE_TSRANGE: return("TSRANGE");
    case PNTYPE_TELNO: return("TELNO");
    case PNTYPE_MEDIA_OBJ: return("MEDIA_OBJ");
    case PNTYPE_PRODUCT: return("PRODUCT");
    case PNTYPE_NUMBER: return("NUMBER");
    case PNTYPE_END_OF_SENT: return("END_OF_SENT");
    case PNTYPE_CONSTITUENT: return("CONSTITUENT");
    case PNTYPE_COMMUNICON: return("COMMUNICON");
    case PNTYPE_ATTRIBUTION: return("ATTRIBUTION");
    case PNTYPE_EMAILHEADER: return("EMAILHEADER");
    case PNTYPE_TABLE: return("TABLE");
    case PNTYPE_ARTICLE: return("ARTICLE");
    case PNTYPE_STRING: return("STRING");
    default: break;
  }
  return("??");
}

void PNodePrintSubstring(FILE *stream, Channel *ch, size_t lowerb,
			 size_t upperb, int expand_control_chars, int maxlen)
{
  unsigned char	*p, *end;
  end = ch->buf + upperb + 1;
  if (upperb >= ch->len) return;
  for (p = ch->buf + lowerb; (maxlen > 0) && (*p) && (p < end); p++, maxlen--) {
    if (expand_control_chars) StreamPutcExpandCntrlChar(*p, stream);
    else fputc(*p, stream);
  }
}

void PNodePrintTypeFeature(FILE *stream, PNode *pn, int shownum, int long_fmt)
{
  char	feat[2];
  if (long_fmt) {
    ScorePrint(stream, pn->score);
  }
/*
  feat = GenFeatAbbrevStringQuick(pn->feature);
*/
  feat[0] = pn->feature;
  feat[1] = TERM;
  if (pn->type != PNTYPE_LEXITEM && pn->type != PNTYPE_CONSTITUENT) {
    fputs(PNodeTypeToString(pn->type), stream);
    if (long_fmt && pn->feature != F_NULL) {
      if (shownum && pn->num != PNUMNA) {
        fprintf(stream, ":%s:%d", feat, pn->num);
      } else {
        fprintf(stream, ":%s", feat);
      }
    }
  } else {
    if (long_fmt && shownum && pn->num != PNUMNA) {
      fprintf(stream, "%s:%d", feat, pn->num);
    } else {
      fputs(feat, stream);
    }
  }
}

void PNodePrint2(FILE *stream, Channel *ch, PNode *pn, int long_fmt)
{
  if (!pn) {
    fputs("NULL", stream);
    return;
  }
  fputc(LBRACKET, stream);
  PNodePrintTypeFeature(stream, pn, long_fmt, long_fmt);
  fputc(SPACE, stream);
  switch (pn->type) {
    case PNTYPE_LEXITEM:
      if (long_fmt) {
        LexitemPrint(stream, pn->lexitem);
        if (!StringAllEqual(pn->punc, SPACE)) {
          fputc('<', stream);
          StreamPutsExpandCntrlChar(pn->punc, stream);
          fputc('>', stream);
        }
      } else {
        LexitemPrint(stream, pn->lexitem);
/*
        if (pn->lexitem && pn->lexitem->word && pn->lexitem->word[0]) {
          fputs(pn->lexitem->word, stream);
        }
 */
      }
      break;
    case PNTYPE_CONSTITUENT:
      if (pn->pn1) PNodePrint1(stream, ch, pn->pn1, long_fmt);
      if (pn->pn2) PNodePrint1(stream, ch, pn->pn2, long_fmt);
      break;
    case PNTYPE_NAME:
      if (long_fmt) {
        NamePrint(stream, pn->u.name);
      } else {
        fputs(pn->u.name->fullname, stream);
      }
      break;
    case PNTYPE_TSRANGE:
      TsRangePrint(stream, pn->u.tsr);
      break;
    case PNTYPE_POLITY:
      ObjListPrintSpace(stream, pn->u.polities, 0);
      break;
    case PNTYPE_TELNO:
      fputs(pn->u.telno->u2.s, stream);
      break;
    case PNTYPE_MEDIA_OBJ:
      ObjListPrintSpace(stream, pn->u.media_obj, 0);
      break;
    case PNTYPE_PRODUCT:
      ObjListPrintSpace(stream, pn->u.product, 0);
      break;
    case PNTYPE_NUMBER:
      ObjPrint(stream, pn->u.number);
      break;
    case PNTYPE_COMMUNICON:
      ObjListPrintSpace(stream, pn->u.communicons, 0);
      break;
    case PNTYPE_ATTRIBUTION:
      AttributionPrint(stream, pn->u.attribution);
      break;
    case PNTYPE_EMAILHEADER:
      EmailHeaderPrint(stream, pn->u.emh);
      break;
    case PNTYPE_TABLE:
      fputc(NEWLINE, stream);
      TablePrint(stream, pn->u.table->flds, pn->u.table->num_flds);
      break;
    case PNTYPE_STRING:
      fprintf(stream, "\"%s\"", pn->u.s);
      break;
    default:
      break;
  }
  if (long_fmt && pn->concepts) {
    fputc(SPACE, stream);
    ObjListPrintSpace(stream, pn->concepts, 0);
  }
  if (long_fmt && ch && pn->upperb > 0L) {
    fprintf(stream, " %ld-%ld:<", (long)pn->lowerb, (long)pn->upperb);
    PNodePrintSubstring(stream, ch, pn->lowerb, pn->upperb, 1, 30);
    fputs(">", stream);
  }
  fputc(RBRACKET, stream);
}

void PNodePrint1(FILE *stream, Channel *ch, PNode *pn, int long_fmt)
{
  for (; pn; pn = pn->next_altern) {
    PNodePrint2(stream, ch, pn, long_fmt);
    if (pn->next_altern) fputs(ALTERN_SEP, stream);
  }
}

void PNodePrint(FILE *stream, Channel *ch, PNode *pn)
{
  PNodePrint1(stream, ch, pn, 1);
  fputc(NEWLINE, stream);
}

void PNodeSocketPrint(Socket *skt, PNode *pn)
{
  char feat[2];
  if (!pn) { SocketWrite(skt, "NULL"); return; }
  SocketWrite(skt, "[");
  feat[0] = pn->feature;
  feat[1] = TERM;
  SocketWrite(skt, "PNODE:");
  SocketWrite(skt, feat);
  if (pn->type != PNTYPE_LEXITEM && pn->type != PNTYPE_CONSTITUENT) {
    SocketWrite(skt, ":");
    SocketWrite(skt, PNodeTypeToString(pn->type));
  }
  switch (pn->type) {
    case PNTYPE_LEXITEM:
      SocketWrite(skt, ":L:");
      LexitemSocketPrint(skt, pn->lexitem);
      break;
    case PNTYPE_CONSTITUENT:
      if (pn->pn1) {
        SocketWrite(skt, " ");
        PNodeSocketPrint(skt, pn->pn1);
      }
      if (pn->pn2) {
        SocketWrite(skt, " ");
        PNodeSocketPrint(skt, pn->pn2);
      }
      break;
    case PNTYPE_NAME:
      SocketWrite(skt, ":\"");
      SocketWrite(skt, pn->u.name->fullname);
      SocketWrite(skt, "\"");
      break;
    case PNTYPE_TSRANGE:
      SocketWrite(skt, ":\"");
      TsRangeSocketPrint(skt, pn->u.tsr);
      SocketWrite(skt, "\"");
      break;
    case PNTYPE_POLITY:
      /* todo */
      break;
    case PNTYPE_TELNO:
      SocketWrite(skt, ":\"");
      SocketWrite(skt, pn->u.telno->u2.s);
      SocketWrite(skt, "\"");
      break;
    case PNTYPE_MEDIA_OBJ:
      /* todo */
      break;
    case PNTYPE_PRODUCT:
      /* todo */
      break;
    case PNTYPE_NUMBER:
      /* todo */
      break;
    case PNTYPE_COMMUNICON:
      /* todo */
      break;
    case PNTYPE_ATTRIBUTION:
      /* todo */
      break;
    case PNTYPE_EMAILHEADER:
      /* todo */
      break;
    case PNTYPE_TABLE:
      /* todo */
      break;
    case PNTYPE_STRING:
      SocketWrite(skt, ":\"");
      SocketWrite(skt, pn->u.s);
      SocketWrite(skt, "\"");
      break;
    default:
      break;
  }
  SocketWrite(skt, "]");
}

/* For debugging. */
void ppn(PNode *pn)
{
  PNodePrettyPrint(stdout, NULL, pn);
  PNodePrettyPrint(Log, NULL, pn);
}

void PNodePrintShort(FILE *stream, Channel *ch, PNode *pn)
{
  PNodePrint1(stream, ch, pn, 0);
  fputc(NEWLINE, stream);
}

Bool PNodeAddVersus(PNode *pn1, PNode *pn2)
{
  if (pn1->versus == NULL) {
    pn1->versusmaxlen = 16;
    pn1->versus = (PNNumber *)MemAlloc(pn1->versusmaxlen*sizeof(PNNumber),
                                       "PNode* versus");
  } else if (pn1->versuslen >= pn1->versusmaxlen) {
    pn1->versusmaxlen = 2*pn1->versusmaxlen;
    pn1->versus = (PNNumber *)MemRealloc(pn1->versus,
                                         pn1->versusmaxlen*sizeof(PNNumber),
                                         "PNode* versus");
  }
  pn1->versus[pn1->versuslen] = pn2->num;
  pn1->versuslen++;
  return(1);
}

Bool PNodeDidVersus(PNode *pn1, PNode *pn2)
{
  int i, len;
  for (i = 0, len = pn1->versuslen; i < len; i++) {
    if (pn2->num == pn1->versus[i]) return(1);
  }
  for (i = 0, len = pn2->versuslen; i < len; i++) {
    if (pn1->num == pn2->versus[i]) return(1);
  }
  return(0);
}

/* todoFREE: Also free lexitems? */
void PNodeFreeTree(PNode *pn)
{
  if (!pn) return;
  if (pn->pn1) PNodeFreeTree(pn->pn1);
  if (pn->pn2) PNodeFreeTree(pn->pn2);
  PNodeFree(pn);
}

int PNodeDepth(PNode *pn)
{
  if (!pn) return(0);
  return(1+IntMax(PNodeDepth(pn->pn1), PNodeDepth(pn->pn2)));
}

PNode *PNodeConstit(char feature, PNode *pn1, PNode *pn2)
{
  PNode	*pn;
  pn = PNodeCreate(feature, NULL, pn1, pn2, 0L, 0L, NULL, NULL);
  pn->type = PNTYPE_CONSTITUENT;
  if (pn1 && pn1->eos != EOSNA) pn->eos = pn1->eos;
  if (pn2 && pn2->eos != EOSNA) pn->eos = pn2->eos;
  return(pn);
}

PNode *PNodeWord(char feature, char *word, char *features, LexEntry *le,
                 Obj *obj)
{
  PNode	*pn;
  pn = PNodeCreate(feature, LexitemCreate(word, le, features),
                   NULL, NULL, 0L, 0L, obj, NULL);
  pn->type = PNTYPE_LEXITEM;
  return(pn);
}

Bool PNodeSimilar(PNode *pn1, PNode *pn2)
{
  if (pn1 == NULL) {
    return(pn2 == NULL);
  }
  if (pn2 == NULL) return(0);
  if (pn1->feature != pn2->feature) return(0);
  if (pn1->lexitem) {
    return(pn2->lexitem && pn1->lexitem->word == pn2->lexitem->word);
  }
  if (pn2->lexitem) return(0);
  return(PNodeSimilar(pn1->pn1, pn2->pn1) &&
         PNodeSimilar(pn1->pn2, pn2->pn2));
}

Bool PNodeSimilarIn(PNode *pn, ObjList *objs)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    if (PNodeSimilar(pn, p->u.sp.pn)) return(1);
  }
  return(0);
}

ObjList *PNodeAppendNonsimilar(ObjList *objs1, ObjList *objs2)
{
  ObjList	*p;
  for (p = objs2; p; p = p->next) {
    if (!PNodeSimilarIn(p->u.sp.pn, objs1)) {
      objs1 = ObjListCreate(p->obj, objs1);
      objs1->u.sp.pn = p->u.sp.pn;
    }
  }
  return(objs1);
}

ObjList *PNodeUniquify(ObjList *objs)
{
  return(PNodeAppendNonsimilar(NULL, objs));
}

int PNodeSyntacticType1(PNode *find_pn, PNode *cur_pn, int inside)
{
  int	r;
  if (find_pn == cur_pn) return(inside);
  if (cur_pn->feature == F_S && inside == SYNTACTIC_TOP_S) {
    if (cur_pn->pn1 && cur_pn->pn1->feature == F_NP &&
        cur_pn->pn2 && cur_pn->pn2->feature == F_VP) {
      if (SYNTACTIC_NA !=
          (r = PNodeSyntacticType1(find_pn, cur_pn->pn1, SYNTACTIC_SUBJ))) {
        return(r);
      }
      if (SYNTACTIC_NA !=
          (r = PNodeSyntacticType1(find_pn, cur_pn->pn2, SYNTACTIC_TOP_VP))) {
        return(r);
      }
    } else {
      if (cur_pn->pn1 && cur_pn->pn1->feature == F_S) {
        if (SYNTACTIC_NA !=
            (r = PNodeSyntacticType1(find_pn, cur_pn->pn1, SYNTACTIC_TOP_S))) {
          return(r);
        }
      }
      if (cur_pn->pn2 && cur_pn->pn2->feature == F_S) {
        if (SYNTACTIC_NA !=
            (r = PNodeSyntacticType1(find_pn, cur_pn->pn2, SYNTACTIC_TOP_S))) {
          return(r);
        }
      }
    }
  } else if (cur_pn->feature == F_VP && inside == SYNTACTIC_TOP_VP) {
    if (cur_pn->pn1 && cur_pn->pn1->feature == F_VP) {
      if (SYNTACTIC_NA !=
          (r = PNodeSyntacticType1(find_pn, cur_pn->pn1, SYNTACTIC_TOP_VP))) {
        return(r);
      }
    }
    if (cur_pn->pn2 && cur_pn->pn2->feature == F_PRONOUN) {
      /* todo: It could also be SYNTACTIC_OBJ/IOBJ. */
      if (SYNTACTIC_NA !=
          (r = PNodeSyntacticType1(find_pn, cur_pn->pn2, SYNTACTIC_SUBJ))) {
        return(r);
      }
    } else if (cur_pn->pn2 && cur_pn->pn2->feature == F_NP) {
      if (SYNTACTIC_NA !=
          (r = PNodeSyntacticType1(find_pn, cur_pn->pn2, SYNTACTIC_OBJ))) {
        return(r);
      }
    } else if (cur_pn->pn2 && cur_pn->pn2->feature == F_PP) {
      if (SYNTACTIC_NA !=
          (r = PNodeSyntacticType1(find_pn, cur_pn->pn2, SYNTACTIC_IOBJ))) {
        return(r);
      }
    }
  } else if (cur_pn->feature == F_NP) {
    if (cur_pn->pn1 && cur_pn->pn1->feature == F_DETERMINER &&
        cur_pn->pn2 && cur_pn->pn2->feature == F_NP) {
      if (SYNTACTIC_NA !=
          (r = PNodeSyntacticType1(find_pn, cur_pn->pn2, inside))) {
        return(r);
      }
    } else if (cur_pn->pn1 && cur_pn->pn1->feature == F_NP &&
        cur_pn->pn2 && cur_pn->pn2->feature == F_NP) {
      if (SYNTACTIC_NA !=
          (r = PNodeSyntacticType1(find_pn, cur_pn->pn1, inside))) {
        return(r);
      }
      if (SYNTACTIC_NA !=
          (r = PNodeSyntacticType1(find_pn, cur_pn->pn2, inside))) {
        return(r);
      }
    } else if (cur_pn->pn1 && cur_pn->pn1->feature == F_NP &&
               cur_pn->pn2 && cur_pn->pn2->feature == F_PP) {
      if (SYNTACTIC_NA !=
          (r = PNodeSyntacticType1(find_pn, cur_pn->pn1, inside))) {
        return(r);
      }
    } else if (cur_pn->pn1 && cur_pn->pn1->feature == F_NOUN &&
               cur_pn->pn2 == NULL) {
      if (SYNTACTIC_NA !=
          (r = PNodeSyntacticType1(find_pn, cur_pn->pn1, inside))) {
        return(r);
      }
    } else if (cur_pn->pn1 && cur_pn->pn1->type == PNTYPE_NAME &&
               cur_pn->pn2 == NULL) {
      if (SYNTACTIC_NA !=
          (r = PNodeSyntacticType1(find_pn, cur_pn->pn1, inside))) {
        return(r);
      }
    }
  } else if (cur_pn->feature == F_PP) {
    if (cur_pn->pn1 && cur_pn->pn1->feature == F_PREPOSITION &&
        cur_pn->pn2 && cur_pn->pn2->feature == F_NP) {
      if (SYNTACTIC_NA !=
          (r = PNodeSyntacticType1(find_pn, cur_pn->pn2, inside))) {
        return(r);
      }
    }
  }
  return(SYNTACTIC_NA);
}

int PNodeSyntacticType(PNode *pn, PNode *pn_top)
{
  return(PNodeSyntacticType1(pn, pn_top, SYNTACTIC_TOP_S));
}

PNode *PNodeSubject(PNode *pn)
{
  PNode	*pna;
  if (!pn) return(NULL);
  if (pn->feature == F_S &&
      pn->pn1 && pn->pn1->feature == F_NP &&
      pn->pn2 && pn->pn2->feature == F_VP) {
    return(pn->pn1);
  }
  if ((pna = PNodeSubject(pn->pn1))) return(pna);
  if ((pna = PNodeSubject(pn->pn2))) return(pna);
  return(NULL);
}

Obj *PNodeNounConcept(PNode *pn)
{
  Obj	*obj;
  if (!pn) return(NULL);
  if (pn->feature == F_NOUN || pn->feature == F_PRONOUN) {
    return(pn->obj);
  }
  if (PNodeIsX_XX_X9(pn)) {
    return(PNodeNounConcept(pn->pn2));
  }   
  if ((obj = PNodeNounConcept(pn->pn1))) return(obj);
  if ((obj = PNodeNounConcept(pn->pn2))) return(obj);
  return(NULL);
}

Bool PNodeSameSubject(PNode *pn1, PNode *pn2)
{
  Obj	*obj1, *obj2;
  obj1 = PNodeNounConcept(PNodeSubject(pn1));
  obj2 = PNodeNounConcept(PNodeSubject(pn2));
  return(obj1 == obj2);
}

PNode *PNodeFindFeat(PNode *pn, char *allfeat, char *anyfeat)
{
  PNode *features;
  if (pn == NULL) return(NULL);
  switch (pn->type) {
    case PNTYPE_LEXITEM:
      if (pn->lexitem &&
          StringAllIn(allfeat, pn->lexitem->features) &&
          StringAnyIn(anyfeat, pn->lexitem->features)) {
        return(pn);
      }
      return(NULL);
    case PNTYPE_CONSTITUENT:
      if ((features = PNodeFindFeat(pn->pn1, allfeat, anyfeat))) {
        return(features);
      }
      if ((features = PNodeFindFeat(pn->pn2, allfeat, anyfeat))) {
        return(features);
      }
      return(NULL);
    default:
      break;
  }
  return(NULL);
}

Bool PNodeFeatureIn(PNode *pn, int feat)
{
  if (!pn) return(0);
  if (pn->feature == feat) return(1);
  return(PNodeFeatureIn(pn->pn1, feat) || PNodeFeatureIn(pn->pn2, feat));
}

/* Any feature in. */
Bool PNodeFeaturesIn(PNode *pn, char *features)
{
  if (!pn) return(0);
  if (StringIn(pn->feature, features)) return(1);
  return(PNodeFeaturesIn(pn->pn1, features) ||
         PNodeFeaturesIn(pn->pn2, features));
}

Bool PNodeClassIn(PNode *pn, Obj *class)
{
  if (!pn) return(0);
  if (pn->lexitem) {
    return(LexEntryConceptIsAncestor(class, pn->lexitem->le));
  }
  return(PNodeClassIn(pn->pn1, class) || PNodeClassIn(pn->pn2, class));
}

Bool PNodeWordIn(PNode *pn, char *srcphrase, int pos, int lang)
{
  if (!pn) return(0);
  if (pn->lexitem && LexEntryWordIs(pn->lexitem->le, srcphrase, pos, lang)) {
    return(1);
  }
  return(PNodeWordIn(pn->pn1, srcphrase, pos, lang) ||
         PNodeWordIn(pn->pn2, srcphrase, pos, lang));
}

Bool PNodeExclusiveFeatures(PNode *pn, char *features)
{
  if (!pn) return(1);
  if (!StringIn(pn->feature, features)) return(0);
  return(PNodeExclusiveFeatures(pn->pn1, features) &&
         PNodeExclusiveFeatures(pn->pn2, features));
}

Bool PNodeIsCompoundTense(PNode *pn)
{
  return(PNodeExclusiveFeatures(pn, FS_COMPTENSE));
}

Bool PNodeIsProperNounTerminal(PNode *pn)
{
  if (pn->type == PNTYPE_NAME ||
      pn->type == PNTYPE_ORG_NAME ||
      pn->type == PNTYPE_POLITY ||
      pn->type == PNTYPE_TELNO ||
      pn->type == PNTYPE_MEDIA_OBJ ||
      pn->type == PNTYPE_PRODUCT ||
      pn->type == PNTYPE_NUMBER) {
    return(1);
  }
  if (pn->type == PNTYPE_LEXITEM &&
      pn->lexitem && pn->lexitem->word &&
      MorphIsProper(pn->lexitem->word)) {
    return(1);
  }
  return(0);
}

Bool PNodeIsProperNP(PNode *pn)
{
  return(pn->feature == F_NP &&
         pn->pn1 && pn->pn1->feature == F_NOUN &&
         pn->pn2 == NULL &&
         PNodeIsProperNounTerminal(pn->pn1));
}

void PNodeCountVPObjects1(PNode *w1, PNode *w2, /* RESULTS */ int *obj,
                          int *iobj)
{
  if (w2) {
    if (w2->feature == F_PP) *iobj = *iobj + 1;
    else if (w2->feature == F_NP) *obj = *obj + 1;
  }
  if (w1 && w1->feature == F_VP) {
    PNodeCountVPObjects1(w1->pn1, w1->pn2, obj, iobj);
  }
}

void PNodeCountVPObjects(PNode *w1, PNode *w2, /* RESULTS */ int *obj,
                         int *iobj)
{
  *obj = 0;
  *iobj = 0;
  PNodeCountVPObjects1(w1, w2, obj, iobj);
}

int PNodeNumberOfWords(PNode *pn)
{
  if (pn == NULL) {
    return(0);
  } else if (pn->lexitem && pn->lexitem->le) {
    return(LexEntryNumberOfWords(pn->lexitem->le));
  } else if (pn->type == PNTYPE_CONSTITUENT) {
    return(PNodeNumberOfWords(pn->pn1) + PNodeNumberOfWords(pn->pn2));
  } else {
  /* A non-PNTYPE_LEXITEM terminal counts for 1 word (for now) */
    return(1);
  }
}

/* A terminal is anything besides a constituent. */
int PNodeNumberOfTerminals(PNode *pn)
{
  if (pn == NULL) {
    return(0);
  } else if (pn->type == PNTYPE_CONSTITUENT) {
    return(PNodeNumberOfTerminals(pn->pn1) + PNodeNumberOfTerminals(pn->pn2));
  } else {
    return(1);
  }
}

LexEntry *PNodeLexEntry(PNode *pn)
{
  if (!pn) return(NULL);
  if (pn->lexitem) return(pn->lexitem->le);
  return(NULL);
}

LexEntry LexEntryBad;

LexEntry *PNodeLeftmostLexEntry1(PNode *pn)
{
  LexEntry	*le;
  if (!pn) return(NULL);
  if (pn->lexitem) return(pn->lexitem->le);
  if (pn->type >= PNTYPE_MIN_SYNPARSE &&
      pn->type <= PNTYPE_MAX_SYNPARSE) {
  /* "with (NAME:)Mary and (NAME:)Alex" */
    return(&LexEntryBad);
  }
  if ((le = PNodeLeftmostLexEntry1(pn->pn1))) return(le);
  if ((le = PNodeLeftmostLexEntry1(pn->pn2))) return(le);
  return(NULL);
}

LexEntry *PNodeLeftmostLexEntry(PNode *pn)
{
  LexEntry	*le;
  if ((le = PNodeLeftmostLexEntry1(pn))) {
    if (le == &LexEntryBad) return(NULL);
    else return(le);
  }
  return(NULL);
}

LexEntry *PNodeLeftmostPreposition(PNode *pn)
{
  LexEntry	*le;
  if (!pn) return(NULL);
  if (pn->lexitem && pn->feature == F_PREPOSITION) return(pn->lexitem->le);
  if ((le = PNodeLeftmostPreposition(pn->pn1))) return(le);
  if ((le = PNodeLeftmostPreposition(pn->pn2))) return(le);
  return(NULL);
}

Lexitem *PNodeLeftmostLexitem(PNode *pn)
{
  if (!pn) return(NULL);
  if (pn->lexitem) return(pn->lexitem);
  return(PNodeLeftmostLexitem(pn->pn1));
}

LexEntry *PNodeRightmostLexEntry(PNode *pn)
{
  LexEntry	*le;
  if (!pn) return(NULL);
  if (pn->lexitem) return(pn->lexitem->le);
  if ((le = PNodeRightmostLexEntry(pn->pn2))) return(le);
  if ((le = PNodeRightmostLexEntry(pn->pn1))) return(le);
  return(NULL);
}

Lexitem *PNodeRightmostLexitem(PNode *pn)
{
  if (!pn) return(NULL);
  if (pn->lexitem) return(pn->lexitem);
  if (pn->pn2) return(PNodeRightmostLexitem(pn->pn2));
  else return(PNodeRightmostLexitem(pn->pn1));
}

PNode *PNodeLeftmost(PNode *pn)
{
  if (!pn) return(NULL);
  if (pn->lexitem) return(pn);
  return(PNodeLeftmost(pn->pn1));
}

/* Of form (CONSTIT (POS word)). */
LexEntry *PNodeSingletonLexEntry(PNode *pn)
{
  if (pn->pn2 == NULL && pn->pn1 && pn->pn1->lexitem) {
    return(pn->pn1->lexitem->le);
  }
  return(NULL);
}

PNode *PNodeLeftmostFeat(PNode *pn, int feature)
{
  PNode *pn1;
  if (pn == NULL) return(NULL);
  if (pn->feature == feature) return(pn);
  if ((pn1 = PNodeLeftmostFeat(pn->pn1, feature))) {
    return(pn1);
  }
  return(PNodeLeftmostFeat(pn->pn2, feature));
}

PNode *PNodeLeftmostKorR(PNode *pn)
{
  if (!pn) return(NULL);
  if (pn->feature == F_PREPOSITION || pn->feature == F_CONJUNCTION) {
    return(pn);
  }
  return(PNodeLeftmostKorR(pn->pn1));
}

PNode *PNodeLeftmostK(PNode *pn)
{
  if (!pn) return(NULL);
  if (pn->feature == F_CONJUNCTION) {
    return(pn);
  }
  return(PNodeLeftmostK(pn->pn1));
}

Bool PNodeKIsLeftmost(PNode *pn)
{
  if (!pn) return(0);
  if (pn->feature == F_CONJUNCTION) return(1);
  return(PNodeKIsLeftmost(pn->pn1));
}

void PNodeFindHeadVerb1(PNode *w, int leftmost, int rightmost,
                        /* RESULTS */ PNode **auxverb, PNode **mainverb)
{
  if (w->pn1 && w->pn1->feature == F_VERB && w->pn2 == NULL) {
    if (leftmost) *auxverb = w->pn1;
    if (rightmost) *mainverb = w->pn1;
  } else if (w->pn1 && w->pn1->feature == F_VP &&
             w->pn2 && w->pn2->feature == F_VERB) {
    PNodeFindHeadVerb1(w->pn1, leftmost, 0, auxverb, mainverb);
    PNodeFindHeadVerb1(w->pn2, 0, rightmost, auxverb, mainverb);
  } else if (w->pn1 && w->pn1->feature == F_VP) {
    PNodeFindHeadVerb1(w->pn1, leftmost, rightmost, auxverb, mainverb);
  } else if (w->pn2 && w->pn2->feature == F_VP) {
    PNodeFindHeadVerb1(w->pn2, leftmost, rightmost, auxverb, mainverb);
  }
}

void PNodeFindHeadVerb(PNode *w, /* RESULTS */ PNode **auxverb,
                       PNode **mainverb)
{
  *auxverb = NULL;
  *mainverb = NULL;
  PNodeFindHeadVerb1(w, 1, 1, auxverb, mainverb);
}

void PNodeFindHeadVerbS(PNode *z, /* RESULTS */ PNode **auxverb,
                        PNode **mainverb)
{
  *auxverb = NULL;
  *mainverb = NULL;
  if (z->pn1 && z->pn1->feature == F_NP &&
      z->pn2 && z->pn2->feature == F_VP) {
    PNodeFindHeadVerb1(z->pn2, 1, 1, auxverb, mainverb);
    return;
  }
  /* todo: Other cases? */
}

LexEntry *PNodeFindMainverbLe(PNode *w)
{
  PNode	*auxverb, *mainverb;
  PNodeFindHeadVerb(w, &auxverb, &mainverb);
  if (mainverb && mainverb->lexitem && mainverb->lexitem->le) {
    return(mainverb->lexitem->le);
  }
  return(NULL);
}

Lexitem *PNodeFindAuxverbLexitem(PNode *w)
{
  PNode	*auxverb, *mainverb;
  PNodeFindHeadVerb(w, &auxverb, &mainverb);
  if (auxverb && auxverb->lexitem) {
    return(auxverb->lexitem);
  }
  return(NULL);
}

/* Normally this is called with an ADJP. */
PNode *PNodeFindHeadAdj(PNode *pn)
{
  PNode	*pn1;
  if (pn == NULL) return(NULL);
  if (pn->feature == F_ADJECTIVE) return(pn);
  if (pn->pn1->feature == F_ADJP && pn->pn2->feature == F_PP) {
    return(PNodeFindHeadAdj(pn->pn1));
  } else if (pn->pn1->feature == F_ADVERB && pn->pn2->feature == F_ADJP) {
    return(PNodeFindHeadAdj(pn->pn2));
  } else if (pn->pn1->feature == F_ADJP && pn->pn2->feature == F_ADVERB) {
    return(PNodeFindHeadAdj(pn->pn1));
  } else if (pn->pn1->feature == F_DETERMINER && pn->pn2->feature == F_ADJP) {
    return(PNodeFindHeadAdj(pn->pn2));
  }
  if ((pn1 = PNodeFindHeadAdj(pn->pn2))) return(pn1);
  if ((pn1 = PNodeFindHeadAdj(pn->pn1))) return(pn1);
  return(NULL);
}

/* See also Syn_ParseIsNPNP_Genitive. */
Bool PNodeIsX_XX_X9(PNode *pn)
{
  return(pn->feature == F_NP &&
         pn->pn1 && pn->pn1->feature == F_NP &&
         pn->pn2 && pn->pn2->feature == F_NP &&
         pn->pn1->pn1 && pn->pn1->pn1->feature == F_NP &&
         pn->pn1->pn1->pn1 && pn->pn1->pn1->pn1->feature == F_PRONOUN &&
         pn->pn1->pn2 && pn->pn1->pn2->feature == F_ELEMENT &&
         LexEntryConceptIsAncestor(N("genitive-element"),
                                   PNodeLeftmostLexEntry(pn->pn1->pn2)));
}

/* Recurse through pn to find head noun features. */
Bool PNodeGetHeadNounFeatures(PNode *pn, Bool actual,
                              /* RESULTS */ int *gender, int *number,
                              int *person)
{
  int	gender1, gender2, number1, number2, person1, person2;
  if (pn == NULL) {
    *gender = F_NULL;
    *number = F_NULL;
    *person = F_NULL;
    return(0);
  }
  if (pn->is_coord &&
      pn->pn1 && pn->pn1->feature == F_NP &&
      pn->pn2 && pn->pn2->feature == F_NP) {
    /* Headless construction of an AND/OR coordinating conjunction. */
    PNodeGetHeadNounFeatures(pn->pn1, actual, &gender1, &number1, &person1);
    PNodeGetHeadNounFeatures(pn->pn2, actual, &gender2, &number2, &person2);
    if (gender1 == gender2) {
      *gender = gender1;
    } else if (gender1 == F_MASCULINE || gender2 == F_MASCULINE) {
      *gender = F_MASCULINE;
    } else *gender = gender1;
    *number = F_PLURAL;
    if (person1 != F_NULL) *person = person1;
    else *person = person2;
    return(1);
  }
  if (PNodeIsX_XX_X9(pn)) {
    if (pn->pn2 && PNodeGetHeadNounFeatures(pn->pn2, actual, gender, number,
                                            person)) {
      return(1);
    }
  }
  if (pn->pn2 && pn->pn2->feature == F_ELEMENT) return(0); /* todo: Needed? */
  if (StringIn(pn->feature, FS_HEADNOUN) && pn->lexitem) {
    if (actual && pn->gender != F_NULL) *gender = pn->gender;
    else *gender = FeatureGet(pn->lexitem->features, FT_GENDER);
    if (actual && pn->number != F_NULL) *number = pn->number;
    else *number = FeatureGet(pn->lexitem->features, FT_NUMBER);
    if (actual && pn->person != F_NULL) *person = pn->person;
    else *person = FeatureGet(pn->lexitem->features, FT_PERSON);
    return(1);
  }
  if (pn->pn1 && PNodeGetHeadNounFeatures(pn->pn1, actual, gender, number,
                                          person)) {
    return(1);
  }
  if (pn->pn2 && PNodeGetHeadNounFeatures(pn->pn2, actual, gender, number,
                                          person)) {
    return(1);
  }
  *gender = F_NULL;
  *number = F_NULL;
  *person = F_NULL;
  return(0);
}

void PNodeGetFeatures(PNode *pn, /* RESULTS */ int *gender, int *number)
{
  if (pn->lexitem) {
    if (pn->gender != F_NULL) *gender = pn->gender;
    else *gender = FeatureGet(pn->lexitem->features, FT_GENDER);
    if (pn->number != F_NULL) *number = pn->number;
    else *number = FeatureGet(pn->lexitem->features, FT_NUMBER);
  } else {
    *gender = F_NULL;
    *number = F_NULL;
  }
}

void PNodeGetGenderNumberPerson(PNode *pn, /* RESULTS */ int *gender,
                                int *number, int *person)
{
  if (pn && pn->lexitem) {
    *gender = FeatureGet(pn->lexitem->features, FT_GENDER);
    *number = FeatureGet(pn->lexitem->features, FT_NUMBER);
    *person = FeatureGet(pn->lexitem->features, FT_PERSON);
  } else {
    *gender = F_NULL;
    *number = F_NULL;
    *person = F_NULL;
  }
}

/* pn1->pn2->pn3 becomes pn1 pn2 pn3 */
void PNodeBreakup(PNode *pn)
{
  PNode	*p, *next;
  p = pn;
  while (p) {
    next = p->next_altern;
    p->next_altern = NULL;
    p = next;
  }
}

void PNodeAddAllcons(PNode *pn, Obj *obj)
{
  if (!ObjListSimilarIn(obj, pn->allcons)) {
    pn->allcons = ObjListCreate(obj, pn->allcons);
  }
}

void PNodePrettyPrint1(FILE *stream, Channel *ch, PNode *pn, int column,
                       int long_fmt)
{
  StreamPrintSpaces(stream, column);
  if (PNodeDepth(pn) <= 2) {
    PNodePrint1(stream, ch, pn, long_fmt);
    return;
  }
  fputc(LBRACKET, stream);
  PNodePrintTypeFeature(stream, pn, 1, long_fmt);
  if (pn->pn1) {
    fputc(NEWLINE, stream);
    PNodePrettyPrint1(stream, ch, pn->pn1, column+1, long_fmt);
  }
  if (pn->pn2) {
    fputc(NEWLINE, stream);
    PNodePrettyPrint1(stream, ch, pn->pn2, column+1, long_fmt);
  }
  fputc(RBRACKET, stream);
}

void PNodePrettyPrint(FILE *stream, Channel *ch, PNode *pn)
{
  PNodePrettyPrint1(stream, ch, pn, 0, 0);
  fputc(NEWLINE, stream);
}

void PNodeTextPrintStream(FILE *stream, PNode *pn, int eos, Discourse *dc)
{
  Text	*text;
  text = TextCreate(128, DiscourseCurrentChannelLineLen(dc), dc);
  PNodeTextPrint(text, pn, eos, 0, 1, dc);
  if (TextLen(text) > 0) {
    TextPrint(stream, text);
/*
    fputc(NEWLINE, stream);
 */
    fflush(stream);
  }
  TextFree(text);
}

void PNodePrintStringArrayAdd(StringArray *sa, PNode *pn, int eos,
                              Discourse *dc)
{
  Text			*text;
  text = TextCreate(SENTLEN, DiscourseCurrentChannelLineLen(dc), dc);
  PNodeTextPrint(text, pn, eos, 0, 0, dc);
  if (TextLen(text) > 0) {
    StringArrayAddCopy(sa, TextString(text), 1);
  }
  TextFree(text);
}

/* todo: Output ' when printing PNodes marked as contractions/elisions.
 * There's no need yet since these aren't generated.
 */
void PNodeTextPrint(Text *text, PNode *pn, int eos, int showgender,
                    int showalterns, Discourse *dc)
{
  if (showalterns) {
    PNodeTextPrint1(text, pn, eos != SPACE, showgender, dc);
  } else {
    PNodeTextPrint2(text, pn, eos != SPACE, showgender, dc);
  }
  if (text->lang == F_FRENCH && (eos == '?' || eos == '!' || eos == ':')) {
    TextPutc(SPACE, text);
    TextPutc(eos, text);
  } else {
    if (eos != '.' || TextLastChar(text) != '.') {
      TextPutc(eos, text);
    }
  }
}

void PNodeTextPrint1(Text *text, PNode *pn, int capitalize, int showgender,
                     Discourse *dc)
{
  PNode	*p;
  for (p = pn; p; p = p->next_altern) {
  /* todo: Eliminate identical-looking alternatives. */
    PNodeTextPrint2(text, p, capitalize, showgender, dc);
    if (p->next_altern) TextPuts(ALTERN_SEP, text);
  }
}

void PNodeTextPrint2(Text *text, PNode *pn, int capitalize, int showgender,
                     Discourse *dc)
{
  int	attach, lang;
  char	buf[PHRASELEN];
  PNode	*sub_pn;
  if (!pn) return;
  switch (pn->type) {
    case PNTYPE_LEXITEM:
      if (pn->lexitem && pn->lexitem->word && pn->lexitem->word[0]) {
        lang = FeatureGet(pn->lexitem->features, FT_LANG);
        if ((lang != F_NULL) &&
            (DC(dc).lang != FeatureGet(pn->lexitem->features, FT_LANG))) {
          if (DC(dc).lang == F_FRENCH) {
            buf[0] = LGUILLEMETS;
            buf[1] = SPACE;
            LexEntryAddSep(pn->lexitem->le, pn->lexitem->word, PHRASELEN,
                           buf+2);
            StringAppendChar(buf, PHRASELEN, SPACE);
            StringAppendChar(buf, PHRASELEN, RGUILLEMETS);
          } else {
            buf[0] = DQUOTE;
            LexEntryAddSep(pn->lexitem->le, pn->lexitem->word, PHRASELEN,
                           buf+1);
            StringAppendChar(buf, PHRASELEN, DQUOTE);
         }
#ifdef notdef
         /* todo: Not if html dump. */
         if (pn->obj) {
           /* Temporary for debugging. */
           StringCat(buf, "(&", PHRASELEN);
           StringCat(buf, M(pn->obj), PHRASELEN);
           StringCat(buf, ")", PHRASELEN);
         }
#endif
        } else {
          LexEntryAddSep(pn->lexitem->le, pn->lexitem->word, PHRASELEN, buf);
        }
        if (capitalize && !StringHeadEqual("http://", buf)) {
          buf[0] = CharToUpper((uc)(buf[0]));
        }
        if (showgender) GenAppendNounGender(pn->lexitem->features, buf);
        attach = 0;
        if (StringIn(F_CONTRACTION, pn->lexitem->features)) {
          StringPrependChar(buf, PHRASELEN, SQUOTE);
          TextAttachNextWord(text);
        }
        if (StringIn(F_ELISION, pn->lexitem->features)) {
          StringAppendChar(buf, PHRASELEN, SQUOTE);
          attach = 1;
        } else {
          StringCat(buf, pn->punc, PHRASELEN);
        }
        TextPrintPhrase(text, TERM, buf);
        if (attach) TextAttachNextWord(text);
      } else if (pn->lexitem && LexitemIsTrace(pn->lexitem)) {
      /* A trace does not cause any output. */
      } else {
        TextPrintPhrase(text, TERM, "???");
      }
      break;
    case PNTYPE_CONSTITUENT:
      if (pn->pn1) PNodeTextPrint1(text, pn->pn1, capitalize, showgender, dc);
      if (pn->pn2) PNodeTextPrint1(text, pn->pn2, 0, showgender, dc);
      break;
    case PNTYPE_NAME:
      StringCpy(buf, pn->u.name->fullname, PHRASELEN);
      StringCat(buf, pn->punc, PHRASELEN);
      TextPrintPhrase(text, TERM, buf);
      break;
    case PNTYPE_TSRANGE:
      if (TsIsDur(&pn->u.tsr->startts)) {
      /* todo: If startts != stopts extend GetDur1 to generate a Dur range. */
        sub_pn = GenDur1(TsGetDur(&pn->u.tsr->startts), dc);
      } else if (TsIsTod(&pn->u.tsr->startts)) {
      /* todo: If startts != stopts extend GenTod1 to generate a Tod range. */
        sub_pn = GenTod1(TsGetTod(&pn->u.tsr->startts), dc);
      } else {
        sub_pn = GenTsRange1(pn->u.tsr, NULL, 0, dc);
      }
      PNodeTextPrint2(text, sub_pn, capitalize, showgender, dc);
      break;
    case PNTYPE_STRING:
      StringCpy(buf, pn->u.s, PHRASELEN);
      StringCat(buf, pn->punc, PHRASELEN);
      TextPrintPhrase(text, TERM, buf);
      break;
    default:
      /* todo: In this case, output the raw channel text. */
      TextPrintPhrase(text, TERM, "???");
  }
}

/* Score */

Bool ScoreIsValid(Float score)
{
  return(score >= 0.0 && score <= 2.0);	/* was 1.0 */
}

/* See Allen (1995, p. 215). */
Float ScoreCombine(Float score1, Float score2)
{
  if (!ScoreIsValid(score1)) {
    Dbg(DBGGEN, DBGBAD, "bad score1");
    Stop();
    return(SCORE_MAX);
  }
  if (!ScoreIsValid(score2)) {
    Dbg(DBGGEN, DBGBAD, "bad score2");
    Stop();
    return(SCORE_MAX);
  }
  return(score1*score2);
}

void ScorePrint(FILE *stream, Float score)
{
  if (ScoreIsValid(score)) {
    if (score < .99) {
      fprintf(stream, "%4.3f:", score);
    }
  } else {
    fputs("GARBAGE_SCORE:", stream);
  }
}

/* End of file. */
