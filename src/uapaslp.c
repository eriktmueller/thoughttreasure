/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19951130: split off from UA.c and PA_4.c
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
#include "repstr.h"
#include "repsubgl.h"
#include "reptime.h"
#include "semanaph.h"
#include "semaspec.h"
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

/* todo: Alter depending on whether <a> has to work the next day, etc. */
void PA_Sleep_Times(Context *cx, Subgoal *sg, Ts *ts, Obj *a, /* RESULTS */
                    Tod *go_to_sleep, Tod *awaken)
{
  *go_to_sleep = SECONDSPERHOURL*23L;
  *awaken = SECONDSPERHOURL*7L;
}

/* Humans can recharge faster than they uncharge. */
Float PA_Sleep_Recharge(Obj *level, Float elapsed, Float sleeptime)
{
  Float	prev, nouveau, delta;
  prev = ObjWeight(level);
  delta = ((1.0 - prev)*elapsed)/sleeptime;
  nouveau = prev + delta;
  ObjWeightSet(level, nouveau);
  return(nouveau);
}

Float PA_Sleep_Uncharge(Obj *level, Float elapsed, Float sleeptime,
                        Float circadian)
{
  Float	prev, nouveau, delta;
  prev = ObjWeight(level);
  delta = ((prev - 1.0)*elapsed)/(circadian - sleeptime);
  nouveau = prev + delta;
  ObjWeightSet(level, nouveau);
  return(nouveau);
}

/* Returns weighted combination of rest and energy levels. */
Float PA_Sleep_Levels(Context *cx, Subgoal *sg, Ts *ts, Obj *a, int ts_skip,
                      int is_asleep)
{
  Float	elapsed, sleeptime, circadian, delta, rest, energy;
  Tod	go_to_sleep_tod, awaken_tod;
  Ts	go_to_sleep_ts, awaken_ts;
  sleeptime = ObjToNumber(DbGetRelationValue(ts, NULL, N("ideal-sleep-of"), a,
                          NumberToObjClass(8.0*SECONDSPERHOURF, N("second"))));
  circadian = ObjToNumber(DbGetRelationValue(ts, NULL,
                                             N("circadian-rhythm-of"), a,
                          NumberToObjClass(24.0*SECONDSPERHOURF, N("second"))));
  if ((!ts_skip) && TsSameDay(ts, &sg->lastts)) {
  /* Calculate new levels based on change in previous levels. */
    elapsed = (Float)TsMinus(ts, &sg->lastts);
    if (is_asleep) {
      rest = PA_Sleep_Recharge(sg->ac->rest_level, elapsed, sleeptime);
      energy = PA_Sleep_Recharge(sg->ac->energy_level, elapsed, sleeptime);
    } else {
      rest = PA_Sleep_Uncharge(sg->ac->rest_level, elapsed, sleeptime,
                               circadian);
      energy = PA_Sleep_Uncharge(sg->ac->energy_level, elapsed, sleeptime,
                                 circadian);
    }
  } else {
  /* Calculate new levels based on time of day. */
    PA_Sleep_Times(cx, sg, ts, a, &go_to_sleep_tod, &awaken_tod);
    if (is_asleep) {
      TsMidnight(ts, &go_to_sleep_ts);
      TsIncrement(&go_to_sleep_ts, go_to_sleep_tod);
      if (TsGT(&go_to_sleep_ts, ts)) {
        TsDecrement(&go_to_sleep_ts, SECONDSPERDAYL);
      }
      elapsed = (Float)TsMinus(ts, &go_to_sleep_ts);
      delta = (2.0*elapsed)/sleeptime;
      rest = energy = -1.0 + delta;
      ObjWeightSet(sg->ac->rest_level, rest);
      ObjWeightSet(sg->ac->energy_level, energy);
    } else {
      TsMidnight(ts, &awaken_ts);
      TsIncrement(&awaken_ts, awaken_tod);
      if (TsGT(&awaken_ts, ts)) TsDecrement(&awaken_ts, SECONDSPERDAYL);
      elapsed = (Float)TsMinus(ts, &awaken_ts);
      delta = -(2.0*elapsed)/(circadian - sleeptime);
      rest = energy = 1.0 + delta;
      ObjWeightSet(sg->ac->rest_level, rest);
      ObjWeightSet(sg->ac->energy_level, energy);
    }
  }
  return(0.8*rest + 0.2*energy);
}

void PA_Sleep(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Tod	go_to_sleep_tod, awaken_tod;
  Ts	go_to_sleep_ts;
#ifndef MICRO
  Ts	awaken_ts;
#endif
  Dbg(DBGPLAN, DBGOK, "PA_Sleep");
  switch (sg->state) {
    case STBEGIN:
      PA_Sleep_Levels(cx, sg, ts, a, 1, 0);
      TE(ts, L(N("asleep"), a, E));
      AS(ts, 0, L(N("awake"), a, E));
      TOSTATE(cx, sg, 100);
      return;
    /* Awake: monitor sleepiness. */
    case 100:
      if (cx->mode == MODE_SPINNING && sg->spin_to_state >= 200) {
        TOSTATE(cx, sg, 200);
        return;
      }
      if (-0.9 > PA_Sleep_Levels(cx, sg, ts, a, 0, 0)) {
        TOSTATE(cx, sg, 200);
        return;
      }
      PA_Sleep_Times(cx, sg, ts, a, &go_to_sleep_tod, &awaken_tod);
      TsMidnight(ts, &go_to_sleep_ts);
      TsIncrement(&go_to_sleep_ts, go_to_sleep_tod);
      /* todo: Night watchpersons. */
      if (TsGT(ts, &go_to_sleep_ts)) {
        TOSTATE(cx, sg, 200);
        return;
      }
      WAIT_TS(cx, sg, ts, SECONDSPERMINL*15L, 100);
      return;
    /* Sleepy enough to want to sleep, or usual time to go to sleep. */
    case 200:
      if (!FINDO(cx, ts, N("bed"), o, N("bed"), a, a)) goto failure;
      SG(cx, sg, 210, STFAILURE,
         L(N("near-reachable"), a, GETO(ts, N("bed"), o), E));
      return;
    case 210:
      SG(cx, sg, 220, STFAILURE, L(N("strip"), a, E));
        /* todo: Optionally put on pajamas. */
        /* todo: Set alarm clock. */
        /* todo: Get out of appointments gracefully. */
      return;
    case 220:
#ifdef MICRO
      SG(cx, sg, 400, STFAILURE, L(N("lie-on"), a, GETO(ts, N("bed"), o), E));
#else
      SG(cx, sg, 300, STFAILURE, L(N("lie-on"), a, GETO(ts, N("bed"), o), E));
#endif
      return;
    /* Try to fall asleep. */
    case 300:
      if (-0.8 > PA_Sleep_Levels(cx, sg, ts, a, 0, 0)) {
        TOSTATE(cx, sg, 400);
        return;
      }
      WAIT_TS(cx, sg, ts, SECONDSPERMINL*1L, 300);
      return;
    /* Asleep. */
    case 400:
      TE(ts, L(N("awake"), a, E));
      AS(ts, 0, L(N("asleep"), a, E));
      TOSTATE(cx, sg, 410);
      return;
    case 410:
      if (cx->mode == MODE_SPINNING && sg->spin_to_state == 100) {
#ifdef MICRO
        TsIncrement(ts, 15L);
#else
        PA_Sleep_Times(cx, sg, ts, a, &go_to_sleep_tod, &awaken_tod);
        TsMidnight(ts, &awaken_ts);
        TsIncrement(&awaken_ts, awaken_tod);
        *ts = awaken_ts;
#endif
        TOSTATE(cx, sg, STBEGIN);
        return;
      }
      /* todo: Dreaming and sleep cycles. */                    
      if (0.9 < PA_Sleep_Levels(cx, sg, ts, a, 0, 1)) {
        TOSTATE(cx, sg, 500);
        return;
      }
      WAIT_PTN(cx, sg, 0, 500, L(N("wake"), ObjWild, a, E));
      /* todo: Also, alarm clock, telephone, wakes you up.
       * You don't do anything while you are asleep except think.
       */
      WAIT_TS(cx, sg, ts, SECONDSPERMINL*15L, 410);
      return;
    /* Awaken. */
    case 500:
      /* todo: Take shower, get dressed, etc. */
      TE(ts, L(N("asleep"), a, E));
      AS(ts, 0, L(N("awake"), a, E));
      TOSTATE(cx, sg, 100);
      return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Sleep: undefined state %d", sg->state);
  }
failure:
  TOSTATE(cx, sg, STFAILURE);
}

/*
 * Examples based on (Longman, 1992).
 * VERB:
 * aspect-generality:
 *  Nurse Burnley works all night and sleeps all day.
 * aspect-inaccomplished-situational:
 *  Jim was sleeping and his mother didn't want to wake him up.
 *  Jim is sleeping.
 *  It was nine o'clock and Nicky was still asleep.
 *  Jim is lying asleep in his bed.
 *  Jim was lying asleep in his bed.
 *  Jim is having trouble falling asleep.
 * aspect-accomplished-situational:
 *  I had slept only a few hours but I had to finish packing my bags
 *  to get ready for the journey.
 * conditional:
 *  If my snoring is that bad I'll go down and sleep on the sofa.
 * aspect-changed-situation:
 *   Jim went to sleep.
 *   Jim fell asleep.
 * ADJECTIVE:
 *  I watched the sleeping child, the gentle rise and fall of her breast.
 *  We found Mom asleep on the sofa.
 *  Mom is asleep.
 * For more examples, see
 * (Cambridge University Press, 1994, p. 127)
 * and (Longman, 1993, p. 1244).
 */
void UA_Sleep(Actor *ac, Ts *ts, Obj *a, Obj *in)
{
  Subgoal	*sg;
  Discourse	*dc;
  Obj		*most_recent;
  ObjList	*known;
  TsRange	tsr24;
  dc = ac->cx->dc;
  if (NULL == (sg = SubgoalFind(ac, N("sleep")))) {
    Dbg(DBGUA, DBGBAD, "UA_Sleep <%s>", M(a));
    return;
  }
  if (ISA(N("asleep"), I(in, 0)) && I(in, 1) ==  a) {
    if (AspectIsAccomplished(dc)) {
      /* Jim had/has slept (for 8 hours). Jim slept for 8 hours. */
      TsRangeLast24Hours(ts, &tsr24);
      switch (sg->state) {
        case 100:
          known = RER(&tsr24, L(N("asleep"), a, E));
          most_recent = ObjListMostRecent(known);
          ObjListFree(known);
          if (most_recent) {
            if (dc->abs_dur != DURNA) {
              if (LongAbs(TsRangeDur(ObjToTsRange(most_recent)) - dc->abs_dur)
                  < SECONDSPERMINL*30L) {
                ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL,
                              NOVELTY_EXPECTED);
                return;
              } else {
                ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_MOSTLY,
                              NOVELTY_HALF);
                return;
              }
            } else {
              ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL,
                            NOVELTY_EXPECTED);
              return;
            }
          } else {
          /* If we are in state 100, then except on initialization this
           * should not happen since there should have been an asserted
           * asleep 
           */
            ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL,
                          NOVELTY_EXPECTED);
          }
          return;
        case 400:
        case 410:
          if (dc->abs_dur != DURNA) {
          /* Move time ahead and let PA_Sleep take over from state 410. */
            TsIncrement(ts, dc->abs_dur);
          }
          ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_EXPECTED);
          PA_SpinTo(ac->cx, sg, 100);
          return;
        case 200:
        case 210:
        case 220:
        case 300:
        case 500:
        default:
          ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_MOSTLY);
          ObjWeightSet(sg->ac->rest_level, 1.0);
          ObjWeightSet(sg->ac->energy_level, 1.0);
          PA_SpinTo(ac->cx, sg, 100);
          return;
        }
    } else if (dc->tense == NULL ||
        AspectIsCompatibleTense(N("aspect-inaccomplished-situational"),
                                dc->tense, DC(dc).lang)) {
      /* Jim was/is sleeping. Jim slept. Jim was asleep. */
      if (sg->state == 410) {
        ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_NONE);
        return;
      } else {
        ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
        ObjWeightSet(sg->ac->rest_level, -1.0);
        ObjWeightSet(sg->ac->energy_level, -1.0);
        PA_SpinTo(ac->cx, sg, 410);
        return;
      }
    }
  } else if (ISA(N("awake"), I(in, 0)) && I(in, 1) ==  a) {
    /* Jim woke up. */
    if (sg->state == 100) {
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_NONE);
      return;
    } else {
      ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
      ObjWeightSet(sg->ac->rest_level, 1.0);
      ObjWeightSet(sg->ac->energy_level, 1.0);
      PA_SpinTo(ac->cx, sg, 100);
      return;
    }
  }
}

/* End of file. */
