/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19951021: begun
 * 19981021T144459: mods
 * 19981210T111300: registered port number
 */

#include "tt.h"

#include "appsimul.h"
#include "compnoun.h"
#include "lexalgm.h"
#include "lexanam.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "lexwf1.h"
#include "lexwf2.h"
#include "lexwf3.h"
#include "pa.h"
#include "pamtrans.h"
#include "repbasic.h"
#include "repchan.h"
#include "repdb.h"
#include "repdbf.h"
#include "repobj.h"
#include "repobjl.h"
#include "repprove.h"
#include "repstr.h"
#include "reptime.h"
#include "semdisc.h"
#include "synpnode.h"
#include "ta.h"
#include "taemail.h"
#include "tale.h"
#include "taname.h"
#include "taprod.h"
#include "tatable.h"
#include "tatagger.h"
#include "tatime.h"
#include "toolcorp.h"
#include "toolfilt.h"
#include "toolgrd.h"
#include "toolrpt.h"
#include "toolsh.h"
#include "toolsvr.h"
#include "tooltest.h"
#include "uadict.h"
#include "uascript.h"
#include "utildbg.h"
#include "utilhtml.h"
#include "utillrn.h"
#include "utilmain.h"

void Tool_Shell_Std()
{
  Tool_Shell(stdin, stdout, stderr, StdDiscourse);
}

void Tool_Shell_Prompt(FILE *in, FILE *out)
{
  if (in == stdin) fputs("* ", out);
}

void Tool_Shell(FILE *in, FILE *out, FILE *err, Discourse *dc)
{
  char  	line[LINELEN], prev[LINELEN];
  time_t	start, finish;
  Dbg(DBGGEN, DBGDETAIL, "Entering ThoughtTreasure shell");
  if (DoLoad) ObjValidate();
  Tool_Shell_Prompt(in, out);
  prev[0] = TERM;
  while (fgets(line, LINELEN, in)) {
    line[strlen(line)-1] = TERM;
    if (streq(line, "!!")) {
      StringCpy(line, prev, LINELEN);
      fprintf(out, "%s\n", line);
    }
    start = time(0);
    if (Tool_Shell_Interpret(line, in, out, err, dc)) {
      StringCpy(prev, line, LINELEN);
    }
    finish = time(0);
    if ((finish - start) > 0) {
      fprintf(Log, "Time spent on command = %ld seconds\n",
              (long)(finish-start));
    }
    Tool_Shell_Prompt(in, out);
  }
}

int Tool_Shell_Error(char *errmsg, char *arg0, char *opt, char *line, FILE *in,
                     FILE *out, FILE *err, Discourse *dc)
{
  Dbg(DBGGEN, DBGBAD, "ThoughtTreasure shell error <%s> in <%s>", errmsg, line);
  if (in != stdin) fprintf(err, "<%s> ", line);
  if (arg0 && arg0[0]) fprintf(err, "%s: ", arg0);
  if (opt && opt[0]) fprintf(err, "%s: ", opt);
  if (errmsg && errmsg[0]) fprintf(err, "%s\n", errmsg);
  return(0);
}

void ShellArgsInit(ShellArgs *sa)
{
  sa->output_reps = 0;
  sa->lang = F_ENGLISH;
  sa->dialect = F_AMERICAN;
  sa->style = F_NULL;
  sa->translate = 0;
  sa->parain = 0;
  sa->echoin = 1;
  sa->port = PORT;
  sa->new[0] = TERM;
  sa->old[0] = TERM;
  sa->file[0] = TERM;
  sa->dir[0] = TERM;
  sa->word[0] = TERM;
  sa->speaker = N("Jim");
  sa->listener = Me;
  sa->obj = NULL;
  sa->dbgflags = -1;
  sa->dbglevel = -1;
}

int Tool_Shell_Interpret(char *line, FILE *in, FILE *out, FILE *err,
                         Discourse *dc)
{
  char		arg0[PHRASELEN], opt[PHRASELEN], optarg[PHRASELEN];
  char		*p;
  int		next_dc_out, retcode;
  ShellArgs	sa;

  if (line[0] == TREE_COMMENT) return(0);
  Dbg(DBGGEN, DBGDETAIL, "ThoughtTreasure shell interpreting line <%s>", line);

  ShellArgsInit(&sa);

  next_dc_out = DCOUT1;
  dc->run_agencies = AGENCY_ALL;
  GenAdviceInit(&dc->ga);
  dc->mode &= ~(DC_MODE_THOUGHTSTREAM|DC_MODE_COMPOUND_NOUN);

  p = line;
  p = StringReadWord(p, PHRASELEN, arg0);
  if (arg0[0] == TERM) return(0);
  if (arg0[0] == '#') return(0);
  while (1) {
    if (p[0] == '#') break;	/* todo */
    p = StringReadWord(p, PHRASELEN, opt);
    if (opt[0] == TERM) break;
    if (opt[0] == '-') {
      p = StringReadWord(p, PHRASELEN, optarg);
      if (optarg[0] == TERM) {
        return(Tool_Shell_Error("missing option argument", arg0, opt, line,
                                in, out, err, dc));
      }
    }
    if (streq(opt, "-dcout")) {
      if (!DiscourseOpenChannel(dc, next_dc_out, optarg, DSPLINELEN, "w+",
                                sa.output_reps, sa.lang, sa.dialect, sa.style,
                                sa.parain, sa.echoin, 1, NULL)) {
        return(Tool_Shell_Error("open failed", arg0, opt, line, in, out,
                                err, dc));
      }
      next_dc_out++;
    } else if (streq(opt, "-dcin")) {
      if (!DiscourseOpenChannel(dc, DCIN, optarg, DSPLINELEN, "r", 0, sa.lang,
                                F_NULL, F_NULL, 0, 0, !streq(arg0, "pcn"),
                                NULL)) {
        return(Tool_Shell_Error("open failed", arg0, opt, line, in, out,
                                err, dc));
      }
    } else if (streq(opt, "-outsyn")) {
      if (atoi(optarg)) sa.output_reps |= AGENCY_SYNTAX;
      else sa.output_reps &= ~AGENCY_SYNTAX;
    } else if (streq(opt, "-outsem")) {
      if (atoi(optarg)) sa.output_reps |= AGENCY_SEMANTICS;
      else sa.output_reps &= ~AGENCY_SEMANTICS;
    } else if (streq(opt, "-outana")) {
      if (atoi(optarg)) sa.output_reps |= AGENCY_ANAPHORA;
      else sa.output_reps &= ~AGENCY_ANAPHORA;
    } else if (streq(opt, "-outund")) {
      if (atoi(optarg)) sa.output_reps |= AGENCY_UNDERSTANDING;
      else sa.output_reps &= ~AGENCY_UNDERSTANDING;
    } else if (streq(opt, "-runsyn")) {
      if (atoi(optarg)) dc->run_agencies |= AGENCY_SYNTAX;
      else dc->run_agencies &= ~AGENCY_SYNTAX;
    } else if (streq(opt, "-runsem")) {
      if (atoi(optarg)) dc->run_agencies |= AGENCY_SEMANTICS;
      else dc->run_agencies &= ~AGENCY_SEMANTICS;
    } else if (streq(opt, "-runana")) {
      if (atoi(optarg)) dc->run_agencies |= AGENCY_ANAPHORA;
      else dc->run_agencies &= ~AGENCY_ANAPHORA;
    } else if (streq(opt, "-runund")) {
      if (atoi(optarg)) dc->run_agencies |= AGENCY_UNDERSTANDING;
      else dc->run_agencies &= ~AGENCY_UNDERSTANDING;
    } else if (streq(opt, "-lang")) {
    /* todo: Error checking. */
      sa.lang = optarg[0];
    } else if (streq(opt, "-dialect")) {
      sa.dialect = optarg[0];
    } else if (streq(opt, "-style")) {
      sa.style = optarg[0];
    } else if (streq(opt, "-parain")) {
      sa.parain = atoi(optarg);
    } else if (streq(opt, "-echoin")) {
      sa.echoin = atoi(optarg);
    } else if (streq(opt, "-port")) {
      sa.port = atoi(optarg);
    } else if (streq(opt, "-translate")) {
      sa.translate = atoi(optarg);
    } else if (streq(opt, "-new")) {
      StringCpy(sa.new, optarg, PHRASELEN);
    } else if (streq(opt, "-old")) {
      StringCpy(sa.old, optarg, PHRASELEN);
    } else if (streq(opt, "-file") || streq(opt, "-f")) {
      StringCpy(sa.file, optarg, PHRASELEN);
    } else if (streq(opt, "-dir")) {
      StringCpy(sa.dir, optarg, PHRASELEN);
    } else if (streq(opt, "-obj") || streq(opt, "-o")) {
      sa.obj = N(optarg);
    } else if (streq(opt, "-speaker")) {
      sa.speaker = N(optarg);
    } else if (streq(opt, "-listener")) {
      sa.listener = N(optarg);
    } else if (streq(opt, "-word") || streq(opt, "-w")) {
      StringCpy(sa.word, optarg, PHRASELEN);
    } else if (streq(opt, "-flags")) {
      if (-1 == (sa.dbgflags = DbgStringToFlags(optarg))) {
        return(Tool_Shell_Error("unknown dbg flags", arg0, opt, line, in, out,
                                err, dc));
      }
    } else if (streq(opt, "-level")) {
      if (-1 == (sa.dbglevel = DbgStringToLevel(optarg))) {
        return(Tool_Shell_Error("unknown dbg level", arg0, opt, line, in, out,
                                err, dc));
      }
    } else {
      return(Tool_Shell_Error("unknown option", arg0, opt, line, in, out,
                              err, dc));
    }
  }

  retcode = Tool_Shell_Interpret_Command(arg0, &sa, line, in, out, err, dc);
  DiscourseCloseChannels(dc);
  return(retcode);
}

int Tool_Shell_Interpret_Command(char *cmd, ShellArgs *sa, char *line, FILE *in,
                                 FILE *out, FILE *err, Discourse *dc)
{
  if (streq(cmd, "parse")) {
    return(Tool_Shell_Parse(sa->translate, sa->speaker, sa->listener,
                            line, in, out, err, dc));
  } else if (streq(cmd, "pcn")) {
    return(Tool_Shell_ParseCompoundNoun(dc));
  } else if (streq(cmd, "validate")) {
    return(Tool_Shell_Validate(sa->new, sa->old, line, in, out, err, dc));
  } else if (streq(cmd, "objhref")) {
    return(Tool_Shell_ObjHref(sa->obj, in, out, err, dc));
  } else if (streq(cmd, "tt")) {
    return(Tool_Shell_TT(sa->file, line, in, out, err, dc));
  } else if (streq(cmd, "dbg")) {
    return(Tool_Shell_Dbg(sa->dbgflags, sa->dbglevel, line, in, out, err, dc));
  } else if (streq(cmd, "corpusload") || streq(cmd, "cl")) {
    return(Tool_Shell_CorpusLoad(sa->lang, sa->file, sa->dir, line, in, out,
                                 err, dc));
  } else if (streq(cmd, "corpusfind") || streq(cmd, "cfe") ||
             streq(cmd, "cff")) {
    if (streq(cmd, "cfe")) sa->lang = F_ENGLISH;
    else if (streq(cmd, "cff")) sa->lang = F_FRENCH;
    return(Tool_Shell_CorpusFind(sa->lang, sa->word, line, in, out, err, dc));
  } else if (streq(cmd, "adverbial")) {
    if (sa->dir[0]) {
      return(Corpus_AdverbialFinderDirectory(sa->dir, "outadv.txt"));
    } else {
      return(Corpus_AdverbialFinderFile(sa->file, "outadv.txt"));
    }
  } else if (streq(cmd, "stop")) {
    Stop();
    return(0);
  } else if (streq(cmd, "exit") || streq(cmd, "quit")) {
    Exit(0);
    return(1);
  } else if (streq(cmd, "test")) {
    DbgLogClear();
    return(Tool_Shell_Interpret("tt -f ../examples/test.tts",in,out,err,dc));
  } else if (streq(cmd, "wf2e")) {
    return(Lex_WordForm2Test(sa->file, EnglishIndex));
  } else if (streq(cmd, "wf2f")) {
    return(Lex_WordForm2Test(sa->file, FrenchIndex));
  } else if (Tool_Shell_Interpret_Other_Command(cmd, sa, in, out, err, dc)) {
    return(1);
  } else if (streq(cmd, "logclear") || streq(cmd, "lc")) {
    DbgLogClear();
    return(0);
  } else if (streq(cmd, "loadbegin")) {
    LoadBegin();
    return(0);
  } else if (streq(cmd, "loadend")) {
    LoadEnd();
    return(0);
  } else if (streq(cmd, "loadinf")) {
    LexEntryReadInflFile(sa->file);
    return(0);
  } else if (streq(cmd, "loadisa")) {
    DbFileRead(sa->file, DBFILETYPE_ISA);
    return(0);
  } else if (streq(cmd, "loadpolity")) {
    DbFileRead(sa->file, DBFILETYPE_POLITY);
    return(0);
  } else if (streq(cmd, "server")) {
    Tool_Server(sa->port);
    return(0);
  }
  return(Tool_Shell_Error("unknown command", cmd, "", line, in, out,
                          err, dc));
}

void Legal()
{
  char	*s;
  if (NULL == (s = StringReadFile("ttlegal.txt", 1))) {
    puts("ttlegal.txt not found. See GNU GPL Version 2.\n");
    return;
  }
  puts(s);
  MemFree(s, "char StringReadFile");
}

void Help()
{
  char	*s;
  if (NULL == (s = StringReadFile("help.txt", 1))) {
    puts("help.txt not found.\n");
    return;
  }
  puts(s);
  MemFree(s, "char StringReadFile");
}

int Tool_Shell_Interpret_Other_Command(char *buf, ShellArgs *sa, FILE *in,
                                       FILE *out, FILE *err, Discourse *dc)
{
  if (streq(buf, "legal"))             Legal();
  else if (streq(buf, "help"))         Help();
  else if (streq(buf, "db"))           DbQueryTool();
  else if (streq(buf, "prover"))       ProverTool();
  else if (streq(buf, "proveit"))      ProveItTest();
  else if (streq(buf, "obj"))          ObjQueryTool();
  else if (streq(buf, "dict0"))        Dictionary();
  else if (streq(buf, "dict")) {
    return(Tool_Shell_Interpret(
"dict0 -lang z -dcout outdicte.txt -lang y -dialect ? -dcout outdictf.txt",
                                in, out, err, dc));
  } else if (streq(buf, "chateng") || streq(buf, "chatter") ||
             streq(buf, "chatterbot") || streq(buf, "talkeng")) {
    Chatterbot(F_ENGLISH, F_AMERICAN, sa->speaker, sa->listener);
  } else if (streq(buf, "chatfr") || streq(buf, "talkfr")) {
    Chatterbot(F_FRENCH, F_NULL, sa->speaker, sa->listener);
  } else if (streq(buf, "daydream"))     Daydream();
  else if (streq(buf, "simul"))        App_Simul(in, out, err);
  else if (streq(buf, "report"))       Tool_Report();
  else if (streq(buf, "filtfeat"))     Tool_Filter(out);
  else if (streq(buf, "covcheckeng")) {
    LexEntryCoverageCheck("In", EnglishIndex, 0);
  } else if (streq(buf, "covcheckfr")) {
    LexEntryCoverageCheck("In", FrenchIndex, 0);
  } else if (streq(buf, "covcheckengm")) {
    LexEntryCoverageCheck("In", EnglishIndex, 1);
  } else if (streq(buf, "covcheckfrm")) {
    LexEntryCoverageCheck("In", FrenchIndex, 1);
  } else if (streq(buf, "suffgender"))  LexEntrySuffixGender();
  else if (streq(buf, "inflscan"))      LexEntryInflScan();
  else if (streq(buf, "polysem"))       LexEntryPrintPolysemous(Log, sa->lang);
  else if (streq(buf, "polyscript"))    ScriptPrintPolysemous(Log, sa->lang);
  else if (streq(buf, "posambig"))      LexEntryPrintPOSAmbiguous(EnglishIndex);
  else if (streq(buf, "lexentry"))      LexEntryTool();
  else if (streq(buf, "lexdumper"))     LexiconDumper();
  else if (streq(buf, "lexentryscan"))  LexEntryScan();
  else if (streq(buf, "lexentryjuxt"))  LexEntryJuxtapose(in, out, err);
  else if (streq(buf, "algmorph"))      AlgMorphTool();
  else if (streq(buf, "anamorph"))      AnaMorphTool();
  else if (streq(buf, "wf1fill"))       WordFormFillin();
  else if (streq(buf, "wf1train"))      WordFormTrain();
  else if (streq(buf, "learnnames"))    LearnNewNames(F_MASCULINE);
  else if (streq(buf, "corpusvalagainst")) {
    LexEntryValidateAgainstCorpus(CorpusFrench, F_FRENCH);
  } else if (streq(buf, "html")) {
    StreamHTMLMassageDirectory("../htm/", "/tmp"); /* todo */
  } else if (streq(buf, "genhtml")) {
    StreamHTMLMassageDirectory("../htm/", "/tmp"); /* todo */
    TT_HTML(0, 1, 1);
  }
  else if (streq(buf, "testgenattr"))   TestGenAttr();
  else if (streq(buf, "testcomptense")) GenTestCompTense();
  else if (streq(buf, "testgentempdb")) GenTestTemporalDb();
  else if (streq(buf, "testgentemp"))   GenTestTemporal();
  else if (streq(buf, "testgenrel"))    GenTestRelation();
  else if (streq(buf, "testtrip"))      TestTrip();
  else if (streq(buf, "testts"))        TestGenTsRange();
  else if (streq(buf, "testsa"))        TestGenSpeechActs();
  else if (streq(buf, "sortbyline"))    StreamSortIn(0);
  else if (streq(buf, "sortbytree"))    StreamSortIn(1);
    /* todo: Sortbytree: last line must be === */
  else if (streq(buf, "grind"))         GrinderGrind();
  else return(0);
  return(1);
}

/* parse -lang z -dcin STDIN -dcout STDOUT
 * parse -dcin In -dcout outuseng.txt -dialect g -dcout outukeng.txt
 */
int Tool_Shell_Parse(int translate, Obj *speaker, Obj *listener,
                     char *line, FILE *in, FILE *out, FILE *err, Discourse *dc)
{
  DiscourseParse(translate, speaker, listener);
  return(1);
}

/* pcn -dcin STDIN -dcout STDOUT
 * pcn -dcin inpcn.txt -dcout outpcn.txt
 */
int Tool_Shell_ParseCompoundNoun(Discourse *dc)
{
  dc->mode |= DC_MODE_COMPOUND_NOUN;
  CompoundNoun_Parse(dc);
  return(1);
}

int Tool_Shell_ObjHref(Obj *obj, FILE *in, FILE *out, FILE *err, Discourse *dc)
{
  char	buf[PHRASELEN];
  if (obj == NULL) return(0);
  HTML_StringLinkToObj(obj, 0, 1, 0, buf);
  fprintf(out, "%s\n", buf);
  return(1);
}

/* exhaustive ifdefs */
#ifdef GCC
#include <sys/wait.h>
#endif
#ifdef SOLARIS
#include <sys/wait.h>
#endif
#ifdef SUNOS
#include <sys/wait.h>
#endif
#ifdef MACOS
/* Nothing */
#endif
#ifdef NEXT
/* Nothing */
#endif

/* Returns -1 if <cmd> not run. Otherwise, returns
 * exit code (0 == success).
 */
int System(char *cmd)
{
  int	retcode;
  retcode = system(cmd);
/* exhaustive ifdefs */
#ifdef GCC
  if (WIFEXITED(retcode)) {
    return(WEXITSTATUS(retcode));
  }
#endif
#ifdef SOLARIS
  if (WIFEXITED(retcode)) {
    return(WEXITSTATUS(retcode));
  }
#endif
#ifdef SUNOS
  if (WIFEXITED(retcode)) {
    return(WEXITSTATUS(retcode));
  }
#endif
#ifdef MACOS
  if (!((retcode) & 0377)) {
    return((int)(!((retcode) & 0377) ? (((retcode) >> 8) & 0377) : -1));
  }
#endif
#ifdef NEXT
  if (!((retcode) & 0377)) {
    return((int)(!((retcode) & 0377) ? (((retcode) >> 8) & 0377) : -1));
  }
#endif
  Dbg(DBGGEN, DBGBAD, "system <%s>", cmd);
  return(-1);
}

int Tool_Shell_Validate(char *new, char *old, char *line, FILE *in,
                        FILE *out, FILE *err, Discourse *dc)
{
  char	cmd[LINELEN];
  sprintf(cmd, "diff %s %s\n", old, new);
  if (0 != System(cmd)) {
    fprintf(out, "validation failure %s %s\n", new, old);
  } else {
    Dbg(DBGGEN, DBGDETAIL, "validation successful <%s> <%s>", new, old);
  }
  return(1);
}

int Tool_Shell_TT(char *file, char *line, FILE *in, FILE *out, FILE *err,
                  Discourse *dc)
{
  FILE	*newin;
  if (NULL == (newin = StreamOpen(file, "r"))) {
    return(Tool_Shell_Error("file not found", "tt", file, line, in, out,
                            err, dc));
  }
  Tool_Shell(newin, out, err, dc);
  StreamClose(newin);
  return(1);
}

int Tool_Shell_Dbg(int flags, int level, char *line, FILE *in, FILE *out,
                   FILE *err, Discourse *dc)
{
  DbgSet(flags, level);
  return(1);
}

int Tool_Shell_CorpusLoad(int lang, char *file, char *dir, char *line,
                          FILE *in, FILE *out, FILE *err, Discourse *dc)
{
  CorpusInit();
  if (file[0]) CorpusIndexFile(LangToCorpus(lang), file, NULL);
  if (dir[0]) CorpusIndexDirectory(LangToCorpus(lang), dir);
  return(1);
}

int Tool_Shell_CorpusFind(int lang, char *word, char *line, FILE *in, FILE *out,
                          FILE *err, Discourse *dc)
{
  CorpusInit();
  CorpusPrintWord(out, LangToCorpus(lang), word, 79);
  /* For debugging. */
  if (lang == F_FRENCH) {
    CorpusFrenchGender(LangToCorpus(lang), word);
  }
  return(1);
}

/* End of file. */
