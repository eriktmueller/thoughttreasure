/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19941210: chatterbot begun
 * 19970815: name change
 */

#include "tt.h"
#include "pa.h"
#include "pagrasp.h"
#include "pamtrans.h"
#include "repbasic.h"
#include "repcloth.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "reptime.h"
#include "semdisc.h"
#include "uapaappt.h"
#include "utildbg.h"

/* SUBGOAL mtrans ?human ?human ?object */
void PA_Mtrans(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dur	d;
  Dbg(DBGPLAN, DBGOK, "PA_Mtrans");
  switch (sg->state) {
    case STBEGIN:
      if ((ObjListIn(I(o, 1), DCSPEAKERS(cx->dc)) &&
           ObjListIn(I(o, 2), DCLISTENERS(cx->dc))) ||
          (ObjListIn(I(o, 2), DCSPEAKERS(cx->dc)) &&
           ObjListIn(I(o, 1), DCLISTENERS(cx->dc)))) {
        Dbg(DBGPLAN, DBGDETAIL, "PA_Mtrans: Discourse near-audible");
        TOSTATE(cx, sg, 1);
        return;
      }
      SG(cx, sg, 1, STFAILURE, L(N("near-audible"), I(o, 1), I(o, 2), E));
      return;
    case 1:
      d = DurationOf(I(o, 0));
      AA(ts, d, o);
      /*
      AS(ts, d, L(N("know"), I(o,2), o, E));
      */
      TsIncrement(ts, d);
      TOSTATE(cx, sg, STSUCCESS); return;
    default: Dbg(DBGPLAN, DBGBAD, "PA_Mtrans: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL obtain-permission */
void PA_ObtainPermission(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_ObtainPermission", E);
  switch (sg->state) {
    case STBEGIN:
      SG(cx, sg, 1, STFAILURE, L(N("propose"), a, I(o, 2), I(o, 3), E));
      /* todo: Present arguments for. */
      return;
    case 1:
      /* todo: Do sg->last_sg another way? */
      if (WAIT_PTN(cx, sg, 1, 999, L(N("accept"), I(sg->last_sg, 2), ObjWild,
                                 I(sg->last_sg, 3), E))) {
        return;
      }
      if (WAIT_PTN(cx, sg, 0, STFAILURE,
                   L(N("reject"), I(sg->last_sg, 2), ObjWild,
                     I(sg->last_sg, 3), E))) {
        return;
      }
      /* todo: Argue with reject. */
      return;
    case 999:
      AR(&sg->startts, ts, o);
      TOSTATE(cx, sg, STSUCCESS);
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_ObtainPermission: undefined state %d",
          sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* SUBGOAL handle-proposal ?human */
void PA_HandleProposal(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  ObjList	*reasons_for_decision;
  Dbg(DBGPLAN, DBGOK, "PA_HandleProposals", E);
  switch (sg->state) {
    case STBEGIN:
      TOSTATE(cx, sg, 100);
      return;
    case 100:
      if (WAIT_PTN(cx, sg, 1, 200, L(N("propose"), ObjWild, a, ObjWild, E))) {
        return;
      }
      return;
    case 200:
      if (ISA(N("appointment"), I(sg->trigger, 0))) {
        if (UA_Appointment_IsProposalAccepted(sg->ac, ts, a, I(sg->trigger, 2),
                                              &reasons_for_decision)) {
          /* todo: State reasons_for_decision. */
          TOSTATE(cx, sg, 300);
          return;
        } else {
          TOSTATE(cx, sg, 400);
          return;
        }
      }
      /* todo: Evaluate other proposal types. */
      TOSTATE(cx, sg, 300);
      return;
    case 300:
      SG(cx, sg, 100, 100,
         L(N("accept"), a, I(sg->trigger, 1), I(sg->trigger, 3), E));
      return;
    case 400:
      SG(cx, sg, 100, 100,
         L(N("reject"), a, I(sg->trigger, 1), I(sg->trigger, 3), E));
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_HandleProposals: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* Replacement for PA_MainLoop which avoids problems of confusing
 * story planning and chatterbot planning. (cf Lin/Bertie example)
 */
void Chatterbot_Loop(Context *cx)
{
  Discourse	*dc;
  Dbg(DBGPLAN, DBGOK, "Chatterbot_Loop");

  dc = cx->dc;
  ContextCurrentDc = dc;	/* todoTHREAD */
  dc->run_agencies = AGENCY_ALL;
  while (1) {
    if (!DiscourseReadUnderstand(cx->dc)) {
      Dbg(DBGPLAN, DBGDETAIL, "Chatterbot_Loop exited because eof received");
      return;
    }
    cx = dc->cx_best;
  }
}
void Chatterbot(int lang, int dialect, Obj *speaker, Obj *listener)
{
  int		save_mode;
  Ts		*ts;
  Obj		*goal;

  save_mode = StdDiscourse->mode;
/*
  StdDiscourse->mode = DC_MODE_TURING|DC_MODE_CONV;
 */
  StdDiscourse->mode = DC_MODE_CONV;

  DiscourseInit(StdDiscourse, NULL, N("online-chat"), speaker, listener, 1);

  DiscourseOpenChannel(StdDiscourse, DCIN, "STDIN", DSPLINELEN, "r",
                       0, lang, dialect, F_NULL, 0, 0, 1, NULL);
  DiscourseOpenChannel(StdDiscourse, DCOUT2, "STDOUT", DSPLINELEN, "w+",
                       0, lang, dialect, F_NULL, 0, 0, 1, NULL);
  goal = L(N("online-chat"), Me, DiscourseSpeaker(StdDiscourse), E);
  ts = DCNOW(StdDiscourse);
  TG(StdDiscourse->cx_best, ts, Me, goal);
  DbgSetStdoutLevel(DBGOFF);
  fprintf(stdout, "Indicate end of input with a blank line.\n");
  fprintf(stdout, "Indicate end of chat session with 'quit'.\n");
/*
  PA_MainLoop(StdDiscourse->cx_best, MODE_PERFORMANCE);
 */
  Chatterbot_Loop(StdDiscourse->cx_best);
  DiscourseCloseChannel(StdDiscourse, DCIN);
  DiscourseCloseChannel(StdDiscourse, DCOUT2);
  DiscourseDeicticStackPop(StdDiscourse);
  DbgSetStdoutLevel(DBGOK);	/* todo: Should save and restore previous. */

  StdDiscourse->mode = save_mode;
}

/* SUBGOAL online-chat ?planner ?partner */
void PA_OnlineChat(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o)
{
  Dbg(DBGPLAN, DBGOK, "PA_OnlineChat", E);
  switch (sg->state) {
    case STBEGIN:
      sg->retries = 0;
      sg->return_to_state = 1;
      TOSTATE(cx, sg, 1);
      return;
/* Hi! */
    case 1:
      if (WAIT_PTN(cx, sg, 0, 10,
                   L(N("interjection-of-greeting"), I(o, 2), a, E))) {
        return;
      }
      WAIT_IDLE(cx, sg, ts, (Dur)2, 20);
      return;
    case 10:    /* Partner says hi first. */
      sg->return_to_state = 100;
      AS(ts, 0, L(N("near-audible"), a, I(o, 2), E));
      SG(cx, sg, 11, STFAILURE,
         L(N("interjection-of-greeting"), a, I(o, 2), E));
      return;
    case 11:
      TOSTATE(cx, sg, 100);
      return;
    case 20:    /* Planner says hi first-no hi from partner yet. */
      sg->return_to_state = 20;
      SG(cx, sg, 21, STFAILURE,
         L(N("interjection-of-greeting"), a, I(o, 2), E));
      return;
    case 21:
      if (WAIT_PTN(cx, sg, 0, 22,
                   L(N("interjection-of-greeting"), I(o, 2), a, E))) {
        return;
      }
      /* todo: Any input. */
      WAIT_IDLE(cx, sg, ts, (Dur)20, 1000);
      return;
    case 22:
      AS(ts, 0, L(N("near-audible"), a, I(o, 2), E));
      TOSTATE(cx, sg, 100);
      return;
/* How are you? */
    case 100:
      sg->return_to_state = 100;
      SG(cx, sg, 101, STFAILURE, L(N("how-are-you"), a, I(o, 2), E));
      return;
    case 101:
      if (WAIT_PTN(cx, sg, 0, 110,
                   L(N("how-are-you-response-with-question"), I(o, 2), a, E))) {
        return;
      }
      if (WAIT_PTN(cx, sg, 0, 150,
                   L(N("how-are-you-response-without-question"), I(o, 2), a,
                     E))) {
        return;
      }
      /* todo: Persistent WAITs would be a more economical representation
       * for interjection-of-departure processing.
       */
      if (WAIT_PTN(cx, sg, 0, 910,
                   L(N("interjection-of-departure"), I(o, 2), a, E))) {
        return;
      }
      WAIT_IDLE(cx, sg, ts, (Dur)10, 1000);
      return;
    case 110:
      SG(cx, sg, 150, STFAILURE,
         L(N("how-are-you-response-without-question"), a, I(o, 2), E));
      return;
    case 150:
/* What's new? */
      SG(cx, sg, 200, STFAILURE, L(N("request-for-news"), a, I(o, 2), E));
      return;
    case 200:
/* Arbitrary conversation. */
      sg->return_to_state = 200;
      if (WAIT_PTN(cx, sg, 0, 200, L(N("intervention"), I(o, 2), a, E))) {
        /* todo: Any input. */
        return;
      }
      if (WAIT_PTN(cx, sg, 0, 810, L(N("leave-taking"), I(o, 2), a, E))) {
        return;
      }
      if (WAIT_PTN(cx, sg, 0, 910,
                   L(N("interjection-of-departure"), I(o, 2), a, E))) {
        return;
      }
      WAIT_IDLE(cx, sg, ts, (Dur)10, 1000);
      return;
/* Post sequences before definitive end of conversation. */
    case 800:	/* Planner wants to end conversation first. */
      SG(cx, sg, 801, STFAILURE, L(N("leave-taking"), a, I(o, 2), E));
      return;
    case 801:
      if (WAIT_PTN(cx, sg, 0, 900, L(N("leave-taking"), I(o, 2), a, E))) {
        return;
      }
      if (WAIT_PTN(cx, sg, 0, 910,
                   L(N("interjection-of-departure"), I(o, 2), a, E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)10, 200);
        /* todo: Failure to end conversation. */
      return;
    case 810:   /* Partner wants to end conversation first. */
      /* todo: Perhaps some more post sequence here. */
      TOSTATE(cx, sg, 900);
      return;
/* End of conversation. */
    case 900:	/* Planner says goodbye first. */
      SG(cx, sg, 901, STFAILURE,
         L(N("interjection-of-departure"), a, I(o, 2), E));
      return;
    case 901:   /* Wait for partner to say goodbye. */
      if (WAIT_PTN(cx, sg, 0, 999,
                   L(N("interjection-of-departure"), I(o, 2), a, E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)5, 999);
      return;
    case 910:	/* Partner says goodbye first. */
      SG(cx, sg, 999, STFAILURE,
         L(N("interjection-of-departure"), a, I(o, 2), E));
      return;
    case 920:   /* Emergency goodbye from planner. No response awaited. */
      SG(cx, sg, 999, STFAILURE,
         L(N("interjection-of-departure"), a, I(o, 2), E));
      return;
    case 999:	/* Success script termination. */
      TE(ts, L(N("near-audible"), a, I(o, 2), E));
      TE(ts, L(N("near-audible"), I(o, 2), a, E));
      AR(&sg->startts, ts, o);
      TOSTATE(cx, sg, STSUCCESS);
      return;
/* Is anybody there? */
    case 1000:
      if (sg->retries == 0) {
        DiscourseClearActivity(cx->dc);
      } else if (DiscourseWasActivity(cx->dc)) {
        TOSTATE(cx, sg, 1010);
        return;
      }
      sg->retries++;
      if (sg->retries > 2) {
        TOSTATE(cx, sg, 920);
        return;
      }
      if (sg->return_to_state < 100) {
        SG(cx, sg, 1001, STFAILURE, L(N("is-anybody-there"), a, I(o, 2), E));
      } else {
        SG(cx, sg, 1001, STFAILURE, L(N("are-you-still-there"), a, I(o, 2), E));
      }
      return;
    case 1001:
      if (WAIT_PTN(cx, sg, 0, 1010, L(N("I-am-here"), I(o, 2), a, E))) {
        return;
      }
      if (WAIT_PTN(cx, sg, 0, 1010, L(N("mtrans"), I(o, 2), a, E))) {
      /* todo: Any input. */
        return;
      }
      if (WAIT_PTN(cx, sg, 0, 910,
                   L(N("interjection-of-departure"), I(o, 2), a, E))) {
        return;
      }
      WAIT_TS(cx, sg, ts, (Dur)(sg->retries*15), 1000);
      return;
    case 1010: /* Person is there. */
      sg->retries = 0;
      TOSTATE(cx, sg, sg->return_to_state);
      return;
    default:
      Dbg(DBGPLAN, DBGBAD, "PA_OnlineChat: undefined state %d", sg->state);
  }
  TOSTATE(cx, sg, STFAILURE);
}

/* End of file. */
