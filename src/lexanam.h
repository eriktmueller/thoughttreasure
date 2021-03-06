/* lexanam.c */
AnaMorphClassList *AnaMorphClassListCreate(AnaMorphClass *mc, AnaMorphClassList *next);
AnaMorphClassList *AnaMorphClassListGet(char *suffix);
void AnaMorphClassListSet(char *suffix, AnaMorphClassList *mcs);
void AnaMorphInit(void);
AnaMorphClass *AnaMorphClassCreate(int lang, int pos, int gender, LexEntry *le);
void AnaMorphClassFree(AnaMorphClass *mc);
void AnaMorphClassIndex1(char *suffix, AnaMorphClass *mc);
void AnaMorphClassIndex(AnaMorphClass *mc);
void ReportAnaMorphHeader(Rpt *rpt, Discourse *dc);
void ReportAnaMorphLine(Rpt *rpt, AnaMorphClass *mc, Discourse *dc);
void ReportAnaMorph1(Rpt *rpt, int countthresh, Discourse *dc);
void ReportAnaMorph(Text *text, Discourse *dc);
AnaMorphClass *AnaMorphClassFindEquiv(AnaMorphClass *mc);
void AnaMorphTrain2(AnaMorphClass *mc);
void AnaMorphTrain1(Word *infl, int stemlen, int lang, int pos, int gender, LexEntry *le);
void AnaMorphTrain(LexEntry *le);
LexEntry *AnaMorphInflectWord2(char *word, int lang, int pos, int gender, char *known_features, char *stem, char *suffix, AnaMorphClass *mc, char *features, int unique_gender, int mascfem, HashTable *ht);
LexEntry *AnaMorphInflectWord1(char *word, int lang, int pos, int gender, char *known_features, int stemlen, AnaMorphClassList *mcs, char *suffix, int mascfem, HashTable *ht);
LexEntry *AnaMorphInflectWord(char *word, int lang, int pos, char *known_features, HashTable *ht);
void AnaMorphTool(void);
