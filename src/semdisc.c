/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19941017: redoing pronouns
 * 19951027: moved anaphora routines to Sem_Anaphora
 * 19981022T123545: mods for String channels
 */

#include "tt.h"
#include "apptrans.h"
#include "lexentry.h"
#include "repbasic.h"
#include "repchan.h"
#include "repcxt.h"
#include "repdb.h"
#include "repgrid.h"
#include "repgroup.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repspace.h"
#include "repstr.h"
#include "reptext.h"
#include "reptime.h"
#include "semdisc.h"
#include "semanaph.h"
#include "semgen1.h"
#include "semgen2.h"
#include "semparse.h"
#include "synpnode.h"
#include "synxbar.h"
#include "ta.h"
#include "ua.h"
#include "uaemot.h"
#include "utildbg.h"
#include "utillrn.h"

/* TsrGenAdvice */

void TsrGenAdviceClear(TsrGenAdvice *tga)
{
  tga->attempt_relative = 0;
  tga->attempt_day_and_part_of_the_day = 0;
  tga->include_time_of_day = 0;
  tga->include_day_of_the_week = 0;
  tga->include_day = 0;
  tga->include_month = 0;
  tga->include_year = 0;
  tga->include_dur = 0;
  tga->prep_ok = 1; /* This is correct. */
  tga->exactly = 0;
}

void TsrGenAdviceSet(TsrGenAdvice *tga)
{
  tga->attempt_relative = 1;
  tga->attempt_day_and_part_of_the_day = 1;
  tga->include_time_of_day = 1;
  tga->include_day_of_the_week = 1;
  tga->include_day = 1;
  tga->include_month = 1;
  tga->include_year = 1;
  tga->include_dur = 0;
  tga->prep_ok = 1;
}

/* todoDATABASE */
void TsrGenAdviceGet(Obj *question, Discourse *dc,
                     /* RESULTS */ TsrGenAdvice *tga, Obj **current,
                     int *gen_tsr)
{
  Obj	*speaker, *listener;
  TsrGenAdviceClear(tga);
  if (dc->exactly) tga->exactly = 1;
  if (gen_tsr) *gen_tsr = 0;
  speaker = DiscourseSpeaker(dc);
  listener = DiscourseListener(dc);
  if (ISA(N("current-time-request"), question) ||
      ISA(N("time-of-day-interrogative-adverb"), question)) {
    tga->include_time_of_day = 1;
    if (current) *current = L(N("current-time-sentence"), listener, speaker, E);
    if (gen_tsr) *gen_tsr = 1;
    if (ISA(N("current-time-request"), question)) {
      tga->prep_ok = 0;
    } else {
      tga->prep_ok = 1;
    }
  } else if (ISA(N("current-date-request"), question) ||
             ISA(N("date-interrogative-adverb"), question)) {
    tga->include_day_of_the_week = 1;
    tga->include_day = 1;
    tga->include_month = 1;
    tga->include_year = 1;
    if (current) *current = L(N("current-date-sentence"), listener, speaker, E);
    if (gen_tsr) *gen_tsr = 1;
    tga->prep_ok = 0;
  } else if (ISA(N("current-date-without-day-request"), question)) {
    tga->include_day = 1;
    tga->include_month = 1;
    tga->include_year = 1;
    if (current) *current = L(N("current-date-sentence"), listener, speaker, E);
    if (gen_tsr) *gen_tsr = 1;
    tga->prep_ok = 0;
  } else if (ISA(N("current-month-request"), question) ||
             ISA(N("month-interrogative-adverb"), question)) {
    tga->include_month = 1;
    if (current) *current = L(N("current-time-sentence"), listener, speaker, E);
    if (gen_tsr) *gen_tsr = 1;
    tga->prep_ok = 0;
  } else if (ISA(N("current-year-request"), question) ||
             ISA(N("current-decade-request"), question) ||
             ISA(N("current-century-request"), question) ||
             ISA(N("year-interrogative-adverb"), question) ||
             ISA(N("decade-interrogative-adverb"), question) ||
             ISA(N("century-interrogative-adverb"), question)) {
    tga->include_year = 1;
    if (current) *current = L(N("current-time-sentence"), listener, speaker, E);
    if (gen_tsr) *gen_tsr = 1;
    tga->prep_ok = 0;
  } else if (ISA(N("absolute-time-interrogative-adverb"), question)) {
    tga->include_time_of_day = 1;
    tga->include_day_of_the_week = 1;
    tga->include_day = 1;
    tga->include_month = 1;
    tga->include_year = 1;
    if (current) {
      *current = L(N("current-time-sentence"), listener, speaker, E);
      tga->prep_ok = 0;
    } else {
      tga->attempt_relative = 1;
      tga->attempt_day_and_part_of_the_day = 1;
      tga->prep_ok = 1;
    }
    if (gen_tsr) *gen_tsr = 1;
  }
}

/* CompTenseHolder */

void CompTenseHolderInit(CompTenseHolder *cth)
{
  cth->mainverb_r = NULL;
  cth->tense_r = NULL;
  cth->mood_r = NULL;
  cth->mainverb = NULL;
  cth->maintense_a = NULL;
  cth->leftmost = 1;
  cth->rightmost = 1;
}

/* GenAdvice */

void GenAdviceInit(GenAdvice *ga)
{
  ga->aspect = NULL;
  ga->consistent = 1; /* Temporary. For nicer generation. */
  ga->gen_tsr = 0;
  TsrGenAdviceSet(&ga->tga);
  ga->gen_advle = NULL;
  ga->gen_negle1 = NULL;
  ga->gen_negle2 = NULL;
  ga->qwq = 0;
  ga->anaphors = NULL;
  ga->flip_lang_ok = 1;
}

/* Discourse */

Discourse *DiscourseCreate(char *langs, char *dialects)
{
  int		i;
  Discourse *dc;
  dc = CREATE(Discourse);
  dc->curchannel = 0;
  for (i = 0; i < DCMAX; i++) ChannelInit(&dc->channel[i]);
  dc->abt = NULL;
  dc->cx_alterns = NULL;
  dc->cx_best = NULL;
  dc->cx_cur = NULL;
  dc->cx_children = NULL;
  dc->mode = DC_MODE_PROGRAMMER|DC_MODE_CONV;
  dc->last_answers = NULL;
  dc->run_agencies = AGENCY_ALL;
  DiscourseDeicticStackClear(dc);
  dc->relax = 0;
  dc->defer_ok = 0;
  dc->tense = NULL;
  dc->pn_root = NULL;
  dc->rel_dur = DURNA;
  dc->abs_dur = 1L;
  dc->task = NULL;
  GenAdviceInit(&dc->ga);
  StringCpy(dc->langs, langs, FEATLEN);
  StringCpy(dc->dialects, dialects, FEATLEN);
  CompTenseHolderInit(&dc->cth);
  dc->exactly = 0;
  return(dc);
}

void DiscourseFree(Discourse *dc)
{
  DiscourseCloseChannels(dc);
  DiscourseDeicticStackClear(dc);
  MemFree(dc, "Discourse");
}

void DiscourseContextInit(Discourse *dc, Ts *ts)
{
  /* todoFREE: Free old. */
  dc->cx_alterns = NULL;
  dc->cx_best = dc->cx_alterns = ContextCreate(ts, 0, dc, dc->cx_alterns);
  dc->cx_cur = NULL;
  dc->cx_children = NULL;
}

int DiscourseOpenChannel(Discourse *dc, int channeli, char *fn, int linelen,
                         char *mode, int output_reps, int lang, int dialect,
                         int style, int paraphrase_input, int echo_input,
                         int batch_ok, char *s)
{
  FILE		*stream;
  if (channeli < 0 || channeli >= DCMAX) {
    Dbg(DBGGEN, DBGBAD, "increase DCMAX");
    return(0);
  }
  if (streq("STDIN", fn)) {
    stream = stdin;
/*
  todo: Only do this if CH_REALTIME_CHAR_BASED.
#ifdef MACOS
    csetmode(C_RAW, stream);
#else
    fcntl(0, F_SETFL, O_NONBLOCK);
#endif
 */
  } else if (streq("STDOUT", fn)) {
    stream = stdout;
  } else if (streq("STRING", fn)) {
    stream = NULL;
  } else if (NULL == (stream = StreamOpen(fn, mode))) {
    return(0);
  }
  ChannelOpen(&dc->channel[channeli], stream, fn, linelen, mode, output_reps,
              lang, dialect, style, paraphrase_input, echo_input, batch_ok,
              s);
  return(1);
}

void DiscourseSetCurrentChannel(Discourse *dc, int channeli)
{
  if (channeli < 0 || channeli >= DCMAX) {
    Dbg(DBGGEN, DBGBAD, "increase DCMAX");
    return;
  }
  dc->curchannel = channeli;
}

Channel *DiscourseGetInputChannel(Discourse *dc)
{
  return(&dc->channel[DCIN]);
}

Dur DiscourseInputIdleTime(Discourse *dc)
{
  Channel	*ch;
  if ((ch = DiscourseGetInputChannel(dc))) {
    return(ChannelIdleTime(ch));
  }
  return(DURPOSINF);
}

Bool DiscourseWasActivity(Discourse *dc)
{
  Channel	*ch;
  if ((ch = DiscourseGetInputChannel(dc))) {
    return(ch->activity);
  }
  return(0);
}

void DiscourseClearActivity(Discourse *dc)
{
  Channel	*ch;
  if ((ch = DiscourseGetInputChannel(dc))) {
    ch->activity = 0;
  }
}

Channel *DiscourseGetIthChannel(Discourse *dc, int channeli)
{
  Channel	*ch;
  if (channeli < 0 || channeli >= DCMAX) {
    Dbg(DBGGEN, DBGBAD, "DiscourseGetIthChannel: no Channel %d", channeli);
    return(NULL);
  }
  ch = &dc->channel[channeli];
  if (ch->stream == NULL) return(NULL);
  return(ch);
}

void DiscourseCloseChannel(Discourse *dc, int channeli)
{
  Channel	*ch;
  if ((ch = DiscourseGetIthChannel(dc, channeli))) {
    ChannelClose(ch);
  }
}

void DiscourseCloseChannels(Discourse *dc)
{
  int	i;
  for (i = 0; i < DCMAX; i++) {
    DiscourseCloseChannel(dc, i);
  }
}

void DiscoursePrintCurrentChannel(FILE *Log, Discourse *dc)
{
  fprintf(Log, "current discourse channel %d <%s> <%s> <%c%c%c>",
          dc->curchannel, DC(dc).fn, DC(dc).mode,
          DC(dc).lang, DC(dc).dialect, DC(dc).style);
}

/* todo: Return 0 if connection terminated. */
Bool DiscourseReadUnderstand(Discourse *dc)
{
  Channel		*ch;
  Dur			idle;
  DiscourseSetCurrentChannel(dc, DCIN);
  ch = &DC(dc);
  if (ch->realtime == CH_REALTIME_CHAR_BASED) {
    /* Get some more characters from the terminal. */
    ChannelReadRealtime(ch);
    if (0 == strncmp((char *)ch->buf, "quit", 4L)) {	/* todo */
      ChannelBufferClear(ch);
      return(0);
    }
    /* If less than 4 seconds idle or no end of sentence in buffer,
     * do nothing.
     */
    idle = ChannelIdleTime(ch);
    if (idle < 4L) return(1);
    if (idle < 15L &&
        (!StringAnyIn(".?!", (char *)ch->buf))) {
      return(1);
    }
  } else if (ch->realtime == CH_REALTIME_LINE_BASED) {
    ChannelReadPrompt(ch);
    if (0 == strncmp((char *)ch->buf, "quit", 4L)) {
      ChannelBufferClear(ch);
      return(0);
    }
  }

  if (ch->len <= 0L) {
    if (ch->realtime) return(1);
    else return(0);
  }

  TA_Scan(ch, dc);
  if (dc->run_agencies & AGENCY_SYNTAX) {
    DiscourseSynParse(ch, dc);
  }
  ChannelBufferClear(ch);
  return(1);
}

void DiscourseSynParse(Channel *ch, Discourse *dc)
{
  int		eoschar;
  size_t	pos;
  PNode		*pn;

  pos = 0;
  pn = ch->pnf->first;
  while (pos < ch->len) {
    /* Process next sentence. */
    if (NULL == (pn = PNodeGetNext(pn, pos))) break;
    if (pn->type == PNTYPE_END_OF_SENT) {
      eoschar = ch->buf[pn->lowerb];
      DiscourseNextSentence(dc);
      UnderstandSentence(ch, pos, pn->lowerb, eoschar, dc);
      DiscourseEndOfSentence(dc);
    } else {
      if (DbgOn(DBGGEN, DBGDETAIL)) {
        Dbg(DBGGEN, DBGDETAIL, "PARSE BLOCK:");
        PNodePrintSubstring(Log, ch, pos, pn->upperb, 1, LINELEN);
        fputc(NEWLINE, Log);
      }
      Sem_ParseParse(pn, dc);
      TranslateSpitUntranslated(ch, pos, pn->upperb);
      /* todo: For translation, we could also paraphrase blocks.
       * "According to" for attributions, and so on.
       */
    }
    pos = pn->upperb+1;
  }
}

void DiscourseInit(Discourse *dc, Ts *ts, Obj *class, Obj *speaker,
                   Obj *listener, int realtime)
{
  Ts		ts1;
  ObjList	*speakers, *listeners;
  if (ts == NULL) {
    TsSetNow(&ts1);
    ts = &ts1;
  }
  DiscourseContextInit(dc, ts);
  if (speaker) {
    speakers = ObjListCreate(speaker, NULL);
  } else {
    speakers = NULL;
  }
  if (listener) {
    listeners = ObjListCreate(listener, NULL);
  } else {
    listeners = NULL;
  }
  DiscourseDeicticStackClear(dc);
  DiscourseDeicticStackPush(dc, class, speakers, listeners, ts, realtime);
}

/* flow of control
 * ===============
 * DiscourseParse =>
 * DiscourseReadUnderstand =>
 * TA_Scan(channel), DiscourseSynParse(channel) =>
 * UnderstandSentence(channel) =>
 * UnderstandSentence1(channel) =>
 * UnderstandSentence2(channel) =>
 *   [Syn_ParseParse(channel) => 
 *    Syn_ParseSem_Parse(pn) =>
 *    Sem_ParseParse(pn)=>concepts],
 *   Understand(concepts) =>
 *   UnderstandlAlternatives(concepts) =>
 *   Sem_Anaphora(concept) =>
 *   UnderstandlAlternative(concept) => ...UAs...
 */
void DiscourseParse(int translate, Obj *speaker, Obj *listener)
{
  TranslationOn = translate;
  DiscourseInit(StdDiscourse, NULL, N("computer-file"), speaker, listener, 0);
  while (DiscourseReadUnderstand(StdDiscourse));
  DiscourseDeicticStackPop(StdDiscourse);
}

void DiscourseSetLang(Discourse *dc, int lang)
{
  DC(dc).lang = lang;
  DC(dc).ht = LexEntryLangHt1(lang);
}

void DiscourseNextSentence(Discourse *dc)
{
  /* For future use. */
  return;
}

void DiscourseEndOfSentence(Discourse *dc)
{
  Float     numer, denom, avg_emot;
  Obj       *polity, *grid;
  ObjList   *p;
  Ts        story_time;
  GridCoord row, col;
  Grid      *gridstate;
  Context   *cx;
  Actor     *ac;
  Subgoal   *sg;

  if (NULL == (cx = dc->cx_best)) return;

  fprintf(Display, "%c\n", FF);

  /* Time */
  story_time = cx->story_time.stopts;
  fputs("Ts = ", Display);
  TsPrint(Display, &story_time);
  fputc(NEWLINE, Display);

  /* Grid */
  gridstate = NULL;
  for (ac = cx->actors; ac; ac = ac->next) {
    if (!ISA(N("animal"), ac->actor)) continue;
    if (ac->actor == Me) continue;
    Dbg(DBGGEN, DBGDETAIL, "next actor:");
    ObjPrint(Log, ac->actor);
    fputc(NEWLINE, Log);
    if (SpaceLocateObject(&story_time, NULL,
                          ac->actor, NULL, 0, &polity, &grid,
			  &row, &col)) {
      Dbg(DBGGEN, DBGDETAIL, "found actor in grid:");
      ObjPrint(Log, grid);
      fputc(NEWLINE, Log);
      if (gridstate == NULL) {
        gridstate = GridBuild(&story_time, ObjToGrid(grid),
                              1, /* whether to use full name */
                              TERM, N("walkable"));
        GridPrint(Display, "gridstate", gridstate, 1);
        break;
      }
    }
  }

  /* Actor emotions and active subgoals */
  for (ac = cx->actors; ac; ac = ac->next) {
    if (!ISA(N("animal"), ac->actor)) continue;
    if (ac->actor == Me) continue;
    fprintf(Display, "%s ", M(ac->actor));

    /* Emotions */
    FloatAvgInit(&numer, &denom);
    for (p = ac->emotions; p; p = p->next) {
      if (ISA(N("motivation"), I(p->obj, 0))) continue;
      FloatAvgAdd(UA_EmotionSignedWeight(p->obj), &numer, &denom);
    }
    avg_emot = FloatAvgResult(numer, denom, 1.0);
    if (avg_emot > 0.001) {
      fputs(":-) ", Display);
    } else if (avg_emot < -0.001) {
      fputs(":-( ", Display);
    } else {
      fputs(":-| ", Display);
    }
    fprintf(Display, "%g ", avg_emot);

/* specific emotions
    for (p = ac->emotions; p; p = p->next) {
      if (ISA(N("motivation"), I(p->obj, 0))) continue;
      fprintf(Display, "%s %g ", M(I(p->obj, 0)),
              UA_EmotionSignedWeight(p->obj));
    }
 */

    /* Rest and energy levels */
    fprintf(Display, "rest %g ", ObjWeight(ac->rest_level));
/*
    fprintf(Display, "energy %g ", ObjWeight(ac->energy_level));
 */

    /* Subgoals */
    for (sg = ac->subgoals; sg; sg = sg->next) {
      if (ISA(N("handle-proposal"), I(sg->obj, 0))) continue;
      if (ISA(N("active-goal"), I(sg->cur_goal, 0))) {
        fprintf(Display, "%s %d ", M(I(sg->obj, 0)), sg->state);
      }
    }
    fputc(NEWLINE, Display);
  }

  fflush(Display);
}

char *AgencyTypeToString(int rep_type)
{
  switch (rep_type) {
    case AGENCY_SYNTAX:
      return("SYNTACTIC PARSE");
    case AGENCY_SEMANTICS:
      return("SEMANTIC PARSE");
    case AGENCY_ANAPHORA:
      return("ANAPHORIC PARSE");
    case AGENCY_UNDERSTANDING:
      return("UNDERSTANDING");
    default:
      return("??");
  }
}

void DiscourseObjRep(Discourse *dc, int rep_type, Obj *obj, int eoschar)
{
  int		i;
  Channel	*ch;
  if (rep_type == AGENCY_UNDERSTANDING) {
    DiscourseFlipListenerSpeaker(dc);
    DiscourseGen(dc, 1, ") ", obj, eoschar);
    DiscourseFlipListenerSpeaker(dc);
  }
  for (i = 0; i < DCMAX; i++) {
    if (NULL == (ch = DiscourseGetIthChannel(dc, i))) continue;
    if (ch->output_reps & rep_type) {
      DiscourseEchoInputChannel(dc, ch, i);
      fprintf(ch->stream, "%s CONCEPT:\n", AgencyTypeToString(rep_type));
      ObjPrettyPrint1(ch->stream, obj, NULL, 0, 0, 1, 0, 0, 0);
      fputc(NEWLINE, ch->stream);
      fflush(ch->stream);
    }
  }
}

void DiscourseObjListRep(Discourse *dc, int rep_type, ObjList *objs)
{
  int		i;
  Channel	*ch;
  for (i = 0; i < DCMAX; i++) {
    if (NULL == (ch = DiscourseGetIthChannel(dc, i))) continue;
    if (ch->output_reps & rep_type) {
      DiscourseEchoInputChannel(dc, ch, i);
      fprintf(ch->stream, "%s CONCEPTS:\n", AgencyTypeToString(rep_type));
      ObjListPrettyPrint1(ch->stream, objs, 1, 0, 0, 0);
      fflush(ch->stream);
    }
  }
}

void DiscoursePNodeRep(Discourse *dc, int rep_type, PNode *pn)
{
  int		i;
  Channel	*ch;
  for (i = 0; i < DCMAX; i++) {
    if (NULL == (ch = DiscourseGetIthChannel(dc, i))) continue;
    if (ch->output_reps & rep_type) {
      DiscourseEchoInputChannel(dc, ch, i);
      fprintf(ch->stream, "%s TREE:\n", AgencyTypeToString(rep_type));
      PNodePrettyPrint(ch->stream, NULL, pn);
      fflush(ch->stream);
    }
  }
}

Channel *DiscourseSetCurrentChannelWrite(Discourse *dc, int i)
{
  Channel *ch;
  if (NULL == (ch = DiscourseGetIthChannel(dc, i))) return(NULL);
  if (ch->mode[0] == 'r') return(NULL);
  DiscourseSetCurrentChannel(dc, i);
  return(ch);
}

int DiscourseCurrentChannelLineLen(Discourse *dc)
{
  Channel *ch;
  if (NULL == (ch = DiscourseGetIthChannel(dc, dc->curchannel))) {
    return(DSPLINELEN);
  }
  return(ch->linelen);
}

Channel *DiscourseSetCurrentChannelRW(Discourse *dc, int i)
{
  Channel	*ch;
  if (NULL == (ch = DiscourseGetIthChannel(dc, i))) return(NULL);
  DiscourseSetCurrentChannel(dc, i);
  return(ch);
}

Channel *DiscourseSetCurrentChannelSameLangAsInput(Discourse *dc, int i)
{
  int		input_lang;
  Channel	*ch;
  if ((ch = DiscourseGetInputChannel(dc))) input_lang = ch->lang;
  else input_lang = F_NULL;
  if (NULL == (ch = DiscourseGetIthChannel(dc, i))) return(NULL);
  if (ch->lang != input_lang) return(NULL);
  DiscourseSetCurrentChannel(dc, i);
  return(ch);
}

/* is_input: <obj> is parsed input concept
 * todo: buffer multiple objects
 */
void DiscourseGen(Discourse *dc, Bool is_input, char *preface, Obj *obj,
                  int eos)
{
  int		i, save_channel;
  Channel	*ch, *inch;
  save_channel = DiscourseGetCurrentChannel(dc);
  /* todo: Output text needs to be buffered. */
  for (i = 0; i < DCMAX; i++) {
    if (NULL == (ch = DiscourseSetCurrentChannelWrite(dc, i))) continue;
    if (is_input && (!ch->paraphrase_input)) continue;
    if (DC(dc).stream == stdout) {
      if ((!ISA(N("intervention"), I(obj, 0))) ||
          (Me == I(obj, 2))) {
        continue;
      }
    }
    StreamGen(DC(dc).stream, preface, obj, eos, dc);
    if ((DC(dc).stream == stdout) &&
        (inch = DiscourseGetInputChannel(dc))) {
      ChannelResetActivityTime(inch);
    }
  }
  DiscourseSetCurrentChannel(dc, save_channel);
}

void DiscourseEchoInput2(Discourse *dc, Channel *ch, int i,
                         char *s, int is_sent)
{
  Text		*text;

  if (!ch) return;
  if (ch->mode[0] == 'r') return;
  if (!ch->echo_input) return;
  if (ch->echo_input_last && streq(ch->echo_input_last, s)) return;

  ch->echo_input_last = s;
  DiscourseSetCurrentChannel(dc, i);
  text = TextCreate(128, DiscourseCurrentChannelLineLen(dc), dc);
  if (is_sent) {
    TextPrintPhrase(text, TERM, s);
  } else {
    TextPuts(s, text);
  }
  if (TextLen(text) > 0) {
    TextPrintWithPreface(DCI(dc, i).stream, "> ", text);
    fputc(NEWLINE, DCI(dc, i).stream);
  }
  TextFree(text);
}

void DiscourseEchoInput1(Discourse *dc, char *s, int is_sent)
{
  int		i, save_channel;
  Dbg(DBGGEN, DBGDETAIL, "INPUT TEXT <%s>", s);
  save_channel = DiscourseGetCurrentChannel(dc);
  for (i = 0; i < DCMAX; i++) {
    DiscourseEchoInput2(dc, DiscourseGetIthChannel(dc, i), i, s, is_sent);
  }
  DiscourseSetCurrentChannel(dc, save_channel);
}

void DiscourseEchoInput(Discourse *dc)
{
  Channel	*ch;
  if ((ch = DiscourseGetInputChannel(dc)) &&
      ch->input_text) {
    DiscourseEchoInput1(dc, ch->input_text, 1);
  }
}

void DiscourseEchoInputChannel(Discourse *dc, Channel *ch, int i)
{
  Channel	*inch;
  if ((inch = DiscourseGetInputChannel(dc)) &&
      inch->input_text) {
    DiscourseEchoInput2(dc, ch, i, inch->input_text, 1);
  }
}

void DiscourseGenParagraph(Discourse *dc, char *preface, ObjList *in_objs,
                           int sort_by_ts, int gen_pairs)
{
  int		i, save_channel;
  Channel	*ch;
  save_channel = DiscourseGetCurrentChannel(dc);
  for (i = 0; i < DCMAX; i++) {
    if (NULL == (ch = DiscourseSetCurrentChannelWrite(dc, i))) continue;
    StreamGenParagraph(DC(dc).stream, preface, in_objs, sort_by_ts, gen_pairs,
                       dc);
  }
  DiscourseSetCurrentChannel(dc, save_channel);
}

void DiscoursePrintDeicticStack(FILE *stream, Discourse *dc)
{
  if (dc->ds_cur < 0) {
    fprintf(stream, "Deictic stack empty\n");
    return;
  }
  fprintf(stream, "Deictic stack level %d <%s> ", dc->ds_cur, M(DCCLASS(dc)));
  TsPrint(stream, DCNOW(dc));
  if (DCREALTIME(dc)) fputs(" realtime", stream);
  fputc(NEWLINE, stream);
  fprintf(stream, "speakers: ");
  ObjListPrint(stream, DCSPEAKERS(dc));
  fprintf(stream, "listeners: ");
  ObjListPrint(stream, DCLISTENERS(dc));
}

Bool DiscourseDeicticStackLevelEmpty(Discourse *dc, int stack_level)
{
  if (stack_level >= DEICTICSTACKMAX || stack_level < -1) return(1);
  if (dc->ds_speakers[stack_level] == NULL &&
      dc->ds_listeners[stack_level]) {
    return(1);
  }
  return(0);
}

void DiscourseDeicticStackClear(Discourse *dc)
{
  int	i;
  dc->ds_cur = -1;
  for (i = 0; i < DCMAX; i++) {
    dc->ds_class[i] = NULL;
    dc->ds_speakers[i] = NULL; /* todoFREE */
    dc->ds_listeners[i] = NULL; /* todoFREE */
    TsSetNa(&dc->ds_now[i]);
    dc->ds_realtime[i] = 0;
  }
}

void DiscourseDeicticStackPush(Discourse *dc, Obj *class, ObjList *speakers,
                               ObjList *listeners, Ts *now, int realtime)
{
  if (dc->ds_cur >= DEICTICSTACKMAX) {
    Dbg(DBGGEN, DBGBAD, "DiscourseDeicticStackPush: increase DEICTICSTACKMAX");
  } else {
    dc->ds_cur++;
  }
  dc->ds_class[dc->ds_cur] = class;
  dc->ds_speakers[dc->ds_cur] = speakers;
  dc->ds_listeners[dc->ds_cur] = listeners;
  dc->ds_now[dc->ds_cur] = *now;
  dc->ds_realtime[dc->ds_cur] = realtime;
  if (DbgOn(DBGGEN, DBGDETAIL)) DiscoursePrintDeicticStack(Log, dc);
}

void DiscourseDeicticStackSpeakersSet(Discourse *dc, ObjList *speakers)
{
  dc->ds_speakers[dc->ds_cur] = speakers;
}

void DiscourseDeicticStackPop(Discourse *dc)
{
  if (dc->ds_cur < -1) {
    Dbg(DBGGEN, DBGBAD, "DiscourseDeicticStackPop: overpopping");
    return;
  }
  /* We do not clear the stack because we may want to unpop (in
   * quotation parsing).
   */
  dc->ds_cur--;
  if (DbgOn(DBGGEN, DBGDETAIL)) DiscoursePrintDeicticStack(Log, dc);
}

void DiscourseSetDeicticStackLevel(Discourse *dc, int stack_level)
{
  if (dc->ds_cur == stack_level) return;
  if (stack_level >= DEICTICSTACKMAX) {
    Dbg(DBGGEN, DBGBAD, "DiscourseSetDeicticStackLevel: %d too big",
        stack_level);
  } else if (stack_level < -1) {
    Dbg(DBGGEN, DBGBAD, "DiscourseSetDeicticStackLevel: %d too little",
        stack_level);
  }
  if (DbgOn(DBGGEN, DBGDETAIL)) DiscoursePrintDeicticStack(Log, dc);
}

void DiscourseFlipListenerSpeaker(Discourse *dc)
{
  ObjList	*tmp;
  if (dc->ds_cur < 0) {
    return;
  }
  tmp = DCLISTENERS(dc);
  DCLISTENERS(dc) = DCSPEAKERS(dc);
  DCSPEAKERS(dc) = tmp;
/*
  if (DbgOn(DBGGEN, DBGDETAIL)) DiscoursePrintDeicticStack(Log, dc);
*/
}

/* Used by Syn_Gen. */
void DiscourseAnaphorInit(Discourse *dc)
{
  dc->ga.anaphors = NULL;
}

/* Used by Syn_Gen to queue up Anaphors to be later added as Antecedents.
 * We cannot add them now because we do not have the top-level PNode tree.
 */
void DiscourseAnaphorAdd(Discourse *dc, Obj *con, Obj *source_con, PNode *pn,
                         int gender, int number, int person)
{
  dc->ga.anaphors = AnaphorCreate(con, source_con, pn, SALIENCE_MINIMAL,
                                  dc->ga.anaphors);
  dc->ga.anaphors->gender = gender;
  dc->ga.anaphors->number = number;
  dc->ga.anaphors->person = person;
}

/* Used by Syn_Gen to commit Anaphors previously queued in one Channel
 * (<dc->curchannel>).
 */
void DiscourseAnaphorCommit(Discourse *dc, PNode *pn_top)
{
  Context	*cx;
  Anaphor	*an;

  for (cx = dc->cx_alterns; cx; cx = cx->next) {
    ContextAntecedentDecayOneCh(cx);
    for (an = dc->ga.anaphors; an; an = an->next) {
      ContextAntecedentRefreshOneCh(cx, an->con, an->pn, pn_top, an->gender,
                                    an->number, an->person);
    }
  }
  AnaphorFreeAll(dc->ga.anaphors);
  dc->ga.anaphors = NULL;
}

int DiscourseAddress(Discourse *dc)
{
  if (DC(dc).lang == F_ENGLISH) return(F_VOUVOIEMENT);
  if (DCLISTENERS(dc) && DCLISTENERS(dc)->next) {
    /* todo: "salut" can be used to a group of tutoyerable people. */
    return(F_VOUVOIEMENT);
  } else if (DCLISTENERS(dc) &&
             DCSPEAKERS(dc) &&
             RE(DCNOW(dc),
                L(N("tutoyer"), DCSPEAKERS(dc)->obj,
                                DCLISTENERS(dc)->obj, E))) {
    return(F_TUTOIEMENT);
  } else return(F_VOUVOIEMENT);
}

Obj *DiscourseSpeaker(Discourse *dc)
{
  if (DCSPEAKERS(dc)) return(DCSPEAKERS(dc)->obj);
  else return(ObjWild);
}

/* todo: Need to generate groups. */
Obj *DiscourseListener(Discourse *dc)
{
  if (DCLISTENERS(dc)) return(DCLISTENERS(dc)->obj);
  else return(ObjWild);
}

Bool DiscourseSpeakerIs(Discourse *dc, Obj *obj)
{
  return(ObjListIsLengthOne(DCSPEAKERS(dc)) && obj == DCSPEAKERS(dc)->obj);
}

Bool DiscourseIsASpeaker(Discourse *dc, Obj *obj)
{
  return(ObjListIn(obj, DCSPEAKERS(dc)));
}

Bool DiscourseListenerIs(Discourse *dc, Obj *obj)
{
  return(ObjListIsLengthOne(DCLISTENERS(dc)) && obj == DCLISTENERS(dc)->obj);
}

Bool DiscourseIsAListener(Discourse *dc, Obj *obj)
{
  return(ObjListIn(obj, DCLISTENERS(dc)));
}

Bool DiscourseSerialComma(Discourse *dc)
{
  return(DC(dc).lang == F_ENGLISH);
}

Obj *DiscourseIsaArticle(Discourse *dc)
{
  return(DC(dc).lang == F_FRENCH ? N("definite-article") :
         N("indefinite-article"));
}

int DiscourseGenderOf(Obj *obj, Discourse *dc)
{
  if (N("female") == DbGetEnumValue(DCNOW(dc), NULL, N("sex"), obj, NULL)) {
    return(F_FEMININE);
  } else {
    return(F_MASCULINE);
  }
}

/* End of file. */
