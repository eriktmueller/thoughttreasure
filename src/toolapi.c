/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19981022T121823: begun
 * 19981115T081021: chatterbot
 */

/* Easier-to-use ThoughtTreasure application programming interface (API).
 * This is not complete; see also the functions at the bottom of tt.h
 * (N, I, PNI, L, Db*, ...), ISA, ISAP, and toolsvr.c.
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "repbasic.h"
#include "repchan.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptime.h"
#include "semdisc.h"
#include "synparse.h"
#include "synpnode.h"
#include "ta.h"
#include "tale.h"
#include "utildbg.h"

Discourse *API_DiscourseCreate()
{
  Discourse     *dc;
  dc = DiscourseCreate(StdDiscourse->langs, StdDiscourse->dialects);
  DiscourseInit(dc, NULL, N("string"), N("role-speaker"), N("role-listener"),
                0);
  return dc;
}

void API_DiscourseFree(Discourse *dc)
{
  DiscourseFree(dc);
}

void API_DiscourseSetInput(Discourse *dc, int lang, int dialect, int style,
                           char *s)
{
  dc->mode = DC_MODE_PROGRAMMER|DC_MODE_CONV;
  DiscourseCloseChannels(dc);
  DiscourseOpenChannel(dc, DCIN, "STRING", INTPOSINF, "r", 0, lang, dialect,
                       style, 0, 0, 1, s);
}

/* Do part-of-speech tagging and scanning of text for other entities.
 * Example:
 *   int retcode;
 *   dc=API_DiscourseCreate();
 *   API_DiscourseSetInput(dc, F_ENGLISH, F_NULL, F_NULL, "Mary had a lamb.");
 *   retcode=API_Tag(dc);
 *   API_DiscourseFree(dc);
 * Output:
 * (1) retcode: 1=success, 0=failure
 * (2) DCI(dc, DCIN).pnf: PNodeList (list of parse nodes)
 */
int API_Tag(Discourse *dc)
{
  Channel   *ch;
  dc->mode = DC_MODE_PROGRAMMER|DC_MODE_CONV;
  dc->run_agencies = AGENCY_SYNTAX;
  DiscourseSetCurrentChannel(dc, DCIN);
  ch = &DC(dc);
  if (ch->len <= 0L) return 0;
  TA_Scan(ch, dc);
  return 1;
}

/* Returns list of list of parses; one list for each found sentence. */
ObjListList *API_SynSemParse(Discourse *dc, int isSemParse)
{
  int       eoschar;
  size_t    pos;
  Channel   *ch;
  PNode     *pn;
  ObjListList *r;

  DiscourseSetCurrentChannel(dc, DCIN);
  ch = &DC(dc);

  /* Parse each sentence. */
  pos = 0;
  pn = ch->pnf->first;
  r = NULL;
  while (pos < ch->len) {
    if (NULL == (pn = PNodeGetNext(pn, pos))) break;
    if (pn->type == PNTYPE_END_OF_SENT) {
      eoschar = ch->buf[pn->lowerb];
      DiscourseNextSentence(dc);
      Sem_ParseResults = NULL;
      Syn_ParseParse(ch, dc, pos, pn->lowerb, eoschar);
      if (isSemParse) {
        Sem_ParseResults = ObjListUniquifyDestSP(Sem_ParseResults);
        Sem_ParseResults = ObjListScoreSortPrune(Sem_ParseResults, 7);
      }
      if (Sem_ParseResults != NULL) {
        r = ObjListListCreate(Sem_ParseResults, r);
      }
      Syn_ParseParseDone(ch);
    }
    pos = pn->upperb+1;
  }
  return r;
}

/* Do syntactic parse of text.
 * Output:
 * - sentences: list of list of PNode
 * - DCI(dc, DCIN).pnf: PNodeList (syntactic parse forest)
 */
ObjListList *API_SyntacticParse(Discourse *dc)
{
  dc->mode = DC_MODE_PROGRAMMER|DC_MODE_CONV;
  dc->run_agencies = AGENCY_SYNTAX;
  return API_SynSemParse(dc, 0);
}

/* Do semantic parse of text.
 * (Note this also does a syntactic parse, so don't also call
 *  API_SyntacticParse. The various agencies are a bit intertwined
 *  and need to be isolated more from one another.)
 * Output:
 * - sentences: list of list of Obj/PNode
 */
ObjListList *API_SemanticParse(Discourse *dc)
{
  dc->mode = DC_MODE_PROGRAMMER|DC_MODE_CONV;
  dc->run_agencies = AGENCY_SYNTAX|AGENCY_SEMANTICS;
  return API_SynSemParse(dc, 1);
}

/* Caller should free result when done using:
 *   MemFree(result, "char StringReadFile");
 */
char *API_Generate(Discourse *dc, int lang, int dialect, int style, int eos,
                   Obj *obj)
{
  char    outfn[FILENAMELEN], *r;
  dc->mode = DC_MODE_PROGRAMMER|DC_MODE_CONV;
  dc->run_agencies = AGENCY_SYNTAX;
  FileTemp("/tmp/Generate", FILENAMELEN, outfn);
  DiscourseCloseChannels(dc);
  DiscourseOpenChannel(dc, DCOUT1, outfn, INTPOSINF, "w+", 0, lang, dialect,
                       style, 0, 0, 1, NULL);
  DiscourseGen(dc, 0, "", obj, eos);
  DiscourseCloseChannel(dc, DCOUT1);
  r = StringReadFile(outfn, 0);
  unlink(outfn);
  return r;
}

/* Caller should free result when done using:
 *   MemFree(result, "char StringReadFile");
 */
char *API_Chatterbot(Discourse *dc, int lang, int dialect, int style, char *s)
{
  char    outfn[FILENAMELEN], *r;
  dc->mode = DC_MODE_TURING|DC_MODE_CONV;
  dc->run_agencies = AGENCY_ALL;
  FileTemp("/tmp/Chatterbot", FILENAMELEN, outfn);
  DiscourseCloseChannels(dc);
  DiscourseOpenChannel(dc, DCIN, "STRING", INTPOSINF, "r", 0, lang, dialect,
                       style, 0, 0, 1, s);
  DiscourseOpenChannel(dc, DCOUT1, outfn, INTPOSINF, "w+", 0, lang, dialect,
                       style, 0, 0, 1, NULL);
  DiscourseReadUnderstand(dc);
  DiscourseCloseChannel(dc, DCIN);
  DiscourseCloseChannel(dc, DCOUT1);
  r = StringReadFile(outfn, 0);
  unlink(outfn);
  return r;
}

/* End of file. */
