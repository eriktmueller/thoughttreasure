/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Discourse-level stylistic transformations on list objects.
 *
 * 19950319: begun
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "lextheta.h"
#include "repbasic.h"
#include "repcxt.h"
#include "repdb.h"
#include "repgroup.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "repstr.h"
#include "reptime.h"
#include "semaspec.h"
#include "semdisc.h"
#include "semgen1.h"
#include "semgen2.h"
#include "semparse.h"
#include "semstyle.h"
#include "semtense.h"
#include "synpnode.h"
#include "syntrans.h"
#include "synxbar.h"
#include "utildbg.h"
#include "utillrn.h"

/* HELPING FUNCTIONS */

ObjList *StyleTemporalTreeCreate()
{
  Obj	*obj;
  obj = L(N("concept"), E);
  TsRangeSetAlways(ObjToTsRange(obj));
  return(ObjListCreate(obj, NULL));
}

ObjList *StyleFindContainingNode(ObjList *objs, TsRange *tsr_con)
{
  ObjList	*p;
  for (p = objs; p; p = p->next) {
    if (TsRangeIsContained(ObjToTsRange(p->obj), tsr_con)) {
      return(p);
    }
  }
  return(NULL);
}

void StyleTemporalTreeAdd(ObjList *tree, Obj *con)
{
  ObjList	*prev_tree, *con_children, *tree_children, *p;
  TsRange	*tsr_con;
  prev_tree = tree;
  tsr_con = ObjToTsRange(con);
  while (1) {
    if (NULL == (tree = StyleFindContainingNode(tree->u.objs, tsr_con))) {
      con_children = tree_children = NULL;
      for (p = prev_tree->u.objs; p; p = p->next) {
        if (TsRangeIsContained(tsr_con, ObjToTsRange(p->obj))) {
          con_children = ObjListCreate(p->obj, con_children);
          con_children->u.objs = p->u.objs;
        } else {
          tree_children = ObjListCreate(p->obj, tree_children);
          tree_children->u.objs = p->u.objs;
        }
      }
      tree_children = ObjListCreate(con, tree_children);
      tree_children->u.objs = con_children;
      ObjListFree(prev_tree->u.objs);
      prev_tree->u.objs = tree_children;
      return;
    }
    prev_tree = tree;
  }
}

void StyleTemporalTreePrint2(FILE *stream, Obj *obj)
{
  ObjPrint1(stream, obj, NULL, 2, 1, 0, 1, 0);
  fputc(NEWLINE, stream);
}

void StyleTemporalTreePrint1(FILE *stream, ObjList *children, Obj *parent)
{
  ObjList	*p;
  if (children == NULL) return;
  if (parent) {
    StreamSep(stream);
    StyleTemporalTreePrint2(stream, parent);
    fprintf(stream, "-------->\n");
    for (p = children; p; p = p->next) {
      fprintf(stream, "     ");
      StyleTemporalTreePrint2(stream, p->obj);
    }
  }
  for (p = children; p; p = p->next) {
    StyleTemporalTreePrint1(stream, p->u.objs, p->obj);
  }
}

void StyleTemporalTreePrint(FILE *stream, ObjList *tree)
{
  StyleTemporalTreePrint1(stream, tree, NULL);
}

/* EXTERNAL FUNCTIONS */

/* in:     [location-of Puchl Vienna] [location-of Puchl country-AUT] [other]
 * result: [location-of Puchl [and Vienna country-AUT]] [other]
 * todo: this should work deep; cf country-AUT is her nationality since...
 */
ObjList *StyleMergeAnds(ObjList *in)
{
  int		len, merged_in;
  ObjList	*p, *q, *r;
  r = NULL;
  for (p = in; p; p = p->next) {
    merged_in = 0;
    for (q = r; q; q = q->next) {
      if (TsRangeEqual(ObjToTsRange(p->obj), ObjToTsRange(q->obj)) &&
          ((len = ObjLen(p->obj)) == ObjLen(q->obj))) {
        if (ObjSimilarList(p->obj, q->obj)) {
          merged_in = 1;
          break;
        } if (len == 3 &&
              I(p->obj, 0) == I(q->obj, 0) &&
              I(p->obj, 1) == I(q->obj, 1) &&
              I(p->obj, 2) != I(q->obj, 2)) {
          q->obj = L(I(p->obj,0),
                     I(p->obj,1),
                     ObjMakeAndList(I(q->obj,2), I(p->obj,2)),
                     E);
          ObjTsRangeSetFrom(q->obj, p->obj);
          merged_in = 1;
          break;
        } else if (len == 3 &&
                  I(p->obj, 0) == I(q->obj, 0) &&
                  I(p->obj, 1) != I(q->obj, 1) &&
                  I(p->obj, 2) == I(q->obj, 2)) {
          q->obj = L(I(p->obj,0),
                     ObjMakeAndList(I(q->obj,1), I(p->obj,1)),
                     I(p->obj,2),
                     E);
          ObjTsRangeSetFrom(q->obj, p->obj);
          merged_in = 1;
          break;
        }
      }
    }
    if (!merged_in) r = ObjListCreate(p->obj, r);
  }
  return(r);
}

void StyleListToTemporalTree(ObjList *in, /* RESULTS */ ObjList **r_tree,
                             ObjList **r_generality, ObjList **r_unterminated)
{
  ObjList	*p, *tree, *generality, *unterminated;
  TsRange	*tsr;
  tree = StyleTemporalTreeCreate();
  generality = unterminated = NULL;
  for (p = in; p; p = p->next) {
    tsr = ObjToTsRange(p->obj);
    if (TsRangeIsAlwaysIgnoreCx(tsr) || TsRangeIsNaIgnoreCx(tsr)) {
      generality = ObjListCreate(p->obj, generality);
    } else if ((!TsIsNa(&tsr->startts)) && TsIsNa(&tsr->stopts)) {
      unterminated = ObjListCreate(p->obj, unterminated);
    } else {
      StyleTemporalTreeAdd(tree, p->obj);
    }
  }
  *r_tree = tree;
  *r_generality = generality;
  *r_unterminated = unterminated;
}

/* todo: This does not work since ObjListSortByStartTs does not preserve
 * u.objs.
 */
void StyleTemporalTreeResort(ObjList *tree)
{
  ObjList	*old_children, *p;
  if (tree == NULL) return;
  old_children = tree->u.objs;
  tree->u.objs = ObjListSortByStartTs(tree->u.objs);
  ObjListFree(old_children);
  for (p = tree->u.objs; p; p = p->next) {
    StyleTemporalTreeResort(p->u.objs);
  }
}

/* todo: Organize information by predefined classes such as occupation-of
 * affiliation, and so on. The current algorithm produces strange results.
 */
ObjList *StyleReorganize1(Obj *parent, ObjList *children, ObjList *r,
                          Discourse *dc)
{
  Obj		*rel, *and_obj;
  ObjList	*p;
  if (parent && children) {
    rel = AspectTempRelGen(ObjToTsRange(parent), ObjToTsRange(children->obj),
                           DCNOW(dc));
    /* todo: Would the rel of the remaining children be the same? */
    and_obj = ObjListToAndObj(children);
    r = ObjListCreate(L(rel, parent, and_obj, E), r);
  } else if (parent) {
  /* todo */
    r = ObjListCreate(parent, r);
  }
  for (p = children; p; p = p->next) {
    r = StyleReorganize1(p->obj, p->u.objs, r, dc);
  }
  return(r);
}

ObjList *StyleReorganize(ObjList *in, Discourse *dc)
{
  ObjList	*in1;
  in1 = ObjListReverseDest(StyleMergeAnds(in));
  return(in1);
#ifdef notdef
  /* todo: The below needs work. */
  StyleListToTemporalTree(in1, &tree, &generality, &unterminated);
  if (DbgOn(DBGGENER, DBGDETAIL)) {
    Dbg(DBGGENER, DBGDETAIL, "StyleReorganize temporal tree:");
    StyleTemporalTreePrint(Log, tree);
  }
  r = StyleReorganize1(NULL, tree->u.objs, ObjListSortByStartTs(unterminated),
                       dc);
  r = ObjListAppendDestructive(generality, r);
  if (r == NULL) {
  /* todo: "She is where?" */
    r = in1;
  } else {
    ObjListFree(in1);
  }
  ObjListFree(tree);
  if (DbgOn(DBGGENER, DBGDETAIL)) {
    Dbg(DBGGENER, DBGDETAIL, "StyleReorganize results:");
    ObjListPrettyPrint(Log, r);
  }
  return(r);
#endif
}

/* End of file. */
