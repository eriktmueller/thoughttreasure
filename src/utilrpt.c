/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940906: begun
 * 19940907: finished debugging
 */

#include "tt.h"
#include "repbasic.h"
#include "repobj.h"
#include "repstr.h"
#include "reptext.h"
#include "reptime.h"
#include "semgen1.h"
#include "semgen2.h"
#include "synpnode.h"
#include "utildbg.h"
#include "utilrpt.h"

/* Rpt */

RptField *RptFieldCreate(char *s, int just)
{
  RptField	*rf;
  rf = CREATE(RptField);
  rf->s = StringCopy(s, "RptFieldCreate");
  rf->just = just;
  rf->next = NULL;
  return(rf);
}

RptField *RptFieldAppend(char *s, int just, RptField *orig_rf)
{
  RptField	*rfnew, *rf;
  rfnew = RptFieldCreate(s, just);
  if (!orig_rf) return(rfnew);
  rf = orig_rf;
  while (1) {
    if (!rf->next) {
      rf->next = rfnew;
      return(orig_rf);
    }
    rf = rf->next;
  }
}

RptLine *RptLineCreate(RptField *fields, RptLine *next)
{
  RptLine	*rl;
  rl = CREATE(RptLine);
  rl->fields = fields;
  rl->next = next;
  return(rl);
}

Rpt *RptCreate()
{
  Rpt	*r;
  r = CREATE(Rpt);
  r->lines = NULL;
  r->numfields = 0;
  r->curfield = -1;
  return(r);
}

void RptFieldFree(RptField *rf)
{
  RptField *n;
  while (rf) {
    if (rf->s) MemFree(rf->s, "RptFieldCreate");
    n = rf->next;
    MemFree(rf, "RptField");
    rf = n;
  }
}

void RptLineFree(RptLine *rl)
{
  RptLine *n;
  while (rl) {
    RptFieldFree(rl->fields);
    n = rl->next;
    MemFree(rl, "RptLine");
    rl = n;
  }
}

void RptFree(Rpt *r)
{
  RptLineFree(r->lines);
}

int RptLineLen(RptLine *rl)
{
  int	len;
  for (len = 0; rl; rl = rl->next, len++) {
  }
  return(len);
}

void RptStartLine(Rpt *r)
{
  r->lines = RptLineCreate(NULL, r->lines);
  r->curfield = -1;
}

void RptAdd(Rpt *r, char *s, int just)
{
  int			len;
  if (!r->lines) RptStartLine(r);
  r->curfield++;
  if (r->curfield >= RPT_MAXFIELDS) {
    Dbg(DBGGENER, DBGBAD, "increase RPT_MAXFIELDS");
    r->curfield = RPT_MAXFIELDS-1;
  }
  if (r->curfield >= r->numfields) {
    r->fieldlens[r->curfield] = 0;
    r->numfields++;
  }
  r->lines->fields = RptFieldAppend(s, just, r->lines->fields);
  len = strlen(s);
  if (len > r->fieldlens[r->curfield]) {
    r->fieldlens[r->curfield] = len;
  }
}

void RptAddTs(Rpt *rpt, Ts *ts, Discourse *dc)
{
  char	buf[PHRASELEN];
  TsToString(ts, 0, PHRASELEN, buf);
  RptAdd(rpt, buf, RPT_JUST_LEFT);
}

void RptAddConcept(Rpt *rpt, Obj *obj, Discourse *dc)
{
  char	buf[PHRASELEN];
  GenConceptString(obj, N("empty-article"), F_NOUN, F_NULL, DC(dc).lang,
                   F_NULL, F_NULL, F_NULL, PHRASELEN, 0, 1, dc, buf);
  RptAdd(rpt, buf, RPT_JUST_LEFT);
}

void RptAddList(Rpt *rpt, ObjList *objs, int justif, Discourse *dc)
{
  char		buf[PHRASELEN];
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    GenConceptString(p->obj, N("empty-article"), F_NOUN, F_NULL,
                     DC(dc).lang, F_NULL, F_NULL, F_NULL, PHRASELEN, 1, 1,
                     dc, buf);
    RptAdd(rpt, buf, justif);
  }
}

void RptAddFeatAbbrev(Rpt *rpt, int feature, int force, Discourse *dc)
{
  char	*s;
  s = GenFeatAbbrevString(feature, force, dc);
  RptAdd(rpt, s, RPT_JUST_LEFT);
}

void RptAddFeaturesAbbrev(Rpt *rpt, char *features, int force, char *except,
                          Discourse *dc)
{
  char	buf[PHRASELEN];
  GenFeaturesAbbrevString(features, 0, force, except, dc, PHRASELEN, buf);
  RptAdd(rpt, buf, RPT_JUST_LEFT);
}

void RptAddInt(Rpt *rpt, int i, Discourse *dc)
{
  char	buf[PHRASELEN];
  sprintf(buf, "%d", i);
  RptAdd(rpt, buf, RPT_JUST_RIGHT);
}

void RptAddLong(Rpt *rpt, long i, Discourse *dc)
{
  char	buf[PHRASELEN];
  sprintf(buf, "%ld", i);
  RptAdd(rpt, buf, RPT_JUST_RIGHT);
}

void RptAddFloat(Rpt *rpt, Float x, Discourse *dc)
{
  char	buf[PHRASELEN];
  sprintf(buf, "%g", x);
  RptAdd(rpt, buf, RPT_JUST_RIGHT);
}

RptLine **RptLineToArray(RptLine *rl, /* RESULTS */ size_t *num, size_t *size)
{
  size_t	i, len;
  RptLine	**r, *p;
  *size = sizeof(RptLine *);
  len = RptLineLen(rl);
  *num = len;
  r = (RptLine **)MemAlloc(sizeof(RptLine *)*len, "RptLineArray");
  for (i = 0, p = rl; p; p = p->next, i++) {
    r[i] = p;
  }
  return(r);
}

/* Destructively reorders the list. */
void RptLineArrayToList(RptLine **arr, size_t num)
{
  RptLine	*r;
  size_t	i;
  r = NULL;
  for (i = 0; i < num-1; i++) {
    arr[i]->next = arr[i+1];
  }
  arr[i]->next = NULL;
}

int RptSortBy;
int RptSortSign;

int RptSortFn(const void *rl1, const void *rl2)
{
  int			i;
  Float			f1, f2;
  RptField	*rf1, *rf2;
  for (rf1 = (*((RptLine **)rl1))->fields, i = 0; rf1 && (i < RptSortBy); i++) {
    rf1 = rf1->next;
  }
  for (rf2 = (*((RptLine **)rl2))->fields, i = 0; rf2 && (i < RptSortBy); i++) {
    rf2 = rf2->next;
  }
  if (rf1 && rf2) {
    if (FloatIsString(rf1->s) &&
        FloatIsString(rf2->s)) {
      f1 = FloatParse(rf1->s, -1);
      f2 = FloatParse(rf2->s, -1);
      if (f1 < f2) return(RptSortSign*-1);
      else if (f1 == f2) return(0);
      else return(RptSortSign);
    } else return(RptSortSign*french_strcmp(rf1->s, rf2->s));
  } else return(-RptSortSign);
}

void RptLineSort1(RptLine **arr, size_t num, size_t size, int sort_by,
                  int sort_sign)
{
  RptSortBy = sort_by;
  RptSortSign = sort_sign;
  qsort(arr, num, size, RptSortFn);
}

RptLine *RptLineSort(RptLine *rl, int numfields, int sort_by, int sort_sign)
{
  int			i;
  RptLine	**arr, *r;
  size_t		num, size;
  if (rl == NULL) return(NULL);
  arr = RptLineToArray(rl, &num, &size);
  if (sort_by == -1) {
    for (i = numfields-1; i >= 0; i--) {
      RptLineSort1(arr, num, size, i, sort_sign);
    }
  } else {
    RptLineSort1(arr, num, size, sort_by, sort_sign);
  }
  RptLineArrayToList(arr, num);
  r = arr[0];
  MemFree(arr, "RptLineArray");
  return(r);
}

void RptSort(Rpt *r, int sort_by, int sort_sign)
{
  if (!r->lines) return;
  if (sort_by >= r->numfields) {
    Dbg(DBGGENER, DBGBAD, "RptSort: sort_by out of range");
    sort_by = -1;
  }
  /* Don't sort first line which is header.
   * todo: Specify number of header lines.
   */
  r->lines->next = RptLineSort(r->lines->next, r->numfields, sort_by,
                               sort_sign);
}

void RptTextPrint1(Text *text, char *s, int fieldlen, int just)
{
  int	i, slen, total;
  if (StringIn(NEWLINE, s)) {
    if (0) {
    /* if (strlen(s) < fieldlen) { */
      StringElimChar(s, NEWLINE);	/* Destructive. Watch out. */
      TextPuts(s, text);
    } else {
      TextPutsIndented(s, text, "    ");
      return;
    }
  }
  slen = strlen(s);
  if (slen > fieldlen) {
    for (i = 0; i < fieldlen; i++) TextPutc(s[i], text);
    return;
  }
  if (just == RPT_JUST_RIGHT) {
    total = fieldlen - slen;
    TextPrintSpaces(text, total);
  } else if (just == RPT_JUST_CENTER) {
    total = (fieldlen - slen)/2;
    TextPrintSpaces(text, total);
  } else total = 0;
  TextPuts(s, text);
  total += slen;
  TextPrintSpaces(text, fieldlen-total);
}

void RptTextPrintLine(Text *text, Rpt *r, RptLine *rl, int maxfieldlen, int sep)
{
  int			i, len;
  RptField	*rf;
  for (rf = rl->fields, i = 0, len = r->numfields; rf && i < len;
       i++, rf = rf->next) {
    if (sep) {
      RptTextPrint1(text,
"________________________________________________________________________________________",
                       IntMin(r->fieldlens[i], maxfieldlen), rf->just);
    } else {
      RptTextPrint1(text, rf->s, IntMin(r->fieldlens[i], maxfieldlen),
                    rf->just);
    }
    if (rf->next) TextPutc(SPACE, text);
  }
  TextPutc(NEWLINE, text);
}

/* <sort_by>: -2 - do not sort
 *            -1 - sort by all fields, right to left
 *             0 - field 0
 *             1 - field 1
 *             ...
 * <sort_sign>: 1 - sort forward
 *             -1 - sort backward
 *
 */
void RptTextPrint(Text *text, Rpt *r, int show_header, int sort_by,
                  int sort_sign, int maxfieldlen)
{
  int		cnt;
  RptLine	*rl;
  cnt = 0;
  if (r->lines) {
    TextNextParagraph(text);
    if (sort_by != -2) RptSort(r, sort_by, sort_sign);
    if (show_header) {
      RptTextPrintLine(text, r, r->lines, maxfieldlen, 0);
      RptTextPrintLine(text, r, r->lines, maxfieldlen, 1);
    }
    for (rl = r->lines->next; rl; rl = rl->next) {
      cnt++;
      RptTextPrintLine(text, r, rl, maxfieldlen, 0);
    }
  }
  if (cnt > 49) TextPrintf(text, "%d entries\n", cnt);
}

int RptLines(Rpt *r)
{
  RptLine	*rl;
  int		i;
  for (rl = r->lines, i = 0; rl; i++, rl = rl->next);
  return(i-1);
}

/* End of file. */
