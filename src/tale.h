/* tale.c */
void TA_LELAdd(LexEntry *le, char *features, int maxlen, LexEntry **le_le, char **le_feat, int *le_len);
void FeatSubstPOS(char *in, int newpos, char *out);
void TA_LexEntryEnter(Channel *ch, IndexEntry *ie, int prepunc, char *postpunc, char *phrase, char *begin, char *rest, HashTable *ht);
Bool TA_LexEntry1(char *phrase, char *postpunc, char *p, char *rest, int prev, HashTable *ht, Channel *ch, Discourse *dc, int nofail);
void TA_LexEntry(Channel *ch, Discourse *dc);
void TA_EndOfSentence(Channel *ch, Discourse *dc);
Bool TA_BlankLine(char *in, Channel *ch, char **nextp);
Bool TA_SeparatorLine(char *s, char **table_end);
