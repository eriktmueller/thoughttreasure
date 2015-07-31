/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19951003: begun
 *
 * - "How many Beatles are there?"
 * - "There are 5 people in a room." => Instantiate 5 people. But if there
 *    are 3000, don't instantiate 3000 people.
 */

#include "tt.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "utildbg.h"

ObjList *GroupMembers(Ts *ts, Obj *group)
{
  return(REI(1, ts, L(N("part-of"), ObjWild, group, E)));
}

Bool GroupConsistsOf(Ts *ts, Obj *group, ObjList *members)
{
  ObjList	*p, *group_members;
  group_members = GroupMembers(ts, group);
  for (p = group_members; p; p = p->next) {
    if (!ObjListIn(p->obj, members)) goto failure;
  }
  for (p = members; p; p = p->next) {
    if (ObjNA == p->obj || NULL == p->obj) continue;
    if (!ObjListIn(p->obj, group_members)) goto failure;
  }
  ObjListFree(group_members);
  return(1);
failure:
  ObjListFree(group_members);
  return(0);
}

Obj *GroupGet(Ts *ts, ObjList *members)
{
  ObjList	*p, *objs;
  if (members == NULL) return(NULL);
  objs = RE(ts, L(N("part-of"), members->obj, ObjWild, E));
  for (p = objs; p; p = p->next) {
    if (GroupConsistsOf(ts, I(p->obj, 2), members)) {
      ObjListFree(objs);
      return(I(p->obj, 2));
    }
  }
  ObjListFree(objs);
  return(NULL);
}

Obj *GroupCreate(Ts *ts, ObjList *members)
{
  Obj		*group;
  ObjList	*p;
  if ((group = GroupGet(ts, members))) return(group);
  group = ObjCreateInstance(N("group"), NULL);
  for (p = members; p; p = p->next) {
    if (ObjNA == p->obj || NULL == p->obj) continue;
    DbAssert(&TsRangeAlways, L(N("part-of"), p->obj, group, E));
    /* todo: This should be cpart-of to be more consistent. */
  }
  return(group);
}

/* End of file. */
