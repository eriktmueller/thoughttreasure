/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19941216: begun
 * 19941217: more work
 * 19960224: fixes
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
#include "semparse.h"
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

Table *TableCreate(Field *flds, int num_flds)
{
  Table	*table;
  table = CREATE(Table);
  table->flds = flds;
  table->num_flds = num_flds;
  return(table);
}

void TableFieldFreeContents(Field *fld)
{
  StringArrayFreeCopy(fld->sa);
  ObjListFree(fld->objs);
}

void TableFreeFields(Field *flds, int num_flds)
{
  int	i;
  for (i = 0; i < num_flds; i++) {
    TableFieldFreeContents(&flds[i]);
  }
}

void TablePrintField(FILE *stream, Field *fld, int fieldnum, int num_flds)
{
  int	i, found;
  fprintf(stream, "FIELD #%d %d-%d <%s> %c ",
          fieldnum,
          fld->startpos,
          fld->stoppos,
          M(fld->class),
          (char)fld->lang);
  found = 0;
  for (i = 0; i < num_flds; i++) {
    if (fld->rels[i] != NULL) {
      found = 1;
      break;
    }
  }
  if (found) {
    fprintf(stream, "relations ");
    for (i = 0; i < num_flds; i++) {
      if (fld->rels[i] != NULL) {
        fprintf(stream, "#%d:<%s> ", i, M(fld->rels[i]));
      }
    }
  }
  fputc(NEWLINE, stream);
  if (fld->sa) {
    fprintf(stream, "strings:\n");
    StringArrayPrint(stream, fld->sa, 0, 0);
  }
  if (fld->objs) {
    fprintf(stream, "parsed objects:\n");
    ObjListPrint(stream, fld->objs);
  }
}

void TablePrint(FILE *stream, Field *flds, int num_flds)
{
  int	i;
  for (i = 0; i < num_flds; i++) {
    TablePrintField(stream, &flds[i], i, num_flds);
  }
}

/* Sense the existence of a table and locate fields by ORing together lines. */
Bool TA_TableFindFields1(char *in, int tabsize,
                         /* RESULTS */ char **table_end, char **nextp,
                         char *fields)
{
  char	lastfields[LINELEN], *lastnewline, *table_end1, *nextp1;
  int	pos, lines;
  lastfields[0] = TERM;
  memset(fields, SPACE, (size_t)LINELEN);
  fields[LINELEN-1] = TERM;
  pos = 0;
  lines = 1;
  lastnewline = in;
  table_end1 = nextp1 = in;
  while (*in) {
    if (*in == TAB) {
      pos = StringTab(pos, tabsize);
      in++;
    } else if (*in == NEWLINE) {
      if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
        fprintf(Log, "%.79s\n", fields);
      }
      if (pos == 0) {
        table_end1 = in;
        nextp1 = in+1;
        break;
      }
      if (!StringInWithin(SPACE, MAXFIELDLEN, fields)) {
        table_end1 = lastnewline;
        nextp1 = lastnewline;
        StringCpy(fields, lastfields, LINELEN);
        break;
      }
      lines++;
      pos = 0;
      in++;
      lastnewline = in;
      if (TA_SeparatorLine(in, &in)) {
        table_end1 = lastnewline;
        nextp1 = in;
        break;
      }
      StringCpy(lastfields, fields, LINELEN);
    } else if (*in != SPACE) {
      fields[pos++] = FLD_FILL;
      in++;
    } else {
      pos++;
      in++;
    }
  }
  if (lines < MINLINES) return(0);
  StringElimTrailingBlanks(fields);
  if (!StringIn(SPACE, fields)) return(0);
  *table_end = table_end1;
  *nextp = nextp1;
  return(1);
}

/* Sense existence of table and return locations of the fields. */
Bool TA_TableFindFields(char *in, int tabsize, /* RESULTS */ char **table_end,
                        char **nextp, Field *flds, int *num_flds)
{
  int		pos, numfields, i;
  char		fields[LINELEN], *p;
  if (!TA_TableFindFields1(in, tabsize, table_end, nextp, fields)) return(0);
  numfields = 0;
  pos = 0;
  p = fields;
  while (1) {
    if (*p == TERM) break;
    if (*p == FLD_FILL) {
      if (numfields >= MAXFIELDS) {
        Dbg(DBGSEMPAR, DBGDETAIL,
            "TA_TableFindFields: too many fields to be table");
        return(0);
      }
      flds[numfields].startpos = pos;
      flds[numfields].class = NULL;
      flds[numfields].lang = F_NULL;
      flds[numfields].sa = NULL;
      flds[numfields].objs = NULL;
skip:
      do {
        p++;
        pos++;
      } while (*p == FLD_FILL);
      if (*p != TERM && *(p + 1) == FLD_FILL) {
      /* Table fields may not be separated by a single space. */
        goto skip;
      }
      flds[numfields].stoppos = pos-1;
      for (i = 0; i < MAXFIELDS; i++) flds[numfields].rels[i] = NULL;
      numfields++;
    } else {
      p++;
      pos++;
    }
  }
  if (numfields >= MINFIELDS) {
    *num_flds = numfields;
    if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
      Dbg(DBGSEMPAR, DBGDETAIL, "TA_Table: table sensed:");
      fprintf(Log, "%s\n", fields);
      fprintf(Log, "%.200s...\n", in);
    }
    return(1);
  }
  return(0);
}

Obj *TA_TableClassDetermine(ObjList *objs, /* RESULTS */ int *count)
{
  ObjList	*p, *parents, *counts;
  Obj		*obj;

  /* Get all parents. */
  parents = NULL;
  for (p = objs; p; p = p->next) {
    if (ObjIsString(p->obj)) {
      parents = ObjListCreate(N("string"), parents);
    } else {
      parents = ObjAncestors1(p->obj, OBJTYPEASYMBOL, 0, 1, parents);
    }
  }

  /* Count parent occurrences. */
  counts = NULL;
  for (p = parents; p; p = p->next) {
    counts = ObjListIncrement(counts, p->obj);
  }

  obj = ObjListMax(counts, count);

  if (objs && DbgOn(DBGSEMPAR, DBGDETAIL)) {
    fprintf(Log, "ALL PARSED OBJECTS:\n");
    ObjListPrint(Log, objs);
    fprintf(Log, "PARENT COUNTS:\n");
    ObjListPrint1(Log, counts, 0, 1, 0, 0);
    fprintf(Log, "DETERMINED PARENT CLASS <%s>\n", M(obj));
  }

  ObjListFree(parents);
  ObjListFree(counts);

  return(obj);
}

void TA_TableParseField(char *in, int tabsize, char *table_end, Discourse *dc,
                        /* RESULTS */ Field *fld)
{
  int		i, fr_count, eng_count;
  Obj		*fr_class, *eng_class;
  ObjList	*fr_objs, *eng_objs, *objs, *r;
  HashTable	*ht;
  StringArray	*sa;

  /* Strip out the field strings. */
  sa = StringArrayStripOutColumn(in, table_end, fld->startpos, fld->stoppos,
                                 tabsize);
  fld->sa = sa;

  /* Parse field strings into objects. */
  fr_objs = eng_objs = NULL;
  for (i = 0; i < sa->len; i++) {
    fr_objs = Sem_ParsePhraseText(sa->strings[i], NULL, FrenchIndex,
                                  fr_objs, dc);
    eng_objs = Sem_ParsePhraseText(sa->strings[i], NULL, EnglishIndex,
                                   eng_objs, dc);
  }

  /* Determine the class of the field. */
  fr_class = TA_TableClassDetermine(fr_objs, &fr_count);
  eng_class = TA_TableClassDetermine(eng_objs, &eng_count);
  ObjListFree(fr_objs);
  ObjListFree(eng_objs);
  if (fr_class && eng_class) {
    if (fr_count > eng_count) {
      fld->lang = F_FRENCH;
      fld->class = fr_class;
    } else {
      fld->lang = F_ENGLISH;
      fld->class = eng_class;
    }
  } else if (fr_class) {
    fld->lang = F_FRENCH;
    fld->class = fr_class;
  } else if (eng_class) {
    fld->lang = F_ENGLISH;
    fld->class = eng_class;
  }

  if (fld->lang == F_NULL) return;

  /* Reparse. Inefficient? We already parsed above, true; but we know the
   * class now.
   */
  ht = LexEntryLangHt1(fld->lang);
  r = NULL;
  for (i = sa->len - 1; i >= 0; i--) {
    objs = Sem_ParsePhraseText(sa->strings[i], fld->class, ht, NULL, dc);
    if (objs) {
      if (!ObjListIsLengthOne(objs)) {
        /* The idea is for the class to disambiguate; this might not always
         * work.
         */
        Dbg(DBGSEMPAR, DBGDETAIL, "ambiguous in table: <%s>", sa->strings[i]);
      }
      r = ObjListCreate(objs->obj, r);
    } else {
      r = ObjListCreate(N("na"), r);
    }
    ObjListFree(objs);
  }

  fld->objs = r;	/* These are reversed, but consistently so. */
}

void TA_TableParseFields(char *in, int tabsize, char *table_end, Field *flds,
                         int num_flds, Discourse *dc)
{
  int	i;
  for (i = 0; i < num_flds; i++) {
    TA_TableParseField(in, tabsize, table_end, dc, &flds[i]);
  }
}

Obj *TA_TableRelationsDetermine1(Field *flds, int elem1_i, int elem2_i)
{
  Obj		*rel;
  ObjList	*p1, *p2, *rels, *counts, *q;
  counts = NULL;
  for (p1 = flds[elem1_i].objs, p2 = flds[elem2_i].objs;
       p1 && p2;
       p1 = p1->next, p2 = p2->next) {
    if (p1->obj == N("na") || p2->obj == N("na")) continue;
    rels = RE(&TsNA, L(ObjWild, p1->obj, p2->obj, E));
    for (q = rels; q; q = q->next) {
      if (ObjLen(q->obj) < 3) continue;
      counts = ObjListIncrement(counts, I(q->obj, 0));
    }
    ObjListFree(rels);
  }
  rel = ObjListMax(counts, NULL);
  if (counts && DbgOn(DBGSEMPAR, DBGDETAIL)) {
    fprintf(Log, "RELATION COUNTS:\n");
    ObjListPrint1(Log, counts, 0, 1, 0, 0);
    fprintf(Log, "DETERMINED RELATION <%s>\n", M(rel));
  }
  ObjListFree(counts);
  return(rel);
}

void TA_TableRelationsDetermine(Field *flds, int num_flds)
{
  int	i, j;
  for (i = 0; i < num_flds; i++) {
    for (j = 0; j < num_flds; j++) {
      if (i == j) flds[i].rels[j] = NULL;
      else flds[i].rels[j] = TA_TableRelationsDetermine1(flds, i, j);
    }
  }
}

void TA_ObjStringPromote(Obj *rel, Obj *obj, int slotnum)
{
  Obj	*restrct;
  if (ObjIsString(obj) && ObjNumParents(obj) == 0 &&
      N("concept") != (restrct = DbGetRestriction(rel, slotnum))) {
    Dbg(DBGSEMPAR, DBGDETAIL, "promote <%s> to <%s>", M(obj), M(restrct));
    ObjAddIsa1(obj, restrct);
  }
}

void TA_TableLearnRels2(Ts *ts, Obj *rel, Obj *elem1_obj, Obj *elem2_obj,
                        Discourse *dc)
{
  Obj		*assertion;
  TsRange	tsr;
  assertion = L(rel, elem1_obj, elem2_obj, E);

  if (ZRE(ts, assertion)) {
    if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
      Dbg(DBGSEMPAR, DBGDETAIL, "assertion already true:");
      ObjPrint(Log, assertion);
      fputc(NEWLINE, Log);
    }
    return;
  }

  TA_ObjStringPromote(rel, elem1_obj, 1);
  TA_ObjStringPromote(rel, elem2_obj, 2);

  TsRangeSetNa(&tsr);
  tsr.startts = *ts;
  ObjSetTsRange(assertion, &tsr);
  /* todo: Perform make-sense checks on assertion. */
  LearnAssert(assertion, dc);
}

void TA_TableLearnRels1(Field *fld2, int obji, Obj *rel, Obj *elem1_obj,
                        Discourse *dc)
{
  int		i;
  ObjList	*p;
  DiscourseSetCurrentChannel(dc, DCIN); /* So that now will be right. */
  for (i = 0, p = fld2->objs; p; p = p->next, i++) {
    if (i == obji) {
      TA_TableLearnRels2(DCNOW(dc), rel, elem1_obj, p->obj, dc);
    }
  }
}

void TA_TableLearnRels(Field *fld, int fieldnum, Obj *obj, int obji,
                       Field *flds, int num_flds, Discourse *dc)
{
  int	i;
  for (i = 0; i < num_flds; i++) {
    if (i == fieldnum) continue;
    if (fld->rels[i]) {
      TA_TableLearnRels1(&flds[i], obji, fld->rels[i], obj, dc);
    }
  }
}

void TA_TableFieldLearn(Field *fld, int fieldnum, Field *flds, int num_flds,
                     Discourse *dc)
{
  int		i;
  ObjList	*p;

  if (fld->class != N("na")) {
    for (i = 0, p = fld->objs; p; p = p->next, i++) {
      if (p->obj == N("na")) {
        p->obj = LearnObjText(fld->class, fld->sa->strings[i], F_NULL,
                              F_SINGULAR, F_NOUN, fld->lang, 0, OBJ_CREATE_C,
                              NULL, dc);
      }
      TA_TableLearnRels(fld, fieldnum, p->obj, i, flds, num_flds, dc);
    }
  }
}

void TA_TableLearn(Field *flds, int num_flds, Discourse *dc)
{
  int	i;
  for (i = 0; i < num_flds; i++) {
    TA_TableFieldLearn(&flds[i], i, flds, num_flds, dc);
  }
}

Bool TA_TableParse(char *in, int tabsize, Discourse *dc,
                   /* RESULTS */ Channel *ch, char **nextp)
{
  int		num_flds;
  char		*table_end;
  Field		*flds;
  flds = MemAlloc(MAXFIELDS*sizeof(Field), "Field array");
  if (!TA_TableFindFields(in, tabsize, &table_end, nextp, flds, &num_flds)) {
    MemFree(flds, "Field array");
    return(0);
  }
  ChannelAddPNode(ch, PNTYPE_TABLE, 1.0, TableCreate(flds, num_flds), NULL,
                  in, table_end);
  *nextp = table_end;
  return(1);
}

void TA_TableProc(char *in, Field *flds, int num_flds, int tabsize,
                  Discourse *dc)
{
  TA_TableParseFields(in, tabsize, NULL, flds, num_flds, dc);
  TA_TableRelationsDetermine(flds, num_flds);
  if (DbgOn(DBGSEMPAR, DBGDETAIL)) TablePrint(Log, flds, num_flds);
  TA_TableLearn(flds, num_flds, dc);
  TableFreeFields(flds, num_flds);
}

/* End of file */
