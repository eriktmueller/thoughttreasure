/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940420: begun
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "repbasic.h"
#include "repchan.h"
#include "repdb.h"
#include "repgrid.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptime.h"
#include "reptrip.h"
#include "semdisc.h"
#include "semgen1.h"
#include "semgen2.h"
#include "semtense.h"
#include "synpnode.h"
#include "tooltest.h"
#include "utildbg.h"

void GenTestRelation()
{
  Ts		ts;
  Obj		*obj;
  PNode		*pn;
  Discourse	*dc;

  TsSetNow(&ts);
  DiscourseContextInit(StdDiscourse, &ts);

  dc = StdDiscourse;
  obj = L(N("nationality-of"), N("Jim"), N("country-USA"), E);
  if ((pn = Generate3(obj, 0, N("present-indicative"), dc))) {
    PNodePrint(Log, DiscourseGetInputChannel(dc), pn);
  }
#ifdef notdef
  if ((pn = GenS_AscriptiveAttachment(obj, N("present-indicative"), dc))) {
    PNodePrint(Log, DiscourseGetInputChannel(dc), pn);
  }
#endif
}

void GenTestTemporal1(Obj *obj1, Obj *obj2, Ts *now)
{
  Obj	*obj;
  *DCNOW(StdDiscourse) = *now;
  obj = GenCreateTemporalRelation(obj1, obj2, StdDiscourse);
  ObjPrint1(Log, obj, NULL, 5, 1, 0, 1, 0);
  fputc(NEWLINE, Log);
  StreamGen(Log, "", obj, '.', StdDiscourse);
}

void GenTestTemporal()
{
  Ts		now, ts;
  Obj		*obj1, *obj2;

  TsSetNow(&ts);
  DiscourseContextInit(StdDiscourse, &ts);

  DiscourseSetLang(StdDiscourse, F_FRENCH);

  Dbg(DBGGEN, DBGDETAIL, "****dress-put-on during take-shower:");
  obj1 = L(N("take-shower"), N("Jim"), E);
  TsRangeParse(ObjToTsRange(obj1), "19940606T080000:19940606T081000");
  obj2 = L(N("dress-put-on"), N("Jim"), N("hat"), E);
  TsRangeParse(ObjToTsRange(obj2), "19940606T080500:19940606T080502");

  TsParse(&now, "19940607");
  GenTestTemporal1(obj1, obj2, &now);

  TsParse(&now, "19940606T080501");
  GenTestTemporal1(obj1, obj2, &now);

  TsParse(&now, "19940605");
  GenTestTemporal1(obj1, obj2, &now);

  Dbg(DBGGEN, DBGDETAIL, "****dress-put-on right before take-shower:");
/*
  TsRangeParse(ObjToTsRange(obj2), "19940606T080000:19940606T080003");
  TsRangeParse(ObjToTsRange(obj1), "19940606T080005:19940606T081000");
 */
  TsRangeParse(ObjToTsRange(obj2), "19940606T075500:19940606T080000");
  TsRangeParse(ObjToTsRange(obj1), "19940606T080000:19940606T081000");

  TsParse(&now, "19940607");
  GenTestTemporal1(obj1, obj2, &now);

  TsParse(&now, "19940606T080020");
  GenTestTemporal1(obj1, obj2, &now);

  TsParse(&now, "19940605");
  GenTestTemporal1(obj1, obj2, &now);

  Dbg(DBGGEN, DBGDETAIL, "****take-shower right before dress-put-on:");
  TsRangeParse(ObjToTsRange(obj1), "19940606T080005:19940606T081000");
  TsRangeParse(ObjToTsRange(obj2), "19940606T081010:19940606T081012");

  TsParse(&now, "19940607");
  GenTestTemporal1(obj1, obj2, &now);

  TsParse(&now, "19940606T080501");
  GenTestTemporal1(obj1, obj2, &now);

  TsParse(&now, "19940605");
  GenTestTemporal1(obj1, obj2, &now);

  Dbg(DBGGEN, DBGDETAIL, "****dress-put-on an hour before take-shower:");
  TsRangeParse(ObjToTsRange(obj1), "19940606T080005:19940606T081000");
  TsRangeParse(ObjToTsRange(obj2), "19940606T070000:19940606T070003");

  TsParse(&now, "19940607");
  GenTestTemporal1(obj1, obj2, &now);

  TsParse(&now, "19940606T080501");
  GenTestTemporal1(obj1, obj2, &now);

  TsParse(&now, "19940605");
  GenTestTemporal1(obj1, obj2, &now);

  Dbg(DBGGEN, DBGDETAIL, "****dress-put-on a day before take-shower:");
  TsRangeParse(ObjToTsRange(obj1), "19940606T080005:19940606T081000");
  TsRangeParse(ObjToTsRange(obj2), "19940605T070000:19940605T070003");

  TsParse(&now, "19940607");
  GenTestTemporal1(obj1, obj2, &now);

  TsParse(&now, "19940606T080501");
  GenTestTemporal1(obj1, obj2, &now);

  TsParse(&now, "19940605T070000");
  GenTestTemporal1(obj1, obj2, &now);

  TsParse(&now, "19940604");
  GenTestTemporal1(obj1, obj2, &now);
}

ObjList *TenseAllCompound()
{
  ObjList	*r;
  r = NULL;
  r = ObjListCreate(N("infinitive"), r);
  r = ObjListCreate(N("infinitive-with-to"), r);
  r = ObjListCreate(N("past-infinitive"), r);
  r = ObjListCreate(N("past-infinitive-with-to"), r);
  r = ObjListCreate(N("present-participle"), r);
  r = ObjListCreate(N("past-participle"), r);
  r = ObjListCreate(N("past-present-participle"), r);
  r = ObjListCreate(N("progressive-past-participle"), r);
  r = ObjListCreate(N("pluperfect-indicative"), r);
  r = ObjListCreate(N("pluperfect-subjunctive"), r);
  r = ObjListCreate(N("preterit-anterior"), r);
  r = ObjListCreate(N("conditional-perfect"), r);
  r = ObjListCreate(N("past-past-recent"), r);
  r = ObjListCreate(N("preterit-indicative"), r);
  r = ObjListCreate(N("preterit-subjunctive"), r);
  r = ObjListCreate(N("do-past"), r);
  r = ObjListCreate(N("imperfect-indicative"), r);
  r = ObjListCreate(N("imperfect-subjunctive"), r);
  r = ObjListCreate(N("past-progressive"), r);
  r = ObjListCreate(N("simple-past"), r);
  r = ObjListCreate(N("perfect-indicative"), r);
  r = ObjListCreate(N("perfect-subjunctive"), r);
  r = ObjListCreate(N("past-future-near"), r);
  r = ObjListCreate(N("conditional"), r);
  r = ObjListCreate(N("conditional-progressive"), r);
  r = ObjListCreate(N("past-recent"), r);
  r = ObjListCreate(N("present-indicative"), r);
  r = ObjListCreate(N("do-present"), r);
  r = ObjListCreate(N("present-progressive"), r);
  r = ObjListCreate(N("present-process-progressive"), r);
  r = ObjListCreate(N("present-subjunctive"), r);
  r = ObjListCreate(N("imperative"), r);
  r = ObjListCreate(N("future-near"), r);
  r = ObjListCreate(N("future-perfect"), r);
  r = ObjListCreate(N("future-perfect-progressive"), r);
  r = ObjListCreate(N("future"), r);
  r = ObjListCreate(N("future-progressive"), r);
  r = ObjListCreate(N("should-simple"), r);
  r = ObjListCreate(N("should-progressive"), r);
  r = ObjListCreate(N("should-perfect-simple"), r);
  r = ObjListCreate(N("should-perfect-progressive"), r);
  r = ObjListCreate(N("double-compound-perfect-indicative"), r);
  r = ObjListCreate(N("double-compound-pluperfect-indicative"), r);
  r = ObjListCreate(N("double-compound-future-perfect"), r);
  r = ObjListCreate(N("double-compound-preterit-anterior"), r);
  r = ObjListCreate(N("double-compound-conditional-perfect"), r);
  r = ObjListCreate(N("double-compound-perfect-subjunctive"), r);
  r = ObjListCreate(N("double-compound-pluperfect-subjunctive"), r);
  r = ObjListCreate(N("double-compound-past-participle"), r);
  r = ObjListCreate(N("double-compound-past-infinitive"), r);
  return(ObjListReverse(r));
}

void GenTestCompTense()
{
  GenTestCompTense1(Log);
}

void GenTestCompTense2(Obj *gentense, PNode *pn, int lang)
{
  LexEntry	*mainverb;
  Obj		*tense, *mood;
  PNode		*adv1, *adv2, *adv3, *pn_mainverb, *pn_agreeverb;
  TenseParseCompTense(pn, lang, &pn_mainverb, &mainverb, &pn_agreeverb,
                      &tense, &mood, &adv1, &adv2, &adv3);
/*
  PNodePrettyPrint(Log, NULL, pn);
 */
  PNodePrint1(Log, NULL, pn, 0);
  fputc(NEWLINE, Log);
  Dbg(DBGGEN, DBGHYPER,
      "parsed tense <%s> mood <%s> adv1 <%s> adv2 <%s> adv3 <%s>",
      M(tense), M(mood),
      adv1 ? adv1->lexitem->le->srcphrase : "",
      adv2 ? adv2->lexitem->le->srcphrase : "",
      adv3 ? adv3->lexitem->le->srcphrase : "");
  if (gentense != tense) {
    Dbg(DBGGEN, DBGHYPER, "parse tense <%s> != gen tense <%s>", M(tense),
        M(gentense));
  }
}

void GenTestCompTense1(FILE *stream)
{
  ObjList	*allcomp, *p;
  LexEntry	*le, *notle, *nele, *pasle, *advle;
  PNode		*pn;
  Discourse	*dc;
  dc = StdDiscourse;
  DC(dc).dialect = F_NULL;
  DC(dc).style = F_NULL;
  DC(dc).infrequent_ok = 1;
  if (!(notle = LexEntryFind("not", "B", EnglishIndex))) return;
  if (!(le = LexEntryFind("be", "V", EnglishIndex))) return;
  if (!(advle = LexEntryFind("happily", "B", EnglishIndex))) return;
  DiscourseSetLang(dc, F_ENGLISH);
  allcomp = TenseAllCompound();
  for (p = allcomp; p; p = p->next) {
    fprintf(stream, "%s: ", M(p->obj));
    if ((pn = TenseGenVerbEnglish(le, NULL, NULL, p->obj, F_PLURAL,
                                  F_SECOND_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
    if ((pn = TenseGenVerbEnglish(le, NULL, NULL, p->obj, F_SINGULAR,
                                  F_THIRD_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
    if ((pn = TenseGenVerbEnglish(le, NULL, NULL, p->obj, F_PLURAL,
                                 F_FIRST_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
  }
  for (p = allcomp; p; p = p->next) {
    fprintf(stream, "English verb type <%s>\n", M(p->obj));
    if ((pn = TenseGenVerbEnglish(le, NULL, notle, p->obj, F_SINGULAR,
                                  F_FIRST_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
    if ((pn = TenseGenVerbEnglish(le, NULL, notle, p->obj, F_SINGULAR,
                                  F_THIRD_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
    if ((pn = TenseGenVerbEnglish(le, NULL, notle, p->obj, F_PLURAL,
                                  F_FIRST_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
  }
  if (!(le = LexEntryFind("change", "V", EnglishIndex))) return;
  DiscourseSetLang(dc, F_ENGLISH);
  allcomp = TenseAllCompound();
  for (p = allcomp; p; p = p->next) {
    fprintf(stream, "English verb type <%s>\n", M(p->obj));
    if ((pn = TenseGenVerbEnglish(le, NULL, NULL, p->obj, F_SINGULAR,
                                  F_FIRST_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
    if ((pn = TenseGenVerbEnglish(le, NULL, NULL, p->obj, F_SINGULAR,
                                  F_THIRD_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
    if ((pn = TenseGenVerbEnglish(le, NULL, NULL, p->obj, F_PLURAL,
                                  F_FIRST_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
  }
  for (p = allcomp; p; p = p->next) {
    fprintf(stream, "English verb type <%s>\n", M(p->obj));
    if ((pn = TenseGenVerbEnglish(le, NULL, notle, p->obj, F_SINGULAR,
                                  F_FIRST_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
    if ((pn = TenseGenVerbEnglish(le, NULL, notle, p->obj, F_SINGULAR,
                                  F_THIRD_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
    if ((pn = TenseGenVerbEnglish(le, NULL, notle, p->obj, F_PLURAL,
                                  F_FIRST_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
  }
  for (p = allcomp; p; p = p->next) {
    fprintf(stream, "English verb type <%s>\n", M(p->obj));
    if ((pn = TenseGenVerbEnglish(le, advle, notle, p->obj, F_SINGULAR,
                                  F_FIRST_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
    if ((pn = TenseGenVerbEnglish(le, advle, notle, p->obj, F_SINGULAR,
                                  F_THIRD_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
    if ((pn = TenseGenVerbEnglish(le, advle, notle, p->obj, F_PLURAL,
                                  F_FIRST_PERSON, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_ENGLISH);
    }
  }
  if (!(nele = LexEntryFind("ne", "B", FrenchIndex))) return;
  if (!(pasle = LexEntryFind("pas", "B", FrenchIndex))) return;
  if (!(advle = LexEntryFind("bien", "B", FrenchIndex))) return;
  DiscourseSetLang(dc, F_FRENCH);
  if (!(le = LexEntryFind("attendre", "V", FrenchIndex))) return;
  for (p = allcomp; p; p = p->next) {
    fprintf(stream, "%s: ", M(p->obj));
    if ((pn = TenseGenVerbFrench(le, "", NULL, NULL, NULL, p->obj, F_MASCULINE,
                                 F_SINGULAR, F_FIRST_PERSON, F_NULL, F_NULL,
                                 F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
    if ((pn = TenseGenVerbFrench(le, "", NULL, NULL, NULL, p->obj, F_MASCULINE,
                                 F_SINGULAR, F_SECOND_PERSON, F_NULL, F_NULL,
                                 F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
    if ((pn = TenseGenVerbFrench(le, "", NULL, NULL, NULL, p->obj, F_MASCULINE,
                                 F_SINGULAR, F_THIRD_PERSON, F_NULL, F_NULL,
                                 F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
    if ((pn = TenseGenVerbFrench(le, "", NULL, NULL, NULL, p->obj, F_MASCULINE,
                                 F_PLURAL, F_FIRST_PERSON, F_NULL, F_NULL,
                                 F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
    if ((pn = TenseGenVerbFrench(le, "", NULL, NULL, NULL, p->obj, F_MASCULINE,
                                 F_PLURAL, F_SECOND_PERSON, F_NULL, F_NULL,
                                 F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
    if ((pn = TenseGenVerbFrench(le, "", NULL, NULL, NULL, p->obj, F_MASCULINE,
                                 F_PLURAL, F_THIRD_PERSON, F_NULL, F_NULL,
                                 F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
  }
  if (!(le = LexEntryFind("arriver", "V", FrenchIndex))) return;
  for (p = allcomp; p; p = p->next) {
    fprintf(stream, "French verb type <%s>\n", M(p->obj));
    if ((pn = TenseGenVerbFrench(le, "l", nele, pasle, advle, p->obj,
                                 F_MASCULINE, F_SINGULAR, F_FIRST_PERSON,
                                 F_NULL, F_NULL, F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
    if ((pn = TenseGenVerbFrench(le, "l", nele, pasle, advle, p->obj,
                                 F_MASCULINE, F_SINGULAR, F_SECOND_PERSON,
                                 F_NULL, F_NULL, F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
    if ((pn = TenseGenVerbFrench(le, "l", nele, pasle, advle, p->obj,
                                 F_MASCULINE, F_SINGULAR, F_THIRD_PERSON,
                                 F_NULL, F_NULL, F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
    if ((pn = TenseGenVerbFrench(le, "l", nele, pasle, advle, p->obj,
                                 F_FEMININE, F_SINGULAR, F_THIRD_PERSON,
                                 F_NULL, F_NULL, F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
    if ((pn = TenseGenVerbFrench(le, "l", nele, pasle, advle, p->obj,
                                 F_MASCULINE, F_PLURAL, F_FIRST_PERSON,
                                 F_NULL, F_NULL, F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
    if ((pn = TenseGenVerbFrench(le, "l", nele, pasle, advle, p->obj,
                                 F_MASCULINE, F_PLURAL, F_SECOND_PERSON,
                                 F_NULL, F_NULL, F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
    if ((pn = TenseGenVerbFrench(le, "l", nele, pasle, advle, p->obj,
                                 F_MASCULINE, F_PLURAL, F_THIRD_PERSON,
                                 F_NULL, F_NULL, F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
    if ((pn = TenseGenVerbFrench(le, "l", nele, pasle, advle, p->obj,
                                 F_FEMININE, F_PLURAL, F_THIRD_PERSON,
                                 F_NULL, F_NULL, F_NULL, 0, dc))) {
      GenTestCompTense2(p->obj, pn, F_FRENCH);
    }
  }
}

/* todo: Because timestamps do not go back far enough, we mostly get things
 * like "Tom Brokaw is an anchorperson, as soon as Immanuel Kant was a
 * philosopher."
 */
void GenTestTemporalDb()
{
  GenTestTemporalDb1(L(N("occupation-of"), ObjWild, ObjWild, E), 0);
}

void GenTestTemporalDb1(Obj *ptn, int pivot)
{
  int		i, i1, i2, len;
  Obj		*obj1, *obj2;
  ObjList	*objs;
  Ts		now;
  ObjPrint(Log, ptn);
  fprintf(Log, "\n");
  objs = DbRetrieveDesc(&TsNA, NULL, ptn, pivot, 1, 0);
  len = ObjListLen(objs);
  if (len >= 2) {
    for (i = 0; i < 5; i++) {
      i1 = Roll(len);
      obj1 = ObjListIth(objs, i1);
      while(N("V1") == I(obj1, 0)) {
        i1 = Roll(len);
        obj1 = ObjListIth(objs, i1);
      }
      i2 = Roll(len);
      while (i2 == i1) i2 = Roll(len);
      obj2 = ObjListIth(objs, i2);
      while(N("V1") == I(obj2, 0)) {
        while (i2 == i1) i2 = Roll(len);
        obj2 = ObjListIth(objs, i2);
      }
      ObjPrettyPrint(Log, obj1);
      ObjPrettyPrint(Log, obj2);
      fprintf(Log, "*** now = 2015\n");
      TsParse(&now, "2015");
	  GenTestTemporal1(obj1, obj2, &now);
      fprintf(Log, "*** now = 1950\n");
      TsParse(&now, "1950");
	  GenTestTemporal1(obj1, obj2, &now);
	}
  } else {
    fprintf(Log, "no matches\n");
  }
}

void GridFindPathTest(Grid *gs)
{
  GridSubspace	*gp;
  GridCoord	fromrow, fromcol, torow, tocol;
  Grid		*gs1;
  int		i;
  for (i = 0; i < 6; i++) {
    fromrow = 1+Roll(gs->rows-2);
    torow = 1+Roll(gs->rows-2);
    fromcol = 1+Roll(gs->cols-2);
    tocol = 1+Roll(gs->cols-2);
    if (!(gp = GridFindPath(gs, fromrow, fromcol, torow, tocol, "~ p"))) {
      Dbg(DBGGS, DBGBAD, "path not found");
    } else {
      gs1 = GridCopy(gs);
      GridSetSubspace(gs1, gp, GR_FILL);
      GridPrint(Log, "grid with path", gs1, 1);
    }
  }
}

void TestGenTsRange()
{
  DiscourseSetLang(StdDiscourse, F_FRENCH);
  TestGenTsRange1();
  DiscourseSetLang(StdDiscourse, F_ENGLISH);
  TestGenTsRange1();
}

void TestGenTsRange2(Obj *obj)
{
  PNode	*pn;
  TsPrint(Log, &(ObjToTsRange(obj)->startts));
  fputc(NEWLINE, Log);
  Dbg(DBGGEN, DBGDETAIL, "GenTsRange");
  if ((pn = GenTsRange(obj, 0, StdDiscourse))) PNodePrint(Log, NULL, pn);
  Dbg(DBGGEN, DBGDETAIL, "GenDurRelativeToNow");
  if ((pn = GenDurRelativeToNow(&ObjToTsRange(obj)->startts, 0, 0, 1,
                                StdDiscourse))) {
    PNodePrint(Log, NULL, pn);
  }
  Dbg(DBGGEN, DBGDETAIL, "GenDurRelativeToNow use word");
  if ((pn = GenDurRelativeToNow(&ObjToTsRange(obj)->startts, 0, 1, 1,
                                StdDiscourse))) {
    PNodePrint(Log, NULL, pn);
  }
  Dbg(DBGGEN, DBGDETAIL, "GenDate");
  if ((pn = GenDate(&ObjToTsRange(obj)->startts, 1, 1, 1, 1, StdDiscourse,
                    NULL))) {
    PNodePrint(Log, NULL, pn);
  }
  Dbg(DBGGEN, DBGDETAIL, "GenTod");
  if ((pn = GenTod(&ObjToTsRange(obj)->startts, StdDiscourse))) {
    PNodePrint(Log, NULL, pn);
  }
  Dbg(DBGGEN, DBGDETAIL, "GenDayAndPartOfTheDay");
  if ((pn = GenDayAndPartOfTheDay(&ObjToTsRange(obj)->startts, 1,
                                  StdDiscourse))) {
    PNodePrint(Log, NULL, pn);
  }
}

void TestGenTsRange1()
{
  Obj		*obj;

  TsParse(DCNOW(StdDiscourse), "19940712T094416");
  obj = L(N("dress-put-on"), N("Jim"), N("hat"), E);

  Dbg(DBGGEN, DBGDETAIL, "****two days ago");
  TsRangeParse(ObjToTsRange(obj), "19940710T090000:19940710T090000");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****yesterday aftern.");
  TsRangeParse(ObjToTsRange(obj), "19940711T140000:19940711T140000");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****yesterday night");
  TsRangeParse(ObjToTsRange(obj), "19940711T234552:19940711T234552");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****3 hrs");
  TsRangeParse(ObjToTsRange(obj), "19940712T064416:19940712T064416");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****60 mins");
  TsRangeParse(ObjToTsRange(obj), "19940712T084416:19940712T084416");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****-60 secs");
  TsRangeParse(ObjToTsRange(obj), "19940712T094316:19940712T094316");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****-1 sec");
  TsRangeParse(ObjToTsRange(obj), "19940712T094415:19940712T094415");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****now");
  TsRangeParse(ObjToTsRange(obj), "19940712T094416:19940712T094416");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****1 sec");
  TsRangeParse(ObjToTsRange(obj), "19940712T094417:19940712T094417");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****2 secs");
  TsRangeParse(ObjToTsRange(obj), "19940712T094418:19940712T094418");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****~90 secs");
  TsRangeParse(ObjToTsRange(obj), "19940712T094547:19940712T094547");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****~5 minutes");
  TsRangeParse(ObjToTsRange(obj), "19940712T094923:19940712T094923");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****~45 minutes");
  TsRangeParse(ObjToTsRange(obj), "19940712T103016:19940712T103016");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****2 hours");
  TsRangeParse(ObjToTsRange(obj), "19940712T114416:19940712T114416");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****17 after noon");
  TsRangeParse(ObjToTsRange(obj), "19940712T121701:19940712T121701");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****15 before 1");
  TsRangeParse(ObjToTsRange(obj), "19940712T124601:19940712T124601");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****4 hours");
  TsRangeParse(ObjToTsRange(obj), "19940712T135916:19940712T135916");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****8 hours");
  TsRangeParse(ObjToTsRange(obj), "19940712T160916:19940712T160916");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****15 hours");
  TsRangeParse(ObjToTsRange(obj), "19940712T234416:19940712T234416");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****1 day");
  TsRangeParse(ObjToTsRange(obj), "19940713T092916:19940713T092916");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****2 days");
  TsRangeParse(ObjToTsRange(obj), "19940714T090016:19940714T090016");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****3 days");
  TsRangeParse(ObjToTsRange(obj), "19940715T091516:19940715T091516");
  TestGenTsRange2(obj);
  Dbg(DBGGEN, DBGDETAIL, "****1 week");
  TsRangeParse(ObjToTsRange(obj), "19940719T093016:19940719T093016");
  TestGenTsRange2(obj);
}

void TestGen(Obj *con)
{
  ObjPrint(Log, con);
  fputc(NEWLINE, Log);
  DiscourseGen(StdDiscourse, 0, "", con, '.');
}

void TestGenSpeechActs1(Obj *head)
{
  TestGen(L(head,
            N("Lucie"),
            N("Martine-Dad"),
            L(N("hand-to"), N("Lucie"), N("Martine-Dad"), N("saltshaker"), E),
            E));
  TestGen(L(head,
            N("Lucie"),
            ObjNA,
            L(N("hand-to"), N("Lucie"), N("Martine-Dad"), N("saltshaker"), E),
            E));
  TestGen(L(head,
            N("Lucie"),
            N("Martine-Dad"),
            ObjNA,
            E));
  TestGen(L(head,
            N("Lucie"),
            ObjNA,
            ObjNA,
            E));
  /* Don't bother testing the cases without a subject. */
  TestGen(L(head,
            N("Lucie"),
            N("Martine-Dad"),
            L(N("hand-to"), N("Martine-Dad"), N("Lucie"), N("saltshaker"), E),
            E));
  TestGen(L(head,
            N("Lucie"),
            ObjNA,
            L(N("hand-to"), N("Martine-Dad"), N("Lucie"), N("saltshaker"), E),
            E));
}

void TestGenSpeechActs()
{
  Ts	ts;

  TsSetNow(&ts);
  DiscourseContextInit(StdDiscourse, &ts);
  TestGenSpeechActs1(N("intimidate"));
  TestGenSpeechActs1(N("say-to"));
  TestGenSpeechActs1(N("persuade"));
  TestGenSpeechActs1(N("indicate"));
  TestGenSpeechActs1(N("accuse"));
  TestGenSpeechActs1(N("admit"));
  TestGenSpeechActs1(N("concede"));
  TestGenSpeechActs1(N("confess"));
  TestGenSpeechActs1(N("assert"));
  TestGenSpeechActs1(N("claim"));
  TestGenSpeechActs1(N("declare"));
  TestGenSpeechActs1(N("observe"));
  TestGenSpeechActs1(N("deny"));
  TestGenSpeechActs1(N("joke"));
  TestGenSpeechActs1(N("advise"));
  TestGenSpeechActs1(N("recommend"));
  TestGenSpeechActs1(N("request"));
  TestGenSpeechActs1(N("speech-act-plead"));
  TestGenSpeechActs1(N("pray"));
  TestGenSpeechActs1(N("speech-act-order"));
  TestGenSpeechActs1(N("propose"));
  TestGenSpeechActs1(N("promise"));
  TestGenSpeechActs1(N("active-goal"));
  TestGenSpeechActs1(N("accept"));
  TestGenSpeechActs1(N("reject"));
  TestGenSpeechActs1(N("present"));
}

void TestTrip()
{
  Ts	leave_after, arrive_before;

  TsParse(&leave_after,   "19940812T080000");
  TsParse(&arrive_before, "19940813T080000");
  TripFind(N("Jim"), N("apt4d20rd"), N("JFK-gate-26"),
           &leave_after, &arrive_before);

  TsParse(&leave_after,   "19940812T080000");
  TsParse(&arrive_before, "19940812T180000");
  TripFind(N("Martine"), N("Martine-apt"), N("maison-de-Lucie-Entrance"),
           &leave_after, &arrive_before);
}

void TestGenAttr1(Obj *attr)
{
  TestGen(L(attr, N("Jim"), E));
  TestGen(L(attr, N("Jim"), D(1.0), E));
  TestGen(L(attr, N("Jim"), D(0.91), E));
  TestGen(L(attr, N("Jim"), D(0.81), E));
  TestGen(L(attr, N("Jim"), D(0.71), E));
  TestGen(L(attr, N("Jim"), D(0.61), E));
  TestGen(L(attr, N("Jim"), D(WEIGHT_DEFAULT), E));
  TestGen(L(attr, N("Jim"), D(0.51), E));
  TestGen(L(attr, N("Jim"), D(0.41), E));
  TestGen(L(attr, N("Jim"), D(0.31), E));
  TestGen(L(attr, N("Jim"), D(0.21), E));
  TestGen(L(attr, N("Jim"), D(0.11), E));
  TestGen(L(attr, N("Jim"), D(0.01), E));
  TestGen(L(attr, N("Jim"), D(0.0), E));
  TestGen(L(attr, N("Jim"), D(-0.01), E));
  TestGen(L(attr, N("Jim"), D(-0.11), E));
  TestGen(L(attr, N("Jim"), D(-0.21), E));
  TestGen(L(attr, N("Jim"), D(-0.31), E));
  TestGen(L(attr, N("Jim"), D(-0.41), E));
  TestGen(L(attr, N("Jim"), D(-0.51), E));
  TestGen(L(attr, N("Jim"), D(-WEIGHT_DEFAULT), E));
  TestGen(L(attr, N("Jim"), D(-0.61), E));
  TestGen(L(attr, N("Jim"), D(-0.71), E));
  TestGen(L(attr, N("Jim"), D(-0.81), E));
  TestGen(L(attr, N("Jim"), D(-0.91), E));
  TestGen(L(attr, N("Jim"), D(-1.0), E));
  TestGen(L(N("not"), L(attr, N("Jim"), E), E));
  TestGen(L(N("not"), L(attr, N("Jim"), D(0.81), E), E));
  TestGen(L(N("not"), L(attr, N("Jim"), D(0.50), E), E));
  TestGen(L(N("not"), L(attr, N("Jim"), D(0.0), E), E));
  TestGen(L(N("not"), L(attr, N("Jim"), D(-0.50), E), E));
  TestGen(L(N("not"), L(attr, N("Jim"), D(-0.81), E), E));
}

void TestGenAttr()
{
  TestGenAttr1(N("intelligent"));
  TestGenAttr1(N("rich"));
  TestGenAttr1(N("good"));
}

/* End of file. */
