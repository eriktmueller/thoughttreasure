/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19950327: begun
 * 19950328: more work
 * 19950329: more work
 * 19950330: more work
 * 19950401: more work
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "pa.h"
#include "repactor.h"
#include "repbasic.h"
#include "repchan.h"
#include "repcxt.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "repstr.h"
#include "repsubgl.h"
#include "reptrip.h"
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

void AppointmentGetRoles(Obj *appointment, /* RESULTS */
                         Obj **dest, Obj **location, Obj **goal,
                         TsRange *tsr)
{
  if (I(appointment, 2)) {
    *dest = I(appointment, 2);
  } else {
    *dest = ObjNA;
  }
  if (I(appointment, 3)) {
    *location = I(appointment, 3);
  } else {
    *location = ObjNA;
  }
  if (I(appointment, 4)) {
    *goal = I(appointment, 4);
  } else {
    *goal = ObjNA;
  }
  if (ObjIsTsRangeNC(I(appointment, 5))) {
    *tsr = *ObjToTsRange(I(appointment, 5));
  } else {
    *tsr = *ObjToTsRange(appointment);
  }
}

#define APMO(o1, o2) ((o1) == ObjNA || (o2) == ObjNA || ObjSimilarList(o1, o2))
#define APMT(tsr1, tsr2) (TsRangeIsNaIgnoreCx(tsr1) || \
                          TsRangeIsNaIgnoreCx(tsr2) || \
                          TsRangeEqual(tsr1, tsr2))

Bool AppointmentMatches(Obj *dest1, Obj *location1, Obj *goal1, TsRange *tsr1,
                        Obj *dest2, Obj *location2, Obj *goal2, TsRange *tsr2)
{
  return(APMO(dest1, dest2) &&
         APMO(location1, location2) &&
         APMO(goal1, goal2) &&
         APMT(tsr1, tsr2));
}

Float AppointmentMakesSense(Obj *appointment)
{
  /* If appointment in cyberspace, parties should have a connection to each
   * other.
   * If appointment in physical space,
   *   location should be near enough both parties, except for voyage goals,
   *   and both parties should be able to move (have working legs, which
   *   ThoughtTreasure does not).
   */
  return(SENSE_TOTAL);
}

/* todo: Add /stand* up#R.Véz/poser* un#D lapin#NM.Vy/ */
void AppointmentNoShow(Context *cx, Subgoal *sg, Obj *noshow_actor)
{
  sg->failure_causes = ObjListCreate(L(N("action"), noshow_actor, E),
                                     sg->failure_causes);
  TOSTATE(cx, sg, STFAILURE);
}

Subgoal *AppointmentAdd(Actor *ac, Ts *ts, Obj *a, Obj *dest, Obj *location,
                        Obj *goal, TsRange *tsr)
{
  Obj	*appointment;
  appointment = L(N("appointment"), a, dest, location, goal,
                  TsRangeToObj(tsr), E);
  if (TsRangeIsNa(tsr)) Stop();
  UA_AppointmentAsker(ac, appointment);
  ContextSetSense(ac->cx, AppointmentMakesSense(appointment));
  return(TG(ac->cx, ts, a, appointment));
}

void AppointmentCancel(Actor *ac, Ts *ts, Obj *a, Subgoal *sg,
                       Obj *cancel_action)
{
  sg->failure_causes = ObjListCreate(cancel_action, sg->failure_causes);
  TOSTATE(ac->cx, sg, STFAILURE);
}

/* todo: Set cx->reinterp with information in sg but not in in. */
void AppointmentMod(Actor *ac, Obj *appointment, Obj *in_value, Obj *sg_value,
                    int theta_role_i)
{
  if (in_value == ObjNA || ObjSimilarList(in_value, sg_value)) {
  /* The value is NA or the same as previously known. */
    ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_NONE);
/*
  } else if (sg_value == ObjNA) {
     Filling in previously missing information. Code is same as below.
 */
  } else {
  /* The value is different: Assume it has been changed. Another
   * (unhandled) possibility is that there is more than one value
   * (especially in the case of goals).
   */
    ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_MOSTLY, NOVELTY_MOSTLY);
    ObjSetIth(appointment, theta_role_i, in_value);
  }
}

void PA_Appointment_LeaveTs(Subgoal *sg, Obj *a, Obj *counter, Obj *to, Ts *ts,
                            TsRange *appointment_tsr,
                            /* RESULTS */ Ts *tsleave_at, Ts *tsleave_before)
{
  Trip	*trip;
  if ((trip = TripFind(a, a, to, ts, &appointment_tsr->startts))) {
  /* todo: efficiency problem */
    *tsleave_at = *tsleave_before = trip->depart;
  } else {
    *tsleave_at = *tsleave_before = appointment_tsr->startts;
    TsIncrement(tsleave_at, 30L);
    TsIncrement(tsleave_before, 30L);
  }
  /* todo: This assumes regularly available transportation, which might not
   * be the case.
   */
  TsDecrement(tsleave_at, HumanPunctuality(a, sg->obj));
  TsIncrement(tsleave_before, HumanWaitLimit(counter, sg->obj));
}

/******************************************************************************
 * <a> changes location or a location of <a> is provided.
 ******************************************************************************/
void UA_Appointment_Spatial(Actor *ac, Ts *ts, Obj *a, Obj *from, Obj *to)
{
  Obj		*sg_dest, *sg_location, *sg_goal;
  Ts		tswait_until, tsleave_at, tsleave_before;
  TsRange	sg_tsr;
  Subgoal	*sg;
  for (sg = ac->subgoals; sg; sg = sg->next) {
    if (!ISA(N("appointment"), I(sg->obj, 0))) continue;
    if (SubgoalStateIsOutcome(sg->state)) continue;
    AppointmentGetRoles(sg->obj, &sg_dest, &sg_location, &sg_goal, &sg_tsr);
    if (sg->state == 100) {
      if (SpaceIsNearReachable(ts, NULL, to, sg_location)) {
        /* Moving to place of appointment. Spin the simulation from leaving
         * time. todo: ts should be reasonably near sg_tsr.
         */
        PA_Appointment_LeaveTs(sg, a, sg_dest, sg_location, &sg->ts, &sg_tsr,
                               &tsleave_at, &tsleave_before);
        sg->ts = tsleave_at;
        ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_MOSTLY, NOVELTY_MOSTLY);
        ContextAddMakeSenseReason(ac->cx, sg->obj);
        PA_SpinTo(ac->cx, sg, 110);
      }
    } else if (sg->state == 110) {
      if (!SpaceIsNearReachable(ts, NULL, to, sg_location)) {
      /* Moving out of place of appointment before other person arrives. */
        tswait_until = sg_tsr.startts;
        TsIncrement(&tswait_until, HumanWaitLimit(a, sg->obj));
        if (TsLT(&sg->ts, &tswait_until)) {
          sg->ts = tswait_until;
        } else {
        /* Something is weird. */
          TsIncrement(&sg->ts, 1L);
        }
        ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_MOSTLY, NOVELTY_MOSTLY);
        ContextAddMakeSenseReason(ac->cx,
                                 L(N("not"), L(N("location-of"), sg_dest,
                                               sg_location, E)));
        /* REDUNDANT: We could do nothing and PA_Appointment step 110 should
         * do the below. But in the interest of redundancy, which I actually
         * think is good, we do this right now:
         */
        AppointmentNoShow(ac->cx, sg, I(sg->obj, 2));
      }
    }
  }
}

/******************************************************************************
 * called from PA_HandleProposal
 ******************************************************************************/
Bool UA_Appointment_IsProposalAccepted(Actor *ac, Ts *ts, Obj *a, Obj *in,
                                       /* RESULTS */
                                       ObjList **reasons_for_decision)
{
  /* todo: Inelegant. */
  ContextRSNReset(ac->cx);
  ac->cx->makes_sense_reasons = NULL;
  ac->cx->not_make_sense_reasons = NULL;
  UA_Appointment(ac, ts, a, in);
  if (ac->cx->rsn.sense > SENSE_HALF) {
    *reasons_for_decision = ac->cx->makes_sense_reasons;
    return(1);
  } else {
    *reasons_for_decision = ac->cx->not_make_sense_reasons;
    return(0);
  }
}

/* "I make an appointment with him."
 * "I have an appointment with him."
 * "He makes an appointment with me."
 * "I'm having dinner with Mary."
 */
void UA_AppointmentMake(Actor *ac, Ts *ts, Obj *a, Obj *in, Obj *in_counter,
                        Obj *in_location, Obj *in_goal, TsRange *in_tsr)
{
  Float		sense;
  Obj		*sg_dest, *sg_location, *sg_goal;
  TsRange	sg_tsr;
  Subgoal	*sg;
  for (sg = ac->subgoals; sg; sg = sg->next) {
    if (!ISA(N("appointment"), I(sg->obj, 0))) continue;
    if (SubgoalStateIsOutcome(sg->state)) continue;
    AppointmentGetRoles(sg->obj, &sg_dest, &sg_location, &sg_goal, &sg_tsr);
    if (TsRangeOverlaps(in_tsr, &sg_tsr)) {
    /* Found an existing appointment which overlaps with the input
     * appointment.
     */
      if (in_counter == sg_dest) {
      /* That existing appointment was with the same person. Hence,
       * it is probably the same appointment as the input. Alter it as
       * necessary.
       * todo: Consider various values of sg_pred?
       */
        AppointmentMod(ac, sg->obj, in_goal, sg_goal, 4);
        AppointmentMod(ac, sg->obj, in_location, sg_location, 3);
        AppointmentMod(ac, sg->obj, TsRangeToObj(in_tsr),
                       TsRangeToObj(&sg_tsr), 5);
        UA_AppointmentAsker(ac, sg->obj);
        return;
      } else {
      /* That existing appointment was with another person. Thus this
       * is a new appointment which conflicts with an existing one.
       * todo: A whiff of negative emotion due to the conflict.
       * todo: Active/infer goal to cancel appointment which conflicts.
       */
        ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_LITTLE, NOVELTY_TOTAL);
        ContextAddNotMakeSenseReason(ac->cx, sg->obj);
        AppointmentAdd(ac, ts, a, in_counter, in_location, in_goal, in_tsr);
        return;
      }
    }
  }
  /* No existing appointment overlaps in time.
   * This is a new appointment. (todo: treat case where this is actually
   * a rescheduling.) todo: Treat omission of tsr?
   */

  /* The more information we get, the more sense this makes. */
  sense = SENSE_TOTAL;
  if (TsRangeIsNa(in_tsr)) sense -= 0.05;
  if (ObjIsNa(in_goal)) sense -= 0.05;
  if (ObjIsNa(in_location)) sense -= 0.05;
  if (ObjIsNa(in_counter)) sense -= 0.05;

  ContextSetRSN(ac->cx, RELEVANCE_TOTAL, sense, NOVELTY_TOTAL);
  sg = AppointmentAdd(ac, ts, a, in_counter, in_location, in_goal, in_tsr);
/*
  PA_SpinTo(ac->cx, sg, 110);
 */
}

void UA_AppointmentAsker(Actor *ac, Obj *appointment)
{
  if (!(ac->cx->dc->mode & DC_MODE_CONV)) return;
  if (ObjIsNa(I(appointment, 3))) {
  /* "Where?"/"Where is your appointment with Mary?" */
    UA_AskerQWQ(ac->cx, 0.5, N("UA_Appointment"),
                N("location"),
                N("location-interrogative-pronoun1"),
                L(N("appointment"), I(appointment, 1), I(appointment, 2),
                  N("location-interrogative-pronoun"),
                  I(appointment, 4), I(appointment, 5), E),
                  /* We don't use NULL here because of French "Où ça?" */
                L(N("appointment"), I(appointment, 1), I(appointment, 2),
                  N("?answer"), I(appointment, 4), I(appointment, 5), E));
  }
  if (ObjIsNa(I(appointment, 5))) {
  /* "When?"/"When is your appointment with Mary?" */
    UA_AskerQWQ(ac->cx, 0.8, N("UA_Appointment"),
                L(N("or"), N("tod"),
                           N("timestamp"),
                           N("time-range"), E), /* todo */
                N("absolute-time-interrogative-adverb"),
      /* todo: Adverb instead of pronoun OK here? */
                NULL,
                L(N("appointment"), I(appointment, 1), I(appointment, 2),
                  I(appointment, 3), I(appointment, 4), N("?answer"), E));
  /* todo: Use N("time-of-day-interrogative-adverb") if date but not tod
   * specified.
   */
  }
  /* todo: Dest and goal slots. */
}

/* todo: "I'm not having dinner with Toni." */
void UA_AppointmentCancel(Actor *ac, Ts *ts, Obj *a, Obj *in, Obj *in_counter,
                          Obj *in_location, Obj *in_goal, TsRange *in_tsr)
{
  int		found;
  Obj		*sg_dest, *sg_location, *sg_goal;
  TsRange	sg_tsr;
  Subgoal	*sg;
  found = 0;
  for (sg = ac->subgoals; sg; sg = sg->next) {
    if (!ISA(N("appointment"), I(sg->obj, 0))) continue;
    if (SubgoalStateIsOutcome(sg->state)) continue;
    AppointmentGetRoles(sg->obj, &sg_dest, &sg_location, &sg_goal, &sg_tsr);
    if (AppointmentMatches(in_counter, in_location, in_goal, in_tsr,
                           sg_dest, sg_location, sg_goal, &sg_tsr)) {
      found = 1;
      if (!SubgoalStateIsOutcome(sg->state)) {
         ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_TOTAL);
      } else {
        ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_LITTLE, NOVELTY_TOTAL);
        ContextAddNotMakeSenseReason(ac->cx, sg->cur_goal);
      }
      AppointmentCancel(ac, ts, a, sg, in);
    }
    /* todo: We allow all matching appointments to be canceled. Is this
     * right?
     */
  }
  if (!found) {
  /* Canceling an appointment not known to exist. */
    sg = AppointmentAdd(ac, ts, a, in_counter, in_location, in_goal,
                        in_tsr);
    AppointmentCancel(ac, ts, a, sg, in);
    ContextSetRSN(ac->cx, RELEVANCE_TOTAL, SENSE_LITTLE, NOVELTY_TOTAL);
    ContextAddNotMakeSenseReason(ac->cx,
         L(N("not"), L(N("appointment"), a, in_counter, E), E));
  }
}

/******************************************************************************
 * Top-level Appointment Understanding Agent.
 ******************************************************************************/
void UA_Appointment(Actor *ac, Ts *ts, Obj *a, Obj *in)
{
  Obj	*in_pred, *in_counter, *in_location, *in_goal, *dummy, *from, *to;
  TsRange	in_tsr;
  /* todo: Infer [dinner] goal from restaurant location. */
  in_pred = I(in, 0);
  if (GetActorSpatial(in, a, &from, &to)) {
    UA_Appointment_Spatial(ac, ts, a, from, to);
  } else if (ISA(N("appointment-script"), in_pred) &&
             GetActorPair(in, a, &in_counter)) {
    AppointmentGetRoles(in, &dummy, &in_location, &in_goal, &in_tsr);
    if (ISA(N("make-appointment"), in_pred) || ISA(N("appointment"), in_pred)) {
      UA_AppointmentMake(ac, ts, a, in, in_counter, in_location, in_goal,
                         &in_tsr);
    } else if (ISA(N("reschedule-appointment"), in_pred)) {
    /* todo: Add this case. */
    } else if (ISA(N("cancel-appointment"), in_pred)) {
      UA_AppointmentCancel(ac, ts, a, in, in_counter, in_location, in_goal,
                           &in_tsr);
    }
  } else if (ISA(N("eat-meal"), in_pred) &&
             GetActorPair(in, a, &in_counter)) {
  /* todo: This applies to every script? */
    in_goal = in;
    in_tsr = *ObjToTsRange(in);
    in_location = NULL;	/* todo */
    UA_AppointmentMake(ac, ts, a, in, in_counter, in_location, in_goal,
                       &in_tsr);
  }
}

ObjList *UA_AppointmentGetList(Context *cx, Obj *a)
{
  Actor	*ac;
  if (NULL == (ac = ContextActorFind(cx, a, 0))) return(NULL);
  return(ActorFindSubgoalsHead(ac, N("appointment")));
}

/* "What are my appointments for tonight?"
 * "What am I doing tomorrow night?"
 * "List my appointments."
 * "Show my appointments for tonight."
 * "Please display my calendar."
 * [program-output TT calendar29] where [owner-of calendar29 Jim]
 * [program-output TT [such-that calendar [owner-of calendar Jim]]]
 * [program-output TT [appointment Jim]]
 */
Bool UA_AppointmentQuestionIsListCommand(Obj *question, TsRange *tsr0,
                                         /* RESULTS */ Obj **actor,
                                         TsRange **tsr)
{
  Obj		*noun, *owner;
  ObjList	*props;
  if (ISA(N("program-output"), I(question, 0)) &&
      I(question, 1) == Me) {
    if (!TsRangeIsNa(ObjToTsRange(question))) {
      *tsr = ObjToTsRange(question);
    } else {
      *tsr = tsr0;
    }
    if (ISA(N("calendar"), I(question, 2)) &&
        (owner = R1EI(2, &TsNA, L(N("owner-of"), I(question, 2), ObjWild,E)))) {
    /* todo: Probably this and other db queries need to at least use a
     * contextized TsNA.
     */
      *actor = owner;
      return(1);
    } else if ((props = ObjIntensionParse(I(question, 2), 0, &noun)) &&
        ISA(N("calendar"), noun) &&
        N("owner-of") == I(props->obj, 0) &&
        N("calendar") == I(props->obj, 1)) {
      *actor = I(props->obj, 2);
      return(1);
    } else if (ISA(N("appointment"), I(I(question, 2), 0)) &&
               (!ObjIsNa(I(I(question, 2), 1)))) {
      *actor = I(I(question, 2), 1);
      return(1);
    }
  }
  /* todo */
  return(0);
}

Answer *UA_AppointmentQuestionList(Obj *question, Context *cx, Obj *actor,
                                   TsRange *tsr, Answer *an, Discourse *dc)
{
  ObjList	*p, *appointments, *answer;

  appointments = UA_AppointmentGetList(cx, actor);
  if (TsRangeIsNa(tsr)) {
    answer = appointments;
  } else {
    answer = NULL;
    for (p = appointments; p; p = p->next) {
      if (TsRangeOverlaps(ObjToTsRange(I(p->obj, 5)), tsr)) {
        answer = ObjListCreate(p->obj, answer);
      }
    }
    ObjListFree(appointments);
  }
  if (answer) {
    an = AnswerCreateQWQ(N("UA_AppointmentQuestion"), question, SENSE_TOTAL,
                         answer, an);
  } else {
  /* todo:
   * "You don't have any appointments tonight."
   * "Nothing."
   * "Your calendar for tonight is empty."
   * etc. depending on exact question.
   */
  }
  return(an);
}

Answer *UA_AppointmentQuestion(Obj *question, Context *cx, TsRange *tsr0,
                               Answer *an, Discourse *dc)
{
  Obj		*actor;
  TsRange	*tsr;
  if (UA_AppointmentQuestionIsListCommand(question, tsr0, &actor, &tsr)) {
    an = UA_AppointmentQuestionList(question, cx, actor, tsr, an, dc);
  }
  return(an);
}

void PA_Appointment(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Ts		tsleave_at, tsleave_before, tswait_until;
  TsRange	*tsr;
  Friend	*f;
  Dbg(DBGPLAN, DBGOK, "PA_Appointment", E);
  switch (sg->state) {
    case STBEGIN:
      TOSTATE(cx, sg, 10);
      return;
    case 10:
/*
   todo: How can we get this to succeed in spinning?
      SG(cx, sg, 100, STFAILURE,
         L(N("obtain-permission"), a, I(o, 2),
           L(N("appointment"), a, I(o, 2), I(o, 3), I(o, 4), I(o, 5), E),
           E));
 */
      TOSTATE(cx, sg, 100);
      return;
    /* WAITING TO LEAVE FOR APPOINTMENT. */
    case 100:
      PA_Appointment_LeaveTs(sg, a, I(o, 2),
                             I(o, 3), ts,
                             ObjToTsRange(I(o, 5)),
                             &tsleave_at, &tsleave_before);
      WAIT_TS(cx, sg, &tsleave_at, 0L, 101);
      return;
    case 101:
      tsr = ObjToTsRange(I(o, 5));
      if (TsGT(ts, &tsr->stopts)) {
      /* It is after the end of the appointment. */
        if (cx->mode == MODE_DAYDREAMING || cx->mode == MODE_PERFORMANCE) {
          AppointmentNoShow(cx, sg, a);
        } else {
          /* Assume appointment was kept. */
          ContextSetRSN(cx, RELEVANCE_HALF, SENSE_HALF, NOVELTY_HALF);
          PA_SpinTo(cx, sg, STSUCCESS);
         }
         return;
      }
      PA_Appointment_LeaveTs(sg, a, I(o, 2),
                             I(o, 3), ts,
                             tsr, &tsleave_at, &tsleave_before);
      if (TsGT(ts, &tsleave_before)) {
        ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_LITTLE, NOVELTY_TOTAL);
        AppointmentNoShow(cx, sg, a);
        return;
      } else if (TsGT(ts, &tsleave_at)) {
        if (sg->ac->appointment_cur) {
          /* todo: Terminate the other appointment. Don't if it is more
           * important.
           */
          sg->ac->appointment_cur = NULL;
        }
        sg->ac->appointment_cur = o;
        ContextSetRSN(cx, RELEVANCE_TOTAL, SENSE_TOTAL, NOVELTY_EXPECTED);
        SG(cx, sg, 110, STFAILURE,
           L(N("near-reachable"), a, I(o, 3), E));
        return;
      }
      TOSTATE(cx, sg, 100);
      return;
    /* I AM WAITING FOR THEM. */
    case 110:
      if (SpaceIsNearReachable(ts, NULL, I(o, 2),
                               I(o, 3))) {
        TOSTATE(cx, sg, 200);
        return;
      }
      tsr = ObjToTsRange(I(o, 5));
      tswait_until = tsr->startts;
      TsIncrement(&tswait_until, HumanWaitLimit(a, o));
      if (TsGE(ts, &tswait_until)) {
        AppointmentNoShow(cx, sg, I(o, 2));
        return;
      }
      WAIT_TS(cx, sg, ts, SECONDSPERMINL*1L, 110);
      return;
    /* APPOINTMENT BEGINS. */
    case 200:
	  /* todo: If null goal, decide what to do. */
      SG(cx, sg, 999, STFAILURE, I(o, 4));
      return;
    /* APPOINTMENT COMPLETES SUCCESSFULLY. */
	case 999:
	  if ((f = FriendFind(sg->ac, ts, I(o, 2), 0))) {
            f->ts_last_seen = *ts;
	  }
	  sg->ac->appointment_cur = NULL;
      AR(&sg->startts, ts, o);
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_Appointment: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* End of file. */
