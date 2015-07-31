/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "pa.h"
#include "repbasic.h"
#include "repchan.h"
#include "repcxt.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
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

void UA_SpaceActorMove(Context *cx, Ts *ts, Obj *actor,
                       Obj *to_grid, GridCoord to_row, GridCoord to_col)
{
  TE(ts, L(N("at-grid"), actor, ObjWild, ObjWild, E));
  AS(ts, 0L, ObjAtGridCreate(actor, to_grid, to_row, to_col));
    /* todo: Delete previous grid? Is this list even necessary?
     * We can always build this by looping through all actors.
     * Note UA_Weather uses this.
     */
}

void UA_Space_ActorTransportNear(Context *cx, Ts *ts, Obj *actor, Obj *to_grid,
                                 Obj *nearx)
{
  Grid		*gr;
  GridCoord	to_row, to_col;
  Obj		*polity, *gridr;
  if (nearx && SpaceLocateObject(ts, NULL, nearx, to_grid, 0, &polity, &gridr,
                                &to_row, &to_col) &&
      gridr == to_grid) {
  /* We've got the row and col. */
  } else {
    if ((gr = ObjToGrid(to_grid))) {
    /* todo: GridFindAccessibleBorder equivalent. */
      to_row = gr->rows/2;
      to_col = gr->cols/2;
    } else {
      to_row = to_col = 1;
    }
  }
  UA_SpaceActorMove(cx, ts, actor, to_grid, to_row, to_col);
}

void UA_Space(Context *cx, Ts *ts, Obj *in)
{
  Obj		*actor, *from, *to;
  Subgoal	*sg;
  if (GetSpatial(in, &from, &to)) {
    actor = I(in, 1);
    if (ISA(N("animate-object"), actor) && !ObjIsNa(to)) {
      sg = TG(cx, ts, actor, L(N("near-reachable"), actor, to, E));
      ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_MOSTLY, NOVELTY_TOTAL);
      PA_SpinTo(cx, sg, STSUCCESS);
      return;
    }
  }
}

/* End of file. */
