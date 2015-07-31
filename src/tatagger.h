/* tatagger.c */
TaggerWords *TaggerWordsCreate(void);
void TaggerWordsFree(TaggerWords *tw);
void TaggerWordsAdd(TaggerWords *tw, size_t position, int firstchar);
void TA_TaggerWriteSentence(FILE *stream, TaggerWords *tw, Channel *ch, size_t lowerb, size_t upperb, int eoschar);
int TA_TaggerWriteInput(char *fn, TaggerWords *tw, Channel *ch);
int TA_TaggedWordRead(FILE *stream, char *word, char *tag);
int PennIsPunct(char *tag);
int PennPOS(char *tag);
int PennTreebankTTCompat(char *tag, char *feat);
Float PennTreebankTTCompatNBest(char *nbest, char *tt);
void TA_TaggerPrune2(PNode *pn, PNode *prev, int dosplice, size_t pos, int firstchar, Channel *ch, char *word, char *tag, PNode **r_pn, PNode **r_prev, int *r_cnt, int *r_spliced);
int TA_TaggerPrune1(char *fn, TaggerWords *tw, Channel *ch);
void TA_TaggerPrune(Channel *ch);
