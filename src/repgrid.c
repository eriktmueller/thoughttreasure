/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940313: begun
 * 19940319: CharMatrix and GridState merged into one type
 * 19940320: fill debugging, path finding by line and running along wall
 * 19940423: integrated with Objs
 * 19940424: redid GridRegion parsing so objects are contiguous;
 *           redid fill and path finding algorithms;
 *           merged Grid and GridRegion
 * 19940612: redid GridRegions as GridSubspaces
 */

#include "tt.h"
#include "repbasic.h"
#include "repgrid.h"
#include "repdb.h"
#include "repdbf.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "repstr.h"
#include "reptext.h"
#include "reptime.h"
#include "toolsvr.h"
#include "utildbg.h"

#define GR_SUB(gr, row, col) (*((gr)->m+((row)*(gr)->cols)+(col)))

#define GR_SUB1(gr, row, col) \
  ((((row) < 0) || ((row) >= (gr)->rows) || \
    ((col) < 0) || ((col) >= (gr)->cols)) ? \
   GR_EMPTY : GR_SUB((gr), (row), (col)))

#define GridIsEmpty(srcgr, row, col, wallchars) \
  (((wallchars)[0] == GR_NOT) ? \
   (StringIn(GR_SUB((srcgr), (row), (col)), wallchars+1)) : \
   (!StringIn(GR_SUB((srcgr), (row), (col)), (wallchars))))

#define GridIsEmpty1(srcgr, row, col, wallchars) \
  ((((row) < 0) || ((row) >= (gr)->rows) || \
    ((col) < 0) || ((col) >= (gr)->cols)) ? 0 : \
    GridIsEmpty((srcgr), (row), (col), (wallchars)))

/* Grid basic. */

Grid	*LastGridState;
Obj	*LastGridObj, *LastGridProp;
Ts	LastGridTs;

void GridInit()
{
  LastGridState = NULL;
  LastGridObj = LastGridProp = NULL;
  TsSetNa(&LastGridTs);
}

Grid *GridCreate(Obj *grid, GridCoord rows, GridCoord cols, int fill)
{
  Grid	*gr;
  int	size;
  gr = CREATE(Grid);
  gr->rows = rows;
  gr->cols = cols;
  size = gr->rows*gr->cols;
  gr->m = (char *)MemAlloc(size, "char Grid m");
  memset((void *)gr->m, fill, size);
  gr->grid = grid;
  return(gr);
}

void GridFree(Grid *gr)
{
  MemFree((void *)gr->m, "char Grid m");
  MemFree((void *)gr, "Grid");
}

Grid *GridCopy(Grid *gr)
{
  Grid	*r;
  int	size;
  r = GridCreate(gr->grid, gr->rows, gr->cols, (int)GR_EMPTY);
  size = gr->rows*gr->cols;
  memcpy(r->m, gr->m, size);
  gr->grid = NULL;
  return(r);
}

Grid *GridBlankCopy(Grid *gr)
{
  return(GridCreate(gr->grid, gr->rows, gr->cols, (int)GR_EMPTY));
}

void GridPrint(FILE *stream, char *type, Grid *gr, int longformat)
{
  GridCoord row;
  fprintf(stream, "<%s %s>", type, ObjToName(gr->grid));
  if (longformat) {
    fputc(NEWLINE, stream);
    CharPutN(stream, '.', gr->cols+2);
    fputc(NEWLINE, stream);
    for (row = 0; row < gr->rows; row++) {
      fputc('.', stream);
      StringPrintLen(stream, gr->m+row*gr->cols, gr->cols);
      fputc('.', stream);
      fputc(NEWLINE, stream);
    }
    CharPutN(stream, '.', gr->cols+2);
    fputc(NEWLINE, stream);
  }
}

Bool GridAdjacentCell(GridCoord r1, GridCoord c1, GridCoord r2, GridCoord c2)
{
  return((r1 == r2 && c1 == c2) ||
         (r1 == r2 && c1 == c2-1) ||
         (r1 == r2 && c1 == c2+1) ||
         (r1 == r2-1 && c1 == c2) ||
         (r1 == r2+1 && c1 == c2) ||
         (r1 == r2-1 && c1 == c2-1) ||
         (r1 == r2+1 && c1 == c2-1) ||
         (r1 == r2-1 && c1 == c2+1) ||
         (r1 == r2+1 && c1 == c2+1));
}

#define GridAdjacentIsFilled(gr, row, col) \
        ((GR_SUB1(gr, row, col) == GR_FILL) || \
         (GR_SUB1(gr, row, (col)-1) == GR_FILL) || \
         (GR_SUB1(gr, row, (col)+1) == GR_FILL) || \
         (GR_SUB1(gr, (row)-1, (col)-1) == GR_FILL) || \
         (GR_SUB1(gr, (row)-1, col) == GR_FILL) || \
         (GR_SUB1(gr, (row)-1, (col)+1) == GR_FILL) || \
         (GR_SUB1(gr, (row)+1, (col)-1) == GR_FILL) || \
         (GR_SUB1(gr, (row)+1, col) == GR_FILL) || \
         (GR_SUB1(gr, (row)+1, (col)+1) == GR_FILL))

Bool GridAdjacentIsEmpty(Grid *gr, GridCoord row, GridCoord col,
                         char *wallchars)
{
  return(GridIsEmpty1(gr, row, col, wallchars) ||
         GridIsEmpty1(gr, row, col-1, wallchars) ||
         GridIsEmpty1(gr, row, col+1, wallchars) ||
         GridIsEmpty1(gr, row-1, col-1, wallchars) ||
         GridIsEmpty1(gr, row-1, col, wallchars) ||
         GridIsEmpty1(gr, row-1, col+1, wallchars) ||
         GridIsEmpty1(gr, row+1, col-1, wallchars) ||
         GridIsEmpty1(gr, row+1, col, wallchars) ||
         GridIsEmpty1(gr, row+1, col+1, wallchars));
}

void GridRowColDist(Obj *grid, /* RESULTS */ Float *rowdist, Float *coldist)
{
  *rowdist = ObjToNumber(R1EI(2, &TsNA,
                              L(N("row-distance-of"), grid, ObjWild, E)));
  *coldist = ObjToNumber(R1EI(2, &TsNA,
                              L(N("col-distance-of"), grid, ObjWild, E)));
  if (*rowdist == FLOATNA) *rowdist = 1.0;
  if (*coldist == FLOATNA) *coldist = 1.0;
}

Float GridDistance1(Float rowdist, Float coldist,
                    GridCoord row1, GridCoord col1,
                    GridCoord row2, GridCoord col2)
{
  return(CoordEuclideanDistance(rowdist*((Float)row1), coldist*((Float)col1),
                                rowdist*((Float)row2), coldist*((Float)col2)));
}

Float GridDistance(Obj *grid, GridCoord row1, GridCoord col1, GridCoord row2,
                   GridCoord col2)
{
  Float	rowdist, coldist;
  GridRowColDist(grid, &rowdist, &coldist);
  return(GridDistance1(rowdist, coldist, row1, col1, row2, col2));
}

/* GridSubspace. */


GridSubspace *GridSubspaceCreate(int maxlen, Grid *grid)
{
  GridSubspace *gs;
  gs = CREATE(GridSubspace);
  gs->grid = grid;
  gs->len = 0;
  gs->maxlen = maxlen;
  gs->rows = (GridCoord *)MemAlloc(sizeof(GridCoord)*maxlen, "GridCoord rows");
  gs->cols = (GridCoord *)MemAlloc(sizeof(GridCoord)*maxlen, "GridCoord cols");
  gs->next = NULL;
  return(gs);
}

/* Watch out how this is used because of the sharing. */
GridSubspace *GridSubspaceCopyShared(GridSubspace *old_gs)
{
  GridSubspace *new_gs;
  new_gs = CREATE(GridSubspace);
  new_gs->grid = old_gs->grid;
  new_gs->len = old_gs->len;
  new_gs->maxlen = old_gs->maxlen;
  new_gs->rows = old_gs->rows;
  new_gs->cols = old_gs->cols;
  new_gs->next = NULL;
  return(new_gs);
}

void GridSubspaceAdd(GridSubspace *gs, GridCoord row, GridCoord col)
{
  if (gs->len >= gs->maxlen) {
    if (gs->maxlen <= 0) {
      Dbg(DBGGRID, DBGBAD, "GridSubspaceCreate");
      return;
    }
    gs->maxlen = gs->maxlen*2;
    gs->rows = (GridCoord *)MemRealloc(gs->rows, sizeof(GridCoord)*gs->maxlen,
                                       "GridCoord rows");
    gs->cols = (GridCoord *)MemRealloc(gs->cols, sizeof(GridCoord)*gs->maxlen,
                                       "GridCoord cols");
  }
  gs->rows[gs->len] = row;
  gs->cols[gs->len] = col;
  gs->len++;
}

void GridSubspaceAddGrid(GridSubspace *gs, Grid *gr)
{
  GridCoord	i, j;
  for (i = 0; i < gr->rows; i++) {
    for (j = 0; j < gr->cols; j++) {
      if (GR_SUB(gr, i, j) != GR_EMPTY) GridSubspaceAdd(gs, i, j);
    }
  }
}

void GridSubspaceFree(GridSubspace *gs)
{
  MemFree(gs->rows, "GridCoord rows");
  MemFree(gs->cols, "GridCoord cols");
  MemFree(gs, "GridSubspace");
}

Bool GridSubspaceIn(GridSubspace *gs, GridCoord row, GridCoord col)
{
  int	i;
  for (i = 0; i < gs->len; i++) {
    if (row == gs->rows[i] && col == gs->cols[i]) {
      return(1);
    }
  }
  return(0);
}

Bool GridSubspaceAdjacentIn(GridSubspace *gs, GridCoord row, GridCoord col)
{
  int	i;
  for (i = 0; i < gs->len; i++)
    if (GridAdjacentCell(row, col, gs->rows[i], gs->cols[i])) return(1);
  return(0);
}

GridSubspace *GridSubspaceGetVar(Grid *gr, int var, int onlyone)
{
  int			c;
  GridCoord		i, j;
  GridSubspace	*gs, *r;
  r = NULL;
  for (i = 0; i < gr->rows; i++) {
    for (j = 0; j < gr->cols; j++) {
      c = GR_SUB(gr, i, j);
      if (c == var) {
        for (gs = r; gs; gs = gs->next) {
          if (onlyone || GridSubspaceAdjacentIn(gs, i, j)) break;
        }
        if (!gs) {
          gs = GridSubspaceCreate(1, gr);
          gs->next = r;
          r = gs;
        }
        GridSubspaceAdd(gs, i, j);
      }  
    }
  }
  return(r);
}

Float GridSubspacePathDist(GridSubspace *gs)
{
  int	i;
  Float	rowdist, coldist, dist;
  dist = 0.0;
  GridRowColDist(gs->grid->grid, &rowdist, &coldist);
  for (i = 1; i < gs->len; i++) {
    dist += GridDistance1(rowdist, coldist, gs->rows[i-1], gs->cols[i-1],
                          gs->rows[i], gs->cols[i]);
  }
  return(dist);
}

void GridSubspacePrint(FILE *stream, Ts *ts, GridSubspace *gs, int longformat)
{
  int  i;
  Grid *gr;
  if (gs->len <= 0) {
    fprintf(stream, "[GRIDSUBSPACE]");
  } else if (gs->len == 1) {
    fprintf(stream, "[GRIDSUBSPACE %du %du]",
            gs->rows[0], gs->cols[0]);
  } else if (longformat) {
    if ((gr = GridBuild(ts, gs->grid, 0, TERM, N("walkable")))) {
      GridSetSubspace(gr, gs, GR_FILL);
      GridPrint(stream, "gridsubspace", gr, 1);
      GridFree(gr);
    }
  } else {
    fprintf(stream, "[GRIDSUBSPACE");
    for (i = 0; i < gs->len; i++) {
      fprintf(stream, " %du %du", gs->rows[i], gs->cols[i]);
    }
    fputc(']', stream);
  }
}

void GridSubspaceSocketPrint(Socket *skt, GridSubspace *gs)
{
  int	i;
  char  buf[PHRASELEN];
  SocketWrite(skt, "[GRIDSUBSPACE");
  for (i = 0; i < gs->len; i++) {
#ifdef _GNU_SOURCE
    snprintf(buf, PHRASELEN, " NUMBER:u:%d NUMBER:u:%d",
             gs->rows[i], gs->cols[i]);
#else
    sprintf(buf, " NUMBER:u:%d NUMBER:u:%d", gs->rows[i], gs->cols[i]);
#endif
    SocketWrite(skt, buf);
  }
  SocketWrite(skt, "]");
}

void GridSubspaceTextPrint(Text *text, GridSubspace *gs)
{
  int	i;
  TextPrintf(text, "[GRIDSUBSPACE");
  for (i = 0; i < gs->len; i++) {
    TextPrintf(text, " NUMBER:u:%d NUMBER:u:%d", gs->rows[i], gs->cols[i]);
  }
  TextPrintf(text, "]");
}

/* IntergridPath */

/* igp->wormholes and igp->grids are not initialized. */
IntergridPath *IntergridPathCreate(int len)
{
  IntergridPath *igp;
  igp = CREATE(IntergridPath);
  igp->maxlen = igp->len = len;
  igp->grids = (Obj **)MemAlloc(sizeof(Obj **)*len, "Obj* grids");
  igp->wormholes = (Obj **)MemAlloc(sizeof(Obj **)*len, "Obj* wormholes");
  return(igp);
}

void IntergridPathAddNext(IntergridPath *igp, Obj *grid, Obj *wormhole)
{
  if (igp->len == igp->maxlen) {
    igp->maxlen = igp->maxlen*2;
    igp->grids = (Obj **)MemRealloc(igp->grids, sizeof(Obj **)*igp->maxlen,
                                    "Obj* grids");
    igp->wormholes = (Obj **)MemRealloc(igp->wormholes,
                             sizeof(Obj **)*igp->maxlen, "Obj* wormholes");
  }
  igp->grids[igp->len] = grid;
  igp->wormholes[igp->len] = wormhole;
  igp->len++;
}

void IntergridPathFree(IntergridPath *igp)
{
  MemFree(igp->grids, "Obj* grids");
  MemFree(igp->wormholes, "Obj* wormholes");
  MemFree(igp, "IntergridPath");
}

IntergridPath *IntergridFindPath1(Ts *ts, TsRange *tsr, Obj *gr1, Obj *gr2,
                                  Obj *grvia, int maxdepth)
{
  ObjList		*objs1, *p1, *objs2, *p2;
  Obj			*worm, *grto;
  IntergridPath	*igp;
  objs1 = RDB(ts, tsr, L(N("at-grid"), N("wormhole"), gr1, ObjWild, E), 1);
  for (p1 = objs1; p1; p1 = p1->next) {
    worm = I(p1->obj, 1);
    objs2 = RDB(ts, tsr, L(N("at-grid"), worm, ObjWild, ObjWild, E), 1);
    grto = NULL;
    for (p2 = objs2; p2; p2 = p2->next) {
      if (I(p2->obj, 2) != gr1 && I(p2->obj, 2) != grvia) {
        grto = I(p2->obj, 2);
        break;
      }
    }
    ObjListFree(objs2);
    if (!grto) continue;
    if (grto == gr2) {
      igp = IntergridPathCreate(10);
      igp->len = 0;
      IntergridPathAddNext(igp, gr2, NULL);
      IntergridPathAddNext(igp, gr1, worm);
      ObjListFree(objs1);
      return(igp);
    }
    if (maxdepth == 0) return(NULL);
    if ((igp = IntergridFindPath1(ts, tsr, grto, gr2, gr1, maxdepth-1))) {
      IntergridPathAddNext(igp, gr1, worm);
      ObjListFree(objs1);
      return(igp);
    }
  }
  ObjListFree(objs1);
  return(NULL);
}

/* Result IntergridPath goes from <gr2> to <gr1>.
 * todo: This should find several paths.
 */
IntergridPath *IntergridFindPath(Ts *ts, TsRange *tsr, Obj *gr1, Obj *gr2)
{
  IntergridPath	*igp;
  int			maxdepth;
  for (maxdepth = 1; maxdepth < MAXINTERGRID; maxdepth++) {
    if ((igp = IntergridFindPath1(ts, tsr, gr1, gr2, NULL, maxdepth))) {
      return(igp);
    }
  }
  return(NULL);
}

/* Grid parsing */

/* Assumes last row is sep terminated. */
Grid *GridParse(Obj *grid, char *s, int sep)
{
  GridCoord	rows, cols, row, col;
  char		*p;
  Grid		*gr;
  rows = cols = col = 0;
  for (p = s; *p; p++) {
    if (*p == sep) {
      rows++;
      if (col > cols) cols = col;
      col = 0;
    } else col++;
  }
  gr = GridCreate(grid, rows, cols, (int)GR_EMPTY);
  row = col = 0;
  for (p = s; *p; p++) {
    if (*p == sep) {
      row++;
      col = 0;
    } else {
      GR_SUB(gr, row, col) = *p;
      col++;
    }
  }
  return(gr);
}

Grid *GridRead(FILE *stream, Obj *grid)
{
  char *s;
  Grid *gr;
  Dbg(DBGGRID, DBGDETAIL, "parsing gridstate <%s>", M(grid), E);
  if (NULL == (s = StringReadUntil(stream, GR_SECTION_SEP))) {
    Dbg(DBGGRID, DBGBAD, "GridRead 1");
    return(NULL);
  }
  if (NULL == (gr = GridParse(grid, s, NEWLINE))) {
    Dbg(DBGGRID, DBGBAD, "GridRead 2");
    return(NULL);
  }
  MemFree(s, "char StringReadUntil");
  GridReadVariables(stream, gr);
/*
  GridFindPathTest(gr);
*/
  return(gr);
}

void GridReadVariables(FILE *stream, Grid *gr)
{
  char line[LINELEN];
  while (1) {
    if (!fgets(line, LINELEN, stream)) {
      Dbg(DBGGRID, DBGBAD, "GridReadVariables 1");
      return;
    }
    if (streq(line, GR_SECTION_SEP)) break;
    line[strlen(line)-1] = TERM;
    GridReadVariable(line, 1, gr, stream);
  }
}

void GridReadVariable(char *line, int required, Grid *gr, FILE *stream)
{
  char			fill[FEATLEN], *s;
  int			fillalg, isobjname;
  GridSubspace	*gs;
  Obj			*gridobj;
  if (!GR_IS_VAR(line[0])) {
    Dbg(DBGGRID, DBGBAD, "GridReadVariable: expected variable <%s>", line);
    return;
  }
  if (line[1] == GR_FILLALG_WALL) {
    fillalg = line[1];
    s = StringReadTo(&line[2], fill, FEATLEN, GR_VARSEP, TERM, TERM);
    if (s[0] == TERM) {
      Dbg(DBGGRID, DBGBAD, "GridReadVariable: missing GR_VARSEP <%s>", line);
      return;
    }
  } else {
    fillalg = GR_FILLALG_NA;
    s = &line[2];
  }
  isobjname = (s[0] == GR_OBJNAME);
  if (!(gs = GridSubspaceGetVar(gr, line[0], isobjname))) {
    if (required) {
      Dbg(DBGGRID, DBGBAD, "GridReadVariable: variable <%c> not in grid <%s>",
          line[0], line);
      return;
    }
    return;
  }
  for (; gs; gs = gs->next) {
    if (fillalg != GR_FILLALG_NA) {
      GridSubspaceFill(gr, gs, fill);
    }
    if (!gs->grid) {
      Dbg(DBGGRID, DBGBAD, "GridReadVariable: no grid");
      continue;
    }
    if (!(gridobj = gs->grid->grid)) {
      Dbg(DBGGRID, DBGBAD, "GridReadVariable: no gridobj");
      continue;
    }
    if (isobjname) {
      DbFileAssert(L(N("at-grid"), NameToObj(s+1, OBJ_CREATE_C),
                   gridobj, GridSubspaceToObj(gs), E), 0);
    } else {
      DbFileAssert(L(N("at-grid"),
                   ObjCreateInstance(NameToObj(s, OBJ_CREATE_A), NULL),
                   gridobj, GridSubspaceToObj(gs), E), 0);
    }
    if (DbgOn(DBGGRID, DBGHYPER)) {
      GridSubspacePrint(Log, &TsNA, gs, 1); /* todo: More accurate ts. */
      fputc(NEWLINE, Log);
    }
  }
}

/* Grid algorithms */

#define GridIsFillable(srcgr, destgr, row, col, wallchars) \
  ((GR_SUB((destgr), (row), (col)) == SPACE) && \
   GridIsEmpty((srcgr), (row), (col), (wallchars)) && \
   GridAdjacentIsFilled((destgr), (row), (col)))

void GridFill(Grid *srcgr, Grid *destgr, char *wallchars)
{
  int		change;
  GridCoord	i, j;
  change = 1;
  while (change) {
    change = 0;
    for (i = 0; i < srcgr->rows; i++) {
      for (j = 0; j < srcgr->cols; j++) {
        if (GridIsFillable(srcgr, destgr, i, j, wallchars)) {
          change = 1;
          GR_SUB(destgr, i, j) = GR_FILL;
        }
      }
    }
  }
}

void GridSubspaceFill(Grid *srcgr, GridSubspace *destgs, char *wallchars)
{
  Grid	*fillgr;
  fillgr = GridBlankCopy(srcgr);
  GridSetSubspace(fillgr, destgs, GR_FILL);
  GridFill(srcgr, fillgr, wallchars);
  GridSetSubspace(fillgr, destgs, GR_EMPTY);
  GridSubspaceAddGrid(destgs, fillgr);
  GridFree(fillgr);
}

int GridFindPathMidpoint(Grid *srcgr, GridCoord fromrow, GridCoord fromcol,
                         GridCoord torow, GridCoord tocol, char *wallchars,
                         GridCoord *midrow, GridCoord *midcol)
{
  int			change;
  register Grid		*grfrom, *grto;
  register GridCoord	i, j;
  change = 1;
  grfrom = GridBlankCopy(srcgr);
  grto = GridBlankCopy(srcgr);
  GR_SUB(grfrom, fromrow, fromcol) = GR_FILL;
  GR_SUB(grto, torow, tocol) = GR_FILL;
  while (change) {
    change = 0;
    for (i = 0; i < srcgr->rows; i++) {
      for (j = 0; j < srcgr->cols; j++) {
        if (GridIsFillable(srcgr, grfrom, i, j, wallchars)) {
          change = 1;
          GR_SUB(grfrom, i, j) = GR_FILL;
          if (GR_SUB(grto, i, j) == GR_FILL) {
            *midrow = i;
            *midcol = j;
            GridFree(grfrom);
            GridFree(grto);
            return(1);
          }
          if (DbgOn(DBGGRID, DBGHYPER)) {
            GridPrint(Log, "midpoint grid to", grto, 1);
            GridPrint(Log, "midpoint grid from", grfrom, 1);
          }
        }
        if (GridIsFillable(srcgr, grto, i, j, wallchars)) {
          change = 1;
          GR_SUB(grto, i, j) = GR_FILL;
          if (GR_SUB(grfrom, i, j) == GR_FILL) {
            *midrow = i;
            *midcol = j;
            GridFree(grfrom);
            GridFree(grto);
            return(1);
          }
        }
      }
    }
  }
  GridFree(grfrom);
  GridFree(grto);
  return(0);
}

GridSubspace *GridFindLinearPath(Grid *gr, GridCoord fromrow, GridCoord fromcol,
                                 GridCoord torow, GridCoord tocol,
                                 char *wallchars)
{
  double	m, b;
  int		dir;
  GridCoord	row, col;
  GridSubspace	*gp;
  if (fromrow == torow && fromcol == tocol) {
    gp = GridSubspaceCreate(1, gr);
    gp->len = 1;
    gp->rows[0] = fromrow;
    gp->cols[0] = fromcol;
    return(gp);
  }
  if (GridAdjacentCell(fromrow, fromcol, torow, tocol)) {
    gp = GridSubspaceCreate(2, gr);
    gp->len = 2;
    gp->rows[0] = fromrow;
    gp->cols[0] = fromcol;
    gp->rows[1] = torow;
    gp->cols[1] = tocol;
    return(gp);
  }
  gp = GridSubspaceCreate(50, gr);
  if (abs(tocol-fromcol) > abs(torow-fromrow)) {
    m = (torow - fromrow)/((double)(tocol-fromcol));
    b = fromrow - m*fromcol;
    if (tocol > fromcol) dir = 1; else dir = -1;
    for (col = fromcol;
         (dir == 1) ? (col <= tocol) : (col >= tocol);
         col += dir) { 
      row = (int)(.5 + m*col + b);
      if (!GridIsEmpty(gr, row, col, wallchars)) {
        GridSubspaceFree(gp);
        return(NULL);
      }
    }
    for (col = fromcol;
         (dir == 1) ? (col <= tocol) : (col >= tocol);
         col += dir) { 
      row = (int)(.5 + m*col + b);
      GridSubspaceAdd(gp, row, col);
    }
  } else {
    m = (tocol - fromcol)/((double)(torow-fromrow));
    b = fromcol - m*fromrow;
    if (torow > fromrow) dir = 1; else dir = -1;
    for (row = fromrow;
         (dir == 1) ? (row <= torow) : (row >= torow);
         row += dir) { 
      col = (int)(.5 + m*row + b);
      if (!GridIsEmpty(gr, row, col, wallchars)) {
        GridSubspaceFree(gp);
        return(NULL);
      }
    }
    for (row = fromrow;
         (dir == 1) ? (row <= torow) : (row >= torow);
         row += dir) { 
      col = (int)(.5 + m*row + b);
      GridSubspaceAdd(gp, row, col);
    }
  }
  return(gp);
}

GridSubspace *GridFindPath1(Grid *gr, GridCoord fromrow, GridCoord fromcol,
                            GridCoord torow, GridCoord tocol, char *wallchars)
{
  GridCoord	midrow, midcol;
  GridSubspace	*gp, *gp1, *gp2;
  int		i, j;
  Dbg(DBGGRID, DBGHYPER, "GridFindPath1 %d %d -> %d %d", fromrow, fromcol,
      torow, tocol);
  if ((gp1 = GridFindLinearPath(gr, fromrow, fromcol, torow, tocol,
                                wallchars))) {
    return(gp1);
  }
  if (!GridFindPathMidpoint(gr, fromrow, fromcol, torow, tocol, wallchars,
                            &midrow, &midcol)) {
    return(NULL);
  }
  if (!(gp1 = GridFindPath1(gr, fromrow, fromcol, midrow, midcol, wallchars))) {
    return(NULL);
  }
  if (!(gp2 = GridFindPath1(gr, midrow, midcol, torow, tocol, wallchars))) {
    return(NULL);
  }
  gp = GridSubspaceCreate(gp1->len + gp2->len - 1, gr);
  gp->len = gp->maxlen;
  for (i = 0, j = 0; i < gp1->len; i++, j++) {
    gp->rows[j] = gp1->rows[i];
    gp->cols[j] = gp1->cols[i];
  }
  for (i = 1; i < gp2->len; i++, j++) {
    gp->rows[j] = gp2->rows[i];
    gp->cols[j] = gp2->cols[i];
  }
  GridSubspaceFree(gp1);
  GridSubspaceFree(gp2);
  return(gp);
}

GridSubspace *GridFindPath(Grid *gr, GridCoord fromrow, GridCoord fromcol,
                           GridCoord torow, GridCoord tocol, char *wallchars)
{
  int			fromsave, tosave;
  GridSubspace		*r;
  fromsave = GR_SUB(gr, fromrow, fromcol);
  tosave = GR_SUB(gr, torow, tocol);
  GR_SUB(gr, fromrow, fromcol) = GR_EMPTY;
  GR_SUB(gr, torow, tocol) = GR_EMPTY;
  r = GridFindPath1(gr, fromrow, fromcol, torow, tocol, wallchars);
  GR_SUB(gr, fromrow, fromcol) = fromsave;
  GR_SUB(gr, torow, tocol) = tosave;
  return(r);
}

/* To speed debugging. */
void GridSubspaceSimplify(GridSubspace *gs)
{
  if (gs->len > 2) {
    gs->rows[1] = gs->rows[gs->len-1];
    gs->cols[1] = gs->cols[gs->len-1];
    gs->len = 2;
  }
}

void GridSetSubspace(Grid *to, GridSubspace *from, int fillchar)
{
  int i;
#ifdef maxchecking
  Grid	*fromgrid;
  if (!(fromgrid = from->grid)) {
    Dbg(DBGGRID, DBGBAD, "GridSetSubspace 1");
    return;
  }
  if (fromgrid->rows != to->rows || fromgrid->cols != to->cols) {
    Dbg(DBGGRID, DBGBAD, "GridSetSubspace 2");
    return;
  }
#endif
  for (i = 0; i < from->len; i++) {
    GR_SUB(to, from->rows[i], from->cols[i]) = fillchar;
  }
}

int GridPlopdownText1(Grid *to, char *text, int len,
                      GridCoord row, GridCoord col, int mustbeempty)
{
  int	    i;
  GridCoord origcol;

  if (row < 0 || row >= to->rows) return(0);
  origcol = col;
  for (i = 0; i < len; i++) {
    if (col >= 0 && col < to->cols) {
      if (mustbeempty && (GR_SUB(to, row, col) != GR_EMPTY)) return(0);
    }
    col++;
  }
  col = origcol;
  for (i = 0; i < len; i++) {
    if (col >= 0 && col < to->cols) {
      GR_SUB(to, row, col) = text[i];
    }
    col++;
  }
  return(1);
}

void GridPlopdownText(Grid *to, char *text, GridCoord row, GridCoord col)
{
  int	len;
  len = strlen(text);
  col = col - len/2;

  /* Make visible. */
  if (col < 0) col = 0;
  if ((col + len) > to->cols) col = to->cols - len;

  if (GridPlopdownText1(to, text, len, row, col, 1)) return;
  if (GridPlopdownText1(to, text, len, row, col-1, 1)) return;
  if (GridPlopdownText1(to, text, len, row, col+1, 1)) return;
  if (GridPlopdownText1(to, text, len, row, col-2, 1)) return;
  if (GridPlopdownText1(to, text, len, row, col+2, 1)) return;
  if (GridPlopdownText1(to, text, len, row, col-3, 1)) return;
  if (GridPlopdownText1(to, text, len, row, col+3, 1)) return;

/*
  if (GridPlopdownText1(to, text, len, row-1, col, 1)) return;
  if (GridPlopdownText1(to, text, len, row+1, col, 1)) return;
  if (GridPlopdownText1(to, text, len, row, col-len, 1)) return;
  if (GridPlopdownText1(to, text, len, row-1, col-len, 1)) return;
  if (GridPlopdownText1(to, text, len, row+1, col-len, 1)) return;
  if (GridPlopdownText1(to, text, len, row, col+len, 1)) return;
  if (GridPlopdownText1(to, text, len, row-1, col+len, 1)) return;
  if (GridPlopdownText1(to, text, len, row+1, col+len, 1)) return;
 */
  GridPlopdownText1(to, text, len, row, col, 0);
}

Bool GridFindAccessibleBorder(GridSubspace *gs, Grid *gr, GridCoord *rowp,
                              GridCoord *colp)
{
  int	i;
  for (i = 0; i < gs->len; i++) {
    if (GridAdjacentIsEmpty(gr, gs->rows[i], gs->cols[i], GR_WALLCHARS)) {
      *rowp = gs->rows[i];
      *colp = gs->cols[i];
      return(1);
    }
  }
  return(0);
}

Grid *GridBuild(Ts *ts, Grid *gr, int namestr, int fillchar, Obj *prop)
{
  ObjList		*objs, *p;
  Grid			*r;
  GridSubspace		*gs;
  Obj			*gridobj, *obj;
  char			*objname, objname1[OBJNAMELEN];
  int			inverted;
  if (NULL == gr) {
    Dbg(DBGGRID, DBGBAD, "GridBuild 1");
    return(NULL);
  }
  if (NULL == (gridobj = gr->grid)) {
    Dbg(DBGGRID, DBGBAD, "GridBuild 2");
    return(NULL);
  }
  if (prop == N("flyable")) {
    return(GridCreate(gridobj, gr->rows, gr->cols, (int)GR_EMPTY));
  }
  if ((inverted = ((prop == N("drivable")) || (prop == N("rollable"))))) {
    r = GridCreate(gridobj, gr->rows, gr->cols, (int)GR_FILL);
  } else {
    r = GridCreate(gridobj, gr->rows, gr->cols, (int)GR_EMPTY);
  }
  objs = RE(ts, L(N("at-grid"), ObjWild, gridobj, ObjWild, E));
  for (p = objs; p; p = p->next) {
    obj = ObjIth(p->obj, 1);
    if (namestr) {
      objname = ObjToName(ObjIth(p->obj, 1));
      gs = ObjToGridSubspace(ObjIth(p->obj, 3));
      if (gs->len > 0) {
        if (!ISA(N("human"), obj)) {
          if (!DbIsEnumValue(ts, prop, obj)) {
            objname = ObjToName(obj);
            GridSetSubspace(r, ObjToGridSubspace(ObjIth(p->obj, 3)),
                            (fillchar == TERM) ? objname[0] : fillchar);
          }
        } else {
          sprintf(objname1, "<%s>", objname);
          GridPlopdownText(r, objname1, gs->rows[0], gs->cols[0]);
        }
      }
    } else if (inverted) {
      if (DbIsEnumValue(ts, prop, obj)) {
        GridSetSubspace(r, ObjToGridSubspace(ObjIth(p->obj, 3)), (int)GR_EMPTY);
      }
    } else {
      if (!DbIsEnumValue(ts, prop, obj)) {
        objname = ObjToName(obj);
        GridSetSubspace(r, ObjToGridSubspace(ObjIth(p->obj, 3)),
                        (fillchar == TERM) ? objname[0] : fillchar);
      }
    }
  }
  ObjListFree(objs);
  return(r);
}

/* Grid state cache. Could add support for multiple grids per ts. */

Grid *GridStateGet(Ts *ts, Obj *gridobj, Obj *prop)
{
  if (TsNE(ts, &LastGridTs) || LastGridProp != prop ||
      gridobj != LastGridObj) {
    if (LastGridState) GridFree(LastGridState);
    LastGridState = GridBuild(ts, ObjToGrid(gridobj), 0, GR_FILL, prop);
    LastGridObj = gridobj;
    LastGridTs = *ts;
    LastGridProp = prop;
  } 
  return(LastGridState);
}

Bool GridObjAtRowCol(Obj *grid, GridCoord row, GridCoord col)
{
  return(0);
}

/* End of file. */
