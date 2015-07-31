/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19941216: begun
 * 19941217: more work
 * 19950202: doing many-to-one learning in more detail (Sony example)
 * 19950206: names
 * 19950221: learning humans, names, media objects, input_text, cleanup
 * 19950223: modified for multiple learn files
 *
 * Learning is twofold:
 * (1) DUMPing to outlrn.txt file.
 * (2) UPDATing incore database (creating objects, modifying assertions,
 *     asserting assertions)
 *
 * todo:
 * - Learning should only occur for the best interpretation. For example,
 *   TAParseName should only learn if the parse node it generates is later
 *   contained in the best parse.
 */

#include "tt.h"
#include "lexentry.h"
#include "lexobjle.h"
#include "repbasic.h"
#include "repchan.h"
#include "repcxt.h"
#include "repdb.h"
#include "repmisc.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "reptime.h"
#include "semanaph.h"
#include "semdisc.h"
#include "synparse.h"
#include "synpnode.h"
#include "taname.h"
#include "uaquest.h"
#include "utilbb.h"
#include "utildbg.h"
#include "utillrn.h"

FILE	*StreamSuggEnglishInfl, *StreamSuggFrenchInfl;

/* LearnFile */

LearnFile *LearnFiles;

void LearnFileAdd(char *fn, Obj *class)
{
  LearnFile	*lf;
  lf = CREATE(LearnFile);
  lf->class = class;
  if (NULL == (lf->stream = StreamOpen(fn, "w+"))) {
    lf->stream = stdout;
  }
  lf->last_parent = NULL;
  lf->last_obj = NULL;
  TsRangeSetNa(&lf->last_tsr);
  lf->close_assertion = 0;
  lf->last_input_text1[0] = TERM;
  lf->last_input_text2[0] = TERM;
  lf->next = LearnFiles;
  LearnFiles = lf;
}

LearnFile *LearnFileFindCatchall()
{
  LearnFile	*lf;
  for (lf = LearnFiles; lf; lf = lf->next) {
    if (lf->class == NULL) return(lf);
  }
  if (LearnFiles) return(LearnFiles);
  Dbg(DBGSEMPAR, DBGBAD, "LearnFileFindCatchall: 1");
  return(NULL);
}

LearnFile *LearnFileFindClass(Obj *class)
{
  LearnFile	*lf;
  for (lf = LearnFiles; lf; lf = lf->next) {
    if (lf->class && ISA(lf->class, class)) return(lf);
  }
  return(LearnFileFindCatchall());
}

LearnFile *LearnFileFindObj(Obj *obj)
{
  LearnFile	*lf;
  for (lf = LearnFiles; lf; lf = lf->next) {
    if (lf->last_obj == obj) return(lf);
  }
  return(NULL);
}

LearnFile *LearnFileFindAssertion(Obj *assertion)
{
  int	i, len;
  LearnFile	*lf;
  for (i = 1, len = ObjLen(assertion); i < len; i++) {
    if ((lf = LearnFileFindObj(I(assertion, i)))) return(lf);
  }
  return(LearnFileFindCatchall());
}

/* INITIALIZATION */

void LearnInit()
{
  if (1) {
    StreamSuggEnglishInfl = NULL;
    StreamSuggFrenchInfl = NULL;
  } else {
    StreamSuggEnglishInfl = StreamOpen("outeinfl.txt", "w+");
    StreamSuggFrenchInfl = StreamOpen("outfinfl.txt", "w+");
  }
  LearnFiles = NULL;
  /* todo: More learning output files could be added here, but beware the
   * limit of 15 open files on the Mac.
   */
  LearnFileAdd("outlname.txt", N("surname"));
  LearnFileAdd("outfname.txt", N("given-name"));
  LearnFileAdd("outhuman.txt", N("human"));
  LearnFileAdd("outlrn.txt", NULL);
}

void LearnClose()
{
  LearnFile	*lf;
  for (lf = LearnFiles; lf; lf = lf->next) {
    fputc(NEWLINE, lf->stream);
    StreamClose(lf->stream);
  }
  if (StreamSuggEnglishInfl != NULL) {
    StreamClose(StreamSuggEnglishInfl);
  }
  if (StreamSuggFrenchInfl != NULL) {
    StreamClose(StreamSuggFrenchInfl);
  }
}

/* HELPING FUNCTIONS */

void LearnAnnotate(LearnFile *lf, Discourse *dc, char *input_text)
{
  Channel	*ch;
  if (input_text &&
      (0 != strncmp(input_text, lf->last_input_text1, LEARN_INTXT_LEN))) {
    StringCpy(lf->last_input_text1, input_text, (int)LEARN_INTXT_LEN);
    fputs("\n; ", lf->stream);
    StreamPutsNoNewlineMax(input_text, lf->stream, (int)LEARN_INTXT_LEN);
    fputc(NEWLINE, lf->stream);
  }
  if (dc &&
      (ch = DiscourseGetInputChannel(dc)) &&
      (ch->input_text != NULL) &&
      ch->input_text[0] &&
      (0 != strncmp(ch->input_text, lf->last_input_text2, LEARN_INTXT_LEN))) {
    StringCpy(lf->last_input_text2, ch->input_text, (int)LEARN_INTXT_LEN);
    fputs("\n; ", lf->stream);
    StreamPutsNoNewlineMax(ch->input_text, lf->stream, (int)LEARN_INTXT_LEN);
    fputc(NEWLINE, lf->stream);
  }
}

/* COMMENTARY FUNCTIONS */

void LearnCommentary_I_Know(Context *cx, Obj *existing)
{
  Discourse	*dc;
  if (cx) dc = cx->dc; else dc = StdDiscourse;
  Dbg(DBGLEX, DBGDETAIL, "ThoughtTreasure ALREADY KNEW");
  CommentaryAdd(cx, ObjListCreate(ObjPastify(L(N("know"), Me, existing, E), dc),
                                  NULL),
                NULL);
}

void LearnCommentary_I_Did_Not_Know(Context *cx, Obj *not_known)
{
  Discourse	*dc;
  if (cx) dc = cx->dc; else dc = StdDiscourse;
  Dbg(DBGLEX, DBGDETAIL, "ThoughtTreasure DID NOT KNOW");
  CommentaryAdd(cx, ObjListCreate(ObjPastify(L(N("not"), L(N("know"), Me,
                                               not_known, E), E),
                                             dc),
                                  NULL),
                NULL);
}

void LearnCommentary_I_Thought(Context *cx, Obj *existing, int gen_ts)
{
  Discourse	*dc;
  if (cx) dc = cx->dc; else dc = StdDiscourse;
  Dbg(DBGLEX, DBGDETAIL, "ThoughtTreasure THOUGHT OTHERWISE");
  CommentaryAdd(cx,
    ObjListCreate(L(N("but"),
                    ObjPastify(L(N("believe"), Me, existing, E), dc),
                    E), NULL),
                gen_ts ? N("absolute-time-interrogative-adverb") : NULL);
       /* todo: "absolute-time-interrogative-adverb" is misleading here.
        * All this does is cause timestamp to be generated.
        */
}

void LearnCommentary_Specification(Context *cx, Obj *existing, Obj *new,
                                   int gen_ts)
{
  Discourse	*dc;
  if (cx) dc = cx->dc; else dc = StdDiscourse;
  CommentaryAdd(cx, ObjListCreate(L(N("but"),
                                ObjPastify(L(N("know"), Me, existing, E), dc),
                                ObjPastify(L(N("not"), L(N("know"), Me, new, E),
                                E), dc),
                                E), NULL),
                gen_ts ? N("absolute-time-interrogative-adverb") : NULL);
}

/* UPDATE FUNCTIONS */

/* Example:
 * le_text:  "Jim Garnier"
 * features: "MNz" (must include pos and lang)
 * obj:      N("Jim-Garnier")
 *
 * todo: This fails on "Mr Bates". Comes up with "Mr Bate" when AlgMorph is on.
 */
void LearnObjLeUPDATE(char *le_text, char *features, Obj *obj)
{
  LexEntry	*le;
  HashTable *ht;
  char		le_text1[PHRASELEN];
  ht = LexEntryLangHt(features);
  StringCopyExcept(le_text, TREE_ESCAPE, WORDLEN, le_text1);
  if (!(le = LexEntryFindInfl(le_text1, features, ht, 0))) {
    le = LexEntryNewWord(le_text1, features, obj, ht);
  }
  LexEntryLinkToObj(le, obj, features, ht, NULL, 0, NULL, NULL, NULL);
}

void LearnNameLeUPDATE(Name *nm, int gender, Obj *human)
{
  char		feat[FEATLEN];
  sprintf(feat, "%c%c%c", (char)gender, (char)F_NOUN, (char)F_ENGLISH);
  LearnObjLeUPDATE(nm->fullname, feat, human);
  sprintf(feat, "%c%c%c", (char)gender, (char)F_NOUN, (char)F_FRENCH);
  LearnObjLeUPDATE(nm->fullname, feat, human);
}

/* DUMP FUNCTIONS */

int LearnFileCloseAssertion(LearnFile *lf)
{
  if (lf->close_assertion) {
    fputc(NEWLINE, lf->stream);
    fputc(TREE_SLOT_SEP, lf->stream);
    lf->close_assertion = 0;
    return(1);
  } else {
    return(0);
  }
}

/* Dumps a learned object to appropriate learn file.
 *
 *          ex1                        ex2
 * parent:  N("media-object-book")     N("media-object-book")
 * obj:     N("Principles-of-AI")      N("book23")
 * le:      "Principles#P of#R AI#"    "Principles#P of#R AI#"
 * feat1:    "z"                        "z"
 * is_le0:  1                          0
 * create_flag: OBJ_CREATE_C           OBJ_CREATE_C
 * outputs:
 * ===book//
 * ===*Principles#P of#R AI#.z//          (ex1)
 * or
 * ===*book23//Principles#P of#R AI#.z/  (ex2)
 *
 * More examples:
 * parent:   N("human")           N("human")
 * obj:      N("Art-Kleps")       N("human23")
 * le:       "Art Kleps"          "Art Kleps" 
 * feat1:    "Mº"                 "Mº"
 * is_le0:   1                    0
 * outputs:  ===*Art Kleps.Mº//    ===*Art-Kleps//Art Kleps.Mº/
 *
 * parent:   N("Anglo-American-surname")  N("Usenet-newsgroup")
 * obj:      N("aasn-Murphy")             N("alt.dcom.telecom")
 * le:       "Murphy"                     "alt.dcom.telecom"
 * feat1:     "Nz"                        "Éz"
 * feat2:    "MNy"                        "y"
 * is_le0:   0                            1
 * create_flag: OBJ_CREATE_A              OBJ_CREATE_A
 * outputs: 
 *   ====aasn-Murphy//Murphy.z/Murphy.My/
 */
void LearnObjDUMP(Obj *parent, Obj *obj, char *le, char *feat1, char *feat2,
                  int is_le0, int create_flag, char *input_text,
                  Discourse *dc)
{
  char		feat[FEATLEN];
  int		annot;
  LearnFile	*lf;
  annot = 0;
  if (NULL == (lf = LearnFileFindClass(parent))) return;
  LearnAnnotate(lf, dc, input_text);
  if (parent != lf->last_parent) {
    LearnFileCloseAssertion(lf);
    LearnAnnotate(lf, dc, input_text);
    annot = 1;
    fprintf(lf->stream, "===%s//\n", M(parent));
    lf->last_parent = parent;
    lf->last_obj = NULL;
  }
  if (obj != lf->last_obj) {
    LearnFileCloseAssertion(lf);
    if (!annot) {
      LearnAnnotate(lf, dc, input_text);
      annot = 1;
    }
    TsRangeSetNa(&lf->last_tsr);
    if (is_le0 && feat1) {
      StringCpy(feat, feat1, FEATLEN);
      FeaturesForDbFile(feat);
      fprintf(lf->stream, "===%c%s.%s//",
              ObjCreateFlagToDbFileChar(create_flag),
              le, feat);
    } else {
      fprintf(lf->stream, "===%c%s//",
              ObjCreateFlagToDbFileChar(create_flag),
              M(obj));
    }
    lf->last_obj = obj;
  }
  if (!annot) {
    LearnAnnotate(lf, dc, input_text);
    annot = 1;
  }
  if ((!is_le0) && feat1) {
    StringCpy(feat, feat1, FEATLEN);
    FeaturesForDbFile(feat);
    fprintf(lf->stream, "%s.%s/", le, feat);
  }
  if (feat2) {
    StringCpy(feat, feat2, FEATLEN);
    FeaturesForDbFile(feat);
    fprintf(lf->stream, "%s.%s/", le, feat);
  }
  fputc(NEWLINE, lf->stream);
  fflush(lf->stream);
}

/* Dumps a learned assertion to outlrn.txt.
 *
 * Example:
 * obj:    [nationality-of Jim country-USA]
 * dc:     StdDiscourse
 * outputs to lf->stream:
 * [nationality-of Jim country-USA]
 *   or
 * nationality-of=country-USA
 *   or
 * nationality-of¤Jim
 */
void LearnAssertDUMP(Obj *obj, Discourse *dc)
{
  int		len;
  LearnFile	*lf;
  if (NULL == (lf = LearnFileFindAssertion(obj))) return;
  LearnAnnotate(lf, dc, NULL);
  len = ObjLen(obj);
  if (!LearnFileCloseAssertion(lf)) fputc(TREE_SLOT_SEP, lf->stream);
  if (!TsRangeEqual(&lf->last_tsr, ObjToTsRange(obj))) {
    TsRangePrint(lf->stream, ObjToTsRange(obj));
    fputc(TREE_SLOT_SEP, lf->stream);
    lf->last_tsr = *ObjToTsRange(obj);
  }
  if (len == 3 && I(obj, 1) == lf->last_obj) {
    fprintf(lf->stream, "%s=", M(I(obj, 0)));
    ObjPrint1(lf->stream, I(obj, 2), NULL, INTPOSINF, 0, 0, 0, 0);
  } else if (len == 3 && I(obj, 2) == lf->last_obj) {
    fprintf(lf->stream, "%s¤", M(I(obj, 0)));
    ObjPrint1(lf->stream, I(obj, 1), NULL, INTPOSINF, 0, 0, 0, 0);
  } else if (len == 2 && I(obj, 1) == lf->last_obj) {
    fprintf(lf->stream, "%s", M(I(obj, 0)));
  } else {
    ObjPrint1(lf->stream, obj, NULL, INTPOSINF, 0, 0, 0, 0);
  }
  lf->close_assertion = 1;
  fflush(lf->stream);
}

/* DUMP AND UPDATE FUNCTIONS */

void LearnAssert(Obj *obj, Discourse *dc)
{
  if (ObjAsserted(obj)) return;
  LearnAssertDUMP(obj, dc);
  DbAssert1(obj);
}

void LearnAssertTs(Ts *ts, Obj *obj, Discourse *dc)
{
  TsRange	*tsr;
  if (obj->type != OBJTYPELIST) {
    Dbg(DBGSEMPAR, DBGBAD, "LearnAssert: asserting nonlist");
    return;
  }
  if (ObjAsserted(obj)) return;
  tsr = ObjToTsRange(obj);
  TsRangeSetNa(tsr);
  tsr->startts = *ts;
  tsr->cx = ts->cx;
  LearnAssert(obj, dc);
}

/* Invoke for database assertions already modified destructively (UPDATed).
 * Examples: Retracts, other ts modifications, specifications.
 */
void LearnAssertDest(Obj *obj, Discourse *dc)
{
  LearnAssertDUMP(obj, dc);
}

/* This is only to be used for retracting non-pattern <obj> already asserted. */
void LearnRetract(Ts *ts, Obj *obj, Discourse *dc)
{
  TsRange	*tsr;
  if (!ObjAsserted(obj)) {
    Dbg(DBGSEMPAR, DBGBAD, "LearnRetract: obj not asserted");
    return;
  }
  tsr = ObjToTsRange(obj);
  tsr->stopts = *ts;
  if (DbgOn(DBGDB, DBGDETAIL)) {
    fputs("****RETRACTED:", Log);
    ObjPrint1(Log, obj, NULL, 5, 1, 0, 0, 0);
    fputc(NEWLINE, Log);
  }
  LearnAssertDest(obj, dc);
}

/* Dump a learned object and update lexical entries.
 * <le> does NOT contain inflection instructions.
 */
void LearnObj(Obj *parent, Obj *obj, char *le, char *feat1, char *feat2,
              int is_le0, int create_flag, char *input_text,
              Discourse *dc)
{
  LearnObjDUMP(parent, obj, le, feat1, feat2, is_le0,
               create_flag, input_text, dc);
  if (feat1) LearnObjLeUPDATE(le, feat1, obj);
  if (feat2) LearnObjLeUPDATE(le, feat2, obj);
}

/* Alter class of object. This would normally be a specification. */
void LearnObjClass(Obj *obj, Obj *was_class, Obj *is_class)
{
  ObjReclassify(obj, was_class, is_class);
  LearnObjDUMP(is_class, obj, M(obj), NULL, NULL, 0, ObjCreateFlag(obj),
               "reclassification", NULL);
}

/* HIGHER-LEVEL LEARNING FUNCTIONS */

Obj *LearnName(char *name, Obj *class, int gender, char *input_text,
               Discourse *dc)
{
  char	objname[PHRASELEN], objname0[PHRASELEN];
  char	*feat1, *feat2;
  int	create_flag;
  Obj	*parent, *obj;
  LexEntryToObjName(name, objname0, 0, 0);
  if (ISA(N("surname"), class)) {
    sprintf(objname, "aasn-%s", objname0);
    parent = N("Anglo-American-surname");
    feat1 = "Nz";
    feat2 = "MNy";
  } else if (gender == F_MASCULINE) {
    sprintf(objname, "aamgn-%s", objname0);
    parent = N("Anglo-American-male-given-name");
    feat1 = "MNz";
    feat2 = "MNy";
  } else if (gender == F_FEMININE) {
    sprintf(objname, "aafgn-%s", objname0);
    parent = N("Anglo-American-female-given-name");
    feat1 = "FNz";
    feat2 = "FNy";
  } else {
    sprintf(objname, "otugn-%s", objname0);
    parent = N("other-unknown-gender-given-name");
    feat1 = "Nz";
    feat2 = "Ny";
  }
  create_flag = OBJ_CREATE_A;
  obj = ObjCreateInstanceNamed(parent, objname, create_flag);
  /* todo: name=>LexEntryPhraseToDbFile=>name if several words? */
  LearnObj(parent, obj, name, feat1, feat2, 0, create_flag, input_text, dc);
  return(obj);
}

void LearnNewNames(int gender)
{
  char	line[LINELEN];
  FILE	*stream;
  Name	*nm;
  if (NULL == (stream = StreamOpen("in", "r"))) return;
  while (fgets(line, LINELEN, stream)) {
    line[strlen(line)-1] = TERM;
    nm = TA_NameParseKnown(line, gender, 1);
    NameFree(nm);
  }
  StreamClose(stream);
}

/* Example:
 * parent: N("media-object-book")  N("Usenet-newsgroup")
 * text:   "Principles of AI"      "alt.dcom.telecom"
 * gender: F_NULL                  F_NULL
 * number: F_SINGULAR              F_SINGULAR
 * pos:    F_NOUN                  F_NOUN
 * lang:   F_ENGLISH               F_ENGLISH
 * create_flag: OBJ_CREATE_C       OBJ_CREATE_A
 * Outputs to lf->stream: 
 *   ===*Principles#P of#R AI#.z//
 */
Obj *LearnObjText(Obj *parent, char *text, int gender, int number, int pos,
                  int lang, int existing_ok, int create_flag, char *input_text,
                  Discourse *dc)
{
  int		is_le0, existing;
  Obj		*obj;
  char		le[PHRASELEN+20], feat[FEATLEN], *p;
  HashTable	*ht;
  ht = LexEntryLangHt1(lang);
  obj = ObjCreateInstanceText(parent, text, existing_ok, create_flag, &is_le0,
                              &existing);
  if (existing) {
    return(obj);
  }
  LexEntryPhraseToDbFile(text, gender, pos, number, lang, ht, PHRASELEN+20, le);
  p = feat;
  if (gender != F_NULL) *p++ = gender;
  if (number != F_NULL) *p++ = number;
  if (pos != F_NULL) pos = F_NOUN;
  *p++ = pos;
  if (lang != F_NULL) *p++ = lang;
  *p = TERM;
  LearnObjDUMP(parent, obj, le, feat, NULL, is_le0, create_flag, input_text,
               dc);
  LexEntryPhraseRead(le, feat, obj, gender, pos, number, lang, ht);
    /* This is called second because it steps on <le>. */
  return(obj);
}

/* Example:
 * parent: N("Usenet-newsgroup")
 * text:   "alt.dcom.telecom"
 * outputs to lf->stream: 
 *   ====alt\.dcom\.telecom.z//alt\.dcom\.telecom.y/
 */
Obj *LearnObjToken(Obj *parent, char *text, char *input_text, Discourse *dc)
{
  char		*feat1, *feat2, le[PHRASELEN+20];
  Obj		*obj;
  obj = StringToObj(text, parent, 1);
  feat1 = "Nz";
  feat2 = "Ny";
  LexEntryPhraseToDbFile(text, F_NULL, F_NULL, F_NULL, F_NULL, NULL,
                       PHRASELEN+20, le);
  LearnObjDUMP(parent, obj, le, feat1, feat2, 1, OBJ_CREATE_A, input_text, dc);
  LexEntryPhraseRead(le, feat1, obj, F_NULL, F_NOUN, F_SINGULAR, F_ENGLISH,
                     EnglishIndex);
  LexEntryPhraseRead(le, feat2, obj, F_NULL, F_NOUN, F_SINGULAR, F_FRENCH,
                     FrenchIndex);
  return(obj);
}

/* Assumes caller clears *sex and *nationality to NULL. */
void LearnHumanGetInfo(ObjList *human_names, /* RESULTS */
                       Obj **sex, Obj **nationality,
                       ObjList **sex_just, ObjList **nation_just)
{
  Obj		*sex1, *nationality1;
  ObjList	*p;
  for (p = human_names; p; p = p->next) {
    if ((sex1 = DbGetEnumValue(&TsNA, NULL, N("sex"), p->obj, NULL))) {
      if (*sex == NULL || *sex == sex1) {
      /* Set or bolster the justification. */
        *sex = sex1;
        *sex_just = ObjListCreate(L(sex1, p->obj, E), *sex_just);
      }
    }
    if ((nationality1 =
         DbGetRelationValue(&TsNA, NULL, N("nationality-of"), p->obj, NULL))) {
      if (*nationality == NULL || *nationality == nationality1) {
      /* Set or bolster the justification. */
        *nationality = nationality1;
        *nation_just =
          ObjListCreate(L(N("nationality-of"), p->obj, nationality1, E),
                        *nation_just);
      }
    }
  }
}

/* Example:
 * nm->fullname: "Art Kleps"
 * dumps to lf->stream and updates in core:
 *    ==*Art-Kleps//Art Kleps.Mº/|male|nationality-of=country-USA|
 * or ==*Art Kleps.Mº//|male|nationality-of=country-USA|
 *
 * todoSCORE: Once a new human is added, it should be stored in the discourse
 * context, to lock out other interpretations.
 */
Obj *LearnHuman(Name *nm, char *input_text, Discourse *dc, /* RESULTS */
                int *gender)
{
  char		feat[FEATLEN];
  int		is_le0;
  Obj		*new_human, *sex, *nationality, *new_assertion;
  ObjList	*sex_just, *nation_just;
  if (nm == NULL) {
    /* todo: Later learning of this name. */
    new_human = ObjCreateInstance(N("human"), NULL);
    feat[0] = TERM;	/* todo */
    LearnObjDUMP(N("human"), new_human, M(new_human), feat, NULL, 0,
                 OBJ_CREATE_C, input_text, dc);
    if (gender) *gender = F_NULL;
    return(new_human);
  }

  new_human = ObjCreateInstanceText(N("human"), nm->fullname, 0, OBJ_CREATE_C,
                                    &is_le0, NULL);

  /* Guess sex and nationality of human.
   * Earlier overrides later. So, for example, nationality from last name
   * overrides from first name. sex from title overrides from name.
   */
  sex = nationality = NULL;
  sex_just = nation_just = NULL;
  LearnHumanGetInfo(nm->surnames1, &sex, &nationality, &sex_just, &nation_just);
  LearnHumanGetInfo(nm->surnames2, &sex, &nationality, &sex_just, &nation_just);
  LearnHumanGetInfo(nm->pretitles, &sex, &nationality, &sex_just, &nation_just);
  LearnHumanGetInfo(nm->givennames1, &sex, &nationality, &sex_just,
                    &nation_just);
  /* Ignore U.S. middlenames which are often misleading. */
  LearnHumanGetInfo(nm->posttitles, &sex, &nationality, &sex_just,
                    &nation_just);
  feat[0] = SexToFeat(sex);
  if (gender) *gender = feat[0];
  feat[1] = F_NAME;
  feat[2] = TERM;
  LearnObjDUMP(N("human"), new_human, nm->fullname, feat, NULL, is_le0,
               OBJ_CREATE_C, input_text, dc);
  LexEntryNameForHuman(new_human, (int)feat[0], nm);
    /* Calls LearnNameLeUPDATE. */

  if (sex) {
    /* todo: Mark these assertions as defeasible. */
    new_assertion = L(sex, new_human, E);
    ObjSetJustification(new_assertion, sex_just);
    TsRangeSetNa(ObjToTsRange(new_assertion));
    LearnAssert(new_assertion, dc);
  }
  if (nationality) {
    new_assertion = L(N("nationality-of"), new_human, nationality, E);
    ObjSetJustification(new_assertion, nation_just);
    TsRangeSetNa(ObjToTsRange(new_assertion));
    LearnAssert(new_assertion, dc);
  }

  return(new_human);
}

/* End of file. */
