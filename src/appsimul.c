/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Test functions for simulation of the human universe.
 *
 * 19940420: begun
 * 19950530: broke into separate file
 *
 * todo:
 * - Allow user to input list of characters, their locations, and their
 *   goals.
 */

#include "tt.h"
#include "pa.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "reptime.h"
#include "semdisc.h"
#include "utildbg.h"

void Simul_JimWatchTV()
{
  TsRange	tsr;
  TsRangeParse(&tsr, "19940518T200500:na");
  DiscourseInit(StdDiscourse, &tsr.startts, NULL, NULL, NULL, 0);
  DbAssert(&tsr, ObjAtGridCreate(N("Jim"), N("20-rue-Drouot-4E"), 20, 79));
  TG(StdDiscourse->cx_best, &tsr.startts, N("Jim"),
     L(N("watch-TV"), N("Jim"), E));
  PA_MainLoop(StdDiscourse->cx_best, MODE_DAYDREAMING);
}

void Simul_JimAttendPerformance()
{
  TsRange	tsr;
  Obj		*script, *perf;
  Obj		*bop;
  DiscourseInit(StdDiscourse, &tsr.startts, NULL, NULL, NULL, 0);
  TsRangeParse(&tsr, "19940525T200500:na");
  DbAssert(&tsr, ObjAtGridCreate(N("Jim"), N("20-rue-Drouot-4E"), 20, 79));
  script = L(N("attend-performance"), N("Jim"), E);
  perf = N("Don-Pasquale-run1");
  DbAssert(&tsr, L(N("live-performance"), script, perf, E));
  TG(StdDiscourse->cx_best, &tsr.startts, N("Jim"), script);
  
  bop = ObjCreateInstance(N("human"), NULL);
  DbAssert(&tsr, ObjAtGridCreate(bop, N("Opéra-Comique-Rez"), 9, 25));
  TG(StdDiscourse->cx_best, &tsr.startts, bop,
     L(N("work-box-office"), bop, N("Opéra-Comique-box-office"), E));

  PA_MainLoop(StdDiscourse->cx_best, MODE_DAYDREAMING);
}

void Simul_JimTakeShower()
{
  TsRange	tsr;
  TsRangeParse(&tsr, "19940518T070500:na");
  DiscourseInit(StdDiscourse, &tsr.startts, NULL, NULL, NULL, 0);
  DbAssert(&tsr, ObjAtGridCreate(N("Jim"), N("20-rue-Drouot-4E"), 20, 79));
  DbAssert(&tsr, L(N("wearing-of"), N("Jim"), N("pajama-top"), E));
  DbAssert(&tsr, L(N("wearing-of"), N("Jim"), N("pajama-bottom"), E));
  TG(StdDiscourse->cx_best, &tsr.startts, N("Jim"),
     L(N("take-shower"), N("Jim"), E));
  PA_MainLoop(StdDiscourse->cx_best, MODE_DAYDREAMING);
}

void Simul_JimDress()
{
  TsRange	tsr;
  TsRangeParse(&tsr, "19940518T070500:na");
  DiscourseInit(StdDiscourse, &tsr.startts, NULL, NULL, NULL, 0);
  DbAssert(&tsr, ObjAtGridCreate(N("Jim"), N("20-rue-Drouot-4E"), 20, 79));
  DbAssert(&tsr, L(N("wearing-of"), N("Jim"), N("pajama-top"), E));
  DbAssert(&tsr, L(N("wearing-of"), N("Jim"), N("pajama-bottom"), E));
  TG(StdDiscourse->cx_best, &tsr.startts, N("Jim"),
     L(N("dress"), N("Jim"), N("business-script"), E));
  PA_MainLoop(StdDiscourse->cx_best, MODE_DAYDREAMING);
}

void Simul_JimTripToJFK()
{
  TsRange	tsr;
  TsRangeParse(&tsr, "19940812T080000:na");
  DiscourseInit(StdDiscourse, &tsr.startts, NULL, NULL, NULL, 0);
  DbAssert(&tsr, ObjAtGridCreate(N("Jim"), N("20-rue-Drouot-4E"), 20, 79));
  TG(StdDiscourse->cx_best, &tsr.startts, N("Jim"),
     L(N("near-reachable"), N("Jim"), N("JFK-gate-26"), E));
  PA_MainLoop(StdDiscourse->cx_best, MODE_DAYDREAMING);
}

void Simul_JimPennyJars()
{
  TsRange	tsr;
  Obj		*penny, *jar1, *jar2;

  penny = ObjCreateInstanceNamed(N("penny"), "penny1", 1);
  jar1 = ObjCreateInstanceNamed(N("jar"), "jar1", 1);
  jar2 = ObjCreateInstanceNamed(N("jar"), "jar2", 1);

  TsRangeParse(&tsr, "19940518T200500:na");

  DiscourseInit(StdDiscourse, &tsr.startts, NULL, NULL, NULL, 0);

  DbAssert(&tsr, ObjAtGridCreate(N("Jim"), N("20-rue-Drouot-4E"), 20, 78));
  DbAssert(&tsr, ObjAtGridCreate(penny, N("20-rue-Drouot-4E"), 18, 80));
  DbAssert(&tsr, ObjAtGridCreate(jar1, N("20-rue-Drouot-4E"), 18, 80));
  DbAssert(&tsr, L(N("inside"), penny, jar1, E));
  DbAssert(&tsr, L(N("closed"), jar1, E));
  DbAssert(&tsr, L(N("closed"), jar2, E));
  DbAssert(&tsr, ObjAtGridCreate(jar2, N("20-rue-Drouot-4E"), 16, 82));

  TG(StdDiscourse->cx_best, &tsr.startts, N("Jim"),
     L(N("inside"), penny, jar2, E));
  PA_MainLoop(StdDiscourse->cx_best, MODE_DAYDREAMING);
}

void Simul_MartineTripToSapins()
{
  TsRange	tsr;
  TsRangeParse(&tsr, "19940812T080000:na");
  DiscourseInit(StdDiscourse, &tsr.startts, NULL, NULL, NULL, 0);
  DbAssert(&tsr, ObjAtGridCreate(N("Martine"), N("sapins-15E"), 5, 45));
  DbAssert(&tsr, ObjAtGridCreate(N("Martine-Mom"), N("sapins-15E"), 5, 55));
  DbAssert(&tsr, ObjAtGridCreate(N("Martine-Dad"), N("sapins-15E"), 6, 55));
  TG(StdDiscourse->cx_best, &tsr.startts, N("Martine"),
     L(N("near-reachable"), N("Martine"), N("maison-de-Lucie-Entrance"), E));
  PA_MainLoop(StdDiscourse->cx_best, MODE_DAYDREAMING);
}

void App_Simul(FILE *in, FILE *out, FILE *err)
{
  int	savemode;
  char	buf[WORDLEN];
  Ts	ts;

  TsSetNow(&ts);
  DiscourseContextInit(StdDiscourse, &ts);

  fprintf(out, "[Simulation]\n");
  fprintf(out, "NUMBER CHARACTER GOAL\n");
  fprintf(out, "1......Jim.......watch-TV\n");
  fprintf(out, "2......Jim.......attend-performance\n");
  fprintf(out, "3......Jim.......take-shower\n");
  fprintf(out, "4......Jim.......dress\n");
  fprintf(out, "5......Jim.......near-reachable JFK-gate-26\n");
  fprintf(out, "6......Jim.......inside penny1 jar2\n");
  fprintf(out, "7......Martine...near-reachable maison-de-Lucie-Entrance\n");
  while (StreamAsk(in, out, "Enter [Simulation] number: ", WORDLEN, buf)) {
    savemode = StdDiscourse->mode;
    StdDiscourse->mode = DC_MODE_THOUGHTSTREAM;
    if (streq(buf, "1"))      Simul_JimWatchTV();
    else if (streq(buf, "2")) Simul_JimAttendPerformance();
    else if (streq(buf, "3")) Simul_JimTakeShower();
    else if (streq(buf, "4")) Simul_JimDress();
    else if (streq(buf, "5")) Simul_JimTripToJFK();
    else if (streq(buf, "6")) Simul_JimPennyJars();
    else if (streq(buf, "7")) Simul_MartineTripToSapins();
    else fprintf(err, "What?\n");
    StdDiscourse->mode = savemode;
  }
}

/* End of file. */
