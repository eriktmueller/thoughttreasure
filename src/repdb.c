/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940419: begun
 * 19940705: incorporated timestamps into objects
 * 19980701: fix to DbRestrictionParse1 causing SEGVs
 */

#include "tt.h"
#include "lexobjle.h"
#include "repbasic.h"
#include "repcxt.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repprove.h"
#include "reptime.h"
#include "semanaph.h"
#include "semdisc.h"
#include "synpnode.h"
#include "utildbg.h"

HashTable *DbHT01, *DbHT02, *DbHT0, *DbHT1, *DbHT2;

long DbAssertionCnt;

void DbInit()
{
  Dbg(DBGDB, DBGHYPER, "DbInit", E);
  DbAssertionCnt = 0;
  DbHT01 = HashTableCreate(4099L);
  DbHT02 = HashTableCreate(4099L);
  DbHT0 = HashTableCreate(4099L);
  DbHT1 = HashTableCreate(4099L);
  DbHT2 = HashTableCreate(4099L);
}

void DbHashSym(char *s1, char *s2, /* RESULT */ char *r)
{
  int	i;
  if (s2[0] == TERM) {
    for (i = 0; *s1 && i < 6; i++, r++, s1++) *r = *s1;
  } else {
    for (i = 0; *s1 && i < 3; i++, r++, s1++) *r = *s1;
    for (i = 0; *s2 && i < 3; i++, r++, s2++) *r = *s2;
  }
  *r = TERM;
}

void DbHashEnter(HashTable *ht, Obj *objs, Obj *elema, Obj *elemb)
{
  char		sym[DHASHSIG];
  ObjList	*prev, *new;
  DbHashSym(M(elema), M(elemb), sym);
  prev = (ObjList *)HashTableGet(ht, sym);
  new = ObjListCreateShort(objs, prev);
  HashTableSetDup(ht, sym, new);
}

ObjList *DbHashRetrieve(HashTable *ht, Obj *elema, Obj *elemb)
{
  char	sym[DHASHSIG];
  DbHashSym(M(elema), M(elemb), sym);
  return((ObjList *)HashTableGet(ht, sym));
}

/******************************************************************************
 * ASSERTION
 ******************************************************************************/

Bool DbGenIsPruned(Obj *obj)
{
  return(ISA(N("script-relation"), I(obj, 0)));
}

void DbAssert1(Obj *obj)
{
  TsRange	*tsr;
  Discourse	*dc;
  if (obj->type != OBJTYPELIST) {
    Dbg(DBGDB, DBGBAD, "DbAssert: asserting nonlist <%s>", M(obj));
    return;
  }
  if (obj->u1.lst.asserted) {
    return;
  }
  if ((!Starting) || (!SaveTime)) {
    DbRestrictValidate(obj, 1);
  }
  DbAssertionCnt++;
  DbHashEnter(DbHT01, obj, I(obj, 0), I(obj, 1));
  DbHashEnter(DbHT02, obj, I(obj, 0), I(obj, 2));
  DbHashEnter(DbHT0, obj, I(obj, 0), NULL);
  DbHashEnter(DbHT1, obj, I(obj, 1), NULL);
  DbHashEnter(DbHT2, obj, I(obj, 2), NULL);
  obj->u1.lst.asserted = 1;
  if (DbgOn(DBGDB, DBGDETAIL)) {
    fputs("****ASSERTED ", Log);
    ObjPrint1(Log, obj, NULL, 5, 1, 0, 1, 0);
    fputc(NEWLINE, Log);
  }
  if (!Starting) {
    /* todo: Max depth. Retraction maintenance. */
    tsr = ObjToTsRange(obj);
#ifdef notdef
    if (tsr->cx) {
      tsr->cx->assertions = ObjListCreate(obj, tsr->cx->assertions);
    }
#endif
#ifdef notdef
    Ts		*ts;
    ts = tsr->startts;
    /* This is looping. */
    inferred = InferenceRunInferenceRules(ts, NULL, obj, NULL, 1);
    /* todo: This is looping on "Jim likes rain."
    for (p = inferred; p; p = p->next) {
      if (YES(ZRE(ts, p->obj))) {
        Dbg(DBGDB, DBGHYPER, "already true:");
        DbgOP(DBGDB, DBGHYPER, p->obj);
      } else {
        DbAssert1(p->obj);
      }
    }
     */
    ObjListFree(inferred);
#endif
#ifdef notdef
    /* todo */
    inferred = InferenceRunActivationRules(ts, NULL, obj, NULL, 1);
    for (p = inferred; p; p = p->next) {
      /* Activate subgoal p->obj. */
    }
    ObjListFree(inferred);
#endif
  }
  /* todo: Assertions should only be generated if the context is accepted? */
  if (ContextCurrentDc) dc = ContextCurrentDc;
  else dc = StdDiscourse;
  if ((!Starting) && GenOnAssert) {
    if ((dc->mode & DC_MODE_THOUGHTSTREAM) &&
        (!DbGenIsPruned(obj))) {
      DiscourseGen(dc,
                   0, 
                   (dc && dc->cx_best &&
                    (dc->cx_best->mode == MODE_PERFORMANCE) &&
                    ISA(N("action"), I(obj, 0))) ? "" : "& ",
                   obj,
                   EOSNA);
    }
    ContextOnAssert(tsr, obj);
  }
}

/* Steps on <obj> tsr. */
void DbAssert(TsRange *tsr, Obj *obj)
{
  if (ObjContainsTsRange(obj)) obj->u2.tsr = *tsr;
  DbAssert1(obj);
}

void DbAssertActionDur(Ts *ts, Dur dur, Obj *obj)
{
  TsRange tsr;
  TsMakeRangeAction(&tsr, ts, dur);
  DbAssert(&tsr, obj);
}

void DbAssertActionRange(Ts *startts, Ts *stopts, Obj *obj)
{
  TsRange tsr;
  TsMakeRange(&tsr, startts, stopts);
  DbAssert(&tsr, obj);
}

void DbAssertState(Ts *ts, Dur dur, Obj *obj)
{
  TsRange tsr;
  TsMakeRangeState(&tsr, ts, dur);
  DbAssert(&tsr, obj);
}

/******************************************************************************
 * RETRIEVAL
 ******************************************************************************/

ObjList *DbRetrieval(Ts *ts, TsRange *tsr, Obj *ptn, ObjList *r, Ts *tsretract,
                     Bool freeptn)
{
  Obj		*elem0, *elem1, *elem2, *obj;
  ObjList	*fl, *f;
  Context       *cx;
  TsRange       *objtsr;
  if (ptn->type != OBJTYPELIST) {
    Dbg(DBGDB, DBGBAD, "DbRetrieval: nonlist");
    return(NULL);
  }
  cx = (tsr ? tsr->cx : ts->cx);
  elem0 = I(ptn, 0);
  elem1 = I(ptn, 1);
  elem2 = I(ptn, 2);
  if (ObjIsNotVar(elem0) && ObjIsNotVarNC(elem1)) {
    fl = DbHashRetrieve(DbHT01, elem0, elem1);
  } else if (ObjIsNotVar(elem0) && ObjIsNotVarNC(elem2)) {
    fl = DbHashRetrieve(DbHT02, elem0, elem2);
  } else if (ObjIsNotVar(elem0)) {
    fl = DbHashRetrieve(DbHT0, elem0, NULL);
  } else if (ObjIsNotVarNC(elem1)) {
    fl = DbHashRetrieve(DbHT1, elem1, NULL);
  } else if (ObjIsNotVarNC(elem2)) {
    fl = DbHashRetrieve(DbHT2, elem2, NULL);
  } else {
    Dbg(DBGDB, DBGBAD, "no retrieval hash");
    if (freeptn) ObjFree(ptn);
    return(r);
  }
  for (f = fl; f; f = f->next) {
    if (!ContextIsAncestor(f->obj->u2.tsr.cx, cx)) continue;
    if (ObjSupersededIn(f->obj, cx)) continue;
    if ((tsr ? TsRangeOverlaps(tsr, &f->obj->u2.tsr) :
               TsRangeMatch(ts, &f->obj->u2.tsr)) &&
        ObjUnifyQuick(ptn, f->obj)) {
      Dbg(DBGDB, DBGHYPER, "found:");
      DbgOP(DBGDB, DBGHYPER, f->obj);
      if (ptn == f->obj) freeptn = 0;
      if (tsretract) {
        if (f->obj->u2.tsr.cx == cx) {
        /* Retract in place. */
          f->obj->u2.tsr.stopts = *tsretract;
          if (DbgOn(DBGDB, DBGDETAIL)) {
            fputs("****RETRACTED ", Log);
            ObjPrint1(Log, f->obj, NULL, 5, 1, 0, 1, 0);
            fputc(NEWLINE, Log);
          }
        } else {
        /* Retract by copying. */
          obj = ObjCopyList(f->obj);
          objtsr = ObjToTsRange(obj);
          TsRangeSetContext(objtsr, cx);
          f->obj->u1.lst.superseded_by =
            ObjListCreate(obj, f->obj->u1.lst.superseded_by);
          if (DbgOn(DBGDB, DBGDETAIL)) {
            fputs("****RETRACTED BY COPYING ", Log);
            ObjPrint1(Log, f->obj, NULL, 5, 1, 0, 1, 0);
            fputc(NEWLINE, Log);
          }
          obj->u2.tsr.stopts = *tsretract;
          DbAssert1(obj);
        }
      }
      r = ObjListCreate(f->obj, r);
    }
  }
  if (freeptn) ObjFree(ptn);
  return(r);
}

ObjList *DbRetrievalDesc(Ts *ts, TsRange *tsr, Obj *ptn, int elemi, ObjList *r,
                         Ts *tsretract, int lockout, int depth, Bool freeptn)
{
  int		i;
  Obj		*pivot, *ptn1;
  ObjList	*orig_r;
  orig_r = r;
  r = DbRetrieval(ts, tsr, ptn, r, tsretract, 0);
  /* Lock out other possibilities: correct for properties. */
  if (lockout && (r != orig_r)) goto done;
  if (depth <= 0) goto done;
  pivot = I(ptn, elemi);
  if (pivot == NULL) {
    Dbg(DBGDB, DBGBAD, "DbRetrievalAnc: empty pivot");
    ObjPrettyPrint(Log, ptn);
    Stop();
    goto done;
  }
  if (pivot->type == OBJTYPELIST) goto done;
  ptn1 = ObjCopyList(ptn);
  for (i = 0; i < pivot->u1.nlst.numchildren; i++) {
    if (ObjIsVar(pivot->u1.nlst.children[i])) continue;
    ObjSetIth(ptn1, elemi, pivot->u1.nlst.children[i]);
    r = DbRetrievalDesc(ts, tsr, ptn1, elemi, r, tsretract, lockout,
                        depth-1, 0);
  }
  ObjFree(ptn1);
done:
  if (freeptn) ObjFree(ptn);
  return(r);
}

ObjList *DbRetrievalAnc(Ts *ts, TsRange *tsr, Obj *ptn, int elemi, ObjList *r,
                        Ts *tsretract, int lockout, int depth, Bool freeptn)
{
  int		i;
  Obj		*pivot, *ptn1;
  ObjList	*orig_r;
  orig_r = r;
  r = DbRetrieval(ts, tsr, ptn, r, tsretract, 0);
  /* Lock out other possibilities: correct for properties. */
  if (lockout && (r != orig_r)) goto done;
  if (depth <= 0) goto done;
  pivot = I(ptn, elemi);
  if (pivot == NULL) {
    Dbg(DBGDB, DBGBAD, "DbRetrievalAnc: empty pivot");
    ObjPrettyPrint(Log, ptn);
    Stop();
    goto done;
  }
  if (pivot->type == OBJTYPELIST) goto done;
  ptn1 = ObjCopyList(ptn);
  for (i = 0; i < pivot->u1.nlst.numparents; i++) {
    if (ObjIsVar(pivot->u1.nlst.parents[i])) continue;
    ObjSetIth(ptn1, elemi, pivot->u1.nlst.parents[i]);
    r = DbRetrievalAnc(ts, tsr, ptn1, elemi, r, tsretract, lockout, depth-1, 0);
  }
  ObjFree(ptn1);
done:
  if (freeptn) ObjFree(ptn);
  return(r);
}

ObjList *DbRetrievalAncDesc(Ts *ts, TsRange *tsr, Obj *ptn, int anci, int desci,
                            ObjList *r, Ts *tsretract, int ancdepth,
                            int descdepth, Bool freeptn)
{
  int		i;
  Obj		*pivot, *ptn1;
  r = DbRetrievalDesc(ts, tsr, ptn, desci, r, tsretract, 1, descdepth, 0);
  if (ancdepth <= 0) goto done;
  pivot = I(ptn, anci);
  if (pivot == NULL) {
    Dbg(DBGDB, DBGBAD, "DbRetrievalAnc: empty pivot");
    ObjPrettyPrint(Log, ptn);
    Stop();
    goto done;
  }
  if (pivot->type == OBJTYPELIST) goto done;
  ptn1 = ObjCopyList(ptn);
  for (i = 0; i < pivot->u1.nlst.numparents; i++) {
    if (ObjIsVar(pivot->u1.nlst.parents[i])) continue;
    ObjSetIth(ptn1, anci, pivot->u1.nlst.parents[i]);
    r = DbRetrievalAncDesc(ts, tsr, ptn1, anci, desci, r, tsretract, ancdepth-1,
                           descdepth, 0);
  }
  ObjFree(ptn1);
done:
  if (freeptn) ObjFree(ptn);
  return(r);
}

ObjList *DbRetrieveExactIth(int i, Ts *ts, TsRange *tsr, Obj *ptn, Bool freeptn)
{
  ObjList	*objs, *p, *r;
  r = NULL;
  objs = DbRetrieval(ts, tsr, ptn, NULL, NULL, freeptn);
  for (p = objs; p; p = p->next) {
    r = ObjListCreate(ObjIth(p->obj, i), r);
  }
  ObjListFree(objs);
  return(r);
}

ObjList *DbRetrieveDescIth(int i, Ts *ts, TsRange *tsr, Obj *ptn, int j,
                           Bool freeptn)
{
  ObjList	*objs, *p, *r;
  r = NULL;
  objs = DbRetrieveDesc(ts, tsr, ptn, j, 1, freeptn);
  for (p = objs; p; p = p->next) {
    r = ObjListCreate(ObjIth(p->obj, i), r);
  }
  ObjListFree(objs);
  return(r);
}

/******************************************************************************
 * RETRACTION
 ******************************************************************************/

void DbRetractList(Ts *ts, ObjList *objs)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    TE(ts, p->obj);
  }
}

/******************************************************************************
 * ENUM RETRIEVAL
 ******************************************************************************/

Obj *DbGetEnumValue(Ts *ts, TsRange *tsr, Obj *prop, Obj *obj, Obj *def)
{
  Obj		*r;
  ObjList	*objs;
  if ((objs = DbRetrievalAncDesc(ts, tsr, L(prop, obj, E), 1, 0,
                                 NULL, NULL, 5, 3, 1))) {
    r = ObjIth(objs->obj, 0);
    ObjListFree(objs);
    return(r);
  } else {
    if (def) {
      Dbg(DBGDB, DBGBAD, "%s of %s unknown; assumed %s", M(prop), M(obj),
          M(def));
    }
    return(def);
  }
}

Obj *DbGetEnumAssertion(Ts *ts, Obj *prop, Obj *obj)
{
  return(ObjListRandomElemFree(DbRetrievalAncDesc(ts, NULL, L(prop, obj, E),
                                                  1, 0, NULL, NULL, 5, 3, 1)));
}

Bool DbIsEnumValue(Ts *ts, Obj *prop, Obj *obj)
{
  return(NULL != RN(ts, L(prop, obj, E), 1));
}

/******************************************************************************
 * ATTRIBUTE RETRIEVAL
 ******************************************************************************/

Obj *DbGetAttributeAssertion(Ts *ts, TsRange *tsr, Obj *attr, Obj *obj)
{
  return(ObjListRandomElemFree(DbRetrievalAnc(ts, tsr, L(attr, obj, E),
                                              1, NULL, NULL, 1, 5, 1)));
}

Float DbGetAttributeValue(Ts *ts, TsRange *tsr, Obj *attr, Obj *obj)
{
  Float		numer, denom;
  ObjList	*objs, *p;
  numer = 0.0;
  denom = 0.0;
  objs = DbRetrievalAnc(ts, tsr, L(attr, obj, E), 1, NULL, NULL, 1, 5, 1);
  for (p = objs; p; p = p->next) {
    numer += ObjWeight(p->obj);
    denom += 1.0;
  }
  ObjListFree(objs);
  if (denom == 0.0) return(FLOATNA);
  return(numer/denom);
}

/* todo: Inheritance CUT to reduce property value search? */
ObjList *DbGetRelationAssertions(Ts *ts, TsRange *tsr, Obj *prop, Obj *obj,
                                 int i, ObjList *r)
{
#ifdef notdef
  Needed for retrieving a class of relations. But this is expensive.
      DbRetrievalAncDesc(ts, NULL, L(prop, obj, ObjWild, E), 1, 0, NULL, NULL,
                         5, 2)
#endif
  if (obj == NULL) return(r);
  return(DbRetrievalAnc(ts, tsr,
                        (i == 1) ? L(prop, obj, ObjWild, E) :
                                   L(prop, ObjWild, obj, E),
                        i, r, NULL, 1, 5, 1));
}

/******************************************************************************
 * RELATION RETRIEVAL
 ******************************************************************************/

ObjList *DbGetRelationValues(Ts *ts, TsRange *tsr, Obj *prop, Obj *obj,
                             int i, ObjList *r)
{
  ObjList	*objs, *p;
  objs = DbGetRelationAssertions(ts, tsr, prop, obj, i, r);
  for (p = objs; p; p = p->next) {
    r = ObjListCreate(I(p->obj, FLIP12(i)), r);
  }
  ObjListFree(objs);
  return(r);
}

/* Given prop/rel/arg0, arg1 returns arg2. */
Obj *DbGetRelationValue(Ts *ts, TsRange *tsr, Obj *prop, Obj *obj, Obj *def)
{
  Obj		*r;
  ObjList	*objs;
  if (obj == NULL) return(def);
  if ((objs = DbGetRelationAssertions(ts, tsr, prop, obj, 1, NULL))) {
    r = ObjIth(objs->obj, 2);
    ObjListFree(objs);
    return(r);
  } else {
    if (def) {
      Dbg(DBGDB, DBGBAD, "%s of %s unknown; assumed %s", M(prop), M(obj),
          M(def));
    }
    return(def);
  }
}

/* Given prop/rel/arg0, arg2 returns arg1. */
Obj *DbGetRelationValue1(Ts *ts, TsRange *tsr, Obj *prop, Obj *obj, Obj *def)
{
  Obj		*r;
  ObjList	*objs;
  if (obj == NULL) return(def);
  if ((objs = DbGetRelationAssertions(ts, tsr, prop, obj, 2, NULL))) {
    r = ObjIth(objs->obj, 1);
    ObjListFree(objs);
    return(r);
  } else {
    if (def) {
      Dbg(DBGDB, DBGBAD, "%s of %s unknown; assumed %s", M(prop), M(obj),
          M(def));
    }
    return(def);
  }
}

/******************************************************************************
 * WHOLE-PART RETRIEVAL
 ******************************************************************************/

Bool DbIsPartOf1(Ts *ts, TsRange *tsr, Obj *part, Obj *whole, int depth,
                 Obj *rel)
{
  ObjList *objs, *p;
  if (depth >= MAXPARTDEPTH) return(0);
  if (!(objs = REB(ts, tsr, L(rel, ObjWild, whole, E)))) {
    return(0);
  }
  for (p = objs; p; p = p->next) {
    if (part == I(p->obj, 1)) {
      ObjListFree(objs);
      return(1);
    } else if (DbIsPartOf1(ts, tsr, part, I(p->obj, 1), depth+1, rel)) {
      ObjListFree(objs);
      return(1);
    }
  }
  ObjListFree(objs);
  return(0);
}

/* For concrete objects only. */
Bool DbIsPartOf(Ts *ts, TsRange *tsr, Obj *part, Obj *whole)
{
  return(DbIsPartOf1(ts, tsr, part, whole, 0, N("cpart-of")));
}

/* For abstract objects only. */
Bool DbIsPartOf_Simple(Obj *part, Obj *whole, int depth)
{
  ObjList *objs, *p;
  if (depth >= MAXPARTDEPTH) return(0);
  if (!(objs = RN(&TsNA, L(N("part-of"), ObjWild, whole, E), 2))) {
    return(0);
  }
  for (p = objs; p; p = p->next) {
    if (part == I(p->obj, 1)) {
      ObjListFree(objs);
      return(1);
    } else if (DbIsPartOf_Simple(part, I(p->obj, 1), depth+1)) {
      ObjListFree(objs);
      return(1);
    }
  }
  ObjListFree(objs);
  return(0);
}

Obj *DbRetrievePart1(Ts *ts, TsRange *tsr, Obj *part, Obj *whole, int depth)
{
  Obj		*obj, *r;
  ObjList	*objs, *p;
  if (depth <= 0) return(NULL);
  objs = RNB(ts, tsr, L(N("part-of"), ObjWild, whole, E), 2);
  for (p = objs; p; p = p->next) {
    if (!(obj = R1DBI(1, ts, tsr, L(N("cpart-of"), I(p->obj, 1), whole, E),
                      1))) {
      obj = ObjCreateInstance(I(p->obj, 1), M(whole));
      DbAssert(&TsRangeAlways, L(N("cpart-of"), obj, whole, E));
      /* todo: cpart-of changing over time. */
    }
    if (ISA(part, obj)) {
      ObjListFree(objs);
      return(obj);
    }
    if ((r = DbRetrievePart1(ts, tsr, part, obj, depth-1))) {
      ObjListFree(objs);
      return(r);
    }
  }
  ObjListFree(objs);
  return(NULL);
}

/* <part> is abstract, <whole> concrete; returns concrete part.
 * Breadth-first search.
 */
Obj *DbRetrievePart(Ts *ts, TsRange *tsr, Obj *part, Obj *whole)
{
  int i;
  Obj *r;
  if (ObjIsList(whole)) return(NULL);
  for (i = 1; i < MAXPARTDEPTH; i++) {
    if ((r = DbRetrievePart1(ts, tsr, part, whole, i))) {
      return(r);
    }
  }
  Dbg(DBGDB, DBGDETAIL, "no part <%s> of <%s> found", M(part), M(whole), E);
  return(NULL);
}

/* <whole> is abstract, <part> concrete; returns concrete whole;
 * Only finds existing wholes (because a knob could be part
 * of a TV or a radio).
 */
Obj *DbRetrieveWhole(Ts *ts, TsRange *tsr, Obj *whole, Obj *part)
{
  Obj		*r;
  ObjList	*objs, *p;
  objs = REB(ts, tsr, L(N("cpart-of"), part, ObjWild, E));
  for (p = objs; p; p = p->next) {
    if (ISA(whole, I(p->obj, 2))) {
      r = I(p->obj, 2);
      ObjListFree(objs);
      return(r);
    } else if ((r = DbRetrieveWhole(ts, tsr, whole, I(p->obj, 2)))) {
      ObjListFree(objs);
      return(r);
    }
  }
  ObjListFree(objs);
  return(NULL);
}

/******************************************************************************
 * INVOLVING RETRIEVAL
 ******************************************************************************/

ObjList *DbRetrieveInvolving(Ts *ts, TsRange *tsr, Obj *obj, int pred_ok,
                             ObjList *r)
{
  if (pred_ok) r = DbRetrieval(ts, tsr, L(obj, E), r, NULL, 1);
  r = DbRetrieval(ts, tsr, L(ObjWild, obj, E), r, NULL, 1);
  r = DbRetrieval(ts, tsr, L(ObjWild, ObjWild, obj, E), r, NULL, 1);
  return(r);
}

ObjList *DbRetrieveInvolving2(Ts *ts, TsRange *tsr, Obj *obj1, Obj *obj2,
                              ObjList *r)
{
  r = DbRetrieval(ts, tsr, L(obj1, obj2, ObjWild, E), r, NULL, 1);
  r = DbRetrieval(ts, tsr, L(obj2, obj1, ObjWild, E), r, NULL, 1);
  r = DbRetrieval(ts, tsr, L(obj1, ObjWild, obj2, E), r, NULL, 1);
  r = DbRetrieval(ts, tsr, L(obj2, ObjWild, obj1, E), r, NULL, 1);
  r = DbRetrieval(ts, tsr, L(ObjWild, obj1, obj2, E), r, NULL, 1);
  r = DbRetrieval(ts, tsr, L(ObjWild, obj2, obj1, E), r, NULL, 1);
  return(r);
}

ObjList *DbRetrieveInvolving2AncDesc(Ts *ts, TsRange *tsr, Obj *obj1, Obj *obj2,
                                     ObjList *rest)
{
  ObjList	*r;
  r = NULL;

  /* todo: There has got to be a better way. */
  r = DbRetrievalAnc(ts, tsr, L(obj1, obj2, E), 0, r, NULL, 1, 5, 1);
  r = DbRetrievalAnc(ts, tsr, L(obj2, obj1, E), 0, r, NULL, 1, 5, 1);
  r = DbRetrievalAnc(ts, tsr, L(obj1, ObjWild, obj2, E), 0, r, NULL, 1, 5, 1);
  r = DbRetrievalAnc(ts, tsr, L(obj2, ObjWild, obj1, E), 0, r, NULL, 1, 5, 1);
  r = DbRetrievalAnc(ts, tsr, L(ObjWild, obj1, obj2, E), 1, r, NULL, 1, 5, 1);
  r = DbRetrievalAnc(ts, tsr, L(ObjWild, obj2, obj1, E), 1, r, NULL, 1, 5, 1);

  r = DbRetrievalAnc(ts, tsr, L(obj1, obj2, E), 1, r, NULL, 1, 5, 1);
  r = DbRetrievalAnc(ts, tsr, L(obj2, obj1, E), 1, r, NULL, 1, 5, 1);
  r = DbRetrievalAnc(ts, tsr, L(obj1, ObjWild, obj2, E), 2, r, NULL, 1, 5, 1);
  r = DbRetrievalAnc(ts, tsr, L(obj2, ObjWild, obj1, E), 2, r, NULL, 1, 5, 1);
  r = DbRetrievalAnc(ts, tsr, L(ObjWild, obj1, obj2, E), 2, r, NULL, 1, 5, 1);
  r = DbRetrievalAnc(ts, tsr, L(ObjWild, obj2, obj1, E), 2, r, NULL, 1, 5, 1);

  r = DbRetrievalDesc(ts, tsr, L(obj1, obj2, E), 0, r, NULL, 1, 5, 1);
  r = DbRetrievalDesc(ts, tsr, L(obj2, obj1, E), 0, r, NULL, 1, 5, 1);
  r = DbRetrievalDesc(ts, tsr, L(obj1, ObjWild, obj2, E), 0, r, NULL, 1, 5, 1);
  r = DbRetrievalDesc(ts, tsr, L(obj2, ObjWild, obj1, E), 0, r, NULL, 1, 5, 1);
  r = DbRetrievalDesc(ts, tsr, L(ObjWild, obj1, obj2, E), 1, r, NULL, 1, 5, 1);
  r = DbRetrievalDesc(ts, tsr, L(ObjWild, obj2, obj1, E), 1, r, NULL, 1, 5, 1);

  r = DbRetrievalDesc(ts, tsr, L(obj1, obj2, E), 1, r, NULL, 1, 5, 1);
  r = DbRetrievalDesc(ts, tsr, L(obj2, obj1, E), 1, r, NULL, 1, 5, 1);
  r = DbRetrievalDesc(ts, tsr, L(obj1, ObjWild, obj2, E), 2, r, NULL, 1, 5, 1);
  r = DbRetrievalDesc(ts, tsr, L(obj2, ObjWild, obj1, E), 2, r, NULL, 1, 5, 1);
  r = DbRetrievalDesc(ts, tsr, L(ObjWild, obj1, obj2, E), 2, r, NULL, 1, 5, 1);
  r = DbRetrievalDesc(ts, tsr, L(ObjWild, obj2, obj1, E), 2, r, NULL, 1, 5, 1);

  r = ObjListUniquify(r);

  return(ObjListAppendDestructive(rest, r));
}

/******************************************************************************
 * INTENSION RESOLUTION
 ******************************************************************************/

Intension *IntensionCreate()
{
  Intension	*itn;
  itn = CREATE(Intension);
  IntensionInit(itn);
  return(itn);
}

void IntensionInit(Intension *itn)
{
  itn->ts = &TsNA;
  itn->tsr = NULL;
  itn->class = NULL;
  itn->props = NULL;
  itn->isas = NULL;
  itn->skip_this_isa = NULL;
  itn->attributes = NULL;
  itn->ts_created = NULL;
  itn->cx = NULL;
  itn->cx_only = 0;
}

void IntensionPrint(FILE *stream, Intension *itn)
{
  fprintf(stream, "Intension <%s>", M(itn->class));
  if (itn->ts) {
    TsPrint(stream, itn->ts);
    fputc(SPACE, stream);
  }
  if (itn->tsr) {
    TsRangePrint(stream, itn->tsr);
    fputc(SPACE, stream);
  }
  fputc(NEWLINE, stream);
  if (itn->props) {
    fputs("props:\n", stream);
    ObjListPrint(stream, itn->props);
  }
  if (itn->isas) {
    fputs("isas:\n", stream);
    ObjListListPrint(stream, itn->isas, 0, 0, 0);
  }
  if (itn->attributes) {
    fputs("attributes:\n", stream);
    ObjListListPrint(stream, itn->attributes, 0, 0, 0);
  }
  if (itn->ts_created) {
    fputs("ts_created ", stream);
    TsPrint(stream, itn->ts_created);
    fputc(NEWLINE, stream);
  }
}

/* concrete: ball23
 * class:    ball
 * props:    [blue ball]
 *
 * concrete: lastname-Garnier
 * class:    last-name
 * props:    [last-name-of Jim last-name]
 *
 * concrete: left-foot23
 * class:    left-foot
 * props:    [of left-foot Jim]
 *
 * concrete: hydrogen
 * class:    atom
 * props:    [atomic-weight-of atom maximal/minimal]
 *           (handle other locations of maximal/minimal)
 *
 * Return value of FLOATNEGINF means the given object is ruled out.
 */
Float DbSatisfiesIntension_Props(Ts *ts, TsRange *tsr, Obj *concrete,
                                 Obj *class, ObjList *props,
                                 Obj *skip_this_prop)
{
  Float		r;
  Obj		*value, *ptn, *elem;
  ObjList	*p;
  r = 0.0;
  for (p = props; p; p = p->next) {
    if (skip_this_prop == p->obj) {
    /* Not an optimization. This is essential to skip for example
     * a genitive prop which was the original pivot.
     */
      continue;
    }
    elem = PropFix(p->obj);
    if (N("set-relation") == I(elem, 0)) {
      if (!ISA(I(elem, 2), I(elem, 1))) {
        return(FLOATNEGINF);
      }
    } else if (N("standard-copula") == I(elem, 0)) {
      if (class == I(elem, 2)) {
        if (concrete != I(elem, 1)) {
          return(FLOATNEGINF);
        }
      } else if (class == I(elem, 1)) {
        if (concrete != I(elem, 2)) {
          return(FLOATNEGINF);
        }
      } else {
        return(FLOATNEGINF);
      }
    } else if (N("maximal") == I(elem, 2)) {
      value = DbGetRelationValue(ts, tsr, I(elem, 0), concrete, NULL);
      if (value) r += ObjToNumber(value);
      else return(FLOATNEGINF);
    } else if (N("minimal") == I(elem, 2)) {
      value = DbGetRelationValue(ts, tsr, I(elem, 0), concrete, NULL);
      if (value) r -= ObjToNumber(value);
      else return(FLOATNEGINF);
    } else {
      ptn = ObjSubst(elem, class, concrete);
      if (!REB(ts, tsr, ptn)) {
  /* todo: Use DbGetRelationValue or imitate its semantics. */
        return(FLOATNEGINF);
      }
/* Given "What is your name?" the below accepts
     [first-name-of TT Anglo-American-male-given-name]
   as well as
      aamgn-Thomas
   I guess we want only exact matches.
      if (!(objs = ProveRetrieve(ts, tsr, ptn, 1))) {
        return(FLOATNEGINF);
      }
      ObjListFree(objs);
*/
    }
  }
  return(r);
}

/* itn->props:      x1 AND x2 AND x3
 * itn->isas:       {x4 OR x5 OR x6} AND {x7 OR x8}
 * itn->attributes: {x9 OR x10 OR x11} AND {x12 OR x13}
 */
Float DbSatisfiesIntension(Obj *concrete, Intension *itn,
                           Obj *skip_this_prop)
{
  int		found;
  ObjListList	*p1;
  ObjList	*p2;
  if (itn->isas) {
    for (p1 = itn->isas; p1; p1 = p1->next) {
      if (p1->u.objs == NULL) continue;
            /* Due to ObjListListISACondenseDest. */
      if (p1->u.objs == itn->skip_this_isa) continue;	/* Optimization. */
      found = 0;
      for (p2 = p1->u.objs; p2; p2 = p2->next) {
        if (ISA(p2->obj, concrete)) {
          found = 1;
          break;
        }
      }
      if (!found) return(FLOATNEGINF);
    }
  }
  if (itn->attributes) {
    for (p1 = itn->attributes; p1; p1 = p1->next) {
      if (p1->u.objs == NULL) continue;	/* REDUNDANT */
      found = 0;
      for (p2 = p1->u.objs; p2; p2 = p2->next) {
        if (RNB(itn->ts, itn->tsr, L(p2->obj, concrete, E), 1)) {
          found = 1;
          break;
        }
      }
      if (!found) return(FLOATNEGINF);
    }
  }
  if (itn->ts_created) {
    if (!YES(RN(itn->ts_created, L(N("create"), ObjWild, concrete, E), 2))) {
         /* Was RD. */
      return(FLOATNEGINF);
    }
  }
#ifdef notdef
  /* Was (wrong logic?): */
  r = 0.0;
  if (itn->props) {
    val = DbSatisfiesIntension_Props(itn->ts, itn->tsr, concrete, itn->class,
                                    itn->props, skip_this_prop);
    if (val < r) r = val;
  }
  return(r);
#endif
  if (itn->props) {
    return(DbSatisfiesIntension_Props(itn->ts, itn->tsr, concrete, itn->class,
                                     itn->props, skip_this_prop));
  }
  return(0.0);
}

ObjList *DbFindIntension2(ObjList *candidates, Intension *itn,
                          Obj *skip_this_prop, ObjList *r)
{
  Float		maxvalue, val;
  ObjList	*p;
  maxvalue = FLOATNEGINF;
  for (p = candidates; p; p = p->next) {
    if (FLOATNEGINF !=
        (val = DbSatisfiesIntension(p->obj, itn, skip_this_prop))) {
      if (val > maxvalue) {
        ObjListFree(r);
        maxvalue = val;
        r = ObjListCreate(p->obj, NULL);
      } else if (val == maxvalue) {
        r = ObjListCreate(p->obj, r);
      }
    }
  }
  ObjListFree(candidates);
  return(r);
}

ObjList *DbPseudoOwnerRetrieve(Ts *ts, TsRange *tsr, Obj *obj, Obj *of_obj,
                               Obj *rel, int iobji, ObjList *r)
{
  ObjList       *objs, *p;
  if (iobji == 1) {
    objs = RDB(ts, tsr, L(rel, obj, of_obj, E), 1);
  } else if (iobji == 2) {
    objs = RDB(ts, tsr, L(rel, of_obj, obj, E), 2);
  } else {
    Dbg(DBGSEMPAR, DBGBAD, "DbPseudoOwnerRetrieve: unknown iobji");
    return(r);
  }
  for (p = objs; p; p = p->next) {
    r = ObjListCreate(I(p->obj, 1), r);
  }
  ObjListFree(objs);
  return(r);
}

/* obj       of_obj   retrieved                       r
 * ----      ------   ---------------------------     ---------
 * (1) PART-WHOLE RELATIONSHIP "Jim's left foot"      [of left-foot Jim]
 * left-foot Jim      [cpart-of left-foot23 Jim]      left-foot23
 *
 * (2) OWNERSHIP "my stereo" [of stereo Jim]
 * stereo    Jim      [owner-of Stereo23 Jim]         Stereo23
 *
 * (3) PSEUDO-OWNERSHIP "my apartment" [of apartment Jim]
 * apartment Jim      [residence-of Jim apt23]        apt23
 *                      "my book" [of book Jim]
 * book      Jim      [unique-author-of Jim book12]   book12
 *
 * todo: Pseudo-ownership is hardcoded. A more general way of implementing
 * this is to look for a relation with restriction to obj which of_obj also
 * satisfies. For already known it might work just to find any relations
 * matching [ObjWild obj of_obj].
 *
 * todo: "my living room"
 */
ObjList *DbGenitiveRetrieve(Ts *ts, TsRange *tsr, Obj *obj, Obj *of_obj)
{
  Obj		*cpart;
  ObjList	*r;

  /* PART-WHOLE RELATIONSHIP */
#ifdef notdef
  /* Why was this here? It prevents "Jim poured shampoo..." from working. */
  if (RNB(ts, tsr, L(N("part-of"), obj, of_obj, E), 2)) {
#endif
    if ((cpart = DbRetrievePart(ts, tsr, obj, of_obj))) {
    /* todoSCORE: This locks out other possibilities. */
      return(ObjListCreate(cpart, NULL));
    }
#ifdef notdef
  }  
#endif

  r = NULL;

  /* OWNERSHIP */
  r = DbPseudoOwnerRetrieve(ts, tsr, obj, of_obj, N("owner-of"), 1, r);

  /* PSEUDO-OWNERSHIP */
  r = DbPseudoOwnerRetrieve(ts, tsr, obj, of_obj, N("residence-of"), 2, r);
  r = DbPseudoOwnerRetrieve(ts, tsr, obj, of_obj, N("headquarters-of"), 2, r);
  r = DbPseudoOwnerRetrieve(ts, tsr, obj, of_obj, N("unique-author-of"), 1, r);

  return(r);
}

ObjList *DbFindHumansWithName(Ts *ts, TsRange *tsr, Obj *rel, ObjList *names,
                              ObjList *intersect)
{
  ObjList	*objs, *p, *q, *r;
  r = NULL;
  for (p = names; p; p = p->next) {
    objs = REB(ts, tsr, L(rel, ObjWild, p->obj, E));
    for (q = objs; q; q = q->next) {
      if ((!ISA(N("human"), I(q->obj, 1))) ||
          ISA(N("human-name"), I(q->obj, 1))) {
      /* todo: Is this really necessary? */
        continue;
      }
      if (intersect && (!ObjListIn(I(q->obj, 1), intersect))) continue;
      if (ObjListIn(I(q->obj, 1), r)) continue;
      r = ObjListCreate(I(q->obj, 1), r);
    }
  }
  return(r);
}

/* Returns ALL humans in db exactly matching the partial specification <nm>.
 * For example, in <nm> only specifies a first name, a lot of humans are
 * returned. If <nm> specifies a new name, NULL is returned.
 */
ObjList *DbHumanNameRetrieve(Ts *ts, TsRange *tsr, Name *nm)
{
  int		count;
  ObjList	*humans, *humans1;

  humans = NULL;
  count = 0;
  if (nm->surnames1) {
    humans1 = DbFindHumansWithName(ts, tsr, N("first-surname-of"),
                                   nm->surnames1, humans);
    if (humans1 == NULL) goto failure;
    count++;
    ObjListFree(humans);
    humans = humans1;
  }
  if (nm->givennames1) {
    humans1 = DbFindHumansWithName(ts, tsr, N("first-name-of"),
                                   nm->givennames1, humans);
    if (humans1 == NULL) goto failure;
    count++;
    ObjListFree(humans);
    humans = humans1;
  }
  if (nm->givennames2) {
    humans1 = DbFindHumansWithName(ts, tsr, N("second-name-of"),
                                   nm->givennames2, humans);
    if (humans1 == NULL) goto failure;
    count++;
    ObjListFree(humans);
    humans = humans1;
  }
  /* todoSCORE: We ignore other name slots for now. */
  return(humans);

failure:
  ObjListFree(humans);
  ObjListFree(humans1);
  return(NULL);
}

ObjList *DbFindIntensionHumanName(Intension *itn)
{
  Obj		*obj;
  ObjList	*candidates, *r, *p;

  if (itn->props && (obj = ObjListRelFind(itn->props, N("NAME-of")))) {
  /* obj: [NAME-of human NAME:"Jim"] */
    if ((candidates = DbHumanNameRetrieve(itn->ts, itn->tsr,
                                          ObjToHumanName(I(obj, 2))))) {
    /* candidates: Jim1, Jim2, Jim3 */
      if ((candidates = DbFindIntension2(candidates, itn, obj, NULL))) {
      /* candidates: Jim1, Jim3 */
        if (itn->cx) {
          r = NULL;
          for (p = candidates; p; p = p->next) {
            if (ContextActorFind(itn->cx, p->obj, 0)) {
              r = ObjListCreate(p->obj, r);
            } 
          } 
          if (r) {
          /* If any matches in context, those lock out all other matches.
           * todoSCORE
           */
            ObjListFree(candidates);
            return(r);
          } 
          /* Otherwise, all matches in the db are returned. */
          ObjListFree(r);
          return(candidates);
        } else {
          return(candidates);
        }
      }  
    }  
  }
  return(NULL);
}

ObjList *DbFindIntensionContext(Intension *itn)
{
  ObjList	*candidates;
  if (itn->cx && itn->class &&
      (candidates = ContextFindObjects(itn->cx, itn->class))) {
    return(DbFindIntension2(candidates, itn, NULL, NULL));
  }
  return(NULL);
}

/* itn->props: [of left-foot Jim]
 * itn->class: left-foot
 */
ObjList *DbFindIntension1(Intension *itn, ObjList *r)
{
  int		iobji;
  Float		weight;
  Obj		*ptn, *candidate, *rel, *of_obj, *obj;
  ObjList	*candidates, *r1;
  Proof		*proofs, *pr;

  /* todo: What if there is both a NAME-of and a genitive?
   * We need to put handlers for all these special types
   * into DbSatisfiesIntension no?
   */

  if ((r1 = DbFindIntensionHumanName(itn))) {
    r = ObjListAppendDestructive(r, r1);
    return(r);
  }

  if ((r1 = DbFindIntensionContext(itn))) {
    r = ObjListAppendDestructive(r, r1);
    return(r);
  }

  if (itn->cx_only) return(NULL);

  if (itn->props && (obj = ObjListFindGenitive(itn->props))) {
    if ((candidates = DbGenitiveRetrieve(itn->ts, itn->tsr, I(obj, 1),
                                         I(obj, 2)))) {
      return(DbFindIntension2(candidates, itn, obj, r));
    }
    return(NULL);
  }

  if (itn->class && itn->props &&
      (obj = ObjListFindRelation(itn->class, itn->props, &rel, &of_obj,
                                 &weight, &iobji))) {
  /* itn->class  obj                     candidates
   * ----------  -------                 ----------
   * human       [sister-of Jim human]   Karen
   * human       [President-of US human] Clinton Bush ...
   * todo: Use weight.
   */
    if ((candidates = DbGetRelationValues(itn->ts, itn->tsr, rel,
                                          of_obj, iobji, NULL))) {
      return(DbFindIntension2(candidates, itn, obj, r));
    }
    /* Why falling through should help I don't know, but we may as well try. */
  }

  if (itn->class == N("object")) {
    if (itn->props) {
      ptn = ObjSubst(itn->props->obj, N("object"), N("?nonhuman"));
      if (Prove(itn->ts, itn->tsr, ptn, NULL, 1, &proofs)) {
        candidates = NULL;
        for (pr = proofs; pr; pr = pr->next) {
          if ((candidate = BdLookup(pr->bd, N("?nonhuman")))) {
            candidates = ObjListCreate(candidate, candidates);
          }
        }
        ProofFree(proofs);
        if (candidates) {
          return(DbFindIntension2(candidates, itn, NULL, r));
        }
      }
    }
    return(NULL);
  }

  if (itn->class) {
    if (NULL == (candidates = ObjDescendants(itn->class, OBJTYPEANY))) {
      candidates = ObjListCreate(itn->class, NULL);
    }
    return(DbFindIntension2(candidates, itn, NULL, r));
  }

  return(NULL);
}

/* props:  [dense atom maximal]
 * result: [atomic-weight-of atom maximal]
 *         [density-of atom maximal]
 * todo: this needs to be generalized to work with ObjListLen(props) > 1
 * result: {OR: {AND: } {AND: }}
 */
ObjListList *ObjListListSuperlativeExpand(ObjList *props)
{
  Obj		*obj;
  ObjList	*p, *objs;
  ObjListList	*r;
  if (ObjListIsLengthOne(props) &&
      ISA(N("adverb-of-superlative-degree"), I(props->obj, 2))) {
    r = NULL;
    objs = RAD(&TsNA, L(N("attr-rel"), I(props->obj, 0), E), 1, 0);
    for (p = objs; p; p = p->next) {
      if (ISA(N("attr-rel-proportional"), I(p->obj, 0))) {
      /* p->obj: [attr-rel-proportional dense atomic-weight-of] */
        obj = L(I(p->obj, 2), I(props->obj, 1), I(props->obj, 2), E);
        r = ObjListListCreate(ObjListCreate(obj, NULL), r);
      }
    }
    if (r) return(r);
  }
  return(ObjListListCreate(props, NULL));
}

ObjList *DbFindIntension(Intension *itn, ObjList *r)
{
  ObjList	*save_props;
  ObjListList	*props, *pp;
  if (itn->props) {
    save_props = itn->props;
    props = ObjListListSuperlativeExpand(itn->props);
    for (pp = props; pp; pp = pp->next) {
      itn->props = pp->u.objs;
      r = DbFindIntension1(itn, r);
    }
    itn->props = save_props;
    return(r);
  } else {
    return(DbFindIntension1(itn, r));
  }
}

/******************************************************************************
 * SELECTIONAL RESTRICTIONS
 ******************************************************************************/

Obj *DbRestrictSlotname(int i)
{
  switch (i) {
    case 1: return(N("r1"));
    case 2: return(N("r2"));
    case 3: return(N("r3"));
    case 4: return(N("r4"));
    case 5: return(N("r5"));
    case 6: return(N("r6"));
    case 7: return(N("r7"));
    case 8: return(N("r8"));
    case 9: return(N("r9"));
    case 10: return(N("r10"));
  }
  Dbg(DBGDB, DBGBAD, "DbRestrictSlotname <%d>", i);
  return(N("r10"));
}

#define MAXRESTRICT 5

Bool DbIsLexEntryFound(Obj *prop, Obj *obj, Discourse *dc)
{
  int	theta_filled[MAXTHETAFILLED];
  ConToThetaFilled(obj, theta_filled);
  return(NULL != ObjToLexEntryGet1(prop, obj, "VAN", F_NULL, F_NULL,
                                   theta_filled, dc));
}

Bool DbRestrictionGen1(Obj *prop, Obj *concept, Obj *rel, int reli,
                       /* RESULT */ int *found)
{
  Obj	*res;
  if (!(res = R1EI(2, &TsNA, L(rel, prop, ObjWild, E)))) return(1);
  *found = 1;
  return(ISAP(res, I(concept, reli)));
}

void DbRestrictionGen(Obj *obj, Discourse *dc)
{
  int	i, len, found;
  Obj	*prop, *child;
  prop = I(obj, 0);
  for (i = 0, len = ObjNumChildren(prop); i < len; i++) {
    if ((child = ObjIthChild(prop, i))) {
      found = 0;
      if (DbRestrictionGen1(child, obj, N("r1"), 1, &found) &&
          DbRestrictionGen1(child, obj, N("r2"), 2, &found) &&
          DbRestrictionGen1(child, obj, N("r3"), 3, &found) &&
          DbRestrictionGen1(child, obj, N("r4"), 4, &found) &&
          DbRestrictionGen1(child, obj, N("r5"), 5, &found) &&
          found &&
          DbIsLexEntryFound(child, obj, dc)) {
            /* DbIsLexEntryFound is redoing work that is done in Generate3.
             * Not only is this inefficient, but it also might be unreliable.
             * todo: Integrate this function into Generate3?
             */
        ObjSetIth(obj, 0, child);
        return;
      }
    }
  }
}

Obj *PronounToClass(Obj *pronoun, PNode *pronoun_pn, int lang)
{
  int	gender, number, person;

  /* Determine from <pronoun> Obj alone. */
  if (ISA(N("location-pronoun"), pronoun)) {
  /* cf Lock out duration-of in "How many breads are there?"
   *                         and "She is where?".
   */
    return(N("location"));
  } else if (ISA(N("nonhuman-pronoun"), pronoun)) {
    return(N("nonhuman"));
  } else if (ISA(N("human-pronoun"), pronoun)) {
    return(N("human"));
  }

  if (lang == F_FRENCH) return(NULL);

  /* Determine from <pronoun_pn>: In English "it" "its" ... refer to nonhuman,
   * while "he" "she" "her" ... refer to human.
   */
  if (pronoun_pn == NULL) return(NULL);	/* todo */
  PNodeGetHeadNounFeatures(pronoun_pn, 0, &gender, &number, &person);
  if (gender == F_NEUTER) return(N("nonhuman"));
  else if (gender == F_MASCULINE || gender == F_FEMININE) return(N("human"));
  else return(NULL);
}

Bool ISAP_PNode(Obj *class, Obj *obj, PNode *obj_pn, int lang)
{
  Obj	*pronoun_class;
  if (ISA(N("F72"), class)) {
  /* <class> is a pronoun---the restriction knows what it is doing.
   * cf location-of-location-interrogative-pronoun1
   */
    return(ISAP(class, obj));
  } else if (ISA(N("F72"), obj)) {
  /* <obj> is a pronoun */
    if (ISA(N("pronoun-there-expletive"), obj)) {
    /* Expletive should never be an explicit argument?
     * cf "How many breads are there?" lock out duration-of
     */
      return(0);
    }
    if (NULL == (pronoun_class = PronounToClass(obj, obj_pn, lang))) {
      return(1);
    }
    /* Examples:
     * leave obj: subject-pronoun class: animal
     */
    return(ISAP(pronoun_class, class) ||
           ISAP(class, pronoun_class));
/* Was:
    if (ISA(N("human"), class)) return(N("human") == pronoun_class);
    else if (ISA(N("nonhuman"), class)) return(N("nonhuman") == pronoun_class);
    else return(1);
 */
  } else {
    return(ISAP(class, obj));
  }
}

/* Example:
 * elems[reli]: tv-set18
 * prop:        flip-to
 * r_rel:       r2
 * reli:        2
 */
Bool DbRestrictionParse1(Obj **elems, PNode **pn_elems, Obj *prop,
                         /* RESULTS */ int *len,
                         /* INPUT */ Obj *r_rel, int reli, Discourse *dc)
{
  Obj		*def;
  if (reli < *len) {
    if ((def = R1NI(2, &TsNA, L(r_rel, prop, ObjWild, E), 1)) &&
        (!ISAP_PNode(def, elems[reli],
                     pn_elems ? pn_elems[reli] : NULL,
                     DC(dc).lang))) {
      if (dc->relax) {
        /* todoFUNNY: Store this up and gen only if this parse is finally
         * accepted.
         */
        Dbg(DBGSEMPAR, DBGBAD, "relaxed: <%s> is not <%s>", M(elems[reli]),
            M(def), E);
        return(1);
      }
      if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
        fprintf(Log, "<%s> rejected because <%s> not a <%s>\n",
                M(prop), M(elems[reli]), M(def));
      }
      return(0);
    }
  }
  return(1);
}

Bool DbRestrictionParse(Obj **elems, PNode **pn_elems,
                        /* RESULTS */ int *newlen, /* INPUT */ Discourse *dc)
{
  int	i;
  Obj	*prop;
  prop = elems[0];
  for (i = 1; i < MAXRESTRICT; i++) {
    if (!DbRestrictionParse1(elems, pn_elems, prop, newlen,
                             DbRestrictSlotname(i), i, dc)) {
      return(0);
    }
  }
  return(1);
}

Obj *DbGetRestriction(Obj *obj, int slotnum)
{
  Obj	*r;
  if (obj == NULL) {
    Dbg(DBGSEMPAR, DBGBAD, "DbGetRestriction");
    return(N("concept"));
  }
  r = R1NI(2, &TsNA, L(DbRestrictSlotname(slotnum), obj, ObjWild, E), 1);
  if (r) return(r);
  else return(N("concept"));
}

Bool DbHasRestriction(Obj *obj, int slotnum)
{
  Obj	*r;
  r = R1NI(2, &TsNA, L(DbRestrictSlotname(slotnum), obj, ObjWild, E), 1);
  if (r) return 1;
  return 0;
}

/* todo: The restriction code is OK but needs some cleaning.
 * 2: satisfies (by default---no restriction is specified)
 * 1: satisfies restriction
 * 0: does not satisfy restriction
 */
Bool DbRestrictValidate1(Obj *obj, Obj *head, int i, Obj *slotvalue, Bool print,
                         int dbglevel)
{
  Obj	*class;
  if (N("concept") == (class = DbGetRestriction(head, i))) {
    /* Was exact; why? */
    return(2);
  }
  if (slotvalue == NULL) {
    if (print) {
      Dbg(DBGDB, DBGDETAIL, "slot %d with restriction to <%s> is empty:",
          i, M(class));
      if (obj) DbgOP(DBGDB, DBGDETAIL, obj);
    }
    return(1);
  }
  if (!ISAP(class, slotvalue)) {
    if (print && !Starting) {
      Dbg(DBGDB, dbglevel, "<%s> slot #%d <%s> is not <%s>", M(head), i,
          M(slotvalue), M(class));
      if (obj) DbgOP(DBGDB, dbglevel, obj);
    }
    return(0);
  }
  return(1);
}

Bool DbRestrictValidate(Obj *obj, Bool print)
{
  Obj	*head;
  int	i, r;
  head = I(obj, 0);
  r = 1;
  for (i = 1; i < MAXRESTRICT; i++) {
    if (!DbRestrictValidate1(obj, head, i, I(obj, i), print, DBGBAD)) {
      r = 0;
    }
  }
  return(r);
}

/******************************************************************************
 * DEBUGGING
 ******************************************************************************/

void DbQueryTool()
{
  char		buf[DWORDLEN];
  Obj		*obj;
  ObjList	*objs;
  Ts		ts;
  printf("Welcome to the Db query tool.\n");
  while (1) {
    if (!StreamAskStd("Enter timestamp (?=wildcard): ", DWORDLEN, buf)) return;
    if (!TsParse(&ts, buf)) continue;
    if ((obj = ObjEnterList())) {
      fprintf(stdout, "query pattern: ");
      fputc(TREE_TS_SLOT, stdout);
      TsPrint(stdout, &ts);
      fputc(TREE_SLOT_SEP, stdout);
      ObjPrint(stdout, obj);
      fprintf(stdout, "\n");
      fprintf(stdout, "results:\n");
      objs = RE(&ts, obj);
      ObjListPrettyPrint1(stdout, objs, 1, 0, 0, 0);
      ObjListPrettyPrint1(Log, objs, 1, 0, 0, 0);
      ObjListFree(objs);
    }
  }
}

/* End of file. */
