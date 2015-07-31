/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
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

Bool WeatherPronoun(Obj *obj)
{
  return(ISA(N("pronoun-it-expletive"), obj) ||
         ISA(N("pronoun-ce-expletive"), obj));
}

Bool WeatherPronounAir(Obj *obj)
{
  return(WeatherPronoun(obj) || N("air") == obj);
}

Bool WeatherPronounSky(Obj *obj)
{
  return(WeatherPronoun(obj) || N("sky") == obj);
}

void UA_Weather1(Context *cx, Ts *ts, Obj *in, Obj *grid)
{
  Obj	*head, *obj;
  if (I(in, 0) == N("not") &&
      (head = I(I(in, 1), 0)) &&
      (obj = I(I(in, 1), 1)) &&
      ISA(N("weather-evaluation"), head) &&
      WeatherPronoun(obj)) {
  /* "It was not a beautiful day." [not [beautiful-day pronoun-ce-expletive]]
   * todo: Generalize this.
   */
    ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
    if (ISA(N("good-weather"), head)) {
      AS(ts, 0L, L(N("bad-weather"), grid, E));
    } else if (ISA(N("bad-weather"), head)) {
      AS(ts, 0L, L(N("good-weather"), grid, E));
    }
    return;
  }
  if (!(head = I(in, 0))) return;
  if (!(obj = I(in, 1))) return;
  if (ISA(N("weather-evaluation"), head) && WeatherPronoun(obj)) {
  /* "It was beautiful."      [beautiful-day pronoun-it-expletive]
   */
    goto assertit;
  } else if (ISA(N("sky-coverage"), head) && WeatherPronounSky(obj)) {
  /* "The sky was overcast."  [overcast-sky sky]
   * "It was sunny."          [sunny pronoun-it-expletive]
   */
    goto assertit;
  } else if (ISA(N("humidity-description"), head) && WeatherPronounAir(obj)) {
  /* "The air was dry." [dry air]
   * "It was humid".    [humid pronoun-it-expletive]
   */
    goto assertit;
  } else if (ISA(N("temperature-description"), head) &&
             WeatherPronounAir(obj)) {
  /* "The air was cold."  [cold air]
   * "It was freezing."   [freezing pronoun-it-expletive]
   */
    goto assertit;
  }
  return;
assertit:
  ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
  AS(ts, 0L, L(head, grid, E));
}

/* todo: Make use of tense "imperfect-indicative".
 * todo: Better identification of which grid is being referred to.
 */
void UA_Weather(Context *cx, Ts *ts, Obj *in)
{
  ObjList	*grids;
  if ((grids = ContextFindGrids(cx))) {
    UA_Weather1(cx, ts, in, grids->obj);
  }
}

/* End of file. */
