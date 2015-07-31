/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19951108: begun
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

void UA_Trade(Actor *ac, Ts *ts, Obj *a, Obj *in)
{
  ObjList	*products, *forsales, *p, *commentary;
  Intension	*itn;
  if (ISA(N("active-goal"), I(in, 0)) &&
      I(in, 1) == a &&
      ISA(N("buy-from"), I(I(in, 2), 0)) &&
      I(I(in, 2), 1) == a &&
      (!ObjIsNa(I(I(in, 2), 3)))) {
    /* "I want to buy a desk phone." */
    if ((products = UA_AskerSelect(ac->cx, I(I(in, 2), 3), &itn))) {
      commentary = NULL;
      for (p = products; p; p = p->next) {
        /* todo: Filter out old ads. */
        forsales = RE(&TsNA, L(N("for-sale"), p->obj, ObjWild, ObjWild, ObjWild,
                             ObjWild, ObjWild, E));
        commentary = ObjListAppendDestructive(commentary, forsales);
      }
/*
      if ((!(ac->cx->dc->mode & DC_MODE_CONV)) ||
          ObjListLen(products) < 10 || ObjListLen(commentary) < 10) 
 */
      if ((!(ac->cx->dc->mode & DC_MODE_CONV)) ||
          ObjListLen(products) < 18) {
        if (commentary) {
          CommentaryAdd(ac->cx, commentary, NULL);
          ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
        }
      } else {
      /* Too much output. Try to narrow down with a question. An answer
       * to the question should result in this function being reinvoked
       * with something that UA_AskerSelect is able to use to narrow down
       * the selected products.
       */
        if (UA_AskerNarrowDown(ac->cx, 1.0, N("UA_Trade"), itn, products, 
                           L(N("active-goal"), a,
                             L(N("buy-from"),
                               a,
                               I(I(in, 2), 2),
                               N("?answer"),
                               I(I(in, 2), 4),
                               E), E), 1)) {
          ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
        }
      }
    }
  }
}

/* End of file. */
