/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

/* Global variables. */

extern int		MaxWordsInPhrase;
extern int		BaseRules[NUMPOS][NUMPOS][NUMPOS];
extern Bool		Debugging, TranslationOn, DoLoad;
extern Bool		LexEntryOff, LexEntryInsideName;
extern Bool		Starting, SaveTime, AnaMorphOn, GenOnAssert;
extern char     	*TTRoot;
extern char		*TT_Report_Dir;
extern FILE		*Log, *Out, *Display;
extern FILE		*StreamSuggEnglishInfl, *StreamSuggFrenchInfl;
extern FILE		*TranslateStream;
extern Ts		TsNA;
extern TsRange		TsRangeAlways;
extern Obj		*Me, *ObjWild, *ObjNA, *Objs, *DbgLastObj, *OBJDEFER;
extern ObjList		*OBJLISTDEFER, *OBJLISTRULEDOUT;
extern ObjList		*Sem_ParseResults;
extern Context		*ContextRoot;
extern HashTable	*FrenchIndex, *EnglishIndex, *SpellIndex;
extern LexEntry		*AllLexEntries;
extern Word		*NewInflections;
extern Discourse	*StdDiscourse;
extern Discourse	*ContextCurrentDc;
extern Corpus		*CorpusFrench;

/* End of file. */
