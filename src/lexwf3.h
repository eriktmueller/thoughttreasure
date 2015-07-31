/* lexwf3.c */
void WordFormAffixFindTrain1(HashTable *ht, int prefix, char *match_features, char *word, char *word_features, int minlen, int maxlen);
HashTable *WordFormAffixFindTrain(int prefix, Obj *class, char *features, int phrases_ok, int minlen, int maxlen);
Bool WordFormAffix(HashTable *ht, int prefix, char *word, int minlen, int maxlen);
void ReportWordFormAffixHeader(Rpt *rpt, Discourse *dc);
void ReportWordFormAffixLine(char *index, size_t count);
void ReportWordFormAffix(Text *text, Discourse *dc, HashTable *ht, Obj *affix_class);
