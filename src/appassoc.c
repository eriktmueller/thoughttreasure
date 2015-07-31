/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Object- and assertion-based associative streams of thought.
 *
 * 19941231: begun
 *
 * todo:
 * - Handle natural-language (as opposed to dictionary tool) request to free
 *   associate on a given concept.
 * - Generate random/nonsense concepts with and without satisfying various
 *   restrictions.
 * - Generate random/nonsense sentences (parse trees). Can be used to
 *   check for coding errors.
 * - Write nonsense technotalk generator, like the Valley Talk generator
 *   we did in Michael Dyer's class.
 */

#include "tt.h"
#include "appassoc.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "utildbg.h"

/******************************************************************************
 * object-based association
 ******************************************************************************/

Obj *AssocObjectPick1(ObjList *objs, int i, ObjList *visited, int concrete_only)
{
  Obj		*obj;
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    if (i < 0) obj = p->obj;
    else obj = I(p->obj, i);
    if (ObjListIn(obj, visited)) continue;
    if (!ObjIsSymbol(obj)) continue;
    if (concrete_only && (!ObjIsConcrete(obj))) continue;
    return(obj);
  }
  return(NULL);
}

Obj *AssocObjectPick(ObjList *objs, int i, ObjList *visited)
{
  Obj	*r;
  objs = ObjListRandomize(objs);
  if ((r = AssocObjectPick1(objs, i, visited, 1))) {
    return(r);
  }
  if ((r = AssocObjectPick1(objs, i, visited, 0))) {
    return(r);
  }
  return(NULL);
}

Obj *AssocObjectNextObject(Obj *from, ObjList *visited)
{
  Obj	*to;
  if ((to = AssocObjectPick(RD(&TsNA, L(ObjWild, ObjWild, from, E), 2),
                               1, visited))) {
    return(to);
  }
  if ((to = AssocObjectPick(RD(&TsNA, L(ObjWild, from, ObjWild, E), 1),
                               2, visited))) {
    return(to);
  }
  if (WithProbability(0.5)) {
    if ((to = AssocObjectPick(ObjChildren(from, OBJTYPEANY), -1, visited))) {
      return(to);
    }
  }
  if ((to = AssocObjectPick(ObjParents(from, OBJTYPEANY), -1, visited))) {
    return(to);
  }
  return(NULL);
}

Obj *AssocObjectRandomObject()
{
  Obj	*obj;
  long	i, cnt;
  for (cnt = 0, obj = Objs; obj; cnt++, obj = obj->next);
  cnt = RandomIntFromTo(0, cnt);
  for (i = 0, obj = Objs; obj; i++, obj = obj->next) {
    if (!ObjIsConcrete(obj)) continue;
    if (i > cnt) return(obj);
  }
  return(Objs);
}

ObjList *AssocObject(Discourse *dc, Obj *con, int maxlen)
{
  int		i;
  ObjList	*associations, *visited;
  associations = NULL;
  if (con == NULL) con = AssocObjectRandomObject();
  visited = NULL;
  for (i = 0; con && i < maxlen; i++) {
    visited = ObjListCreate(con, visited);
    associations = ObjListCreate(con, associations);
    con = AssocObjectNextObject(con, visited);
  }
  associations = ObjListReverseDest(associations);
  return(associations);
}

/******************************************************************************
 * concept-based association
 ******************************************************************************/

int AssocAssertion2(Obj *obj1, Obj *obj2, Obj *class, ObjList *r, int depth,
                    int maxdepth, /* RESULTS */ ObjList **out)
{
  int			i, len, found;
  Obj			*iobj;
  ObjList		*objs, *p, *r1, *r2;
  found = 0;
  /*
  IndentUp();
  IndentPrint(Log);
  fprintf(Log, "AssocAssertion2 <%s> <%s>\n", M(obj1), M(obj2));
   */
  objs = ObjListRandomize(DbRetrieveInvolving(&TsNA, NULL, obj1, 0, NULL));
  for (p = objs; p; p = p->next) {
    /*
    IndentPrint(Log);
    ObjPrint(Log, p->obj);
    fputc(NEWLINE, Log);
     */
    if (p->obj == NULL) continue;	/* todo */
    if (ObjListIn(p->obj, r)) continue;
    for (i = 1, len = ObjLen(p->obj); i < len; i++) {
      if ((obj1 == (iobj = ObjIth(p->obj, i))) ||
          (iobj == ObjNA) ||
          (class && !ISA(class, iobj))) {
        continue;
      }
      if (obj2 == iobj) {
        r = ObjListCreate(p->obj, r);
        found = 1;
        break;
      } else {
        r2 = ObjListCreate(p->obj, r);
        if (AssocAssertion1(iobj, obj2, class, r2, depth+1, maxdepth, &r1)) {
          r = r1;
          found = 1;
          break;
        } else ObjListFreeFirst(r2);
      }
    }
    if (found) break;
  }
  ObjListFree(objs);
  *out = r;
  /*
  IndentDown();
   */
  if ((!found) && r != NULL) return(1);
  return(found);
}

int AssocAssertion1(Obj *obj1, Obj *obj2, Obj *class, ObjList *r, int depth,
                    int maxdepth, /* RESULTS */ ObjList **out)
{
  int		i;
  if (depth > maxdepth) return(0);
  for (i = 0; i < 5; i++) {
    if (AssocAssertion2(obj1, obj2, class, r, depth, maxdepth, out)) {
      return(1);
    }
  }
  return(0);
}

/* if <obj2> == NULL, free associate from <obj1>
 * else associate along a path from <obj1> to <obj2>
 */
ObjList *AssocAssertion(Obj *obj1, Obj *obj2, Obj *class)
{
  ObjList	*r;
  IndentInit();
  AssocAssertion1(obj1, obj2, class, NULL, 0, 30, &r);
  r = ObjListReverseDest(r);
  if (DbgOn(DBGGEN, DBGHYPER)) {
    Dbg(DBGGEN, DBGHYPER, "AssocAssertion output:");
    ObjListPrint(Log, r);
  }
  return(r);
}

/* End of file. */
