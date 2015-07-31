/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19950313: begun
 * 19950330: added recursion checking
 */

#include "tt.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "reptime.h"
#include "utilbb.h"
#include "utildbg.h"

ABrainTask *BBrainBegin(Obj *process, Dur timelimit, int recursion_limit)
{
  ABrainTask	*abt;
  abt = CREATE(ABrainTask);
  abt->process = process;
  TsSetNow(&abt->startts);
  abt->timeoutts = abt->startts;
  abt->timelimit = timelimit;
  TsIncrement(&abt->timeoutts, timelimit);
  abt->recursion_count = 0;
  abt->recursion_limit = recursion_limit;
  return(abt);
}

void BBrainEnd(ABrainTask *abt)
{
  if (abt == NULL) return;
  MemFree(abt, "ABrainTask");
}

void BBrainABrainRecursion(ABrainTask *abt)
{
  if (abt == NULL) return;
  abt->recursion_count++;
}

Bool BBrainStopsABrain(ABrainTask *abt)
{
  Ts	now;
  if (abt == NULL) return(0);
  TsSetNow(&now);
  if ((!Debugging) && TsGT(&now, &abt->timeoutts)) {
  /* The B-Brain is disabled once a breakpoint has been reached,
   * since time elapses while the program is stopped.
   */
    Dbg(DBGGEN, DBGBAD, "B-brain stops A-brain working on <%s> (> %ld secs)",
        M(abt->process), abt->timelimit);
    return(1);
  }
  if (abt->recursion_limit != INTNA &&
      abt->recursion_count > abt->recursion_limit) {
    return(1);
  }
  return(0);
}

/* End of file. */
