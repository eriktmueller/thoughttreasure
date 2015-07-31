/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940704: cleanup
 * 19951008: added score
 */

#include "tt.h"
#include "lexentry.h"
#include "repbasic.h"
#include "repcxt.h"
#include "repdb.h"
#include "repgroup.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "repstr.h"
#include "reptime.h"
#include "semanaph.h"
#include "semcase.h"
#include "semdisc.h"
#include "semgen1.h"
#include "semgen2.h"
#include "semparse.h"
#include "synpnode.h"
#include "synxbar.h"
#include "utildbg.h"

CaseFrame *CaseFrameAdd(CaseFrame *next, Obj *cas, Obj *concept, 
                        Float score, PNode *pn, Anaphor *anaphors,
                        LexEntryToObj *leo)
{
  CaseFrame *cf;

  if (!ScoreIsValid(score)) {
    Dbg(DBGGEN, DBGBAD, "CaseFrameAdd: bad score");
    Stop();
    score = SCORE_MAX;
  }
 
  cf = CREATE(CaseFrame);
  cf->cas = cas;
  cf->concept = concept;
  cf->sp.score = score;
  cf->sp.pn = pn;
  cf->sp.anaphors = anaphors;
  cf->sp.leo = leo;
  cf->theta_marked = 0;
  cf->next = next;
  return(cf);
}

CaseFrame *CaseFrameAddSP(CaseFrame *next, Obj *cas, Obj *concept, 
                          SP *sp)
{
  return(CaseFrameAdd(next, cas, concept, sp->score, sp->pn, sp->anaphors,
                      sp->leo));
}

CaseFrame *CaseFrameRemove(CaseFrame *cf)
{
  CaseFrame	*r;
  r = cf->next;
  MemFree(cf, "CaseFrame");
  return(r);
}

void CaseFrameThetaMarkClear(CaseFrame *cf)
{
  for (; cf; cf = cf->next) cf->theta_marked = 0;
}

Obj *CaseFrameGet(CaseFrame *cf, Obj *cas, /* RESULTS */ SP *sp)
{
  for (; cf; cf = cf->next) {
    if (cas == cf->cas) {
      cf->theta_marked = 1;
      if (sp) *sp = cf->sp;
      return(cf->concept);
    }
  }
  if (sp) SPInit(sp);
  return(NULL);
}

int CaseFrameIsClause(PNode *z)
{
/* Keep this consistent with Sem_ParseSatisfiesSSubcategRestrict. */
  if (z == NULL) return(0);
  return(z->feature == F_S ||
         (z->feature == F_NP &&
          z->pn2 == NULL &&
          z->pn1 && z->pn1->feature == F_S) ||
         (z->feature == F_PP &&
          z->pn1 && z->pn1->feature == F_PREPOSITION &&
          z->pn2 && z->pn2->feature == F_NP &&
          z->pn2->pn1 && z->pn2->pn1->feature == F_S));
}

/* This just makes sure a clausal theta role is given a clause.
 * todo: Move all subcategorization matching here?
 */
int CaseFrameSubcatOK(CaseFrame *cf, Obj *cas, int subcat)
{
  if (cas != N("obj") && cas != N("iobj")) return(1);
  if (subcat == F_NULL) {
    return(!CaseFrameIsClause(cf->sp.pn));
  } else {
    return(CaseFrameIsClause(cf->sp.pn));
  }
}

Obj *CaseFrameGetLe(CaseFrame *cf, Obj *cas, LexEntry *le, int subcat,
                    /* RESULTS */ SP *sp)
{
  for (; cf; cf = cf->next) {
    if (cas == cf->cas &&
        ((!le) || le == PNodeLeftmostPreposition(cf->sp.pn)) &&
        CaseFrameSubcatOK(cf, cas, subcat)) {
      cf->theta_marked = 1;
      if (sp) *sp = cf->sp;
      return(cf->concept);
    }
  }
  if (sp) SPInit(sp);
  return(NULL);
}

Obj *CaseFrameGetClass(CaseFrame *cf, Obj *cas, Obj *class,
                       /* RESULTS */ SP *sp)
{
  for (; cf; cf = cf->next) {
    if (cas == cf->cas && (!cf->theta_marked) && ISA(class, cf->concept)) {
      cf->theta_marked = 1;
      if (sp) *sp = cf->sp;
      return(cf->concept);
    }
  }
  if (sp) SPInit(sp);
  return(NULL);
}

/* As per the theta-criterion (Chomsky, 1982/1987, p. 26)
 * do not theta-mark an argument more than once.
 */
Obj *CaseFrameGetNonThetaMarked(CaseFrame *cf, Obj *cas, /* RESULTS */ SP *sp)
{
  for (; cf; cf = cf->next) {
    if ((!cf->theta_marked) && cas == cf->cas) {
      cf->theta_marked = 1;
      if (sp) *sp = cf->sp;
      return(cf->concept);
    }
  }
  if (sp) SPInit(sp);
  return(NULL);
}

Obj *CaseFrameGetLe_DoNotMark(CaseFrame *cf, Obj *cas, LexEntry *le,
                              /* RESULTS */ SP *sp)
{
  for (; cf; cf = cf->next) {
    if (cas == cf->cas &&
        ((!le) || le == PNodeLeftmostPreposition(cf->sp.pn))) {
      if (sp) *sp = cf->sp;
      return(cf->concept);
    }
  }
  if (sp) SPInit(sp);
  return(NULL);
}

Bool CaseFrameThetaMarkExpletive(CaseFrame *cf, LexEntry *le)
{
  for (; cf; cf = cf->next) {
    if (cf->cas == N("expl") && (!cf->theta_marked) &&
        ((!le) || le == PNodeLeftmostLexEntry(cf->sp.pn))) {
      cf->theta_marked = 1;
      return(1);
    }
  }
  return(0);
}

void CaseFramePrint1(FILE *stream, CaseFrame *cf)
{
  fprintf(stream, "{%s:", M(cf->cas));
  if (cf->concept) {
    fputc(SPACE, stream);
    ObjPrint(stream, cf->concept);
  }
  fputc(SPACE, stream);
  SPPrint(stream, &cf->sp);
/*
  if (cf->cas == N("iobj") || cf->cas == N("expl")) {
    if (le = PNodeLeftmostLexEntry(cf->sp.pn)) {
      fprintf(stream, " <%s.%s>", le->srcphrase, le->features);
    }
  }
 */
  fputs("}", stream);
}

void CaseFramePrint(FILE *stream, CaseFrame *cf)
{
  for (; cf; cf = cf->next) {
    IndentPrint(stream);
    CaseFramePrint1(stream, cf);
    fputc(NEWLINE, stream);
  }
}

/* As per the theta-criterion (Chomsky, 1982/1987, p. 26)
 * make sure all arguments are theta-marked.
 * What about extra subjects passed down in case of potential deletion?
 */
Bool CaseFrameDisobeysThetaCriterion(CaseFrame *cf, Obj *obj)
{
  for (; cf; cf = cf->next) {
  /* todo: This is too drastic in the case of iobjs? */
    if ((!cf->theta_marked) &&
        (cf->cas == N("obj") || cf->cas == N("iobj") || cf->cas == N("expl"))) {
      if (DbgOn(DBGSEMPAR, DBGDETAIL)) {
        fprintf(Log, "<%s> rejected because ",  M(obj));
        CaseFramePrint1(Log, cf);
        fputs(" not theta-marked\n", Log);
      }
      return(1);
    }
  }
  return(0);
}

/* End of file. */
