/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "lexentry.h"
#include "lextheta.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptime.h"
#include "toolgrd.h"

void GrinderGrind()
{
  GrinderGrindObjs("ttkb/obj.txt.unsorted");
  GrinderGrindLes("ttkb/le.txt.unsorted");
  GrinderGrindInfls("ttkb/infl.txt.unsorted");
}

void GrinderPreambleWrite(FILE *stream)
{
}

void GrinderGrindObjs(char *fn)
{
  Obj  *obj;
  FILE *stream;

  if (NULL == (stream = StreamOpenEnv(fn, "w+"))) return;
  GrinderPreambleWrite(stream);
  for (obj = Objs; obj; obj = obj->next) {
    if (!ObjIsSymbol(obj)) continue;
    if (!ISA(N("concept"), obj)) continue;
    GriderGrindObj(stream, obj);
  }
  StreamClose(stream);
}

void GrinderAssertionWrite2(FILE *stream, Obj *pred, Obj *arg1, Obj *arg2)
{
  fputc(LBRACKET, stream);
  fputs(ObjToName(pred), stream);
  fputc(' ', stream);
  fputs(ObjToName(arg1), stream);
  fputc(' ', stream);
  fputs(ObjToName(arg2), stream);
  fputc(RBRACKET, stream);
}

void GrinderAssertionWrite(FILE *stream, Obj *obj)
{
  if (TsRangeIsAlways(ObjToTsRange(obj))) {
    ObjPrint1(stream, obj, NULL, 5, 0, 0, 0, 0);
  } else {
    ObjPrint1(stream, obj, NULL, 5, 1, 0, 0, 0);
  }
}

void GrinderFeatureWrite(FILE *stream, char *s)
{
  fputc(LE_SEP, stream);
  fputs(s, stream);
  fputc(LE_SEP, stream);
}

void GrinderWordWrite(FILE *stream, char *s)
{
  char *p;
  for (p = s; p[0] != TERM; p++) {
    if (IsWhitespace(p[0])) fputc('_', stream);
    else fputc(p[0], stream);
  }
}

void GrinderLeWriteUid(FILE *stream, LexEntry *le)
{
  int  gender;
  GrinderWordWrite(stream, le->srcphrase);
  fputc('-', stream);
  gender = FeatureGet(le->features, FT_GENDER);
  if (gender != F_NULL) fputc(gender, stream);
  fputc(FeatureGet(le->features, FT_POS), stream);
  fputc(FeatureGet(le->features, FT_LANG), stream);
}

void GrinderPhraseSepWrite(FILE *stream, char *s)
{
  char *p;
  fputc(F_REALLY, stream);
  if (!s) {
    fputc(F_REALLY, stream);
    return;
  }
  for (p = s; p[0] != TERM; p++) {
    if (p[0] == NEWLINE) {
      fputc(F_REALLY, stream);
    } else if (p[0] == ' ') {
      fputc('_', stream);
    } else {
     fputc(p[0], stream);
    }
  }
}

void GrinderOleWrite(FILE *stream, ObjToLexEntry *ole)
{
  GrinderLeWriteUid(stream, ole->le);
}

void GriderGrindObj(FILE *stream, Obj *obj)
{
  Obj           *isa_rel;
  ObjList       *parents, *children, *assertions, *p;
  ObjToLexEntry *ole;

  fputs(ObjToName(obj), stream);
  fputc(' ', stream);

  for (ole = obj->ole; ole; ole = ole->next) {
    GrinderOleWrite(stream, ole);
    fputc(' ', stream);
  }

  parents = ObjParents(obj, OBJTYPEANY);
  if (ObjIsConcrete(obj)) isa_rel = N("isa");
  else isa_rel = N("ako");
  for (p = parents; p; p = p->next) {
    if (!ObjIsSymbol(p->obj)) continue;
    GrinderAssertionWrite2(stream, isa_rel, obj, p->obj);
    fputc(' ', stream);
  }
  ObjListFree(parents);

  children = ObjChildren(obj, OBJTYPEANY);
  for (p = children; p; p = p->next) {
    if (!ObjIsSymbol(p->obj)) continue;
    if (ObjIsConcrete(p->obj)) isa_rel = N("isa");
    else isa_rel = N("ako");
    GrinderAssertionWrite2(stream, isa_rel, p->obj, obj);
    fputc(' ', stream);
  }
  ObjListFree(children);

  assertions = DbRetrieveInvolving(&TsNA, NULL, obj, 1, NULL);
  for (p = assertions; p; p = p->next) {
    if (ISA(N("relation-on-the-fly"), I(p->obj, 0))) continue;
    GrinderAssertionWrite(stream, p->obj);
    fputc(' ', stream);
  }
  ObjListFree(assertions);

  fputc(NEWLINE, stream);
}

void GrinderGrindLes(char *fn)
{
  LexEntry *le;
  FILE     *stream;

  if (NULL == (stream = StreamOpenEnv(fn, "w+"))) return;
  GrinderPreambleWrite(stream);
  for (le = AllLexEntries; le; le = le->next) {
    GrinderGrindLe(stream, le);
  }
  StreamClose(stream);
}

void GrinderThetaRoleWrite(FILE *stream, ThetaRole *tr, int slotnum)
{
  if (tr->cas == N("expl")) fputc(':', stream);
  else fprintf(stream, "%d:", slotnum);
  fputs(ObjToName(tr->cas), stream);
  fputc(':', stream);
  if (tr->le) GrinderLeWriteUid(stream, tr->le);
  fputc(':', stream);
  if (tr->subcat != F_NULL) fputc(tr->subcat, stream);
  fputc(':', stream);
  fputs(TRPOSToString(tr->position), stream);
  fputc(':', stream);
  if (tr->isoptional) fputc('1', stream);
  else fputc('0', stream);
}

void GrinderThetaRolesWrite(FILE *stream, ThetaRole *theta_roles)
{
  int   slotnum;
  for (slotnum = 1; theta_roles; theta_roles = theta_roles->next, slotnum++) {
    if (theta_roles->cas == N("expl")) { slotnum--; }
    fputc(' ', stream);
    GrinderThetaRoleWrite(stream, theta_roles, slotnum);
  }
}

void GrinderLeoWrite(FILE *stream, LexEntryToObj *leo)
{
  fputs(ObjToName(leo->obj), stream);
  fputc(' ', stream);
  GrinderFeatureWrite(stream, leo->features);
  GrinderThetaRolesWrite(stream, leo->theta_roles);
}

void GrinderGrindLe(FILE *stream, LexEntry *le)
{
  LexEntryToObj *leo;
  GrinderLeWriteUid(stream, le);
  fputc(' ', stream);
  GrinderFeatureWrite(stream, le->features);
  fputc(' ', stream);
  GrinderPhraseSepWrite(stream, le->phrase_seps);
  fputc(' ', stream);
  for (leo = le->leo; leo; leo = leo->next) {
    GrinderLeoWrite(stream, leo);
    fputc(' ', stream);
  }
  fputc(NEWLINE, stream);
}

void GrinderGrindWord(FILE *stream, Word *w, LexEntry *le)
{
  GrinderWordWrite(stream, w->word);
  fputc(' ', stream);
  GrinderFeatureWrite(stream, w->features);
  fputc(' ', stream);
  GrinderLeWriteUid(stream, le);
  fputc(NEWLINE, stream);
}

void GrinderGrindWords(FILE *stream, Word *words, LexEntry *le)
{
  Word *w;
  for (w = words; w; w = w->next) {
    if (w->word[0] == TERM) continue;
    GrinderGrindWord(stream, w, le);
  }
}

void GrinderGrindInfls(char *fn)
{
  LexEntry *le;
  FILE     *stream;

  if (NULL == (stream = StreamOpenEnv(fn, "w+"))) return;
  GrinderPreambleWrite(stream);
  for (le = AllLexEntries; le; le = le->next) {
    GrinderGrindWords(stream, le->infl, le);
  }
  StreamClose(stream);
}

/* End of file. */
