/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940313: ThoughtTreasure begun in Think C / Mac OS 7.1
 * 19951018: port to ProWorks / Solaris 2.4 begun
 * 19951020: port seems basically to work
 * 19970712: port to gcc / DJGPP 2.01 / Windows95
 * 19981113: port to Red Hat Linux 5.2
 * 20150731: port to Apple Command Line Tools 6.4 / OS X 10.10.4
 */

#include "tt.h"
#include "apptrans.h"
#include "lexentry.h"
#include "lexmorph.h"
#include "lexobjle.h"
#include "lexwf1.h"
#include "lexwf2.h"
#include "lexwf3.h"
#include "repbasic.h"
#include "repchan.h"
#include "repcxt.h"
#include "repdb.h"
#include "repdbf.h"
#include "repgrid.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repprove.h"
#include "repstr.h"
#include "reptime.h"
#include "semanaph.h"
#include "semdisc.h"
#include "semparse.h"
#include "synbase.h"
#include "synparse.h"
#include "synpnode.h"
#include "taname.h"
#include "toolrpt.h"
#include "toolsh.h"
#include "uaquest.h"
#include "utilbb.h"
#include "utildbg.h"
#include "utilhtml.h"
#include "utillrn.h"

Bool            DoLoad, Starting, SaveTime, GenOnAssert;
Discourse	*StdDiscourse;
Obj		*Me;
char            *TTRoot;

void NoticePrint(FILE *stream)
{
  StreamSep(stream);
fprintf(stream, "ThoughtTreasure version 0.00023 of 20150731 (");
/* exhaustive ifdefs */
#ifdef GCC
fprintf(stream, "gcc)\n");
#endif
#ifdef SOLARIS
fprintf(stream, "Solaris)\n");
#endif
#ifdef SUNOS
fprintf(stream, "SunOS)\n");
#endif
#ifdef MACOS
fprintf(stream, "Mac OS)\n");
#endif
#ifdef NEXT
fprintf(stream, "NEXT)\n");
#endif
fprintf(stream, "Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.\n");
fprintf(stream, "All Rights Reserved.\n");
fprintf(stream,
"ThoughtTreasure comes with ABSOLUTELY NO WARRANTY.\n");
fprintf(stream,
"By using ThoughtTreasure, you are agreeing to be bound by the terms of\n");
fprintf(stream,
"the License Agreement. (Type 'legal' to view.)\n");
  fprintf(stream, "For a list of commands type 'help'.\n");
  StreamSep(stream);
}

void EnvInit()
{
  if (NULL == (TTRoot = getenv("TTROOT"))) {
    TTRoot = "..";
    Dbg(DBGGEN, DBGOK, "TTROOT not set, assuming <%s>", TTRoot);
  } else {
    Dbg(DBGGEN, DBGOK, "TTROOT = <%s>", TTRoot);
  }
}

void Init(char *langs, char *dialects, int anamorph)
{
  Starting = 1;
  GenOnAssert = 1;
  LexEntryOff = 0;
  SaveTime = 1;
    /* SaveTime = 1: no spell correction hashing or phrase derivation,
     *               saves memory.
     * SaveTime = 0: uses more time and memory.
     */
  qallocInit();
  DbgInit();
  DbgSet(DBGALL, DBGBAD);
  DbgSetStdoutLevel(DBGOK);
  NoticePrint(stderr);
  NoticePrint(Log);
  EnvInit();
  StringInit();
  RandomInit();
  GridInit();
  ObjInit();
  ObjListInit();
  DbInit();
  ContextInit();
  TsInit();
  TsRangeInit();
  LexEntryInit();
  Lex_WordForm2Init();
  MorphInit(anamorph);
  WordFormInit();
  InferenceInit();
  ReportInit();
  CommentaryInit();
  TranslateInit();
  LearnInit();
  StopAtInit();
  TT_HTML_Init();
  StdDiscourse = DiscourseCreate(langs, dialects);
  DiscourseSetLang(StdDiscourse, F_ENGLISH);
  Starting = 0;
}

void LoadBegin()
{
  Starting = 1;
  BaseRuleCompile();
  LexEntryStatsBegin();
}

void LoadEnd()
{
  LexEntryStatsEnd();
  TA_NameInit();
  Me = N("TT");
  Sem_ParseInit();
  Starting = 0;
}

void Load()
{
  Ts	ts1, ts2;
  Dur	d;
  TsSetNow(&ts1);
  if (StringIn(F_FRENCH, StdDiscourse->langs)) {
    LexEntryReadInflFile("db/frinfl.txt");
  }
  if (StringIn(F_ENGLISH, StdDiscourse->langs)) {
    LexEntryReadInflFile("db/eninfl.txt");
  }
  DbFileRead("db/name.txt", DBFILETYPE_ISA);
  DbFileRead("db/food.txt", DBFILETYPE_ISA);
  DbFileRead("db/drug.txt", DBFILETYPE_ISA);
  DbFileRead("db/geog.txt", DBFILETYPE_POLITY);
  DbFileRead("db/absobj.txt", DBFILETYPE_ISA);
  DbFileRead("db/street.txt", DBFILETYPE_ISA);
  DbFileRead("db/grid.txt", DBFILETYPE_ISA);
  DbFileRead("db/all.txt", DBFILETYPE_ISA);
  DbFileRead("db/physics.txt", DBFILETYPE_ISA);
  DbFileRead("db/chem.txt", DBFILETYPE_ISA);
  DbFileRead("db/trans.txt", DBFILETYPE_ISA);
  DbFileRead("db/celest.txt", DBFILETYPE_ISA);
  DbFileRead("db/physobj.txt", DBFILETYPE_ISA);
  DbFileRead("db/photo.txt", DBFILETYPE_ISA);
  DbFileRead("db/furniture.txt", DBFILETYPE_ISA);
  DbFileRead("db/personalarticle.txt", DBFILETYPE_ISA);
  DbFileRead("db/officeproduct.txt", DBFILETYPE_ISA);
  DbFileRead("db/appliance.txt", DBFILETYPE_ISA);
  DbFileRead("db/hardware.txt", DBFILETYPE_ISA);
  DbFileRead("db/toy.txt", DBFILETYPE_ISA);
  DbFileRead("db/musicinstrument.txt", DBFILETYPE_ISA);
  DbFileRead("db/ling.txt", DBFILETYPE_ISA);
  /* FeatPrintUnused(stdout); */
  DbFileRead("db/relation.txt", DBFILETYPE_ISA);
  DbFileRead("db/action.txt", DBFILETYPE_ISA);
  DbFileRead("db/attr.txt", DBFILETYPE_ISA);
  DbFileRead("db/enum.txt", DBFILETYPE_ISA);
  DbFileRead("db/living.txt", DBFILETYPE_ISA);
  DbFileRead("db/human.txt", DBFILETYPE_ISA);
  DbFileRead("db/clothing.txt", DBFILETYPE_ISA);
  DbFileRead("db/mediaobj.txt", DBFILETYPE_ISA);
  DbFileRead("db/software.txt", DBFILETYPE_ISA);
  DbFileRead("db/book.txt", DBFILETYPE_ISA);
  DbFileRead("db/homeentertain.txt", DBFILETYPE_ISA);
  DbFileRead("db/liveentertain.txt", DBFILETYPE_ISA);
  DbFileRead("db/film.txt", DBFILETYPE_ISA);
  DbFileRead("db/theory.txt", DBFILETYPE_ISA);
  DbFileRead("db/sound.txt", DBFILETYPE_ISA);
  DbFileRead("db/music.txt", DBFILETYPE_ISA);
  DbFileRead("db/musicconcept.txt", DBFILETYPE_ISA);
  DbFileRead("db/tv.txt", DBFILETYPE_ISA);
  DbFileRead("db/company.txt", DBFILETYPE_ISA);
  DbFileRead("db/elec.txt", DBFILETYPE_ISA);
  DbFileRead("db/net.txt", DBFILETYPE_ISA);
  TsSetNow(&ts2);
  d = TsMinus(&ts2, &ts1);
  Dbg(DBGGEN, DBGBAD, "Load time = %.2ld:%.2ld.", d/60, d%60);
}

int main(int argc, char **argv)
{
  int		c, errflg, anamorph;
  char		*ttshell_file, *ttshell_cmd;
  char		*langs, *dialects;
#ifdef MACOS
#else
  extern char	*optarg;
  extern int	optind;
#endif

  anamorph = 0;
  DoLoad = 1;

  /* Default is English-only. For English and French, do:
   *   tt -g zy -d ?Àgç
   */
  langs = "z";
  dialects = "?Àg";

  /* printf("sizeof(Obj) = %d bytes\n", sizeof(Obj)); */
  errflg = 0;
  ttshell_file = NULL;
  ttshell_cmd = NULL;
#ifdef MACOS
  /* todo: implement option parsing on Mac. */
#else
  while ((c = getopt(argc, argv, "alc:d:f:g:")) != EOF) {
    switch (c) {
      case 'a':
        anamorph = 1;
        /* anamorph==1: AnaMorph (analogical morphology), uses more memory.
         * anamorph==0: AlgMorph (algorithmic morphology), saves memory.
         */
        break;
      case 'l':
        DoLoad = 0;
        break;
      case 'c':
        ttshell_cmd = optarg;
        break;
      case 'd':
        dialects = optarg;
        break;
      case 'f':
        ttshell_file = optarg;
        break;
      case 'g':
        langs = optarg;
        break;
      case '?':
        errflg++;
    }
  }
#endif
  if (errflg) {
    fprintf(stderr,
      "usage: tt [-a] [-l] [-c cmd] [-f file] [-g langs] [-d dialects]\n");
    exit(1);
  }

/*
  for ( ; optind < argc; optind++) {
    printf("%s\n", argv[optind]);
  }
 */

  Init(langs, dialects, anamorph);
  if (DoLoad) {
    LoadBegin();
    Load();
    LoadEnd();
  }

  if (ttshell_cmd) {
    Tool_Shell_Interpret(ttshell_cmd, stdin, Log, Log, StdDiscourse);
  }
  if (ttshell_file) {
    Tool_Shell_TT(ttshell_file, "(command-line arguments to tt)",
                  stdin, Log, Log, StdDiscourse);
  }

  DbgSet(DBGALLSEM, DBGDETAIL);
  MemCheckPrint();
  MemCheckReset();
  DbgLastObj = NULL;

  Lex_WordForm2DoneLoading();
  while (1) Tool_Shell_Std();
}

/* End of file. */
