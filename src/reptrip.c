/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940809: begun
 * 19940811: more work
 * 19940812: more work
 * 19940813: more work
 * 19940814: more work
 * 19940815: more work
 * 19940825: some minor redoing to meld with planner
 *
 * todo:
 * - Select best trip.
 * - Take into account gas costs, food costs, wear and tear.
 */

#include "tt.h"
#include "reptrip.h"
#include "repbasic.h"
#include "repdb.h"
#include "repgrid.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "reptime.h"
#include "utildbg.h"

/*
#define TRIPDBG
 */

/* TripLeg */

TripLeg *TripLegCreate(Bool start, Obj *action, Obj *driver, Obj *transporter,
                       Obj *script, Obj *grid, Obj *from, Obj *to,
                       Obj *wormhole, GridSubspace *path, Ts *depart,
                       Ts *arrive, Obj *cost, TripLeg *next, Bool print)
{
  TripLeg	*tl;
  tl = CREATE(TripLeg);
  tl->start = start;
  tl->action = action;
  tl->driver = driver;
  tl->transporter = transporter;
  tl->script = script;
  tl->grid = grid;
  tl->from = from;
  tl->to = to;
  tl->wormhole = wormhole;
  tl->path = path;
  tl->depart = *depart;
  tl->arrive = *arrive;
  tl->cost = cost;
  tl->next = next;
  if (DbgOn(DBGSPACE, DBGDETAIL) && print)
    TripLegPrint(Log, tl);
  return(tl);
}

void TripLegFree(TripLeg *tl)
{
  TripLeg *n;
  while (tl) {
    n = tl->next;
    /* todoFREE: Can't free this because it is shared (not copied).
    if (tl->path) GridSubspaceFree(tl->path);
     */
    MemFree(tl, "TripLeg");
    tl = n;
  }
}

TripLeg *TripLegLast(TripLeg *legs)
{
  while (legs->next) legs = legs->next;
  return(legs);
}

TripLeg *TripLegAppend(TripLeg *tl1, TripLeg *tl2)
{
  if (tl1) {
    TripLegLast(tl1)->next = tl2;
    return(tl1);
  } else return(tl2);
}

TripLeg *TripLegCopyAll(TripLeg *tl)
{
  TripLeg	*r;
  r = NULL;
  for (; tl; tl = tl->next) {
    r = TripLegAppend(r, TripLegCreate(tl->start, tl->action, tl->driver,
                                       tl->transporter, tl->script, tl->grid,
                                       tl->from, tl->to, tl->wormhole,
                                       tl->path, &tl->depart, &tl->arrive,
                                       tl->cost, NULL, 0));
  }
  return(r);
}

TripLeg *TripLegNextSegment(TripLeg *tl)
{
  tl = tl->next;
  while (tl && !tl->start) tl = tl->next;
  return(tl); 
}

void TripLegPrint(FILE *stream, TripLeg *tl)
{
  fprintf(stream,
"%c %10.10s %10.10s %10.10s %7.7s %10.10s %10.10s %10.10s %10.10s %4.4s ",
          tl->start ? '*' : SPACE,
          M(tl->action), M(tl->driver), M(tl->transporter), M(tl->script),
          M(tl->grid), M(tl->from), M(tl->to), M(tl->wormhole),
          tl->path ? "yes" : "");
  TsPrint(stream, &tl->depart);
  fputc(SPACE, stream);
  TsPrint(stream, &tl->arrive);
  fputc(SPACE, stream);
  fprintf(stream, "%5.5s\n", M(tl->cost)); 
}

void TripLegShortPrint(FILE *stream, TripLeg *tl)
{
  fprintf(stream,
         "<tripleg %7.7s %7.7s %7.7s %7.7s>",
          M(tl->action), M(tl->driver), M(tl->transporter), M(tl->grid));
}

void TripLegPrintAll(FILE *stream, TripLeg *tl)
{
  fprintf(stream,
"%c %10.10s %10.10s %10.10s %7.7s %10.10s %10.10s %10.10s %10.10s %4.4s %14.14s %14.14s %5.5s\n",
          SPACE, "action", "driver", "transport", "script", "grid", "from",
          "to", "wormhole", "path", "depart", "arrive", "cost");
  for (; tl; tl = tl->next) TripLegPrint(stream, tl);
}

/* Trip */

Trip *TripCreate(TripLeg *legs, Obj *cost, Ts *depart, Ts *arrive, Trip *next)
{
  Trip	*tr;
  tr = CREATE(Trip);
  tr->legs = legs;
  tr->cost = cost;
  tr->depart = *depart;
  tr->arrive = *arrive;
  tr->next = next;
  return(tr);
}

void TripFree(Trip *tr)
{
  Trip *n;
  while (tr) {
    TripLegFree(tr->legs);
    n = tr->next;
    MemFree(tr, "Trip");
    tr = n;
  }
}

Obj *FinanceAdd(Obj *a1, Obj *a2)
{
  return(ObjNA);
}

Trip *TripAppendEach(Trip *head, Trip *tails, Trip *r)
{
  TripLeg	*legs;
  Trip		*tail;
  for (tail = tails; tail; tail = tail->next) {
    legs = TripLegAppend(TripLegCopyAll(head->legs),
                         TripLegCopyAll(tail->legs));
    r = TripCreate(legs, FinanceAdd(head->cost, tail->cost),
                   &head->depart, &tail->arrive, r);
  }
  return(r);
}

void TripPrint(FILE *stream, Trip *tr)
{
  fprintf(stream, "Trip ");
  TsPrint(stream, &tr->depart);
  fputc(SPACE, stream);
  TsPrint(stream, &tr->arrive);
  fputc(SPACE, stream);
  fprintf(stream, "%10.10s", M(tr->cost));
  if (tr->legs) {
    fputc(NEWLINE, stream);
    TripLegPrintAll(stream, tr->legs);
  } else fputs("\nnoop", stream);
}

void TripPrintAll(FILE *stream, Trip *tr)
{
  for (; tr; tr = tr->next) TripPrint(stream, tr);
}

/* Trip planning code. */

Bool Trip3(int leave_immed, Obj *actor, Obj *obj1, Obj *obj2, Ts *leave_after,
           Ts *arrive_before, Obj *polity1, Obj *grid1, GridCoord row1,
           GridCoord col1, Obj *polity2, Obj *grid2, GridCoord row2,
           GridCoord col2, int drive_ok, Trip *in_tr, /* RESULTS */
           Trip **out_tr)
{
  Trip		*r;
  *out_tr = in_tr;
  Dbg(DBGSPACE, DBGDETAIL, "Trip3 from <%s> to <%s>", M(obj1), M(obj2));
  if (DbgOn(DBGSPACE, DBGDETAIL)) {
    TsPrint(Log, leave_after);
    fputc(SPACE, Log);
    TsPrint(Log, arrive_before);
    fputc(NEWLINE, Log);
  }
  if (grid1 == NULL) {
    Dbg(DBGSPACE, DBGDETAIL, "<%s> not in a grid", M(obj1));
    return(0);
  }
  if (grid2 == NULL) {
    Dbg(DBGSPACE, DBGDETAIL, "<%s> not in a grid", M(obj2));
    return(0);
  }
  if (obj1 == obj2 ||
      (grid1 == grid2 &&
       SpaceGridIsNearReachable(grid1, row1, col1, row2, col2))) {
    Dbg(DBGSPACE, DBGDETAIL, "ALREADY near-reachable <%s> and <%s>",
        M(obj1), M(obj2), E);
    *out_tr = TripCreate(NULL, NULL, leave_after, leave_after, in_tr);
    return(1);
  }
  r = in_tr;
  TripGridTraverse(leave_immed, N("grid-walk"), NULL, actor, NULL, actor, NULL,
                   N("walkable"), obj1, obj2, leave_after, arrive_before,
                   polity1, grid1, row1, col1, polity2, grid2, row2, col2,
                   NULL, r, &r);
  if (grid1 != grid2) {	/* todo: && small-sized (walkable) grid */
    if (drive_ok) {
      TripDrive(actor, obj1, obj2, leave_after, arrive_before, polity1, grid1,
                row1, col1, polity2, grid2, row2, col2, r, &r);
    }
    TripTransport(N("take-subway"), actor, obj1, obj2, leave_after,
                  arrive_before, polity1, grid1, row1, col1, polity2, grid2,
                  row2, col2, drive_ok, r, &r);
    TripTransport(N("take-bus"), actor, obj1, obj2, leave_after, arrive_before,
                  polity1, grid1, row1, col1, polity2, grid2, row2, col2,
                  drive_ok, r, &r);
    TripTransport(N("passenger-fly"), actor, obj1, obj2, leave_after,
                  arrive_before, polity1, grid1, row1, col1, polity2, grid2,
                  row2, col2, drive_ok, r, &r);
  }
  *out_tr = r;
  return(r != in_tr);
}

Bool Trip2(int leave_immed, Obj *actor, Obj *obj1, Obj *obj2, Ts *leave_after,
           Ts *arrive_before, Obj *polity1, Obj *grid1, GridCoord row1,
           GridCoord col1, int drive_ok, Trip *in_tr, /* RESULTS */
           Trip **out_tr)
{
  Obj			*polity2, *grid2;
  GridCoord		row2, col2;
  if (!SpaceLocateObject(leave_after, NULL, obj2, grid1, 0, &polity2, &grid2,
                         &row2, &col2)) {
    *out_tr = in_tr;
    return(0);
  }
  return(Trip3(leave_immed, actor, obj1, obj2, leave_after, arrive_before,
               polity1, grid1, row1, col1, polity2, grid2, row2, col2,
               drive_ok, in_tr, out_tr));
}

Bool Trip1(int leave_immed, Obj *actor, Obj *obj1, Obj *obj2, Ts *leave_after,
           Ts *arrive_before, int drive_ok, Trip *in_tr, /* RESULTS */
           Trip **out_tr)
{
  Obj			*polity1, *grid1;
  GridCoord		row1, col1;
  if (!SpaceLocateObject(leave_after, NULL, obj1, NULL, 0, &polity1, &grid1,
                         &row1, &col1)) {
    *out_tr = in_tr;
    return(0);
  }
  return(Trip2(leave_immed, actor, obj1, obj2, leave_after, arrive_before,
               polity1, grid1, row1, col1, drive_ok, in_tr, out_tr));
}

Trip *TripFind(Obj *actor, Obj *obj1, Obj *obj2, Ts *leave_after,
               Ts *arrive_before)
{
  Trip	*tr;
  if (Trip1(1, actor, obj1, obj2, leave_after, arrive_before, 1, NULL, &tr)) {
    if (tr->legs) {
      Dbg(DBGSPACE, DBGDETAIL, "found trips:");
      TripPrintAll(Log, tr);
    }
  } else {
    Dbg(DBGSPACE, DBGBAD, "no trips found from <%s> to <%s>", M(obj1), M(obj2));
  }
  return(tr);
}

/* <leave_immed> 1 = leave at <leave_after>
 *               0 = leave at <arrive_before> - traversal duration
 * Note <leave_after> is scheduled departure time when <sched_arrive> non-NULL.
 */
Bool TripGridTraverse(int leave_immed, Obj *action, Ts *sched_arrive,
                      Obj *actor, Obj *driver, Obj *transporter, Obj *script,
                      Obj *prop, Obj *obj1, Obj *obj2, Ts *leave_after,
                      Ts *arrive_before, Obj *polity1, Obj *grid1,
                      GridCoord row1, GridCoord col1, Obj *polity2,
                      Obj *grid2, GridCoord row2, GridCoord col2,
                      Obj *cost, Trip *in_tr, /* RESULTS */ Trip **out_tr)
{
  int		i;
  Dur		dur;
  Float		dist, speed;
  Ts		ts_cur, depart;
  IntergridPath	*igp;
  Obj		*obj_cur, *grid_next, *polity_next, *grid_cur;
  GridCoord	row_next, col_next, row_cur, col_cur;
  TripLeg	*legs, *leg;
  *out_tr = in_tr;
  Dbg(DBGSPACE, DBGDETAIL, "TripGridTraverse using <%s> from <%s> to <%s>",
      M(transporter), M(obj1), M(obj2));
  if (DbgOn(DBGSPACE, DBGDETAIL)) {
    TsPrint(Log, leave_after);
    fputc(SPACE, Log);
    TsPrint(Log, arrive_before);
    fputc(NEWLINE, Log);
  }
  speed = ObjToNumber(DbGetRelationValue(&TsNA, NULL, N("travel-max-speed-of"),
                                         transporter, NULL));
  if (speed == FLOATNA) speed = 1.0;

  dist = SpaceDistance2(leave_after, NULL, polity1, grid1, row1, col1,
                        polity2, grid2, row2, col2);
  dur = (Dur)(dist/speed);

  if (transporter == actor && dur > SECONDSPERHOURL) {
    /* Don't walk: more than an hour away by foot. */
    return(0);
  }
  
  if (ISA(N("motor-vehicle"), transporter) && dur < 120L) {
    /* Don't drive: less than 2-minute drive. */
    return(0);
  }

  if (leave_immed) {
    ts_cur = *leave_after;
  } else {
    ts_cur = *arrive_before;
    TsIncrement(&ts_cur, -dur);
    if (TsLT(&ts_cur, leave_after)) {
      if (DbgOn(DBGSPACE, DBGDETAIL)) {
        Dbg(DBGSPACE, DBGDETAIL, "too far: dist %gm speed %g dur %ldsec",
            dist, speed, dur);
        TsPrint(Log, leave_after);
        fputc(SPACE, Log);
        TsPrint(Log, arrive_before);
        fputc(SPACE, Log);
        TsPrint(Log, &ts_cur);
        fputc(NEWLINE, Log);
      }
#ifndef TRIPDBG
      return(0);
#endif
    }
  }

  legs = NULL;
  if (grid1 != grid2) {
    if (!(igp = IntergridFindPath(leave_after, NULL, grid2, grid1))) {
      Dbg(DBGSPACE, DBGDETAIL, "no intergrid path found from <%s> to <%s>",
          M(obj1), M(obj2), E);
      return(0);
    }
    if (igp->grids[0] != grid1) {
      Dbg(DBGSPACE, DBGBAD, "TripGridTraverse 1");
      /* todo: More recovery? */
      return(0);
    }
    obj_cur = obj1;
    row_cur = row1;
    col_cur = col1;
    grid_cur = igp->grids[0];
    for (i = 0; i < igp->len-1; i++) {
      if (!SpaceLocateObject(&ts_cur, NULL, igp->wormholes[i+1], grid_cur, 0,
                             &polity_next, &grid_next, &row_next, &col_next)) {
        return(0);
      }
      if (!(leg = TripGridTraverse1((i == 0), action, prop, actor, driver,
                                    transporter, script, obj_cur,
                                    igp->wormholes[i+1], grid_cur, row_cur,
                                    col_cur, row_next, col_next, speed, cost,
                                    &ts_cur))) {
        return(0);
      }
      if (i == 0 && sched_arrive) leg->depart = *leave_after;
      ts_cur = leg->arrive;
      cost = NULL;
      legs = TripLegAppend(legs, leg);
      depart = ts_cur;
      TsIncrement(&ts_cur, DurationOf(N("warp")));
      leg = TripLegCreate(0, N("warp"), NULL, NULL, NULL, NULL, grid_cur,
                          igp->grids[i+1], igp->wormholes[i+1], NULL, &depart,
                          &ts_cur, NULL, NULL, 1);
      legs = TripLegAppend(legs, leg);
      if (!SpaceLocateObject(&ts_cur, NULL, igp->wormholes[i+1],
                             igp->grids[i+1], 0, &polity_next, &grid_next,
                             &row_next, &col_next)) {
        return(0);
      }
      grid_cur = igp->grids[i+1];
      obj_cur = igp->wormholes[i+1];
      row_cur = row_next;
      col_cur = col_next;
    }
    if (!(leg = TripGridTraverse1((i == 0), action, prop, actor, driver,
                                  transporter, script, obj_cur, obj2, grid_cur,
                                  row_cur, col_cur, row2, col2, speed, NULL,
                                  &ts_cur))) {
      return(0);
    }
    if (sched_arrive) {
      ts_cur = *sched_arrive;
      leg->arrive = *sched_arrive;
    } else {
      ts_cur = leg->arrive;
    }
    legs = TripLegAppend(legs, leg);
  } else {
    if (!(leg = TripGridTraverse1(1, action, prop, actor, driver, transporter,
                                  script, obj1, obj2, grid1, row1, col1, row2,
                                  col2, speed, NULL, &ts_cur))) {
      return(0);
    }
    if (sched_arrive) {
      ts_cur = *sched_arrive;
      leg->arrive = *sched_arrive;
    } else {
      ts_cur = leg->arrive;
    }
    legs = TripLegAppend(legs, leg);
  }
  if (TsGT(&ts_cur, arrive_before)) {
    Dbg(DBGSPACE, DBGDETAIL, "too late <%s> <%s>", M(obj1), M(obj2));
    if (DbgOn(DBGSPACE, DBGDETAIL)) {
      TsPrint(Log, &ts_cur);
      fputs(" > ", Log);
      TsPrint(Log, arrive_before);
      fputc(NEWLINE, Log);
    }
#ifndef TRIPDBG
    return(0);
#endif
  }
  *out_tr = TripCreate(legs, NULL, &legs->depart, &ts_cur, in_tr);
  return(1);
}

TripLeg *TripGridTraverse1(Bool start, Obj *action, Obj *prop, Obj *actor,
                           Obj *driver, Obj *transporter, Obj *script,
                           Obj *obj1, Obj *obj2, Obj *grid, GridCoord row1,
                           GridCoord col1, GridCoord row2, GridCoord col2,
                           Float speed, Obj *cost, Ts *depart)
{
  Float		dist;
  GridSubspace	*gs;
  Grid		*gridstate;
  Dur		dur;
  Ts		arrive;
  Dbg(DBGSPACE, DBGDETAIL, "finding path from <%s> %d %d to <%s> %d %d",
      M(obj1), row1, col1, M(obj2), row2, col2);
  gridstate = GridStateGet(depart, grid, prop);
  if ((gs = GridFindPath(gridstate, row1, col1, row2, col2, "*"))) {
    GridSubspacePrint(Log, depart, gs, 1);
    dist = GridSubspacePathDist(gs);
    dur = (Dur)(dist/speed);
    Dbg(DBGSPACE, DBGDETAIL, "dist %g speed %g dur %ld", dist, speed, dur);
    arrive = *depart;
    TsIncrement(&arrive, dur);
    return(TripLegCreate(start, action, driver, transporter, script, grid,
                         obj1, obj2, NULL, gs, depart, &arrive, cost, NULL,
                         1));
  }
  Dbg(DBGSPACE, DBGDETAIL, "unable to find grid path from <%s> to <%s>",
      M(obj1), M(obj2));
  GridPrint(Log, "gridstate", gridstate, 1);
  return(NULL);
}

#define DRIVEOVERHEAD	1.20	/* todo: Inelegant. */

Bool TripDrive(Obj *actor, Obj *obj1, Obj *obj2, Ts *leave_after,
               Ts *arrive_before, Obj *polity1, Obj *grid1, GridCoord row1,
               GridCoord col1, Obj *polity2, Obj *grid2, GridCoord row2,
               GridCoord col2, Trip *in_tr, /* RESULTS */ Trip **out_tr)
{
  Obj		*car, *driver, *polity_car, *grid_car, *pkg, *polity_pkg;
  Obj		*grid_pkg;
  Float		dist, speed;
  Dur		dur;
  GridCoord	row_car, col_car, row_pkg, col_pkg;
  Ts		start_driving_before;
  Trip		*subtrips1, *st1, *subtrips2, *subtrips3, *st3, *subtrips4, *r;
  *out_tr = in_tr;
  if ((dist = SpaceDistance2(leave_after, NULL, polity1, grid1, row1, col1,
                             polity2, grid2, row2, col2)) < 300.0) {
    /* Don't drive: less than 300 meters away. */
    return(0);
  }
  Dbg(DBGSPACE, DBGDETAIL, "TripDrive from <%s> to <%s>", M(obj1), M(obj2));
  if (DbgOn(DBGSPACE, DBGDETAIL)) {
    TsPrint(Log, leave_after);
    fputc(SPACE, Log);
    TsPrint(Log, arrive_before);
    fputc(NEWLINE, Log);
  }
  /* todo: Also if child has driver's license. */
  if (!(driver = GuardianOf(leave_after, NULL, actor))) driver = actor;
  /* todo: Analogy: create rental car in major cities. */
  if (!(car = SpaceFindNearestOwned(leave_after, NULL, N("motor-vehicle"),
                                    obj1, driver))) {
    return(0);
  }
  if (!SpaceLocateObject(leave_after, NULL, car, NULL, 0, &polity_car,
                         &grid_car, &row_car, &col_car)) {
    return(0);
  }
  speed = ObjToNumber(DbGetRelationValue(&TsNA, NULL, N("travel-max-speed-of"),
                                         car, NULL));
  if (speed == FLOATNA) speed = 1.0;
  dur = (Dur)(DRIVEOVERHEAD*(dist/speed));
  start_driving_before = *arrive_before;
  TsIncrement(&start_driving_before, -dur);
  if (DbgOn(DBGSPACE, DBGDETAIL)) {
    Dbg(DBGSPACE, DBGDETAIL, "start_driving_before");
    TsPrint(Log, &start_driving_before);
    fputc(NEWLINE, Log);
  }
  if ((!(pkg = SpaceFindNearest(leave_after, NULL, N("parking"), obj2))) ||
      (!SpaceLocateObject(leave_after, NULL, pkg, NULL, 0, &polity_pkg,
                          &grid_pkg, &row_pkg, &col_pkg))) {
    Dbg(DBGSPACE, DBGDETAIL, "no parking found");
    pkg = obj2;
    polity_pkg = polity2;
    grid_pkg = grid2;
    row_pkg = row2;
    col_pkg = col2;
  }
  if (!Trip3(0, actor, obj1, car, leave_after, &start_driving_before, polity1,
             grid1, row1, col1, polity_car, grid_car, row_car, col_car,
             0, NULL, &subtrips1)) {
    Dbg(DBGSPACE, DBGDETAIL, "no path to car");
    return(0);
  }
  
  subtrips3 = NULL;
  for (st1 = subtrips1; st1; st1 = st1->next) {
    if (TripGridTraverse(1, N("grid-drive-car"), NULL, actor, driver, car, NULL,
                         N("drivable"), car, pkg, &st1->arrive, arrive_before,
                         polity_car, grid_car, row_car, col_car, polity_pkg,
                         grid_pkg, row_pkg, col_pkg, NULL, NULL, &subtrips2)) {
      subtrips3 = TripAppendEach(st1, subtrips2, subtrips3);
      TripFree(subtrips2);
    }
  }
  TripFree(subtrips1);
  r = in_tr;
  for (st3 = subtrips3; st3; st3 = st3->next) {
    if (Trip1(1, actor, pkg, obj2, &st3->arrive, arrive_before, 0, NULL,
              &subtrips4)) {
      r = TripAppendEach(st3, subtrips4, r);
    }
    TripFree(subtrips4);
  }
  TripFree(subtrips3);
  *out_tr = r;
  return(r != in_tr);
}

Bool TripGetDepArr(Obj *script, Obj *prop, Ts *leave_after, Ts *arrive_before,
                   /* RESULTS */ Dur *checkdurp, Ts *checkin, Ts *depart,
                   Ts *arrive)
{
  int		i;
  Tod		tod, start_tod, end_tod;
  Dur		tdur, tfreq, checkdur;
  Ts		midnight, dep, arr, check;
  TsRange	*tsr;

  tsr = ObjToTsRange(prop);
  tfreq = ObjToDur(DbGetRelationValue(&TsNA, NULL, N("frequency-of"),
                                      script, NULL),
                   0L);
  tdur = ObjToDur(DbGetRelationValue(&TsNA, NULL, N("duration-of"),
                                     script, NULL),
                  tsr->dur);
  checkdur = ObjToDur(DbGetRelationValue(&TsNA, NULL, N("checkin-duration-of"),
                                         script, NULL),
                      0L);
  *checkdurp = checkdur;

  TsMidnight(arrive_before, &midnight);
  for (i = 0; i < 8; i++, TsIncrement(&midnight, -SECONDSPERDAYL)) {
    if (!DaysIn(TsToDay(&midnight), tsr->days)) continue;
    start_tod = tsr->tod;
    if (tfreq != 0L) end_tod = start_tod + tsr->dur;
    else end_tod = start_tod;
    for (tod = end_tod; tod >= start_tod; tod -= tfreq) {
      dep = midnight;
      TsIncrement(&dep, tod);
      check = dep;
      TsIncrement(&check, -checkdur);
      arr = dep;
      TsIncrement(&arr, tdur);
      if (DbgOn(DBGSPACE, DBGHYPER)) {
        Dbg(DBGSPACE, DBGHYPER,
            "considering trip: (check leave_after arrive_before)");
        TsPrint(Log, &check);
        fputc(SPACE, Log);
        TsPrint(Log, leave_after);
        fputc(SPACE, Log);
        TsPrint(Log, arrive_before);
        fputc(NEWLINE, Log);
      }
      if (TsGE(&check, leave_after) && TsLE(&arr, arrive_before)) {
        *checkin = check;
        *depart = dep;
        *arrive = arr;
        return(1);
      }
      if (tfreq == 0L) break;
    }
  }
  return(0);
}

/* Consider a particular scheduled trip. */
Bool TripTransport1(Obj *script, Obj *prop, Obj *traversal_action, Obj *t1,
                    Obj *t2, Obj *polity_t1, Obj *grid_t1, GridCoord row_t1,
                    GridCoord col_t1, Obj *able, Obj *trip_class, Obj *actor,
                    Obj *obj1, Obj *obj2, Ts *leave_after, Ts *arrive_before,
                    Obj *polity1, Obj *grid1, GridCoord row1, GridCoord col1,
                    Obj *polity2, Obj *grid2, GridCoord row2, GridCoord col2,
                    int drive_ok, Trip *in_tr, /* RESULTS */ Trip **out_tr)
{
  GridCoord	row_t2, col_t2;
  Ts		checkin, depart, arrive, arrive1;
  Obj		*driver, *polity_t2, *grid_t2, *transporter, *cost;
  Dur		checkdur;
  Trip		*subtrips1, *st1, *subtrips2, *subtrips3, *st3, *subtrips4, *r;
  *out_tr = in_tr;
  
  if (!(transporter = DbGetRelationValue(&TsNA, NULL, N("transporter"),
                                         script, NULL))) {
    return(0);
  }
  driver = DbGetRelationValue(&TsNA, NULL, N("driver"), script, NULL);
  cost = DbGetRelationValue(&TsNA, NULL, N("cost-of"), script,
                            NULL);
  if (!SpaceLocateObject(leave_after, NULL, t2, NULL, 0, &polity_t2, &grid_t2,
                         &row_t2, &col_t2)) {
    return(0);
  }
  if (!TripGetDepArr(script, prop, leave_after, arrive_before,
                     &checkdur, &checkin, &depart, &arrive)) {
    Dbg(DBGSPACE, DBGDETAIL, "TripTransport1 failure <%s> <%s> <%s>",
        M(trip_class), M(obj1), M(obj2));
    return(0);
  }
  if (!Trip3(0, actor, obj1, t1, leave_after, &checkin, polity1,
             grid1, row1, col1, polity_t1, grid_t1, row_t1, col_t1, 
             1, NULL, &subtrips1)) {
    Dbg(DBGSPACE, DBGDETAIL, "get to transporter failure <%s> <%s> <%s>",
        M(trip_class), M(obj1), M(obj2));
    return(0);
  }
  subtrips3 = NULL;
  for (st1 = subtrips1; st1; st1 = st1->next) {
    if (!TripGridTraverse(1, traversal_action, &arrive, actor, driver,
                          transporter, script, able, t1, t2, &depart,
                          arrive_before, polity_t1, grid_t1, row_t1, col_t1,
                          polity_t2, grid_t2, row_t2, col_t2, cost, NULL,
                          &subtrips2)) {
      Dbg(DBGSPACE, DBGDETAIL,
          "TripTransport1 TripGridTraverse failure <%s> <%s> <%s>",
          M(trip_class), M(obj1), M(obj2));
    } else {
      subtrips3 = TripAppendEach(st1, subtrips2, subtrips3);
      TripFree(subtrips2);
     }
  }
  TripFree(subtrips1);
  r = in_tr;
  arrive1 = *arrive_before;
  TsIncrement(&arrive1, checkdur/2L);
    /* todo: Inelegant. Not sure of good solution. */
  for (st3 = subtrips3; st3; st3 = st3->next) {
    if (!Trip3(1, actor, t2, obj2, &st3->arrive, &arrive1, polity_t2,
               grid_t2, row_t2, col_t2, polity2, grid2, row2, col2, 1,
               NULL, &subtrips4)) {
      Dbg(DBGSPACE, DBGDETAIL,
          "TripTransport1 destination failure <%s> <%s> <%s>",
          M(trip_class), M(obj1), M(obj2));
    } else {
      r = TripAppendEach(st3, subtrips4, r);
      TripFree(subtrips4);
    }
  }
  TripFree(subtrips3);
  *out_tr = r;
  return(r != in_tr);
}

Bool TripTransport(Obj *trip_class, Obj *actor, Obj *obj1, Obj *obj2,
                   Ts *leave_after, Ts *arrive_before,
                   Obj *polity1, Obj *grid1, GridCoord row1, GridCoord col1,
                   Obj *polity2, Obj *grid2, GridCoord row2, GridCoord col2,
                   int drive_ok, Trip *in_tr, /* RESULTS */ Trip **out_tr)
{
  Float		radius;
  GridCoord	row_t1, col_t1;
  Obj		*able, *term, *t2, *polity_t1, *grid_t1, *traversal_action;
  ObjList	*t1s, *t2s, *t1, *scheduled, *p;
  TsRange	tsr;
  Trip		*r;
  *out_tr = in_tr;
  if (trip_class == N("passenger-fly")) {
    able = N("flyable");
    term = N("boarding-room");
    traversal_action = N("grid-fly-plane");
    radius = 10000.0;
  } else if (trip_class == N("take-bus")) {
    able = N("drivable");
    term = N("bus-stop");
    traversal_action = N("grid-drive-car");
    radius = 500.0;
  } else if (trip_class == N("take-subway")) {
    able = N("rollable");
    term = N("subway-platform");
    traversal_action = N("grid-conduct-train");
    radius = 500.0;
  } else {
    Dbg(DBGSPACE, DBGBAD, "TripTransport");
    able = N("drivable");
    term = N("bus-stop");
    traversal_action = N("grid-drive-car");
    radius = 500.0;
  }
  /* todo: Analogy: create airports in major cities. */
  if (!(t1s = SpaceFindWithinRadiusOwned(leave_after, NULL, term, obj1,
                                         radius, NULL))) {
    return(0);
  }
  if (!(t2s = SpaceFindWithinRadiusOwned(leave_after, NULL, term, obj2,
                                         radius, NULL))) {
    return(0);
  }
  Dbg(DBGSPACE, DBGDETAIL, "TripTransport by <%s> from <%s> to <%s>",
      M(trip_class), M(obj1), M(obj2));
  if (DbgOn(DBGSPACE, DBGDETAIL)) {
    TsPrint(Log, leave_after);
    fputc(SPACE, Log);
    TsPrint(Log, arrive_before);
    fputc(NEWLINE, Log);
  }
  TsRangeSetAlways(&tsr);
  tsr.startts = *leave_after;
  tsr.stopts = *arrive_before;
  r = in_tr;
  for (t1 = t1s; t1; t1 = t1->next) {
    if (!SpaceLocateObject(leave_after, NULL, t1->obj, NULL, 0, &polity_t1,
                           &grid_t1, &row_t1, &col_t1)) {
      continue;
    }
    scheduled = RDR(&tsr, L(N("origination-of"), trip_class, t1->obj, E), 1);
    for (p = scheduled; p; p = p->next) {
      t2 = DbGetRelationValue(&TsNA, NULL, N("destination-of"), I(p->obj, 1),
                              NULL);
      if (t1->obj == t2) continue;
      if (!ObjListIn(t2, t2s)) continue;
      TripTransport1(I(p->obj, 1), p->obj, traversal_action, t1->obj, t2,
                     polity_t1, grid_t1, row_t1, col_t1, able, trip_class,
                     actor, obj1, obj2, leave_after, arrive_before, polity1,
                     grid1, row1, col1, polity2, grid2, row2, col2, drive_ok,
                     r, &r);
    }
    ObjListFree(scheduled);
  }
  *out_tr = r;
  return(r != in_tr);
}

/* End of file. */
