/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940821: begun
 * 19940822: debugging
 */

#include "tt.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repprove.h"
#include "repspace.h"
#include "reptime.h"
#include "utildbg.h"

#define MAXDEPTH 30

Float ObjScoreGet(Obj *obj)
{
  Float weight;
  weight = ObjWeight(obj);
  if (weight == WEIGHT_DEFAULT) {
    return 1.0;
  }
  return weight;
}

Float ProofScoreCombine(Float score1, Float score2)
{
  return score1 * score2;
}

/* Proof */

ProofReason *ProofReasonCreate(Proof *proof, ProofReason *next)
{
  ProofReason	*pr;
  pr = CREATE(ProofReason);
  pr->proof = proof;
  pr->next = next;
  return(pr);
}

ProofReason *ProofReasonLast(ProofReason *reasons)
{
  while (reasons->next) reasons = reasons->next;
  return(reasons);
}

ProofReason *ProofReasonAppend(ProofReason *pr1, ProofReason *pr2)
{
  if (pr1) {
    ProofReasonLast(pr1)->next = pr2;
    return(pr1);
  } else return(pr2);
}

ProofReason *ProofReasonCopyAll(ProofReason *pr)
{
  ProofReason	*r;
  r = NULL;
  for (; pr; pr = pr->next) {
    r = ProofReasonAppend(r, ProofReasonCreate(pr->proof, NULL));
  }
  return(r);
}

void ProofReasonFree(ProofReason *pr)
{
  ProofReason *n;
  while (pr) {
    n = pr->next;
    MemFree(pr, "ProofReason");
    pr = n;
  }
}

Proof *ProofCreate(Float score, Obj *fact, Bd *bd, Obj *rule,
                   ProofReason *reasons, Proof *next)
{
  Proof	*p;
  p = CREATE(Proof);
  p->score = score;
  p->fact = fact;
  p->bd = bd;
  p->rule = rule;
  p->reasons = reasons;
  p->next = next;
  return(p);
}

Proof *ProofCopyHead(Proof *p)
{
  return(ProofCreate(p->score, p->fact, BdCopy(p->bd), p->rule,
                     p->reasons, NULL));
}

void ProofFree(Proof *p)
{
  Proof *n;
  while (p) {
    ProofReasonFree(p->reasons);
    BdFree(p->bd);
    n = p->next;
    MemFree(p, "Proof");
    p = n;
  }
}

Proof *ProofAddLevel(Float score, Bd *bd, Obj *fact, Obj *rule, Proof *proof,
                     Proof *r)
{
  Float		score1;
  ProofReason	*pr;
  Proof		*p;
  Bd		*bd1;
  for (p = proof; p; p = p->next) {
    pr = ProofReasonCreate(ProofCopyHead(p), NULL);
    score1 = ProofScoreCombine(score, p->score);
    if (bd != NULL) {
      bd1 = BdCopyAppend(bd, p->bd);
    } else {
      bd1 = BdCopy(p->bd);
    }
    r = ProofCreate(score1, fact, bd1, rule, pr, r);
  }
  return(r);
}

void ProofPrintAll1(FILE *stream, Proof *p, int top)
{
  for (; p; p = p->next) {
    if (top) {
      IndentInit();
      StreamSep(stream);
    }
    ProofPrint(stream, p);
  }
}

void ProofReasonPrintAll(FILE *stream, ProofReason *pr)
{
  for (; pr; pr = pr->next) ProofPrintAll1(stream, pr->proof, 0);
}

void ProofPrint(FILE *stream, Proof *p)
{
  IndentUp();
  IndentPrint(stream);
  fprintf(stream, "score %g ", p->score);
  ObjPrint(stream, p->fact);
  fputs(" because ", stream);
  ObjPrint(stream, p->rule);
  fputc(NEWLINE, stream);
  BdPrint(stream, p->bd);
  ProofReasonPrintAll(stream, p->reasons);
  IndentDown();
}

void ProofPrintAll(FILE *stream, Proof *p)
{
  ProofPrintAll1(stream, p, 1);
}

Proof *ProofAppendEach(Proof *head, Proof *tails, Proof *r)
{
  Float		score;
  ProofReason	*reasons;
  Proof		*tail;
  for (tail = tails; tail; tail = tail->next) {
    reasons = ProofReasonAppend(ProofReasonCopyAll(head->reasons),
                                ProofReasonCopyAll(tail->reasons));
    score = ProofScoreCombine(head->score, tail->score);
    r = ProofCreate(score, head->fact, BdCopyAppend(head->bd, tail->bd),
                    head->rule, reasons, r);
  }
  return(r);
}

/* Prove. */

Bool ProveISA(Ts *ts, TsRange *tsr, Obj *goal, ObjList *rules,
              ObjList *more, Bool querydb, int depth,
              /* RESULTS */ Proof **out_pr)
{
  Obj		*arg1, *arg2;
  ObjList	*objs, *p;
  Bd		*bd;
  Proof		*proof_r;
  arg1 = I(goal, 1);
  arg2 = I(goal, 2);
  if (ObjIsVar(arg1)) {
    objs = ObjDescendants(arg2, OBJTYPEANY);
    proof_r = NULL;
    for (p = objs; p; p = p->next) {
      bd = BdAssign(BdCreate(), arg1, p->obj);
      proof_r = ProofCreate(1.0, goal, bd, N("isa"), NULL, proof_r);
    }
    if (proof_r) {
      *out_pr = proof_r;
      return(1);
    }
  } else if (ObjIsVar(arg2)) {
    objs = ObjAncestors(arg1, OBJTYPEANY);
    proof_r = NULL;
    for (p = objs; p; p = p->next) {
      bd = BdAssign(BdCreate(), arg2, p->obj);
      proof_r = ProofCreate(1.0, goal, bd, N("isa"), NULL, proof_r);
    }
    if (proof_r) {
      *out_pr = proof_r;
      return(1);
    }
  } else if (ISA(arg2, arg1)) {
    *out_pr = ProofCreate(1.0, goal, BdCreate(), N("isa"), NULL, NULL);
    return 1;
  }
  return 0;
}

Bool ProveAnd(Ts *ts, TsRange *tsr, Obj *goal, ObjList *rules, ObjList *more,
              Bool querydb, int depth, /* RESULTS */ Proof **out_pr)
{
  int	i, len;
  Obj	*obj;
  Proof	*p, *proof_r, *proof1, *proof2;
  if (!Prove1(ts, tsr, I(goal, 1), rules, more, querydb, depth+1, &proof_r)) {
    return(0);
  }
  proof_r = ProofAddLevel(1.0, NULL, goal, N("and"), proof_r, NULL);
  proof2 = NULL;
  for (i = 2, len = ObjLen(goal); i < len; i++) {
    proof2 = NULL;
    for (p = proof_r; p; p = p->next) {
      obj = ObjInstan(I(goal, i), p->bd);	/* todoFREE */
      if (Prove1(ts, tsr, obj, rules, more, querydb, depth+1, &proof1)) {
        proof2 = ProofAppendEach(p,
                   ProofAddLevel(1.0, NULL, goal, N("and"), proof1, NULL),
                                 proof2);
/*
        ProofFree(proof1);
 */
      }
    }
/*
    ProofFree(proof_r);
 */
    if (!proof2) return(0);
    proof_r = proof2;
  }
  *out_pr = proof_r;
  return(1);
}

Bool ProveOr(Ts *ts, TsRange *tsr, Obj *goal, ObjList *rules, ObjList *more,
             Bool querydb, int depth, /* RESULTS */ Proof **out_pr)
{
  int	i, len;
  Proof	*proof_r, *proof1;
  proof_r = NULL;
  for (i = 1, len = ObjLen(goal); i < len; i++) {
    if (Prove1(ts, tsr, I(goal, i), rules, more, querydb, depth+1, &proof1)) {
      proof_r = ProofAddLevel(1.0, NULL, goal, N("or"), proof1, proof_r);
/*
      ProofFree(proof1);
 */
    }
  }
  *out_pr = proof_r;
  return(1);
}

Bool ProveNot(Ts *ts, TsRange *tsr, Obj *goal, ObjList *rules, ObjList *more,
              Bool querydb, int depth, /* RESULTS */ Proof **out_pr)
{
  Proof	*proof1;
  if (Prove1(ts, tsr, I(goal, 1), rules, more, querydb, depth+1, &proof1)) {
 /*
    ProofFree(proof1);
  */
    return(0);
  }
  *out_pr = ProofCreate(1.0, goal, BdCreate(), N("not"), NULL, NULL);
  return(1);
}

Bool ProveComparativeNumber(Ts *ts, TsRange *tsr, Obj *goal, ObjList *rules,
                            ObjList *more, Bool querydb, int depth,
                            /* RESULTS */ Proof **out_pr)
{
  Obj	*head;
  Float	arg1, arg2;
  Bool	r;
  head = I(goal, 0);
  arg1 = ObjToNumber(I(goal, 1));
  arg2 = ObjToNumber(I(goal, 2));
  if (arg1 == FLOATNA || arg2 == FLOATNA) {
    r = 0;
  } else if (head == N("eq")) {
    r = arg1 == arg2;
  } else if (head == N("le")) {
    r = arg1 <= arg2;
  } else if (head == N("ge")) {
    r = arg1 >= arg2;
  } else if (head == N("ne")) {
    r = arg1 != arg2;
  } else if (head == N("lt")) {
    r = arg1 < arg2;
  } else if (head == N("gt")) {
    r = arg1 > arg2;
  } else {
    Dbg(DBGOBJ, DBGBAD, "unknown arithmetic relation <%s>", M(head));
    r = 0;
  }
  if (r) {
    *out_pr = ProofCreate(1.0, goal, BdCreate(), N("arithmetic-relation"),
                          NULL, NULL);
    return 1;
  } else {
    return 0;
  }
}

Bool ProveComparativeObject(Ts *ts, TsRange *tsr, Obj *goal, ObjList *rules,
                            ObjList *more, Bool querydb, int depth,
                            /* RESULTS */ Proof **out_pr)
{
  Obj	*head, *arg1, *arg2;
  Bool	r;
  head = I(goal, 0);
  arg1 = I(goal, 1);
  arg2 = I(goal, 2);
  if (arg1 == NULL || arg2 == NULL) {
    r = 0;
  } else if (head == N("eq")) {
    r = arg1 == arg2;
  } else if (head == N("ne")) {
    r = arg1 != arg2;
  } else {
    Dbg(DBGOBJ, DBGBAD, "unhandled relation <%s>", M(head));
    r = 0;
  }
  if (r) {
    *out_pr = ProofCreate(1.0, goal, BdCreate(), N("relation"),
                          NULL, NULL);
    return 1;
  } else {
    return 0;
  }
}

Bool ProveComparative(Ts *ts, TsRange *tsr, Obj *goal, ObjList *rules,
                      ObjList *more, Bool querydb, int depth,
                      /* RESULTS */ Proof **out_pr)
{
  if (ObjIsNumber(I(goal, 1)) && ObjIsNumber(I(goal, 2))) {
    return ProveComparativeNumber(ts, tsr, goal, rules, more, querydb,
                                  depth, out_pr);
  } else {
    return ProveComparativeObject(ts, tsr, goal, rules, more, querydb,
                                  depth, out_pr);
  }
}

/* todo: Add symmetric relations, opposites. Cycle prevention.
 * todo: Not sure of BdCopy correctness.
 * todoFREE: Lots of freeing to do.
 */
Bool ProveFact(Ts *ts, TsRange *tsr, Obj *goal, ObjList *rules, ObjList *more,
               Bool querydb, int depth, /* RESULTS */ Proof **out_pr)
{
  int		i;
  Bd		*new_bd;
  Float		score;
  Obj		*head, *obj, *prop, *goal1;
  ObjList	*matches, *p, *objs;
  TsRange	tsr1;
  Proof		*proof_r, *proof1;

  if (!goal) return(0);
  if (ISA(N("grid-traversal-noncanonical"), I(goal, 0))) {
    prop = DbGetRelationValue(&TsNA, NULL, N("canonical-of"), I(goal, 0),
                              I(goal, 0));
    /* todo: Resolve "the street". */
    goal = L(prop, I(goal, 1), E);
  }

  proof_r = NULL;
  if (querydb && N("location-of") == I(goal, 0)) {
    if (SpaceLocatedAt(ts, tsr, I(goal, 1), I(goal, 2), &tsr1)) {
      goal1 = ObjCopyList(goal);
      ObjSetTsRange(goal1, &tsr1);
      proof_r = ProofCreate(1.0, goal1, BdCreate(), N("true"), NULL, proof_r);
    }
  }
  if (querydb && N("near") == I(goal, 0)) {
    if ((objs = SpaceFindNear(ts, tsr, I(goal, 1), I(goal, 2), NULL))) {
      proof_r = ProofCreate(1.0, goal, BdCreate(), N("true"), NULL, proof_r);
      ObjListFree(objs);
    } else if ((objs = SpaceFindNear(ts, tsr, I(goal, 2), I(goal, 1), NULL))) {
    /* Is this really necessary? near should be symmetric */
      proof_r = ProofCreate(1.0, goal, BdCreate(), N("true"), NULL, proof_r);
      ObjListFree(objs);
    }
  }
  for (i = 0; i < ObjLen(goal); i++) {
    if (querydb && (matches = DbRetrieveDesc(ts, tsr, goal, i, 0, 0))) {
      for (p = matches; p; p = p->next) {
        if ((new_bd = ObjUnify2(goal, p->obj, BdCreate()))) {
          score = ObjScoreGet(p->obj);
          proof_r = ProofCreate(score, p->obj, new_bd, N("true"), NULL,
                                proof_r);
        } else {
          Dbg(DBGOBJ, DBGBAD, "ProveFact unification failure");
          ObjPrettyPrint(Log, goal);
          ObjPrettyPrint(Log, p->obj);
        }
      }
      ObjListFree(matches);
      break;
        /* If we don't break, then we have to eliminate duplicates. */
    }
  }
  /* todo: Add other kinds of near. */
  head = I(goal, 0);
  if (head == N("near-audible")) {
    if (N("?human") == I(goal, 1)) {
    /* todo: Generalize this. */
      matches = SpaceFindNearAudible(ts, tsr, N("human"), I(goal, 2));
      for (p = matches; p; p = p->next) {
        new_bd = BdAssign(BdCreate(), I(goal, 1), p->obj);
        proof_r = ProofCreate(1.0, goal, new_bd, N("true"), NULL, proof_r);
      }
    }
  }

  for (p = more; p; p = p->next) {
    if ((new_bd = ObjUnify2(p->obj, goal, BdCreate()))) {
      score = ObjScoreGet(p->obj);
      proof_r = ProofCreate(score, goal, new_bd, N("true"), NULL, proof_r);
    }
  }

  /* Backward chain on rules. */
  for (p = rules; p; p = p->next) {
    if ((new_bd = ObjUnify2(I(p->obj, 2), goal, BdCreate()))) {
      obj = ObjInstan(I(p->obj, 1), new_bd);	/* todoFREE: obj */
      score = ObjScoreGet(p->obj);
      if (Prove1(ts, tsr, obj, rules, more, querydb, depth+1, &proof1)) {
        proof_r = ProofAddLevel(score, new_bd, goal, p->obj, proof1, proof_r);
/*
        ProofFree(proof1);
 */
      }
    }
  }
  if (proof_r) {
    *out_pr = proof_r;
    return(1);
  }
  return(0);
}

Bool Prove1(Ts *ts, TsRange *tsr, Obj *goal, ObjList *rules, ObjList *more,
            Bool querydb, int depth, /* RESULTS */ Proof **out_pr)
{
  Bool	r;
  Obj	*head;
  if (depth > MAXDEPTH) {
    Dbg(DBGOBJ, DBGBAD, "Prove1: max depth reached");
    return 0;
  }
  if (DbgOn(DBGOBJ, DBGHYPER)) {
    StreamPrintSpaces(Log, depth);
    fprintf(Log, "PROVE ");
    ObjPrint(Log, goal);
    fputc(NEWLINE, Log);
  }
  head = I(goal, 0);
  if (N("isa") == head || N("ako") == head) {
    r = ProveISA(ts, tsr, goal, rules, more, querydb, depth, out_pr);
  } else if (N("and") == head) {
    r = ProveAnd(ts, tsr, goal, rules, more, querydb, depth, out_pr);
  } else if (N("or") == head) {
    r = ProveOr(ts, tsr, goal, rules, more, querydb, depth, out_pr);
  } else if (N("not") == head) {
    r = ProveNot(ts, tsr, goal, rules, more, querydb, depth, out_pr);
  } else if (ISA(N("arithmetic-relation"), head)) {
    r = ProveComparative(ts, tsr, goal, rules, more, querydb, depth, out_pr);
  } else {
    r = ProveFact(ts, tsr, goal, rules, more, querydb, depth, out_pr);
  }
  if (DbgOn(DBGOBJ, DBGHYPER)) {
    StreamPrintSpaces(Log, depth);
    if (r) {
      fprintf(Log, "PROVED ");
    } else {
      fprintf(Log, "NOT PROVED ");
    }
    ObjPrint(Log, goal);
    fputc(NEWLINE, Log);
  }
  return r;
}

ObjList *ProofRules;

Bool Prove(Ts *ts, TsRange *tsr, Obj *goal, ObjList *more, Bool querydb,
           /* RESULTS */ Proof **out_pr)
{
  if (Prove1(ts, tsr, goal, ProofRules, more, querydb, 0, out_pr)) {
    if (DbgOn(DBGOBJ, DBGHYPER)) {
      fprintf(Log, "found proofs of ");
      ObjPrint(Log, goal);
      fputc(NEWLINE, Log);
      ProofPrintAll(Log, *out_pr);
    }
    return(1);
  }
  return(0);
}

ObjList *ProveRetrieve(Ts *ts, TsRange *tsr, Obj *ptn, Bool freeptn)
{
  ObjList	*r;
  Proof		*proofs, *pr;
  ptn = ObjCopyWithoutWeight(ptn);	/* todo */
  if (!Prove(ts, tsr, ptn, NULL, 1, &proofs)) return(NULL);
  r = NULL;
  for (pr = proofs; pr; pr = pr->next) {
    r = ObjListCreate(pr->fact, r);
  }
/*
  ProofFree(proofs);
 */
  return(r);
}

void ProverTool()
{
  char  	buf[DWORDLEN];
  Obj   	*obj;
  ObjList       *objs;
  Ts    	ts;
  printf("Welcome to the prover.\n");
  while (1) {
    if (!StreamAskStd("Enter timestamp (?=wildcard): ", DWORDLEN, buf)) return;
    if (!TsParse(&ts, buf)) continue;
    printf("Enter query pattern: ");
    if ((obj = ObjRead(stdin))) {
      fprintf(stdout, "query pattern: ");
      fputc(TREE_TS_SLOT, stdout);
      TsPrint(stdout, &ts);
      fputc(TREE_SLOT_SEP, stdout);
      ObjPrint(stdout, obj);
      fprintf(stdout, "\n");
      fprintf(stdout, "results:\n");
      objs = ProveRetrieve(&ts, NULL, obj, 0);
      ObjListPrettyPrint1(stdout, objs, 1, 0, 0, 0);
      ObjListPrettyPrint1(Log, objs, 1, 0, 0, 0);
      ObjListFree(objs);
    }
  }   
}   

/* Inference: Not currently in use. */

ObjList	*InferenceRules;
ObjList	*ActivationRules;

void InferenceInit()
{
  InferenceRules = NULL;
  ActivationRules = NULL;
  ProofRules = NULL;
}

Bool InferenceAdd(Obj *obj)
{
  Obj	*head;
  head = I(obj, 0);
  if (head == N("if")) {
    InferenceRules = ObjListCreate(obj, InferenceRules);
  } else if (head == N("if-activate")) {
    ActivationRules = ObjListCreate(obj, ActivationRules);
  } else if (head == N("ifthen")) {
    ProofRules = ObjListCreate(obj, ProofRules);
  } else return(0);
  return(1);
}

ObjList *InferenceRun1(Ts *ts, TsRange *tsr, Obj *obj, ObjList *inferences,
                       ObjList *more, Bool querydb)
{
  Obj		*obj1;
  ObjList	*p, *r;
  Proof		*proofs, *pr;
  Dbg(DBGOBJ, DBGHYPER, "InferenceRun1");
  r = NULL;
  for (p = inferences; p; p = p->next) {
    DbgOP(DBGOBJ, DBGHYPER, p->obj);
    if (Prove(ts, tsr, I(p->obj, 1), more, querydb, &proofs)) {
      for (pr = proofs; pr; pr = pr->next) {
        obj1 = ObjInstan(I(p->obj, 2), pr->bd);	/* todo: run a proof instead */
        ObjSetTsRange(obj1, ObjToTsRange(obj));
        DbgOP(DBGOBJ, DBGHYPER, obj1);
        r = ObjListCreate(obj1, r);
      }
/*
      ProofFree(proofs);
 */
    }
  }
  return(r);
}

ObjList *InferenceRunInferenceRules(Ts *ts, TsRange *tsr, Obj *obj,
                                    ObjList *more, Bool querydb)
{
  return(InferenceRun1(ts, tsr, obj, InferenceRules, more, querydb));
}

ObjList *InferenceRunActivationRules(Ts *ts, TsRange *tsr, Obj *obj,
                                     ObjList *more, Bool querydb)
{
  return(InferenceRun1(ts, tsr, obj, ActivationRules, more, querydb));
}

/* End of file. */
