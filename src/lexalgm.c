/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * Algorithmic inflectional morphology.
 *
 * 19940107: begun
 * 19940109: started turning into lexicon builder helper
 * 19940113: English inflections
 * 19940115: added re and ir verbs
 *
 * This module attempts to capture generalizations about inflections.
 * Since specific words are not hard-coded, its output must be checked
 * against a dictionary. There are plenty of exceptions.
 *
 * todo:
 * - Replace this with AnaMorph once AnaMorph's efficiency is improved?
 */

#include "tt.h"
#include "lexalgm.h"
#include "lexentry.h"
#include "repbasic.h"
#include "repstr.h"
#include "utildbg.h"

LexEntry *AlgMorphInflectWord(char *word, int lang, int pos, char *features,
                              HashTable *ht)
{
  int		gender;
  char		lefeatures[FEATLEN], leword[PHRASELEN];
  LexEntry	*le;
  LexEntryBuildLeFeatures(features, pos, lefeatures);
  NewInflections = NULL;
  gender = FeatureGet(features, FT_GENDER);
  AlgMorphGetRoot(word, gender, FeatureGet(features, FT_NUMBER), pos, lang,
                  leword);
  Dbg(DBGLEX, DBGHYPER, "root for <%s>.<%s> is <%s>", word, features, leword);
  if (ht == FrenchIndex) {
    StreamPrintf(StreamSuggFrenchInfl, "%s.%s/\n", leword, lefeatures);
  } else {
    StreamPrintf(StreamSuggEnglishInfl, "%s.%s/\n", leword, lefeatures);
  }
  AlgMorphRoot(leword, features, gender, pos, lang);
  if (NewInflections == NULL) {
    if (ht == FrenchIndex) StreamPuts("|\n", StreamSuggFrenchInfl);
    else StreamPuts("|\n", StreamSuggEnglishInfl);
    NewInflections = WordCreate(word, lefeatures, NewInflections, ht);
  }
  le = LexEntryCreate(leword, lefeatures, ht);
  le->infl = NewInflections;
  IndexEntryIndex(le, ht);
#ifdef maxchecking
  NewInflections = NULL;
#endif
  return(le);
}

void AlgMorphGetRoot(char *word, int gender, int number, int pos, int lang,
                     /* RESULT */ char *root)
{
  if (lang == F_FRENCH) {
    if (pos == F_VERB) AlgMorphGetFrenchVerbRoot(word, root);
    else if (pos == F_ADJECTIVE) AlgMorphGetFrenchAdjNounRoot(word, root);
    else if (pos == F_NOUN) AlgMorphGetFrenchAdjNounRoot(word, root);
    else StringCpy(root, word, PHRASELEN);
  } else if (lang == F_ENGLISH) {
    if (pos == F_VERB) AlgMorphGetEnglishVerbRoot(word, root);
    else if (pos == F_NOUN) AlgMorphGetEnglishNounRoot(word, root);
    else StringCpy(root, word, PHRASELEN);
  } else {
    Dbg(DBGGEN, DBGBAD, "AlgMorphGetWordRoot");
    StringCpy(root, word, PHRASELEN);
  }
}

int AlgMorphMap(char *in, int len, char *from, char *to, char *out)
{
  int	lenfrom;
  lenfrom = strlen(from);
  if (streq(&in[len-lenfrom], from)) {
    StringCpy(out, in, PHRASELEN);
    out[len-lenfrom] = TERM;
    StringCat(out, to, PHRASELEN);
    return(1);
  } else return(0);
}

void AlgMorphGetEnglishVerbRoot(char *infl, /* RESULT */ char *root)
{
  int len;
  len = strlen(infl);
  if (AlgMorphMap(infl, len, "ies", "y", root)) return;
  if (AlgMorphMap(infl, len, "ing", "", root)) return;
  if (AlgMorphMap(infl, len, "ed", "", root)) return;
  if (AlgMorphMap(infl, len, "es", "e", root)) return;
  if (AlgMorphMap(infl, len, "s", "", root)) return;
  StringCpy(root, infl, PHRASELEN);
}

void AlgMorphGetEnglishNounRoot(char *infl, /* RESULT */ char *root)
{
  int len;
  len = strlen(infl);
  if (AlgMorphMap(infl, len, "ies", "y", root)) return;
  if (AlgMorphMap(infl, len, "ss", "ss", root)) return;
  if (AlgMorphMap(infl, len, "es", "e", root)) return;
  if (AlgMorphMap(infl, len, "s", "", root)) return;
  StringCpy(root, infl, PHRASELEN);
}

void AlgMorphGetFrenchAdjNounRoot(char *infl, /* RESULT */ char *root)
{
  int len;
  len = strlen(infl);
  if (AlgMorphMap(infl, len, "ienne", "ien", root)) return;
  if (AlgMorphMap(infl, len, "iennes", "ien", root)) return;
  if (AlgMorphMap(infl, len, "ière", "ier", root)) return;
  if (AlgMorphMap(infl, len, "ières", "ier", root)) return;
  if (AlgMorphMap(infl, len, "ives", "if", root)) return;
  if (AlgMorphMap(infl, len, "ive", "if", root)) return;
/* Counterexamples: artificielle, femelle.
  if (AlgMorphMap(infl, len, "elle", "eau", root)) return;
  if (AlgMorphMap(infl, len, "elles", "eau", root)) return;
*/
  if (AlgMorphMap(infl, len, "aine", "ain", root)) return;
  if (AlgMorphMap(infl, len, "aines", "ain", root)) return;
  if (AlgMorphMap(infl, len, "ales", "al", root)) return;
  if (AlgMorphMap(infl, len, "ale", "al", root)) return;
  if (AlgMorphMap(infl, len, "ée", "é", root)) return;
  if (AlgMorphMap(infl, len, "ées", "ée", root)) return;
  if (AlgMorphMap(infl, len, "sie", "sie", root)) return;
  if (AlgMorphMap(infl, len, "gnes", "gne", root)) return;
  if (AlgMorphMap(infl, len, "gne", "gne", root)) return;
  if (AlgMorphMap(infl, len, "ques", "que", root)) return;
  if (AlgMorphMap(infl, len, "que", "que", root)) return;
  if (AlgMorphMap(infl, len, "trice", "teur", root)) return;
  if (AlgMorphMap(infl, len, "ette", "et", root)) return;
  if (AlgMorphMap(infl, len, "euse", "eur", root)) return;
  if (AlgMorphMap(infl, len, "ète", "et", root)) return;
  if (AlgMorphMap(infl, len, "oux", "ou", root)) return;
  if (AlgMorphMap(infl, len, "aux", "ail", root)) return;
  if (AlgMorphMap(infl, len, "es", "", root)) return;
  if (AlgMorphMap(infl, len, "s", "", root)) return;
  StringCpy(root, infl, PHRASELEN);
}

void AlgMorphGetFrenchVerbRoot(char *infl, /* RESULT */ char *root)
{
  int len;
  len = strlen(infl);
  if (AlgMorphMap(infl, len, "aient", "er", root)) return;
  if (AlgMorphMap(infl, len, "èrent", "er", root)) return;
  if (AlgMorphMap(infl, len, "erait", "er", root)) return;
  if (AlgMorphMap(infl, len, "erons", "er", root)) return;
  if (AlgMorphMap(infl, len, "eront", "er", root)) return;
  if (AlgMorphMap(infl, len, "erais", "er", root)) return;
  if (AlgMorphMap(infl, len, "erait", "er", root)) return;
  if (AlgMorphMap(infl, len, "erions", "er", root)) return;
  if (AlgMorphMap(infl, len, "eriez", "er", root)) return;
  if (AlgMorphMap(infl, len, "eraient", "er", root)) return;
  if (AlgMorphMap(infl, len, "asses", "er", root)) return;
  if (AlgMorphMap(infl, len, "assions", "er", root)) return;
  if (AlgMorphMap(infl, len, "assiez", "er", root)) return;
  if (AlgMorphMap(infl, len, "assent", "er", root)) return;
  if (AlgMorphMap(infl, len, "issons", "ir", root)) return;
  if (AlgMorphMap(infl, len, "issez", "ir", root)) return;
  if (AlgMorphMap(infl, len, "issent", "ir", root)) return;
  if (AlgMorphMap(infl, len, "issais", "ir", root)) return;
  if (AlgMorphMap(infl, len, "issait", "ir", root)) return;
  if (AlgMorphMap(infl, len, "issions", "ir", root)) return;
  if (AlgMorphMap(infl, len, "issiez", "ir", root)) return;
  if (AlgMorphMap(infl, len, "issaient", "ir", root)) return;
  if (AlgMorphMap(infl, len, "irent", "ir", root)) return;
  if (AlgMorphMap(infl, len, "irons", "ir", root)) return;
  if (AlgMorphMap(infl, len, "iront", "ir", root)) return;
  if (AlgMorphMap(infl, len, "irais", "ir", root)) return;
  if (AlgMorphMap(infl, len, "irait", "ir", root)) return;
  if (AlgMorphMap(infl, len, "irions", "ir", root)) return;
  if (AlgMorphMap(infl, len, "iriez", "ir", root)) return;
  if (AlgMorphMap(infl, len, "iraient", "ir", root)) return;
  if (AlgMorphMap(infl, len, "isses", "ir", root)) return;
  /* length 4 */
  if (AlgMorphMap(infl, len, "irai", "ir", root)) return;
  if (AlgMorphMap(infl, len, "iras", "ir", root)) return;
  if (AlgMorphMap(infl, len, "irez", "ir", root)) return;
  if (AlgMorphMap(infl, len, "isse", "ir", root)) return;
  if (AlgMorphMap(infl, len, "ions", "er", root)) return;
  if (AlgMorphMap(infl, len, "âmes", "er", root)) return;
  if (AlgMorphMap(infl, len, "âtes", "er", root)) return;
  if (AlgMorphMap(infl, len, "eras", "er", root)) return;
  if (AlgMorphMap(infl, len, "erez", "er", root)) return;
  if (AlgMorphMap(infl, len, "asse", "er", root)) return;
  if (AlgMorphMap(infl, len, "îmes", "ir", root)) return;
  if (AlgMorphMap(infl, len, "îtes", "ir", root)) return;
  /* length 3 */
  if (AlgMorphMap(infl, len, "era", "er", root)) return;
  if (AlgMorphMap(infl, len, "ons", "er", root)) return;
  if (AlgMorphMap(infl, len, "ent", "er", root)) return;
  if (AlgMorphMap(infl, len, "ais", "er", root)) return;
  if (AlgMorphMap(infl, len, "ait", "er", root)) return;
  if (AlgMorphMap(infl, len, "iez", "er", root)) return;
  if (AlgMorphMap(infl, len, "ira", "ir", root)) return;
  /* length 2 */
  if (AlgMorphMap(infl, len, "es", "er", root)) return;
  if (AlgMorphMap(infl, len, "ez", "er", root)) return;
  if (AlgMorphMap(infl, len, "ai", "er", root)) return;
  if (AlgMorphMap(infl, len, "as", "er", root)) return;
  if (AlgMorphMap(infl, len, "ât", "er", root)) return;
  if (AlgMorphMap(infl, len, "is", "ir", root)) return;
  if (AlgMorphMap(infl, len, "it", "ir", root)) return;
  if (AlgMorphMap(infl, len, "ît", "ir", root)) return;
  /* length 1 */
  if (AlgMorphMap(infl, len, "e", "er", root)) return;
  if (AlgMorphMap(infl, len, "a", "er", root)) return;
  StringCpy(root, infl, PHRASELEN);
}

void AlgMorphRoot(char *root, char *features, int gender, int pos, int lang)
{
  Dbg(DBGGEN, DBGDETAIL, "AlgMorphRoot: <%s>.<%s>", root, features);
  if (lang == F_FRENCH) {
    if (pos == F_VERB) {
      AlgMorphFrenchVerbPrint(StreamSuggFrenchInfl, root);
    } else if (pos == F_ADJECTIVE) {
      AlgMorphFrenchAdjNounPrint(StreamSuggFrenchInfl, root, pos);
    } else if (pos == F_NOUN) {
      if (StringIn(F_MASC_FEM, features)) {
        AlgMorphFrenchAdjNounPrint(StreamSuggFrenchInfl, root, pos);
      } else {
        AlgMorphFrenchNounPrint(StreamSuggFrenchInfl, root, gender);
      }
    }
  } else if (lang == F_ENGLISH) {
    if (pos == F_VERB) {
      AlgMorphEnglishVerbPrint(StreamSuggEnglishInfl, root);
    } else if (pos == F_NOUN) {
      AlgMorphEnglishNounPrint(StreamSuggEnglishInfl, root);
    }
  } else {
    Dbg(DBGGEN, DBGBAD, "AlgMorphRoot");
  }
}

void AlgMorphTool()
{
  char buf[PHRASELEN];
  printf("Welcome to the algorithmic morphology tool.\n");
  while (StreamAskStd("Enter language (y/z): ", PHRASELEN, buf)) {
    if (buf[0] == 'y') AlgMorphFrench();
    else AlgMorphEnglish();
  }
} 

void AlgMorphFrench()
{
  char buf[PHRASELEN];
  while (StreamAskStd("Enter part of speech (V/A/N): ", PHRASELEN, buf)) {
    if (buf[0] == 'V') AlgMorphFrenchVerbUi(stdout);
    else if (buf[0] == 'A') AlgMorphFrenchAdjUi(stdout);
    else if (buf[0] == 'N') AlgMorphFrenchNounUi(stdout);
  }
}
    
void AlgMorphFrenchVerbUi(FILE *stream)
{
  char inf[PHRASELEN];
  while (StreamAskStd("Enter french infinitive: ", PHRASELEN, inf)) {
    AlgMorphFrenchVerbPrint(stream, inf);
  } 
}

int AlgMorphGuessConj(char *inf, char *stem_result)
{
  int len;
  len = strlen(inf);
  StringCpy(stem_result, inf, PHRASELEN);
  if (len < 3) return(0);
  stem_result[len-2] = TERM;
  if (streq(&inf[len-2], "er")) {
    if (inf[len-4] == 'é' && !LexEntryIsFrenchVowel(inf[len-3])) return(6);
    if (inf[len-5] == 'é' && (!LexEntryIsFrenchVowel(inf[len-4])) &&
       !LexEntryIsFrenchVowel(inf[len-3])) return(6);
    if (inf[len-4] == 'e') {
      if (inf[len-3] == 'l' || inf[len-3] == 'r') return(4);
      if (!LexEntryIsFrenchVowel(inf[len-3])) return(5);
    }
    return(1);
  }
  if (streq(&inf[len-2], "ir")) return(2);
  if (streq(&inf[len-2], "re")) return(41);
  return(0);
}
   
void AlgMorphFrenchVerbPrint(FILE *stream, char *inf)
{
  char stem[PHRASELEN];
/*
  char buf[PHRASELEN];
*/
  int conj;
  conj = AlgMorphGuessConj(inf, stem);
/*
  StreamPrintf(stream,
"Automatically inferred conjugations:\n");
  StreamPrintf(stream,
"   1 (donner), +3a (placer), +3b (bouger), 7 (=1, prier), 8 (noyer).\n");
  StreamPrintf(stream,
"Required input conj numbers: 4 (appeler), 5 (acheter),\n");
  StreamPrintf(stream,
"   6 (céder), 2 (finir), 41 (vendre).\n");
  StreamPrintf(stream,
"Input conj number of %s is guessed to be %d\n", inf, conj);  
  if (!StreamAskStd("enter override conjugation: ", PHRASELEN, buf)) return;
  if (buf[0]) conj = atoi(buf);
*/
/*
  StreamPrintf(stream, "using conjugation %d\n", conj);
  StreamPrintf(stream, "%s.Vy/\n", inf);
*/
  AlgMorphFrenchVerbPrint2(stream, stem, conj, F_INFINITIVE, F_NULL, F_NULL,
                           F_NULL, F_NULL);
  StreamPutc(NEWLINE, stream);
  AlgMorphFrenchVerbPrint1(stream, stem, conj, F_PRESENT, F_INDICATIVE); 
  AlgMorphFrenchVerbPrint1(stream, stem, conj, F_IMPERFECT, F_INDICATIVE); 
  AlgMorphFrenchVerbPrint1(stream, stem, conj, F_PASSE_SIMPLE, F_INDICATIVE); 
  AlgMorphFrenchVerbPrint1(stream, stem, conj, F_FUTURE, F_INDICATIVE);
  AlgMorphFrenchVerbPrint1(stream, stem, conj, F_CONDITIONAL, F_INDICATIVE);  
  AlgMorphFrenchVerbPrint1(stream, stem, conj, F_PRESENT, F_SUBJUNCTIVE); 
  AlgMorphFrenchVerbPrint1(stream, stem, conj, F_IMPERFECT, F_SUBJUNCTIVE); 
  AlgMorphFrenchVerbPrint2(stream, stem, conj, F_PAST_PARTICIPLE, F_NULL,
                           F_NULL, F_SINGULAR, F_MASCULINE);
  AlgMorphFrenchVerbPrint2(stream, stem, conj, F_PAST_PARTICIPLE, F_NULL,
                           F_NULL, F_SINGULAR, F_FEMININE);
  AlgMorphFrenchVerbPrint2(stream, stem, conj, F_PAST_PARTICIPLE, F_NULL,
                           F_NULL, F_PLURAL, F_MASCULINE);
  AlgMorphFrenchVerbPrint2(stream, stem, conj, F_PAST_PARTICIPLE, F_NULL,
                           F_NULL, F_PLURAL, F_FEMININE);
  StreamPutc(NEWLINE, stream);
  AlgMorphFrenchVerbPrint1(stream, stem, conj, F_PRESENT, F_IMPERATIVE); 
  AlgMorphFrenchVerbPrint2(stream, stem, conj, F_PRESENT_PARTICIPLE, F_NULL,
                           F_NULL, F_NULL, F_NULL);
  StreamPutc(NEWLINE, stream);
  StreamPrintf(stream, "|\n");
}

void AlgMorphFrenchVerbPrint1(FILE *stream, char *stem, int conj, int tense,
                              int mood)
{
  AlgMorphFrenchVerbPrint2(stream, stem, conj, tense, mood, F_FIRST_PERSON,
                           F_SINGULAR, F_NULL);
  AlgMorphFrenchVerbPrint2(stream, stem, conj, tense, mood, F_SECOND_PERSON,
                           F_SINGULAR, F_NULL);
  AlgMorphFrenchVerbPrint2(stream, stem, conj, tense, mood, F_THIRD_PERSON,
                           F_SINGULAR, F_NULL);
  AlgMorphFrenchVerbPrint2(stream, stem, conj, tense, mood, F_FIRST_PERSON,
                           F_PLURAL, F_NULL);
  AlgMorphFrenchVerbPrint2(stream, stem, conj, tense, mood, F_SECOND_PERSON,
                           F_PLURAL, F_NULL);
  AlgMorphFrenchVerbPrint2(stream, stem, conj, tense, mood, F_THIRD_PERSON,
                           F_PLURAL, F_NULL);
  StreamPutc(NEWLINE, stream);
}

void AlgMorphFrenchVerbPrint2(FILE *stream, char *stem, int conj, int tense,
                              int mood, int person, int number, int gender)
{
  char	word[PHRASELEN], features[FEATLEN];
  AlgMorphFrenchVerb(stem, conj, tense, mood, person, number, gender, word);
  if (word[0]) {
    StreamPuts(word, stream);
    StreamPutc('.', stream);
    features[0] = TERM;
    if (tense != F_NULL) StringAppendChar(features, FEATLEN, tense);
    if (mood != F_NULL) StringAppendChar(features, FEATLEN, mood);
    if (person != F_NULL) StringAppendChar(features, FEATLEN, person);
    if (number != F_NULL) StringAppendChar(features, FEATLEN, number);
    if (gender != F_NULL) StringAppendChar(features, FEATLEN, gender);
    StreamPuts(features, stream);
    StreamPutc('/', stream);
    StringAppendChar(features, FEATLEN, F_VERB);
    StringAppendChar(features, FEATLEN, F_FRENCH);
    NewInflections = WordCreate(word, features, NewInflections, FrenchIndex);
  }
}

void AlgMorphFrenchVerb(char *stem, int conj, int tense, int mood,
                        int person, int number, int gender, char *result)
{
  int len, is_g, is_c, is_y, conj458;
  char stem3[PHRASELEN], stem458[PHRASELEN], stem6[PHRASELEN];
  result[0] = TERM;
  len = strlen(stem);
  if (len < 3) {
    StringCpy(result, stem, PHRASELEN);
    return;
  }
  is_g = (stem[len-1] == 'g');
  is_c = (stem[len-1] == 'c');
  is_y = (stem[len-1] == 'y') && (stem[len-2] != 'a');
  StringCpy(stem3, stem, PHRASELEN);
  if (is_c) stem3[len-1] = 'ç';
  StringCpy(stem458, stem, PHRASELEN);
  if (conj == 4) {
    stem458[len] = stem458[len-1];
    stem458[len+1] = TERM;
    conj458 = 1;
  } else if (conj == 5) {
    if (stem458[len-2] == 'e') stem458[len-2] = 'è';
    conj458 = 1;
  } else if (is_y) {
    stem458[len-1] = 'i';
    conj458 = 1;
  } else conj458 = 0;
  StringCpy(stem6, stem, PHRASELEN);
  if (stem6[len-2] == 'é') stem6[len-2] = 'è';
  else if (stem6[len-3] == 'é') stem6[len-3] = 'è';
  switch (tense) {
    case F_PRESENT:
      if (mood == F_INDICATIVE) {
        if (person == F_FIRST_PERSON && number == F_SINGULAR) {
          if (conj == 41) sprintf(result, "%ss", stem);
          else if (conj == 2) sprintf(result, "%sis", stem);
          else if (conj458) sprintf(result, "%se", stem458);
          else if (conj == 6) sprintf(result, "%se", stem6);
          else sprintf(result, "%se", stem);
        } else if (person == F_SECOND_PERSON && number == F_SINGULAR) {
          if (conj == 41) sprintf(result, "%ss", stem);
          else if (conj == 2) sprintf(result, "%sis", stem);
          else if (conj458) sprintf(result, "%ses", stem458);
          else if (conj == 6) sprintf(result, "%ses", stem6);
          else sprintf(result, "%ses", stem);
        } else if (person == F_THIRD_PERSON && number == F_SINGULAR) {
          if (conj == 41) sprintf(result, "%s", stem);
          else if (conj == 2) sprintf(result, "%sit", stem);
          else if (conj458) sprintf(result, "%se", stem458);
          else if (conj == 6) sprintf(result, "%se", stem6);
          else sprintf(result, "%se", stem);
        } else if (person == F_FIRST_PERSON && number == F_PLURAL) {
          if (is_g) sprintf(result, "%seons", stem);
          else if (conj == 2) sprintf(result, "%sissons", stem);
          else sprintf(result, "%sons", stem3);
        } else if (person == F_SECOND_PERSON && number == F_PLURAL) {
          if (conj == 2) sprintf(result, "%sissez", stem);
          else sprintf(result, "%sez", stem);
        } else if (person == F_THIRD_PERSON && number == F_PLURAL) {
          if (conj458) sprintf(result, "%sent", stem458);
          else if (conj == 2) sprintf(result, "%sissent", stem);
          else if (conj == 6) sprintf(result, "%sent", stem6);
          else sprintf(result, "%sent", stem);
        }
      } else if (mood == F_SUBJUNCTIVE) {
        if (person == F_FIRST_PERSON && number == F_SINGULAR) {
          if (conj458) sprintf(result, "%se", stem458);
          else if (conj == 2) sprintf(result, "%sisse", stem);
          else if (conj == 6) sprintf(result, "%se", stem6);
          else sprintf(result, "%se", stem);
        } else if (person == F_SECOND_PERSON && number == F_SINGULAR) {
          if (conj458) sprintf(result, "%ses", stem458);
          else if (conj == 2) sprintf(result, "%sisses", stem);
          else if (conj == 6) sprintf(result, "%ses", stem6);
          else sprintf(result, "%ses", stem);
        } else if (person == F_THIRD_PERSON && number == F_SINGULAR) {
          if (conj458) sprintf(result, "%se", stem458);
          else if (conj == 2) sprintf(result, "%sisse", stem);
          else if (conj == 6) sprintf(result, "%se", stem6);
          else sprintf(result, "%se", stem);
        } else if (person == F_FIRST_PERSON && number == F_PLURAL) {
          if (conj == 2) sprintf(result, "%sissions", stem);
          else sprintf(result, "%sions", stem);
        } else if (person == F_SECOND_PERSON && number == F_PLURAL) {
          if (conj == 2) sprintf(result, "%sissiez", stem);
          else sprintf(result, "%siez", stem);
        } else if (person == F_THIRD_PERSON && number == F_PLURAL) {
          if (conj458) sprintf(result, "%sent", stem458);
          else if (conj == 2) sprintf(result, "%sissent", stem);
          else if (conj == 6) sprintf(result, "%sent", stem6);
          else sprintf(result, "%sent", stem);
        }
      } else if (mood == F_IMPERATIVE) {
        if (person == F_SECOND_PERSON && number == F_SINGULAR) {
          if (conj == 41) sprintf(result, "%ss", stem);
          else if (conj == 2) sprintf(result, "%sis", stem);
          else if (conj458) sprintf(result, "%se", stem458);
          else if (conj == 6) sprintf(result, "%se", stem6);
          else sprintf(result, "%se", stem);
        } else if (person == F_FIRST_PERSON && number == F_PLURAL) {
          if (is_g) sprintf(result, "%seons", stem);
          else if (conj == 2) sprintf(result, "%sissons", stem);
          else sprintf(result, "%sons", stem3);
        } else if (person == F_SECOND_PERSON && number == F_PLURAL) {
          if (conj == 2) sprintf(result, "%sissez", stem);
          else sprintf(result, "%sez", stem);
        }
      }
      break;
    case F_IMPERFECT:
      if (mood == F_INDICATIVE) {
        if (person == F_FIRST_PERSON && number == F_SINGULAR) {
          if (conj == 2) sprintf(result, "%sissais", stem);
          else if (is_g) sprintf(result, "%seais", stem);
          else sprintf(result, "%sais", stem3);
        } else if (person == F_SECOND_PERSON && number == F_SINGULAR) {
          if (conj == 2) sprintf(result, "%sissais", stem);
          else if (is_g) sprintf(result, "%seais", stem);
          else sprintf(result, "%sais", stem3);
        } else if (person == F_THIRD_PERSON && number == F_SINGULAR) {
          if (conj == 2) sprintf(result, "%sissait", stem);
          else if (is_g) sprintf(result, "%seait", stem);
          else sprintf(result, "%sait", stem3);
        } else if (person == F_FIRST_PERSON && number == F_PLURAL) {
          if (conj == 2) sprintf(result, "%sissions", stem);
          else sprintf(result, "%sions", stem);
        } else if (person == F_SECOND_PERSON && number == F_PLURAL) {
          if (conj == 2) sprintf(result, "%sissiez", stem);
          else sprintf(result, "%siez", stem);
        } else if (person == F_THIRD_PERSON && number == F_PLURAL) {
          if (conj == 2) sprintf(result, "%sissaient", stem);
          else if (is_g) sprintf(result, "%seaient", stem);
          else sprintf(result, "%saient", stem3);
        }
      } else if (mood == F_SUBJUNCTIVE) {
        if (person == F_FIRST_PERSON && number == F_SINGULAR) {
          if (conj == 41 || conj == 2) sprintf(result, "%sisse", stem);
          else if (is_g) sprintf(result, "%seasse", stem);
          else sprintf(result, "%sasse", stem3);
        } else if (person == F_SECOND_PERSON && number == F_SINGULAR) {
          if (conj == 41 || conj == 2) sprintf(result, "%sisses", stem);
          else if (is_g) sprintf(result, "%seasses", stem);
          else sprintf(result, "%sasses", stem3);
        } else if (person == F_THIRD_PERSON && number == F_SINGULAR) {
          if (conj == 41 || conj == 2) sprintf(result, "%sît", stem);
          else if (is_g) sprintf(result, "%seât", stem);
          else sprintf(result, "%sât", stem3);
        } else if (person == F_FIRST_PERSON && number == F_PLURAL) {
          if (conj == 41 || conj == 2) sprintf(result, "%sissions", stem);
          else if (is_g) sprintf(result, "%seassions", stem);
          else sprintf(result, "%sassions", stem3);
        } else if (person == F_SECOND_PERSON && number == F_PLURAL) {
          if (conj == 41 || conj == 2) sprintf(result, "%sissiez", stem);
          else if (is_g) sprintf(result, "%seassiez", stem);
          else sprintf(result, "%sassiez", stem3);
        } else if (person == F_THIRD_PERSON && number == F_PLURAL) {
          if (conj == 41 || conj == 2) sprintf(result, "%sissent", stem);
          else if (is_g) sprintf(result, "%seassent", stem);
          else sprintf(result, "%sassent", stem3);
        }
      }
      break;
    case F_PASSE_SIMPLE:
      if (mood == F_INDICATIVE) {
        if (person == F_FIRST_PERSON && number == F_SINGULAR) {
          if (conj == 41 || conj == 2) sprintf(result, "%sis", stem);
          else if (is_g) sprintf(result, "%seai", stem);
          else sprintf(result, "%sai", stem3);
        } else if (person == F_SECOND_PERSON && number == F_SINGULAR) {
          if (conj == 41 || conj == 2) sprintf(result, "%sis", stem);
          else if (is_g) sprintf(result, "%seas", stem);
          else sprintf(result, "%sas", stem3);
        } else if (person == F_THIRD_PERSON && number == F_SINGULAR) {
          if (conj == 41 || conj == 2) sprintf(result, "%sit", stem);
          else if (is_g) sprintf(result, "%sea", stem);
          else sprintf(result, "%sa", stem3);
        } else if (person == F_FIRST_PERSON && number == F_PLURAL) {
          if (conj == 41 || conj == 2) sprintf(result, "%sîmes", stem);
          else if (is_g) sprintf(result, "%seâmes", stem);
          else sprintf(result, "%sâmes", stem3);
        } else if (person == F_SECOND_PERSON && number == F_PLURAL) {
          if (conj == 41 || conj == 2) sprintf(result, "%sîtes", stem);
          else if (is_g) sprintf(result, "%seâtes", stem);
          else sprintf(result, "%sâtes", stem3);
        } else if (person == F_THIRD_PERSON && number == F_PLURAL) {
          if (conj == 41 || conj == 2) sprintf(result, "%sirent", stem);
          else sprintf(result, "%sèrent", stem);
        }
      }
      break;
    case F_FUTURE:
      if (mood == F_INDICATIVE) {
        if (person == F_FIRST_PERSON && number == F_SINGULAR) {
          if (conj == 41) sprintf(result, "%srai", stem);
          else if (conj == 2) sprintf(result, "%sirai", stem);
          else if (conj458) sprintf(result, "%serai", stem458);
          else sprintf(result, "%serai", stem);
        } else if (person == F_SECOND_PERSON && number == F_SINGULAR) {
          if (conj == 41) sprintf(result, "%sras", stem);
          else if (conj == 2) sprintf(result, "%siras", stem);
          else if (conj458) sprintf(result, "%seras", stem458);
          else sprintf(result, "%seras", stem);
        } else if (person == F_THIRD_PERSON && number == F_SINGULAR) {
          if (conj == 41) sprintf(result, "%sra", stem);
          else if (conj == 2) sprintf(result, "%sira", stem);
          else if (conj458) sprintf(result, "%sera", stem458);
          else sprintf(result, "%sera", stem);
        } else if (person == F_FIRST_PERSON && number == F_PLURAL) {
          if (conj == 41) sprintf(result, "%srons", stem);
          else if (conj == 2) sprintf(result, "%sirons", stem);
          else if (conj458) sprintf(result, "%serons", stem458);
          else sprintf(result, "%serons", stem);
        } else if (person == F_SECOND_PERSON && number == F_PLURAL) {
          if (conj == 41) sprintf(result, "%srez", stem);
          else if (conj == 2) sprintf(result, "%sirez", stem);
          else if (conj458) sprintf(result, "%serez", stem458);
          else sprintf(result, "%serez", stem);
        } else if (person == F_THIRD_PERSON && number == F_PLURAL) {
          if (conj == 41) sprintf(result, "%sront", stem);
          else if (conj == 2) sprintf(result, "%siront", stem);
          else if (conj458) sprintf(result, "%seront", stem458);
          else sprintf(result, "%seront", stem);
        }
      }
      break;
    case F_CONDITIONAL:
      if (mood == F_INDICATIVE) {
        if (person == F_FIRST_PERSON && number == F_SINGULAR) {
          if (conj == 41) sprintf(result, "%srais", stem);
          else if (conj == 2) sprintf(result, "%sirais", stem);
          else if (conj458) sprintf(result, "%serais", stem458);
          else sprintf(result, "%serais", stem);
        } else if (person == F_SECOND_PERSON && number == F_SINGULAR) {
          if (conj == 41) sprintf(result, "%srais", stem);
          else if (conj == 2) sprintf(result, "%sirais", stem);
          else if (conj458) sprintf(result, "%serais", stem458);
          else sprintf(result, "%serais", stem);
        } else if (person == F_THIRD_PERSON && number == F_SINGULAR) {
          if (conj == 41) sprintf(result, "%srait", stem);
          else if (conj == 2) sprintf(result, "%sirait", stem);
          else if (conj458) sprintf(result, "%serait", stem458);
          else sprintf(result, "%serait", stem);
        } else if (person == F_FIRST_PERSON && number == F_PLURAL) {
          if (conj == 41) sprintf(result, "%srions", stem);
          else if (conj == 2) sprintf(result, "%sirions", stem);
          else if (conj458) sprintf(result, "%serions", stem458);
          else sprintf(result, "%serions", stem);
        } else if (person == F_SECOND_PERSON && number == F_PLURAL) {
          if (conj == 41) sprintf(result, "%sriez", stem);
          else if (conj == 2) sprintf(result, "%siriez", stem);
          else if (conj458) sprintf(result, "%seriez", stem458);
          else sprintf(result, "%seriez", stem);
        } else if (person == F_THIRD_PERSON && number == F_PLURAL) {
          if (conj == 41) sprintf(result, "%sraient", stem);
          else if (conj == 2) sprintf(result, "%siraient", stem);
          else if (conj458) sprintf(result, "%seraient", stem458);
          else sprintf(result, "%seraient", stem);
        }
      }
      break;
    case F_PAST_PARTICIPLE:
      if (number == F_SINGULAR && gender == F_MASCULINE) {
        if (conj == 41) sprintf(result, "%su", stem);
        else if (conj == 2) sprintf(result, "%si", stem);
        else sprintf(result, "%sé", stem);
      } else if (number == F_SINGULAR && gender == F_FEMININE) {
        if (conj == 41) sprintf(result, "%sue", stem);
        else if (conj == 2) sprintf(result, "%sie", stem);
        else sprintf(result, "%sée", stem);
      } else if (number == F_PLURAL && gender == F_MASCULINE) {
        if (conj == 41) sprintf(result, "%sus", stem);
        else if (conj == 2) sprintf(result, "%sis", stem);
        else sprintf(result, "%sés", stem);
      } else if (number == F_PLURAL && gender == F_FEMININE) {
        if (conj == 41) sprintf(result, "%sues", stem);
        else if (conj == 2) sprintf(result, "%sies", stem);
        else sprintf(result, "%sées", stem);
      }
      break;
    case F_PRESENT_PARTICIPLE:
      if (conj == 2) sprintf(result, "%sissant", stem);
      else if (is_g) sprintf(result, "%seant", stem);
      else sprintf(result, "%sant", stem3);
      break;
    case F_INFINITIVE:
      if (conj == 41) sprintf(result, "%sre", stem);
      else if (conj == 2) sprintf(result, "%sir", stem);
      else sprintf(result, "%ser", stem);
      break;
    }
}

void AlgMorphFrenchAdjUi(FILE *stream)
{
  char stem[PHRASELEN];
  while (StreamAskStd("Enter french masculine singular adjective: ",
                      PHRASELEN, stem)) {
    AlgMorphFrenchAdjNounPrint(stream, stem, F_ADJECTIVE);
  } 
}

void AlgMorphFrenchAdjNounPrint(FILE *stream, char *stem, int pos)
{
  int conj;
/*
  char buf[PHRASELEN];
  AlgMorphAdjNounConjMsg(stream);
  if (!StreamAskStd("enter conjugation: ", PHRASELEN, buf)) return;
  if (buf[0]) conj = atoi(buf);
  else conj = 0;
  StreamPrintf(stream, "using conjugation %d\n", conj);
*/
  conj = 0;
/*
  StreamPrintf(stream, "%s.%cy/\n", stem, pos);
*/
  AlgMorphFrenchAdjNounPrint1(stream, stem, conj, F_MASCULINE, F_SINGULAR, pos);
  AlgMorphFrenchAdjNounPrint1(stream, stem, conj, F_MASCULINE, F_PLURAL, pos);
  AlgMorphFrenchAdjNounPrint1(stream, stem, conj, F_FEMININE, F_SINGULAR, pos);
  AlgMorphFrenchAdjNounPrint1(stream, stem, conj, F_FEMININE, F_PLURAL, pos);
  StreamPrintf(stream, "\n|\n");
} 

void AlgMorphFrenchAdjNounPrint1(FILE *stream, char *stem, int conj,
                                 int gender, int number, int pos)
{
  char	word[PHRASELEN], features[FEATLEN];
  AlgMorphFrenchAdjNoun(stem, conj, gender, number, word);
  if (word[0]) {
    StreamPrintf(stream, "%s.%c%c/", word, gender, number);
    sprintf(features, "%c%c%c%c", gender, number, pos, F_FRENCH);
    NewInflections = WordCreate(word, features, NewInflections, FrenchIndex);
  }
}

void AlgMorphAdjNounConjMsg(FILE *stream)
{
  StreamPrintf(stream, "Possible 'conjugations':\n");
  StreamPrintf(stream, "0 attempt to infer automatically\n");
  StreamPrintf(stream, "1 default: grand grands grande grandes\n");
  StreamPrintf(stream, "2 sometimes inferred: complet complète\n");
  StreamPrintf(stream,
"3 sometimes inferred: cadet cadette (double the final consonant)\n");
  StreamPrintf(stream, "4 always inferred by 'eur': flatteur flatteuse\n");
  StreamPrintf(stream, "5 never inferred: consolateur consolatrice\n");
  StreamPrintf(stream, "6 never inferred: bijou bijoux\n");
  StreamPrintf(stream, "7 never inferred: travail travaux\n");
}
 
void AlgMorphFrenchAdjNoun(char *stem, int conj, int gender,
                           int number, /* RESULT */ char *result)
{
  int	len, is_g, is_e, is_s, is_x, is_al, is_eau;
  char	stem2[PHRASELEN], stem3[PHRASELEN], stem4[PHRASELEN], stem5[PHRASELEN];
  char	stem_x[PHRASELEN], stem_al[PHRASELEN], stem_aux[PHRASELEN];
  result[0] = TERM;
  len = strlen(stem);
  if (len < 3) {
    StringCpy(result, stem, PHRASELEN);
    return;
  }
  is_g = (stem[len-1] == 'g');
  is_e = (stem[len-1] == 'e');
  is_s = (stem[len-1] == 's');
  is_x = (stem[len-1] == 'x') || (stem[len-1] == 'z');
  is_al = streq(&stem[len-2], "al");
  is_eau = (conj == 6) || streq(&stem[len-3], "eau");
  if (conj == 0 && (streq(&stem[len-2], "et") || streq(&stem[len-2], "er")))
    conj = 2;
  if (conj == 2) {
    StringCpy(stem2, stem, PHRASELEN);
    stem2[len-2] = 'è';
  }
  if (conj == 0 &&
             (streq(&stem[len-2], "el") || streq(&stem[len-3], "eil") ||
              streq(&stem[len-2], "en") || streq(&stem[len-2], "et") ||
              streq(&stem[len-2], "on")))
    conj = 3;
  if (conj == 3) sprintf(stem3, "%s%c", stem, stem[len-1]);
  if (conj == 0 && streq(&stem[len-3], "eur"))
    conj = 4;
  if (conj == 4) {
    StringCpy(stem4, stem, PHRASELEN);
    stem4[len-1] = 's';
  } else if (conj == 5) {
    StringCpy(stem5, stem, PHRASELEN);
    StringCpy(&stem5[len-3], "ric", PHRASELEN);
  }
  if (is_x) {
  /* There are several exceptions not captured here: roux doux ... */
    StringCpy(stem_x, stem, PHRASELEN);
    stem_x[len-1] = 's';
  }
  if (is_al) {
    StringCpy(stem_al, stem, PHRASELEN);
    StringCpy(&stem_al[len-1], "ux", PHRASELEN);
  }
  if (conj == 7) {
    StringCpy(stem_aux, stem, PHRASELEN);
    StringCpy(&stem_aux[len-2], "ux", PHRASELEN);
  }
  if (gender == F_MASCULINE && number == F_SINGULAR) {
    StringCpy(result, stem, PHRASELEN);
  } else if (gender == F_MASCULINE && number == F_PLURAL) {
    if (conj == 7) StringCpy(result, stem_aux, PHRASELEN);
    else if (is_s || is_x) StringCpy(result, stem, PHRASELEN);
    else if (is_al) StringCpy(result, stem_al, PHRASELEN);
    else if (is_eau) sprintf(result, "%sx", stem);
    else sprintf(result, "%ss", stem);
  } else if (gender == F_FEMININE && number == F_SINGULAR) {
    if (conj == 1) sprintf(result, "%se", stem);
    else if (conj == 2) sprintf(result, "%se", stem2);
    else if (conj == 3) sprintf(result, "%se", stem3);
    else if (conj == 4) sprintf(result, "%se", stem4);
    else if (conj == 5) sprintf(result, "%se", stem5);
    else if (is_x) sprintf(result, "%se", stem_x);
    else if (is_g) sprintf(result, "%sue" ,stem);
    else if (is_e) StringCpy(result, stem, PHRASELEN);
    else sprintf(result, "%se", stem);
  } else if (gender == F_FEMININE && number == F_PLURAL) {
    if (conj == 1) sprintf(result, "%ses", stem);
    else if (conj == 2) sprintf(result, "%ses", stem2);
    else if (conj == 3) sprintf(result, "%ses", stem3);
    else if (conj == 4) sprintf(result, "%ses", stem4);
    else if (conj == 5) sprintf(result, "%ses", stem5);
    else if (conj == 7) StringCpy(result, stem_aux, PHRASELEN);
    else if (is_x) sprintf(result, "%ses", stem_x);
    else if (is_g) sprintf(result, "%sues", stem);
    else if (is_e) sprintf(result, "%ss", stem);
    else if (is_eau) sprintf(result, "%sx", stem);
    else sprintf(result, "%ses", stem);
  }
}

void AlgMorphFrenchNounUi(FILE *stream)
{
  char buf[PHRASELEN], stem[PHRASELEN];
  while (StreamAskStd("Enter french noun gender (M/F/both): ", PHRASELEN,
                      buf)) {
    if (!StreamAskStd("Enter french noun stem: ", PHRASELEN, stem)) continue;
    if (buf[0] != F_MASCULINE && buf[0] != F_FEMININE) 
      AlgMorphFrenchAdjNounPrint(stream, stem, F_NOUN);
    else AlgMorphFrenchNounPrint(stream, stem, buf[0]);
  } 
}

void AlgMorphFrenchNounPrint(FILE *stream, char *stem, int gender)
{
/*
  char buf[PHRASELEN];
*/
  char word[PHRASELEN], features[FEATLEN];
  int conj;
/*
  AlgMorphAdjNounConjMsg(stream);
  if (!StreamAskStd("enter conjugation: ", PHRASELEN, buf)) return;
  if (buf[0]) conj = atoi(buf);
  else conj = 0;
  StreamPrintf(stream, "using conjugation %d\n", conj);
*/
  conj = 0;
/*
  StreamPrintf(stream, "%s.N%cy/\n", stem, gender);
*/
  AlgMorphFrenchAdjNoun(stem, conj, F_MASCULINE, F_SINGULAR, word);
  if (word[0]) {
    StreamPrintf(stream, "%s.%c/", word, F_SINGULAR);
    sprintf(features, "%c%c%c%c", gender, F_SINGULAR, F_NOUN, F_FRENCH);
    NewInflections = WordCreate(word, features, NewInflections, FrenchIndex);
  }
  AlgMorphFrenchAdjNoun(stem, conj, F_MASCULINE, F_PLURAL, word);
  if (word[0]) {
    StreamPrintf(stream, "%s.%c/", word, F_PLURAL);
    sprintf(features, "%c%c%c%c", gender, F_PLURAL, F_NOUN, F_FRENCH);
    NewInflections = WordCreate(word, features, NewInflections, FrenchIndex);
  }
  StreamPrintf(stream, "\n|\n");
} 

void AlgMorphEnglish()
{
  char buf[PHRASELEN];
  while (StreamAskStd("Enter part of speech (V/N): ", PHRASELEN, buf)) {
    if (buf[0] == 'V') AlgMorphEnglishVerbUi(stdout);
    else if (buf[0] == 'N') AlgMorphEnglishNounUi(stdout);
  }
}
    
void AlgMorphEnglishVerbUi(FILE *stream)
{
  char inf[PHRASELEN];
  while (StreamAskStd("Enter english infinitive (without 'to'): ",
                      PHRASELEN, inf)) {
    AlgMorphEnglishVerbPrint(stream, inf);
  } 
}

void AlgMorphEnglishVerbPrint(FILE *stream, char *stem)
{
/*
  StreamPrintf(stream, "%s.Vz/\n", stem);
*/
  AlgMorphEnglishVerbPrint1(stream, stem, F_INFINITIVE, F_NULL, F_NULL, F_NULL);
  StreamPutc(NEWLINE, stream);
  AlgMorphEnglishVerbPrint1(stream, stem, F_PRESENT, F_INDICATIVE,
                            F_FIRST_PERSON, F_SINGULAR);
  AlgMorphEnglishVerbPrint1(stream, stem, F_PRESENT, F_INDICATIVE,
                            F_THIRD_PERSON, F_SINGULAR);
  AlgMorphEnglishVerbPrint1(stream, stem, F_PRESENT, F_INDICATIVE, F_NULL,
                            F_PLURAL);
  StreamPutc(NEWLINE, stream);
  AlgMorphEnglishVerbPrint1(stream, stem, F_IMPERFECT, F_NULL, F_NULL, F_NULL);
  AlgMorphEnglishVerbPrint1(stream, stem, F_PAST_PARTICIPLE, F_NULL, F_NULL,
                            F_NULL);
  AlgMorphEnglishVerbPrint1(stream, stem, F_PRESENT_PARTICIPLE, F_NULL, F_NULL,
                            F_NULL);
  StreamPutc(NEWLINE, stream);
  StreamPrintf(stream, "|\n");
}

void AlgMorphEnglishVerbPrint1(FILE *stream, char *stem, int tense,
                               int mood, int person, int number)
{
  char word[PHRASELEN], features[FEATLEN];
  AlgMorphEnglishVerb(stem, tense, mood, person, number, word);
  if (word[0]) {
    StreamPuts(word, stream);
    StreamPutc('.', stream);
    features[0] = TERM;
    if (tense != F_NULL) StringAppendChar(features, FEATLEN, tense);
    if (person != F_NULL) StringAppendChar(features, FEATLEN, person);
    if (number != F_NULL) StringAppendChar(features, FEATLEN, number);
    StreamPuts(features, stream);
    StreamPutc('/', stream);
    StringAppendChar(features, FEATLEN, F_VERB);
    StringAppendChar(features, FEATLEN, F_ENGLISH);
    NewInflections = WordCreate(word, features, NewInflections, EnglishIndex);
  }
}

void AlgMorphEnglishVerb(char *stem, int tense, int mood, int person,
                         int number, /* RESULT */ char *result)
{
  int len, is_es, is_ie, is_me, is_cd, is_c, is_lp, is_ms, is_e;
  char stem_ie[PHRASELEN], stem_me[PHRASELEN];
  int	rtn;
  rtn = 0;
  if (rtn) return;
  result[0] = TERM;
  if (rtn) return;
  len = strlen(stem);
  if (rtn) return;
  if (len < 3) {
    StringCpy(result, stem, PHRASELEN);
    return;
  }
  if (rtn) return;
  if ((is_ie = ((stem[len-1] == 'y') &&
                !LexEntryIsEnglishVowel(stem[len-2])))) {
    StringCpy(stem_ie, stem, PHRASELEN);
    StringCpy(&stem_ie[len-1], "ie", PHRASELEN);
  }
  if (rtn) return;
  if ((is_me = ((stem[len-1] == 'e') &&
                !LexEntryIsEnglishVowel(stem[len-2])))) {
    StringCpy(stem_me, stem, PHRASELEN);
    stem_me[len-1] = TERM;
  }
  if (rtn) return;
  is_cd = (!LexEntryIsEnglishVowel(stem[len-1])) &&
          LexEntryIsEnglishVowel(stem[len-2]) &&
          (!LexEntryIsEnglishVowel(stem[len-3]));
  is_c = stem[len-1] == 'c';
  if (rtn) return;
  is_lp = ((stem[len-1] == 'l') || (stem[len-1] == 'p')) &&
          LexEntryIsEnglishVowel(stem[len-2]) &&
          !LexEntryIsEnglishVowel(stem[len-3]);
  is_ms = streq(stem+1, "ie");
  is_e = (stem[len-1] == 'e');
  if (rtn) return;
  switch (tense) {
    case F_PRESENT:
      if (mood == F_INDICATIVE) {
        if (person == F_THIRD_PERSON && number == F_SINGULAR) {
          is_es = (stem[len-1] == 's') || (stem[len-1] == 'z') || 
                   streq(&stem[len-2], "sh") || streq(&stem[len-2], "ch");
  if (rtn) return;
          if (is_es) sprintf(result, "%ses", stem);
          else if (is_ie) sprintf(result, "%ss", stem_ie);
          else sprintf(result, "%ss", stem);
        } else {
          StringCpy(result, stem, PHRASELEN);
        }
      } else {
        StringCpy(result, stem, PHRASELEN);
      }
      break;
    case F_IMPERFECT:
    case F_PAST_PARTICIPLE:
      if (is_ie) sprintf(result, "%sd", stem_ie);
      else if (is_lp) sprintf(result, "@%sed", stem);
/* todo: British
      else if (is_lp) sprintf(result, "%s%ced.%cg/%sed", stem, stem[len-1],
                              tense, stem);
 */
      else if (is_c) sprintf(result, "%sked", stem);
      else if (is_me || is_e) sprintf(result, "%sd", stem);
      else if (is_cd) sprintf(result, "%s%ced", stem, stem[len-1]);
      else sprintf(result, "%sed", stem);
      break;
    case F_PRESENT_PARTICIPLE:
      if (is_ms) sprintf(result, "%cying", stem[0]);
      else if (is_me) sprintf(result, "%sing", stem_me);
      else if (is_lp) sprintf(result, "@%sing", stem);
/* todo: British
      else if (is_lp) sprintf(result, "%s%cing.eg/%sing", stem, stem[len-1],
                              stem);
 */
      else if (is_c) sprintf(result, "%sking", stem);
      else if (is_cd) sprintf(result, "%s%cing", stem, stem[len-1]);
      else sprintf(result, "%sing", stem);
      break;
    case F_INFINITIVE:
      StringCpy(result, stem, PHRASELEN);
      break;
    }
}

void AlgMorphEnglishNounUi(FILE *stream)
{
  char stem[PHRASELEN];
  while (StreamAskStd("Enter english noun: ", PHRASELEN, stem))
    AlgMorphEnglishNounPrint(stream, stem);
}

void AlgMorphEnglishNounPrint(FILE *stream, char *stem)
{
  char plural[PHRASELEN], features[FEATLEN];
/*
  StreamPrintf(stream, "%s.%c%c/\n", stem, F_NOUN, F_ENGLISH);
*/
  StreamPrintf(stream, "%s.%c/", stem, F_SINGULAR);
  sprintf(features, "%c%c%c", F_SINGULAR, F_NOUN, F_ENGLISH);
  NewInflections = WordCreate(stem, features, NewInflections, EnglishIndex);
  AlgMorphEnglishVerb(stem, F_PRESENT, F_INDICATIVE, F_THIRD_PERSON,
                      F_SINGULAR, plural);
  if (plural[0]) {
    StreamPrintf(stream, "%s.%c/", plural, F_PLURAL);
    sprintf(features, "%c%c%c", F_PLURAL, F_NOUN, F_ENGLISH);
    NewInflections = WordCreate(plural, features, NewInflections, EnglishIndex);
  }
  StreamPrintf(stream, "\n|\n");
}

/* End of file. */
