/* lexmorph.c */
void MorphInit(Bool flag);
LexEntry *MorphInflectWord(char *word, int lang, int pos, char *known_features, HashTable *ht);
void MorphRestrictPOS(char *word, int lang, char *features);
Bool MorphIsProper(char *word);
Bool MorphIsAcronym(char *word);
Bool MorphIsWord(char *word);
Bool MorphIsInvariant(char *word, char *features, Obj *obj);
