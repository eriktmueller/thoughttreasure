/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940825: begun
 */

#include "tt.h"
#include "repbasic.h"
#include "repdb.h"
#include "repgroup.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "reptime.h"
#include "utildbg.h"

Float AgeOf(Ts *ts, Obj *obj)
{
  Dur		dur;
  Obj		*born;
  TsRange	*tsr;
  if (!TsIsSpecific(ts)) return(FLOATNA);
  born = R1E(&TsNA, L(N("born"), obj, ObjWild, E));
  if (born) {
    tsr = ObjToTsRange(born);
    dur = TsMinus(ts, &tsr->startts);
    return(((Float)dur)/SECONDSPERYEARF);
  }
  /* todo: Make inferences based on occupation etc. */
  return(20.0);
}

Bool IsOwnerOf(Ts *ts, TsRange *tsr, Obj *object, Obj *owner)
{
  return(YES(REB(ts, tsr, L(N("owner-of"), object, owner, E))));
}

Obj *GuardianOf(Ts *ts, TsRange *tsr, Obj *human)
{
  return(R1EIB(2, ts, tsr, L(N("legal-guardian-of"), human, ObjWild, E)));
}

Bool GetActorPair1(Obj *con, Obj *actor, int i,
                   /* RESULTS */ Obj **counter_actor)
{
  ObjList	*members, *p, *counter_group;
  if (ISA(N("group"), I(con, i))) {
    members = GroupMembers(&TsNA, I(con, i));
    if (ObjListIn(actor, members)) {
      counter_group = NULL;
      for (p = members; p; p = p->next) {
        if (p->obj == actor) continue;
        counter_group = ObjListCreate(p->obj, counter_group);
      }
      *counter_actor = GroupCreate(&TsNA, counter_group);
      ObjListFree(counter_group);
      ObjListFree(members);
      return(1);
    }
    ObjListFree(members);
  }
  return(0);
}

Bool GetActorPair(Obj *con, Obj *actor, /* RESULTS */ Obj **counter_actor)
{
  if (GetActorPair1(con, actor, 1, counter_actor)) return(1);
  if (GetActorPair1(con, actor, 2, counter_actor)) return(1);
  if (I(con, 1) == actor && ObjIsConcreteNC(I(con, 2))) {
    *counter_actor = I(con, 2);
    return(1);
  } else if (I(con, 2) == actor && ObjIsConcrete(I(con, 1))) {
    *counter_actor = I(con, 1);
    return(1);
  }
  *counter_actor = NULL;
  return(0);
}

Bool GetSpatial(Obj *con, /* RESULTS */ Obj **from, Obj **to)
{
  if (ISA(N("location-of"), I(con, 0))) {
  /* Example: [location-of Martine maison-de-Lucie]
   * "Martine arrive chez Lucie."
   */
    *from = NULL;
    *to = I(con, 2);
    return(1);
  } else if (ISA(N("ptrans"), I(con, 0))) {
  /* Example: [ptrans Jim na Opéra-Comique]
   * "Jim est allé à l'Opéra Comique"
   */
    *from = I(con, 2);
    *to = I(con, 3);
    return(1);
  }
  return(0);
}

Bool GetActorSpatial(Obj *con, Obj *actor, /* RESULTS */ Obj **from, Obj **to)
{
  if (actor != I(con, 1)) return(0);
  return(GetSpatial(con, from, to));
}

Dur DurationOf(Obj *action)
{
  Obj	*dur;
  if ((dur = DbGetRelationValue(&TsNA, NULL, N("duration-of"), action, NULL))) {
    return((Dur)ObjToNumber(dur));
  }
  return(1L);
}

Dur DurValueOf(Obj *obj)
{
  Obj	*val;
  if ((val = DbGetRelationValue(&TsNA, NULL, N("value-of"), obj, NULL))) {
    return((Dur)ObjToNumber(val));
  }
  return(0L);
}

Float FloatValueOf(Obj *obj)
{
  Obj	*val;
  if ((val = DbGetRelationValue(&TsNA, NULL, N("value-of"), obj, NULL))) {
    return(ObjToNumber(val));
  }
  return(0.0);
}

Float FloatMinValueOf(Obj *obj)
{
  Obj	*val;
  if ((val = DbGetRelationValue(&TsNA, NULL, N("min-value-of"), obj, NULL))) {
    return(ObjToNumber(val));
  }
  return(0.0);
}

Float FloatMaxValueOf(Obj *obj)
{
  Obj	*val;
  if ((val = DbGetRelationValue(&TsNA, NULL, N("max-value-of"), obj, NULL))) {
    return(ObjToNumber(val));
  }
  return(0.0);
}

Bool FloatIsInRange(Float x, Obj *obj)
{
  return(x > FloatMinValueOf(obj) &&
         x <= FloatMaxValueOf(obj));
}

Dur DurCanonicalFactorOf(Obj *obj)
{
  Obj	*val;
  if ((val = DbGetRelationValue(&TsNA, NULL, N("canonical-factor-of"),
                                obj, NULL))) {
    return((Dur)ObjToNumber(val));
  }
  return(0L);
}

Float FloatCanonicalFactorOf(Obj *obj)
{
  Obj	*val;
  if ((val = DbGetRelationValue(&TsNA, NULL, N("canonical-factor-of"),
                                obj, NULL))) {
    return(ObjToNumber(val));
  }
  return(0.0);
}

Float RoughSizeOf(Ts *ts, Obj *obj)
{
  Float	length, width, height;
  length = ObjToNumber(DbGetRelationValue(ts, NULL, N("length-of"), obj, NULL));
  width = ObjToNumber(DbGetRelationValue(ts, NULL, N("width-of"), obj, NULL));
  height = ObjToNumber(DbGetRelationValue(ts, NULL, N("height-of"), obj, NULL));
  if (length != FLOATNA &&
      width != FLOATNA &&
      height != FLOATNA) {
    return(pow(length*width*height, .33333333333333));
  } else if (length != FLOATNA) {
    return(length);
  } else if (width != FLOATNA) {
    return(width);
  } else if (height != FLOATNA) {
    return(height);
  } else {
    return(FLOATNA);
  }
}

/******************************************************************************
 * Example:
 * goal:    [active-goal Jim [watch-TV Jim]]
 * Returns: watch
 ******************************************************************************/
Obj *GoalObjectiveOf(Obj *goal)
{
  return(I(I(goal, 2), 0));
}

/******************************************************************************
 * Example:
 * goal_status: active
 * actor:       Jim
 * objective:   watch-TV
 * Returns:     [active-goal Jim [watch-TV Jim]]
 ******************************************************************************/
Obj *GoalCreate(Obj *goal_status, Obj *actor, Obj *objective)
{
  return(L(goal_status, actor, L(objective, actor, E), E));
}

Dur HumanPunctuality(Obj *human, Obj *appointment)
{
  return(5L*60L);
}

/* todo: Base this on ptraits and ptrait generalizations.
 * Also vary it based on attitude toward person and importance
 * of appointment.
 */
Dur HumanWaitLimit(Obj *human, Obj *appointment)
{
  return(20L*60L);
}

Bool IsGarbage(Obj *obj)
{
  Obj	*head;
  if (ObjIsNa(obj) || ObjIsVar(obj)) return(1);
#ifdef notdef
  if (ObjInDeep(ObjNA, obj)) {
  /* na should be considered garbage:
   *   "Is Irish soda bread a drink?"
   * na should NOT be considered garbage:
   *   "[active-goal X [buy X na car na]]" */
   */
    return(1);
  }
#endif
  head = I(obj, 0);
  if (N("one") == head) return(1);
  /* note ISA(N("standard-copula"), head) is not garbage; cf UA_Question */
  if (ISADeep(N("relative-pronoun"), obj)) {
  /* These are supposed to be dealt with by Sem_Parse. If they
   * are still present, then someone isn't constraining away
   * faulty parses.
   */
    return(1);
  }
  return(0);
}

Bool NotNA(Obj *obj)
{
  return(obj != NULL && obj != ObjNA);
}

void LEADTO(Ts *ts, Obj *cause, Obj *result)
{
  TsRange	tsr;
  TsMakeRangeAlways(&tsr, ts);
  DbAssert(&tsr, L(N("leadto"), cause, result, E));
}

ObjList *CAUSES(Obj *result, Context *cx)
{
  Ts	ts;
  TsSetNaCx(&ts, cx);
  return(REI(1, &ts, L(N("leadto"), ObjWild, result, E)));
}

ObjList *RESULTS(Obj *cause, Context *cx)
{
  Ts	ts;
  TsSetNaCx(&ts, cx);
  return(REI(2, &ts, L(N("leadto"), cause, ObjWild, E)));
}

Obj *SexOpposite(Obj *sex)
{
  if (ISA(N("male"), sex)) return(N("female"));
  else return(N("male"));
}

int SexToFeat(Obj *sex)
{
  if (N("female") == sex) {
    return(F_FEMININE);
  } else {
    return(F_MASCULINE);
  }
}

Bool IsInterrogative(Obj *obj)
{
  return(ISA(N("interrogative-pronoun"), obj) ||
         ISA(N("interrogative-adverb"), obj) ||
         ISA(N("interrogative-determiner"), obj));
}

Bool IsActor(Obj *obj)
{
  return(ISA(N("human"), obj) && obj != N("human") &&
         (!ISA(N("group"), obj)) && (!IsInterrogative(obj)));
}

Obj *ThereAreXClass(Float count, Obj *class)
{
  return(L(N("standard-copula"),
           N("pronoun-there-expletive"),
           ObjIntension1(class,
                        L(NumberToObj(count), class, E)),
           E));
}

Obj *ActorGet(Obj *obj)
{
  return I(obj, 1);
}

/* End of file. */
