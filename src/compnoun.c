/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19980630: begun
 * 19980702: more work
 */

#include "tt.h"
#include "compnoun.h"
#include "repchan.h"
#include "repobj.h"
#include "repobjl.h"
#include "semdisc.h"
#include "synparse.h"
#include "ta.h"
#include "utildbg.h"

char *CompoundNoun_Input;
char *CompoundNoun_AnswerKey;
int CompoundNoun_AnsweredCorrectly;
int CompoundNoun_Answered;
int CompoundNoun_Total;

void CompoundNoun_Parse1(ObjList *cons, Discourse *dc)
{
  char *answer;
  Float recall, precision;
  Float score, maxscore;
  Obj *obj;
  ObjList *p;
  int correct;
  for (p = cons; p; p = p->next) {
    score = p->u.sp.score;
    if (!ObjIsList(p->obj)) score = 0.0;
    if (!ISA(N("compound-noun"), I(p->obj, 0))) score = 0.0;
    if (score > .99) score = 0.98; /* so that score prints */
    p->u.sp.score = score;
  }
  Dbg(DBGSEMPAR, DBGDETAIL, "**** COMPOUND NOUN PARSE RESULTS ****");
  maxscore = ObjListScoreMax(cons);

  if (maxscore > 0.0) {
    obj = ObjListScoreHighest(cons);
    answer = M(I(obj, 0));
    obj =  ObjListScoreHighest(cons);
    fprintf(Out, "MAKES MOST SENSE: ");
    fprintf(Log, "MAKES MOST SENSE: ");
    ObjPrint(Out, obj);
    ObjPrint(Log, obj);
  } else {
    answer = NULL;
    fprintf(Out, "DOES NOT MAKE SENSE");
    fprintf(Log, "DOES NOT MAKE SENSE");
  }
  if (answer == NULL) answer = "2-of-1";
  fprintf(Out, "\n====================\n");
  fprintf(Log, "\n====================\n");
  ObjListPrint1(Out, cons, 0, 0, 1, 0);
  ObjListPrint1(Log, cons, 0, 0, 1, 0);
  if (CompoundNoun_AnswerKey != NULL) {
    CompoundNoun_Total++;
    correct = 0;
    if (answer != NULL) {
      CompoundNoun_Answered++;
      if (streq(answer, CompoundNoun_AnswerKey)) {
        correct = 1;
        CompoundNoun_AnsweredCorrectly++;
      }
    }
    if (CompoundNoun_Answered > 0) {
      precision = ((Float)CompoundNoun_AnsweredCorrectly) /
                  ((Float)CompoundNoun_Answered);
      precision = ((Float)CompoundNoun_AnsweredCorrectly) /
                  ((Float)CompoundNoun_Answered);
    } else {
      precision = 0.0;
    }
    if (CompoundNoun_Total > 0) {
      recall = ((Float)CompoundNoun_AnsweredCorrectly)/
               ((Float)CompoundNoun_Total);
    } else {
      recall = 0.0;
    }
    fprintf(Out, "RES %s %25s %20s %20s\n",
            correct ? "Y" : "N",
            CompoundNoun_Input,
            ((answer != NULL) ? answer : "FAILURE"),
            CompoundNoun_AnswerKey);

    fprintf(Out, "ANSWER recall %g precision %g counts %d %d %d\n",
            recall*100.0,
            precision*100.0,
            CompoundNoun_AnsweredCorrectly,
            CompoundNoun_Answered,
            CompoundNoun_Total);
    fprintf(Log, "ANSWER recall %g precision %g counts %d %d %d\n",
            recall*100.0,
            precision*100.0,
            CompoundNoun_AnsweredCorrectly,
            CompoundNoun_Answered,
            CompoundNoun_Total);
  }
}

void CompoundNoun_Parse(Discourse *dc)
{
  char *s;
  Channel *ch;
  size_t  lowerb, upperb;
  int i, len;
  DiscourseSetCurrentChannel(dc, DCIN);
  ch = &DC(dc);
  CompoundNoun_Total = CompoundNoun_Answered = 0;
  CompoundNoun_AnsweredCorrectly = 0;
  while (ChannelReadLine(ch)) {
    fprintf(Out, "> %s", ch->buf);
    fprintf(Log, "> %s", ch->buf);
    s = CompoundNoun_Input = (char *)ch->buf;
    CompoundNoun_AnswerKey = NULL;
    len = strlen(s);
    for (i = 0; i < len; i++) {
      if (s[i] == '!') {
        ChannelTruncate(ch, i);
        CompoundNoun_AnswerKey = &s[i+1];
        CompoundNoun_AnswerKey[strlen(CompoundNoun_AnswerKey)-1] = '\0';
        break;
      }
    }
    TA_Scan(ch, dc);
    if (ch->pnf->first && ch->pnf->last) {
      lowerb = ch->pnf->first->lowerb;
      upperb = ch->pnf->last->upperb;
      Sem_ParseResults = NULL;
      Syn_ParseParse(ch, dc, lowerb, upperb, '.');
      CompoundNoun_Parse1(Sem_ParseResults, dc);
    }
  }
  ChannelBufferClear(ch);
}

/* End of file. */
