/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "reptime.h"
#include "semanaph.h"
#include "synpnode.h"
#include "utildbg.h"

ObjList *OBJLISTDEFER, *OBJLISTRULEDOUT;

/******************************************************************************
 * INITIALIZATION
 ******************************************************************************/

void ObjListInit()
{
  OBJLISTDEFER = ObjListCreate(ObjNA, NULL);
  OBJLISTRULEDOUT = ObjListCreate(ObjNA, NULL);
}

/******************************************************************************
 * CREATING
 ******************************************************************************/

ObjList *ObjListCreate(Obj *obj, ObjList *next)
{
  ObjList *objs;
  objs = CREATE(ObjList);
  objs->obj = obj;
  objs->u.sp.score = SCORE_MAX;
  objs->u.sp.leo = NULL;
  objs->u.sp.pn = NULL;
  objs->u.sp.anaphors = NULL;
  objs->next = next;
  return(objs);
}

ObjListList *ObjListListCreate(ObjList *objs, ObjListList *next)
{
  ObjList *r;
  r = CREATE(ObjList);
  r->obj = NULL;
  r->u.objs = objs;
  r->next = next;
  return(r);
}
 
/* SP = Sem_Parse */
ObjList *ObjListCreateSP(Obj *obj, Float score, LexEntryToObj *leo,
                         PNode *pn, Anaphor *anaphors, ObjList *next)
{
  ObjList *objs;
  objs = CREATE(ObjList);
  objs->obj = obj;
  objs->u.sp.score = score;
  objs->u.sp.leo = leo;
  objs->u.sp.pn = pn;
  objs->u.sp.anaphors = anaphors;
  objs->next = next;
  return(objs);
}

ObjList *ObjListCreateSPFrom(Obj *obj, PNode *pn, ObjList *p, ObjList *next)
{
  return(ObjListCreateSP(obj, p->u.sp.score, p->u.sp.leo, pn,
                         p->u.sp.anaphors, next));
}

ObjList *ObjListCreateSPCombine(Obj *obj, PNode *pn, ObjList *p, ObjList *q,
                                ObjList *next)
{
  return(ObjListCreateSP(obj, ScoreCombine(p->u.sp.score, q->u.sp.score),
                         NULL, pn,
                         AnaphorAppend(p->u.sp.anaphors, q->u.sp.anaphors),
                         next));
}

/* Users of this have to be careful. */
ObjList *ObjListCreateShort(Obj *obj, ObjList *next)
{
  ObjListShort *objs;
  objs = CREATE(ObjListShort);
  objs->obj = obj;
  objs->next = next;
  return((ObjList *)objs);
}

ObjList *ObjListCreateString(char *s, ObjList *next)
{
  ObjList *objs;
  objs = CREATE(ObjList);
  objs->obj = NULL;
  objs->u.s = s;
  objs->next = next;
  return(objs);
}

/******************************************************************************
 * FREEING
 ******************************************************************************/

void ObjListFree(ObjList *objs)
{
  ObjList *n;
  while (objs) {
    n = objs->next;
#ifdef maxchecking
    objs->obj = NULL;
#endif
    MemFree(objs, "ObjList");
    objs = n;
  }
}

void ObjListFreeObjs(ObjList *objs)
{
  ObjList *n;
  while (objs) {
    ObjFree(objs->obj);
    n = objs->next;
    MemFree(objs, "ObjList");
    objs = n;
  }
}

void ObjListListFree(ObjList *objs)
{
  ObjList *n;
  while (objs) {
    ObjListFree(objs->u.objs);
    n = objs->next;
    MemFree(objs, "ObjList");
    objs = n;
  }
}

void ObjListFreePNodes(ObjList *objs)
{
  ObjList *n;
  while (objs) {
    PNodeFreeTree(objs->u.sp.pn);
    n = objs->next;
    MemFree(objs, "ObjList");
    objs = n;
  }
}

void ObjListFreeFirst(ObjList *objs)
{
  MemFree(objs, "ObjList");
}

/******************************************************************************
 * COPYING
 ******************************************************************************/

ObjList *ObjListCopy(ObjList *objs)
{
  ObjList	*p, *r;
  r = NULL;
  for (p = objs; p; p = p->next) r = ObjListCreate(p->obj, r);
  return(r);
}

ObjList *ObjListReverse(ObjList *objs)
{
  ObjList	*p, *r;
  r = NULL;
  for (p = objs; p; p = p->next) r = ObjListCreate(p->obj, r);
  return(r);
}

ObjList *ObjListSubtractSimilar(ObjList *objs1, ObjList *objs2)
{
  ObjList	*p, *r;
  r = NULL;
  for (p = objs1; p; p = p->next) {
    if (!ObjListSimilarIn(p->obj, objs2)) {
      r = ObjListCreate(p->obj, r);
    }
  }
  return(r);
}

ObjList *ObjListReverseDest(ObjList *objs)
{
  ObjList	*p, *r;
  r = NULL;
  for (p = objs; p; p = p->next) r = ObjListCreate(p->obj, r);
  ObjListFree(objs);
  return(r);
}

ObjList *ObjListCollectRels(ObjList *objs, Obj *rel)
{
  ObjList	*r, *p;
  r = NULL;
  for (p = objs; p; p = p->next) {
    if (I(p->obj, 0) == rel) r = ObjListCreate(p->obj, r);
  }
  return(r);
}

/* This makes copies of the list objects themselves.
 * It does ObjListFreeObjs(objs).
 */ 
ObjList *ObjListReplicateNTimes(ObjList *objs, int n)
{
  int		i;
  ObjList	*r, *p;
  r = NULL;
  for (i = 0; i < n; i++) {
    for (p = objs; p; p = p->next) {
      r = ObjListCreate(ObjCopyList(p->obj), r);
    }
  }
  ObjListFreeObjs(objs);
  return(r);
}

ObjList *ObjListReplicateNTimesAnaphors(ObjList *objs, int n)
{
  int		i;
  ObjList	*r, *p;
  r = NULL;
  for (i = 0; i < n; i++) {
    for (p = objs; p; p = p->next) {
      r = ObjListCreate(ObjCopyList(p->obj), r);
      r->u.sp.anaphors = p->u.sp.anaphors;
    }
  }
  ObjListFreeObjs(objs);
  return(r);
}

/******************************************************************************
 * ACCESSORS
 ******************************************************************************/

Obj *ObjListHead(ObjList *objs, Obj *def)
{
  if (objs) return(objs->obj);
  else return(def);
}

ObjList *ObjListLast(ObjList *objs)
{
  while (objs->next) objs = objs->next;
  return(objs);
}

int ObjListLen(ObjList *objs)
{
  int	j;
  for (j = 0; objs; j++, objs = objs->next);
  return(j);
}

Bool ObjListIsLengthOne(ObjList *objs)
{
  return(objs && !objs->next);
}

int ObjListIsOK(ObjList *objs)
{
  for (; objs; objs = objs->next) {
    if (!ObjIsOK(objs->obj)) return(0);
  }
  return(1);
}

Bool ObjListIn(Obj *obj, ObjList *objs)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    if (p->obj == obj) return(1);
  }
  return(0);
}

Obj *ObjListIth(ObjList *objs, int i)
{
  int	j;
  for (j = 0; objs; j++, objs = objs->next)
    if (j == i) return(objs->obj);
  return(NULL);
}

Obj *ObjListMostRecent(ObjList *objs)
{
  Obj		*most_recent;
  ObjList	*p;
  Ts		ts_most_recent;
  most_recent = NULL;
  TsSetNegInf(&ts_most_recent);
  for (p = objs; p; p = p->next) {
    if (TsGT(&ObjToTsRange(p->obj)->startts, &ts_most_recent)) {
      most_recent = p->obj;
      ts_most_recent = ObjToTsRange(p->obj)->startts;
    }
  }
  return(most_recent);
}

Bool ObjList_AndISA(Obj *anc, ObjList *des)
{
  ObjList	*p;
  for (p = des; p; p = p->next) {
    if (!ISA(anc, p->obj)) return(0);
  }
  return(1);
}

ObjList *ObjListCommonAncestors1(Obj *obj1, ObjList *objs2, int depth,
                                 int maxdepth, ObjList *r)
{
  int		i;
  if (obj1->type == OBJTYPELIST ||
      depth >= maxdepth ||
      ObjBarrierISA(obj1)) {
    ObjListFree(r);
    return(NULL);
  }
  for (i = 0; i < obj1->u1.nlst.numparents; i++) {
    if (ObjList_AndISA(obj1->u1.nlst.parents[i], objs2)) {
      r = ObjListCreate(obj1->u1.nlst.parents[i], r);
    } else {
      r = ObjListCommonAncestors1(obj1->u1.nlst.parents[i], objs2, depth+1,
                                  maxdepth, r);
    }
  }
  return(r);
}

ObjList *ObjListCommonAncestors(ObjList *objs)
{
  if (objs == NULL) return(NULL);
  if (objs->next == NULL) return(ObjListCreate(objs->obj, NULL));
  return(ObjListUniquifyDest(ObjListCommonAncestors1(objs->obj, objs->next,
                                                     0, MAXISADEPTH,
                                                     NULL)));
}

Obj *ObjListCommonAncestor(ObjList *objs)
{
  Obj		*r;
  ObjList	*objs1;
  if ((objs1 = ObjListCommonAncestors(objs))) {
    r = objs1->obj;
    if (ObjIsContrast(r)) {
      r = ObjGetAParent(r);
        /* cf "I want to buy a desk phone."/"You are modal." */
    }
  } else {
    r = NULL;
  }
  ObjListFree(objs1);
  return(r);
}

Obj *ObjListRelFind(ObjList *objs, Obj *rel)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    if (rel == I(p->obj, 0)) return(p->obj);
  }
  return(NULL);
}

Bool ObjListInDeep(ObjList *objs, Obj *obj)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    if (ObjInDeep(obj, p->obj)) return(1);
  }
  return(0);
}

Bool ObjListISADeep(Obj *anc, ObjList *desc)
{
  ObjList	*p;
  for (p = desc; p; p = p->next) {
    if (ISA(anc, p->obj)) return(1);
  }
  return(0);
}

/* old: {(a b) (c) (d e f)}
 * new: g h
 * returns: {(a b g) (a b h) (c g) (c h) (d e f g) (d e f h)}
 */
ObjListList *ObjListListCartesianProduct(ObjListList *old, ObjList *new)
{
  ObjList	*p;
  ObjListList	*r, *pp;
  r = NULL;
  if (old == NULL) {
    for (p = new; p; p = p->next) {
      r = ObjListListCreate(ObjListCreate(p->obj, NULL), r);
    }
    return(r);
  }
  for (pp = old; pp; pp = pp->next) {
    for (p = new; p; p = p->next) {
    /* todoFREE: if you free, watch out for sharing. */
      r = ObjListListCreate(ObjListAppend(pp->u.objs,
                                          ObjListCreate(p->obj, NULL)),
                            r);
    }
  }
  return(r);
}

ObjList *ObjListListISACondenseDest(ObjList *objs)
{
  int			found_more_specific;
  ObjListList	*p1, *q1;
  ObjList		*p2, *q2;
  for (p1 = objs; p1; p1 = p1->next) {
    for (p2 = p1->u.objs; p2; p2 = p2->next) {
      found_more_specific = 0;
      for (q1 = objs; q1; q1 = q1->next) {
        for (q2 = q1->u.objs; q2; q2 = q2->next) {
          if (p2->obj == q2->obj) continue;
          if (ISA(p2->obj, q2->obj)) {
            Dbg(DBGGEN, DBGDETAIL,
                "ObjListListISACondenseDest: <%s> isa <%s>", M(q2->obj),
                M(p2->obj));
            found_more_specific = 1;
            break;
          }
        }
      }
      if (found_more_specific) {
        ObjListFree(p1->u.objs);
        p1->u.objs = NULL; /* was ObjListRemove(p1->u.objs, p2->obj); */
        /* If we found a match on p2->obj, then that means that was the
         * right interpretation. Thus all other interpretations of that
         * element are ruled out. Hence assign NULL
         * cf "WE 554 single-line dial wall phone"
         */
        break;
      }
    }
  }
  return(objs);
}

Obj *ObjListFindRelation(Obj *class, ObjList *props,
                         /* RESULTS */ Obj **rel, Obj **of_obj, Float *weight,
                         int *iobji)
{
  ObjList	*p;
  for (p = props; p; p = p->next) {
    if (ISA(N("relation"), I(p->obj, 0))) {
      if (ObjLen(p->obj) == 3 || ObjLen(p->obj) == 4) {
        if (I(p->obj, 1) == class) {
          *rel = I(p->obj, 0);
          *of_obj = I(p->obj, 2);
          *weight = ObjWeight(p->obj);
          *iobji = 2;
          return(p->obj);
        } if (I(p->obj, 2) == class) {
          *rel = I(p->obj, 0);
          *of_obj = I(p->obj, 1);
          *weight = ObjWeight(p->obj);
          *iobji = 1;
          return(p->obj);
        }
      }
    }
  }
  return(NULL);
}

Obj *ObjListFindGenitive(ObjList *props)
{
  ObjList	*p;
  for (p = props; p; p = p->next) {
    if (N("of") == I(p->obj, 0)) return(p->obj);
  }
  return(NULL);
}

ObjList *ObjListEnsureProductOfDest(ObjList *objs, Obj *brand)
{
  ObjList       *p;
  for (p = objs; p; p = p->next) {
    if (!RN(&TsNA, L(N("product-of"), brand, p->obj, E), 2)) {
      objs = ObjListRemove(objs, p->obj);
    } 
  }  
  return(objs);
}

/******************************************************************************
 * DESTRUCTIVE ACCESSORS
 ******************************************************************************/

Bool ObjListNonEmpty(ObjList *p)
{
  if (p) {
    ObjListFree(p);
    return(1);
  }
  return(0);
}

Obj *ObjListRandomElemFree(ObjList *objs)
{
  int	len;
  Obj	*obj;
  len = ObjListLen(objs);
  if (len > 0) {
    obj = ObjListIth(objs, Roll(len));
    ObjListFree(objs);
    return(obj);
  }
  return(NULL);
}

/* todo: efficiency
 * todoFREE
 */
ObjList *ObjListRandomize(ObjList *objs)
{
  int	len;
  Obj	*obj;
  len = ObjListLen(objs);
  if (len > 0) {
    obj = ObjListIth(objs, Roll(len));
    return(ObjListCreate(obj,
           ObjListRandomize(ObjListRemove(ObjListCopy(objs), obj))));
  }
  return(NULL);
}

/* Destructive on <objs>.
 * objs: {N("single-line-phone") N("manual-phone") N("dial-phone")
 *        N("multiline-phone")}
 * returns: {{N("single-line-phone") N("multiline-phone")}
 *           {N("manual-phone") N("dial-phone")}}
 */
ObjListList *ObjListPartition(ObjList *objs)
{
  int		depth;
  ObjList	*p1, *p2, *q1, *ancestors1, *partition;
  ObjListList	*r;
  r = NULL;
  depth = 0;
  while (objs && depth <= 3) {
    for (p1 = objs; p1; p1 = p1->next) {
      if (!ObjListIn(p1->obj, objs)) continue;	/* todo: inefficient */
      ancestors1 = ObjAncestorsOfDepth(p1->obj, OBJTYPEANY, depth);
      for (q1 = ancestors1; q1; q1 = q1->next) {
        partition = NULL;
        for (p2 = objs; p2; p2 = p2->next) {
          if (p1->obj == p2->obj) continue;
          if (ISA(q1->obj, p2->obj)) {
            partition = ObjListCreate(p2->obj, partition);
            objs = ObjListRemove(objs, p2->obj);	/* todo: inefficient */
          }
        }
        if (partition) {
          partition = ObjListCreate(p1->obj, partition);
          r = ObjListListCreate(partition, r);
          objs = ObjListRemove(objs, p1->obj);	/* todo: inefficient */
          break;
        }
      }
      ObjListFree(ancestors1);
    }
    depth++;
  }
  /* Put any remaining into their own partition. */
  for (p1 = objs; p1; p1 = p1->next) {
    r = ObjListListCreate(ObjListCreate(p1->obj, NULL), r);
  }
  return(r);
}

/******************************************************************************
 * APPENDERS
 ******************************************************************************/

ObjList *ObjListAppend(ObjList *objs1, ObjList *objs2)
{
  ObjList	*p;
  for (p = objs2; p; p = p->next) objs1 = ObjListCreate(p->obj, objs1);
  return(objs1);
}

ObjList *ObjListAppendNonequalSP(ObjList *objs1, ObjList *objs2, PNode *pn,
                                 Float pn_score)
{
  ObjList	*p;
  for (p = objs2; p; p = p->next) {
    if (!ObjListIn(p->obj, objs1)) {
      objs1 = ObjListCreateSP(p->obj, ScoreCombine(pn_score, p->u.sp.score),
                              p->u.sp.leo, p->u.sp.pn, p->u.sp.anaphors,
                              objs1);
      objs1->u.sp.pn = pn;
    }
  }
  return(objs1);
}

void ObjListSetPNodeSP(ObjList *objs, PNode *pn)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    p->u.sp.pn = pn;
  }
}

void ObjListSetScoreSP(ObjList *objs, Float score)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    p->u.sp.score = score;
  }
}

ObjList *ObjListAppendNonsimilar(ObjList *objs1, ObjList *objs2)
{
  ObjList	*p;
  for (p = objs2; p; p = p->next) {
    if (!ObjListSimilarIn(p->obj, objs1)) {
      objs1 = ObjListCreate(p->obj, objs1);
    }
  }
  return(objs1);
}

ObjList *ObjListAppendNonequal(ObjList *objs1, ObjList *objs2)
{
  ObjList	*p;
  for (p = objs2; p; p = p->next) {
    if (!ObjListIn(p->obj, objs1)) {
      objs1 = ObjListCreate(p->obj, objs1);
    }
  }
  return(objs1);
}

ObjList *ObjListUniquify(ObjList *objs)
{
  return(ObjListAppendNonsimilar(NULL, objs));
}

/* todoFREE */
ObjList *ObjListTenseAddSP(ObjList *objs, Obj *tense)
{
  ObjList	*p, *r;
  if (tense == NULL) {
    fprintf(Log, "rejected because compound tense not recognized\n");
    return(NULL);
  }
  r = NULL;
  for (p = objs; p; p = p->next) {
    r = ObjListCreateSP(L(tense, p->obj, E), p->u.sp.score, p->u.sp.leo,
                        p->u.sp.pn, p->u.sp.anaphors, r);
  }
  return(r);
}

ObjList *ObjListWrap(ObjList *objs, Obj *wrap)
{
  ObjList	*p, *r;
  r = NULL;
  for (p = objs; p; p = p->next) {
    r = ObjListCreateSP(L(wrap, p->obj, E), p->u.sp.score, p->u.sp.leo,
                        p->u.sp.pn, p->u.sp.anaphors, r);
  }
  return r;
}

/* This is O(n**2) and gets very slow. */
ObjList *ObjListUniquifyDest(ObjList *objs)
{
  ObjList	*r;
  r = ObjListAppendNonsimilar(NULL, objs);
  ObjListFree(objs);
  return(r);
}

ObjList *ObjListAddIfNotIn(ObjList *objs, Obj *obj)
{
  if (!ObjListIn(obj, objs)) {
    return(ObjListCreate(obj, objs));
  }
  return(objs);
}

ObjList *ObjListAddIfNotInObjList(ObjList *objs, ObjList *objs1)
{
  for (; objs1; objs1 = objs1->next) {
    if (!ObjListIn(objs1->obj, objs)) {
      objs = ObjListCreate(objs1->obj, objs);
    }
  }
  return(objs);
}

ObjList *ObjListAddIfSimilarNotIn(ObjList *objs, Obj *obj)
{
  if (!ObjListSimilarIn(obj, objs)) {
    return(ObjListCreate(obj, objs));
  }
  return(objs);
}

/******************************************************************************
 * MODIFIERS
 ******************************************************************************/

ObjList *ObjListRemove(ObjList *objs, Obj *obj)
{
  ObjList	*p, *prev;
  prev = NULL;
  for (p = objs; p; p = p->next) {
    if (obj == p->obj) {
      /* Splice out. */
      if (prev) {
        prev->next = p->next;
        /* todoFREE */
      } else objs = p->next;
    } else prev = p;
  }
  return(objs);
}

/* An obj->count map. */
ObjList *ObjListIncrement(ObjList *objs, Obj *obj)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    if (p->obj == obj) {
      p->u.n++;
      return(objs);
    }
  }
  p = ObjListCreate(obj, objs);
  p->u.n = 1;
  return(p);
}

void ObjListISACount(/* INPUT AND RESULTS */ ObjList *anc,
                     /* INPUT */ ObjList *des)
{
  ObjList	*p, *q;
  for (q = des; q; q = q->next) q->u.n = 0;
  for (p = anc; p; p = p->next) {
    p->u.n = 0;
    for (q = des; q; q = q->next) {
      if (q->u.n) continue;
        /* Don't double count descendants---this way we get a valid
         * coverage number for use by ObjListCutScore
         */
      if (ISA(p->obj, q->obj)) {
        p->u.n++;
        q->u.n = 1;
      }
    }
  }
}

void ObjListListISACount(/* INPUT AND RESULTS */ ObjListList *anc,
                         /* INPUT */ ObjList *des)
{
  ObjListList	*p;
  for (p = anc; p; p = p->next) {
    ObjListISACount(p->u.objs, des);
  }
}

void ObjListAttributeCount(Ts *ts, TsRange *tsr,
                           /* INPUT AND RESULTS */ ObjList *attrs,
                           /* INPUT */ ObjList *des)
{
  ObjList	*p, *q;
  for (q = des; q; q = q->next) q->u.n = 0;
  for (p = attrs; p; p = p->next) {
    p->u.n = 0;
    for (q = des; q; q = q->next) {
      if (q->u.n) continue;
        /* Don't double count descendants---this way we get a valid
         * coverage number for use by ObjListCutScore
         */
      if (RNB(ts, tsr, L(p->obj, q->obj, E), 1)) {
        p->u.n++;
        q->u.n = 1;
      }
    }
  }
}

void ObjListListAttributeCount(Ts *ts, TsRange *tsr,
                               /* INPUT AND RESULTS */ ObjListList *anc,
                               /* INPUT */ ObjList *des)
{
  ObjListList	*p;
  for (p = anc; p; p = p->next) {
    ObjListAttributeCount(ts, tsr, p->u.objs, des);
  }
}

Obj *ObjListMax(ObjList *objs, /* RESULTS */ int *max_n)
{
  int		maxn;
  Obj		*maxobj;
  ObjList	*p;
  maxn = INTNEGINF;
  maxobj = NULL;
  for (p = objs; p; p = p->next) {
    if (p->u.n > maxn) {
      maxn = p->u.n;
      maxobj = p->obj;
    }
  }
  if (max_n) *max_n = maxn;
  return(maxobj);
}

int ObjListSum(ObjList *objs)
{
  ObjList	*p;
  int		sum;
  sum = 0;
  for (p = objs; p; p = p->next) {
    sum += p->u.n;
  }
  return(sum);
}

Float ObjListAvg(ObjList *objs)
{
  ObjList	*p;
  Float		n, sum;
  n = sum = 0.0;
  for (p = objs; p; p = p->next) {
    n += 1.0;
    sum += (Float)p->u.n;
  }
  if (n == 0.0) return(0.0);
  return(sum/n);
}

Float ObjListVariance(ObjList *objs)
{
  ObjList	*p;
  Float		avg, n, sum;
  avg = ObjListAvg(objs);
  n = sum = 0.0;
  for (p = objs; p; p = p->next) {
    n += 1.0;
    sum += pow(((Float)p->u.n) - avg, 2.0);
  }
  if (n == 0.0) return(0.0);
  return(sum/n);
}

Float ObjListSd(ObjList *objs)
{
  return(pow(ObjListVariance(objs), 0.5));
}

Float ObjListCutScore(ObjList *cut)
{
  Float	sd, cov;
  sd = ObjListSd(cut);
  cov = (Float)ObjListSum(cut);
  if (DbgOn(DBGUA, DBGDETAIL)) {
    fputs("-----------\n", Log);
    ObjListPrint1(Log, cut, 0, 1, 0, 0);
    fprintf(Log, "sd %g cov %g score %g\n", sd, cov, cov - sd);
  }
  return(cov - sd);
}

ObjList *ObjListListBestCut(ObjListList *cut)
{
  Float		score, maxscore;
  ObjList	*maxcut;
  ObjListList	*p;
  maxcut = NULL;
  maxscore = FLOATNEGINF;
  for (p = cut; p; p = p->next) {
    score = ObjListCutScore(p->u.objs);
    if (score > maxscore) {
      maxcut = p->u.objs;
      maxscore = score;
    }
  }
  return(maxcut);
}

void ObjListSetTsRange(ObjList *objs, TsRange *tsr)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    ObjSetTsRange(p->obj, tsr);
  }
}

ObjList *ObjListAppendDestructive(ObjList *objs1, ObjList *objs2)
{
  if (objs2 == objs1) {
  /* cf pn->concepts */
    return(objs1);
  }
  if (objs1) {
    ObjListLast(objs1)->next = objs2;
    return(objs1);
  } else return(objs2);
}

Anaphor *ObjListSPAnaphorAppend(ObjList *objs)
{
  ObjList	*p;
  Anaphor	*an;
  an = NULL;
  for (p = objs; p; p = p->next) {
    an = AnaphorAppendDestructive(an, AnaphorCopyAll(p->u.sp.anaphors));
  }
  return(an);
}

ObjList *ObjListRemoveMoreGeneral(ObjList *objs, Obj *obj)
{
  ObjList	*p, *prev;
  prev = NULL;
  for (p = objs; p; p = p->next) {
    if (ObjIsSpecif(obj, p->obj)) {
      /* Splice out. */
      if (prev) {
        prev->next = p->next;
        /* todoFREE */
      } else objs = p->next;
    } else prev = p;
  }
  return(objs);
}

ObjList *ObjListRemoveLessGeneralISA(ObjList *objs)
{
  int		found_more_general;
  ObjList	*p, *q, *prev;
  prev = NULL;
  for (p = objs; p; p = p->next) {
    found_more_general = 0;
    for (q = objs; q; q = q->next) {
      if (p->obj == q->obj) continue;
      if (ISA(q->obj, p->obj)) {
        found_more_general = 1;
        break;
      }
    }
    if (found_more_general) {
      /* Splice out. */
      if (prev) {
        prev->next = p->next;
        /* todoFREE */
      } else objs = p->next;
    } else prev = p;
  }
  return(objs);
}

ObjList *ObjListRemoveInDeep(ObjList *objs, Obj *obj)
{
  ObjList	*p, *prev;
  prev = NULL;
  for (p = objs; p; p = p->next) {
    if (ObjInDeep(obj, p->obj)) {
      /* Splice out. */
      if (prev) {
        prev->next = p->next;
        /* todoFREE */
      } else objs = p->next;
    } else prev = p;
  }
  return(objs);
}

/* Eliminate duplicate concepts and eliminate concepts for which there is
 * a more specific version.
 */
ObjList *ObjListUniquifySpecSP(ObjList *objs)
{
  ObjList	*p, *r;
  r = NULL;
  for (p = objs; p; p = p->next) {
    r = ObjListRemoveMoreGeneral(r, p->obj);
    r = ObjListCreateSP(p->obj, p->u.sp.score, p->u.sp.leo, p->u.sp.pn,
                        p->u.sp.anaphors, r);
  }
  return(r);
}

/* Frees <objs>. */
ObjList *ObjListUniquifySpecDestSP(ObjList *objs)
{
  ObjList	*r;
  r = ObjListUniquifySpecSP(objs);
  ObjListFree(objs);
  return(r);
}

ObjList *ObjListAppendNonsimilarSP(ObjList *objs1, ObjList *objs2)
{
  ObjList	*p;
  for (p = objs2; p; p = p->next) {
    if (!ObjListSimilarIn(p->obj, objs1)) {
      objs1 = ObjListCreateSP(p->obj, p->u.sp.score, p->u.sp.leo,
                              p->u.sp.pn, p->u.sp.anaphors, objs1);
    }
  }
  return(objs1);
}

/* Frees <objs>. */
ObjList *ObjListUniquifyDestSP(ObjList *objs)
{
  ObjList	*r;
  r = ObjListAppendNonsimilarSP(NULL, objs);
  ObjListFree(objs);
  return(r);
}

Float ObjListSPScoreCombine(ObjList *objs)
{
  ObjList	*p;
  Float		r;
  r = SCORE_MAX;
  for (p = objs; p; p = p->next) {
    r = ScoreCombine(r, p->u.sp.score);
  }
  return(r);
}

void ObjListSPScoreCombineWith(ObjList *objs, Float score)
{
  while (objs) {
    objs->u.sp.score = ScoreCombine(score, objs->u.sp.score);
    objs = objs->next;
  }
}

/******************************************************************************
 * MAPS
 ******************************************************************************/

ObjList *ObjListMapAdd(ObjList *map, Obj *from, Obj *to)
{
  map = ObjListCreate(from, map);
  map->u.tgt_obj = to;
  return(map);
}

/* todo: Could reimplement Bd as ObjList maps. */
Obj *ObjListMapLookup(ObjList *map, Obj *from)
{
  ObjList	*p;
  for (p = map; p; p = p->next) {
    if (from == p->obj) return(p->u.tgt_obj);
  }
  return(NULL);
}

Obj *ObjListMapLookupInv(ObjList *map, Obj *to)
{
  ObjList	*p;
  for (p = map; p; p = p->next) {
    if (to == p->u.tgt_obj) return(p->obj);
  }
  return(NULL);
}

void ObjListMapPrint(FILE *stream, ObjList *map)
{
  ObjList	*p;
  for (p = map; p; p = p->next) {
    ObjPrint1(stream, p->obj, NULL, 10, 0, 0, 0, 0);
    fputs(" --> ", stream);
    ObjPrint1(stream, p->u.tgt_obj, NULL, 10, 0, 0, 0, 0);
    fputc(NEWLINE, stream);
  }
}

/******************************************************************************
 * SIMILARITY
 ******************************************************************************/

Bool ObjListSimilarIn(Obj *obj, ObjList *objs)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    if ((I(obj, 0) == I(p->obj, 0)) &&
          /* todo: An OK optimization for cases handled? */
        ObjSimilarList(obj, p->obj)) {
      return(1);
    }
  }
  return(0);
}

Bool ObjListSimilar(ObjList *objs1, ObjList *objs2)
{
  while (objs1 && objs2) {
    if (!ObjSimilarList(objs1->obj, objs2->obj)) return(0);
    objs1 = objs1->next;
    objs2 = objs2->next;
  }
  if (objs1 || objs2) return(0);
  return(1);
}

/******************************************************************************
 * SORTING
 ******************************************************************************/

Obj **ObjListToArray(ObjList *objs, /* RESULTS */ size_t *numelem,
                     size_t *size)
{
  size_t	i, len;
  Obj		**r;
  ObjList	*p;
  len = ObjListLen(objs);
  r = (Obj **)MemAlloc(sizeof(Obj *)*len, "ObjArray");
  for (i = 0, p = objs; p; p = p->next) {
    if (p->obj == NULL) {
      Dbg(DBGOBJ, DBGBAD, "ObjListToArray: empty ObjList element");
    } else if (ObjIsOK1(p->obj)) {
      r[i++] = p->obj;
    } else {
      Dbg(DBGOBJ, DBGBAD, "ObjListToArray: bad Obj");
    }
  }
  *size = sizeof(Obj *);
  *numelem = i;
  return(r);
}

ObjList *ObjListArrayToList(Obj **arr, size_t numelem)
{
  ObjList	*r;
  size_t	i;
  r = NULL;
  for (i = 0; i < numelem; i++) {
    r = ObjListCreate(arr[i], r);
  }
  return(r);
}

/* <compar> is backwards from the qsort standard. */
ObjList *ObjListSort(ObjList *objs,
                     int (*compar)(const void *, const void *))
{
#ifdef GCC
  /* todo: Locate problem with qsort or someone trashing arr. */
  return(ObjListCopy(objs));
#else
  Obj		**arr;
  ObjList	*r;
  size_t	numelem, size;
  arr = ObjListToArray(objs, &numelem, &size);
  if (arr >= (Obj **)0x8000000) {
    Dbg(DBGOBJ, DBGDETAIL,
"ObjListSort: aborted due to qsort sign extension bug in Solaris x86 2.4");
    MemFree(arr, "ObjArray");
    return(ObjListCopy(objs));
  }
#ifdef notdef
  size_t        i;
  for (i = 0; i < numelem; i++) {
    if (!ObjIsOK1(arr[i])) {
      Dbg(DBGOBJ, DBGBAD, "ObjListSort: arr[%ld] is bad Obj", i);
      MemFree(arr, "ObjArray");
      return(ObjListCopy(objs));
    }
  }
#endif
  Dbg(DBGOBJ, DBGOK, "qsort numelem=%ld size=%ld", numelem, size);
  qsort((void *)arr, numelem, size, compar);
  r = ObjListArrayToList(arr, numelem);
  MemFree(arr, "ObjArray");
  return(r);
#endif
}

int ObjStartTsGT(const void *obj1, const void *obj2)
{
  if (TsGT1(&ObjToTsRange(*((Obj **)obj1))->startts,
            &ObjToTsRange(*((Obj **)obj2))->startts)) {
    return(-1);
  } else {
    return(1);
  }
}

ObjList *ObjListSortByStartTs(ObjList *objs)
{
  return(ObjListSort(objs, ObjStartTsGT));
}

int ObjStartTsrGT(const void *obj1, const void *obj2)
{
  if (TsGT1(&ObjToTsRange(*((Obj **)obj1))->startts,
            &ObjToTsRange(*((Obj **)obj2))->startts)) {
    return(-1);
  } else if (TsGT1(&ObjToTsRange(*((Obj **)obj1))->stopts,
            &ObjToTsRange(*((Obj **)obj2))->stopts)) {
    return(-1);
  } else {
    return(1);
  }
}

ObjList *ObjListSortByTsRange(ObjList *objs)
{
  return(ObjListSort(objs, ObjStartTsrGT));
}

Float ObjListScoreMax(ObjList *objs)
{
  Float		maxscore;
  ObjList	*p;
  maxscore = FLOATNEGINF;
  for (p = objs; p; p = p->next) {
    if (p->u.sp.score > maxscore) maxscore = p->u.sp.score;
  }
  return(maxscore);
}

/* Destructive on <objs>.
 * todo: Make this more efficient.
 */
ObjList *ObjListScoreSortPrune1(ObjList *objs, int maxlen)
{
  Float		maxscore;
  ObjList	*prev, *p, *r;
  r = NULL;
  while (1) {
    if (objs == NULL) return(r);
    maxscore = ObjListScoreMax(objs);
    prev = NULL;
    for (p = objs; p; p = p->next) {
      if (maxscore == p->u.sp.score) {
        /* Splice out. */
        if (prev) {
          prev->next = p->next;
        } else objs = p->next;
        p->next = NULL;
        r = ObjListAppendDestructive(r, p);
        maxlen--;
        if (maxlen <= 0) return(r);
      } else prev = p;
    }
  }
}

/* Destructive on <objs>.
 * This always keeps the first <objs> element, because, mysteriously,
 * the first element returned has been observed to have validity
 * more often than would be expected by chance.
 */
ObjList *ObjListScoreSortPrune(ObjList *objs, int maxlen)
{
  ObjList	*r;
  if (objs == NULL) return(NULL);
  r = ObjListScoreSortPrune1(objs->next, maxlen-1);
  objs->next = r;
  return(objs);
}

Obj *ObjListScoreHighest(ObjList *objs)
{
  Float maxscore;
  ObjList *p;
  maxscore = ObjListScoreMax(objs);
  for (p = objs; p; p = p->next) {
    if (maxscore == p->u.sp.score) {
      return p->obj;
    }
  }
  return NULL;
}

/******************************************************************************
 * PRINTING
 ******************************************************************************/

void SPPrint(FILE *stream, SP *sp)
{
/*
  ScorePrint(stream, sp->score);
*/
  PNodePrint1(stream, NULL, sp->pn, 0);
  if (sp->anaphors) {
    fputc(SPACE, stream);
    AnaphorPrintAllLine(stream, sp->anaphors);
  }
}

void SPInit(SP *sp)
{
  sp->score = SCORE_MAX;
  sp->pn = NULL;
  sp->anaphors = NULL;
  sp->leo = NULL;
}

void ObjListPrint1(FILE *stream, ObjList *objs, int show_tsr, int show_count,
                   int show_score, int show_sp)
{
  ObjList	*p;
  if (objs == NULL) {
    fputs("NULL ObjList\n", stream);
    return;
  }
  for (p = objs; p; p = p->next) {
    if (show_count) fprintf(Log, "%ld ", p->u.n);
    if (show_score) ScorePrint(stream, p->u.sp.score);
    ObjPrint1(stream, p->obj, NULL, 10, show_tsr, 0, 0, 0);
    if (show_sp) {
      fputc(SPACE, stream);
      SPPrint(stream, &p->u.sp);
    }
    fputc(NEWLINE, stream);
  }
}

void ObjListPrint(FILE *stream, ObjList *objs)
{
  ObjListPrint1(stream, objs, 1, 0, 1, 0);
}


void ObjListListPrint(FILE *stream, ObjListList *objs, int show_tsr,
                      int show_count, int show_score)
{
  ObjListList	*p;
  for (p = objs; p; p = p->next) {
    fputs("###############\n", stream);
    ObjListPrint1(stream, p->u.objs, show_tsr, show_count, show_score, 0);
  }
}

void ObjListPrintSpace(FILE *stream, ObjList *objs, int show_tsr)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    ObjPrint1(stream, p->obj, NULL, 10, show_tsr, 0, 0, 0);
    if (p->next) fputc(SPACE, stream);
  }
}

void ObjListPrintNames(FILE *stream, ObjList *objs)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    fputs(M(p->obj), stream);
    if (p->next) fputc(',', stream);
  }
}

void ObjListPrettyPrint1(FILE *stream, ObjList *objs, int show_tsr,
                         int show_addr, int indicate_pn, int html)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    ScorePrint(stream, p->u.sp.score);
    ObjPrettyPrint1(stream, p->obj, NULL, 0, 0, show_tsr, show_addr,
                    indicate_pn, html);
    fputc(NEWLINE, stream);
  }
}

void ObjListPrettyPrint(FILE *stream, ObjList *objs)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    ScorePrint(stream, p->u.sp.score);
    ObjPrettyPrint(stream, p->obj);
  }
}

/* End of file. */
