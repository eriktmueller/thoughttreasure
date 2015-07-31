/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940916: converted to Text and incorporated new feature print utilities
 * 19940919: isa printing
 * 19940920: contrast printing
 * 19940921: redid tables
 * 19950530: slight HTML mods
 */

#include "tt.h"
#include "appassoc.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "lextheta.h"
#include "repbasic.h"
#include "repchan.h"
#include "repcxt.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptext.h"
#include "reptime.h"
#include "semanaph.h"
#include "semdisc.h"
#include "semgen1.h"
#include "semgen2.h"
#include "synparse.h"
#include "synpnode.h"
#include "toolrpt.h"
#include "ua.h"
#include "uaanal.h"
#include "uaasker.h"
#include "uadict.h"
#include "uaemot.h"
#include "uafriend.h"
#include "uagoal.h"
#include "uaoccup.h"
#include "uapaappt.h"
#include "uapagroc.h"
#include "uapashwr.h"
#include "uapaslp.h"
#include "uaquest.h"
#include "uarel.h"
#include "uaspace.h"
#include "uatime.h"
#include "uatrade.h"
#include "uaweath.h"
#include "utilbb.h"
#include "utildbg.h"
#include "utilhtml.h"
#include "utillrn.h"
#include "utilrpt.h"

/* HELPING FUNCTIONS */

ObjList *DictRelationsAndCounts(Ts *ts, TsRange *tsr, Obj *obj, int i,
                                ObjList *rels_and_counts, ObjList *assertions,
                                /* RESULTS */ ObjList **assertions_p)
{
  ObjList	*prev_assertions, *p;
  prev_assertions = assertions;
  /* Gather assertions involving obj. */
  assertions = DbRetrieval(ts, tsr,
                           i == 1 ? L(ObjWild, obj, E) :
                                    L(ObjWild, ObjWild, obj, E),
                           assertions, NULL, 1);
  /* Increment relation counts. */
  for (p = assertions; p != prev_assertions; p = p->next) {
    if (ISA(N("restriction"), I(p->obj, 0))) continue;
    if (ISA(N("V1"), I(p->obj, 0))) continue;
    if (ISA(N("GS"), I(p->obj, 0))) continue;
    rels_and_counts = ObjListIncrement(rels_and_counts, I(p->obj, 0));
  }
  *assertions_p = assertions;
  return(rels_and_counts);
}

/* Determine frequent relations involving a set of objects. */
ObjList *DictFrequentRels(Ts *ts, TsRange *tsr, ObjList *objs, int i,
                          /* RESULTS */ ObjList **assertions_p)
{
  int		numobjs;
  ObjList	*p, *rels_and_counts, *assertions, *r;
  rels_and_counts = assertions = NULL;
  /* Count occurrences of each relation. */
  for (p = objs; p; p = p->next) {
    rels_and_counts = DictRelationsAndCounts(ts, tsr, p->obj, i,
                                             rels_and_counts, assertions,
                                             &assertions);
  }
  numobjs = ObjListLen(objs);
  /* Construct result list of relations occurring more than 25% of the number
   * of objects.
   */
  r = NULL;
  for (p = rels_and_counts; p; p = p->next) {
    if (1) r = ObjListCreate(p->obj, r);
/*
    if ((ObjListN(p)*4) > numobjs) r = ObjListCreate(p->obj, r);
 */
  }
  ObjListFree(rels_and_counts);
  *assertions_p = assertions;
  return(r);
}

void DictPrintRelValues(Rpt *rpt, TsRange *tsr, ObjList *assertions, Obj *obj,
                        Obj *rel, int i, Discourse *dc,
                        /* RESULTS */ char *buf1)
{
  char		buf[PHRASELEN];
  ObjList	*p;
  buf1[0] = TERM;
  for (p = assertions; p; p = p->next) {
    if (tsr && !TsRangeEqual(tsr, ObjToTsRange(p->obj))) continue;
    if (obj && obj != I(p->obj, i)) continue;
    GenConceptString(I(p->obj, i == 2 ? 1 : 2), N("empty-article"), F_NOUN,
                     F_NULL, DC(dc).lang, F_NULL, F_NULL, F_NULL, PHRASELEN,
                     0, 1, dc, buf);
    if (buf[0]) {
      if (p->next) StringAppendDots(buf, " ", PHRASELEN);
      StringAppendDots(buf1, buf, PHRASELEN);
    }
  }
}

/* TableObjByValue: object   relation
 *                  object1  values1
 *                  object2  values2
 */
Rpt *DictTableObjByValue1(ObjList *assertions, ObjList *objs, Obj *obj,
                          Obj *rel, int i, Discourse *dc)
{
  char		buf[PHRASELEN], buf1[PHRASELEN];
  ObjList	*p;
  Rpt	*rpt;
  
  rpt = RptCreate();
  /* Values. */
  for (p = objs; p; p = p->next) {
    GenConceptString(p->obj, N("empty-article"), F_NOUN, F_NULL, DC(dc).lang,
                     F_NULL, F_NULL, F_NULL, PHRASELEN, 0, 1, dc, buf);
    if (buf[0]) {
      DictPrintRelValues(rpt, NULL, assertions, p->obj, rel, i, dc, buf1);
      if (buf1[0]) {
        RptStartLine(rpt);
        RptAdd(rpt, buf, RPT_JUST_LEFT);
        RptAdd(rpt, buf1, RPT_JUST_LEFT);
      }
    }
  }
  /* Headings. */
  RptStartLine(rpt);
  RptAddConcept(rpt, obj, dc);
  RptAddConcept(rpt, rel, dc);
  return(rpt);
}

void DictTableObjByValue(Text *text, Ts *ts, TsRange *tsr, Obj *obj,
                         ObjList *objs, Discourse *dc)
{
  int		i;
  ObjList	*rels, *assertions, *assertions1, *p;
  Rpt		*rpt;
  for (i = 1; i < 3; i++) {
    rels = DictFrequentRels(ts, tsr, objs, i, &assertions);
    for (p = rels; p; p = p->next) {
      assertions1 = ObjListCollectRels(assertions, p->obj);
      rpt = DictTableObjByValue1(assertions1, objs, obj, p->obj, i, dc);
      if (RptLines(rpt) > 1) {
        RptTextPrint(text, rpt, 1, -1, 1, 63);
      }
      RptFree(rpt);
      ObjListFree(assertions1);
    }
    ObjListFree(rels);
    ObjListFree(assertions);
  }
}

/* TableTsRangeByValue:     from        to   relation
 *                      19930501  19940602   values1
 *                      19940602  19940708   values2
 */
Rpt *DictTableTsRangeByValue1(ObjList *assertions, Obj *rel, int i,
                              Discourse *dc)
{
  char		buf[PHRASELEN];
  Rpt		*rpt;
  ObjList	*tsranges, *p;
  TsRange	tsr;
  rpt = RptCreate();

  /* Sort timestamp ranges. */
  tsranges = ObjListSortByTsRange(assertions);

  /* Values. */
  TsRangeSetNever(&tsr);
  for (p = tsranges; p; p = p->next) {
    if (TsRangeEqual(&tsr, ObjToTsRange(p->obj))) continue;
    tsr = *ObjToTsRange(p->obj);
    DictPrintRelValues(rpt, &tsr, assertions, NULL, rel, i, dc, buf);
    if (buf[0]) {
      RptStartLine(rpt);
      RptAddTs(rpt, &tsr.startts, dc);
      RptAddTs(rpt, &tsr.stopts, dc);
      RptAdd(rpt, buf, RPT_JUST_LEFT);
    }
  }

  /* Headings. */
  RptStartLine(rpt);
  RptAdd(rpt, "", RPT_JUST_LEFT);
  RptAdd(rpt, "", RPT_JUST_LEFT);
  RptAddConcept(rpt, rel, dc);

  ObjListFree(tsranges);
  return(rpt);
}

void DictTableTsRangeByValue(Text *text, TsRange *tsr, Obj *obj, Discourse *dc)
{
  int		i;
  ObjList	*rels, *assertions, *assertions1, *p;
  Rpt		*rpt;
  for (i = 1; i < 3; i++) {
    rels = DictRelationsAndCounts(NULL, tsr, obj, i, NULL, NULL, &assertions);
    for (p = rels; p; p = p->next) {
      assertions1 = ObjListCollectRels(assertions, p->obj);
      if (ObjListN(p) > 1) {
        rpt = DictTableTsRangeByValue1(assertions1, p->obj, i, dc);
        if (RptLines(rpt) > 1) {
          RptTextPrint(text, rpt, 1, -1, 1, 50);
        }
        RptFree(rpt);
      }
      ObjListFree(assertions1);
    }
    ObjListFree(rels);
    ObjListFree(assertions);
  }
}

void DictObjTables(Text *text, Obj *obj, Discourse *dc)
{
  ObjList	*children;
  GenAdvice	save_ga;

  save_ga = dc->ga;
  dc->ga.consistent = 1;

  TextNewlineIfNecessary(text);
  if ((children = ObjChildren(obj, OBJTYPEASYMBOL))) {
/*
  if ((children = ObjDescendants(obj, OBJTYPEASYMBOL))) {
 */
    DictTableObjByValue(text, &TsNA, NULL, obj, children, dc);
  }
  DictTableTsRangeByValue(text, &TsRangeAlways, obj, dc);

  dc->ga = save_ga;
}

/* Dict */

int DictLastLang;

Bool DictOle(int html, Obj *type, Text *text, ObjToLexEntry *ole, int post,
             int orig_pos, int pos, int lang, Discourse *dc)
{
  char	word[PHRASELEN], feat[FEATLEN], *word1, *feat1;
  Word	*infl;

  if (html) {
    if (type) {
      if (type == N("synonym-of")) {
        HTML_TextNewParagraph(text);
        HTML_TextPrint(text, "Synonyms:");
        HTML_TextBeginUnnumberedList(text);
      } else {
        HTML_TextEndUnnumberedList(text);
        HTML_TextNewParagraph(text);
        if (type == N("antonym-of")) {
          HTML_TextPrint(text, "Antonyms:");
        } else {
          HTML_TextPrint(text, "Other:");
        }
        HTML_TextBeginUnnumberedList(text);
      }
    }
  } else {
    if (type && type != N("synonym-of")) {
      TextNewlineIfNecessary(text);
      TextPrintConceptAbbrev(text, type, 1, ':', dc);
    }
  }
  if ((!html) && lang != DictLastLang) {
    TextPrintFeatAbbrev(text, lang, 1, TERM, dc);
    DictLastLang = lang;
  }
  if (html) {
    HTML_TextUnnumberedListElement(text);
    HTML_LEHrefString(ole->le, word);
    TextPuts(word, text);
  }
  if (StringIn(F_COMMON_INFL, ole->features)) {
    if ((infl = LexEntryGetInflection(ole->le, F_INFINITIVE, F_MASCULINE,
                                      FeatureGet(ole->features, FT_NUMBER),
                                      F_NULL, F_NULL, F_NULL, 1, dc))) {
      word1 = infl->word;
      feat1 = infl->features;
    } else {
      word1 = ole->le->srcphrase;
      feat1 = ole->le->features;
    }
  } else {
    word1 = ole->le->srcphrase;
    feat1 = ole->le->features;
  }
  StringCpy(feat, ole->features, FEATLEN);
  if (pos != orig_pos) {
    StringAppendChar(feat, FEATLEN, pos);
  }
  if (html) {
    LexEntryAddSep(ole->le, word1, PHRASELEN, word);
    TextPrintWordAndFeatures(text, html, lang, word, feat, ole->theta_roles,
                             1, TERM, FS_FREQUENT FT_NUMBER FS_CHECKING, dc);
#ifdef notdef
    word[0] = TERM;
    GenAppendNounGender(feat1, word);
    TextPuts(word, text);
#endif
    TextPrintf(text, "</a>\n");
  } else {
    LexEntryAddSep(ole->le, word1, PHRASELEN, word);
    GenAppendNounGender(feat1, word);
    TextPrintWordAndFeatures(text, html, lang, word, feat, ole->theta_roles,
                             1, post,
                             FS_FREQUENT FT_GENDER FT_NUMBER FS_CHECKING,
                             dc);
  }
  return(1);
}

Bool DictRelatedWords2(int html, Obj *type, Text *text, Obj *obj, int orig_pos,
                       int pos, int dialect, int freq, int lang,
                       LexEntry *le, Discourse *dc)
{
  int			i, cnt, found;
  ObjToLexEntry *ole;
  found = 0;
  for (ole = obj->ole, cnt = 0; ole; ole = ole->next) {
    if (ole->le == le) continue;
    if (pos != FeatureGet(ole->le->features, FT_POS)) continue;
    if (dialect != FeatureGet(ole->features, FT_DIALECT)) continue;
    if (freq != FeatureGet(ole->features, FT_FREQ)) continue;
    if (!StringIn(lang, ole->le->features)) continue;
    cnt++;
  }
  for (ole = obj->ole, i = 0; ole; ole = ole->next) {
    if (ole->le == le) continue;
    if (pos != FeatureGet(ole->le->features, FT_POS)) continue;
    if (dialect != FeatureGet(ole->features, FT_DIALECT)) continue;
    if (freq != FeatureGet(ole->features, FT_FREQ)) continue;
    if (!StringIn(lang, ole->le->features)) continue;
    i++;
    if (DictOle(html, type, text, ole, ((i == cnt) ? ';' : ','), orig_pos,
                pos, lang, dc)) {
      found = 1;
      type = NULL;
    }
  }
  return(found);
}

Bool DictRelatedWords1(int html, Obj *type, Text *text, Obj *obj, int orig_pos,
                       LexEntry *le, ABrainTask *abt, Discourse *dc)
{
  char	*pos, *dialect, *freq, *lang;
  char	pos1[FEATLEN];
  
  DictLastLang = F_NULL;
  if (orig_pos != F_NULL) {
    pos1[0] = orig_pos;
    pos1[1] = TERM;
  } else {
    pos1[0] = TERM;
  }
  StringAppendIfNotAlreadyIns(FT_POS, FEATLEN, pos1);
  for (pos = pos1; *pos; pos++) {
    for (dialect = dc->dialects; *dialect; dialect++) {
      for (freq = FT_FREQ_DICT; *freq; freq++) {
        for (lang = dc->langs; *lang; lang++) {
          if (BBrainStopsABrain(abt)) return(0);
          if (DictRelatedWords2(html, type, text, obj, orig_pos,
                                *(uc *)pos, *(uc *)dialect,
                                *(uc *)freq, *(uc *)lang,
                                le, dc)) {
            type = NULL;
          }
        }
      }
    }
  }
  return(1);
}

Bool DictRelatedWords(int html, Text *text, Obj *obj, int orig_pos,
                      LexEntry *le, ABrainTask *abt, Discourse *dc)
{
  if (!DictRelatedWords1(html, N("synonym-of"), text, obj, orig_pos,
                         le, abt, dc)) return(0);
  if (!DictRelatedWords1(html, N("antonym-of"), text, obj, orig_pos,
                         le, abt, dc)) {
    return(0);
  }
  TextNewlineIfNecessary(text);
  return(1);
}

/* todo: Convert generation into parsable and generatable reps? */
void DictChildren(Text *text, int objtype, Obj *type, Obj *parent, Obj *except,
                  int orig_pos, Discourse *dc)
{
  ObjList	*objs;
  PNode		*pn;
  if ((objs = ObjChildren(parent, objtype))) {
    if ((pn = GenNounPrepNoun(N("empty-article"), type, N("prep-of"), parent,
                              F_PLURAL, F_PLURAL, dc))) {
      DictObjList(text, pn, objs, except, N("definite-article"), orig_pos,
                  F_NULL, '.', 30, dc);
      ObjListFree(objs);
      PNodeFreeTree(pn);
    } else ObjListFree(objs);
  }
}

void DictContrastParent1(Text *text, Obj *obj, Obj *parent, Obj *grandparent,
                         int orig_pos, LexEntry *le, Discourse *dc)
{
  ObjList	*objs;
  PNode		*pn;
  TextPrintIsa(text, obj, grandparent, dc);
  if ((objs = ObjChildren(parent, OBJTYPEASYMBOL))) {
    if ((pn = GenInterventionObj(F_S, N("in-contrast"), dc))) {
      DictObjList(text, pn, objs, obj, N("definite-article"), orig_pos,
                  F_NULL, '.', 30, dc);
      ObjListFree(objs);
      PNodeFreeTree(pn);
    } else ObjListFree(objs);
  }
}

void DictContrastParent(Text *text, Obj *obj, Obj *parent, int orig_pos,
                        LexEntry *le, Discourse *dc)
{
  ObjList	*grandparents, *p;
  grandparents = ObjParents(parent, OBJTYPEANY);
  for (p = grandparents; p; p = p->next) {
    DictContrastParent1(text, obj, parent, p->obj, orig_pos, le, dc);
  }
  ObjListFree(grandparents);
}

void DictObjList(Text *text, PNode *header, ObjList *objs, Obj *except,
                 Obj *article, int pos, int paruniv, int post, int maxlen,
                 Discourse *dc)
{
  char		buf[PHRASELEN];
  int		i;
  ObjList	*p;
  StringArray	*sa;
  sa = StringArrayCreate();
  for (i = 0, p = objs; p && i < maxlen; i++, p = p->next) {
    if (p->obj == except) continue;
    GenConceptString(p->obj, article, pos, paruniv, DC(dc).lang, F_NULL, F_NULL,
                     F_NULL, PHRASELEN, 0, 0, dc, buf);
    if (buf[0]) StringArrayAddCopy(sa, buf, 1);
  }
  if (StringArrayLen(sa) > 0) {
    StringArraySort(sa);
    if (p) {
      GenConceptString(N("other-nonhuman"), NULL, F_PRONOUN, F_NULL,
                       DC(dc).lang, F_MASCULINE, F_PLURAL, F_NULL, PHRASELEN,
                       0, 0, dc, buf);
      if (buf[0]) StringArrayAddCopy(sa, buf, 1);
    }
    if (header) PNodeTextPrint(text, header, ':', 0, 1, dc);
    GenConceptString(N("and"), NULL, F_CONJUNCTION, F_NULL, DC(dc).lang,
                     F_NULL, F_NULL, F_NULL, PHRASELEN, 0, 0, dc, buf);
    StringArrayTextPrintList(text, sa, DiscourseSerialComma(dc), post, buf);
  }
  StringArrayFreeCopy(sa);
}

void DictAbstractParent(Text *text, Obj *obj, Obj *parent, int orig_pos,
                        LexEntry *le, int describe_children, Discourse *dc)
{
  if ((dc->mode & DC_MODE_TURING) && parent == N("human")) return;
  TextPrintIsa(text, obj, parent, dc);

  if ((dc->mode & DC_MODE_TURING) || parent == N("human")) return;
/* REDUNDANT: dc->mode & DC_MODE_TURING redundant in case parent == N("human")
 * removed.
 */
  if (describe_children) {
    /* "Types of " */
    DictChildren(text, OBJTYPEASYMBOL, N("idea-type"), obj, NULL, orig_pos, dc);
  }
  DictChildren(text, OBJTYPEASYMBOL,
               ObjIsConcrete(obj) ? N("idea-type") : N("other-type"),
               parent, obj, orig_pos, dc);
  if (describe_children) {
    DictChildren(text, OBJTYPECSYMBOL, N("instance"), obj, NULL, orig_pos, dc);
  }
  /* "Other instances of " */
  DictChildren(text, OBJTYPECSYMBOL,
               ObjIsConcrete(obj) ? N("other-instance") : N("instance"),
               parent, obj, orig_pos, dc);
}

void DictDbFile1(Text *text, Obj *obj, Obj *parent, int orig_pos, LexEntry *le,
                 int first, Discourse *dc)
{
  GenAdvice	save_ga;

  save_ga = dc->ga;
  dc->ga.consistent = 1;

  TextNextParagraph(text);
  if (ObjIsContrast(parent)) {
    DictContrastParent(text, obj, parent, orig_pos, le, dc);
  } else {
    DictAbstractParent(text, obj, parent, orig_pos, le, first, dc);
  }

  dc->ga = save_ga;
}

void DictDbFile(Text *text, Obj *obj, int orig_pos, LexEntry *le, Discourse *dc)
{
  int		first;
  ObjList	*parents, *p;
  parents = ObjParents(obj, OBJTYPEANY);
  first = 1;
  for (p = parents; p; p = p->next) {
    DictDbFile1(text, obj, p->obj, orig_pos, le, first, dc);
    first = 0;
  }
  ObjListFree(parents);
}

void DictAssocAssertion(Text *text, Obj *obj, Discourse *dc, int free_ok)
{
  ObjList	*assertions, *p, *objs;
  assertions = NULL;
  for (p = DCLISTENERS(dc); p; p = p->next) {
    objs = AssocAssertion(obj, p->obj, N("human"));
    assertions = ObjListAppendNonequal(assertions, objs);
    ObjListFree(objs);
  }
  if (assertions == NULL && free_ok) {
    assertions = AssocAssertion(obj, NULL, NULL);
  }
  if (assertions) {
    TextNextParagraph(text);
    TextGenParagraph(text, assertions, 0, 0, dc);
    TextNextParagraph(text);
  }
  ObjListFree(assertions);
}

void DictAssocObject(Text *text, Obj *obj, Discourse *dc)
{
  ObjList	*assertions;
  assertions = AssocObject(dc, obj, 76);
  if (assertions) {
    TextNextParagraph(text);
    TextGenParagraph(text, assertions, 0, 0, dc);
    TextNextParagraph(text);
  }
  ObjListFree(assertions);
}

void DictHuman(Text *text, Ts *ts, TsRange *tsr, Obj *human, Discourse *dc)
{
  ObjList	*assertions, *objs;
  if (!ISA(N("human"), human)) return;
  assertions = NULL;
  objs = REB(ts, tsr, L(N("occupation-of"), human, ObjWild, E));
  assertions = ObjListAppendNonequal(assertions, objs);
  ObjListFree(objs);
  objs = REB(ts, tsr, L(N("employee-of"), ObjWild, human, E));
  assertions = ObjListAppendNonequal(assertions, objs);
  ObjListFree(objs);
  if (assertions) {
    TextNextParagraph(text);
    TextGenParagraph(text, assertions, 0, 0, dc);
  }
}

void DictInvolving(Text *text, Obj *obj, Discourse *dc)
{
  GenAdvice	save_ga;
  ObjList	*assertions;
  assertions = DbRetrieveInvolving(&TsNA, NULL, obj, 0, NULL);
  if (assertions) {
    save_ga = dc->ga;
    dc->ga.gen_tsr = 1;
    TsrGenAdviceSet(&dc->ga.tga);
    TextNextParagraph(text);
    TextGenParagraph(text, assertions, 1, 0, dc);
    dc->ga = save_ga;
  }
}

void DictObj(Text *text, Obj *obj, int orig_pos, LexEntry *le, Discourse *dc)
{
  ABrainTask	*abt;
  if (dc->mode & DC_MODE_TURING) {
    DictDbFile(text, obj, orig_pos, le, dc);
    DictHuman(text, DCNOW(dc), NULL, obj, dc);
    DictAssocAssertion(text, obj, dc, 0);
  } else {
    abt = BBrainBegin(N("answer"), 10L, INTNA);
    DictRelatedWords(0, text, obj, orig_pos, le, abt, dc);
    BBrainEnd(abt);
    DictDbFile(text, obj, orig_pos, le, dc);
    DictHuman(text, DCNOW(dc), NULL, obj, dc);
    DictInvolving(text, obj, dc);
    DictObjTables(text, obj, dc);
    DictAssocAssertion(text, obj, dc, 1);
    DictAssocObject(text, obj, dc);
  }
}

void DictInfl(Text *text, Word *infl, Discourse *dc)
{
  for (; infl; infl = infl->next) {
    TextPrintFeaturesAbbrev(text, infl->features, 1, FT_POS FT_LANG, dc);
    TextPutc(SPACE, text);
    TextTabTo(20, text);
    TextPuts(infl->word, text);
    TextPutc(NEWLINE, text);
  }
}

ObjList *DictLexEntry(Text *text, LexEntry *le, int showinfl, ObjList *objs,
                      Discourse *dc)
{
  int			orig_pos, meaningnum;
  PNode			*pn;
  LexEntryToObj *leo;
  orig_pos = FeatureGet(le->features, FT_POS);
  if (showinfl) DictInfl(text, le->infl, dc);
  for (meaningnum = 1, leo = le->leo; leo; meaningnum++, leo = leo->next) {
    TextNewlineIfNecessary(text);
    TextPrintf(text, "  (%d)", meaningnum);
    if (dc->mode & DC_MODE_PROGRAMMER) {
      if (leo->obj) {
        TextPrintTTObject(text, leo->obj, dc);
        TextPrintTTObject(text, ObjGetAParent(leo->obj), dc);
      }
    }
    if (leo->features[0]) {
      TextPrintFeaturesAbbrev(text, leo->features, 1, "", dc);
    }
    if (leo->theta_roles) {
      ThetaRoleTextPrint(text, 0, le, leo->theta_roles, leo->obj, dc);
    }
    if (leo->obj == NULL) {
    } else if (!ObjListIn(leo->obj, objs)) {
      if (leo->obj) {
        DictObj(text, leo->obj, orig_pos, le, dc);
        objs = ObjListCreate(leo->obj, objs);
      }
    } else {
      /* todo: Should reference definition letter-number. */
      pn = GenInterventionObj(F_S, N("see-above"), dc);
      PNodeTextPrint(text, pn, '.', 0, 0, dc);
      PNodeFreeTree(pn);
    }
  }
  return(objs);
}

ObjList *DictPhrase2(Text *text, int pos, char *phrase, HashTable *ht,
                     int showinfl, ObjList *objs, char *lenum, Discourse *dc)
{
  IndexEntry	*ie, *origie, *ie1;
  LexEntry		*le;
  int			duple, freeme;
  for (ie = origie = LexEntryFindPhrase(ht, phrase, INTPOSINF, 0, 0, &freeme);
       ie; ie = ie->next) {
    if (pos != FeatureGet(ie->lexentry->features, FT_POS)) continue;
    le = ie->lexentry;
    duple = 0;
    for (ie1 = origie; ie1 != ie; ie1 = ie1->next) {
      if (ie1->lexentry == le) {
        duple = 1;
        break;
      }
    }
    TextNewlineIfNecessary(text);
    if (duple) {
      TextPrintf(text, "(%c')", (*lenum)-1);
    } else {
      TextPrintf(text, "(%c) ", *lenum);
    }
    TextPrintFeaturesAbbrev(text, ie->features, 1, FS_CHECKING, dc);
    if (!streq(phrase, le->srcphrase)) {
      TextPrintf(text, " %s", le->srcphrase);
    }
    TextPutc(NEWLINE, text);
    if (!duple) {
      objs = DictLexEntry(text, le, showinfl, objs, dc);
      (*lenum)++;
    }
  }
  if (freeme) IndexEntryFree(origie);
  return(objs);
}

ObjList *DictPhrase1(Text *text, char *phrase, HashTable *ht, int showinfl,
                     ObjList *objs, char *lenum, Discourse *dc)
{
  char	*pos;
  for (pos = FT_POS; *pos; pos++) {
    objs = DictPhrase2(text, *(uc *)pos, phrase, ht, showinfl, objs, lenum, dc);
  }
  return(objs);
}

void DictPhrase(Text *text, char *phrase, int showinfl, Discourse *dc)
{
  ObjList		*objs;
  char			lenum, title[PHRASELEN];
  StringToUpper(phrase, PHRASELEN, title);
  TextPuts("*** ", text);
  TextPuts(title, text);
  TextPutc(NEWLINE, text);
  lenum = 'A';
  objs = NULL;
  objs = DictPhrase1(text, phrase, FrenchIndex, showinfl, objs, &lenum, dc);
  objs = DictPhrase1(text, phrase, EnglishIndex, showinfl, objs, &lenum, dc);
  TextNewlineIfNecessary(text);
  if (StringIsPalindrome(phrase)) {
    TextPrintIsa(text, StringToObj(phrase, N("palindrome"), 0),
                 N("palindrome"), dc);
  }
  ReportAnagramsOf(text, phrase, dc);
  TextNewlineIfNecessary(text);
  ObjListFree(objs);
}

/* TESTING */

/* todo: Words inside phrases. */
void Dictionary()
{
  char		phrase[PHRASELEN];
  DiscourseInit(StdDiscourse, NULL, N("computer-file"), N("Jim"), Me, 0);
  printf("[Dictionary]\n");
  while (1) {
    if (!StreamAskStd("Enter word or phrase (@=inflect): ", PHRASELEN,
                      phrase)) {
      return;
    }
    if (phrase[0] == '@') Dictionary1(phrase+1, 1, StdDiscourse);
    else Dictionary1(phrase, 0, StdDiscourse);
  }
}

void Dictionary1(char *phrase, int showinfl, Discourse *dc)
{
  int	i, save_channel;
  Text	*text;
  save_channel = DiscourseGetCurrentChannel(dc);
  for (i = DCOUT1; i < DCMAX; i++) {
    if (NULL == DiscourseSetCurrentChannelRW(dc, i)) continue;
    text = TextCreat(dc);
    DictPhrase(text, phrase, showinfl, dc);
    TextPrint(DC(dc).stream, text);
    TextFree(text);
  }
  DiscourseSetCurrentChannel(dc, save_channel);
}

/* End of file. */
