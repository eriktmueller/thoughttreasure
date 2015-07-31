/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include "tt.h"
#include "repbasic.h"
#include "repobj.h"
#include "repobjl.h"
#include "repprove.h"

void ProveIt();
ObjList *ObjListFileRead();
Obj *ObjFileRead();

/* Type "proveit" in ThoughtTreasure shell to run this. */
void ProveItTest()
{
/*
  ProveIt("ex1_goal.ttp","ex1_rules.ttp","ex1_facts.ttp","ex1_proveit.txt");
  ProveIt("ex2_goal.ttp","ex2_rules.ttp","ex2_facts.ttp","ex2_proveit.txt");
  ProveIt("ex3_goal.ttp","ex3_rules.ttp","ex3_facts.ttp","ex3_proveit.txt");
 */
  ProveIt("ex4_goal.ttp","ex4_rules.ttp","ex4_facts.ttp","ex4_proveit.txt");
}

void ProveIt(char *goal_filename,
             char *rule_filename,
             char *fact_filename,
             char *output_filename)
{
  Obj		*goal;
  ObjList	*rules;
  ObjList	*facts;
  Proof		*proofs;
  FILE		*outstream;

  if (NULL == (outstream = StreamOpen(output_filename, "w+"))) {
    return;
  }
  goal = ObjFileRead(goal_filename);
  if (goal == NULL) {
    return;
  }
  rules = ObjListFileRead(rule_filename);
  facts = ObjListFileRead(fact_filename);
  fprintf(outstream, "input goal = ");
  ObjPrint(outstream, goal);
  fputc(NEWLINE, outstream);
  fprintf(outstream, "input rules =\n");
  ObjListPrint(outstream, rules);
  fprintf(outstream, "input facts =\n");
  ObjListPrint(outstream, facts);
  if (Prove1(&TsNA, NULL, goal, rules, facts, 1, 0, &proofs)) {
    fprintf(outstream, "found proofs:\n");
    ProofPrintAll(outstream, proofs);
  } else {
    fprintf(outstream, "did not find proofs\n");
  }
  StreamClose(outstream);
}

ObjList *ObjListFileRead(char *filename)
{
  FILE		*instream;
  ObjList	*objs;
  Obj		*obj;
  if (NULL == (instream = StreamOpen(filename, "r"))) {
    return NULL;
  }
  objs = NULL;
  while ((obj = ObjRead(instream))) {
    objs = ObjListCreate(obj, objs);
  }
  return objs;
}

Obj *ObjFileRead(char *filename)
{
  FILE		*instream;
  Obj		*obj;
  if (NULL == (instream = StreamOpen(filename, "r"))) {
    return NULL;
  }
  obj = ObjRead(instream);
  return obj;
}

/* End of file. */
