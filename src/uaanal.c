/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19950409: begun
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "repbasic.h"
#include "repchan.h"
#include "repcxt.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptime.h"
#include "semanaph.h"
#include "semdisc.h"
#include "synparse.h"
#include "synpnode.h"
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
#include "utillrn.h"

/* Example:
 * src_grid         map                 prefix         returns map augmented w/
 * ---------        -----               -------------  -------------------------
 * 20-rue-Drouot-4E Jim->Charlotte      Charlotte      Jim-bed->Charlotte-bed
 * sapins-15E       Martine->Charlotte  Charlotte      Martine-bed->
 *                                                                Charlotte-bed
 *                  Martine-Mom->Charlotte-Mom
 */
ObjList *UA_Analogy_Grid(Context *cx, Ts *ts, Obj *src_grid, ObjList *map,
                         char *prefix, Obj *tgt_polity)
{
  int		i, len, save_goa;
  char		tgt_grid_name[OBJNAMELEN];
  Obj		*tgt_grid_obj, *obj;
  ObjList	*p, *src_assertions, *src_physobjs, *q, *involving;
  Grid		*tgt_grid;

  if (DbgOn(DBGUA, DBGDETAIL)) {
    fprintf(Log, "GRID ANALOGY INPUT MAP <%s>\n", prefix);
    ObjListMapPrint(Log, map);
  }

  Dbg(DBGUA, DBGDETAIL, "Collect source assertions.");
  /* Collect source assertions. */
  src_assertions = DbRetrieveInvolving(&TsNA, NULL, src_grid, 0, NULL);
  src_physobjs = NULL;
  for (p = src_assertions; p; p = p->next) {
    for (i = 0, len = ObjLen(p->obj); i < len; i++) {
      if (ISA(N("human"), I(p->obj, i))) continue;	/* new */
      if (ISA(N("physical-object"), I(p->obj, i)) &&
          (!ISA(N("grid"), I(p->obj, i)))) {
        src_physobjs = ObjListAddIfNotIn(src_physobjs, I(p->obj, i));
        /* We can add directly to src_assertions which we are looping through
         * because we add to the head of the list: they will never been seen
         * by this loop.
         */
        involving = DbRetrieveInvolving(&TsNA, NULL, I(p->obj, i), 0, NULL);
        for (q = involving; q; q = q->next) {
          src_assertions = ObjListAddIfNotIn(src_assertions, q->obj);
        }
        ObjListFree(involving);
      }
    }
  }

  Dbg(DBGUA, DBGDETAIL, "Instantiate any dangling actors.");
  /* Instantiate any dangling actors. These cases should be handled before
   * getting here, since more intelligent instantiation is needed.
   */
  for (p = src_assertions; p; p = p->next) {
    for (i = 0, len = ObjLen(p->obj); i < len; i++) {
      if (IsActor(I(p->obj, i))) {
        if (!ObjListIn(I(p->obj, i), map)) {
          Dbg(DBGUA, DBGBAD, "<%s> not in map", M(I(p->obj, i)));
          map = ObjListMapAdd(map, I(p->obj, i),
                              ObjCreateInstance(N("human"), prefix));
        }
      }
    }
  }

  Dbg(DBGUA, DBGDETAIL, "Create target grid and add to map.");
  /* Create target grid and add to map. */
  StringCpy(tgt_grid_name, prefix, OBJNAMELEN);
  StringCat(tgt_grid_name, "-", OBJNAMELEN);
  StringCat(tgt_grid_name, M(src_grid), OBJNAMELEN);
  tgt_grid_obj = ObjGridCopy(src_grid, tgt_grid_name);
  tgt_grid = ObjToGrid(tgt_grid_obj);
  map = ObjListMapAdd(map, src_grid, tgt_grid_obj);
  
  Dbg(DBGUA, DBGDETAIL, "Add new instances of physical objects to map.");
  /* Add new instances of physical objects to map. */
  for (p = src_physobjs; p; p = p->next) {
    if (p->obj == src_grid) continue;
    if (ObjListMapLookup(map, p->obj)) continue;
    map = ObjListMapAdd(map, p->obj, ObjCreateInstance(ObjGetAParent(p->obj),
                        prefix));
  }

  if (DbgOn(DBGUA, DBGDETAIL)) {
    fprintf(Log, "GRID ANALOGY MAP <%s>\n", prefix);
    ObjListMapPrint(Log, map);
  }

  Dbg(DBGUA, DBGDETAIL, "Create and assert target assertions.");
  save_goa = GenOnAssert; 
  GenOnAssert = 0;
  /* Create and assert target assertions. */
  /* todo: Learning? But this is so massive.
   * Don't worry about massive, but only learn if this becomes sufficiently
   * different than the initial prototype to warrant saving.
   */
  for (p = src_assertions; p; p = p->next) {
    if (ISA(N("fanciful"), I(p->obj, 0)) ||
        ISA(N("level-of"), I(p->obj, 0)) ||
        ISA(N("orientation-of"), I(p->obj, 0)) ||
        ISA(N("polity-of"), I(p->obj, 0)) ||
        ISA(N("part-of"), I(p->obj, 0))) {
      continue;
    }
    if (ISA(N("at-grid"), I(p->obj, 0))) {
      if (ISA(N("wormhole"), I(p->obj, 1))) continue;
      if (src_grid != I(p->obj, 2)) {
      /* cf [at-grid Mrs-Jeanne-Püchl-STREET3375 Richelieu-Drouot-area ...] */
        continue;
      }
    }
    obj = ObjSubsts(p->obj, map, tgt_grid);
    ObjSetTsRangeContext(obj, cx);
    DbAssert1(obj);
  }
  if (tgt_polity) {
    obj = L(N("polity-of"), tgt_grid_obj, tgt_polity, E);
    ObjSetTsRangeContext(obj, cx);
    DbAssert1(obj);
  }
  GenOnAssert = save_goa;

  Dbg(DBGUA, DBGDETAIL, "UA_Analogy_Grid done.");
  return(map);
}

/* Create an object of class <physobj_class> in a new grid by analogy and
 * transport <actor> there. Transport because this grid is not connected
 * via wormholes so Trip planning won't succeed.
 * Examples:
 * goal_obj          physobj_class            actor
 * ----------------- ------------------------ ---------
 * [sleep Charlotte] bed                      Charlotte
 * [take-shower C.]  shower                   Charlotte
 * [grocer M.]       employee-side-of-counter M.
 */
Obj *UA_Analogy_ObjectNearActor(Context *cx, Ts *ts, Obj *goal_obj,
                                Obj *tgt_physobj_class, Obj *actor)
{
  Obj		*head, *nationality, *tgt_polity, *src_grid, *tgt_grid;
  Obj		*tgt_physobj, *mother, *father, *husband, *wife, *sex, *store;
  ObjList	*map, *p, *children;
  head = I(goal_obj, 0);

  if (ISA(N("sleep"), head) ||
      ISA(N("take-shower"), head) ||
      ISA(N("grocer"), head)) {

    tgt_polity = NULL;
    if ((nationality = DbGetRelationValue(ts, NULL, N("nationality-of"), actor,
                                          NULL))) {
      if (N("country-FRA") == nationality) {
        tgt_polity = N("Paris");
      } else if (N("country-USA") == nationality) {
        tgt_polity = N("city-New-York-NY-USA");
      } else if (N("country-GBR") == nationality) {
        tgt_polity = N("city-London-GBR");
      } else if (N("country-AUT") == nationality) {
        tgt_polity = N("city-Vienna-AUT");
      }
    }

    sex = DbGetEnumValue(ts, NULL, N("sex"), actor, NULL);
    mother = R1EI(2, ts, L(N("mother-of"), actor, ObjWild, E));
    father = R1EI(2, ts, L(N("father-of"), actor, ObjWild, E));
    children = REI(2, ts, L(N("child-of"), actor, ObjWild, E));
    husband = R1EI(2, ts, L(N("husband-of"), actor, ObjWild, E));
    wife = R1EI(2, ts, L(N("wife-of"), actor, ObjWild, E));

    map = NULL;
    if (ISA(N("grocer"), head)) {
    /* Grocer. Base it on small-city-food-store1. */
      store = R1DI(1, ts, L(N("owner-of"), N("store"), actor, E), 1);
      if (sex == N("male")) {
        map = ObjListMapAdd(map, N("Henri-Bidault"), actor);
        if (wife) map = ObjListMapAdd(map, N("Florence-Bidault"), wife);
      } else {
        map = ObjListMapAdd(map, N("Florence-Bidault"), actor);
        if (husband) map = ObjListMapAdd(map, N("Henri-Bidault"), husband);
      }
      if (store) map = ObjListMapAdd(map, N("small-city-food-store1"), store);
      src_grid = N("grid-small-city-food-store1");
    } else if (mother || father || children) {
    /* Family living situation. Base it on Martine story. */
      if (children) {
      /* <actor> is parent */
        if (sex == N("male")) map = ObjListMapAdd(map, N("Martine-Dad"), actor);
        else map = ObjListMapAdd(map, N("Martine-Mom"), actor);
        map = ObjListMapAdd(map, N("Martine"), children->obj);
        	/* todo: >1 child -- need to add to Sapins paradigm */
      } else {
      /* <actor> is child */
        map = ObjListMapAdd(map, N("Martine"), actor);
        if (mother) map = ObjListMapAdd(map, N("Martine-Mom"), mother);
        if (father) map = ObjListMapAdd(map, N("Martine-Dad"), father);
      }
      src_grid = N("sapins-15E");
    } else {
    /* Single person. Base it on Jim. */
      map = ObjListMapAdd(map, N("Jim"), actor);
      src_grid = N("20-rue-Drouot-4E");
    }

    map = UA_Analogy_Grid(cx, ts, src_grid, map, M(actor), tgt_polity);
    tgt_grid = ObjListMapLookup(map, src_grid);
    tgt_physobj = NULL;
    for (p = map; p; p = p->next) {
      if (ISA(tgt_physobj_class, p->obj)) {
        tgt_physobj = p->u.tgt_obj;
        break;
      }
    }
    /* Transport target actors besides <actor> into <tgt_grid> */
    for (p = map; p; p = p->next) {
      if (p->u.tgt_obj == actor) continue;
      if (!IsActor(p->u.tgt_obj)) continue;
      UA_Space_ActorTransportNear(cx, ts, p->u.tgt_obj, tgt_grid, NULL);
    }

    if (tgt_physobj == NULL) {
      Dbg(DBGUA, DBGBAD, "<%s> not found", M(tgt_physobj_class));
      return(NULL);
    }

    /* Transport <actor> into <tgt_grid> near <tgt_physobj> */
    UA_Space_ActorTransportNear(cx, ts, actor, tgt_grid, tgt_physobj);

    return(tgt_physobj);
  }
  return(NULL);
}

/* End of file */
