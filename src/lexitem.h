/* lexitem.c */
Lexitem *LexitemCreate(char *word, LexEntry *le, char *features);
Bool LexitemIsTrace(Lexitem *lexitem);
void LexitemMakeTrace(Lexitem *lexitem);
void LexitemPrint(FILE *stream, Lexitem *lexitem);
void LexitemSocketPrint(Socket *skt, Lexitem *li);
int LexitemInitialSound(Lexitem *lexitem, Discourse *dc);
LexitemList *LexitemListCreate(Lexitem *lexitem, LexitemList *next);
