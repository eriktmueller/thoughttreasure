/* lexwf2.c */
Affix *AffixCreate(char *word, char *features, int lang, Obj *obj, Affix *next);
char *LexEntryRoot(LexEntry *le);
void Lex_WordForm2Init(void);
void Lex_WordForm2Enter(char *srcphrase, char *features, int pos, int lang, Obj *obj);
void Lex_WordForm2DoneLoading(void);
void Lex_WordForm2Derive4(HashTable *ht, int is_prefix, char *affix_word, char *affix_features, Obj *affix_obj, Obj *lhs_obj, char *lhs_word, char *lhs_feature, char *lhs_leo_features, LexEntry *rhs_le, char *rhs_word, int *found);
void Lex_WordForm2Derive3(HashTable *ht, int is_prefix, int maxlevel, int derive_depth, Affix *af, char *lhs_word, LexEntry *rhs_le, char *rhs_word, int *found);
Bool Lex_WordForm2EnglishAffixParse(int is_prefix, char *lhs_word0, char *affix_word, char *lhs_word);
void Lex_WordForm2Derive2(HashTable *ht, int is_prefix, int maxlevel, int derive_depth, Affix *af, char *lhs_word0, LexEntry *rhs_le, char *rhs_word, int *found);
IndexEntry *Lex_WordForm2Derive1A(HashTable *ht, char *rhs_word0, int maxlevel, int derive_depth, LexEntry *rhs_le, IndexEntry *ie_rest);
IndexEntry *Lex_WordForm2Derive1(HashTable *ht, char *rhs_word0, int maxlevel, int derive_depth);
IndexEntry *Lex_WordForm2Derive(HashTable *ht, char *rhs_word0, int maxlevel, int derive_depth);
int Lex_WordForm2Test(char *fn, HashTable *ht);
