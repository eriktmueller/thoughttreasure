/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940823: begun
 */

#include "tt.h"
#include "lexentry.h"
#include "lexmorph.h"
#include "lexobjle.h"
#include "repbasic.h"
#include "repdb.h"
#include "repobj.h"
#include "repobjl.h"
#include "repstr.h"
#include "semdisc.h"
#include "uascript.h"
#include "utildbg.h"

Float Percent(int numer, int denom)
{
  if (denom == 0) return(0.0);
  else return(100.0*(numer/(Float)denom));
}

void StatInit(Stat *s)
{
  s->sum = s->total = 0;
  s->min = LONGPOSINF;
  s->max = LONGNEGINF;
}

void StatNext(Stat *s, long a)
{
  if (a > s->max) s->max = a;
  if (a < s->min) s->min = a;
  s->sum += a;
  s->total++;
}

Float StatAvg(Stat *s)
{
  if (s->total == 0) return(0.0);
  else return(s->sum/(Float)s->total);
}

long StatMin(Stat *s)
{
  if (s->min == LONGPOSINF) return(-1);
  return(s->min);
}

long StatMax(Stat *s)
{
  if (s->max == LONGNEGINF) return(-1);
  return(s->max);
}

#define SEMYSIZE	3

void StatLexEntry2(FILE *stream, char *features, Bool is_phrase)
{
  int		i;
  long		selected, total, concount, inflcount, semy[SEMYSIZE];
  LexEntry	*le;
  LexEntryToObj	*leo;
  Word		*word;
  Stat		constat, inflstat;
  ObjList	*objs;
  selected = total = 0;
  StatInit(&constat);
  StatInit(&inflstat);
  for (i = 0; i < SEMYSIZE; i++) {
    semy[i] = 0L;
  }
  for (le = AllLexEntries; le; le = le->next) {
    total++;
    if (StringAllIn(features, le->features) &&
        (is_phrase == 2 || is_phrase == LexEntryIsPhraseOrExpl(le))) {
      selected++;
      objs = NULL;
      for (leo = le->leo; leo; leo = leo->next) {
        objs = ObjListAddIfNotIn(objs, leo->obj);
      }
      concount = ObjListLen(objs);
      ObjListFree(objs);
      semy[IntMin(concount, SEMYSIZE-1)]++;
      StatNext(&constat, concount);
      inflcount = 0;
      for (word = le->infl; word; word = word->next) inflcount++;
      StatNext(&inflstat, inflcount);
    }
  }
  fprintf(stream, "%-3.3s", features);
  fprintf(stream, "%7.7s",
          is_phrase == 2 ? "" : (is_phrase == 1 ? "phrase" : "word"));
  fprintf(stream, "%6ld", selected);
  fprintf(stream, "%5.1f", Percent(selected, total));
  fprintf(stream, "%6ld", constat.sum);
  fprintf(stream, "%5.1f", StatAvg(&constat));
  fprintf(stream, "%5.1f", Percent(semy[0], selected));
  fprintf(stream, "%5.1f", Percent(semy[1], selected));
  fprintf(stream, "%5.1f", Percent(semy[2], selected));
  fprintf(stream, "%5.1f", Percent(semy[2], semy[1]+semy[2]));
  fprintf(stream, "%7ld", inflstat.sum);
  fprintf(stream, "%5.1f", StatAvg(&inflstat));
  fputc(NEWLINE, stream);
  fflush(stream);
}

void StatLexEntry1(FILE *stream, char *features)
{
  StatLexEntry2(stream, features, 0);
  StatLexEntry2(stream, features, 1);
  StatLexEntry2(stream, features, 2);
}

void StatLexEntry(FILE *stream)
{
  char	*lang, *pos, features[3];
  fprintf(stream, "Lexical entries:\n");
fprintf(stream,
"fea type       n    %%  objs  /le    0    1  >=2 poly  infls i/le\n"
);
fprintf(stream,
"___ ______ _____ ____ _____ ____ ____ ____ ____ ____ ______ ____\n"
);
  features[2] = TERM;
  for (lang = FT_LANG; (features[0] = lang[0]); lang++) {
    for (pos = "NAV"; (features[1] = pos[0]); pos++) {
      StatLexEntry1(stream, features);
    }
    features[1] = TERM;
    StatLexEntry1(stream, features);
    features[1] = F_INFL_CHECKED;
    StatLexEntry2(stream, features, 0);
  }
  features[0] = TERM;
  StatLexEntry1(stream, features);
}

#define SYNSIZE	5

void StatObj1(FILE *stream, Obj *class)
{
  int		i;
  long		selected, total, lecount, syn[SYNSIZE];
  char		*title;
  Obj		*obj;
  ObjList	*assertions, *objs;
  ObjToLexEntry	*ole;
  Stat		lestat, parentstat;

  selected = total = 0;
  StatInit(&lestat);
  StatInit(&parentstat);
  for (i = 0; i < SYNSIZE; i++) {
    syn[i] = 0L;
  }
  assertions = NULL;
  for (obj = Objs; obj; obj = obj->next) {
    if (!ObjIsSymbol(obj)) continue;
    total++;
    if (class == N("concept") || ISA(class, obj)) {
      selected++;
      lecount = 0;
      for (ole = obj->ole; ole; ole = ole->next) {
        lecount++;
      }
      syn[IntMin(lecount, SYNSIZE-1)]++;
      StatNext(&lestat, lecount);
      StatNext(&parentstat, (long)ObjNumParents(obj));
      objs = DbRetrieveInvolving(&TsNA, NULL, obj, 0, NULL);
      assertions = ObjListAddIfNotInObjList(assertions, objs); /* todo: Slow. */
      ObjListFree(objs);
    }
  }

  if (class == N("F65")) title = "adjective";
  else if (class == N("F66")) title = "adverb";
  else if (class == N("F68")) title = "determiner";
  else if (class == N("F72")) title = "pronoun";
  else if (class == N("F82")) title = "preposition";
  else if (class == N("F85")) title = "interjection";
  else if (class == N("F86")) title = "verb";
  else title = M(class);

  fprintf(stream, "%-17.17s", title);
  fprintf(stream, "%5ld", selected);
  fprintf(stream, "%5.1f", Percent(selected, total));
  fprintf(stream, "%6ld", lestat.sum);
  fprintf(stream, "%4.1f", StatAvg(&lestat));
  fprintf(stream, "%5.1f", Percent(syn[0], selected));
  fprintf(stream, "%5.1f", Percent(syn[1], selected));
  fprintf(stream, "%5.1f", Percent(syn[2], selected));
  fprintf(stream, "%5.1f", Percent(syn[3], selected));
  fprintf(stream, "%5.1f", Percent(syn[4], selected));
  fprintf(stream, "%6d", ObjListLen(assertions));
  fprintf(stream, "%6ld", parentstat.sum);
  fputc(NEWLINE, stream);
  fflush(stream);

  ObjListFree(assertions);
}

void StatObj(FILE *stream)
{
  fprintf(stream, "Objects:\n");
  fprintf(stream,
"class                n    %%   les  /o    0    1    2    3    4 asser   ISA\n"
);
  fprintf(stream,
"________________ _____ ____ _____ ___ ____ ____ ____ ____ ____ _____ _____\n"
);
  StatObj1(stream, N("concept"));
  StatObj1(stream, N("human-name"));
  StatObj1(stream, N("polity"));
  StatObj1(stream, N("holiday"));
  StatObj1(stream, N("eat-meal"));
  StatObj1(stream, N("eating-place"));
  StatObj1(stream, N("food"));
  StatObj1(stream, N("object"));
  StatObj1(stream, N("abstract-object"));
  StatObj1(stream, N("media-object"));
  StatObj1(stream, N("theory"));
  StatObj1(stream, N("protocol"));
  StatObj1(stream, N("mail"));
  StatObj1(stream, N("computer-program"));
  StatObj1(stream, N("operating-system"));
  StatObj1(stream, N("computer-language"));
  StatObj1(stream, N("AI-program"));
  StatObj1(stream, N("media-object-book"));
  StatObj1(stream, N("title"));
  StatObj1(stream, N("surname"));
  StatObj1(stream, N("given-name"));
  StatObj1(stream, N("country"));
  StatObj1(stream, N("country-subdivision"));
  StatObj1(stream, N("city"));
  StatObj1(stream, N("research-institution"));
  StatObj1(stream, N("educational-institution"));
  StatObj1(stream, N("political-system"));
  StatObj1(stream, N("government-institution"));
  StatObj1(stream, N("human-occupation"));
  StatObj1(stream, N("diploma"));
  StatObj1(stream, N("legal-concept"));
  StatObj1(stream, N("intellectual-property-right"));
  StatObj1(stream, N("financial-market"));
  StatObj1(stream, N("financial-instrument"));
  StatObj1(stream, N("metaphysical-concept"));
  StatObj1(stream, N("religion"));
  StatObj1(stream, N("data-structure"));
  StatObj1(stream, N("knowledge-structure"));
  StatObj1(stream, N("mathematics"));
  StatObj1(stream, N("number"));
  StatObj1(stream, N("unit-of-time"));
  StatObj1(stream, N("unit-of-temperature"));
  StatObj1(stream, N("unit-of-liquid-volume"));
  StatObj1(stream, N("unit-of-distance"));
  StatObj1(stream, N("unit-of-area"));
  StatObj1(stream, N("unit-of-volume"));
  StatObj1(stream, N("unit-of-velocity"));
  StatObj1(stream, N("unit-of-weight"));
  StatObj1(stream, N("value-range-name"));
  StatObj1(stream, N("part-of-the-day"));
  StatObj1(stream, N("speed-description"));
  StatObj1(stream, N("temperature-description"));
  StatObj1(stream, N("value-name"));
  StatObj1(stream, N("fraction"));
  StatObj1(stream, N("ordinal-number"));
  StatObj1(stream, N("cardinal-number"));
  StatObj1(stream, N("linguistic-concept"));
  StatObj1(stream, N("affix"));
  StatObj1(stream, N("concept-cliche"));
  StatObj1(stream, N("langue"));
  StatObj1(stream, N("grammatical-aspect"));
  StatObj1(stream, N("tense"));
  StatObj1(stream, N("canned-question"));
  StatObj1(stream, N("F65"));
  StatObj1(stream, N("F66"));
  StatObj1(stream, N("F68"));
  StatObj1(stream, N("F72"));
  StatObj1(stream, N("F82"));
  StatObj1(stream, N("F85"));
  StatObj1(stream, N("F86"));
  StatObj1(stream, N("atom"));
  StatObj1(stream, N("molecule"));
  StatObj1(stream, N("compound"));
  StatObj1(stream, N("clothing"));
  StatObj1(stream, N("organization"));
  StatObj1(stream, N("international-organization"));
  StatObj1(stream, N("company"));
  StatObj1(stream, N("music"));
  StatObj1(stream, N("physical-object"));
  StatObj1(stream, N("building"));
  StatObj1(stream, N("exterior-area"));
  StatObj1(stream, N("roadway"));
  StatObj1(stream, N("street"));
  StatObj1(stream, N("avenue"));
  StatObj1(stream, N("room"));
  StatObj1(stream, N("furniture"));
  StatObj1(stream, N("musical-instrument"));
  StatObj1(stream, N("personal-article"));
  StatObj1(stream, N("material"));
  StatObj1(stream, N("paper"));
  StatObj1(stream, N("transportation-vehicle"));
  StatObj1(stream, N("car"));
  StatObj1(stream, N("electronic-device"));
  StatObj1(stream, N("computer-equipment"));
  StatObj1(stream, N("switching-system"));
  StatObj1(stream, N("phone"));
  StatObj1(stream, N("microphone"));
  StatObj1(stream, N("network"));
  StatObj1(stream, N("beverage"));
  StatObj1(stream, N("drug"));
  StatObj1(stream, N("celestial-object"));
  StatObj1(stream, N("satellite"));
  StatObj1(stream, N("living-thing"));
  StatObj1(stream, N("part-of-human"));
  StatObj1(stream, N("human"));
  StatObj1(stream, N("state"));
  StatObj1(stream, N("relation"));
  StatObj1(stream, N("ipr"));
  StatObj1(stream, N("human-relation"));
  StatObj1(stream, N("attitude"));
  StatObj1(stream, N("goal"));
  StatObj1(stream, N("company-relation"));
  StatObj1(stream, N("temporal-relation"));
  StatObj1(stream, N("logical-relation"));
  StatObj1(stream, N("causal-relation"));
  StatObj1(stream, N("attribute"));
  StatObj1(stream, N("color"));
  StatObj1(stream, N("personality-trait"));
  StatObj1(stream, N("object-trait"));
  StatObj1(stream, N("sick"));
  StatObj1(stream, N("mental-state"));
  StatObj1(stream, N("emotion"));
  StatObj1(stream, N("weather"));
  StatObj1(stream, N("broadcast-station"));
  StatObj1(stream, N("broadcast"));
  StatObj1(stream, N("enum"));
  StatObj1(stream, N("action"));
  StatObj1(stream, N("primitive-action"));
  StatObj1(stream, N("mtrans"));
  StatObj1(stream, N("speech-act"));
  StatObj1(stream, N("ptrans"));
  StatObj1(stream, N("script"));
  StatObj1(stream, N("cognitive-process"));
  StatObj1(stream, N("interpersonal-script"));
  StatObj1(stream, N("grid"));
  StatObj1(stream, N("wormhole"));
  StatObj1(stream, N("universe"));
  StatObj1(stream, N("particle"));
  StatObj1(stream, N("force"));
}

void ReportStatistics(FILE *stream)
{
  Dbg(DBGGEN, DBGDETAIL, "starting stats");
  StreamSep(stream);
  fprintf(stream, "ThoughtTreasure Statistics\n");
  StreamSep(stream);
  StatLexEntry(stream);
  StreamSep(stream);
  StatObj(stream);
  StreamSep(stream);
  Dbg(DBGGEN, DBGDETAIL, "done with stats");
}

Bool StatIsHumanRole(Obj *obj)
{
  return ISA(N("human"), obj) || ISA(N("relation"), obj);
}

void StatClassify(FILE *stream)
{
  Obj           *obj;
  ObjList       *objs, *p;
  LexEntry	*le;
  LexEntryToObj	*leo;
  ThetaRole     *tr;
  int           isScript, foundmisc, foundhuman, foundphysobj;
  long numAtomicConcepts, numISA, numPartWhole, numTyping, numOtherAssertion;
  long numPhysObjProp, numInference, numLex, numLeo, numArg, numAtGrid;
  long numSubstance, numDefn, numAntonym, numRole, numFact, numProper;
  long numPhrase, numEntryCond, numCauses, numPurpose;
  long numActionLoc, numActionEvent, numScript;
  long numScriptRelEvents, numScriptRelMisc, numScriptRelPeople;
  long numScriptRelThings, numScriptRelPlaces, numScriptRelRole;
  long numScriptPerEvents, numScriptPerMisc, numScriptPerPlaces;
  long numScriptPerPeople, numScriptPerThings, numScriptPerRole;
  long numScriptPerEntryCond, numScriptPerResult, numScriptPerGoal;
  long numScriptPerEmotion;
  long numScriptPerDur, numScriptPerPeriod, numScriptPerCost;
  long numScriptPerRoleScript;
  long numScriptRelEntryCond, numScriptRelResult, numScriptRelGoal;
  long numScriptRelEmotion;
  long numScriptRelDur, numScriptRelPeriod, numScriptRelCost;
  long numScriptRelRoleScript;

  int pos, subcat_found;
  long numLexVerb, numLexSubcat;
  long numScriptTotalAssertions;
  long numOnTheFly;

  numLexVerb = 0L;
  numLexSubcat = 0L;

  numAtomicConcepts = numISA = numPartWhole = numTyping = 0L;
  numOtherAssertion = numSubstance = numProper = numPhrase = 0L;
  numPhysObjProp = numInference = numLex = numLeo = numArg = numAtGrid = 0L;
  numDefn = numAntonym = numRole = numFact = 0L;
  numEntryCond = numCauses = numPurpose = 0L;
  numActionLoc = numActionEvent = numScript = 0L;
  numScriptRelEvents = numScriptRelMisc = numScriptRelPeople = 0L;
  numScriptRelThings = numScriptRelPlaces = numScriptRelRole = 0L;
  numScriptPerEvents = numScriptPerMisc = numScriptPerPlaces = 0L;
  numScriptPerPeople = numScriptPerThings = numScriptPerRole = 0L;
  numScriptPerEntryCond = numScriptPerResult = numScriptPerGoal = 0L;
  numScriptPerEmotion = 0L;
  numScriptPerDur = numScriptPerPeriod = numScriptPerCost = 0L;
  numScriptPerRoleScript = 0L;
  numScriptRelEntryCond = numScriptRelResult = numScriptRelGoal = 0L;
  numScriptRelEmotion = 0L;
  numScriptRelDur = numScriptRelPeriod = numScriptRelCost = 0L;
  numScriptRelRoleScript = 0L;
  numScriptTotalAssertions = 0L;
  numOnTheFly = 0L;

  for (obj = Objs; obj; obj = obj->next) {
    if (!ObjIsSymbol(obj)) continue;
    if (!ISA(N("concept"), obj)) continue;
    numAtomicConcepts++;
    numISA += (long)ObjNumParents(obj);
    if (IsScript(obj)) {
      numScript++;
      numScriptPerEvents++; /* by definition */
      foundmisc = 0;
      if (RN(&TsNA, L(N("related-concept-of"), obj, ObjWild, E), 1)) {
        foundmisc = 1;
      }
      if (RN(&TsNA, L(N("performed-in"), obj, ObjWild, E), 1)) {
        numScriptPerPlaces++;
      }
      if ((objs = RD(&TsNA, L(N("role-relation"), obj, ObjWild, E), 0))) {
        numScriptPerRole++;
        foundhuman = foundphysobj = 0;
        for (p = objs; p; p = p->next) {
          if (StatIsHumanRole(I(p->obj, 2))) {
            foundhuman = 1;
          } else {
            foundphysobj = 1;
          }
        }
        if (foundhuman) numScriptPerPeople++;
        if (foundphysobj) numScriptPerThings++;
      }
      if (RN(&TsNA, L(N("entry-condition-of"), obj, ObjWild, E), 1)) {
        numScriptPerEntryCond++;
        foundmisc = 1;
      }
      if (RN(&TsNA, L(N("result-of"), obj, ObjWild, E), 1)) {
        numScriptPerResult++;
        foundmisc = 1;
      }
      if (RN(&TsNA, L(N("goal-of"), obj, ObjWild, E), 1)) {
        numScriptPerGoal++;
        foundmisc = 1;
      }
      if (RN(&TsNA, L(N("emotion-of"), obj, ObjWild, E), 1)) {
        numScriptPerEmotion++;
        foundmisc = 1;
      }
      if (RN(&TsNA, L(N("duration-of"), obj, ObjWild, E), 1)) {
        numScriptPerDur++;
        foundmisc = 1;
      }
      if (RN(&TsNA, L(N("period-of"), obj, ObjWild, E), 1)) {
        numScriptPerPeriod++;
        foundmisc = 1;
      }
      if (RN(&TsNA, L(N("cost-of"), obj, ObjWild, E), 1)) {
        numScriptPerCost++;
        foundmisc = 1;
      }
      if (RD(&TsNA, L(N("role-script-relation"), obj, ObjWild, E), 0)) {
        numScriptPerRoleScript++;
        foundmisc = 1;
      }
      if (foundmisc) numScriptPerMisc++;
    }
  }
  for (obj = Objs; obj; obj = obj->next) {
    if (!ObjAsserted(obj)) continue;
    if (ISA(N("relation-on-the-fly"), I(obj, 0))) {
      numOnTheFly++;
      continue;
    }
    if (I(obj, 1)) {
      isScript = IsScript(I(obj, 1));
    } else {
      isScript = 0;
    }
    if (isScript) {
      numScriptTotalAssertions++;
#ifdef notdef
    if (isScript && ISA(N("event-assertion-relation"), I(obj, 0))) {
      numScriptRelEvents++;
    } else if (isScript && ISA(N("related-concept-of"), I(obj, 0))) {
      numScriptRelMisc++;
    } else if (isScript && ISA(N("performed-in"), I(obj, 0))) {
      numScriptRelPlaces++;
    } else if (isScript && ISA(N("role-relation"), I(obj, 0))) {
      numScriptRelRole++;
      if (StatIsHumanRole(I(obj, 2))) {
        numScriptRelPeople++;
      } else {
        numScriptRelThings++;
      }
    } else if (isScript && ISA(N("entry-condition-of"), I(obj, 0))) {
      numScriptRelEntryCond++;
      numScriptRelMisc++; /* watch out: double counting */
    } else if (isScript && ISA(N("result-of"), I(obj, 0))) {
      numScriptRelResult++;
      numScriptRelMisc++; /* watch out: double counting */
    } else if (isScript && ISA(N("goal-of"), I(obj, 0))) {
      numScriptRelGoal++;
      numScriptRelMisc++; /* watch out: double counting */
    } else if (isScript && ISA(N("emotion-of"), I(obj, 0))) {
      numScriptRelEmotion++;
      numScriptRelMisc++; /* watch out: double counting */
    } else if (isScript && ISA(N("duration-of"), I(obj, 0))) {
      numScriptRelDur++;
      numScriptRelMisc++; /* watch out: double counting */
    } else if (isScript && ISA(N("period-of"), I(obj, 0))) {
      numScriptRelPeriod++;
      numScriptRelMisc++; /* watch out: double counting */
    } else if (isScript && ISA(N("cost-of"), I(obj, 0))) {
      numScriptRelCost++;
      numScriptRelMisc++; /* watch out: double counting */
    } else if (isScript && ISA(N("role-script-relation"), I(obj, 0))) {
      numScriptRelRoleScript++;
      numScriptRelMisc++; /* watch out: double counting */
    } else if (isScript) {
      numScriptRelMisc++; /* watch out: double counting */
      /* todo: This is set up to count scripts. Need to
       * fix this again for global statistics on assertions.
       */
#endif
    } else if (ISA(N("at-grid"), I(obj, 0))) {
      numAtGrid++;
    } else if (ISA(N("part-of"), I(obj, 0))) {
      numPartWhole++;
    } else if (ISA(N("material"), I(obj, 0))) {
      numSubstance++;
    } else if (ISA(N("physical-object-relation"), I(obj, 0)) ||
               ISA(N("color"), I(obj, 0))) {
      numPhysObjProp++;
    } else if (ISA(N("selectional-restriction"), I(obj, 0)) ||
               ISA(N("relation-type"), I(obj, 0)) ||
               ISA(N("other-relation-type"), I(obj, 0))) {
      numTyping++;
    } else if (ISA(N("attr-rel"), I(obj, 0))) {
      numDefn++;
    } else if (ISA(N("antonym-of"), I(obj, 0))) {
      numAntonym++;
    } else if (ISA(N("role-relation"), I(obj, 0))) {
      numRole++;
    } else if (ISA(N("if-then"), I(obj, 0))) {
      numInference++;
    } else if (ISA(N("entry-condition-of"), I(obj, 0))) {
      numEntryCond++;
    } else if (ISA(N("result-of"), I(obj, 0))) {
      numCauses++;
    } else if (ISA(N("goal-of"), I(obj, 0))) {
      numPurpose++;
    } else if (ISA(N("emotion-of"), I(obj, 0))) {
      numCauses++;
    } else if (ISA(N("performed-in"), I(obj, 0))) {
      numActionLoc++;
    } else if (ISA(N("event-assertion-relation"), I(obj, 0))) {
      numActionEvent++;
#ifdef notdef
    } else if (ISA(N("specialty-of"), I(obj, 0)) ||
               ISA(N("skill-of"), I(obj, 0)) ||
               ISA(N("human-occupation"), I(obj, 1))) { /* REALLY */
      numFact++;
    } else if (ISA(N("attribute"), I(obj, 0)) ||
               ISA(N("relation"), I(obj, 0))) {
      numFact++;
#endif
    } else {
      numOtherAssertion++;
    }
  }
  for (le = AllLexEntries; le; le = le->next) {
    pos = FeatureGet(le->features, FT_POS);
    numLex++;
    if (MorphIsProper(le->srcphrase)) numProper++;
    if (LexEntryIsPhrase(le)) numPhrase++;
    for (leo = le->leo; leo; leo = leo->next) {
      numLeo++;
      subcat_found = 0;
      for (tr = leo->theta_roles; tr; tr = tr->next) {
        numArg++;
        if (tr->cas == N("subj") || tr->cas == N("obj") ||
            tr->cas == N("iobj")) {
          subcat_found = 1;
        }
      }
      if (pos == F_VERB && subcat_found) numLexSubcat++;
    }
    if (pos == F_VERB) {
      numLexVerb++;
    }
  }

  fprintf(stream, "FORSCRIPT:PER:Events %ld\n", numScriptPerEvents);
  fprintf(stream, "FORSCRIPT:PER:Misc %ld\n", numScriptPerMisc);
  fprintf(stream, "FORSCRIPT:PER:People %ld\n", numScriptPerPeople);
  fprintf(stream, "FORSCRIPT:PER:Places %ld\n", numScriptPerPlaces);
  fprintf(stream, "FORSCRIPT:PER:Script %ld\n", numScript);
  fprintf(stream, "FORSCRIPT:PER:Things %ld\n", numScriptPerThings);
  fprintf(stream, "FORSCRIPT:PER:Roles %ld\n", numScriptPerRole);

  fprintf(stream, "FORSCRIPT:PER:EntryCond %ld\n", numScriptPerEntryCond);
  fprintf(stream, "FORSCRIPT:PER:Result %ld\n", numScriptPerResult);
  fprintf(stream, "FORSCRIPT:PER:Goal %ld\n", numScriptPerGoal);
  fprintf(stream, "FORSCRIPT:PER:Emotion %ld\n", numScriptPerEmotion);
  fprintf(stream, "FORSCRIPT:PER:Duration %ld\n", numScriptPerDur);
  fprintf(stream, "FORSCRIPT:PER:Period %ld\n", numScriptPerPeriod);
  fprintf(stream, "FORSCRIPT:PER:Cost %ld\n", numScriptPerCost);
  fprintf(stream, "FORSCRIPT:PER:RoleScript %ld\n", numScriptPerRoleScript);

  fprintf(stream, "FORSCRIPT:RELATION:Events %ld\n", numScriptRelEvents);
  fprintf(stream, "FORSCRIPT:RELATION:Misc %ld\n", numScriptRelMisc);
  fprintf(stream, "FORSCRIPT:RELATION:People %ld\n", numScriptRelPeople);
  fprintf(stream, "FORSCRIPT:RELATION:Places %ld\n", numScriptRelPlaces);
  fprintf(stream, "FORSCRIPT:RELATION:Script %ld\n", numScript);
  fprintf(stream, "FORSCRIPT:RELATION:Things %ld\n", numScriptRelThings);
  fprintf(stream, "FORSCRIPT:RELATION:Roles %ld\n", numScriptRelRole);

  fprintf(stream, "FORSCRIPT:RELATION:EntryCond %ld\n", numScriptRelEntryCond);
  fprintf(stream, "FORSCRIPT:RELATION:Result %ld\n", numScriptRelResult);
  fprintf(stream, "FORSCRIPT:RELATION:Goal %ld\n", numScriptRelGoal);
  fprintf(stream, "FORSCRIPT:RELATION:Emotion %ld\n", numScriptRelEmotion);
  fprintf(stream, "FORSCRIPT:RELATION:Duration %ld\n", numScriptRelDur);
  fprintf(stream, "FORSCRIPT:RELATION:Period %ld\n", numScriptRelPeriod);
  fprintf(stream, "FORSCRIPT:RELATION:Cost %ld\n", numScriptRelCost);
  fprintf(stream, "FORSCRIPT:RELATION:RoleScript %ld\n",
          numScriptRelRoleScript);

  fprintf(stream, "FORSCRIPT:AVGPER:Events %g\n",
          numScriptRelEvents/(numScript+0.0));
  fprintf(stream, "FORSCRIPT:AVGPER:Misc %g\n",
          numScriptRelMisc/(numScript+0.0));
  fprintf(stream, "FORSCRIPT:AVGPER:People %g\n",
          numScriptRelPeople/(numScript+0.0));
  fprintf(stream, "FORSCRIPT:AVGPER:Places %g\n",
          numScriptRelPlaces/(numScript+0.0));
  fprintf(stream, "FORSCRIPT:AVGPER:Script %g\n",
          numScript/(numScript+0.0));
  fprintf(stream, "FORSCRIPT:AVGPER:Things %g\n",
          numScriptRelThings/(numScript+0.0));
  fprintf(stream, "FORSCRIPT:AVGPER:Roles %g\n",
          numScriptRelRole/(numScript+0.0));

  fprintf(stream, "FORSCRIPT:AVGPER:EntryCond %g\n",
          numScriptRelEntryCond/(numScript+0.0));
  fprintf(stream, "FORSCRIPT:AVGPER:Result %g\n",
          numScriptRelResult/(numScript+0.0));
  fprintf(stream, "FORSCRIPT:AVGPER:Goal %g\n",
          numScriptRelGoal/(numScript+0.0));
  fprintf(stream, "FORSCRIPT:AVGPER:Emotion %g\n",
          numScriptRelEmotion/(numScript+0.0));
  fprintf(stream, "FORSCRIPT:AVGPER:Duration %g\n",
          numScriptRelDur/(numScript+0.0));
  fprintf(stream, "FORSCRIPT:AVGPER:Period %g\n",
          numScriptRelPeriod/(numScript+0.0));
  fprintf(stream, "FORSCRIPT:AVGPER:Cost %g\n",
          numScriptRelCost/(numScript+0.0));
  fprintf(stream, "FORSCRIPT:AVGPER:RoleScript %g\n",
          numScriptRelRoleScript/(numScript+0.0));

  fprintf(stream, "CONCEPT %ld\n", numAtomicConcepts);
  fprintf(stream, "ASSERTION:ANTONYM %ld\n", numAntonym);
  fprintf(stream, "ASSERTION:ATGRID %ld\n", numAtGrid);
  fprintf(stream, "ASSERTION:CAUSES %ld\n", numCauses);
  fprintf(stream, "ASSERTION:DEFINITION %ld\n", numDefn);
  fprintf(stream, "ASSERTION:ENTRYCOND %ld\n", numEntryCond);
  fprintf(stream, "ASSERTION:EVENT %ld\n", numActionEvent);
  fprintf(stream, "ASSERTION:FACT %ld\n", numFact);
  fprintf(stream, "ASSERTION:INFERENCERULE %ld\n", numInference);
  fprintf(stream, "ASSERTION:ISA %ld\n", numISA);
  fprintf(stream, "ASSERTION:LOCATION %ld\n", numActionLoc);
  fprintf(stream, "ASSERTION:PART %ld\n", numPartWhole);
  fprintf(stream, "ASSERTION:PHYSOBJPROP %ld\n", numPhysObjProp);
  fprintf(stream, "ASSERTION:PURPOSE %ld\n", numPurpose);
  fprintf(stream, "ASSERTION:SCRIPTROLE %ld\n", numRole);
  fprintf(stream, "ASSERTION:SUBSTANCE %ld\n", numSubstance);
  fprintf(stream, "ASSERTION:TYPING %ld\n", numTyping);
  fprintf(stream, "ASSERTION:OTHER %ld\n", numOtherAssertion);
  fprintf(stream, "ASSERTION:SCRIPTTOTAL %ld\n", numScriptTotalAssertions);
  fprintf(stream, "ASSERTION:ONTHEFLY %ld\n", numOnTheFly);
  fprintf(stream, "LEXICAL:ITEM %ld\n", numLex);
  fprintf(stream, "LEXICAL:PHRASE %ld\n", numPhrase);
  fprintf(stream, "LEXICAL:PROPER %ld\n", numProper);
  fprintf(stream, "LEXICAL:LEO %ld\n", numLeo);
  fprintf(stream, "LEXICAL:ARG %ld\n", numArg);
  fprintf(stream, "LEXICAL:LEXVERB %ld\n", numLexVerb);
  fprintf(stream, "LEXICAL:LEXSUBCAT %ld\n", numLexSubcat);
}

/* End of file. */
