/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19940101: begun
 * 19940722: converted out of old Lists
 * 19950420: added expletives
 */

#include "tt.h"
#include "repobj.h"
#include "repbasic.h"
#include "synbase.h"
#include "utildbg.h"

void BaseRuleCompileA(FILE *stream);
void BaseRuleProcessList(Obj *obj);

/* todo: Should allocate dynamically? */
int BaseRules[NUMPOS][NUMPOS][NUMPOS];

int POSGI(char c)
{
  switch (c) {
    case F_NULL: return(0);
    case F_ADJECTIVE: return(1);
    case F_ADVERB: return(2);
    case F_ADVP: return(3);
    case F_DETERMINER: return(4);
    case F_NOUN: return(5);
    case F_PREPOSITION: return(6);
    case F_INTERJECTION: return(7);
    case F_VERB: return(8);
    case F_VP: return(9);
    case F_NP: return(10);
    case F_PP: return(11);
    case F_S: return(12);
    case F_PRONOUN: return(13);
    case F_CONJUNCTION: return(14);
    case F_ADJP: return(15);
    case F_S_POS: return(16);
    case F_ELEMENT: return(17);
    case F_EXPLETIVE: return(18);
  }
  fprintf(Log, "Unknown POSG <%c>\n", c);
  return(17);
}

char IPOSG(int i)
{
  switch (i) {
    case 0: return(F_NULL);
    case 1: return(F_ADJECTIVE);
    case 2: return(F_ADVERB);
    case 3: return(F_ADVP);
    case 4: return(F_DETERMINER);
    case 5: return(F_NOUN);
    case 6: return(F_PREPOSITION);
    case 7: return(F_INTERJECTION);
    case 8: return(F_VERB);
    case 9: return(F_VP);
    case 10: return(F_NP);
    case 11: return(F_PP);
    case 12: return(F_S);
    case 13: return(F_PRONOUN);
    case 14: return(F_CONJUNCTION);
    case 15: return(F_ADJP);
    case 16: return(F_S_POS);
    case 17: return(F_ELEMENT);
    case 18: return(F_EXPLETIVE);
  }
  return(F_NULL);
}

void BaseRuleInit()
{
  int i, j, k;
  for (i = 0; i < NUMPOS; i++) {
    for (j = 0; j < NUMPOS; j++) {
      for (k = 0; k < NUMPOS; k++) {
        BaseRules[i][j][k] = 0;
      }
    }
  }
}

void BaseRuleEnter(char a, char b, char z)
{
  BaseRules[POSGI(a)][POSGI(b)][POSGI(z)]++;
}

void BaseRulePrint(FILE *stream)
{
  int i, j, k, cnt;
  StreamSep(stream);
  fputs("Base rules:\n", stream);
  for (i = 0; i < NUMPOS; i++) {
    for (j = 0; j < NUMPOS; j++) {
      for (k = 0; k < NUMPOS; k++) {
        if (0 < (cnt = BaseRules[i][j][k])) {
          if (j != 0) {
            fprintf(stream, "%c -> %c %c  (%d)\n", IPOSG(k), IPOSG(i),
                    IPOSG(j), cnt);
          } else {
            fprintf(stream, "%c -> %c    (%d)\n", IPOSG(k), IPOSG(i), cnt);
          }
        }
      }
    }
  }
  StreamSep(stream);
}

void BaseRuleCompile()
{
  FILE *stream;
  BaseRuleInit();
  if (NULL == (stream = StreamOpenEnv("db/baserule.txt", "r"))) {
    return;
  }
  BaseRuleCompileA(stream);
  fclose(stream);
}

void BaseRuleCompileA(FILE *stream)
{
  Obj	*obj;
  while ((obj = ObjRead(stream))) {
    /*
    ObjPrettyPrint(Log, obj);
     */
    BaseRuleProcessList(obj);
    /* ObjFreeList(obj); */
  }
  BaseRulePrint(Log);
}

char BaseRuleGetFeature(Obj *obj)
{
  char	*s;
  if (!ObjIsList(obj)) return(F_NULL);
  s = ObjToName(I(obj, 0));
  if (s[0] && s[1]) return(s[1]);
  else return(F_NULL);
}

void BaseRuleProcessList(Obj *obj)
{
  int	i, len;
  char	*s, f0, f1, f2;
  Obj	*obj1;
  len = ObjLen(obj);
  if (len < 2) {
    Dbg(DBGGEN, DBGBAD, "BaseRuleProcessList 1");
    return;
  }
  if (len > 3) {
    Dbg(DBGGEN, DBGBAD, "BaseRuleProcessList 2");
    return;
  }
  if (!ObjIsSymbol(obj1 = I(obj, 0))) {
    Dbg(DBGGEN, DBGBAD, "BaseRuleProcessList 3");
    return;
  }
  s = ObjToName(obj1);
  if (s[0] && s[1]) {
    f0 = s[1];
  } else {
    Dbg(DBGGEN, DBGBAD, "BaseRuleProcessList 4");
    return;
  }
  f1 = BaseRuleGetFeature(I(obj, 1));
  if (f1 == F_NULL) return; /* We have hit a word. */
  if (len == 3) f2 = BaseRuleGetFeature(I(obj, 2));
  else f2 = F_NULL;
  BaseRuleEnter(f1, f2, f0);
  for (i = 1; i < len; i++) {
    BaseRuleProcessList(I(obj, i));
  }
}

/* End of file. */
