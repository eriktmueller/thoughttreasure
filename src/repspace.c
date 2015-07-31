/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940129: begun
 * 19940525: added Intergrid path search
 */

#include "tt.h"
#include "repbasic.h"
#include "repdb.h"
#include "repgrid.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "repstr.h"
#include "reptime.h"
#include "utildbg.h"

/******************************************************************************
 * Coord
 ******************************************************************************/

Bool CoordParse(char *text, /* RESULTS */ Float *lat, Float *lng)
{
  char	*s, latdeg[WORDLEN], latmin[WORDLEN], latdir;
  char	longdeg[WORDLEN], longmin[WORDLEN], longdir;
  int	ilatdeg, ilatmin, ilongdeg, ilongmin;
  s = text;
  if (s[0] == TERM) {
    Dbg(DBGSPACE, DBGBAD, "missing latdeg: <%s>", text);
    return(0);
  }
  s = StringReadTo(s, latdeg, WORDLEN, '.', TERM, TERM);
  if (s[0] == TERM) {
    Dbg(DBGSPACE, DBGBAD, "missing latmin: <%s>", text);
    return(0);
  }
  s = StringReadTo(s, latmin, WORDLEN, 'N', 'S', TERM);
  latdir = *(s-1);
  if (s[0] == TERM) {
    Dbg(DBGSPACE, DBGBAD, "missing longdeg: <%s>", text);
    return(0);
  }
  s = StringReadTo(s, longdeg, WORDLEN, '.', TERM, TERM);
  if (s[0] == TERM) {
    Dbg(DBGSPACE, DBGBAD, "missing longmin: <%s>", text);
    return(0);
  }
  s = StringReadTo(s, longmin, WORDLEN, 'E', 'W', TERM);
  longdir = *(s-1);
  if (s[0] == TERM) Dbg(DBGSPACE, DBGBAD, "missing stem of 'coor': <%s>", text);
  if (!streq(s, "coor")) Dbg(DBGSPACE, DBGBAD, "stem not 'coor': <%s>", text);
  if (INTNA == (ilatdeg = IntParse(latdeg, -1))) {
    Dbg(DBGSPACE, DBGBAD, "trouble parsing latdeg: <%s>", text); return(0); 
  }
  if (INTNA == (ilatmin = IntParse(latmin, -1))) {
    Dbg(DBGSPACE, DBGBAD, "trouble parsing latmin: <%s>", text); return(0); 
  }
  if (INTNA == (ilongdeg = IntParse(longdeg, -1))) {
    Dbg(DBGSPACE, DBGBAD, "trouble parsing longdeg: <%s>", text); return(0); 
  }
  if (INTNA == (ilongmin = IntParse(longmin, -1))) {
    Dbg(DBGSPACE, DBGBAD, "trouble parsing longmin: <%s>", text); return(0); 
  }
  *lat = ((Float)ilatdeg) + ((Float)ilatmin)/60.0;
  *lng = ((Float)ilongdeg) + ((Float)ilongmin)/60.0;
  if (latdir == 'S') {
    *lat = -(*lat);
  } else if (latdir != 'N') {
    Dbg(DBGSPACE, DBGBAD, "latdir <%s> not N or S: <%s>", latdir, text);
    return(0); 
  }
  if (longdir == 'W') {
    *lng = -(*lng);
  } else if (longdir != 'E') {
    Dbg(DBGSPACE, DBGBAD, "longdir <%s> not E or W: <%s>", longdir, text);
    return(0); 
  }
  return(1);
}

/* Latitude and longitude in degrees.
 * North represented as positive. South negative.
 * East represented as positive. West negative.
 */
Float CoordPlanetDistance(Float lat1, Float long1, Float lat2, Float long2,
                          Float radius)
{
  lat1 = (TTPI/2.0)*(1.0-lat1/90.0);
  lat2 = (TTPI/2.0)*(1.0-lat2/90.0);
  long1 = (TTPI/180.0)*long1;
  long2 = (TTPI/180.0)*long2;
  return(radius*acos(sin(lat1)*cos(long1)*sin(lat2)*cos(long2)+
                sin(lat1)*sin(long1)*sin(lat2)*sin(long2)+
                cos(lat1)*cos(lat2)));
}

Float CoordEarthDistance(Float lat1, Float long1, Float lat2, Float long2)
{
  return(CoordPlanetDistance(lat1, long1, lat2, long2, EARTHRADIUS));
}

Float CoordEuclideanDistance(Float x0, Float y0, Float x1, Float y1)
{
  return(pow(pow(x1 - x0, 2.0) + pow(y1 - y0, 2.0), 0.5));
}

Float CoordEuclideanDistance3(Float x0, Float y0, Float z0,
                              Float x1, Float y1, Float z1)
{
  return(pow(pow(x1 - x0, 2.0) + pow(y1 - y0, 2.0) + pow(z1 - z0, 2.0), 0.5));
}

Float CoordDistance(Obj *coord1, Obj *coord2)
{
  Float lat1, lat2, long1, long2;
  if (!CoordParse(M(coord1), &lat1, &long1)) return(FLOATPOSINF);
  if (!CoordParse(M(coord2), &lat2, &long2)) return(FLOATPOSINF);
  return(CoordEarthDistance(lat1, long1, lat2, long2));
}

/******************************************************************************
 * POLITY
 ******************************************************************************/

/* Example:
 * polity: &9arr
 * class: city
 * result: Paris
 */
Obj *PolityToSuperpolity(Ts *ts, TsRange *tsr, Obj *polity, Obj *class)
{
  ObjList	*objs, *p;
  Obj		*obj;
  if (ISA(class, polity)) return(polity);
  for (p = objs =
        DbRetrieveExact(ts, tsr, L(N("cpart-of"), polity, ObjWild, E), 1);
       p; p = p->next) {
    if ((obj = PolityToSuperpolity(ts, tsr, I(p->obj, 2), class))) {
      ObjListFree(objs);
      return(obj);
    }
  }
  ObjListFree(objs);
  return(NULL);
}

/* Example:
 * polity: "9arr"
 * enclosing_polity: "country-FRA"
 * result: 1
 */
Bool PolityContained(Ts *ts, TsRange *tsr, Obj *polity, Obj *enclosing_polity)
{
  ObjList	*objs, *p;
  if (polity == enclosing_polity) return(1);
  for (p = objs = DbRetrieveExact(ts, tsr, L(N("cpart-of"), polity, ObjWild, E),
                                  1);
       p; p = p->next) {
    if (PolityContained(ts, tsr, I(p->obj, 2), enclosing_polity)) {
      ObjListFree(objs);
      return(1);
    }
  }
  ObjListFree(objs);
  return(0);
}

Obj *PolityCoord(Ts *ts, TsRange *tsr, Obj *polity)
{
  ObjList	*objs, *p;
  Obj		*obj;
  if ((obj = R1EIB(2, ts, tsr, L(N("coordinates-of"), polity, ObjWild, E)))) {
    return(obj);
  }
  for (p = objs = REB(ts, tsr, L(N("cpart-of"), polity, ObjWild, E));
       p; p = p->next) {
    if ((obj = PolityCoord(ts, tsr, I(p->obj, 2)))) {
      ObjListFree(objs);
      return(obj);
    }
  }
  ObjListFree(objs);
  return(NULL);
}

Float PolityDistance(Ts *ts, TsRange *tsr, Obj *polity1, Obj *polity2)
{
  Obj *coord1, *coord2;
  coord1 = PolityCoord(ts, tsr, polity1);
  coord2 = PolityCoord(ts, tsr, polity2);
  if (coord1 == NULL || coord2 == NULL) return(FLOATPOSINF);
  if (coord1 == coord2) return(1000.0); /* todo */
  return(CoordDistance(coord1, coord2));
}

/******************************************************************************
 * Space
 ******************************************************************************/

Float SpaceDistance2(Ts *ts, TsRange *tsr, Obj *polity1, Obj *grid1,
                     GridCoord row1, GridCoord col1, Obj *polity2, Obj *grid2,
                     GridCoord row2, GridCoord col2)
{
  if (grid1 && grid2 && grid1 == grid2)
    return(GridDistance(grid1, row1, col1, row2, col2));
  if (polity1 && polity2 && polity1 == polity2)
    return(1000.0); /* todo: calculate distance via wormholes */
  if (polity1 && polity2)
    return(PolityDistance(ts, tsr, polity1, polity2));
  return(FLOATPOSINF);
}

Float SpaceDistance1(Ts *ts, TsRange *tsr, Obj *polity1, Obj *grid1,
                     GridCoord row1, GridCoord col1, Obj *obj2)
{
  Obj		*polity2;
  Obj		*grid2;
  GridCoord	row2, col2;
  if (!SpaceLocateObject(ts, tsr, obj2, grid1, 0, &polity2, &grid2, &row2,
                         &col2)) {
    return(FLOATPOSINF);
  }
  return(SpaceDistance2(ts, tsr, polity1, grid1, row1, col1, polity2, grid2,
                        row2, col2));
}

Float SpaceDistance(Ts *ts, TsRange *tsr, Obj *obj1, Obj *obj2)
{
  Obj			*polity1;
  Obj			*grid1;
  GridCoord		row1, col1;
  if (!SpaceLocateObject(ts, tsr, obj1, NULL, 0, &polity1, &grid1, &row1,
                         &col1)) {
    return(FLOATPOSINF);
  }
  return(SpaceDistance1(ts, tsr, polity1, grid1, row1, col1, obj2));
}

void SpaceAtGridGet(Obj *ga, /* RESULTS */ Obj **polityp, Obj **gridp,
                    GridCoord *rowp, GridCoord *colp)
{
  Grid		*gridstate;
  GridSubspace	*gridsubspace;
  *polityp = SpaceGridToPolity(NULL, ObjToTsRange(ga), I(ga, 2));
  *gridp = I(ga, 2);
  gridsubspace = ObjToGridSubspace(I(ga, 3));
  if (gridsubspace == NULL || gridsubspace->len == 0) {
    Dbg(DBGSPACE, DBGBAD, "empty GridSubspace", M(I(ga, 3)));
    *rowp = INTNA;
    *colp = INTNA;
    return;
  }
  if (gridsubspace->len > 1) {
    /* todo: Drivable? Startts? */
    gridstate = GridStateGet(&ObjToTsRange(ga)->startts, *gridp, N("walkable"));
    if (GridFindAccessibleBorder(gridsubspace, gridstate, rowp, colp)) {
      return;
    }
  }
  *rowp = gridsubspace->rows[0];
  *colp = gridsubspace->cols[0];
  return;
}

/******************************************************************************
 * SPACE/POLITY
 ******************************************************************************/

Obj *SpaceGridToPolity(Ts *ts, TsRange *tsr, Obj *grid)
{
  Obj *o;
  if (!(o = R1EIB(2, ts, tsr, L(N("polity-of"), grid, ObjWild, E)))) {
    Dbg(DBGSPACE, DBGDETAIL, "grid <%s> is missing polity-of", M(grid));
    return(NULL);
  }
  return(o);
}

/* Example:
 * grid: &20-rue-Drouot-4E
 * polity_class: city
 * result: Paris
 */
Obj *SpaceGridToPolityClass(Ts *ts, TsRange *tsr, Obj *grid, Obj *polity_class)
{
  Obj *smallest_polity;
  if (!(smallest_polity = SpaceGridToPolity(ts, tsr, grid))) {
    return(NULL);
  }
  return(PolityToSuperpolity(ts, tsr, smallest_polity, polity_class));
}

Bool SpaceGridInPolity(Ts *ts, TsRange *tsr, Obj *grid, Obj *polity)
{
  Obj *smallest_polity;
  if (!(smallest_polity = SpaceGridToPolity(ts, tsr, grid))) {
    return(0);
  }
  return(PolityContained(ts, tsr, smallest_polity, polity));
}

Bool SpaceInPolity(Ts *ts, TsRange *tsr, Obj *obj, Obj *polity,
                   /* RESULTS */ TsRange *tsr_r)
{
  Obj			*encl_polity, *encl_grid;
  ObjList		*ga, *gas;
  GridCoord		encl_row, encl_col;
  if (!(gas = SpaceLocateObject1(ts, tsr, obj, NULL, 0))) {
    return(0);
  }
  for (ga = gas; ga; ga = ga->next) {
    SpaceAtGridGet(ga->obj,  &encl_polity, &encl_grid, &encl_row, &encl_col);
    if (PolityContained(ts, tsr, polity, encl_polity)) {
      if (tsr_r) TsRangeSetAlways(tsr_r);	/* todo */
      ObjListFree(gas);
      return(1);
    }
  }
  ObjListFree(gas);
  return(0);
}

/******************************************************************************
 * ENCLOSING OBJECTS
 ******************************************************************************/

Obj *SpaceFindEncloseMake(TsRange *ga_tsr, Obj *physobj, Obj *loc)
{
  Obj		*obj;
  TsRange	*tsr;
  if (physobj) {
    obj = L(N("location-of"), physobj, loc, E);
    tsr = ObjToTsRange(obj);
    *tsr = *ga_tsr;
    return(obj);
  } else {
    /* todo: Should really store ts, but symbol objects don't allow this. */
    return(loc);
  }
}

ObjList *SpaceFindEncloseObject(Ts *ts, TsRange *tsr, TsRange *ga_tsr,
                                Obj *physobj, Obj *enclose_class,
                                Obj *encl_grid, GridCoord encl_row,
                                GridCoord encl_col, Obj *enclose_restriction,
                                ObjList *r)
{
  ObjList	*objs, *p;
  if (enclose_restriction &&
      (!ISAP(enclose_restriction, enclose_class))) {
    return(r);
  }
  objs = RDB(ts, tsr, L(N("at-grid"), enclose_class, encl_grid, ObjWild, E), 1);
  for (p = objs; p; p = p->next) {
    if (GridSubspaceIn(ObjToGridSubspace(I(p->obj, 3)), encl_row, encl_col)) {
      r = ObjListCreate(SpaceFindEncloseMake(ga_tsr, physobj, I(p->obj, 1)), r);
    }
  }
  ObjListFree(objs);
  return(r);
}

ObjList *SpaceFindEnclosePolity(Ts *ts, TsRange *tsr, TsRange *ga_tsr,
                                Obj *physobj, Obj *polity_class, Obj *grid,
                                Obj *enclose_restriction, ObjList *r)
{
  Obj		*gridpolity, *superpolity;
  if (enclose_restriction && (!ISAP(enclose_restriction, polity_class))) {
    return(r);
  }
  gridpolity = SpaceGridToPolity(ts, tsr, grid);
  if ((superpolity = PolityToSuperpolity(ts, tsr, gridpolity, polity_class))) {
    r = ObjListCreate(SpaceFindEncloseMake(ga_tsr, physobj, superpolity), r);
  }
  return(r);
}

ObjList *SpaceFindEnclose1(Ts *ts, TsRange *tsr, TsRange *ga_tsr, Obj *physobj,
                           Obj *encl_grid, GridCoord encl_row,
                           GridCoord encl_col, Obj *enclose_restriction,
                           ObjList *r)
{
  r = SpaceFindEnclosePolity(ts, tsr, ga_tsr, physobj, N("universe"),
                             encl_grid, enclose_restriction, r);
  r = SpaceFindEnclosePolity(ts, tsr, ga_tsr, physobj, N("galaxy"),
                             encl_grid, enclose_restriction, r);
  r = SpaceFindEnclosePolity(ts, tsr, ga_tsr, physobj, N("solar-system"),
                             encl_grid, enclose_restriction, r);
  r = SpaceFindEnclosePolity(ts, tsr, ga_tsr, physobj, N("planet"),
                             encl_grid, enclose_restriction, r);
  r = SpaceFindEnclosePolity(ts, tsr, ga_tsr, physobj, N("continent"),
                             encl_grid, enclose_restriction, r);
  r = SpaceFindEnclosePolity(ts, tsr, ga_tsr, physobj, N("country"), encl_grid,
                             enclose_restriction, r);
  r = SpaceFindEnclosePolity(ts, tsr, ga_tsr, physobj, N("city"), encl_grid,
                             enclose_restriction, r);
  r = SpaceFindEnclosePolity(ts, tsr, ga_tsr, physobj, N("city-subdivision"),
                             encl_grid, enclose_restriction, r);
  r = SpaceFindEnclosePolity(ts, tsr, ga_tsr, physobj, N("city-subsubdivision"),
                             encl_grid, enclose_restriction, r);
  r = SpaceFindEncloseObject(ts, tsr, ga_tsr, physobj, N("roadway"), encl_grid,
                             encl_row, encl_col, enclose_restriction, r);
  r = SpaceFindEncloseObject(ts, tsr, ga_tsr, physobj, N("building"), encl_grid,
                             encl_row, encl_col, enclose_restriction, r);
  r = SpaceFindEncloseObject(ts, tsr, ga_tsr, physobj, N("apartment"),
                             encl_grid, encl_row, encl_col,
                             enclose_restriction, r);
  r = SpaceFindEncloseObject(ts, tsr, ga_tsr, physobj, N("room"), encl_grid,
                             encl_row, encl_col, enclose_restriction, r);
  return(r);
}

/* Example return values: physobj non NULL: [location-of foyer23 apt89]
 *                        physobj NULL: apt89
 */
ObjList *SpaceFindEnclose(Ts *ts, TsRange *tsr, Obj *physobj,
                          Obj *enclose_restriction, ObjList *r)
{
  GridCoord		encl_row, encl_col;
  Obj			*encl_polity, *encl_grid;
  ObjList		*gas, *ga;
  if (enclose_restriction == N("location")) enclose_restriction = NULL;
  if (enclose_restriction == NULL) {
    enclose_restriction = 
        L(N("and"),
          N("physical-object"),
          L(N("not"), N("celestial-object"), E), E);
  }
#ifndef MICRO
  if (ts == NULL && TsEQ(&tsr->startts, &tsr->stopts)) {
  /* Where was Lin at 10:13:19? (the time is parsed into a range) */
    ts = &tsr->stopts;
    tsr = NULL;
  }
#endif
  if (!(gas = SpaceLocateObject1(ts, tsr, physobj, NULL, 0))) {
    Dbg(DBGSPACE, DBGDETAIL, "SpaceFindEnclose: unable to locate <%s>",
        M(physobj));
    return(r);
  }
  for (ga = gas; ga; ga = ga->next) {
    SpaceAtGridGet(ga->obj,  &encl_polity, &encl_grid, &encl_row, &encl_col);
    r = SpaceFindEnclose1(ts, tsr, ObjToTsRange(ga->obj), physobj, encl_grid,
                          encl_row, encl_col, enclose_restriction, r);
  }
  ObjListFree(gas);
  return(r);
}

/* Called by the prover. */
Bool SpaceEncloses(Ts *ts, TsRange *tsr, Obj *encloser, Obj *enclosed,
                   /* RESULTS */ TsRange *tsr_r)
{
  Obj		*encl_polity, *encl_grid;
  ObjList	*ga, *objs, *p, *gas;
  GridCoord	encl_row, encl_col;
  if (ObjIsNa(encloser) || ObjIsVar(encloser)) return(0);
  if (ObjIsNa(enclosed) || ObjIsVar(enclosed)) return(0);
  if (!(gas = SpaceLocateObject1(ts, tsr, enclosed, NULL, 0))) {
    return(0);
  }
  for (ga = gas; ga; ga = ga->next) {
    SpaceAtGridGet(ga->obj,  &encl_polity, &encl_grid, &encl_row, &encl_col);
    objs = DbRetrieveExact(ts, tsr, L(N("at-grid"), encloser, encl_grid,
                           ObjWild, E), 1);
    for (p = objs; p; p = p->next) {
      if (GridSubspaceIn(ObjToGridSubspace(I(p->obj, 3)), encl_row, encl_col)) {
        if (tsr_r) *tsr_r = *ObjToTsRange(ga->obj);
        ObjListFree(objs);
        ObjListFree(gas);
        return(1);
      }
    }
    ObjListFree(objs);
  }
  ObjListFree(gas);
  return(0);
}

/******************************************************************************
 * LOCATING OBJECTS
 * todo: These routines should be merged into one general routine
 * with many options.
 ******************************************************************************/

ObjList *SpaceLocateObject1(Ts *ts, TsRange *tsr, Obj *obj, Obj *gridprefer,
                            int desc_ok)
{
  ObjList		*r, *objs, *p;

  if (desc_ok) {
    if ((r = DbRetrieveDesc(ts, tsr, L(N("at-grid"),
                                       obj,
                                       gridprefer ? gridprefer : ObjWild,
                                       ObjWild,
                                       E), 1, 1, 1))) {
      return(r);
    }
    if (gridprefer &&
        (r = DbRetrieveDesc(ts, tsr, L(N("at-grid"), obj, ObjWild, ObjWild, E),
                            1, 1, 1))) {
      return(r);
    }
  } else {
    if ((r = DbRetrieveExact(ts, tsr, L(N("at-grid"),
                                        obj,
                                        gridprefer ? gridprefer : ObjWild,
                                        ObjWild,
                                        E), 1))) {
      return(r);
    }
    if (gridprefer &&
        (r = DbRetrieveExact(ts, tsr, L(N("at-grid"), obj, ObjWild, ObjWild, E),
                             1))) {
      return(r);
    }
  }
  for (p = objs = REB(ts, tsr, L(N("cpart-of"), obj, ObjWild, E)); p;
       p = p->next) {
    if ((r = SpaceLocateObject1(ts, tsr, I(p->obj, 2), gridprefer, desc_ok))) {
      ObjListFree(objs);
      return(r);
    }
  }
  ObjListFree(objs);
  for (p = objs = REB(ts, tsr, L(N("inside"), obj, ObjWild, E)); p;
       p = p->next) {
    if ((r = SpaceLocateObject1(ts, tsr, I(p->obj, 2), gridprefer, desc_ok))) {
      ObjListFree(objs);
      return(r);
    }
  }
  ObjListFree(objs);
  for (p = objs = REB(ts, tsr, L(N("wearing-of"), ObjWild, obj, E)); p;
       p = p->next) {
    if ((r = SpaceLocateObject1(ts, tsr, I(p->obj, 1), gridprefer, desc_ok))) {
      ObjListFree(objs);
      return(r);
    }
  }
  ObjListFree(objs);
  return(NULL);
}

Bool SpaceFindResidence(Ts *ts, TsRange *tsr, Obj *human, /* RESULTS */
                        Obj **residence, Obj **polityp, Obj **gridp,
                        GridCoord *rowp, GridCoord *colp)
{
  ObjList	*residences, *p;
  residences = REB(ts, tsr, L(N("residence-of"), human, ObjWild, E));
  for (p = residences; p; p = p->next) {
    if (SpaceLocateObject(ts, tsr, I(p->obj, 2), NULL, 0, polityp, gridp,
                          rowp, colp)) {
      /* todo: More than just being in some grid, we want this to be
       * an apartment-level grid, not a map-level grid like country-FRA.
       */
      if (residence) *residence = I(p->obj, 2);
      ObjListFree(residences);
      return(1);
    }
  }   
  ObjListFree(residences);
  Dbg(DBGSPACE, DBGDETAIL, "residence-of <%s> not found", M(human));
  return(0);
}

/* todo: This modifies the database, so it should really be moved
 * to UA_Space.
 */
Bool SpaceLocateHuman(Ts *ts, TsRange *tsr, Obj *human, /* RESULTS */
                      Obj **residence, Obj **polityp, Obj **gridp,
                      GridCoord *rowp, GridCoord *colp)
{
  Ts	*ts1;
  if (SpaceFindResidence(ts, tsr, human, NULL, polityp, gridp,
                         rowp, colp)) {
    if (ts) {
      ts1 = ts;
    } else if (tsr) {
      ts1 = &tsr->startts;
    } else {
      Dbg(DBGSPACE, DBGBAD, "SpaceLocateHuman 1");
      return(0);
    }
    TE(ts1, L(N("at-grid"), human, ObjWild, ObjWild, E));
    AS(ts1, 0L, ObjAtGridCreate(human, *gridp, *rowp, *colp));
    return(1);
  }
  return(0);
}

Bool SpaceLocateObject(Ts *ts, TsRange *tsr, Obj *obj, Obj *gridprefer,
                       int desc_ok, /* RESULTS */ Obj **polityp, Obj **gridp,
                       GridCoord *rowp, GridCoord *colp)
{
  ObjList	*gas;
  if ((gas = SpaceLocateObject1(ts, tsr, obj, gridprefer, desc_ok))) {
    /* todo: Favor the most detailed grid. */
    SpaceAtGridGet(gas->obj, polityp, gridp, rowp, colp);
    return(1);
  }
  if (ISAP(N("human"), obj) && ObjIsConcrete(obj)) {
    if (SpaceLocateHuman(ts, tsr, obj, NULL, polityp, gridp, rowp, colp)) {
      return(1);
    }
  }
  if (DbgOn(DBGSPACE, DBGHYPER)) {
    fprintf(Log, "unable to locate <%s> %d ", M(obj), desc_ok);
    if (gridprefer) fprintf(Log, "<%s> ", M(gridprefer));
    if (ts) {
      TsPrint(Log, ts);
      fputc(SPACE, Log);
    }
    if (tsr) {
      TsRangePrint(Log, tsr);
      fputc(SPACE, Log);
    }
    fputc(NEWLINE, Log);
  }
  return(0);
}

Bool SpaceObjectIsLocated(Ts *ts, TsRange *tsr, Obj *obj)
{
  Obj			*polity, *grid;
  GridCoord		row, col;
  return(SpaceLocateObject(ts, tsr, obj, NULL, 0, &polity, &grid, &row, &col));
}

Obj *SpaceCountryOfObject(Ts *ts, TsRange *tsr, Obj *obj)
{
  Obj			*polity, *grid, *r;
  GridCoord		row, col;
  if (!SpaceLocateObject(ts, tsr, obj, NULL, 0, &polity, &grid, &row, &col)) {
    return(NULL);
  }
  r = PolityToSuperpolity(ts, tsr, polity, N("country"));
  Dbg(DBGSPACE, DBGDETAIL, "<%s> in <%s>", M(obj), M(r), E);
  return(r);
}

/* todo: Does not find parts of objects. */
ObjList *SpaceFindAll(Ts *ts, TsRange *tsr, Obj *desired)
{
  ObjList	*r, *objs;
  r = RDIB(1, ts, tsr, L(N("at-grid"), desired, ObjWild, ObjWild, E), 1);
  if ((objs = RDIB(1, ts, tsr, L(N("polity-of"), desired, ObjWild, E), 1))) {
    if (r) ObjListLast(r)->next = objs;
    else r = objs;
  }
  if ((objs = RDIB(1, ts, tsr, L(N("inside"), desired, ObjWild, E), 1))) {
    if (r) ObjListLast(r)->next = objs;
    else r = objs;
  }
  return(r);
}

ObjList *SpaceGetAllInGrid(Ts *ts, TsRange *tsr, Obj *grid)
{
  ObjList	*r, *objs, *p;
  r = NULL;
  objs = DbRetrieveExact(ts, tsr, L(N("at-grid"), ObjWild, grid, ObjWild, E),
                         1);
  for (p = objs; p; p = p->next) {
    r = ObjListCreate(I(p->obj, 1), r);
  }
  ObjListFree(objs);
  return(r);
}

ObjList *SpaceFindWithinRadiusOwned1(Ts *ts, TsRange *tsr, Obj *desired,
                                     Obj *nearx, Float radius, Obj *owner)
{
  Obj			*polity1, *grid1;
  ObjList		*objs, *p, *r;
  GridCoord		row1, col1;
  Float			dist;
  r = NULL;
  if (!SpaceLocateObject(ts, tsr, nearx, NULL, 0, &polity1, &grid1, &row1,
                         &col1)) {
    return(NULL);
  }
  if (desired) {
    objs = SpaceFindAll(ts, tsr, desired);
  } else {
    objs = SpaceGetAllInGrid(ts, tsr, grid1);
  }
  for (p = objs; p; p = p->next) {
    if (owner && !REB(ts, tsr, L(N("owner-of"), p->obj, owner, E))) continue;
    dist = SpaceDistance1(ts, tsr, polity1, grid1, row1, col1, p->obj);
    Dbg(DBGSPACE, DBGHYPER, "distance between <%s> <%s> = %g", M(nearx),
        M(p->obj), dist);
    if (dist <= radius) r = ObjListCreate(p->obj, r);
  }
  ObjListFree(objs);
  return(r);
}

ObjList *SpaceFindWithinRadiusOwned(Ts *ts, TsRange *tsr, Obj *desired,
                                    Obj *nearx, Float radius, Obj *owner)
{
  ObjList	*r;
  int		i;
  for (i = 0; i < 4; i++) {
    if ((r = SpaceFindWithinRadiusOwned1(ts, tsr, desired, nearx, radius,
                                         owner))) {
      return(r);
    }
    radius = radius * 2.0;
  }
  if (owner) {
    Dbg(DBGSPACE, DBGDETAIL, "no <%s> of <%s> found in radius %g near <%s>",
        M(desired), M(owner), radius, M(nearx), E);
  } else {
    Dbg(DBGSPACE, DBGDETAIL, "no <%s> found in radius %g near <%s>",
        M(desired), radius, M(nearx), E);
  }
  return(NULL);
}

/* <desired> is abstract. <nearx> is concrete. */
Obj *SpaceFindNearestOwnedInPolity(Ts *ts, TsRange *tsr, Obj *desired,
                                   Obj *nearx, Obj *owner, Obj *polity)
{
  Obj			*polity1, *polity2, *grid1, *grid2, *nearest;
  ObjList		*objs, *p;
  GridCoord		row1, col1, row2, col2;
  Float			mindist, dist;
  if (nearx) {
    if (!SpaceLocateObject(ts, tsr, nearx, NULL, 0, &polity1, &grid1, &row1,
                           &col1)) {
      return(NULL);
    }
  } else {
    grid1 = NULL;
  }
  nearest = NULL;
  mindist = FLOATPOSINF;
  for (p = objs = SpaceFindAll(ts, tsr, desired); p; p = p->next) {
    if (owner && !IsOwnerOf(ts, tsr, p->obj, owner)) {
      continue;
    }
    if (!SpaceLocateObject(ts, tsr, p->obj, grid1, 0, &polity2, &grid2, &row2,
                           &col2)) {
      if (polity) continue;
      dist = FLOATPOSINF;
    } else {
      if (polity && (!SpaceGridInPolity(ts, tsr, grid2, polity))) {
        continue;
      }
      if (nearx) {
        dist = SpaceDistance2(ts, tsr, polity1, grid1, row1, col1, polity2,
                              grid2, row2, col2);
      } else dist = 1000.0;
    }
    Dbg(DBGSPACE, DBGHYPER, "distance between <%s> <%s> = %g", M(nearx),
        M(p->obj), dist);
    if (dist < mindist) {
      nearest = p->obj;
      mindist = dist;
    }
  }
  ObjListFree(objs);
  if (nearest == NULL) {
    if (owner) {
      Dbg(DBGSPACE, DBGDETAIL, "no <%s> of <%s> found near <%s>",
          M(desired), M(owner), M(nearx));
    } else {
      Dbg(DBGSPACE, DBGDETAIL, "no <%s> found near <%s>", M(desired), M(nearx));
    }
  }
  return(nearest);
}

/* <desired> is abstract. <nearx> is concrete. */
Obj *SpaceFindNearestOwned(Ts *ts, TsRange *tsr, Obj *desired, Obj *nearx,
                           Obj *owner)
{
  return(SpaceFindNearestOwnedInPolity(ts, tsr, desired, nearx, owner, NULL));
}

/* todo: This should look in just the nearby grid first. Then others
 * if nothing found.
 */
Obj *SpaceFindNearest(Ts *ts, TsRange *tsr, Obj *desired, Obj *nearx)
{
  return(SpaceFindNearestOwned(ts, tsr, desired, nearx, NULL));
}

Obj *SpaceFindNearestOwnedIfPossible(Ts *ts, TsRange *tsr, Obj *desired,
                                     Obj *nearx, Obj *owner)
{
  Obj	*obj;
  if ((obj = SpaceFindNearestOwned(ts, tsr, desired, nearx, owner))) {
    return(obj);
  }
  return(SpaceFindNearestOwned(ts, tsr, desired, nearx, NULL));
}

Obj *SpaceFindNearestInGrid(Ts *ts, TsRange *tsr, Obj *not_equal, Obj *grid,
                            GridCoord row1, GridCoord col1)
{
  Obj			*nearest;
  Float			mindist, dist;
  ObjList		*objs, *p;
  GridSubspace	*gs;
  nearest = NULL;
  mindist = FLOATPOSINF;
  objs = RDB(ts, tsr, L(N("at-grid"), ObjWild, grid, ObjWild, E), 1);
  for (p = objs; p; p = p->next) {
    if (not_equal == I(p->obj, 1)) continue;
    if ((gs = ObjToGridSubspace(I(p->obj, 3)))) {
      dist = GridDistance(grid, row1, col1, gs->rows[0], gs->cols[0]);
      if (dist < mindist) {
        nearest = I(p->obj, 1);
        mindist = dist;
      }
    }
  }
  ObjListFree(objs);
  return(nearest);
}

/* <desired> is abstract. <nearx> is concrete.
 * This does not find parts of objects.
 */
ObjList *SpaceFindNearby1(Ts *ts, TsRange *tsr, Obj *desired, Obj *nearx,
                          Obj *owner)
{
  Obj			*nearpolity, *neargrid;
  ObjList		*objs, *p, *r;
  GridCoord		nearrow, nearcol;
  if (!SpaceLocateObject(ts, tsr, nearx, NULL, 0, &nearpolity, &neargrid,
                         &nearrow, &nearcol)) return(NULL);
  if (neargrid == NULL) return(NULL);
  r = NULL;
  for (p = objs = RDB(ts, tsr, L(N("at-grid"), desired, neargrid, ObjWild, E),
                      1);
       p; p = p->next) {
    if (owner && !IsOwnerOf(ts, tsr, I(p->obj, 1), owner)) continue;
    r = ObjListCreate(I(p->obj, 1), r);
  }
  ObjListFree(objs);
  return(r);
}

ObjList *SpaceFindNearby(Ts *ts, TsRange *tsr, Obj *desired, Obj *nearx)
{
  ObjList	*r;
  if ((r = SpaceFindNearby1(ts, tsr, desired, nearx, nearx))) return(r);
  /* todo: Get rid of below case. is to deal with Jim's clothes with no
   * owner-of yet.
   */
  else return(SpaceFindNearby1(ts, tsr, desired, nearx, NULL));
}

ObjList *SpaceFindNear(Ts *ts, TsRange *tsr, Obj *physobj, Obj *restrct,
                       ObjList *r)
{
  Float		radius;
  Obj		*near_obj;
  ObjList	*objs, *p;

  /* (1) Find objects within radius. */
  radius = 100.0;      /* todo: Should depend on grid scale. */
  objs = SpaceFindWithinRadiusOwned(ts, tsr, NULL, physobj, radius, NULL);

  /* (2) Find all objects matching restrct. */
  for (p = objs; p; p = p->next) {
    if (p->obj == physobj) continue;
    if (restrct == NULL ||
        (ISAP(restrct, p->obj))) {
/*
      r = ObjListCreate(p->obj, r);
*/
      near_obj = L(N("near"), physobj, p->obj, E);
      ObjSetTsRange(near_obj, tsr);
        /* todo: would be better to use grid-at tsr */
      r = ObjListCreate(near_obj, r);
    }  
  }
  return(r);
}

/* todo */
ObjList *SpaceFindNearAudible(Ts *ts, TsRange *tsr, Obj *desired, Obj *nearx)
{
  return(SpaceFindWithinRadiusOwned1(ts, tsr, desired, nearx, 100.0, NULL));
}

/* Handles various non canonical rep cases. Should be same as those returned
 * by SpaceFindEnclose.
 */
Bool SpaceLocatedAt(Ts *ts, TsRange *tsr, Obj *obj, Obj *location,
                    /* RESULTS */ TsRange *tsr_r)
{
  if (ISA(N("polity"), location)) {
    return(SpaceInPolity(ts, tsr, obj, location, tsr_r));
  } else {
    return(SpaceEncloses(ts, tsr, location, obj, tsr_r));
  }
}

/******************************************************************************
 * NEARNESS
 ******************************************************************************/

/* todo: Consult SpaceFindEnclose to find out whether <obj1> and <obj2>
 * are in the same room.
 */
Bool SpaceIsNearAudible(Ts *ts, TsRange *tsr, Obj *obj1, Obj *obj2)
{
  if (YES(REB(ts, tsr, L(N("near-audible"), obj1, obj2, E))) ||
      YES(REB(ts, tsr, L(N("near-audible"), obj2, obj1, E)))) {
    return(1);
  }
  if (ObjIsVar(obj1) || ObjIsVar(obj2)) return(1);
  return(SpaceDistance(ts, tsr, obj1, obj2) < 20.0);
}

Bool SpaceGridIsNearReachable(Obj *grid, GridCoord row1, GridCoord col1,
                              GridCoord row2, GridCoord col2)
{
  return(GridAdjacentCell(row1, col1, row2, col2));
  /* todo: Within a certain distance thresh, depending on grid resolution
   * and object sizes.
   */
}

Bool SpaceIsNearReachable(Ts *ts, TsRange *tsr, Obj *obj1, Obj *obj2)
{
  Obj		*polity1, *polity2, *grid1, *grid2;
  GridCoord	row1, row2, col1, col2;
  if (obj1 == obj2) return(1);
  if (!SpaceLocateObject(ts, tsr, obj1, NULL, 0, &polity1, &grid1, &row1,
                         &col1)) {
    return(0);
  }
  if (!SpaceLocateObject(ts, tsr, obj2, NULL, 0, &polity2, &grid2, &row2,
                         &col2)) {
    return(0);
  }
  if (grid1 != grid2) return(0);
  return(SpaceGridIsNearReachable(grid1, row1, col1, row2, col2));
}

/* The reflexive transitive closure of near-graspable.
 * todoTHREAD: Not thread safe.
 */

#define MAXNG	20
int	SpaceNGLen;
Obj	*SpaceNGObjs1[MAXNG], *SpaceNGObjs2[MAXNG];

Bool SpaceNGIn(Obj *obj1, Obj *obj2)
{
  int i;
  for (i = 0; i < SpaceNGLen; i++) {
    if (SpaceNGObjs1[i] == obj1 && SpaceNGObjs2[i] == obj2) return(1);
  }
  return(0);
}

Bool SpaceNGAdd(Obj *obj1, Obj *obj2)
{
  if (SpaceNGLen == MAXNG) {
    Dbg(DBGSPACE, DBGBAD, "increase MAXNG");
    return(0);
  }
  SpaceNGObjs1[SpaceNGLen] = obj1;
  SpaceNGObjs2[SpaceNGLen] = obj2;
  SpaceNGLen++;
  return(1);
}

Bool SpaceIsNearGraspable1(Ts *ts, Obj *obj1, Obj *obj2)
{
  ObjList	*objs, *p;
  if (obj1 == obj2) return(1);
  if (SpaceNGIn(obj1, obj2)) return(0);
  if (!SpaceNGAdd(obj1, obj2)) return(0);
  if (YES(RE(ts, L(N("holding"), obj1, obj2, E)))) return(1);
  if (YES(RE(ts, L(N("holding"), obj2, obj1, E)))) return(1);
  for (p = objs = RE(ts, L(N("near-graspable"), obj1, ObjWild, E));
       p; p = p->next) {
    if (SpaceIsNearGraspable1(ts, obj2, I(p->obj, 2))) {
      ObjListFree(objs);
      return(1);
    }
  }
  ObjListFree(objs);
  for (p = objs = RE(ts, L(N("near-graspable"), ObjWild, obj1, E));
       p; p = p->next) {
    if (SpaceIsNearGraspable1(ts, obj2, I(p->obj, 1))) {
      ObjListFree(objs);
      return(1);
    }
  }
  ObjListFree(objs);
  return(0);
}

Bool SpaceIsNearGraspable(Ts *ts, Obj *obj1, Obj *obj2)
{
  SpaceNGLen = 0;
  return(SpaceIsNearGraspable1(ts, obj1, obj2));
}

/* End of file. */
