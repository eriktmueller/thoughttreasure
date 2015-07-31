/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "repactor.h"
#include "repbasic.h"
#include "repcxt.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repsubgl.h"
#include "synpnode.h"
#include "uafriend.h"
#include "utildbg.h"

/* Antecedent */

/* Salience metric based on (Huls, Bos, and Claassen, 1995).
 *
 * todo: If <pn_top> == NULL, we are being called from generation.
 * In this case, we have to rerun this after the pn_top IS available.
 * Antecedents decay before generation.
 */

void AntecedentInit(Antecedent *an)
{
  an->sigweight_major = 0;
  an->sigweight_subject = 0;
  an->sigweight_nested = 0;
  an->sigweight_list = 0;
  an->gender = F_NULL;
  an->number = F_NULL;
  an->person = F_NULL;
}

void AntecedentPrint(FILE *stream, int i, Antecedent *an)
{
  fprintf(stream,
          "Channel %d Antecedent salience %d (=%d+%d+%d+%d) <%c%c%c>\n",
          i, SALIENCE(an), an->sigweight_major, an->sigweight_subject,
          an->sigweight_nested, an->sigweight_list, an->gender,
          an->number, an->person);
}

void AntecedentDecay(/* RESULTS */ Antecedent *an)
{
  if (an->sigweight_major > 0) an->sigweight_major--;
  if (an->sigweight_subject > 0) an->sigweight_subject--;
  if (an->sigweight_nested > 0) an->sigweight_nested--;
  if (an->sigweight_list > 0) an->sigweight_list--;
}

void AntecedentRefresh(/* RESULTS */ Antecedent *an,
                       /* INPUT */ Obj *obj, PNode *pn, PNode *pn_top,
                       int gender, int number, int person)
{
  if (gender != F_NULL) an->gender = gender;
  if (number != F_NULL) an->number = number;
  if (person != F_NULL) an->person = person;
  if (ObjIsList(obj)) {
    an->sigweight_list += 3;
  } else {
    if (pn_top == NULL) return;
    switch (PNodeSyntacticType(pn, pn_top)) {
      case SYNTACTIC_SUBJ:
        an->sigweight_major += 3;
        an->sigweight_subject += 2;
        break;
      case SYNTACTIC_OBJ:
        an->sigweight_major += 3;
        break;
      case SYNTACTIC_IOBJ:
        an->sigweight_major += 3;
        break;
      default:
      /* Anything else more deeply nested than a top-level subject,
       * object, or indirect object. No need to distinguish the
       * various types, since they all get the same significance
       * weight of 1.
       */
        PNodeSyntacticType(pn, pn_top);
        an->sigweight_nested += 1;
        break;
    }
  }
}

/* Actor */

Actor *ActorCreate(Obj *actor, Context *cx, Actor *next)
{
  int		i;
  Actor		*ac;
  ac = CREATE(Actor);
  ac->actor = actor;
  ac->subgoals = NULL;
  ac->emotions = NULL;
  ac->friends = NULL;
  ac->appointment_cur = NULL;
  if (ISA(N("animal"), actor)) {
    ac->rest_level = L(N("restedness"), actor, D(1.0), E);
    ac->energy_level = L(N("energy"), actor, D(1.0), E);
  } else {
    ac->rest_level = NULL;
    ac->energy_level = NULL;
  }
  ac->cx = cx;
  ac->next = next;
  for (i = 0; i < DCMAX; i++) {
    AntecedentInit(&ac->antecedent[i]);
  }
  return(ac);
}

Actor *ActorCopy(Actor *ac_parent, Context *cx_parent, Context *cx_child,
                 Actor *next)
{
  int	i;
  Actor	*ac_child;
  ac_child = CREATE(Actor);
  ac_child->actor = ac_parent->actor;
  ac_child->subgoals = SubgoalCopyAll(ac_parent->subgoals, ac_child, cx_parent,
                                      cx_child);

#ifdef notdef
  ac_child->emotions = ContextMapAssertions(cx_parent, cx_child,
                                            ac_parent->emotions);
#endif
  ac_child->emotions = ac_parent->emotions;
    /* todo: These are asserted and ObjWeightSet is invoked on these,
     * so we need to copy.
     */

  ac_child->friends = FriendCopyAll(ac_parent->friends, cx_parent, cx_child,
                                    ac_parent->subgoals);

  ac_child->appointment_cur = ac_parent->appointment_cur;
    /* There should be no need to copy this. It is a pointer to
     * a sg->obj, which is also shared, among Subgoals.
     */

  ac_child->rest_level = ObjCopyList(ac_parent->rest_level);
  ac_child->energy_level = ObjCopyList(ac_parent->energy_level);
    /* ObjWeightSet is invoked on these, so we want to copy.
     * They were not asserted last time I checked.
     */

  ac_child->cx = cx_child;
  ac_child->next = next;

  for (i = 0; i < DCMAX; i++) {
    ac_child->antecedent[i] = ac_parent->antecedent[i];
  }

  return(ac_child);
}

Actor *ActorCopyAll(Actor *ac_parent, Context *cx_parent, Context *cx_child)
{
  Actor	*ac_children;
  ac_children = NULL;
  while (ac_parent) {
    ac_children = ActorCopy(ac_parent, cx_parent, cx_child, ac_children);
    ac_parent = ac_parent->next;
  }
  return(ac_children);
}

ObjList *ActorFindSubgoalsHead(Actor *ac, Obj *head_class)
{
  Subgoal	*sg;
  ObjList	*r;
  r = NULL;
  for (sg = ac->subgoals; sg; sg = sg->next) {
    if (SubgoalStateIsOutcome(sg->state)) continue;
    if (ISA(head_class, I(sg->obj, 0))) {
      r = ObjListCreate(sg->obj, r);
    }
  }
  return(r);
}

void ActorPrint(FILE *stream, Actor *ac)
{
  int		i;
  Subgoal	*sg;
  fputs("Actor ", stream);
  fputs(M(ac->actor), stream);
  fputc(NEWLINE, stream);
  fputs(
"------------------antecedents----------------------------------------------\n",
stream);
  for (i = 0; i < DCMAX; i++) {
    AntecedentPrint(stream, i, &ac->antecedent[i]);
  }
  if (ac->subgoals) {
    fputs(
"------------------subgoals-------------------------------------------------\n",
stream);
    for (sg = ac->subgoals; sg; sg = sg->next) {
      SubgoalPrint(stream, sg, 0);
      fputc(NEWLINE, stream);
    }
  }
  if (ac->emotions) {
    fputs(
"------------------emotions-------------------------------------------------\n",
stream);
    ObjListPrint(stream, ac->emotions);
  }
  if (ac->rest_level || ac->energy_level) {
    fputs(
"------------------levels---------------------------------------------------\n",
stream);
    if (ac->rest_level) {
      ObjPrint(stream, ac->rest_level);
      fputc(NEWLINE, stream);
    }
    if (ac->energy_level) {
      ObjPrint(stream, ac->energy_level);
      fputc(NEWLINE, stream);
    }
  }
  if (ac->friends) {
    fputs(
"------------------friends--------------------------------------------------\n",
stream);
    FriendPrintAll(stream, ac->friends);
  }
  if (ac->appointment_cur) {
    fputs(
"------------------current appointment--------------------------------------\n",
stream);
    ObjPrint(stream, ac->appointment_cur);
    fputc(NEWLINE, stream);
  }
}

void ActorPrintAll(FILE *stream, Actor *actors)
{
  Actor	*ac;
  for (ac = actors; ac; ac = ac->next) {
    fputc(NEWLINE, stream);
    ActorPrint(stream, ac);
  }
}

/* End of file. */
